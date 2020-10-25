/*
	long_time.h

	Custom serializer for xsd:time stored in an ULONG64 with usec
	(microsecond) precision

	- can represent 00:00:00 to 23:59:59.999999 (= 86399999999) UTC.
	- inbound value with timezone offset is converted to UTC.
	- ULONG64 is equivalent to unsigned long long int and uint64_t.
	- microsecond resolution (1/1000000 sec) means 1 second = 1000000.

	To obtain the current time and set the UTC time value accordingly:

	#include <sys/time.h>
        struct timeval tv;
	xsd__time t; // a ULONG64 custom serialized as xsd:time in UTC
	gettimeofday(&tv, NULL);
	t = tv.tv_usec + (tv.tv_sec % (24*60*60)) * 1000000;

	#import this file into your gSOAP .h file

	To automate the wsdl2h-mapping of xsd:time to ULONG64, add this
	line to the typemap.dat file:

	xsd__time = #import "custom/long_time.h" | xsd__time

	The typemap.dat file is used by wsdl2h to map types (wsdl2h option -t).

	When using soapcpp2 option -q<name> or -p<name>, you must change
	time.c as follows:

		#include "soapH.h"  ->  #include "nameH.h"

	Compile and link your code with custom/long_time.c

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

extern typedef unsigned long long xsd__time; /* HH:MM:SS.uuuuuu time in usec (1/1000000 sec) */
