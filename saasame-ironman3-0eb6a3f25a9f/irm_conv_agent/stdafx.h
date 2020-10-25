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
#include <boost/program_options.hpp>
#include <boost/application.hpp>
#include <setup/windows/setup/service_setup.hpp>
#include <macho.h>
#include <VersionHelpers.h>

namespace po = boost::program_options;
using namespace macho;