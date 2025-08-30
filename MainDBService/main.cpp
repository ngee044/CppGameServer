// Main: minimal wiring using split classes

#include "Logger.h"

#include <ArgumentParser.h>
#include <Configurations.h>
#include <DbJobExecutor.h>
#include <MainDBService.h>
#include <PostgresDB.h>

#include <fmt/format.h>

#include <memory>
#include <string>
#include <signal.h>

using namespace Utilities;
using namespace Database;

void register_signal(void);
void deregister_signal(void);
void signal_callback(int32_t signum);

std::shared_ptr<Configurations> configurations_ = nullptr;
std::shared_ptr<MainDBService> main_db_service_ = nullptr;

auto main(int argc, char* argv[]) -> int
{
	configurations_ = std::make_shared<Configurations>(ArgumentParser(argc, argv));

	Logger::handle().file_mode(configurations_->write_file());
	Logger::handle().console_mode(configurations_->write_console());
	Logger::handle().write_interval(configurations_->write_interval());
	Logger::handle().log_root(configurations_->log_root_path());

	Logger::handle().start(configurations_->service_title());

	PostgresDB db(configurations_->postgres_conn());
	auto [db_result, db_msg] = db.execute_query_and_get_result("SELECT 1;");
	if (!db_result.has_value())
	{
		Logger::handle().write(LogTypes::Error, fmt::format("database connection failed: {}", db_msg.value_or("unknown")));
		return -1;
	}

	auto executor = std::make_shared<DbJobExecutor>(db, configurations_->allowed_ops(), configurations_->allowed_tables());
	main_db_service_ = std::make_shared<MainDBService>(configurations_, executor);

	auto [ok, err] = main_db_service_->start();
	if (!ok)
	{
		Logger::handle().write(LogTypes::Error, fmt::format("consumer start failed: {}", err.value_or("unknown")));
		return -1;
	}

	Logger::handle().write(LogTypes::Information, "MainDBService is running");

	main_db_service_->wait_stop();
	main_db_service_.reset();

	configurations_.reset();

	Logger::handle().stop();
	Logger::destroy();

	return 0;
}

void register_signal(void)
{
	signal(SIGINT, signal_callback);
	signal(SIGILL, signal_callback);
	signal(SIGABRT, signal_callback);
	signal(SIGFPE, signal_callback);
	signal(SIGSEGV, signal_callback);
	signal(SIGTERM, signal_callback);
}

void deregister_signal(void)
{
	signal(SIGINT, nullptr);
	signal(SIGILL, nullptr);
	signal(SIGABRT, nullptr);
	signal(SIGFPE, nullptr);
	signal(SIGSEGV, nullptr);
	signal(SIGTERM, nullptr);
}

void signal_callback(int32_t signum)
{
	deregister_signal();

	if (main_db_service_ == nullptr)
	{
		return;
	}

	Logger::handle().write(LogTypes::Information, fmt::format("attempt to stop MainDBService from signal {}", signum));
	main_db_service_->stop();
}