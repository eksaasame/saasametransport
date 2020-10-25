#pragma once
/*this is a file to include others file*/
#ifndef INCLUDE_H_
#define INCLUDE_H_
#include <thrift/concurrency/ThreadManager.h>
#include <thrift/concurrency/PlatformThreadFactory.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/server/TThreadPoolServer.h>
#include <thrift/server/TThreadedServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TTransportUtils.h>
#include <thrift/TToString.h>
#include <thrift/stdcxx.h>

//#include "physical_packer_job.h"


#include <vector>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <sstream>
#include <signal.h>
#include <uuid/uuid.h>

#include "../gen-cpp/physical_packer_service.h"
#include "../gen-cpp/saasame_types.h"
#include "../tools/log.h"
#include "../tools/system_tools.h"
#include "../tools/json_spirit/json_spirit.h"

#include <boost/format.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/date_time.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/shared_ptr.hpp>
//#include <boost/exception/to_string.hpp>

/*linux system include*/
#include <stdlib.h> 
using namespace json_spirit;
using namespace saasame::transport;
using namespace json_tools;
#endif