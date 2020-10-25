#pragma once
/*log header*/
#ifndef log_H
#define log_H
#include <string>
#include "string_operation.hpp"
using namespace std;
/*verbose default setting*/
    /*LOG MARCO*/
    enum TRACE_LOG_LEVEL {
        LOG_LEVEL_NONE = 0L,
        LOG_LEVEL_ERROR = 1L,
        LOG_LEVEL_WARNING = 2L,
        LOG_LEVEL_NOTICE = 3L,
        LOG_LEVEL_INFO = 4L,
        LOG_LEVEL_DEBUG = 5L,
        LOG_LEVEL_INTERNAL = 6L,
        LOG_LEVEL_TRACE = 10L,
        LOG_LEVEL_RECORD = -1L //a special log level, it means something must be recorded
    };
    extern int global_verbose;
    class log
    {
    public:
        static string trace_log_header(TRACE_LOG_LEVEL level) {
            time_t ltm = time(NULL);
            char ch;
            switch (level) {
            case LOG_LEVEL_ERROR:     ch = 'E';    break;
            case LOG_LEVEL_WARNING:   ch = 'W';    break;
            case LOG_LEVEL_NOTICE:    ch = 'N';    break;
            case LOG_LEVEL_INFO:      ch = 'I';    break;
            case LOG_LEVEL_DEBUG:     ch = 'D';    break;
            case LOG_LEVEL_INTERNAL:  ch = 'L';    break;
            case LOG_LEVEL_TRACE:     ch = 'T';    break;
            case LOG_LEVEL_RECORD:    ch = 'R';    break;
            default:                  ch = 'U';
            };
            struct tm* tm = localtime(&ltm);
            return string_op::strprintf(
                "%4d-%02d-%02d %02d:%02d:%02d (T%04u) |%c|",
                tm->tm_year + 1900,
                tm->tm_mon + 1,
                tm->tm_mday,
                tm->tm_hour,
                tm->tm_min,
                tm->tm_sec,
                pthread_self(),
                ch
            );
        }
    };

#define G_LEVEL global_verbose

#define _LOG( level, g_level, fmt, ... )                                                              \
    do{                                                                                      \
        if ( level <= g_level){                \
            string log = log::trace_log_header( level );                                       \
            log.append( string_op::strprintf( fmt, ##__VA_ARGS__ ) );                  \
            if ( log.length() && log[log.length() - 1] != '\n' )                         \
                log.append("\n");                                                        \
            printf("%s", log.c_str() );                                                \
        }                                                                                    \
    }while(0)    


#define LOG( level, fmt, ... )    _LOG( level, G_LEVEL, fmt, ##__VA_ARGS__ ) 

#define VALIDATE

#define FUNC_TRACER \
	auto_log logFun(string_op::strprintf("%s(%d) : %s :", __FILE__, __LINE__, __FUNCTION__));
#define FUN_TRACE FUNC_TRACER

#define LOG_ERROR(fmt, ...) \
	_LOG(LOG_LEVEL_ERROR, G_LEVEL,"%s(%d) : %s :" fmt, __FILE__, __LINE__, __FUNCTION__,##__VA_ARGS__);
#define LOG_WARNING(fmt, ...) \
	_LOG(LOG_LEVEL_WARNING, G_LEVEL,"%s(%d) : %s :" fmt, __FILE__, __LINE__, __FUNCTION__,##__VA_ARGS__);
#define LOG_NOTICE(fmt, ...) \
	_LOG(LOG_LEVEL_NOTICE, G_LEVEL,"%s(%d) : %s :" fmt, __FILE__, __LINE__, __FUNCTION__,##__VA_ARGS__);
#define LOG_INFO(fmt, ...) \
	_LOG(LOG_LEVEL_INFO, G_LEVEL,"%s(%d) : %s :" fmt, __FILE__, __LINE__, __FUNCTION__,##__VA_ARGS__);
#define LOG_DEBUG(fmt, ...) \
	_LOG(LOG_LEVEL_DEBUG, G_LEVEL,"%s(%d) : %s :" fmt, __FILE__, __LINE__, __FUNCTION__,##__VA_ARGS__);
#define LOG_INTERNAL(fmt, ...) \
	_LOG(LOG_LEVEL_INTERNAL, G_LEVEL,"%s(%d) : %s :" fmt, __FILE__, __LINE__, __FUNCTION__,##__VA_ARGS__);
#define LOG_TRACE(fmt, ...) \
	_LOG(LOG_LEVEL_TRACE, G_LEVEL,"%s(%d) : %s :" fmt, __FILE__, __LINE__, __FUNCTION__,##__VA_ARGS__);
#define LOG_RECORD(fmt, ...) \
	_LOG(LOG_LEVEL_RECORD, G_LEVEL,"%s(%d) : %s :" fmt, __FILE__, __LINE__, __FUNCTION__,##__VA_ARGS__);

    class auto_log {
    public:
        auto_log(string message) :__message(message) {
            LOG_INTERNAL("%s - Entered.", __message.c_str());
        }
        ~auto_log() {
            LOG_INTERNAL("%s - Exit.", __message.c_str());
        }
    private:
        string __message;
    };

#endif