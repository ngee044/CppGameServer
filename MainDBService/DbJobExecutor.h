#pragma once

#include "PostgresDB.h"
#include <boost/json.hpp>

#include <optional>
#include <string>
#include <tuple>

class DbJobExecutor
{
public:
	DbJobExecutor(Database::PostgresDB& db,
				  const std::vector<std::string>& allowed_ops,
				  const std::vector<std::string>& allowed_tables);

	auto handle_message(const std::string& message) -> std::tuple<bool, std::optional<std::string>>;

private:
	auto to_sql(const boost::json::object& obj) -> std::tuple<bool, std::string, std::string>;
	auto execute_batch(const std::vector<std::string>& sqls) -> std::tuple<bool, std::optional<std::string>>;
	
	auto op_allowed(const std::string& op) const -> bool;
	auto table_allowed(const std::string& table) const -> bool;
	
	auto quote_identifier(const std::string& ident) const -> std::string;
	auto json_value_to_sql_literal(const boost::json::value& v) const -> std::string;
	auto build_where_clause(const boost::json::object& where) const -> std::string;
	auto build_insert_sql(const std::string& table, const boost::json::object& values) const -> std::string;
	auto build_update_sql(const std::string& table, const boost::json::object& values, const boost::json::object& where) const -> std::string;
	auto build_delete_sql(const std::string& table, const boost::json::object& where) const -> std::string;

private:
	Database::PostgresDB& db_;
	std::vector<std::string> allowed_ops_;
	std::vector<std::string> allowed_tables_;
};
