

#include "..\gen-cpp\saasame_mgmt_types.h"

namespace saasame { namespace mgmt {

bool disk_info::operator < (const disk_info &rhs) const{
    if (this == &rhs)
        return false;
    else
        return this->number < rhs.number;
}

bool partition_info::operator < (const partition_info &rhs) const{
    if (this == &rhs)
        return false;
    else{
        if (disk_number == rhs.disk_number)
            return partition_number < rhs.partition_number;
        else
            return disk_number < rhs.disk_number;
    }
}

bool volume_info::operator < (const volume_info &rhs) const{
    if (this == &rhs)
        return false;
    else
        return this->size < rhs.size;
}

bool network_info::operator < (const network_info &rhs) const{
    if (this == &rhs)
        return false;
    else if (this->ip_addresses.size() > 0 && this->ip_addresses.size() == rhs.ip_addresses.size())
        return 0 < strcmp(this->ip_addresses[0].c_str(), rhs.ip_addresses[0].c_str());
    else
        return this->ip_addresses.size() < rhs.ip_addresses.size();
}

}} // namespace