/*
	calcclient.c

	Example calculator service client in C

	Compilation in C (see samples/calc/calc.h):
	> soapcpp2 -c calc.h
	> cc -o calcclient calcclient.c stdsoap2.c soapC.c soapClient.c

--------------------------------------------------------------------------------
gSOAP XML Web services tools
Copyright (C) 2001-2010, Robert van Engelen, Genivia, Inc. All Rights Reserved.
This software is released under one of the following two licenses:
Genivia's license for commercial use.
--------------------------------------------------------------------------------
Product and source code licensed by Genivia, Inc., contact@genivia.com
--------------------------------------------------------------------------------
*/

#include "soapH.h"
#include "calc.nsmap"

#include "tandem.h"

const char server[] = "http://websrv.cs.fsu.edu/~engelen/calcserver.cgi";

int main(int argc, char **argv)
{ struct soap soap;
  double a, b, result;
  if (argc < 4)
  { fprintf(stderr, "Usage: [add|sub|mul|div|pow] num num\n");
    exit(0);
  }
  soap_init(&soap);
  tandem_init(&soap, argv[0]);
  socket_set_inet_name(argv[0]); /* ? See Tandem TCP/IP programming */
  soap.send_timeout = 10; /* 10 sec */
  soap.recv_timeout = 10; /* 10 sec */
  a = strtod(argv[2], NULL);
  b = strtod(argv[3], NULL);
  switch (*argv[1])
  { case 'a':
      soap_call_ns__add(&soap, server, "", a, b, &result);
      break;
    case 's':
      soap_call_ns__sub(&soap, server, "", a, b, &result);
      break;
    case 'm':
      soap_call_ns__mul(&soap, server, "", a, b, &result);
      break;
    case 'd':
      soap_call_ns__div(&soap, server, "", a, b, &result);
      break;
    case 'p':
      soap_call_ns__pow(&soap, server, "", a, b, &result);
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
    printf("result = %g\n", result);
  soap_destroy(&soap);
  soap_end(&soap);
  tandem_done(&soap);
  soap_done(&soap);
  return 0;
}
