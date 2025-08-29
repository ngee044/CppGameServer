// Main: minimal wiring using split classes

#include "Logger.h"
#include "ArgumentParser.h"
#include "Configurations.h"
#include "DbJobExecutor.h"
#include "MainDBService.h"
#include "PostgresDB.h"
#include "fmt/format.h"

using namespace Utilities;
using namespace Database;

auto main(int argc, char* argv[]) -> int
{
	Logger::handle().start("MainDBService");

	Utilities::ArgumentParser arguments(argc, argv);
	Configurations cfg(std::move(arguments));

	PostgresDB db(cfg.postgres_conn());
	// Health check: perform a simple SELECT and ensure we can fetch tuples
	auto [db_result, db_msg] = db.execute_query_and_get_result("SELECT 1;");
	if (!db_result.has_value())
	{
		Logger::handle().write(LogTypes::Error, fmt::format("database connection failed: {}", db_msg.value_or("unknown")));
		return 1;
	}

	DbJobExecutor executor(db, cfg.allowed_ops(), cfg.allowed_tables());
	MainDBService app(cfg, executor);
	MainDBService::instance(&app);

	auto [ok, err] = app.start();
	if (!ok)
	{
		Logger::handle().write(LogTypes::Error, fmt::format("consumer start failed: {}", err.value_or("unknown")));
		return 1;
	}

	Logger::handle().write(LogTypes::Information, "MainDBService is running. Press Ctrl+C to stop.");
	app.run_until_signal();
	Logger::handle().write(LogTypes::Information, "MainDBService is stopping...");
	app.stop();

	Logger::handle().stop();
	Utilities::Logger::destroy();
	return 0;
}
