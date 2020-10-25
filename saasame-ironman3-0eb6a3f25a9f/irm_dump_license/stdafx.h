// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"
#include <stdio.h>
#include <tchar.h>
#include "macho.h"
#include "..\irm_transporter\license_db.h"
#include "boost\program_options.hpp"
#include "boost\program_options\parsers.hpp"
#include "boost\date_time\local_time_adjustor.hpp"
#include "boost\date_time\c_local_time_adjustor.hpp"
#include "boost\date_time.hpp"
#include <io.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <sstream>

using namespace macho;
using namespace macho::windows;
using namespace saasame::transport;
namespace po = boost::program_options;

typedef boost::date_time::c_local_adjustor<boost::posix_time::ptime> local_adj;

// TODO: reference additional headers your program requires here
