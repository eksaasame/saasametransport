/*
	imports.h

	Common XSD types and externs for gSOAP header file import

--------------------------------------------------------------------------------
gSOAP XML Web services tools
Copyright (C) 2001-2008, Robert van Engelen, Genivia Inc. All Rights Reserved.
This software is released under one of the following licenses:
Genivia's license for commercial use.
--------------------------------------------------------------------------------
Product and source code licensed by Genivia, Inc., contact@genivia.com
--------------------------------------------------------------------------------

*/

template <class T> class std::vector;

typedef char	*xsd__anyURI,
		*xsd__ID,
		*xsd__NCName,
		*xsd__NMTOKEN,
		*xsd__NMTOKENS,
		*xsd__QName,
		*xsd__token,
		*xsd__string;
typedef bool	xsd__boolean;

extern class ostream;
extern class istream;

#include "includes.h"

extern class SetOfString;
