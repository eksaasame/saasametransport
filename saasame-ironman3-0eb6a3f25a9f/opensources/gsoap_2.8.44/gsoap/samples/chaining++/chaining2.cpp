/*
	chaining2.cpp

	Example chaining of service classes into one service application
	Uses soapcpp2 option -j (preferred over -i)

	To generate non-client-server header and fault handlers:
	$ soapcpp2 -CS -penv env.h

	The quote service:
	$ soapcpp2 -j -S -qQuote quote.h

	The calc service:
	$ soapcpp2 -j -S -qCalc calc.h

	c++ -o chaining.cgi chaining.cpp stdsoap2.cpp envC.c QuoteServiceLib.cpp CalcServiceLib.cpp

--------------------------------------------------------------------------------
gSOAP XML Web services tools
Copyright (C) 2001-2011, Robert van Engelen, Genivia, Inc. All Rights Reserved.
This software is released under one of the following two licenses:
Genivia's license for commercial use.
--------------------------------------------------------------------------------
Product and source code licensed by Genivia, Inc., contact@genivia.com
--------------------------------------------------------------------------------
*/

#include "QuotequoteService.h"
#include "CalccalcService.h"

/* A dummy global table, to keep the linker happy */
struct Namespace namespaces[] = { {NULL} };

int main()
{
	struct soap *soap = soap_new();

	Quote::quoteService quote(soap);
	Calc::calcService calc(soap);

	/* serve over stdin/out, CGI style */
	if (soap_begin_serve(soap))
		soap_stream_fault(soap, std::cerr);
	else if (quote.dispatch() == SOAP_NO_METHOD)
	{
		if (calc.dispatch() == SOAP_NO_METHOD || soap->error)
			soap_send_fault(soap);
	}
	else if (soap->error)
		soap_send_fault(soap);

	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);
	return 0;
}

int Quote::quoteService::getQuote(char *s, float *r)
{ *r = 123; /* a dummy service, stocks don't move */
  return SOAP_OK;
}

int Calc::calcService::add(double a, double b, double *r)
{ *r = a + b;
  return SOAP_OK;
}

int Calc::calcService::sub(double a, double b, double *r)
{ *r = a - b;
  return SOAP_OK;
}

int Calc::calcService::mul(double a, double b, double *r)
{ *r = a * b;
  return SOAP_OK;
}

int Calc::calcService::div(double a, double b, double *r)
{ *r = a / b;
  return SOAP_OK;
}

int Calc::calcService::pow(double a, double b, double *r)
{ *r = ::pow(a, b);
  return SOAP_OK;
}

