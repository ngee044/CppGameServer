#include "RabbitMQPublisher.h"

namespace CommonMessageMQ
{
	RabbitMQPublisher::RabbitMQPublisher(const std::string& host,
										 int port,
										 const std::string& user_name,
										 const std::string& password,
										 const SSLOptions& ssl_options,
										 int channel_id)
		: channel_id_(channel_id)
		, emitter_(nullptr)
	{
		emitter_ = std::make_unique<RabbitMQQueueEmitter>(host, port, user_name, password, ssl_options);
	}

	RabbitMQPublisher::~RabbitMQPublisher()
	{
		if (emitter_ != nullptr)
		{
			emitter_->stop();
			emitter_.reset();
		}
	}

	auto RabbitMQPublisher::start() -> std::tuple<bool, std::optional<std::string>>
	{
		if (emitter_ == nullptr)
		{
			return { false, std::optional<std::string>("RabbitMQ emitter is nullptr") };
		}

		return emitter_->start();
	}

	auto RabbitMQPublisher::stop() -> std::tuple<bool, std::optional<std::string>>
	{
		if (emitter_ == nullptr)
		{
			return { false, std::optional<std::string>("RabbitMQ emitter is nullptr") };
		}

		return emitter_->stop();
	}

	auto RabbitMQPublisher::publish(const std::string& queue_name,
									const std::string& message,
									const std::string& message_type,
									const std::optional<long>& ttl_seconds)
		-> std::tuple<bool, std::optional<std::string>>
	{
		if (emitter_ == nullptr)
		{
			return { false, std::optional<std::string>("RabbitMQ emitter is nullptr") };
		}

		std::optional<uint32_t> ttl_milliseconds = std::nullopt;
		if (ttl_seconds.has_value())
		{
			ttl_milliseconds = static_cast<uint32_t>(ttl_seconds.value() * 1000);
		}

		return emitter_->publish(channel_id_, queue_name, message, message_type, ttl_milliseconds);
	}

	auto RabbitMQPublisher::publish_async(const std::string& queue_name,
										  const std::string& message,
										  const std::string& message_type,
										  const std::optional<long>& ttl_seconds)
		-> std::tuple<bool, std::optional<std::string>>
	{
		if (emitter_ == nullptr)
		{
			return { false, std::optional<std::string>("RabbitMQ emitter is nullptr") };
		}

		std::optional<uint32_t> ttl_milliseconds = std::nullopt;
		if (ttl_seconds.has_value())
		{
			ttl_milliseconds = static_cast<uint32_t>(ttl_seconds.value() * 1000);
		}

		return emitter_->publish_async(channel_id_, queue_name, message, message_type, ttl_milliseconds);
	}
}
