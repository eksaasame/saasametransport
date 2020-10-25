/*
	xml-rpc-io.h
	
	XML-RPC io stream operations on XML-RPC values

--------------------------------------------------------------------------------
gSOAP XML Web services tools
Copyright (C) 2001-2008, Robert van Engelen, Genivia, Inc. All Rights Reserved.
This software is released under one of the following two licenses:
Genivia's license for commercial use.
--------------------------------------------------------------------------------
Product and source code licensed by Genivia, Inc., contact@genivia.com
--------------------------------------------------------------------------------
*/

#ifdef JSON_NAMESPACE
namespace json {
#endif

/// print a value in XML-RPC format
extern std::ostream& operator<<(std::ostream&, const struct value&);

/// parse a value from XML stream
extern std::istream& operator>>(std::istream&, struct value&);

#ifdef JSON_NAMESPACE
} // namespace json
#endif
