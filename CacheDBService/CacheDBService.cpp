#include "CacheDBService.h"

#include "Logger.h"
#include "JobPool.h"

#include "fmt/format.h"
#include <algorithm>
#include <chrono>
#include <thread>
#include <utility>
#include <future>

#include "boost/json.hpp"
#include "boost/json/parse.hpp"

using namespace Utilities;

CacheDBService::CacheDBService(std::shared_ptr<Configurations> configurations)
    : configurations_(std::move(configurations))
    , redis_client_(nullptr)
    , work_queue_emitter_(nullptr)
    , thread_pool_(nullptr)
{
}

CacheDBService::~CacheDBService()
{
	stop();
}

auto CacheDBService::start() -> std::tuple<bool, std::optional<std::string>>
{
    stop_promise_ = std::promise<void>();
    stop_future_ = stop_promise_.get_future().share();

	{
		std::lock_guard<std::mutex> lock(pending_mutex_);
		pending_messages_.clear();
	}

	if (redis_client_ == nullptr)
	{
		redis_client_ = std::make_unique<Redis::RedisClient>(
			configurations_->redis_host(),
			configurations_->redis_port(),
			Redis::TLSOptions(),
			configurations_->redis_db_index());
	}

	auto [connected, connect_error] = ensure_redis_connection();
	if (!connected)
	{
		return { false, connect_error };
	}

	if (work_queue_emitter_ == nullptr)
	{
		work_queue_emitter_ = std::make_unique<RabbitMQ::WorkQueueEmitter>(
			configurations_->rabbit_mq_host(),
			configurations_->rabbit_mq_port(),
			configurations_->rabbit_mq_user_name(),
			configurations_->rabbit_mq_password());
	}

	auto [mq_connected, mq_connect_error] = ensure_rabbitmq_connection();
	if (!mq_connected)
	{
		return { false, mq_connect_error };
	}

	auto [pool_created, pool_error] = create_thread_pool();
	if (!pool_created)
	{
		return { false, pool_error };
	}

	// Kick first cycle; subsequent cycles re-enqueue themselves via job pool
	schedule_publish_job();

	return { true, std::nullopt };
}

auto CacheDBService::wait_stop() -> std::tuple<bool, std::optional<std::string>>
{
    if (!stop_future_.valid())
    {
        return { false, std::optional<std::string>("service is not running") };
    }
    stop_future_.wait();
    stop_future_ = std::shared_future<void>();
    return { true, std::nullopt };
}

auto CacheDBService::stop() -> std::tuple<bool, std::optional<std::string>>
{
    destroy_thread_pool();
    if (stop_future_.valid())
    {
        try
        {
            stop_promise_.set_value();
        }
        catch (const std::future_error&)
        {
            // already satisfied
        }
    }
    stop_future_ = std::shared_future<void>();
    return { true, std::nullopt };
}

auto CacheDBService::create_thread_pool() -> std::tuple<bool, std::optional<std::string>>
{
	destroy_thread_pool();

	try
	{
		thread_pool_ = std::make_shared<ThreadPool>();
	}
	catch (const std::bad_alloc& e)
	{
		return { false, fmt::format("Memory allocation failed to ThreadPool: {}", e.what()) };
	}

	auto allocate_workers = [&](int count, const std::vector<JobPriorities>& priorities) -> std::tuple<bool, std::optional<std::string>>
	{
		int effective_count = std::max(0, count);
		for (int idx = 0; idx < effective_count; ++idx)
		{
			try
			{
				auto worker = std::make_shared<ThreadWorker>(priorities);
				thread_pool_->push(worker);
			}
			catch (const std::bad_alloc& e)
			{
				return { false, fmt::format("Memory allocation failed to ThreadWorker: {}", e.what()) };
			}
		}
		return { true, std::nullopt };
	};

	auto [high_ok, high_err] = allocate_workers(configurations_->high_priority_worker_count(), std::vector<JobPriorities>{ JobPriorities::High });
	if (!high_ok)
	{
		return { false, high_err };
	}

	auto [normal_ok, normal_err] = allocate_workers(configurations_->normal_priority_worker_count(), std::vector<JobPriorities>{ JobPriorities::Normal, JobPriorities::High });
	if (!normal_ok)
	{
		return { false, normal_err };
	}

	auto [low_ok, low_err] = allocate_workers(configurations_->low_priority_worker_count(), std::vector<JobPriorities>{ JobPriorities::Low });
	if (!low_ok)
	{
		return { false, low_err };
	}

	// Ensure at least one long-term worker for scheduled jobs
	auto [long_ok, long_err] = allocate_workers(1, std::vector<JobPriorities>{ JobPriorities::LongTerm });
	if (!long_ok)
	{
		return { false, long_err };
	}

	auto [result, message] = thread_pool_->start();
	if (!result)
	{
		return { false, message };
	}

	return { true, std::nullopt };
}

auto CacheDBService::destroy_thread_pool() -> void
{
	if (thread_pool_ == nullptr)
	{
		return;
	}
	thread_pool_->stop();
	thread_pool_.reset();
}

auto CacheDBService::ensure_stream_group() -> std::tuple<bool, std::optional<std::string>>
{
	// No consumption required currently; return success
	return { true, std::nullopt };
}

auto CacheDBService::ensure_redis_connection() -> std::tuple<bool, std::optional<std::string>>
{
	if (redis_client_ == nullptr)
	{
		return { false, std::optional<std::string>("Redis client is null") };
	}

	if (redis_client_->is_connected())
	{
		return { true, std::nullopt };
	}

	int max_retries = configurations_->redis_reconnect_max_retries();
	int interval_ms = configurations_->redis_reconnect_interval_ms();

	for (int retry = 0; retry < max_retries; ++retry)
	{
		auto [connected, connect_error] = redis_client_->connect();
		if (connected)
		{
			Logger::handle().write(LogTypes::Information,
				fmt::format("Redis reconnected successfully after {} retries", retry));
			return { true, std::nullopt };
		}

		Logger::handle().write(LogTypes::Warning,
			fmt::format("Redis connection failed (retry {}/{}): {}",
				retry + 1, max_retries, connect_error.value_or("unknown error")));

		if (retry < max_retries - 1)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms));
		}
	}

	return { false, std::optional<std::string>(
		fmt::format("Failed to reconnect to Redis after {} retries", max_retries)) };
}

auto CacheDBService::ensure_rabbitmq_connection() -> std::tuple<bool, std::optional<std::string>>
{
	if (work_queue_emitter_ == nullptr)
	{
		return { false, std::optional<std::string>("WorkQueueEmitter is null") };
	}

	int max_retries = configurations_->rabbit_mq_reconnect_max_retries();
	int interval_ms = configurations_->rabbit_mq_reconnect_interval_ms();

	for (int retry = 0; retry < max_retries; ++retry)
	{
		auto [started, start_error] = work_queue_emitter_->start();
		if (started)
		{
			if (retry > 0)
			{
				Logger::handle().write(LogTypes::Information,
					fmt::format("RabbitMQ reconnected successfully after {} retries", retry));
			}
			return { true, std::nullopt };
		}

		Logger::handle().write(LogTypes::Warning,
			fmt::format("RabbitMQ connection failed (retry {}/{}): {}",
				retry + 1, max_retries, start_error.value_or("unknown error")));

		if (retry < max_retries - 1)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms));
		}
	}

	return { false, std::optional<std::string>(
		fmt::format("Failed to connect to RabbitMQ after {} retries", max_retries)) };
}

auto CacheDBService::publish_message(const std::string& message_body) -> std::tuple<bool, std::optional<std::string>>
{
	if (work_queue_emitter_ == nullptr)
	{
		return { false, std::optional<std::string>("WorkQueueEmitter is null") };
	}

	auto [success, error_message] = work_queue_emitter_->publish(
		configurations_->rabbit_channel_id(),
		configurations_->publish_queue_name(),
		message_body,
		configurations_->content_type(),
		std::nullopt);

	// If operation failed due to connection issue, try to reconnect and retry
	if (!success && error_message.has_value() &&
		(error_message.value().find("connection") != std::string::npos ||
		 error_message.value().find("socket") != std::string::npos ||
		 error_message.value().find("channel") != std::string::npos))
	{
		Logger::handle().write(LogTypes::Warning,
			fmt::format("RabbitMQ publish failed, attempting reconnection: {}", error_message.value()));

		auto [reconnected, reconnect_error] = ensure_rabbitmq_connection();
		if (reconnected)
		{
			return work_queue_emitter_->publish(
				configurations_->rabbit_channel_id(),
				configurations_->publish_queue_name(),
				message_body,
				configurations_->content_type(),
				std::nullopt);
		}
		return { false, reconnect_error };
	}

	return { success, error_message };
}

auto CacheDBService::set_key_value(const std::string& key, const std::string& value, long ttl_seconds) -> std::tuple<bool, std::optional<std::string>>
{
	auto [connected, connect_error] = ensure_redis_connection();
	if (!connected)
	{
		return { false, connect_error };
	}

	auto [success, error_message] = redis_client_->set(key, value, ttl_seconds);

	// If operation failed due to connection issue, try one more time after reconnection
	if (!success && error_message.has_value() &&
		(error_message.value().find("connection") != std::string::npos ||
		 error_message.value().find("timeout") != std::string::npos))
	{
		Logger::handle().write(LogTypes::Warning,
			fmt::format("Redis SET operation failed, attempting reconnection: {}", error_message.value()));

		auto [reconnected, reconnect_error] = ensure_redis_connection();
		if (reconnected)
		{
			return redis_client_->set(key, value, ttl_seconds);
		}
		return { false, reconnect_error };
	}

	return { success, error_message };
}

auto CacheDBService::get_key_value(const std::string& key) -> std::tuple<std::optional<std::string>, std::optional<std::string>>
{
	auto [connected, connect_error] = ensure_redis_connection();
	if (!connected)
	{
		return { std::nullopt, connect_error };
	}

	auto [value, error_message] = redis_client_->get(key);

	// If operation failed due to connection issue, try one more time after reconnection
	if (!value.has_value() && error_message.has_value() &&
		(error_message.value().find("connection") != std::string::npos ||
		 error_message.value().find("timeout") != std::string::npos))
	{
		Logger::handle().write(LogTypes::Warning,
			fmt::format("Redis GET operation failed, attempting reconnection: {}", error_message.value()));

		auto [reconnected, reconnect_error] = ensure_redis_connection();
		if (reconnected)
		{
			return redis_client_->get(key);
		}
		return { std::nullopt, reconnect_error };
	}

	return { value, error_message };
}

auto CacheDBService::enqueue_database_operation(const std::string& json_body) -> std::tuple<bool, std::optional<std::string>>
{
	// Basic JSON validation
	try
	{
			auto json_value = boost::json::parse(json_body);
			if (!json_value.is_object())
		{
			return { false, std::optional<std::string>("message is not a JSON object") };
		}
	}
	catch (const std::exception& e)
	{
		return { false, std::optional<std::string>(std::string("invalid JSON: ") + e.what()) };
	}

	{
		std::lock_guard<std::mutex> lock(pending_mutex_);
		pending_messages_.push_back(PendingMessage{ json_body });
	}
	return { true, std::nullopt };
}

void CacheDBService::schedule_publish_job()
{
	if (thread_pool_ == nullptr || is_stop_requested())
	{
		return;
	}
	auto [queued, queue_error] = thread_pool_->push(std::make_shared<Job>(
		JobPriorities::LongTerm,
		std::bind(&CacheDBService::publish_to_main_db_service, this),
		"publish_to_main_db_service"));
	if (!queued)
	{
		Logger::handle().write(LogTypes::Error, queue_error.value_or("failed to schedule publish job"));
	}
}

auto CacheDBService::publish_to_main_db_service() -> std::tuple<bool, std::optional<std::string>>
{
	if (thread_pool_ == nullptr)
	{
		return { false, std::optional<std::string>("thread_pool is null") };
	}

	if (is_stop_requested())
	{
		return { true, std::nullopt };
	}

	const auto wait_interval = std::chrono::milliseconds(configurations_->publish_to_main_db_service_interval_ms());
	const auto wake_slice = std::chrono::milliseconds(100);
	auto deadline = std::chrono::steady_clock::now() + wait_interval;
	while (!is_stop_requested() && std::chrono::steady_clock::now() < deadline)
	{
		auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - std::chrono::steady_clock::now());
		auto sleep_span = remaining > wake_slice ? wake_slice : remaining;
		if (sleep_span.count() > 0)
		{
			std::this_thread::sleep_for(sleep_span);
		}
	}

	if (is_stop_requested())
	{
		return { true, std::nullopt };
	}

	std::vector<PendingMessage> messages_to_flush;
	{
		std::lock_guard<std::mutex> lock(pending_mutex_);
		messages_to_flush.swap(pending_messages_);
	}

	for (const auto& pending_message : messages_to_flush)
	{
		if (is_stop_requested())
		{
			return { true, std::nullopt };
		}

		auto [publish_success, publish_error] = publish_message(pending_message.message_body);
		if (!publish_success)
		{
			Logger::handle().write(LogTypes::Error, fmt::format("Failed to publish message: {}", publish_error.value_or("unknown error")));
			std::lock_guard<std::mutex> lock(pending_mutex_);
			pending_messages_.push_back(pending_message);
		}
	}

	auto job_pool = thread_pool_->job_pool();
	if (job_pool == nullptr || job_pool->lock())
	{
		return { false, std::optional<std::string>("job_pool is null or locked") };
	}

	if (!is_stop_requested())
	{
		auto [queued, queue_error] = job_pool->push(
			std::make_shared<Job>(JobPriorities::LongTerm, std::bind(&CacheDBService::publish_to_main_db_service, this), "publish_to_main_db_service"));
		if (!queued)
		{
			return { false, queue_error };
		}
	}

	return { true, std::nullopt };
}

auto CacheDBService::is_stop_requested() const -> bool
{
    if (!stop_future_.valid())
    {
        return false;
    }
    return stop_future_.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
}
