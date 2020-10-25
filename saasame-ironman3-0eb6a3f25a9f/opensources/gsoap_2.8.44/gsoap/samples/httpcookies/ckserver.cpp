/*
	ckserver.cpp

	Example HTTP cookie-enabled server

	Run from command line to start a stand-alone server:
	$ ./ckserver 8080
	where 8080 is a port number

	Then run the client in a new window:
	$ ./ckclient

	Please see the ckclient.cpp file for HTTP cookie-related details.
	Remember to change the soap.cookie_domain value to your host

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
#include "ck.nsmap"

int main(int argc, char **argv)
{ int m, s;
  struct soap soap;
  soap_init(&soap);
  // cookie domain for CGI must be the current host name:
  // soap.cookie_domain = "www.cs.fsu.edu";
  // Cookie domain for stand-alone server:
  soap.cookie_domain = "localhost:8080";
  // the path which is used to filter/set cookies with this destination
  soap.cookie_path = "/";
  if (argc < 2)
  { // CGI app: grab cookies from 'HTTP_COOKIE' env var
    soap_getenv_cookies(&soap);
    soap_serve(&soap);
  }
  else
  { int port;
    char buf[100];
    port = atoi(argv[1]);
    m = soap_bind(&soap, NULL, port, 100);
    if (m < 0)
    { soap_print_fault(&soap, stderr);
      exit(1);
    }
    sprintf(buf, "localhost:%d", port);
    soap.cookie_domain = buf;
    fprintf(stderr, "Socket connection successful %d\n", m);
    for (int i = 1; ; i++)
    { s = soap_accept(&soap);
      if (s < 0)
        exit(-1);
      fprintf(stderr, "%d: accepted %d IP=%d.%d.%d.%d ... ", i, s, (int)(soap.ip>>24)&0xFF, (int)(soap.ip>>16)&0xFF, (int)(soap.ip>>8)&0xFF, (int)soap.ip&0xFF);
      if (!soap_serve(&soap))
        fprintf(stderr, "served\n");
      else
        soap_print_fault(&soap, stderr);
      // clean up 
      soap_destroy(&soap);
      soap_end(&soap);
      // remove all old cookies from database so no interference when new
      // requests with new cookies arrive
      soap_free_cookies(&soap);
      // Note: threads can have their own cookie DB which they need to cleanup
      // before they terminate
    }
  }
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Demo cookie
//
////////////////////////////////////////////////////////////////////////////////

int ck__demo(struct soap *soap, char **r)
{ int n;
  char *s, buf[16];
  // The host and path are set by soap_cookie_domain and soap_cookie_path
  // which MUST be the current domain and path of the CGI app or stand-alone
  // server in order to accept cookies intended for this service
  s = soap_cookie_value(soap, "demo", NULL, NULL);
  // cookie was returned by client?
  if (s)
    n = atoi(s)+1; // yes: increment int value as demo example session
  else
    n = 1; // no: return cookie with value 1 to client to start session
  sprintf(buf, "%d", n);
  soap_set_cookie(soap, "demo", buf, NULL, NULL);
  // cookie expires in 5 seconds:
  soap_set_cookie_expire(soap, "demo", 5, NULL, NULL);
  if ((*r = (char*)soap_malloc(soap, strlen(buf)+1)))
    strcpy(*r, buf);
  return SOAP_OK;
}

