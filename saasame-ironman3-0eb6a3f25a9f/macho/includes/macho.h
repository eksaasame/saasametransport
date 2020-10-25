// macho.h ----------------------------------------------//
// -----------------------------------------------------------------------------

// Copyright 2013-2014 Albert Wei

// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt

// -----------------------------------------------------------------------------

// Revision History
// 04-29-2014 - Initial Release

// -----------------------------------------------------------------------------

#pragma once

#ifndef __MACHO_HEADER_FILES__
#define __MACHO_HEADER_FILES__

#include "winsock2.h"
#include <boost/preprocessor/facilities/is_empty.hpp>
#include "common\stringutils.hpp"
#include "common\exception_base.hpp"
#include "common\tracelog.hpp"
#include "common\base32.hpp"
#include "common\crc32.hpp"
#include "common\crc8.hpp"
#include "common\xor_crypto.hpp"
#include "common\sha1.hpp"
#include "common\md5.hpp"
#include "common\guid.hpp"
#include "common\jobs_scheduler.hpp"

#include "windows\environment.hpp"
#include "windows\lock.hpp"
#include "windows\mutex.hpp"
#include "windows\registry.hpp"
#include "windows\reg_cluster_edit.hpp"
#include "windows\auto_handle_base.hpp"
#include "windows\protected_data.hpp"
#include "windows\application_settings.hpp"
#include "windows\msi_component.hpp"
#include "windows\com_init.hpp"
#include "windows\wmi.hpp"
#include "windows\file_version_info.hpp"
#include "windows\executable_file_info.hpp"
#include "windows\service.hpp"
#include "windows\cabinet.hpp"
#include "windows\storage.hpp"
#include "windows\network_adapter.hpp"
#include "windows\cluster.hpp"
#include "windows\task_scheduler.hpp"
#include "windows\wnet_connection.hpp"
#include "windows\file_lock.hpp"
#include "windows\certificates.hpp"
#include "windows\process.hpp"

#include "common\archive.hpp"

#if ( ( _MSC_VER >= 1700 ) || ( defined(WINDDK) && !BOOST_PP_IS_EMPTY(WINDDK) ) )
#include "windows\setup_inf_file.hpp"
#include "windows\hardware_device.hpp"
#endif

#endif