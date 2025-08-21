#include "MainDbServiceApp.h"

#include <csignal>
#include <chrono>
#include <thread>

using namespace RabbitMQ;

MainDbServiceApp::MainDbServiceApp(const Configurations& cfg, DbJobExecutor& executor)
	: cfg_(cfg)
	, executor_(executor)
	, consumer_(cfg.rabbit_mq_host(), cfg.rabbit_mq_port(), cfg.rabbit_mq_user_name(), cfg.rabbit_mq_password())
	, stop_flag_(false)
{
}

auto MainDbServiceApp::start() -> std::tuple<bool, std::optional<std::string>>
{
	auto [started, start_err] = consumer_.start();
	if (!started)
	{
		return { false, start_err };
	}

	auto [connected, conn_err] = consumer_.connect(cfg_.rabbit_heartbeat());
	if (!connected)
	{
		return { false, conn_err };
	}

	auto [opened, open_err] = consumer_.channel_open(cfg_.rabbit_channel_id(), cfg_.consume_queue_name());
	if (!opened.has_value())
	{
		return { false, open_err };
	}

	auto [prepared, prep_err] = consumer_.prepare_consume();
	if (!prepared)
	{
		return { false, prep_err };
	}

	auto cb = [this](const std::string&, const std::string& body, const std::string&) -> std::tuple<bool, std::optional<std::string>>
	{
		return executor_.handle_message(body);
	};
	auto [registered, reg_err] = consumer_.register_consume(cfg_.rabbit_channel_id(), cfg_.consume_queue_name(), cb);
	if (!registered)
	{
		return { false, reg_err };
	}

	return consumer_.start_consume();
}

void MainDbServiceApp::run_until_signal()
{
	std::signal(SIGINT, &MainDbServiceApp::on_signal);
	std::signal(SIGTERM, &MainDbServiceApp::on_signal);

	while (!stop_flag_.load())
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(200));
	}
}

auto MainDbServiceApp::stop() -> void
{
	consumer_.stop_consume();
	consumer_.channel_close();
	consumer_.disconnect();
}

void MainDbServiceApp::on_signal(int)
{
	instance()->stop_flag_.store(true);
}

auto MainDbServiceApp::instance(MainDbServiceApp* inst) -> MainDbServiceApp*
{
	static MainDbServiceApp* self = nullptr;
	if (inst != nullptr)
	{
		self = inst;
	}
	return self;
}
