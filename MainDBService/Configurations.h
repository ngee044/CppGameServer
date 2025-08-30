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

	auto service_title() const -> std::string;
	auto log_root_path() const -> std::string;

	auto write_file() const -> LogTypes;
	auto write_console() const -> LogTypes;
	auto write_interval() const -> int;

	auto rabbit_mq_host() const -> std::string;
	auto rabbit_mq_port() const -> int;
	auto rabbit_mq_user_name() const -> std::string;
	auto rabbit_mq_password() const -> std::string;
	auto rabbit_heartbeat() const -> int;
	auto rabbit_channel_id() const -> int;
	auto consume_queue_name() const -> std::string;
	auto requeue_on_failure() const -> bool;
	auto dlx_exchange() const -> std::optional<std::string>;
	auto dlx_routing_key() const -> std::optional<std::string>;
	auto message_ttl_ms() const -> std::optional<int>;

	auto postgres_conn() const -> std::string;
	auto allowed_ops() const -> const std::vector<std::string>&;
	auto allowed_tables() const -> const std::vector<std::string>&;

protected:
	auto load() -> void;
	auto parse(ArgumentParser& arguments) -> void;

private:
	std::string service_title_;
	std::string root_path_;

	std::string log_root_path_;
	LogTypes write_file_;
	LogTypes write_console_;
	int write_interval_;

	// MQ
	std::string rabbit_mq_host_;
	int rabbit_mq_port_;
	std::string rabbit_mq_user_name_;
	std::string rabbit_mq_password_;
	int rabbit_heartbeat_;
	int rabbit_channel_id_;
	std::string consume_queue_name_;
	bool requeue_on_failure_;
	std::optional<std::string> dlx_exchange_;
	std::optional<std::string> dlx_routing_key_;
	std::optional<int> message_ttl_ms_;

	// DB
	std::string postgres_conn_;

	// Policy
	std::vector<std::string> allowed_ops_;
	std::vector<std::string> allowed_tables_;
};
