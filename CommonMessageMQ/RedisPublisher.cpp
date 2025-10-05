#include "RedisPublisher.h"

namespace CommonMessageMQ
{
	RedisPublisher::RedisPublisher(const std::string& address,
								   int port,
								   const TLSOptions& tls_options,
								   int db_index)
		: emitter_(nullptr)
	{
		emitter_ = std::make_unique<RedisQueueEmitter>(address, port, tls_options, db_index);
	}

	RedisPublisher::~RedisPublisher()
	{
		if (emitter_ != nullptr)
		{
			emitter_->stop();
			emitter_.reset();
		}
	}

	auto RedisPublisher::start() -> std::tuple<bool, std::optional<std::string>>
	{
		if (emitter_ == nullptr)
		{
			return { false, std::optional<std::string>("Redis emitter is nullptr") };
		}

		return emitter_->start();
	}

	auto RedisPublisher::stop() -> std::tuple<bool, std::optional<std::string>>
	{
		if (emitter_ == nullptr)
		{
			return { false, std::optional<std::string>("Redis emitter is nullptr") };
		}

		return emitter_->stop();
	}

	auto RedisPublisher::publish(const std::string& queue_name,
								 const std::string& message,
								 const std::string& message_type,
								 const std::optional<long>& ttl_seconds)
		-> std::tuple<bool, std::optional<std::string>>
	{
		if (emitter_ == nullptr)
		{
			return { false, std::optional<std::string>("Redis emitter is nullptr") };
		}

		return emitter_->publish(queue_name, message, message_type, ttl_seconds);
	}

	auto RedisPublisher::publish_async(const std::string& queue_name,
									   const std::string& message,
									   const std::string& message_type,
									   const std::optional<long>& ttl_seconds)
		-> std::tuple<bool, std::optional<std::string>>
	{
		if (emitter_ == nullptr)
		{
			return { false, std::optional<std::string>("Redis emitter is nullptr") };
		}

		return emitter_->publish_async(queue_name, message, message_type, ttl_seconds);
	}
}
