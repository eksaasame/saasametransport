

#include "..\gen-cpp\saasame_types.h"

namespace saasame{ namespace transport {

bool cascading::operator < (const cascading &rhs) const{
	if (this == &rhs)
		return false;
	else
		return 0 < strcmp(this->machine_id.c_str(), rhs.machine_id.c_str());
}

bool virtual_partition_info::operator < (const virtual_partition_info &rhs) const{
	if (this == &rhs)
		return false;
	else
		return this->offset < rhs.offset;
}

bool disk_info::operator < (const disk_info &rhs) const{
    if (this == &rhs)
        return false;
    else if (this->number == rhs.number)
        return  0 < strcmp(this->location.c_str(), rhs.location.c_str());
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

bool cluster_network::operator < (const cluster_network &rhs) const{
    if (this == &rhs)
        return false;
    else
        return 0 < strcmp(this->cluster_network_address.c_str(), rhs.cluster_network_address.c_str()) ;
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

bool cluster_group::operator < (const cluster_group &rhs) const{
    if (this == &rhs)
        return false;
    else
        return 0 < strcmp(this->group_name.c_str(), rhs.group_name.c_str());
}

bool cluster_info::operator < (const cluster_info &rhs) const{
    if (this == &rhs)
        return false;
    else
        return 0 < strcmp(this->cluster_name.c_str(), rhs.cluster_name.c_str());
}

bool service_info::operator < (const service_info &rhs) const{
    if (this == &rhs)
        return false;
    else
        return 0 < strcmp(this->id.c_str(), rhs.id.c_str());
}

bool connection::operator < (const connection &rhs) const{
    if (this == &rhs)
        return false;
    else
        return 0 < strcmp(this->id.c_str(), rhs.id.c_str());
}

bool image_map_info::operator < (const image_map_info &rhs) const{
    if (this == &rhs)
        return false;
    else
        return 0 < strcmp(this->image.c_str(), rhs.image.c_str());
}

}} // namespace