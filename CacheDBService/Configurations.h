#pragma once

#include "ArgumentParser.h"
#include "LogTypes.h"

#include <optional>
#include <string>
#include <tuple>
#include <vector>


using namespace Utilities;

class Configurations
{
public:
	Configurations(ArgumentParser&& arguments);
	virtual ~Configurations(void);

	// Logger
	auto service_title() const -> std::string;
	auto log_root_path() const -> std::string;
	auto write_file() const -> LogTypes;
	auto write_console() const -> LogTypes;
	auto write_interval() const -> int;

	// Logger setters
	auto set_service_title(const std::string& value) -> void;
	auto set_log_root_path(const std::string& value) -> void;
	auto set_write_file(const LogTypes& value) -> void;
	auto set_write_console(const LogTypes& value) -> void;
	auto set_write_interval(const int& value) -> void;

	// Thread pool
	auto high_priority_worker_count() const -> int;
	auto normal_priority_worker_count() const -> int;
	auto low_priority_worker_count() const -> int;

	// Redis
	auto redis_host() const -> std::string;
	auto redis_port() const -> int;
	auto redis_db_index() const -> int;
	auto redis_stream_key() const -> std::string;
	auto redis_group_name() const -> std::string;
	auto redis_consumer_name() const -> std::string;
	auto redis_block_ms() const -> int;
	auto redis_count() const -> int;
	auto redis_auto_create_group() const -> bool;
	auto publish_to_main_db_service_interval_ms() const -> int;
	auto redis_reconnect_max_retries() const -> int;
	auto redis_reconnect_interval_ms() const -> int;

	// MQ publisher
	auto rabbit_mq_host() const -> std::string;
	auto rabbit_mq_port() const -> int;
	auto rabbit_mq_user_name() const -> std::string;
	auto rabbit_mq_password() const -> std::string;
	auto rabbit_channel_id() const -> int;
	auto publish_queue_name() const -> std::string;
	auto content_type() const -> std::string;
	auto rabbit_mq_reconnect_max_retries() const -> int;
	auto rabbit_mq_reconnect_interval_ms() const -> int;

protected:
	auto load() -> void;
	auto parse(ArgumentParser& arguments) -> void;

private:
	std::string root_path_;

	std::string service_title_;
	std::string log_root_path_;
	LogTypes write_file_;
	LogTypes write_console_;
	int write_interval_;

	// Thread pool
	int high_priority_worker_count_;
	int normal_priority_worker_count_;
	int low_priority_worker_count_;

	// Redis
	std::string redis_host_;
	int redis_port_;
	int redis_db_index_;
	std::string redis_stream_key_;
	std::string redis_group_name_;
	std::string redis_consumer_name_;
	int redis_block_ms_;
	int redis_count_;
	bool redis_auto_create_group_;
	int publish_to_main_db_service_interval_ms_;
	int redis_reconnect_max_retries_;
	int redis_reconnect_interval_ms_;

	// MQ
	std::string rabbit_mq_host_;
	int rabbit_mq_port_;
	std::string rabbit_mq_user_name_;
	std::string rabbit_mq_password_;
	int rabbit_channel_id_;
	std::string publish_queue_name_;
	std::string content_type_;
	int rabbit_mq_reconnect_max_retries_;
	int rabbit_mq_reconnect_interval_ms_;
};
