#include "RabbitMQConsumer.h"

namespace CommonMessageMQ
{
	RabbitMQConsumer::RabbitMQConsumer(const std::string& host,
									   int port,
									   const std::string& user_name,
									   const std::string& password,
									   const SSLOptions& ssl_options,
									   int channel_id,
									   int heartbeat)
		: channel_id_(channel_id)
		, heartbeat_(heartbeat)
		, worker_(nullptr)
	{
		worker_ = std::make_unique<RabbitMQWorkQueueConsume>(host, port, user_name, password, ssl_options);
	}

	RabbitMQConsumer::~RabbitMQConsumer()
	{
		if (worker_ != nullptr)
		{
			worker_.reset();
		}
	}

	auto RabbitMQConsumer::start() -> std::tuple<bool, std::optional<std::string>>
	{
		if (worker_ == nullptr)
		{
			return { false, std::optional<std::string>("RabbitMQ worker is nullptr") };
		}

		return worker_->connect(heartbeat_);
	}

	auto RabbitMQConsumer::wait_stop() -> std::tuple<bool, std::optional<std::string>>
	{
		if (worker_ == nullptr)
		{
			return { false, std::optional<std::string>("RabbitMQ worker is nullptr") };
		}

		// CppToolkit RabbitMQBase doesn't have wait_stop, just return success
		return { true, std::nullopt };
	}

	auto RabbitMQConsumer::stop() -> std::tuple<bool, std::optional<std::string>>
	{
		if (worker_ == nullptr)
		{
			return { false, std::optional<std::string>("RabbitMQ worker is nullptr") };
		}

		worker_->stop_consume();
		worker_->channel_close();
		worker_->disconnect();

		return { true, std::nullopt };
	}

	auto RabbitMQConsumer::subscribe(const std::string& queue_name,
									 const std::string& consumer_group,
									 const std::function<std::tuple<bool, std::optional<std::string>>(
										 const std::string&, const std::string&, const std::string&)>& task_callback,
									 const std::function<std::tuple<bool, std::optional<std::string>>(
										 const std::string&, const std::string&, const std::string&)>& expired_callback)
		-> std::tuple<bool, std::optional<std::string>>
	{
		if (worker_ == nullptr)
		{
			return { false, std::optional<std::string>("RabbitMQ worker is nullptr") };
		}

		// RabbitMQ doesn't use consumer_group in the same way as Redis
		// The queue_name itself acts as the consumer group mechanism

		auto [declred_name, error] = worker_->channel_open(channel_id_, queue_name);
		if (error.has_value())
		{
			return { false, error };
		}

		auto [success1, error1] = worker_->prepare_consume();
		if (!success1)
		{
			return { false, error1 };
		}

		auto [success2, error2] = worker_->register_consume(channel_id_, declred_name.value(), task_callback);
		if (!success2)
		{
			return { false, error2 };
		}

		return worker_->start_consume();
	}
}
