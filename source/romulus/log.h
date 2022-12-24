#pragma once

enum LogLevel
{
    LOG_TRACE, // Shows the steps though an execution path, you almost never want this
    LOG_DEBUG, // Used to help debug problems, more technical/detailed than INFO
    LOG_INFO, // General program flow/informational messages, things like this subsystem started, loaded "x" rom, etc.
    LOG_WARN, // An unexpected situation occurred that the program can continue to operate in, but may cause problems
    LOG_ERROR, // An error that will cause failure of a given subsystem, but processing can continue or recover
    LOG_FATAL, // An error that cannot be recovered from and will cause either a halt or unknowable state

    LOG_NUM_LEVELS,

    LOG_DISABLED // For conveinience: setting minimium log level to this will kill all log processing
};

// Logs will only process/output above this level
// TODO: Have some general way to control this 
static LogLevel minimumLogLevel = LOG_INFO;

// Writes the message directly out to the log streams with just level appended
void logRaw(LogLevel level, const char* message);

// For any lines longer than this the variadic functions will fail
// And not necessarily in a pretty way (allocates a static temp buffer of x bytes)
#define MAX_LOG_LINE 1024

void log(LogLevel level, const char* message, ...);
void logTrace(const char* message, ...);
void logDebug(const char* message, ...);
void logInfo(const char* message, ...);
void logWarn(const char* message, ...);
void logError(const char* message, ...);
void logFatal(const char* message, ...);
