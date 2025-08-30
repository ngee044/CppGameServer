#include "Logger.h"
#include "ArgumentParser.h"
#include "Configurations.h"
#include "CacheDBService.h"

using namespace Utilities;

auto main(int argc, char* argv[]) -> int
{
	Logger::handle().start("CacheDBService");

	ArgumentParser args(argc, argv);
	Configurations cfg(std::move(args));

	CacheDBService svc(cfg);
	auto [ok, err] = svc.start();
	if (!ok)
	{
		Logger::handle().write(LogTypes::Error, err.value_or("start failed"));
		return 1;
	}

	Logger::handle().write(LogTypes::Information, "CacheDBService is running. Press Ctrl+C to stop.");
	// For simplicity, block main thread; stop via external signal by operator (Ctrl+C)
	// In future, signal handling can set a global flag and call svc.stop()
	svc.wait_stop();
	Logger::handle().write(LogTypes::Information, "CacheDBService is stopping...");
	svc.stop();

	Logger::handle().stop();
	Utilities::Logger::destroy();
	return 0;
}
