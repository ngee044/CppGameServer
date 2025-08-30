#include "CacheDBService.h"

#include "Logger.h"

#include "fmt/format.h"
#include <chrono>
#include <thread>

#include "boost/json.hpp"
#include "boost/json/parse.hpp"

using namespace Utilities;

CacheDBService::CacheDBService(const Configurations& cfg)
	: cfg_(cfg)
	, redis_(cfg.redis_host(), cfg.redis_port(), Redis::TLSOptions(), cfg.redis_db_index())
	, emitter_(cfg.rabbit_mq_host(), cfg.rabbit_mq_port(), cfg.rabbit_mq_user_name(), cfg.rabbit_mq_password())
	, stop_flag_(false)
{
}

CacheDBService::~CacheDBService() { stop(); }

auto CacheDBService::start() -> std::tuple<bool, std::optional<std::string>>
{
	stop_flag_.store(false);

	// Ensure stream group exists (best-effort)
	ensure_stream_group();

	// Create thread pool and worker
	thread_pool_ = std::make_shared<Thread::ThreadPool>("CacheDBServiceThreadPool");
	{
		auto [started, err] = thread_pool_->start();
		if (!started)
		{
			return { false, err };
		}
	}
	thread_pool_->push(std::make_shared<Thread::ThreadWorker>(
		std::vector<Thread::JobPriorities>{ Thread::JobPriorities::LongTerm },
		"CacheDBServiceWorker"));

	// Kick first cycle; subsequent cycles re-enqueue themselves via std::bind
	schedule_flush_job();

	return { true, std::nullopt };
}

auto CacheDBService::wait_stop() -> std::tuple<bool, std::optional<std::string>>
{
	// Simple wait loop; external stop() should be called by operator
	while (!stop_flag_.load())
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(200));
	}
	return { true, std::nullopt };
}

auto CacheDBService::stop() -> std::tuple<bool, std::optional<std::string>>
{
	stop_flag_.store(true);
	if (thread_pool_ != nullptr)
	{
		thread_pool_->remove_workers(Thread::JobPriorities::LongTerm);
		thread_pool_->stop(true);
		thread_pool_.reset();
	}
	return { true, std::nullopt };
}

auto CacheDBService::ensure_stream_group() -> std::tuple<bool, std::optional<std::string>>
{
	if (!cfg_.redis_auto_create_group())
	{
		return { true, std::nullopt };
	}

	auto [ok, err] = redis_.xgroup_create(cfg_.redis_stream_key(), cfg_.redis_group_name(), "$", true);
	if (!ok && err.has_value())
	{
		// 그룹이 이미 있으면 에러가 날 수 있으므로, 존재 에러는 무시
		Logger::handle().write(LogTypes::Debug, fmt::format("xgroup_create: {}", err.value()));
	}
	return { true, std::nullopt };
}

auto CacheDBService::publish_json(const std::string& body) -> std::tuple<bool, std::optional<std::string>>
{
	return emitter_.publish(cfg_.rabbit_channel_id(), cfg_.publish_queue_name(), body, cfg_.content_type());
}

void CacheDBService::consume_loop() {}

void CacheDBService::schedule_flush_job()
{
	if (thread_pool_ == nullptr)
	{
		return;
	}
	thread_pool_->push(std::make_shared<Thread::Job>(
		Thread::JobPriorities::LongTerm,
		std::bind(&CacheDBService::flush_cycle, this),
		"CacheDBService::flush_cycle",
		false));
}

auto CacheDBService::flush_cycle() -> std::tuple<bool, std::optional<std::string>>
{
	using clock = std::chrono::steady_clock;
	auto interval = std::chrono::milliseconds(cfg_.cache_db_service_to_publish_flush_interval_ms());
	auto end_time = clock::now() + interval;

	while (!stop_flag_.load())
	{
		long remain_ms = static_cast<long>(std::chrono::duration_cast<std::chrono::milliseconds>(end_time - clock::now()).count());
		if (remain_ms <= 0)
		{
			break;
		}
		long block_ms = std::max(0L, std::min(static_cast<long>(cfg_.redis_block_ms()), remain_ms));
		auto [streams, err] = redis_.xreadgroup(
			cfg_.redis_group_name(),
			cfg_.redis_consumer_name(),
			std::vector<std::string>{ cfg_.redis_stream_key() },
			std::vector<std::string>{ ">" },
			cfg_.redis_count(),
			block_ms);

		if (err.has_value())
		{
			Logger::handle().write(LogTypes::Error, fmt::format("xreadgroup error: {}", err.value()));
			continue;
		}

		for (auto& stream : streams)
		{
			const auto& key = stream.first;
			for (auto& entry : stream.second)
			{
				const auto& id = entry.first;
				const auto& fields = entry.second;
				std::string body;
				auto it = fields.find("message");
				if (it != fields.end())
				{
					body = it->second;
				}
				else
				{
					boost::json::object o;
					for (const auto& kv : fields)
					{
						o[kv.first] = kv.second;
					}
					body = boost::json::serialize(o);
				}

				{
					std::lock_guard<std::mutex> lk(pending_mutex_);
					pending_.push_back(Pending{ key, id, body });
				}
			}
		}
	}

	// Flush
	std::vector<Pending> to_flush;
	{
		std::lock_guard<std::mutex> lk(pending_mutex_);
		to_flush.swap(pending_);
	}
	for (const auto& p : to_flush)
	{
		auto [pub_ok, pub_err] = publish_json(p.body);
		if (!pub_ok)
		{
			Logger::handle().write(LogTypes::Error, fmt::format("publish failed: {}", pub_err.value_or("unknown")));
			continue; // leave unacked
		}
		auto [acked, ack_err] = redis_.xack(p.key, cfg_.redis_group_name(), std::vector<std::string>{ p.id });
		if (ack_err.has_value())
		{
			Logger::handle().write(LogTypes::Error, fmt::format("xack failed: {}", ack_err.value()));
		}
	}

	// Re-enqueue next cycle if not stopping
	if (!stop_flag_.load() && thread_pool_ != nullptr)
	{
		schedule_flush_job();
	}

	return { true, std::nullopt };
}
