/**
 * @file log.h
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2023-11-26
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include <map>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

#include "singleton.h"

namespace amot {

class LoggerManager {
public:
	using LogPtr = std::shared_ptr<spdlog::logger>;
	LoggerManager() {
		m_root = spdlog::stdout_color_mt("root");
		m_root->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%^%l%$] [thread %t] : %v");
		m_loggers["root"] = m_root;

		Init();
	}

	LogPtr GetLogger(const std::string& name) {
		std::lock_guard<std::mutex> locker(m_mutex);
		auto it = m_loggers.find(name);
		if(it != m_loggers.end()) {
			return it->second;
		}

		LogPtr logger = spdlog::stdout_color_mt(name);
		m_loggers[name] = logger;
		return logger;
	}

	LogPtr GetFileLogger(const std::string& name, std::string path) {
		std::lock_guard<std::mutex> locker(m_mutex);
		auto it = m_loggers.find(name);
		if(it != m_loggers.end()) {
			return it->second;
		}

		LogPtr logger = spdlog::basic_logger_mt(name, path, true);
		logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%^%l%$] [thread %t] : %v");
		m_loggers[name] = logger;
		return logger;
	}

	void Init() {  }

	LogPtr GetRoot() const { return m_root; }

private:
	std::map<std::string, LogPtr> m_loggers;
	LogPtr m_root;

	std::mutex m_mutex;
};

// 日志器管理类单例模式
using LoggerMgr =  amot::Singleton<LoggerManager>;
}