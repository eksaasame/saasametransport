/*
	mashupclient.c

	Example mashup service client in C

	soapcpp2 -c mashup.h
	cc -o mashupclient mashupclient.c stdsoap2.c soapC.c soapClient.c

--------------------------------------------------------------------------------
gSOAP XML Web services tools
Copyright (C) 2001-2008, Robert van Engelen, Genivia, Inc. All Rights Reserved.
This software is released under one of the following two licenses:
Genivia's license for commercial use.
--------------------------------------------------------------------------------
Product and source code licensed by Genivia, Inc., contact@genivia.com
--------------------------------------------------------------------------------
*/

#include "soapH.h"
#include "mashup.nsmap"

int main()
{
  struct soap *soap = soap_new();
  struct _ns3__commingtotown response;

  if (soap_call___ns5__dtx(soap, NULL, NULL, "", &response))
    soap_print_fault(soap, stderr);
  else if (response.days == 0)
    printf("Today is the day!\n");
  else
    printf("Wait %d more days to xmas\n", response.days);

  return 0;
}
