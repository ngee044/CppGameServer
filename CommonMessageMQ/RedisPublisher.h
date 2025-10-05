#pragma once

#include "MessageQueuePublisher.h"
#include "RedisQueueEmitter.h"

#include <memory>

using namespace Redis;

namespace CommonMessageMQ
{
	class RedisPublisher : public MessageQueuePublisher
	{
	public:
		RedisPublisher(const std::string& address,
					   int port = 6379,
					   const TLSOptions& tls_options = TLSOptions(),
					   int db_index = 0);
		virtual ~RedisPublisher();

		auto start() -> std::tuple<bool, std::optional<std::string>> override;
		auto stop() -> std::tuple<bool, std::optional<std::string>> override;

		auto publish(const std::string& queue_name,
					 const std::string& message,
					 const std::string& message_type,
					 const std::optional<long>& ttl_seconds = std::nullopt)
			-> std::tuple<bool, std::optional<std::string>> override;

		auto publish_async(const std::string& queue_name,
						   const std::string& message,
						   const std::string& message_type,
						   const std::optional<long>& ttl_seconds = std::nullopt)
			-> std::tuple<bool, std::optional<std::string>> override;

	private:
		std::unique_ptr<RedisQueueEmitter> emitter_;
	};
}
