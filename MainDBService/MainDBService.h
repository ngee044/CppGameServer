#pragma once

#include "Configurations.h"
#include "DbJobExecutor.h"
#include "WorkQueueConsume.h"

#include <atomic>
#include <optional>
#include <string>
#include <tuple>

class MainDBService
{
public:
	MainDBService(const Configurations& cfg, DbJobExecutor& executor);

	auto start() -> std::tuple<bool, std::optional<std::string>>;
	void run_until_signal();
	auto stop() -> void;

	static void on_signal(int);
	static auto instance(MainDBService* inst = nullptr) -> MainDBService*;

private:
	const Configurations& cfg_;
	DbJobExecutor& executor_;
	RabbitMQ::WorkQueueConsume consumer_;
	std::atomic<bool> stop_flag_;
};

