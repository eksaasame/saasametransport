#include "log.h"
#include <fcntl.h>
#include <unistd.h>
#include <stdexcept>

int global_verbose = 10;
bool b_out_to_file = false;
bool b_flush_i = true;
bool b_go_to_dmesg_i = false;

string execute_command_(const char* cmd)
{
    char buffer[128];
    std::string result = "";
    FILE* pipe = popen(cmd, "r");
    if (!pipe) throw std::runtime_error("popen() failed!");
    try {
        while (!feof(pipe)) {
            if (fgets(buffer, 128, pipe) != NULL)
                result += buffer;
        }
    }
    catch (...) {
        pclose(pipe);
        throw;
    }
    pclose(pipe);
    //LOG_TRACE("result = %s\r\n", result.c_str());
    return result;
}


const int stdoutfd = dup(fileno(stderr));

int redirect_stdout(const char* fname) {
    fflush(stderr);
    int newstdout = open(fname, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    printf("stdoutfd = %d", newstdout);
    dup2(newstdout, fileno(stderr));
    close(newstdout);
    //sleep(1);
}

int restore_stdout() {
    fflush(stderr);
    dup2(stdoutfd, fileno(stderr));
    //close(stdoutfd);
    return stdoutfd;
}


string log::trace_log_header(TRACE_LOG_LEVEL level) {
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