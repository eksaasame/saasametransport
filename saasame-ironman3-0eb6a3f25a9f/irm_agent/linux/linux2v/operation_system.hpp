#pragma once
#include "console.hpp"
#include <boost/filesystem.hpp>
#include <boost/thread.hpp>
#include <string.h>

class operation_system {
public:
    typedef boost::shared_ptr<operation_system> ptr;
    static operation_system::ptr get(boost::filesystem::path etc = "/etc");

    std::string distribution() const {
        if (_distribution == "Red" || _distribution == "RedHat")
            return "RedHat";
        else if (_distribution == "SLES" || _distribution == "SUSE" || _distribution == "SuSE")
            return "SuSE";
        return _distribution;
    }

    std::string os_family() const {
        if (_distribution == "Red" || _distribution == "CentOS" || _distribution == "RedHat")
            return "RedHat";
        else if (_distribution == "Debian" || _distribution == "Ubuntu")
            return "Ubuntu";
        else if (_distribution == "SLES" || _distribution == "SUSE" || _distribution == "SuSE")
            return "SuSE";
        return "";
    }

    std::string major() const {
        return _version.empty() ? "" : tokenize(_version, ".", 0, false)[0];
    }

    std::string minor() const {
        std::vector<std::string> versions = tokenize(_version, ".", 0, false);
        return versions.size() > 1 ? versions[1] : "";
    }

    std::string version() const {
        std::vector<std::string> versions = tokenize(_version, ".", 0, false);
        if (versions.size() > 1)
            return boost::str(boost::format("%s.%s") % versions[0] % versions[1]);
        else if (versions.size() > 0)
            return boost::str(boost::format("%s.0") % versions[0]);
        return "";
    }

private:
    bool detect_os_version(boost::filesystem::path etc);
    bool _parse_redhat_release(boost::filesystem::path content);
    bool _parse_ubuntu_release(boost::filesystem::path content);
    bool _parse_SuSE_release(boost::filesystem::path content);
    bool _parse_lsb_release(boost::filesystem::path content);

    operation_system() {
    }
    std::string _distribution;
    std::string _version;
    std::string _label;
};

bool operation_system::_parse_redhat_release(boost::filesystem::path content) {
    bool result = false;
    std::vector<std::string> labels = tokenize(tokenize(read_from_file(content), "\n")[0], "(");
    std::vector<std::string> elements = tokenize(labels[0], " ");
    _distribution = elements[0];
    std::vector<std::string> values = tokenize(elements[elements.size() - 1], "()", 0, false);
    _label = values.empty() ? "" : values[0];
    if (labels.size() > 1) {
        if (elements[elements.size() - 2] == "4") {
            values = tokenize(labels[1], "()", 0, false);
            _version = boost::str(boost::format("%s.%s") % elements[elements.size() - 2] % values[values.size()-1]);
        }
        else {
            _version = elements[elements.size() - 2];
        }
        result = true;
    }
    else {
        _version = elements[elements.size() - 2];
        result = true;
    }
    return result;
}

bool operation_system::_parse_ubuntu_release(boost::filesystem::path content) {
    bool result = false;
    for (std::string line : tokenize(read_from_file(content), "\n")) {
        if (0 == line.find("VERSION_ID=")) {
            _label = strip(tokenize(line, "\"", 0, false)[1]);
            std::string versionrelease = _label;
            if (std::string::npos != _label.find(",")) {
                versionrelease = tokenize(_label, ",", 0, false)[1];
            }
            else if (std::string::npos != _label.find("(")) {
                versionrelease = tokenize(_label, "(", 0, false)[1];
            }
            std::vector<std::string> values = tokenize(versionrelease, " ", 0, false);
            _version = values[0];
            result = true;
        }
        else if (0 == line.find("NAME=")) {
            _distribution = tokenize(line, "\"", 0, false)[1];
        }
    }
    return result;
}

bool operation_system::_parse_SuSE_release(boost::filesystem::path content) {
    bool result = false;
    for (std::string line : tokenize(read_from_file(content), "\n")) {
        if (0 == line.find("SUSE")) {
            _distribution = "SuSE";
        }
        else if (0 == line.find("openSUSE")) {
            _distribution = "openSUSE";
        }
        else if (0 == line.find("VERSION =")) {
            _version = strip(tokenize(line, "=", 0, false)[1]);
            result = true;
        }
        else if (0 == line.find("PATCHLEVEL =")) {
            std::string patch = strip(tokenize(line, "=", 0, false)[1]);
            _version = boost::str(boost::format("%s.%s") % _version % patch);
            result = true;
        }
    }
    return result;
}

bool operation_system::_parse_lsb_release(boost::filesystem::path content) {
    bool result = false;
    for (std::string line : tokenize(read_from_file(content), "\n")) {
        if (0 == line.find("DISTRIB_ID=")) {
            _distribution = tokenize(line, "=")[1];
        }
        else if (0 == line.find("DISTRIB_RELEASE=")) {
            _version = tokenize(tokenize(line, "=")[1], " ")[0];
            result = true;
        }
        else if (0 == line.find("DISTRIB_DESCRIPTION=")) {
            _label = strip(tokenize(line, "\"")[1]);
        }
    }
    return result;
}

bool operation_system::detect_os_version(boost::filesystem::path etc) {
    if (boost::filesystem::exists(etc / "redhat-release"))
        return _parse_redhat_release(etc / "redhat-release");
    else if (boost::filesystem::exists(etc / "os-release"))
        return _parse_ubuntu_release(etc / "os-release");
    else if (boost::filesystem::exists(etc / "SuSE-release"))
        return _parse_SuSE_release(etc / "SuSE-release");
    else if (boost::filesystem::exists(etc / "lsb-release"))
        return _parse_lsb_release(etc / "lsb-release");
    return false;
}

operation_system::ptr operation_system::get(boost::filesystem::path etc) {
    operation_system::ptr o(new operation_system());
    if (o->detect_os_version(etc))
        return o;
    return operation_system::ptr();
}
