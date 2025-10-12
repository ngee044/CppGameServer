#pragma once

#include "MessageQueueConsumer.h"
#include "RedisWorkQueueConsume.h"

#include <memory>

using namespace Redis;

namespace CommonMessageMQ
{
	class RedisConsumer : public MessageQueueConsumer
	{
	public:
		RedisConsumer(const std::string& consumer_name,
					  const std::string& address,
					  int port = 6379,
					  const TLSOptions& tls_options = TLSOptions(),
					  int db_index = 0);
		virtual ~RedisConsumer();

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
		std::unique_ptr<RedisWorkQueueConsume> worker_;
	};
}
