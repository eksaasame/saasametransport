#include "string_operation.h"
#include <algorithm> 
#include <memory> 
#include <ctype.h>
#include <string.h>
std::string string_op::strprintf(const string fmt, ...)
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

std::string string_op::remove_trailing_whitespace(std::string input)
{
    input.erase(std::find_if(input.rbegin(), input.rend(), [](int ch) {
        return !std::isspace(ch);
    }).base(), input.end());
    return input;
}
std::string string_op::remove_begining_whitespace(std::string input)
{
    input.erase(input.begin(), std::find_if(input.begin(), input.end(), [](int ch) {
        return !std::isspace(ch);
    }));
    return input;
}

std::vector<std::string> string_op::tokenize2(const std::string& str, const std::string& delimiters, size_t num, bool empty) {

    std::vector<std::string> tokens;
    tokens = tokenize(str, delimiters, num, empty);

    for (size_t i = 0; i < tokens.size(); i++) {
        tokens[i] = remove_trailing_whitespace(tokens[i]);
        tokens[i] = remove_begining_whitespace(tokens[i]);
    }

    return tokens;
}


std::vector<std::string> string_op::tokenize(const std::string& str, const std::string& delimiters, size_t num, bool empty) {

    std::vector<std::string> tokens;
    std::string::size_type delimPos = 0, tokenPos = 0, pos = 0;

    if (str.length() < 1) return tokens;

    size_t count = 1;

    while (1) {
        delimPos = str.find_first_of(delimiters, pos);
        tokenPos = str.find_first_not_of(delimiters, pos);

        if ((count != num) && (std::string::npos != delimPos)) {
            if (std::string::npos != tokenPos) {
                if (tokenPos < delimPos) {
                    tokens.push_back(str.substr(pos, delimPos - pos));
                    count++;
                }
                else if (empty) {
                    tokens.push_back("");
                    count++;
                }
            }
            else if (empty) {
                tokens.push_back("");
                count++;
            }
            pos = delimPos + 1;
        }
        else {
            if (std::string::npos != tokenPos)
                tokens.push_back(str.substr(pos));
            else if (empty)
                tokens.push_back("");
            break;
        }
    }
    return tokens;
}

std::string string_op::parser_double_quotation_mark(const std::string& str)
{
    std::string strTemp = remove_trailing_whitespace(str);
    strTemp = remove_begining_whitespace(strTemp);

    if (strTemp.length() > 1 && strTemp[0] == '\"' && str[strTemp.length() - 1] == '\"')
        return strTemp.substr(1, strTemp.length() - 2);
    else
        return strTemp;
}