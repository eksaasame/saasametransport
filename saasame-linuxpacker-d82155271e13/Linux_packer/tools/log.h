#pragma once
/*log header*/
#ifndef log_H
#define log_H
#include <string>
#include "string_operation.h"
#include <iostream>

extern int global_verbose;
extern bool b_out_to_file;
extern bool b_flush_i;
extern bool b_go_to_dmesg_i;
extern const int stdoutfd;
using namespace std;

int redirect_stdout(const char* fname);
int restore_stdout();
string execute_command_(const char* cmd);
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
            static string trace_log_header(TRACE_LOG_LEVEL level);
    };

#define G_LEVEL global_verbose

#define _LOG( level, g_level, fmt, ... )                                                              \
    do{                                                                                      \
        if ( level <= g_level){                \
            string log = log::trace_log_header( level );                                       \
            log.append( string_op::strprintf( fmt, ##__VA_ARGS__ ));                  \
            string klog = log;                                                          \
            if ( log.length() && log[log.length() - 1] != '\n' )                         \
                log.append("\n");                                                        \
            fprintf(stderr,"%s",log.c_str());                                            \
            if(b_flush_i){fflush(stderr);}                                               \
            if(b_go_to_dmesg_i){string echo = "echo \""+klog+"\" > /dev/kmsg";execute_command_(echo.c_str());}\
        }                                                                                \
    }while(0)    


#define LOG( level, fmt, ... )    _LOG( level, G_LEVEL, fmt, ##__VA_ARGS__ ) 

#define VALIDATE

#define ENABLE_FUNC_TRACER 1
#if ENABLE_FUNC_TRACER
#define FUNC_TRACER \
	auto_log logFun(string_op::strprintf("%s(%d) : %s :", __FILE__, __LINE__, __FUNCTION__));
#else
#define FUNC_TRACER
#endif
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
#define LOG_FLUSH_IMMEDIATELY do{ b_flush_i=true;clog.flush();}while(0)    
#define LOG_FLUSH_BUFFERED do{ b_flush_i=false;}while(0)   


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

#define SET_LOG_OUTPUT(filename)       \
do{                                    \
    std::ofstream out(filename);       \
    std::clog.rdbuf(out.rdbuf());      \
}while(0)

#endif