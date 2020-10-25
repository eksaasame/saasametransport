/*
	soap.h

	WSDL/SOAP binding schema

--------------------------------------------------------------------------------
gSOAP XML Web services tools
Copyright (C) 2000-2012, Robert van Engelen, Genivia Inc. All Rights Reserved.
This software is released under one of the following licenses:
Genivia's license for commercial use.
--------------------------------------------------------------------------------
Product and source code licensed by Genivia, Inc., contact@genivia.com
--------------------------------------------------------------------------------
*/

//gsoap soap schema documentation:	WSDL 1.1 SOAP binding schema
//gsoap soap schema namespace:		http://schemas.xmlsoap.org/wsdl/soap/
//gsoap soap schema elementForm:        qualified
//gsoap soap schema attributeForm:      unqualified

//gsoap wsoap schema documentation:	WSDL 2.0 SOAP binding schema
//gsoap wsoap schema namespace:		http://www.w3.org/ns/wsdl/soap
//gsoap wsoap schema elementForm:       qualified
//gsoap wsoap schema attributeForm:     unqualified

#import "imports.h"

class wsdl__definitions;		// forward declaration
class wsdl__message;			// forward declaration
class wsdl__part;			// forward declaration

enum soap__styleChoice { rpc, document };

class soap__binding
{ public:
	@xsd__anyURI			transport;
 	@enum soap__styleChoice		*style;
};

class soap__operation
{ public:
	@xsd__anyURI			soapAction;
	@xsd__boolean			soapActionRequired	= true;
	@enum soap__styleChoice		*style;
};

enum soap__useChoice { literal, encoded };

class soap__body
{ public:
	@xsd__anyURI			encodingStyle;
 	@xsd__NMTOKENS			parts;
	@enum soap__useChoice		use;
	@xsd__anyURI			namespace_;
};

class soap__fault
{ public:
	@xsd__NMTOKEN			name;
	@xsd__anyURI			encodingStyle;
	@enum soap__useChoice		use;
	@xsd__anyURI			namespace_;
};

class soap__headerfault
{ public:
	@xsd__QName			message;
 	@xsd__NMTOKEN			part;
	@enum soap__useChoice		use;
	@xsd__anyURI			encodingStyle;
	@xsd__anyURI			namespace_;
  private:
  	wsdl__message			*messageRef;
  	wsdl__part			*partRef;
  public:
  	int				traverse(wsdl__definitions&);
	void				messagePtr(wsdl__message*);
	void				partPtr(wsdl__part*);
	wsdl__message			*messagePtr() const;
	wsdl__part			*partPtr() const;
};

class soap__header
{ public:
	@xsd__QName			message;
 	@xsd__NMTOKEN			part;
	@enum soap__useChoice		use;
	@xsd__anyURI			encodingStyle;
	@xsd__anyURI			namespace_;
	std::vector<soap__headerfault>	headerfault;		// <soap:headerfault>*
  private:
  	wsdl__message			*messageRef;
  	wsdl__part			*partRef;
  public:
  	int				traverse(wsdl__definitions&);
	void				messagePtr(wsdl__message*);
	void				partPtr(wsdl__part*);
	wsdl__message			*messagePtr() const;
	wsdl__part			*partPtr() const;
};

class soap__address
{ public:
	@xsd__anyURI			location;
};

class wsoap__module
{ public:
	@xsd__anyURI			ref;
	@xsd__boolean			required = false;
};

class wsoap__header
{ public:
	@xsd__QName			element;
	@xsd__boolean			mustUnderstand_ = false;
	@xsd__boolean			required = false;
  private:
  	xs__element			*elementRef;
  public:
  	int				traverse(wsdl__definitions&);
	void				elementPtr(xs__element*);
	xs__element			*elementPtr() const;
};


