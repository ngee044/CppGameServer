#include "MainDBService.h"

#include <csignal>
#include <chrono>
#include <thread>

using namespace RabbitMQ;

MainDBService::MainDBService(const Configurations& cfg, DbJobExecutor& executor)
	: cfg_(cfg)
	, executor_(executor)
	, consumer_(cfg.rabbit_mq_host(), cfg.rabbit_mq_port(), cfg.rabbit_mq_user_name(), cfg.rabbit_mq_password())
	, stop_flag_(false)
{
}

auto MainDBService::start() -> std::tuple<bool, std::optional<std::string>>
{
	auto [started, start_err] = consumer_.start();
	if (!started)
	{
		return { false, start_err };
	}

	auto [connected, conn_err] = consumer_.connect(cfg_.rabbit_heartbeat());
	if (!connected)
	{
		return { false, conn_err };
	}

	// Apply queue policies before channel open/declare
	{
		std::optional<uint32_t> ttl_ms = std::nullopt;
		if (cfg_.message_ttl_ms().has_value() && cfg_.message_ttl_ms().value() > 0)
		{
			ttl_ms = static_cast<uint32_t>(cfg_.message_ttl_ms().value());
		}
		consumer_.set_queue_policies(cfg_.dlx_exchange(), cfg_.dlx_routing_key(), ttl_ms);
	}

	auto [opened, open_err] = consumer_.channel_open(cfg_.rabbit_channel_id(), cfg_.consume_queue_name());
	if (!opened.has_value())
	{
		return { false, open_err };
	}

	auto [prepared, prep_err] = consumer_.prepare_consume();
	if (!prepared)
	{
		return { false, prep_err };
	}

	// Apply failure handling policy (ack/nack requeue behavior)
	consumer_.set_requeue_on_failure(cfg_.requeue_on_failure());

	auto cb = [this](const std::string&, const std::string& body, const std::string& content_type) -> std::tuple<bool, std::optional<std::string>>
	{
		// Enforce JSON payloads for safety (allow parameters like charset)
		if (content_type.rfind("application/json", 0) != 0)
		{
			return { false, std::optional<std::string>("unsupported content-type: " + content_type) };
		}
		return executor_.handle_message(body);
	};
	auto [registered, reg_err] = consumer_.register_consume(cfg_.rabbit_channel_id(), cfg_.consume_queue_name(), cb);
	if (!registered)
	{
		return { false, reg_err };
	}

	return consumer_.start_consume();
}

void MainDBService::run_until_signal()
{
	std::signal(SIGINT, &MainDBService::on_signal);
	std::signal(SIGTERM, &MainDBService::on_signal);

	while (!stop_flag_.load())
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(200));
	}
}

auto MainDBService::stop() -> void
{
	consumer_.stop_consume();
	consumer_.channel_close();
	consumer_.disconnect();
}

void MainDBService::on_signal(int)
{
	instance()->stop_flag_.store(true);
}

auto MainDBService::instance(MainDBService* inst) -> MainDBService*
{
	static MainDBService* self = nullptr;
	if (inst != nullptr)
	{
		self = inst;
	}
	return self;
}

