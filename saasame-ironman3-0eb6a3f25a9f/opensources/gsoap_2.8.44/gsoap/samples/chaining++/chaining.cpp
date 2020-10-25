/*
	chaining.cpp

	Example chaining of C++ service classes into one service application,
	assuming they are built with soapcpp2 option -i
	NOTE: option -j is preferred, see chaining2.cpp

	To generate non-client-server header and fault handlers:
	$ soapcpp2 -CS -penv env.h

	The quote service:
	$ soapcpp2 -i -S -qQuote quote.h

	The calc service:
	$ soapcpp2 -i -S -qCalc calc.h

	c++ -o chaining.cgi chaining.cpp stdsoap2.cpp envC.c QuotequoteService.cpp CalccalcService.cpp QuoteC.cpp CalcC.cpp

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
#include "envH.h" /* include envH.h as the last file, if this file is needed */

/* A dummy global table, to keep the linker happy */
struct Namespace namespaces[] = { {NULL} };

int main()
{
	Quote::quoteService quote;
	Calc::calcService calc;

	/* serve over stdin/out, CGI style */
	if (soap_begin_serve(&quote))
		quote.soap_stream_fault(std::cerr);
	else if (quote.dispatch() == SOAP_NO_METHOD)
	{
		soap_copy_stream(&calc, &quote);
		soap_free_stream(&quote); /* quote is no longer using this socket */
		if (calc.dispatch())
			soap_send_fault(&calc);
	}
	else if (quote.error)
		soap_send_fault(&quote);

	quote.destroy();
	calc.destroy();
	return 0;
}

int Quote::quoteService::getQuote(char *s, float *r)
{ *r = 123; /* a dummy service, these stocks don't move! */
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

