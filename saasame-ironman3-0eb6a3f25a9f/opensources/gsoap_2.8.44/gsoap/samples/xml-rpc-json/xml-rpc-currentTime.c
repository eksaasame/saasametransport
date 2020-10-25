/*
	xml-rpc-currentTime.c

	XML-RPC currenTime (C version)
	Updated for gSOAP 2.8.26 with new XML-RPC C API xml-rpc.c

	Prints current time.

	Compile:
	soapcpp2 -c -CSL xml-rpc.h
	cc xml-rpc-currentTime.c xml-rpc.c stdsoap2.c soapC.c

--------------------------------------------------------------------------------
gSOAP XML Web services tools
Copyright (C) 2000-2015, Robert van Engelen, Genivia, Inc. All Rights Reserved.
This software is released under one of the following two licenses:
Genivia's license for commercial use.
--------------------------------------------------------------------------------
Product and source code licensed by Genivia, Inc., contact@genivia.com
--------------------------------------------------------------------------------
*/

#include "soapH.h"

int main()
{
  struct soap *soap = soap_new1(SOAP_C_UTFSTRING);
  struct methodResponse r;
  /* connect, send request w/o parameters (NULL) and receive response */
  if (call_method(soap, "http://www.cs.fsu.edu/~engelen/currentTime.cgi", "currentTime.getCurrentTime", NULL, &r))
  {
    soap_print_fault(soap, stderr);
    exit(soap->error);
  }
  if (r.fault)
  {
    soap_write_fault(soap, r.fault); /* print fault on stdout */
  }
  else if (r.params && r.params->__size == 1)
  { /* print response parameter */
    if (nth_param(r.params, 0)->__type == SOAP_TYPE__dateTime_DOTiso8601)
      printf("Time = %s\n", *dateTime_of(nth_param(r.params, 0)));
    else
      printf("Time not provided\n");
  }
  soap_end(soap);
  soap_free(soap);
  return 0;
}

/* Don't need a namespace table. We put an empty one here to avoid link errors */
struct Namespace namespaces[] = { {NULL, NULL} };
