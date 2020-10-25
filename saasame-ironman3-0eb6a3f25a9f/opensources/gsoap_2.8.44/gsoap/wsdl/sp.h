/*
	sp.h

	WS-SecurityPolicy 1.2 binding schemas

--------------------------------------------------------------------------------
gSOAP XML Web services tools
Copyright (C) 2001-2010, Robert van Engelen, Genivia Inc. All Rights Reserved.
This software is released under one of the following licenses:
Genivia's license for commercial use.
--------------------------------------------------------------------------------
Product and source code licensed by Genivia, Inc., contact@genivia.com
--------------------------------------------------------------------------------

*/

//gsoap sp schema documentation:	WS-SecurityPolicy binding
//gsoap sp schema namespace:		http://docs.oasis-open.org/ws-sx/ws-securitypolicy/200702
// 1.1 //gsoap sp schema namespace:	http://schemas.xmlsoap.org/ws/2005/07/securitypolicy
//gsoap sp schema elementForm:		qualified             
//gsoap sp schema attributeForm:	unqualified           

#import "imports.h"
#import "wsam.h"
#import "wst.h"

class sp__Header
{ public:
	@xsd__NCName			Name;
	@xsd__anyURI			Namespace;
};

class sp__Parts
{ public:
	xsd__string			Body;
	std::vector<sp__Header>		Header;
	xsd__string			Attachments;
};

class sp__Elements
{ public:
	@xsd__anyURI			XPathVersion;
	std::vector<xsd__string>	XPath;
};

class sp__Token : public wsp__Assertion
{ public:
	@xsd__anyURI			IncludeToken;
	wsa__EndpointReferenceType	*Issuer;
	xsd__anyURI			IssuerName;
	wst__Claims			*wst__Claims_;
};
