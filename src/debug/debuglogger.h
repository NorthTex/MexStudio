#ifndef Header_DebugLogger
#define Header_DebugLogger

#include "mostQtHeaders.h"
#include "Debug/Logger.hpp"

#ifdef DEBUG_LOGGER

// #define debugLoggerStart(pathname) DebugLogger::start(pathname)
// #define debugLoggerStop() DebugLogger::stop()
// #define debugLoggerIsLogging() DebugLogger::isLogging()

#define DLOG_DEBUG(message) DebugLogger::logMessage(QtDebugMsg, __FILE__, __LINE__, message)
#define DLOG_INFO(message) DebugLogger::logMessage(QtInfoMsg, __FILE__, __LINE__, message)
#define DLOG_WARNING(message) DebugLogger::logMessage(QtWarningMsg, __FILE__, __LINE__, message)
#define DLOG_CRITICAL(message) DebugLogger::logMessage(QtCriticalMsg, __FILE__, __LINE__, message)
#define DLOG_FATAL(message) DebugLogger::logMessage(QtFatalMsg, __FILE__, __LINE__, message)


#else // DEBUG_LOGGER

#define debugLoggerStart(pathname)
#define debugLoggerStop()
#define debugLoggerIsLogging() false

#define DLOG_DEBUG(message)
#define DLOG_INFO(message)
#define DLOG_WARNING(message)
#define DLOG_CRITICAL(message)
#define DLOG_FATAL(message)

#endif // DEBUG_LOGGER

#endif // DEBUGLOGGER_H
