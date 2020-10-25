/*
	json-currentTime.c

	JSON currenTime (C version)

	Prints current time.

	Compile:
	soapcpp2 -c -CSL xml-rpc.h
	cc json-currentTime.c xml-rpc.c json.c stdsoap2.c soapC.c

--------------------------------------------------------------------------------
gSOAP XML Web services tools
Copyright (C) 2001-2012, Robert van Engelen, Genivia, Inc. All Rights Reserved.
This software is released under one of the following two licenses:
Genivia's license for commercial use.
--------------------------------------------------------------------------------
Product and source code licensed by Genivia, Inc., contact@genivia.com
--------------------------------------------------------------------------------
*/

#include "json.h"

int main()
{
  struct soap *ctx = soap_new1(SOAP_C_UTFSTRING | SOAP_XML_INDENT);
  ctx->send_timeout = 10; // 10 sec, stop if server is not accepting msg
  ctx->recv_timeout = 10; // 10 sec, stop if server does not respond in time

  struct value *request = new_value(ctx);
  struct value response;

  // make the JSON REST POST request and get response
  *string_of(request) = "getCurrentTime";
  if (json_call(ctx, "http://www.cs.fsu.edu/~engelen/currentTimeJSON.cgi", request, &response))
    soap_print_fault(ctx, stderr);
  else if (is_string(&response)) // JSON does not support a dateTime value: this is a string
    printf("Time = %s\n", *string_of(&response));
  else // error?
  {
    printf("Error: ");
    json_write(ctx, &response);
  }

  // clean up
  soap_destroy(ctx);
  soap_end(ctx);
  soap_free(ctx);
  return 0;
}

/* Don't need a namespace table. We put an empty one here to avoid link errors */
struct Namespace namespaces[] = { {NULL, NULL} };
