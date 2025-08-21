// Main: minimal wiring using split classes

#include "Logger.h"
#include "ArgumentParser.h"
#include "Configurations.h"
#include "DbJobExecutor.h"
#include "MainDbServiceApp.h"
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
	auto [db_ok, db_msg] = db.execute_command("SELECT 1;");
	if (!db_ok)
	{
		Logger::handle().write(LogTypes::Error, fmt::format("database connection failed: {}", db_msg.value_or("unknown")));
		return 1;
	}

	DbJobExecutor executor(db, cfg.allowed_ops(), cfg.allowed_tables());
	MainDbServiceApp app(cfg, executor);
	MainDbServiceApp::instance(&app);

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
