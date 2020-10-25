/*
	calcrest.c

	Example REST calculator service client in C

	Compilation in C (see samples/calc/calc.h):
	$ wsdl2h -R -c calcrest.wsdl
	$ soapcpp2 calcrest.h
	$ cc -o calcrest calcrest.c stdsoap2.c soapC.c soapClient.c
	where stdsoap2.c is in the 'gsoap' directory, or use libgsoap:
	$ cc -o calcrest calcrest.c soapC.c soapClient.c -lgsoap

--------------------------------------------------------------------------------
gSOAP XML Web services tools
Copyright (C) 2001-2013, Robert van Engelen, Genivia, Inc. All Rights Reserved.
This software is released under one of the following two licenses:
Genivia's license for commercial use.
--------------------------------------------------------------------------------
Product and source code licensed by Genivia, Inc., contact@genivia.com
--------------------------------------------------------------------------------
*/

#include "soapH.h"
#include "calcPOST.nsmap"

const char server[] = "http://websrv.cs.fsu.edu/~engelen/calcrest.cgi";
/* "http://"; */ /* to test with stdin/out */

int main(int argc, char **argv)
{ struct soap soap;
  struct ns2__pair in;
  double out;
  soap_init1(&soap, SOAP_XML_INDENT);
  if (argc < 4)
    return soap_serve(&soap); /* simple CGI server over stdin/out */
  in.a = strtod(argv[2], NULL);
  in.b = strtod(argv[3], NULL);
  switch (*argv[1])
  { case 'a':
      soap_call___ns1__add(&soap, server, "", &in, &out);
      break;
    case 's':
      soap_call___ns1__sub(&soap, server, "", &in, &out);
      break;
    case 'm':
      soap_call___ns1__mul(&soap, server, "", &in, &out);
      break;
    case 'd':
      soap_call___ns1__div(&soap, server, "", &in, &out);
      break;
    case 'p':
      soap_call___ns1__pow(&soap, server, "", &in, &out);
      break;
    default:
      fprintf(stderr, "Unknown command\n");
      exit(0);
  }
  if (soap.error)
  { soap_print_fault(&soap, stderr);
    exit(1);
  }
  else
    printf("result = %g\n", out);
  soap_destroy(&soap);
  soap_end(&soap);
  soap_done(&soap);
  return 0;
}

int __ns1__add(struct soap *soap, struct ns2__pair *in, double *out)
{ *out = in->a + in->b;
  return SOAP_OK;
}

int __ns1__sub(struct soap *soap, struct ns2__pair *in, double *out)
{ *out = in->a - in->b;
  return SOAP_OK;
}

int __ns1__mul(struct soap *soap, struct ns2__pair *in, double *out)
{ *out = in->a * in->b;
  return SOAP_OK;
}

int __ns1__div(struct soap *soap, struct ns2__pair *in, double *out)
{ *out = in->a / in->b;
  return SOAP_OK;
}

int __ns1__pow(struct soap *soap, struct ns2__pair *in, double *out)
{ *out = pow(in->a, in->b);
  return SOAP_OK;
}
