// Simplified configurations tailored for MainDBService

#include "Configurations.h"

#include "File.h"
#include "LogTypes.h"
#include "Logger.h"
#include "Converter.h"

#include "fmt/xchar.h"
#include "fmt/format.h"

#include "boost/json.hpp"
#include "boost/json/parse.hpp"

#include <filesystem>

using namespace Utilities;

Configurations::Configurations(ArgumentParser&& arguments)
	: root_path_("")
	, service_title_("MainDBService")
	, log_root_path_("")
	, write_file_(LogTypes::None)
	, write_console_(LogTypes::None)
	, write_interval_(0)
	, rabbit_mq_host_("127.0.0.1")
	, rabbit_mq_port_(5672)
	, rabbit_mq_user_name_("guest")
	, rabbit_mq_password_("guest")
	, rabbit_heartbeat_(60)
	, rabbit_channel_id_(1)
	, consume_queue_name_("db.write")
	, requeue_on_failure_(false)
	, dlx_exchange_(std::nullopt)
	, dlx_routing_key_(std::nullopt)
	, message_ttl_ms_(std::nullopt)
	, postgres_conn_("host=127.0.0.1 port=5432 dbname=game user=postgres password=postgres")
{
	root_path_ = arguments.program_folder();
	load();
	parse(arguments);
}

Configurations::~Configurations(void)
{
}

auto Configurations::service_title() const -> std::string
{
	return service_title_;
}

auto Configurations::log_root_path() const -> std::string
{
	return log_root_path_;
}

auto Configurations::write_file() const -> LogTypes
{
	return write_file_;
}

auto Configurations::write_console() const -> LogTypes
{
	return write_console_;
}

auto Configurations::write_interval() const -> int
{
	return write_interval_;
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

auto Configurations::rabbit_heartbeat() const -> int
{
	return rabbit_heartbeat_;
}

auto Configurations::rabbit_channel_id() const -> int
{
	return rabbit_channel_id_;
}

auto Configurations::consume_queue_name() const -> std::string
{
	return consume_queue_name_;
}

auto Configurations::requeue_on_failure() const -> bool
{
	return requeue_on_failure_;
}

auto Configurations::dlx_exchange() const -> std::optional<std::string>
{
	return dlx_exchange_;
}

auto Configurations::dlx_routing_key() const -> std::optional<std::string>
{
	return dlx_routing_key_;
}

auto Configurations::message_ttl_ms() const -> std::optional<int>
{
	return message_ttl_ms_;
}

auto Configurations::postgres_conn() const -> std::string
{
	return postgres_conn_;
}

auto Configurations::allowed_ops() const -> const std::vector<std::string>&
{
	return allowed_ops_;
}

auto Configurations::allowed_tables() const -> const std::vector<std::string>&
{
	return allowed_tables_;
}

auto Configurations::load() -> void
{
	std::filesystem::path path = root_path_ + "main_db_service_cfg.json";
	if (!std::filesystem::exists(path))
	{
		Logger::handle().write(LogTypes::Error, fmt::format("Configurations file does not exist: {}", path.string()));
		return;
	}

	File source;
	source.open(fmt::format("{}main_db_service_cfg.json", root_path_), std::ios::in | std::ios::binary, std::locale(""));
	auto [source_data, error_message] = source.read_bytes();
	if (source_data == std::nullopt)
	{
		Logger::handle().write(LogTypes::Error, error_message.value());
		return;
	}

	boost::json::object message = boost::json::parse(Converter::to_string(source_data.value())).as_object();

	if (message.contains("service_title"))
	{
		service_title_ = message.at("service_title").as_string().data();
	}

	if (message.contains("log_root_path"))
	{
		log_root_path_ = message.at("log_root_path").as_string().data();
	}

	if (message.contains("write_file"))
	{
		write_file_ = static_cast<LogTypes>(message.at("write_file").as_int64());
	}

	if (message.contains("write_console"))
	{
		write_console_ = static_cast<LogTypes>(message.at("write_console").as_int64());
	}

	if (message.contains("write_interval"))
	{
		write_interval_ = static_cast<int>(message.at("write_interval").as_int64());
	}

	if (message.contains("rabbit_host"))
	{
		rabbit_mq_host_ = message.at("rabbit_host").as_string().data();
	}
	else if (message.contains("rabbit_mq_host"))
	{
		rabbit_mq_host_ = message.at("rabbit_mq_host").as_string().data();
	}

	if (message.contains("rabbit_port"))
	{
		rabbit_mq_port_ = static_cast<int>(message.at("rabbit_port").as_int64());
	}
	else if (message.contains("rabbit_mq_port"))
	{
		rabbit_mq_port_ = static_cast<int>(message.at("rabbit_mq_port").as_int64());
	}

	if (message.contains("rabbit_user"))
	{
		rabbit_mq_user_name_ = message.at("rabbit_user").as_string().data();
	}
	else if (message.contains("rabbit_mq_user_name"))
	{
		rabbit_mq_user_name_ = message.at("rabbit_mq_user_name").as_string().data();
	}

	if (message.contains("rabbit_password"))
	{
		rabbit_mq_password_ = message.at("rabbit_password").as_string().data();
	}
	else if (message.contains("rabbit_mq_password"))
	{
		rabbit_mq_password_ = message.at("rabbit_mq_password").as_string().data();
	}

	if (message.contains("rabbit_heartbeat"))
	{
		rabbit_heartbeat_ = static_cast<int>(message.at("rabbit_heartbeat").as_int64());
	}
	if (message.contains("rabbit_channel_id"))
	{
		rabbit_channel_id_ = static_cast<int>(message.at("rabbit_channel_id").as_int64());
	}

	if (message.contains("rabbit_queue"))
	{
		consume_queue_name_ = message.at("rabbit_queue").as_string().data();
	}
	else if (message.contains("consume_queue_name"))
	{
		consume_queue_name_ = message.at("consume_queue_name").as_string().data();
	}

	if (message.contains("requeue_on_failure"))
	{
		requeue_on_failure_ = message.at("requeue_on_failure").as_bool();
	}

	if (message.contains("postgres_conn"))
	{
		postgres_conn_ = message.at("postgres_conn").as_string().data();
	}

	// DLQ / TTL options (optional)
	if (message.contains("dlx_exchange") && message.at("dlx_exchange").is_string())
	{
		std::string v = message.at("dlx_exchange").as_string().data();
		if (!v.empty())
		{
			dlx_exchange_ = v;
		}
	}
	if (message.contains("dlx_routing_key") && message.at("dlx_routing_key").is_string())
	{
		std::string v = message.at("dlx_routing_key").as_string().data();
		if (!v.empty())
		{
			dlx_routing_key_ = v;
		}
	}
	if (message.contains("message_ttl_ms") && message.at("message_ttl_ms").is_int64())
	{
		int v = static_cast<int>(message.at("message_ttl_ms").as_int64());
		if (v > 0)
		{
			message_ttl_ms_ = v;
		}
	}

	if (message.contains("allowed_ops") && message.at("allowed_ops").is_array())
	{
		allowed_ops_.clear();
		for (auto& v : message.at("allowed_ops").as_array())
		{
			allowed_ops_.push_back(boost::json::value_to<std::string>(v));
		}
	}
	if (message.contains("allowed_tables") && message.at("allowed_tables").is_array())
	{
		allowed_tables_.clear();
		for (auto& v : message.at("allowed_tables").as_array())
		{
			allowed_tables_.push_back(boost::json::value_to<std::string>(v));
		}
	}
}

auto Configurations::parse(ArgumentParser& arguments) -> void
{
	// Optional CLI overrides
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
	if (auto v = arguments.to_int("--rabbit_heartbeat"); v != std::nullopt)
	{
		rabbit_heartbeat_ = v.value();
	}
	if (auto v = arguments.to_int("--rabbit_channel_id"); v != std::nullopt)
	{
		rabbit_channel_id_ = v.value();
	}
	if (auto v = arguments.to_string("--consume_queue_name"); v != std::nullopt)
	{
		consume_queue_name_ = v.value();
	}
	if (auto v = arguments.to_string("--postgres_conn"); v != std::nullopt)
	{
		postgres_conn_ = v.value();
	}
	if (auto v = arguments.to_bool("--requeue_on_failure"); v != std::nullopt)
	{
		requeue_on_failure_ = v.value();
	}
	if (auto v = arguments.to_string("--dlx_exchange"); v != std::nullopt && !v->empty())
	{
		dlx_exchange_ = v.value();
	}
	if (auto v = arguments.to_string("--dlx_routing_key"); v != std::nullopt && !v->empty())
	{
		dlx_routing_key_ = v.value();
	}
	if (auto v = arguments.to_int("--message_ttl_ms"); v != std::nullopt && v.value() > 0)
	{
		message_ttl_ms_ = v.value();
	}
}
