/*
	stl.h

	#import "stl.h" in a gSOAP header file to enable STL vectors, deques,
	lists, and sets.

	You can now remap std::vector in wsdl2h's output to another container
	by defining the '$CONTAINER' variable in typemap.dat, for example to
	use std::list:

	$CONTAINER = std::list

	Use soapcpp2 option -Ipath:path:... to specify the path(s) for #import

gSOAP XML Web services tools
Copyright (C) 2000-2015, Robert van Engelen, Genivia Inc., All Rights Reserved.
This part of the software is released under one of the following licenses:
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
Copyright (C) 2000-2015 Robert A. van Engelen, Genivia inc. All Rights Reserved.
--------------------------------------------------------------------------------
Product and source code licensed by Genivia, Inc., contact@genivia.com
--------------------------------------------------------------------------------
*/

#import "stldeque.h"
#import "stllist.h"
#import "stlvector.h"
#import "stlset.h"
