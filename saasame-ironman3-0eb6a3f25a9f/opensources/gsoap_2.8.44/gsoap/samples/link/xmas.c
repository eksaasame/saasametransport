/*
	xmas.c

	Example CGI service with multiple C client static linkage

	To generate non-client-server header and fault handlers:
	$ soapcpp2 -c -CS -penv env.h

	The gmt client:
	$ soapcpp2 -c -C -n -pgmt gmt.h

	The calc client:
	$ soapcpp2 -c -C -n -pcalc calc.h

	The xmas server:
	$ soapcpp2 -c -S -n -pxmas xmas.h

	cc -o xmas.cgi xmas.c stdsoap2.c envC.c xmasServerLib.c gmtClientLib.c calcClientLib.c

	The namespace table must include all proper namespace bindings.
	When multiple namespace tables are used, then the namespace tables can
	be reset using soap_set_namespaces()

--------------------------------------------------------------------------------
gSOAP XML Web services tools
Copyright (C) 2001-2008, Robert van Engelen, Genivia, Inc. All Rights Reserved.
This software is released under one of the following two licenses:
Genivia's license for commercial use.
--------------------------------------------------------------------------------
Product and source code licensed by Genivia, Inc., contact@genivia.com
--------------------------------------------------------------------------------
*/

#include "envH.h"
#include "gmtH.h"
#include "calcH.h"
#include "xmasH.h"

#include "gmt.nsmap"
#include "calc.nsmap"
#include "xmas.nsmap"

int main()
{
  struct soap soap;

  soap_init(&soap);
  soap_set_namespaces(&soap, xmas_namespaces);
  return xmas_serve(&soap);
}

/******************************************************************************\
 *
 *	Server operation
 *
\******************************************************************************/

int __ns1__dtx(struct soap *soap, _XML x, struct _ns2__commingtotown *response)
{
  struct soap *csoap = soap_copy(soap);
  struct tm tm;
  time_t now, xmas;
  double sec, days;

  soap_set_namespaces(csoap, gmt_namespaces);
  if (soap_call_t__gmt(csoap, "http://www.cs.fsu.edu/~engelen/gmtlitserver.cgi", NULL, &now))
  {
    soap_end(csoap);
    soap_free(csoap);
    return soap_receiver_fault(soap, "Cannot connect to GMT server", NULL);
  }

  tm.tm_sec = 0;
  tm.tm_min = 0;
  tm.tm_hour = 0;
  tm.tm_mday = 25;
  tm.tm_mon = 11;
  tm.tm_year = gmtime(&now)->tm_year; /* this year */
  tm.tm_isdst = 0;
  tm.tm_zone = NULL;

  xmas = soap_timegm(&tm);

  if (xmas < now)
  {
    tm.tm_year++; /* xmas just passed, go to next year */
    xmas = soap_timegm(&tm);
  }

  sec = difftime(xmas, now);
  
  soap_set_namespaces(csoap, calc_namespaces);
  if (soap_call_ns__add(csoap, NULL, NULL, sec, 86400, &days))
  {
    soap_end(csoap);
    soap_free(csoap);
    return soap_receiver_fault(soap, "Cannot connect to calc server", NULL);
  }

  soap_end(csoap);
  soap_free(csoap);

  response->days = (int)days;

  return SOAP_OK;
}

/* dummy namespaces to prevent link errors when not using WITH_NONAMSPACES */
SOAP_NMAC struct Namespace namespaces[] =
{
	{NULL, NULL, NULL, NULL}
};
