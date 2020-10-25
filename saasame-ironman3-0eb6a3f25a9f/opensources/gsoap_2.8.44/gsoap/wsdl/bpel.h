/*
	bpel.h

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

#import "imports.h"

//gsoap plnk  schema documentation:	Partner Link Type Schema for WS-BPEL 2.0
//gsoap plnk  schema namespace:		http://docs.oasis-open.org/wsbpel/2.0/plnktype
//gsoap plnk  schema elementForm:	qualified
//gsoap plnk  schema attributeForm:	unqualified

//gsoap vprop schema documentation:	Variable Properties Schema for WS-BPEL 2.0
//gsoap vprop schema namespace:		http://docs.oasis-open.org/wsbpel/2.0/varprop
//gsoap vprop schema elementForm:	qualified
//gsoap vprop schema attributeForm:	unqualified

class wsdl__definitions;
class wsdl__portType;
class plnk__tPartnerLinkType;

class plnk__tRole
{ public:
	@xsd__NCName			name;
	@xsd__QName			portType;
	xsd__string			documentation;
  private:
	wsdl__portType			*portTypeRef;
	plnk__tPartnerLinkType		*plnkRef;
  public:
					plnk__tRole();
	int				traverse(wsdl__definitions&);
	wsdl__portType			*portTypePtr() const;
	void				plnkPtr(plnk__tPartnerLinkType*);
	plnk__tPartnerLinkType		*plnkPtr() const;
};

class plnk__tPartnerLinkType
{ public:
	@xsd__NCName			name;
	std::vector<plnk__tRole> 	role;
	xsd__string			documentation;
  public:
	int				traverse(wsdl__definitions&);
};

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
  public:
	int				traverse(wsdl__definitions&);
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
  private:
	vprop__tProperty                *vpropRef;
  public:
	int				traverse(wsdl__definitions&);
	vprop__tProperty                *vpropPtr() const;
};
