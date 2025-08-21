#pragma once

#include "LogTypes.h"
#include "ArgumentParser.h"

#include <string>
#include <vector>

using namespace Utilities;

class Configurations
{
public:
	Configurations(ArgumentParser&& arguments);
	virtual ~Configurations(void);

	// Accessors needed by MainDBService
	auto rabbit_mq_host() const -> std::string;
	auto rabbit_mq_port() const -> int;
	auto rabbit_mq_user_name() const -> std::string;
	auto rabbit_mq_password() const -> std::string;
	auto rabbit_heartbeat() const -> int;
	auto rabbit_channel_id() const -> int;
	auto consume_queue_name() const -> std::string;

	auto postgres_conn() const -> std::string;
	auto allowed_ops() const -> const std::vector<std::string>&;
	auto allowed_tables() const -> const std::vector<std::string>&;

protected:
	auto load() -> void;
	auto parse(ArgumentParser& arguments) -> void;

private:
	std::string root_path_;

	// MQ
	std::string rabbit_mq_host_;
	int rabbit_mq_port_;
	std::string rabbit_mq_user_name_;
	std::string rabbit_mq_password_;
	int rabbit_heartbeat_;
	int rabbit_channel_id_;
	std::string consume_queue_name_;

	// DB
	std::string postgres_conn_;

	// Policy
	std::vector<std::string> allowed_ops_;
	std::vector<std::string> allowed_tables_;
};
