#pragma once

#include "Configurations.h"
#include "RedisClient.h"
#include "RabbitMQWorkQueueEmitter.h"
#include "ThreadPool.h"
#include "ThreadWorker.h"
#include "Job.h"
#include "JobPriorities.h"

#include <future>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

using namespace Thread;

class CacheDBService
{
public:
	CacheDBService(std::shared_ptr<Configurations> configurations);
	virtual ~CacheDBService();

	auto start() -> std::tuple<bool, std::optional<std::string>>;
	auto wait_stop() -> std::tuple<bool, std::optional<std::string>>;
	auto stop() -> std::tuple<bool, std::optional<std::string>>;

	// Direct cache access API
	auto set_key_value(const std::string& key, const std::string& value, long ttl_seconds = 0) -> std::tuple<bool, std::optional<std::string>>;
	auto get_key_value(const std::string& key) -> std::tuple<std::optional<std::string>, std::optional<std::string>>;
	auto enqueue_database_operation(const std::string& json_body) -> std::tuple<bool, std::optional<std::string>>;

protected:
	auto create_thread_pool() -> std::tuple<bool, std::optional<std::string>>;
	auto destroy_thread_pool() -> void;
	auto ensure_stream_group() -> std::tuple<bool, std::optional<std::string>>;
	auto publish_message(const std::string& message_body) -> std::tuple<bool, std::optional<std::string>>;
	auto ensure_redis_connection() -> std::tuple<bool, std::optional<std::string>>;
	auto ensure_rabbitmq_connection() -> std::tuple<bool, std::optional<std::string>>;

private:
	std::shared_ptr<Configurations> configurations_;
    std::unique_ptr<Redis::RedisClient> redis_client_;
    std::unique_ptr<RabbitMQ::RabbitMQWorkQueueEmitter> work_queue_emitter_;
    std::shared_ptr<ThreadPool> thread_pool_;

    std::promise<void> stop_promise_;
    std::shared_future<void> stop_future_;

	struct PendingMessage
	{
		std::string message_body;
	};

	std::mutex pending_mutex_;
	std::vector<PendingMessage> pending_messages_;

	void schedule_publish_job();
	auto publish_to_main_db_service() -> std::tuple<bool, std::optional<std::string>>;
	auto is_stop_requested() const -> bool;
};
