#pragma once
#ifndef string_operation_H
#define string_operation_H
#include <string>
#include <vector>
#include <stdarg.h>

#define REMOVE_WITESPACE(_str_) string_op::remove_trailing_whitespace(string_op::remove_begining_whitespace(_str_))

using namespace std;
class string_op
{
public:
    static std::string strprintf(const string fmt, ...);
    static std::string remove_trailing_whitespace(std::string input);
    static std::string remove_begining_whitespace(std::string input);
    static std::vector<std::string> tokenize2(const std::string& str, const std::string& delimiters, size_t num, bool empty = true);
    static std::vector<std::string> tokenize(const std::string& str, const std::string& delimiters, size_t num, bool empty = true);
    static std::string parser_double_quotation_mark(const std::string& str);

};
#endif