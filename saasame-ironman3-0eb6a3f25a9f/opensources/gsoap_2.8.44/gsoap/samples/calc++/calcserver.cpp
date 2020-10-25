/*
	calcserver.cpp

	Example calculator service in C++

	$ soapcpp2 -i calc.h
	$ cc -o calcserver++ calcserver.cpp stdsoap2.cpp soapC.cpp soapcalcService.cpp
	where stdsoap2.cpp is in the 'gsoap' directory, or use libgsoap++:
	$ cc -o calcserver++ calcserver.cpp soapC.cpp soapcalcService.cpp -lgsoap++

--------------------------------------------------------------------------------
gSOAP XML Web services tools
Copyright (C) 2001-2008, Robert van Engelen, Genivia, Inc. All Rights Reserved.
This software is released under one of the following two licenses:
Genivia's license for commercial use.
--------------------------------------------------------------------------------
Product and source code licensed by Genivia, Inc., contact@genivia.com
--------------------------------------------------------------------------------
*/

#include "soapcalcService.h"
#include "calc.nsmap"

int main(int argc, char **argv)
{ calcService calc;
  if (argc < 2)
    calc.serve();	/* serve as CGI application */
  else
  { int port = atoi(argv[1]);
    if (!port)
    { fprintf(stderr, "Usage: calcserver++ <port>\n");
      exit(0);
    }
    /* run iterative server on port until fatal error */
    if (calc.run(port))
    { calc.soap_stream_fault(std::cerr);
      exit(-1);
    }
  }
  return 0;
} 

int calcService::add(double a, double b, double *result)
{ *result = a + b;
  return SOAP_OK;
} 

int calcService::sub(double a, double b, double *result)
{ *result = a - b;
  return SOAP_OK;
} 

int calcService::mul(double a, double b, double *result)
{ *result = a * b;
  return SOAP_OK;
} 

int calcService::div(double a, double b, double *result)
{ if (b)
    *result = a / b;
  else
  { char *s = (char*)soap_malloc(this, 1024);
    (SOAP_SNPRINTF(s, 1024, 100), "<error xmlns=\"http://tempuri.org/\">Can't divide %f by %f</error>", a, b);
    return soap_senderfault("Division by zero", s);
  }
  return SOAP_OK;
} 

int calcService::pow(double a, double b, double *result)
{ *result = ::pow(a, b);
  if (soap_errno == EDOM)	/* soap_errno is like errno, but compatible with Win32 */
  { char *s = (char*)soap_malloc(this, 1024);
    (SOAP_SNPRINTF(s, 1024, 100), "<error xmlns=\"http://tempuri.org/\">Can't take power of %f to %f</error>", a, b);
    return soap_senderfault("Power function domain error", s);
  }
  return SOAP_OK;
}
