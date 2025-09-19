// Configurations for CacheDBService

#include "Configurations.h"

#include "File.h"
#include "Logger.h"
#include "Converter.h"

#include "fmt/format.h"
#include "fmt/xchar.h"

#include "boost/json.hpp"
#include "boost/json/parse.hpp"

#include <filesystem>

using namespace Utilities;

Configurations::Configurations(ArgumentParser&& arguments)
	: root_path_("")
	, service_title_("CacheDBService")
	, log_root_path_("")
	, write_file_(LogTypes::None)
	, write_console_(LogTypes::None)
	, write_interval_(0)
	, high_priority_worker_count_(1)
	, normal_priority_worker_count_(1)
	, low_priority_worker_count_(1)
	, redis_host_("127.0.0.1")
	, redis_port_(6379)
	, redis_db_index_(0)
	, redis_stream_key_("cache:changes")
	, redis_group_name_("cache-writers")
	, redis_consumer_name_("cache-writer-1")
	, redis_block_ms_(1000)
	, redis_count_(50)
	, redis_auto_create_group_(true)
	, rabbit_mq_host_("127.0.0.1")
	, rabbit_mq_port_(5672)
	, rabbit_mq_user_name_("guest")
	, rabbit_mq_password_("guest")
	, rabbit_channel_id_(1)
	, publish_queue_name_("db.write")
	, content_type_("application/json")
	, publish_to_main_db_service_interval_ms_(1000)
{
	root_path_ = arguments.program_folder();
	load();
	parse(arguments);
}

Configurations::~Configurations(void)
{
}

// Logger getters
auto Configurations::service_title() const -> std::string { return service_title_; }
auto Configurations::log_root_path() const -> std::string { return log_root_path_; }
auto Configurations::write_file() const -> LogTypes { return write_file_; }
auto Configurations::write_console() const -> LogTypes { return write_console_; }
auto Configurations::write_interval() const -> int { return write_interval_; }

// Logger setters
auto Configurations::set_service_title(const std::string& value) -> void { service_title_ = value; }
auto Configurations::set_log_root_path(const std::string& value) -> void { log_root_path_ = value; }
auto Configurations::set_write_file(const LogTypes& value) -> void { write_file_ = value; }
auto Configurations::set_write_console(const LogTypes& value) -> void { write_console_ = value; }
auto Configurations::set_write_interval(const int& value) -> void { write_interval_ = value; }

auto Configurations::redis_host() const -> std::string
{
	return redis_host_;
}

auto Configurations::redis_port() const -> int
{
	return redis_port_;
}

auto Configurations::redis_db_index() const -> int
{
	return redis_db_index_;
}

auto Configurations::redis_stream_key() const -> std::string
{
	return redis_stream_key_;
}

auto Configurations::redis_group_name() const -> std::string
{
	return redis_group_name_;
}

auto Configurations::redis_consumer_name() const -> std::string
{
	return redis_consumer_name_;
}

auto Configurations::redis_block_ms() const -> int
{
	return redis_block_ms_;
}

auto Configurations::redis_count() const -> int
{
	return redis_count_;
}

auto Configurations::redis_auto_create_group() const -> bool
{
	return redis_auto_create_group_;
}

auto Configurations::publish_to_main_db_service_interval_ms() const -> int
{
	return publish_to_main_db_service_interval_ms_;
}

auto Configurations::rabbit_mq_host() const -> std::string
{
	return rabbit_mq_host_;
}

auto Configurations::rabbit_mq_port() const -> int
{
	return rabbit_mq_port_;
}

auto Configurations::rabbit_mq_user_name() const -> std::string
{
	return rabbit_mq_user_name_;
}

auto Configurations::rabbit_mq_password() const -> std::string
{
	return rabbit_mq_password_;
}

auto Configurations::rabbit_channel_id() const -> int
{
	return rabbit_channel_id_;
}

auto Configurations::publish_queue_name() const -> std::string
{
	return publish_queue_name_;
}

auto Configurations::content_type() const -> std::string
{
	return content_type_;
}

auto Configurations::load() -> void
{
	std::filesystem::path path = root_path_ + "cache_db_service_cfg.json";
	if (!std::filesystem::exists(path))
	{
		Logger::handle().write(LogTypes::Error, fmt::format("Configurations file does not exist: {}", path.string()));
		return;
	}

	File source;
	source.open(fmt::format("{}cache_db_service_cfg.json", root_path_), std::ios::in | std::ios::binary, std::locale(""));
	auto [source_data, error_message] = source.read_bytes();
	if (source_data == std::nullopt)
	{
		Logger::handle().write(LogTypes::Error, error_message.value());
		return;
	}

	boost::json::object obj = boost::json::parse(Converter::to_string(source_data.value())).as_object();

	// Logger
	if (obj.contains("service_title"))
	{
		service_title_ = obj.at("service_title").as_string().data();
	}
	if (obj.contains("log_root_path"))
	{
		log_root_path_ = obj.at("log_root_path").as_string().data();
	}
	if (obj.contains("write_file"))
	{
		write_file_ = static_cast<LogTypes>(obj.at("write_file").as_int64());
	}
	if (obj.contains("write_console"))
	{
		write_console_ = static_cast<LogTypes>(obj.at("write_console").as_int64());
	}
	if (obj.contains("write_interval"))
	{
		write_interval_ = static_cast<int>(obj.at("write_interval").as_int64());
	}

	// Thread pool
	if (obj.contains("high_priority_count"))
	{
		high_priority_worker_count_ = static_cast<int>(obj.at("high_priority_count").as_int64());
	}
	if (obj.contains("normal_priority_count"))
	{
		normal_priority_worker_count_ = static_cast<int>(obj.at("normal_priority_count").as_int64());
	}
	if (obj.contains("low_priority_count"))
	{
		low_priority_worker_count_ = static_cast<int>(obj.at("low_priority_count").as_int64());
	}

	// Redis
	if (obj.contains("redis_host"))
	{
		redis_host_ = obj.at("redis_host").as_string().data();
	}
	if (obj.contains("redis_port"))
	{
		redis_port_ = static_cast<int>(obj.at("redis_port").as_int64());
	}
	if (obj.contains("redis_db_index"))
	{
		redis_db_index_ = static_cast<int>(obj.at("redis_db_index").as_int64());
	}
	if (obj.contains("redis_stream_key"))
	{
		redis_stream_key_ = obj.at("redis_stream_key").as_string().data();
	}
	if (obj.contains("redis_group_name"))
	{
		redis_group_name_ = obj.at("redis_group_name").as_string().data();
	}
	if (obj.contains("redis_consumer_name"))
	{
		redis_consumer_name_ = obj.at("redis_consumer_name").as_string().data();
	}
	if (obj.contains("redis_block_ms"))
	{
		redis_block_ms_ = static_cast<int>(obj.at("redis_block_ms").as_int64());
	}
	if (obj.contains("redis_count"))
	{
		redis_count_ = static_cast<int>(obj.at("redis_count").as_int64());
	}
	if (obj.contains("redis_auto_create_group"))
	{
		redis_auto_create_group_ = obj.at("redis_auto_create_group").as_bool();
	}
	if (obj.contains("publish_to_main_db_service_interval_ms"))
	{
		publish_to_main_db_service_interval_ms_ = static_cast<int>(obj.at("publish_to_main_db_service_interval_ms").as_int64());
	}

	// MQ
	if (obj.contains("rabbit_mq_host"))
	{
		rabbit_mq_host_ = obj.at("rabbit_mq_host").as_string().data();
	}
	if (obj.contains("rabbit_mq_port"))
	{
		rabbit_mq_port_ = static_cast<int>(obj.at("rabbit_mq_port").as_int64());
	}
	if (obj.contains("rabbit_mq_user_name"))
	{
		rabbit_mq_user_name_ = obj.at("rabbit_mq_user_name").as_string().data();
	}
	if (obj.contains("rabbit_mq_password"))
	{
		rabbit_mq_password_ = obj.at("rabbit_mq_password").as_string().data();
	}
	if (obj.contains("rabbit_channel_id"))
	{
		rabbit_channel_id_ = static_cast<int>(obj.at("rabbit_channel_id").as_int64());
	}
	if (obj.contains("publish_queue_name"))
	{
		publish_queue_name_ = obj.at("publish_queue_name").as_string().data();
	}
	if (obj.contains("content_type"))
	{
		content_type_ = obj.at("content_type").as_string().data();
	}
}

auto Configurations::parse(ArgumentParser& arguments) -> void
{
	// Logger
	if (auto v = arguments.to_string("--service_title"); v != std::nullopt)
	{
		service_title_ = v.value();
	}
	if (auto v = arguments.to_string("--log_root_path"); v != std::nullopt)
	{
		log_root_path_ = v.value();
	}
	if (auto v = arguments.to_int("--write_file"); v != std::nullopt)
	{
		write_file_ = static_cast<LogTypes>(v.value());
	}
	if (auto v = arguments.to_int("--write_console"); v != std::nullopt)
	{
		write_console_ = static_cast<LogTypes>(v.value());
	}
	if (auto v = arguments.to_int("--write_interval"); v != std::nullopt)
	{
		write_interval_ = v.value();
	}

	// Thread pool
	if (auto v = arguments.to_int("--high_priority_count"); v != std::nullopt)
	{
		high_priority_worker_count_ = v.value();
	}
	if (auto v = arguments.to_int("--normal_priority_count"); v != std::nullopt)
	{
		normal_priority_worker_count_ = v.value();
	}
	if (auto v = arguments.to_int("--low_priority_count"); v != std::nullopt)
	{
		low_priority_worker_count_ = v.value();
	}

	// Redis
	if (auto v = arguments.to_string("--redis_host"); v != std::nullopt)
	{
		redis_host_ = v.value();
	}
	if (auto v = arguments.to_int("--redis_port"); v != std::nullopt)
	{
		redis_port_ = v.value();
	}
	if (auto v = arguments.to_int("--redis_db_index"); v != std::nullopt)
	{
		redis_db_index_ = v.value();
	}
	if (auto v = arguments.to_string("--redis_stream_key"); v != std::nullopt)
	{
		redis_stream_key_ = v.value();
	}
	if (auto v = arguments.to_string("--redis_group_name"); v != std::nullopt)
	{
		redis_group_name_ = v.value();
	}
	if (auto v = arguments.to_string("--redis_consumer_name"); v != std::nullopt)
	{
		redis_consumer_name_ = v.value();
	}
	if (auto v = arguments.to_int("--redis_block_ms"); v != std::nullopt)
	{
		redis_block_ms_ = v.value();
	}
	if (auto v = arguments.to_int("--redis_count"); v != std::nullopt)
	{
		redis_count_ = v.value();
	}
	if (auto v = arguments.to_bool("--redis_auto_create_group"); v != std::nullopt)
	{
		redis_auto_create_group_ = v.value();
	}
	if (auto v = arguments.to_int("--publish_to_main_db_service_interval_ms"); v != std::nullopt)
	{
		publish_to_main_db_service_interval_ms_ = v.value();
	}

	// MQ
	if (auto v = arguments.to_string("--rabbit_mq_host"); v != std::nullopt)
	{
		rabbit_mq_host_ = v.value();
	}
	if (auto v = arguments.to_int("--rabbit_mq_port"); v != std::nullopt)
	{
		rabbit_mq_port_ = v.value();
	}
	if (auto v = arguments.to_string("--rabbit_mq_user_name"); v != std::nullopt)
	{
		rabbit_mq_user_name_ = v.value();
	}
	if (auto v = arguments.to_string("--rabbit_mq_password"); v != std::nullopt)
	{
		rabbit_mq_password_ = v.value();
	}
	if (auto v = arguments.to_int("--rabbit_channel_id"); v != std::nullopt)
	{
		rabbit_channel_id_ = v.value();
	}
	if (auto v = arguments.to_string("--publish_queue_name"); v != std::nullopt)
	{
		publish_queue_name_ = v.value();
	}
	if (auto v = arguments.to_string("--content_type"); v != std::nullopt)
	{
		content_type_ = v.value();
	}
}
// Thread pool getters
auto Configurations::high_priority_worker_count() const -> int { return high_priority_worker_count_; }
auto Configurations::normal_priority_worker_count() const -> int { return normal_priority_worker_count_; }
auto Configurations::low_priority_worker_count() const -> int { return low_priority_worker_count_; }
