/*
	wsam.h

	WS-Addressing and WS-Addressing Metadata

--------------------------------------------------------------------------------
gSOAP XML Web services tools
Copyright (C) 2001-2010, Robert van Engelen, Genivia Inc. All Rights Reserved.
This software is released under one of the following licenses:
Genivia's license for commercial use.
--------------------------------------------------------------------------------
Product and source code licensed by Genivia, Inc., contact@genivia.com
--------------------------------------------------------------------------------
*/

//gsoap wsa schema documentation:	WS-Addressing
//gsoap wsa schema namespace:		http://www.w3.org/2005/08/addressing
//gsoap wsa schema elementForm:		qualified
//gsoap wsa schema attributeForm:	unqualified

//gsoap wsam schema documentation:	WS-Addressing Metadata
//gsoap wsam schema namespace:		http://www.w3.org/2007/05/addressing/metadata
//gsoap wsam schema elementForm:	qualified
//gsoap wsam schema attributeForm:	unqualified

//gsoap wsaw schema documentation:	WS-Addressing WSDL
//gsoap wsaw schema namespace:		http://www.w3.org/2006/05/addressing/wsdl
//gsoap wsaw schema elementForm:	qualified
//gsoap wsaw schema attributeForm:	unqualified

class wsa__EndpointReferenceType
{ public:
	xsd__anyURI			Address;
	_XML				__any;
};

