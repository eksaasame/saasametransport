/*
	plnk.h

	BPEL 2.0 binding schema

--------------------------------------------------------------------------------
gSOAP XML Web services tools
Copyright (C) 2000-2014, Robert van Engelen, Genivia Inc. All Rights Reserved.
This software is released under one of the following licenses:
Genivia's license for commercial use.
--------------------------------------------------------------------------------
Product and source code licensed by Genivia, Inc., contact@genivia.com
--------------------------------------------------------------------------------
*/

#import "stlvector.h"

typedef char    *xsd__NCName,
                *xsd__QName,
                *xsd__string;

//gsoap plnk  schema documentation:	Partner Link Type Schema for WS-BPEL 2.0
//gsoap plnk  schema namespace:		http://docs.oasis-open.org/wsbpel/2.0/plnktype
//gsoap plnk  schema elementForm:	qualified
//gsoap plnk  schema attributeForm:	unqualified

class plnk__tRole
{ public:
	@xsd__NCName			name;
	@xsd__QName			portType;
	xsd__string			documentation;
};

class plnk__tPartnerLinkType
{ public:
	@xsd__NCName			name;
	std::vector<plnk__tRole> 	role;
	xsd__string			documentation;
};
