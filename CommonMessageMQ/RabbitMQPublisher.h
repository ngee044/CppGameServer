#pragma once

#include "MessageQueuePublisher.h"
#include "RabbitMQQueueEmitter.h"

#include <memory>

using namespace RabbitMQ;

namespace CommonMessageMQ
{
	class RabbitMQPublisher : public MessageQueuePublisher
	{
	public:
		RabbitMQPublisher(const std::string& host,
						  int port,
						  const std::string& user_name,
						  const std::string& password,
						  const SSLOptions& ssl_options = SSLOptions(),
						  int channel_id = 1);
		virtual ~RabbitMQPublisher();

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
		int channel_id_;
		std::unique_ptr<RabbitMQQueueEmitter> emitter_;
	};
}
