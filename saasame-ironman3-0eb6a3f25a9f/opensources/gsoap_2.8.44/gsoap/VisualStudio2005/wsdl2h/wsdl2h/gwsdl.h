/*
	gwsdl.h

	OGSI GWSDL binding schema interface

--------------------------------------------------------------------------------
gSOAP XML Web services tools
Copyright (C) 2001-2008, Robert van Engelen, Genivia, Inc. All Rights Reserved.
This software is released under one of the following licenses:
Genivia's license for commercial use.
--------------------------------------------------------------------------------
Product and source code licensed by Genivia, Inc., contact@genivia.com
--------------------------------------------------------------------------------

*/

//gsoap gwsdl schema documentation:	OGSI GWSDL binding schema
//gsoap gwsdl schema namespace:		http://www.gridforum.org/namespaces/2003/03/gridWSDLExtensions
//gsoap sd schema namespace:		http://www.gridforum.org/namespaces/2003/03/serviceData

#import "schema.h"

class wsdl__operation;

enum sd__mutability { static_, constant, extendable, mutable_ };

class sd__serviceData
{ public:
	@xsd__NMTOKEN			name;
	@xsd__QName			type;
	@xsd__boolean			nillable		= false;
	@xsd__string			minOccurs;		// xsd:nonNegativeInteger
	@xsd__string			maxOccurs;		// xsd:nonNegativeInteger|unbounded
	@enum sd__mutability		mutability		= extendable;
	@xsd__boolean			modifiable		= false;
	/* has any content */
  public:
};

class sd__staticServiceDataValues
{ public:
	int				__type; /* any content, probably should use DOM */
	void*				_any;
};

class gwsdl__portType
{ public:
	@xsd__NMTOKEN			name;
	@xsd__QName			extends;		// a list of QNames
	xsd__string			documentation;		// <wsdl:documentation>?
	std::vector<wsdl__operation*>	operation;		// <wsdl:operation>*
	std::vector<sd__serviceData>	sd__serviceData_;
	sd__staticServiceDataValues	*sd__staticServiceDataValues_;
  public:
};
