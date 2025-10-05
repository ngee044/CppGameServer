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
		worker_ = std::make_unique<RabbitMQQueueWorker>(host, port, user_name, password, ssl_options);
	}

	RabbitMQConsumer::~RabbitMQConsumer()
	{
		if (worker_ != nullptr)
		{
			worker_->stop();
			worker_.reset();
		}
	}

	auto RabbitMQConsumer::start() -> std::tuple<bool, std::optional<std::string>>
	{
		if (worker_ == nullptr)
		{
			return { false, std::optional<std::string>("RabbitMQ worker is nullptr") };
		}

		return worker_->start(heartbeat_);
	}

	auto RabbitMQConsumer::wait_stop() -> std::tuple<bool, std::optional<std::string>>
	{
		if (worker_ == nullptr)
		{
			return { false, std::optional<std::string>("RabbitMQ worker is nullptr") };
		}

		return worker_->wait_stop();
	}

	auto RabbitMQConsumer::stop() -> std::tuple<bool, std::optional<std::string>>
	{
		if (worker_ == nullptr)
		{
			return { false, std::optional<std::string>("RabbitMQ worker is nullptr") };
		}

		return worker_->stop();
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
		return worker_->subscribe(channel_id_, queue_name, task_callback, expired_callback);
	}
}
