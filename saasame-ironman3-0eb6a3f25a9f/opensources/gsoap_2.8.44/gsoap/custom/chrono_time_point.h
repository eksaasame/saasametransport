/*
	chrono_time_point.h

	Custom serializer for std::chrono::system_clock::time_point as
	xsd:dateTime

	Requires C++11 or higher (compile with -std=c++11).

	#import this file into your gSOAP .h file

	To automate the wsdl2h-mapping of xsd:dateTime to
	std::chrono::system_clock::time_point, add this line to the typemap.dat
	file:

	xsd__dateTime = #import "custom/chrono_time_point.h" | xsd__dateTime

	The typemap.dat file is used by wsdl2h to map types (wsdl2h option -t).

	When using soapcpp2 option -q<name> or -p<name>, you must change
	chrono_time_point.cpp as follows:

		#include "soapH.h"  ->  #include "nameH.h"

	Compile and link your code with custom/chrono_time_point.cpp

gSOAP XML Web services tools
Copyright (C) 2000-2015, Robert van Engelen, Genivia Inc., All Rights Reserved.
This part of the software is released under ONE of the following licenses:
 Genivia's license for commercial use.
--------------------------------------------------------------------------------
gSOAP public license.

The contents of this file are subject to the gSOAP Public License Version 1.3
(the "License"); you may not use this file except in compliance with the
License. You may obtain a copy of the License at
http://www.cs.fsu.edu/~engelen/soaplicense.html
Software distributed under the License is distributed on an "AS IS" basis,
WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
for the specific language governing rights and limitations under the License.

The Initial Developer of the Original Code is Robert A. van Engelen.
Copyright (C) 2000-2015, Robert van Engelen, Genivia, Inc., All Rights Reserved.
--------------------------------------------------------------------------------
Product and source code licensed by Genivia, Inc., contact@genivia.com
--------------------------------------------------------------------------------
*/

#include <chrono>
extern typedef class std::chrono::system_clock::time_point xsd__dateTime;
