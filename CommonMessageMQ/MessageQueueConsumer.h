#pragma once

#include <tuple>
#include <string>
#include <optional>
#include <functional>

namespace CommonMessageMQ
{
	class MessageQueueConsumer
	{
	public:
		virtual ~MessageQueueConsumer() = default;

		virtual auto start() -> std::tuple<bool, std::optional<std::string>> = 0;
		virtual auto wait_stop() -> std::tuple<bool, std::optional<std::string>> = 0;
		virtual auto stop() -> std::tuple<bool, std::optional<std::string>> = 0;

		virtual auto subscribe(const std::string& queue_name,
							   const std::string& consumer_group,
							   const std::function<std::tuple<bool, std::optional<std::string>>(
								   const std::string&, const std::string&, const std::string&)>& task_callback,
							   const std::function<std::tuple<bool, std::optional<std::string>>(
								   const std::string&, const std::string&, const std::string&)>& expired_callback = nullptr)
			-> std::tuple<bool, std::optional<std::string>> = 0;
	};
}
