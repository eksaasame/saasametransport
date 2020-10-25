/*
	mime.h

	mime and xmime binding schema

--------------------------------------------------------------------------------
gSOAP XML Web services tools
Copyright (C) 2001-2008, Robert van Engelen, Genivia Inc. All Rights Reserved.
This software is released under one of the following licenses:
Genivia's license for commercial use.
--------------------------------------------------------------------------------
Product and source code licensed by Genivia, Inc., contact@genivia.com
--------------------------------------------------------------------------------

*/

//gsoap mime schema documentation:	WSDL/MIME binding schema
//gsoap mime schema namespace:		http://schemas.xmlsoap.org/wsdl/mime/
//gsoap mime schema elementForm:	qualified
//gsoap mime schema attributeForm:	unqualified

//gsoap xmime schema documentation:	xmime binding schema
//gsoap xmime schema namespace:		http://www.w3.org/2005/05/xmlmime

#import "imports.h"
#import "soap.h"

class mime__content
{ public:
 	@xsd__NMTOKEN			part;
	@xsd__string			type;
};

class mime__part
{ public:
	soap__body			*soap__body_;
	std::vector<soap__header>	soap__header_;
	std::vector<mime__content>	content;
  public:
  	int				traverse(wsdl__definitions&);
};

class mime__multipartRelated
{ public:
	std::vector<mime__part>		part;
  public:
  	int				traverse(wsdl__definitions&);
};

class mime__mimeXml
{ public:
	@xsd__NMTOKEN			part;
};
