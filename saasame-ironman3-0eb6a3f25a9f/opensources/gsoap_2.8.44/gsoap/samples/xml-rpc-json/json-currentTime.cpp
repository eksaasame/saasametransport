/*
	json-currentTime.cpp

	JSON currenTime (C++ version)

	Prints current time.

	Compile:
	soapcpp2 -CSL xml-rpc.h
	c++ json-currentTime.cpp xml-rpc.cpp json.cpp stdsoap2.cpp soapC.cpp

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

using namespace std;

int main()
{
  soap *ctx = soap_new1(SOAP_C_UTFSTRING | SOAP_XML_INDENT);
  ctx->send_timeout = 10; // 10 sec, stop if server is not accepting msg
  ctx->recv_timeout = 10; // 10 sec, stop if server does not respond in time

  value request(ctx), response(ctx);

  // make the JSON REST POST request and get response
  request = "getCurrentTime";
  if (json_call(ctx, "http://www.cs.fsu.edu/~engelen/currentTimeJSON.cgi", request, response))
    soap_print_fault(ctx, stderr);
  else if (response.is_string()) // JSON does not support a dateTime value: this is a string
    cout << "Time = " << response << endl;
  else // error?
    cout << "Error: " << response << endl;

  // clean up
  soap_destroy(ctx);
  soap_end(ctx);
  soap_free(ctx);
  return 0;
}

/* Don't need a namespace table. We put an empty one here to avoid link errors */
struct Namespace namespaces[] = { {NULL, NULL} };
