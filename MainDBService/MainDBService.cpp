#include "MainDBService.h"
#include "Logger.h"

#include <fmt/format.h>


using namespace RabbitMQ;

MainDBService::MainDBService(std::shared_ptr<Configurations> configurations, std::shared_ptr<DbJobExecutor> executor)
	: configurations_(configurations)
	, executor_(executor)
	, consumer_(std::make_shared<WorkQueueConsume>(configurations_->rabbit_mq_host(), configurations_->rabbit_mq_port(), configurations_->rabbit_mq_user_name(), configurations_->rabbit_mq_password()))
{
}

auto MainDBService::start() -> std::tuple<bool, std::optional<std::string>>
{
	if (consumer_ == nullptr)
	{
		return { false, "Consumer is not initialized" };
	}
	
	auto [success, error] = consume_queue();
	if (!success)
	{
		return { false, error };
	}

	return { true, std::nullopt };
}

auto MainDBService::wait_stop() -> std::tuple<bool, std::optional<std::string>>
{
	if (consumer_ == nullptr)
	{
		return { false, "Consumer is not initialized" };
	}

	consumer_->wait_stop();

	return { true, std::nullopt };
}

auto MainDBService::stop() -> void
{
	if (consumer_ != nullptr)
	{
		consumer_->stop();
		consumer_.reset();
	}
}

auto MainDBService::consume_queue() -> std::tuple<bool, std::optional<std::string>>
{
	auto [started, start_err] = consumer_->start();
	if (!started)
	{
		return { false, start_err };
	}

	auto [connected, conn_err] = consumer_->connect(configurations_->rabbit_heartbeat());
	if (!connected)
	{
		return { false, conn_err };
	}

	{
		std::optional<uint32_t> ttl_ms = std::nullopt;
		if (configurations_->message_ttl_ms().has_value() && configurations_->message_ttl_ms().value() > 0)
		{
			ttl_ms = static_cast<uint32_t>(configurations_->message_ttl_ms().value());
		}
		consumer_->set_queue_policies(configurations_->dlx_exchange(), configurations_->dlx_routing_key(), ttl_ms);
	}

	auto [opened, open_err] = consumer_->channel_open(configurations_->rabbit_channel_id(), configurations_->consume_queue_name());
	if (!opened.has_value())
	{
		return { false, open_err };
	}

	auto [prepared, prep_err] = consumer_->prepare_consume();
	if (!prepared)
	{
		return { false, prep_err };
	}

	consumer_->set_requeue_on_failure(configurations_->requeue_on_failure());

	auto callback = [this](const std::string&, const std::string& body, const std::string& content_type) -> std::tuple<bool, std::optional<std::string>>
	{
		if (content_type.rfind("application/json", 0) != 0)
		{
			return { false, std::optional<std::string>("unsupported content-type: " + content_type) };
		}
		// TODO
		// DATA(JSON) Validation

		return executor_->handle_message(body);
	};

	auto [registered, registered_error] = consumer_->register_consume(configurations_->rabbit_channel_id(), configurations_->consume_queue_name(), callback);
	if (!registered)
	{
		return { false, registered_error };
	}

	return consumer_->start_consume();
}
