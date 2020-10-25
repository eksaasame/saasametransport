#pragma once
#ifndef string_operation_H
#define string_operation_H
#include <string>
#include <stdarg.h>

using namespace std;
class string_op
{
public:
    static std::string strprintf(const string fmt, ...)
    {
        int final_n, n = fmt.size() * 2;
        unique_ptr<char[]> formatted;
        va_list ap;
        while (1)
        {
            formatted.reset(new char[n]);
            strcpy(&formatted[0], fmt.c_str());
            va_start(ap, fmt);
            final_n = vsnprintf(&formatted[0], n, fmt.c_str(), ap);
            va_end(ap);
            if (final_n < 0 || final_n >= n)
                n += abs(final_n - n + 1);
            else
                break;
        }
        return string(formatted.get());
    }
};
#endif