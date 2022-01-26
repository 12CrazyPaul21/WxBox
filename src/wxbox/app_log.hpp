#ifndef __WXBOX_APP_LOG_H
#define __WXBOX_APP_LOG_H

#ifdef SPDLOG_ACTIVE_LEVEL
#undef SPDLOG_ACTIVE_LEVEL
#endif

#if _DEBUG
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#define SPDLOG_TRACE_ON
#define SPDLOG_DEBUG_ON
#else
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_INFO
#undef SPDLOG_TRACE_ON
#undef SPDLOG_DEBUG_ON
#endif

#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>

#endif  // #ifndef __WXBOX_APP_LOG_H