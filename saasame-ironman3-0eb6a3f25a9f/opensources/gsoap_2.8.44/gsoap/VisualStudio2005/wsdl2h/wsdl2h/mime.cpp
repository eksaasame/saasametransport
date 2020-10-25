/*
	mime.cpp

	WSDL/MIME binding schema

--------------------------------------------------------------------------------
gSOAP XML Web services tools
Copyright (C) 2001-2008, Robert van Engelen, Genivia Inc. All Rights Reserved.
This software is released under one of the following licenses:
Genivia's license for commercial use.
--------------------------------------------------------------------------------
Product and source code licensed by Genivia, Inc., contact@genivia.com
--------------------------------------------------------------------------------

*/

#include "wsdlH.h"		// cannot include "schemaH.h"
#include "includes.h"

extern const char *qname_token(const char*, const char*);

////////////////////////////////////////////////////////////////////////////////
//
//	mime:multipartRelated
//
////////////////////////////////////////////////////////////////////////////////

int mime__multipartRelated::traverse(wsdl__definitions& definitions)
{
  if (vflag)
    cerr << "Analyzing mime multpartRelated " << endl;
  for (vector<mime__part>::iterator pt = part.begin(); pt != part.end(); ++pt)
    (*pt).traverse(definitions);
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////
//
//	mime:part
//
////////////////////////////////////////////////////////////////////////////////

int mime__part::traverse(wsdl__definitions& definitions)
{
  if (vflag)
    cerr << "Analyzing mime part " << endl;
  for (vector<soap__header>::iterator hd = soap__header_.begin(); hd != soap__header_.end(); ++hd)
    (*hd).traverse(definitions);
  return SOAP_OK;
}
