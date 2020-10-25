/*
	dime.h

	WSDL/DIME binding schema

--------------------------------------------------------------------------------
gSOAP XML Web services tools
Copyright (C) 2001-2008, Robert van Engelen, Genivia Inc. All Rights Reserved.
This software is released under one of the following licenses:
Genivia's license for commercial use.
--------------------------------------------------------------------------------
Product and source code licensed by Genivia, Inc., contact@genivia.com
--------------------------------------------------------------------------------

*/

//gsoap dime schema documentation:	WSDL/DIME binding schema
//gsoap dime schema namespace:		http://schemas.xmlsoap.org/ws/2002/04/dime/wsdl/

#import "imports.h"

class dime__message
{ public:
	@xsd__anyURI			layout;
//	@xsd__boolean			wsdl__required;
};
