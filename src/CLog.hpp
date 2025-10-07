#pragma once

#include <samplog/samplog.hpp>
#include "CSingleton.hpp"
#include "CError.hpp"

#include <fmt/format.h>

using samplog::PluginLogger_t;
using samplog::LogLevel;
using samplog::AmxFuncCallInfo;

class CLog : public CSingleton<CLog>
{
	friend class CSingleton<CLog>;
	friend class CScopedDebugInfo;
private:
	CLog() :
		m_Logger("mysql")
	{ }
	~CLog() = default;

public:
	inline bool IsLogLevel(LogLevel level)
	{
		return m_Logger.IsLogLevel(level);
	}

	template<typename... Args>
	inline void Log(LogLevel level, const char *format, Args &&...args)
	{
		if (!IsLogLevel(level))
			return;

		string str = format;
		if (sizeof...(args) != 0)
			str = fmt::format(format, std::forward<Args>(args)...);

		m_Logger.Log(level, str.c_str());
	}

	// should only be called in native functions
	template<typename... Args>
	void LogNative(LogLevel level, const char *fmt, Args &&...args)
	{
		if (!IsLogLevel(level))
			return;

		string msg = fmt::format("{}", fmt::format(fmt, std::forward<Args>(args)...));
		Log(level, msg.c_str());
	}

	template<typename T>
	inline void LogNative(const CError<T> &error)
	{
		LogNative(LogLevel::ERROR, "{} error: {}",
				  error.module(), error.msg());
	}

private:
	PluginLogger_t m_Logger;

};
