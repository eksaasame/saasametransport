/*
	struct_tm_date.h

	Custom serializer for xsd:date Gregorian calendar date stored in a
	<time.h> struct tm. Basically the same as the struct tm xsd:dateTime
	serializer, but without the time part, which is considered 00:00:00.
	
	Timezone offsets are taken into account to adjust the time part from
	00:00:00 UTC.

	Note that tm_wday is always set to 0 (Sunday) when receiving a
	xsd:date value, because the tm content is not calculated.

	#import this file into your gSOAP .h file

	To automate the wsdl2h-mapping of xsd:date to int, add this
	line to the typemap.dat file:

	xsd__date = #import "custom/struct_tm_date.h" | xsd__date

	The typemap.dat file is used by wsdl2h to map types (wsdl2h option -t).

	When using soapcpp2 option -q<name> or -p<name>, you must change
	date.c as follows:

		#include "soapH.h"  ->  #include "nameH.h"

	Compile and link your code with custom/struct_tm_date.c

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

#include <time.h>

extern typedef struct tm xsd__date;
