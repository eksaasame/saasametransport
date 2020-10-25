/*
	http.h

	WSDL/HTTP binding schema interface

--------------------------------------------------------------------------------
gSOAP XML Web services tools
Copyright (C) 2000-2012, Robert van Engelen, Genivia Inc. All Rights Reserved.
This software is released under one of the following licenses:
Genivia's license for commercial use.
--------------------------------------------------------------------------------
Product and source code licensed by Genivia, Inc., contact@genivia.com
--------------------------------------------------------------------------------
*/

//gsoap http schema documentation:	WSDL 1.1 HTTP binding schema
//gsoap http schema namespace:		http://schemas.xmlsoap.org/wsdl/http/
//gsoap http schema elementForm:	qualified
//gsoap http schema attributeForm:	unqualified

//gsoap whttp schema documentation:	WSDL 2.0 HTTP binding schema
//gsoap whttp schema namespace:		http://www.w3.org/ns/wsdl/http
//gsoap whttp schema elementForm:	qualified
//gsoap whttp schema attributeForm:	unqualified

#import "imports.h"

class http__address
{ public:
	@xsd__anyURI		location;
};

class http__binding
{ public:
	@xsd__NMTOKEN		verb;
};

class http__operation
{ public:
	@xsd__anyURI		location;
};

class whttp__header
{ public:
	@xsd__string		name;
	@xsd__QName		type;
	@xsd__boolean		required = false;
};
