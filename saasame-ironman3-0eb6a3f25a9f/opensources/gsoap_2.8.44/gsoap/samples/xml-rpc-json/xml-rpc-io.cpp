/*
	xml-rpc-io.cpp

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
# include "jsonH.h"
#else
# include "soapH.h"
#endif

#ifdef JSON_NAMESPACE
using namespace json;
#endif

std::ostream& operator<<(std::ostream& o, const struct value& v)
{
  if (v.soap)
  {
    std::ostream *os = v.soap->os;
    v.soap->os = &o;
    soap_write_value(v.soap, &v);
    v.soap->os = os;
  }
  else
  {
    soap *ctx = soap_new();
    ctx->os = &o;
    soap_write_value(ctx, &v);
    soap_destroy(ctx);
    soap_end(ctx);
    soap_free(ctx);
  }
  return o;
}

std::istream& operator>>(std::istream& i, struct value& v)
{
  if (!v.soap)
    v.soap = soap_new();
  std::istream *is = v.soap->is;
  v.soap->is = &i;
  if (soap_read_value(v.soap, &v))
  {
    soap_sprint_fault(v.soap, v.soap->msgbuf, sizeof(v.soap->msgbuf));
    v["$error"] = v.soap->msgbuf;
    v["$code"] = v.soap->error;
  }
  v.soap->is = is;
  return i;
}
