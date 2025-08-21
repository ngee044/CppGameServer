#include "DbJobExecutor.h"

using namespace Database;

DbJobExecutor::DbJobExecutor(PostgresDB& db,
							   const std::vector<std::string>& allowed_ops,
							   const std::vector<std::string>& allowed_tables)
	: db_(db)
	, allowed_ops_(allowed_ops)
	, allowed_tables_(allowed_tables)
{}

auto DbJobExecutor::handle_message(const std::string& message) -> std::tuple<bool, std::optional<std::string>>
{
	auto v = boost::json::parse(message);
    if (!v.is_object())
    {
        return { false, "invalid message: not a JSON object" };
    }
	auto obj = v.as_object();

	if (obj.if_contains("batch") && obj["batch"].is_array())
	{
		std::vector<std::string> sqls;
		for (auto& item : obj["batch"].as_array())
		{
            if (!item.is_object())
            {
                return { false, "batch item must be object" };
            }
			auto io = item.as_object();
			auto [ok, err, sql] = to_sql(io);
			if (!ok)
			{
				return { false, err };
			}
			sqls.push_back(sql);
		}
		return execute_batch(sqls);
	}

	auto [ok, err, sql] = to_sql(obj);
	if (!ok)
	{
		return { false, err };
	}
	return db_.execute_command(sql);
}

auto DbJobExecutor::to_sql(const boost::json::object& obj) -> std::tuple<bool, std::string, std::string>
{
	if (obj.if_contains("sql"))
	{
        if (!op_allowed("exec"))
        {
            return { false, "op not allowed: exec", "" };
        }
        return { true, "", boost::json::value_to<std::string>(obj.at("sql")) };
	}
	if (obj.if_contains("op") && obj.if_contains("table"))
	{
		auto op = boost::json::value_to<std::string>(obj.at("op"));
		auto table = boost::json::value_to<std::string>(obj.at("table"));
        if (!op_allowed(op))
        {
            return { false, std::string("op not allowed: ") + op, "" };
        }
        if (!table_allowed(table))
        {
            return { false, std::string("table not allowed: ") + table, "" };
        }
		if (op == "insert")
		{
			auto values = obj.if_contains("values") && obj.at("values").is_object() ? obj.at("values").as_object() : boost::json::object{};
            return { true, "", build_insert_sql(table, values) };
		}
		if (op == "update")
		{
			auto values = obj.if_contains("values") && obj.at("values").is_object() ? obj.at("values").as_object() : boost::json::object{};
			auto where = obj.if_contains("where") && obj.at("where").is_object() ? obj.at("where").as_object() : boost::json::object{};
            return { true, "", build_update_sql(table, values, where) };
		}
		if (op == "delete")
		{
			auto where = obj.if_contains("where") && obj.at("where").is_object() ? obj.at("where").as_object() : boost::json::object{};
            return { true, "", build_delete_sql(table, where) };
		}
        return { false, std::string("unsupported op: ") + op, "" };
	}
    return { false, "unsupported message format", "" };
}

auto DbJobExecutor::execute_batch(const std::vector<std::string>& sqls) -> std::tuple<bool, std::optional<std::string>>
{
	auto [ok_begin, err_begin] = db_.execute_command("BEGIN;");
	if (!ok_begin)
	{
		return { false, err_begin };
	}
	for (const auto& s : sqls)
	{
		auto [ok, err] = db_.execute_command(s);
		if (!ok)
		{
			db_.execute_command("ROLLBACK;");
			return { false, err };
		}
	}
	auto [ok_commit, err_commit] = db_.execute_command("COMMIT;");
	if (!ok_commit)
	{
		return { false, err_commit };
	}
	return { true, std::nullopt };
}

auto DbJobExecutor::op_allowed(const std::string& op) const -> bool
{
    if (allowed_ops_.empty())
	{
		return true;
	}
    for (const auto& a : allowed_ops_)
	{
		if (a == op)
		{
			return true;
		}
	}
	return false;
}

auto DbJobExecutor::table_allowed(const std::string& table) const -> bool
{
    if (allowed_tables_.empty())
	{
		return true;
	}
    for (const auto& t : allowed_tables_)
	{
		if (t == table)
		{
			return true;
		}
	}
	return false;
}

auto DbJobExecutor::quote_identifier(const std::string& ident) const -> std::string
{
	return "\"" + ident + "\"";
}

auto DbJobExecutor::json_value_to_sql_literal(const boost::json::value& v) const -> std::string
{
	if (v.is_null())
	{
		return "NULL";
	}
	if (v.is_bool())
	{
		return v.as_bool() ? "TRUE" : "FALSE";
	}
	if (v.is_int64())
	{
		return std::to_string(v.as_int64());
	}
	if (v.is_uint64())
	{
		return std::to_string(v.as_uint64());
	}
	if (v.is_double())
	{
		return std::to_string(v.as_double());
	}
	if (v.is_string())
	{
		auto s = boost::json::value_to<std::string>(v);
		return "'" + db_.escape_string(s) + "'";
	}
	auto s = boost::json::serialize(v);
	return "'" + db_.escape_string(s) + "'";
}

auto DbJobExecutor::build_where_clause(const boost::json::object& where) const -> std::string
{
	if (where.empty())
	{
		return "";
	}
	std::string clause = " WHERE ";
	bool first = true;
	for (auto& kv : where)
	{
		if (!first)
		{
			clause += " AND ";
		}
		first = false;
        clause += quote_identifier(std::string(kv.key().data(), kv.key().size()));
		clause += " = ";
		clause += json_value_to_sql_literal(kv.value());
	}
	return clause;
}

auto DbJobExecutor::build_insert_sql(const std::string& table, const boost::json::object& values) const -> std::string
{
	std::string sql = "INSERT INTO ";
	sql += quote_identifier(table);
	sql += " (";
	bool first = true;
	for (auto& kv : values)
	{
		if (!first)
		{
			sql += ",";
		}
		first = false;
        sql += quote_identifier(std::string(kv.key().data(), kv.key().size()));
	}
	sql += ") VALUES (";
	first = true;
	for (auto& kv : values)
	{
		if (!first)
		{
			sql += ",";
		}
		first = false;
		sql += json_value_to_sql_literal(kv.value());
	}
	sql += ");";
	return sql;
}

auto DbJobExecutor::build_update_sql(const std::string& table, const boost::json::object& values, const boost::json::object& where) const -> std::string
{
	std::string sql = "UPDATE ";
	sql += quote_identifier(table);
	sql += " SET ";
	bool first = true;
	for (auto& kv : values)
	{
		if (!first)
		{
			sql += ",";
		}
		first = false;
        sql += quote_identifier(std::string(kv.key().data(), kv.key().size()));
		sql += " = ";
		sql += json_value_to_sql_literal(kv.value());
	}
	sql += build_where_clause(where);
	sql += ";";
	return sql;
}

auto DbJobExecutor::build_delete_sql(const std::string& table, const boost::json::object& where) const -> std::string
{
	std::string sql = "DELETE FROM ";
	sql += quote_identifier(table);
	sql += build_where_clause(where);
	sql += ";";
	return sql;
}
