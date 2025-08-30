#pragma once

#include "Configurations.h"
#include "RedisClient.h"
#include "WorkQueueEmitter.h"

#include <atomic>
#include <optional>
#include <string>
#include <tuple>
#include <vector>
#include <mutex>

#include "ThreadPool.h"
#include "ThreadWorker.h"
#include "Job.h"
#include "JobPriorities.h"

class CacheDBService
{
public:
	CacheDBService(const Configurations& cfg);
	virtual ~CacheDBService();

	auto start() -> std::tuple<bool, std::optional<std::string>>;
	auto wait_stop() -> std::tuple<bool, std::optional<std::string>>;
	auto stop() -> std::tuple<bool, std::optional<std::string>>;

private:
	void consume_loop();
	auto ensure_stream_group() -> std::tuple<bool, std::optional<std::string>>;
	auto publish_json(const std::string& body) -> std::tuple<bool, std::optional<std::string>>;

private:
	const Configurations& cfg_;
	Redis::RedisClient redis_;
	RabbitMQ::WorkQueueEmitter emitter_;
	std::atomic<bool> stop_flag_;
	std::shared_ptr<Thread::ThreadPool> thread_pool_;

	struct Pending
	{
		std::string key;
		std::string id;
		std::string body;
	};
	std::mutex pending_mutex_;
	std::vector<Pending> pending_;

	// scheduling helpers
	void schedule_flush_job();
	auto flush_cycle() -> std::tuple<bool, std::optional<std::string>>;
};
