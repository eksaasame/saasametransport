/*
	float128.h

	Custom serializer for the <quadmath.h> __float128 quad precision float
	type as xsd:decimal.

	#import this file into your gSOAP .h file.

	Add this line to typemap.dat to automate the mapping with wsdl2h:

	xsd__decimal = #import "custom/float128.h" | xsd__decimal

	When using soapcpp2 option -q<name> or -p<name>, you must change
	float128.c as follows:

		#include "soapH.h"  ->  #include "nameH.h"

	Compile and link your code with custom/float128.c

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

#include <quadmath.h>
extern class __float128;
extern typedef __float128 xsd__decimal;
