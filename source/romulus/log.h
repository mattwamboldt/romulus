#pragma once

enum LogLevel
{
    LOG_TRACE,
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
    LOG_FATAL
};

// Writes the message directly out to the log streams with just level appended
void logRaw(LogLevel level, const char* message);

// For any lines longer than this the variadic functions will fail
// And not necessarily in a pretty way
#define MAX_LOG_LINE 1024

void log(LogLevel level, const char* message, ...);
void logTrace(const char* message, ...);
void logDebug(const char* message, ...);
void logInfo(const char* message, ...);
void logWarn(const char* message, ...);
void logError(const char* message, ...);
void logFatal(const char* message, ...);

