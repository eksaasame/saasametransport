/*
	calcclient.cpp

	Example calculator service client in C++

	Service definitions in calc.h (not generated by wsdl2h, but similar)

	$ soapcpp2 -i calc.h
	$ cc -o calcclient calcclient.cpp stdsoap2.cpp soapC.cpp soapcalcProxy.cpp
	where stdsoap2.cpp is in the 'gsoap' directory, or use libgsoap++:
	$ cc -o calcclient calcclient.cpp soapC.cpp soapcalcProxy.cpp -lgsoap++

--------------------------------------------------------------------------------
gSOAP XML Web services tools
Copyright (C) 2001-2008, Robert van Engelen, Genivia, Inc. All Rights Reserved.
This software is released under one of the following two licenses:
Genivia's license for commercial use.
--------------------------------------------------------------------------------
Product and source code licensed by Genivia, Inc., contact@genivia.com
--------------------------------------------------------------------------------
*/

#include "soapcalcProxy.h"
#include "calc.nsmap"

const char server[] = "http://websrv.cs.fsu.edu/~engelen/calcserver.cgi";

int main(int argc, char **argv)
{ if (argc < 4)
  { fprintf(stderr, "Usage: [add|sub|mul|div|pow] num num\n");
    exit(0);
  }
  double a, b, result;
  a = strtod(argv[2], NULL);
  b = strtod(argv[3], NULL);
  calcProxy calc;
  calc.soap_endpoint = server;
  switch (*argv[1])
  { case 'a':
      calc.add(a, b, &result);
      break;
    case 's':
      calc.sub(a, b, &result);
      break;
    case 'm':
      calc.mul(a, b, &result);
      break;
    case 'd':
      calc.div(a, b, &result);
      break;
    case 'p':
      calc.pow(a, b, &result);
      break;
    default:
      fprintf(stderr, "Unknown command\n");
      exit(0);
  }
  if (calc.error)
    calc.soap_stream_fault(std::cerr);
  else
    printf("result = %g\n", result);
  return 0;
}
