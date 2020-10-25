/*
	ckclient.cpp

	Example HTTP cookie-enabled client

	See cookie processing details in the code below.

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

////////////////////////////////////////////////////////////////////////////////
//
//	Example cookie client
//
////////////////////////////////////////////////////////////////////////////////

static const char *ckserver = "http://localhost:8080";

// To access a stand-alone server on a port, use:
// const char ckserver[] = "IP:PORT";
// const char ckserver[] = "http://IP:PORT";	// include HTTP header in request
// const char ckserver[] = "http://www.domain.com:8008";
// const char ckserver[] = ""; // use I/O redirect
// const char ckserver[] = "http://"; // use I/O redirect (includes HTTP headers)

int main(int argc, char **argv)
{ char *r;
  struct soap soap;
  if (argc >= 2)
    ckserver = argv[1];
  soap_init(&soap);

  // gSOAP's cookie handling is fully automatic at the client-side.
  // A database of cookies is kept and returned to the appropriate servers.
  // In this demo, the value (int) of the (invisible) cookie is returned as
  // an output parameter by the service to demonstrate that each call uses
  // a unique and updated cookie. Cookies are not automatically saved to a
  // file by the client. So the internal cookie database is discarded when
  // the program terminates.

  // To avoid "cookie storms" caused by malicious servers that return an 
  // unreasonable amount of cookies, gSOAP clients/servers are restricted to
  // a database size that the user can limit (32 cookies by default):
  
  soap.cookie_max = 10;

  // In case all three calls below return the same cookie value 1, the service
  // (ck.cgi) is unable to return cookies (e.g. because the Web server does
  // not allow CGI applications to handle cookies which the user need to fix
  // by reconfiguration and restart of the Web server). Or the
  // soap.cookie_domain value is not set in the ckserver code to the host on
  // which the service runs .

  // First call (no cookies returned to service, service will return a cookie):
  if (soap_call_ck__demo(&soap, ckserver, NULL, &r))
  { soap_print_fault(&soap, stderr);
    soap_print_fault_location(&soap, stderr);
    exit(-1);
  }
  printf("The server responded with: %s\n", r);

  // Second call (return cookie to service indicating continuation of session):
  if (soap_call_ck__demo(&soap, ckserver, NULL, &r))
  { soap_print_fault(&soap, stderr);
    soap_print_fault_location(&soap, stderr);
    exit(-1);
  }
  printf("The server responded with: %s\n", r);

  // Third call (return cookie to service indicating continuation of session):
  if (soap_call_ck__demo(&soap, ckserver, NULL, &r))
  { soap_print_fault(&soap, stderr);
    soap_print_fault_location(&soap, stderr);
    exit(-1);
  }
  printf("The server responded with: %s\n", r);

  // Fourth call (let cookie expire)
  printf("Waiting 6 seconds to let cookie expire...\n");
  sleep(6); // wait to let cookie expire
  if (soap_call_ck__demo(&soap, ckserver, NULL, &r))
  { soap_print_fault(&soap, stderr);
    soap_print_fault_location(&soap, stderr);
    exit(-1);
  }
  printf("The server responded with: %s\n", r);

  soap_end(&soap);
  return 0;
}
