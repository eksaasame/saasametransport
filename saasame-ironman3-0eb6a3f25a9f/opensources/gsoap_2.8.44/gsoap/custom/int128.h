/*
	int128.h

	Custom serializer for __int128_t as xsd:integer (big int).

	#import this file into your gSOAP .h file.

	Add this line to typemap.dat to automate the mapping with wsdl2h:

	xsd__integer = #import "custom/int128.h" | xsd__integer

	When using soapcpp2 option -q<name> or -p<name>, you must change
	int128.c as follows:

		#include "soapH.h"  ->  #include "nameH.h"

	Compile and link your code with custom/int128.c

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

extern class __int128_t;
extern typedef __int128_t xsd__integer;
