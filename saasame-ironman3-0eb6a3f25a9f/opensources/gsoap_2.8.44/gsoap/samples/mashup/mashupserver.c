/*
	mashupserver.c

	Example mashup CGI service in C

	soapcpp2 -c mashup.h
	cc -o mashupserver mashupserver.c stdsoap2.c soapC.c soapClient.c soapServer.c

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

int main(int argc, char **argv)
{
  return soap_serve(soap_new());
}

/******************************************************************************\
 *
 *	Server operation
 *
\******************************************************************************/

int __ns5__dtx(struct soap *soap, _XML x, struct _ns3__commingtotown *response)
{
  struct soap *csoap = soap_copy(soap);
  struct _ns1__gmt gmt;
  struct _ns1__gmtResponse gmtResponse;
  struct tm tm;
  time_t *now, xmas;
  double sec, days;

  if (soap_call___ns4__gmt(csoap, "http://www.cs.fsu.edu/~engelen/gmtlitserver.cgi", NULL, &gmt, &gmtResponse))
  {
    soap_end(csoap);
    soap_free(csoap);
    return soap_receiver_fault(soap, "Cannot connect to GMT server", NULL);
  }

  now = gmtResponse.param_1;

  if (!now)
    return soap_receiver_fault(soap, "Could not retrieve current time", NULL);

  memset(&tm, 0, sizeof(struct tm));
  tm.tm_mday = 25;
  tm.tm_mon = 11;
  tm.tm_year = gmtime(now)->tm_year; /* this year */

  xmas = soap_timegm(&tm);

  if (xmas < *now)
  {
    tm.tm_year++; /* xmas just passed, go to next year */
    xmas = soap_timegm(&tm);
  }

  sec = difftime(xmas, *now);
  
  if (soap_call_ns2__div(csoap, NULL, NULL, sec, 86400, &days))
  {
    soap_end(csoap);
    soap_free(csoap);
    return soap_receiver_fault(soap, "Cannot connect to calc server", NULL);
  }

  soap_delegate_deletion(csoap, soap); /* server should delete data */

  soap_end(csoap);
  soap_free(csoap);

  response->days = (int)days;

  return SOAP_OK;
}

/******************************************************************************\
 *
 *	Dummy operations (these are implemented in the calc and gmt servers
 *
\******************************************************************************/

int ns2__add(struct soap *soap, double a, double b, double *r) { return SOAP_NO_METHOD; }
int ns2__sub(struct soap *soap, double a, double b, double *r) { return SOAP_NO_METHOD; }
int ns2__mul(struct soap *soap, double a, double b, double *r) { return SOAP_NO_METHOD; }
int ns2__div(struct soap *soap, double a, double b, double *r) { return SOAP_NO_METHOD; }
int ns2__pow(struct soap *soap, double a, double b, double *r) { return SOAP_NO_METHOD; }

int __ns4__gmt(struct soap *soap, struct _ns1__gmt *a, struct _ns1__gmtResponse *r) { return SOAP_NO_METHOD; }