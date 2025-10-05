#include "Logger.h"
#include "ArgumentParser.h"
#include "Configurations.h"
#include "CacheDBService.h"

#include "fmt/format.h"
#include "fmt/xchar.h"

#include <memory>
#include <signal.h>

using namespace Utilities;

void register_signal(void);
void deregister_signal(void);
void signal_callback(int32_t signum);

std::shared_ptr<Configurations> configurations_ = nullptr;
std::shared_ptr<CacheDBService> service_ = nullptr;

auto main(int argc, char* argv[]) -> int
{
	configurations_ = std::make_shared<Configurations>(ArgumentParser(argc, argv));

	// Apply logger configuration before starting
	Logger::handle().file_mode(configurations_->write_file());
	Logger::handle().console_mode(configurations_->write_console());
	Logger::handle().write_interval(static_cast<uint16_t>(configurations_->write_interval()));
	Logger::handle().log_root(configurations_->log_root_path());
	Logger::handle().start(configurations_->service_title());

	service_ = std::make_shared<CacheDBService>(configurations_);

	auto [started, error_message] = service_->start();
	if (!started)
	{
		Logger::handle().write(LogTypes::Error, error_message.value_or("failed to start CacheDBService"));
	}
	else
	{
		Logger::handle().write(LogTypes::Information, "CacheDBService started successfully");
		register_signal();
		service_->wait_stop();
	}

	service_.reset();
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
	if (service_ == nullptr)
	{
		return;
	}
	Logger::handle().write(LogTypes::Information, fmt::format("attempt to stop CacheDBService from signal {}", signum));
	service_->stop();
}
