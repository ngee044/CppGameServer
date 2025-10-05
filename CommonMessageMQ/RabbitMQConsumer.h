#pragma once

#include "MessageQueueConsumer.h"
#include "RabbitMQQueueWorker.h"

#include <memory>

using namespace RabbitMQ;

namespace CommonMessageMQ
{
	class RabbitMQConsumer : public MessageQueueConsumer
	{
	public:
		RabbitMQConsumer(const std::string& host,
						 int port,
						 const std::string& user_name,
						 const std::string& password,
						 const SSLOptions& ssl_options = SSLOptions(),
						 int channel_id = 1,
						 int heartbeat = 60);
		virtual ~RabbitMQConsumer();

		auto start() -> std::tuple<bool, std::optional<std::string>> override;
		auto wait_stop() -> std::tuple<bool, std::optional<std::string>> override;
		auto stop() -> std::tuple<bool, std::optional<std::string>> override;

		auto subscribe(const std::string& queue_name,
					   const std::string& consumer_group,
					   const std::function<std::tuple<bool, std::optional<std::string>>(
						   const std::string&, const std::string&, const std::string&)>& task_callback,
					   const std::function<std::tuple<bool, std::optional<std::string>>(
						   const std::string&, const std::string&, const std::string&)>& expired_callback = nullptr)
			-> std::tuple<bool, std::optional<std::string>> override;

	private:
		int channel_id_;
		int heartbeat_;
		std::unique_ptr<RabbitMQQueueWorker> worker_;
	};
}
