/*
	vprop.h

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

typedef char    *xsd__anyURI,
                *xsd__NCName,
                *xsd__QName,
                *xsd__string;

//gsoap vprop schema documentation:	Variable Properties Schema for WS-BPEL 2.0
//gsoap vprop schema namespace:		http://docs.oasis-open.org/wsbpel/2.0/varprop
//gsoap vprop schema elementForm:	qualified
//gsoap vprop schema attributeForm:	unqualified

class vprop__tQuery
{ public:
	@xsd__anyURI			queryLanguage = "urn:oasis:names:tc:wsbpel:2.0:sublang:xpath1.0";
	xsd__QName			__mixed;
};

class vprop__tProperty
{ public:
	@xsd__NCName			name;
	@xsd__QName			type;
	@xsd__QName			element;
	xsd__string			documentation;
};

class vprop__tPropertyAlias
{ public:
	vprop__tQuery*			query;
	@xsd__QName			propertyName;
	@xsd__QName			messageType;
	@xsd__NCName			part;
	@xsd__QName			type;
	@xsd__QName			element;
	xsd__string			documentation;
};
