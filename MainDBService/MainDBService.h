#pragma once

#include "Configurations.h"
#include "DbJobExecutor.h"
#include "RabbitMQWorkQueueConsume.h"

#include <optional>
#include <string>
#include <memory>

using namespace RabbitMQ;

class MainDBService
{
public:
	MainDBService(std::shared_ptr<Configurations> configurations, std::shared_ptr<DbJobExecutor> executor);

	auto start() -> std::tuple<bool, std::optional<std::string>>;
	auto wait_stop() -> std::tuple<bool, std::optional<std::string>>;
	auto stop() -> void;

protected:
	auto consume_queue() -> std::tuple<bool, std::optional<std::string>>;

private:
	std::shared_ptr<Configurations> configurations_;
	std::shared_ptr<DbJobExecutor> executor_;


	std::shared_ptr<RabbitMQWorkQueueConsume> consumer_;
};

