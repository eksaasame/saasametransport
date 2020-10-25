/*
	duration.h

	Custom serializer for xsd:duration stored in a LONG64 with millisecond
	(ms) precision.

	LONG64 is equivalent to long long int and int64_t

	Millisecond resolution (1/1000 sec) means 1 second = 1000.

	The `xsd__duration` type is a 64 bit signed integer that can represent
	106751991167 days forward (positive) and backward (negative), with
	increments of 1 ms (1/1000 second).

	Durations that exceed a month are always output in days, rather than
	months to avoid days-per-month conversion inacurracies.

	Durations that are received in years and months instead of total number
	of days from a reference point are not well defined, since there is no
	accepted reference time point (it may or may not be the current time).
	The decoder simple assumes that there are 30 days per month. For
	example, conversion of "P4M" gives 120 days. Therefore, the durations
	"P4M" and "P120D" are assumed to be identical, which is not necessarily
	true depending on the reference point in time.

	Rescaling of the duration value by may be needed when adding the
	duration value to a `time_t` value, because `time_t` may or may not
	have a seconds resolution, depending on the platform and possible
	changes to `time_t`.

	#import this file into your gSOAP .h file

	To automate the wsdl2h-mapping of xsd:duration to LONG64, add this
	line to the typemap.dat file:

	xsd__duration = #import "custom/duration.h" | xsd__duration

	The typemap.dat file is used by wsdl2h to map types (wsdl2h option -t).

	When using soapcpp2 option -q<name> or -p<name>, you must change
	duration.c as follows:

		#include "soapH.h"  ->  #include "nameH.h"

	Compile and link your code with custom/duration.c

gSOAP XML Web services tools
Copyright (C) 2000-2009, Robert van Engelen, Genivia Inc., All Rights Reserved.
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
Copyright (C) 2000-2009, Robert van Engelen, Genivia, Inc., All Rights Reserved.
--------------------------------------------------------------------------------
Product and source code licensed by Genivia, Inc., contact@genivia.com
--------------------------------------------------------------------------------
*/

extern typedef long long xsd__duration; /* duration in ms (1/1000 sec) */
