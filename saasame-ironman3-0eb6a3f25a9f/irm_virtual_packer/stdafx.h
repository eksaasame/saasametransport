// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#include <stdio.h>
#include <tchar.h>



// TODO: reference additional headers your program requires here
#include <iostream>
#include <fstream>
#include <set>
#include <boost/program_options.hpp>
#include <boost/application.hpp>
#include <setup/windows/setup/service_setup.hpp>
#include "carrier_service.h"
#include "saasame_constants.h"
#include <macho.h>
#include <thrift/concurrency/ThreadManager.h>
#include <thrift/concurrency/PosixThreadFactory.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/server/TThreadPoolServer.h>
#include <thrift/server/TThreadedServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TTransportUtils.h>
#include <thrift/transport/TSSLSocket.h>
#include <thrift/transport/TSSLServerSocket.h>
#include <thrift/TToString.h>
#include "macho.h"

using namespace macho::windows;
using namespace macho;
using namespace apache::thrift;
using namespace apache::thrift::concurrency;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;
using namespace apache::thrift::server;
using namespace saasame::transport;

using boost::shared_ptr;

#pragma comment(lib, "libevent.lib")
#pragma comment(lib, "libevent_core.lib")
#pragma comment(lib, "libevent_extras.lib")
#pragma comment(lib, "libthrift.lib")
#pragma comment(lib, "libthriftnb.lib")
#pragma comment(lib, "libeay32.lib" )
#pragma comment(lib, "ssleay32.lib" )
#pragma comment(lib, "irm_hypervisor_ex.lib")

