#include "RedisConsumer.h"

namespace CommonMessageMQ
{
	RedisConsumer::RedisConsumer(const std::string& consumer_name,
								 const std::string& address,
								 int port,
								 const TLSOptions& tls_options,
								 int db_index)
		: worker_(nullptr)
	{
		worker_ = std::make_unique<RedisQueueWorker>(consumer_name, address, port, tls_options, db_index);
	}

	RedisConsumer::~RedisConsumer()
	{
		if (worker_ != nullptr)
		{
			worker_->stop();
			worker_.reset();
		}
	}

	auto RedisConsumer::start() -> std::tuple<bool, std::optional<std::string>>
	{
		if (worker_ == nullptr)
		{
			return { false, std::optional<std::string>("Redis worker is nullptr") };
		}

		return worker_->start();
	}

	auto RedisConsumer::wait_stop() -> std::tuple<bool, std::optional<std::string>>
	{
		if (worker_ == nullptr)
		{
			return { false, std::optional<std::string>("Redis worker is nullptr") };
		}

		return worker_->wait_stop();
	}

	auto RedisConsumer::stop() -> std::tuple<bool, std::optional<std::string>>
	{
		if (worker_ == nullptr)
		{
			return { false, std::optional<std::string>("Redis worker is nullptr") };
		}

		return worker_->stop();
	}

	auto RedisConsumer::subscribe(const std::string& queue_name,
								  const std::string& consumer_group,
								  const std::function<std::tuple<bool, std::optional<std::string>>(
									  const std::string&, const std::string&, const std::string&)>& task_callback,
								  const std::function<std::tuple<bool, std::optional<std::string>>(
									  const std::string&, const std::string&, const std::string&)>& expired_callback)
		-> std::tuple<bool, std::optional<std::string>>
	{
		if (worker_ == nullptr)
		{
			return { false, std::optional<std::string>("Redis worker is nullptr") };
		}

		return worker_->subscribe(queue_name, consumer_group, task_callback, expired_callback);
	}
}
