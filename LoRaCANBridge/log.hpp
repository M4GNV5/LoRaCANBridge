#pragma once

// TODO transform this into an Arduino library and make it available via the library manager

#include <Arduino.h>

extern Print *logger;

typedef enum
{
	LOG_OFF,
	LOG_ERROR,
	LOG_WARNING,
	LOG_INFO,
	LOG_DEBUG,
} LogLevel;

void logMessagePrefix(LogLevel level, const char *file, int line)
{
	static const char * const levels[] = {
		"OFF",
		"ERROR",
		"WARNING",
		"INFO",
		"DEBUG",
	};

	logger->print('[');

	if(level < LOG_OFF || level > LOG_DEBUG)
		logger->print("???");
	else
		logger->print(levels[level]);

	logger->print("] [");
	logger->print(strrchr(file, '/') + 1);
	logger->print(':');
	logger->print(line);
	logger->print("] ");
}

inline void logMessagePart()
{
}

template<typename T, typename... Args>
inline void logMessagePart(T curr, Args... rest)
{
	logger->print(curr);
	logMessagePart(rest...);
}

template<typename... Args>
inline void logMessage(LogLevel level, const char *file, int line, Args... args)
{
	if(level > LOG_LEVEL)
		return;

	logMessagePrefix(level, file, line);
	logMessagePart(args...);
	logger->println();
}
#define LOG(level, ...) logMessage(LOG_##level, __FILE__, __LINE__, __VA_ARGS__)
