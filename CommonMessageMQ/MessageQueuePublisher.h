#pragma once

#include <tuple>
#include <string>
#include <optional>

namespace CommonMessageMQ
{
	class MessageQueuePublisher
	{
	public:
		virtual ~MessageQueuePublisher() = default;

		virtual auto start() -> std::tuple<bool, std::optional<std::string>> = 0;
		virtual auto stop() -> std::tuple<bool, std::optional<std::string>> = 0;

		virtual auto publish(const std::string& queue_name,
							 const std::string& message,
							 const std::string& message_type,
							 const std::optional<long>& ttl_seconds = std::nullopt)
			-> std::tuple<bool, std::optional<std::string>> = 0;

		virtual auto publish_async(const std::string& queue_name,
								   const std::string& message,
								   const std::string& message_type,
								   const std::optional<long>& ttl_seconds = std::nullopt)
			-> std::tuple<bool, std::optional<std::string>> = 0;
	};
}
