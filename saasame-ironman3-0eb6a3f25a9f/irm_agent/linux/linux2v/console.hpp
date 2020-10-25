#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <boost/format.hpp>
#include <iostream>


std::string read_from_file(const boost::filesystem::path file_path, uint64_t offset = 0, uint32_t size = 0) {
    std::ifstream file(file_path.string(), std::ios::in | std::ios::binary);
    std::string result, str;
    if (file.is_open()) {
        if (0 == size) {
            while (std::getline(file, str)) {
                if (!result.empty())
                    result.append("\n");
                result.append(str);
            }
        }
        else {
            result.resize(size);
            file.seekg(offset, std::ios::beg);
            file.read(&result[0], size);
        }
        file.close();
    }
    return result;
}

std::vector<std::string> tokenize(const std::string& str, const std::string& delimiters, size_t num = 0, bool empty = true) {
    std::vector<std::string> tokens;
    std::string::size_type delimPos = 0, tokenPos = 0, pos = 0;
    if (str.empty())
    {
        if (empty)
            tokens.push_back("");
        return tokens;
    }
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


std::string erase_trailing(const std::string& str, const std::string delimiters = " \t\f\v\n\r") {
    size_t found;
    std::string strTemp;
    strTemp = str;
    if (!strTemp.length()) {
        strTemp.clear();
    }
    else {
        found = strTemp.find_last_not_of(delimiters);
        if (found != std::string::npos)
            strTemp.erase(found + 1);
        else
            strTemp.clear();
    }
    return strTemp;
}

std::string remove_begining(const std::string& str, const std::string delimiters = " \t\f\v\n\r") {
    size_t found;
    std::string strTemp;
    strTemp = str;
    if (!strTemp.length()) return strTemp;

    found = strTemp.find_first_not_of(delimiters);

    if (found != 0) {
        if (found != std::string::npos) {
            strTemp = strTemp.substr(found, strTemp.length() - found);
        }
        else
            strTemp.clear();
    }
    return strTemp;
}

std::string strip(const std::string &str, const std::string delimiters = " \t\f\v\n\r") {
    return remove_begining(erase_trailing(str, delimiters), delimiters);
}

class console {
public:
    static std::string execute(const std::string command){
        std::stringstream ret;
        if (!command.empty()) {
            FILE *fp;
            char path[1035];
            /* Open the command for reading. */
            fp = popen(command.c_str(), "r");
            if (fp == NULL) {
                printf("Failed to run command\n");
            }
            else {
                /* Read the output a line at a time - output it. */
                while (fgets(path, sizeof(path)-1LL, fp) != NULL) {
                    ret << boost::str(boost::format("%s") % path);
                }
            }
            /* close */
            pclose(fp);
        }
        return ret.str();
    }
};