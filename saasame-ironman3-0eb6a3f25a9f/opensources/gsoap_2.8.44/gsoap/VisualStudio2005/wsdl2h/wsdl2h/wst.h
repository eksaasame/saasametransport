/*
	wst.h

	WS-Trust

--------------------------------------------------------------------------------
gSOAP XML Web services tools
Copyright (C) 2001-2008, Robert van Engelen, Genivia Inc. All Rights Reserved.
This software is released under one of the following licenses:
Genivia's license for commercial use.
--------------------------------------------------------------------------------
Product and source code licensed by Genivia, Inc., contact@genivia.com
--------------------------------------------------------------------------------
*/

//gsoap wst schema documentation:	WS-Trust
//gsoap wst schema namespace:		http://docs.oasis-open.org/ws-sx/ws-trust/200512
//gsoap wst schema elementForm:		qualified
//gsoap wst schema attributeForm:	unqualified

class wst__Claims
{ public:
	@xsd__string			Dialect;
	xsd__string			__item;
};
