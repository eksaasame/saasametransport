#pragma once
#ifndef sslHelper_H
#define sslHelper_H
#include <thrift/transport/TSSLServerSocket.h>
#include <thrift/transport/TSSLSocket.h>
#include <thrift/stdcxx.h>
#include "log.h"

using namespace std;
using namespace apache::thrift;
using namespace apache::thrift::transport;

class MyAccessManager : public AccessManager
{
	Decision verify(const sockaddr_storage& sa) throw()
	{
		return SKIP;
	}

	Decision verify(const std::string& host, const char* name, int size) throw()
	{
		//if (name)
		//{
		//    std::string cn(name);

		//    if (boost::iequals(cn, std::string("saasame")))
		//        return ALLOW;
		//}
		//return DENY;
		return ALLOW;
	}

	Decision verify(const sockaddr_storage& sa, const char* data, int size) throw()
	{
		return DENY;
	}
};

#endif