/*
	primes.cpp

	Prime sieve example that demsontrates the use of a user-defined
	simple_vector container and auto-generated code to display the
	container contents in XML.

	Build:
	> soapcpp2 -CS primes.h
	> c++ -o primes primes.cpp soapC.cpp stdsoap2.cpp

	Usage:
	> ./primes

--------------------------------------------------------------------------------
gSOAP XML Web services tools
Copyright (C) 2001-2011, Robert van Engelen, Genivia, Inc. All Rights Reserved.
This software is released under one of the following two licenses:
Genivia's license for commercial use.
--------------------------------------------------------------------------------
Product and source code licensed by Genivia, Inc., contact@genivia.com
--------------------------------------------------------------------------------
*/

// include all generated header files:
#include "soapH.h"
#include <iostream>

int main()
{
  primes p;     // also instantiates the 'soap' context
  p.sieve(100); // sieve primes

  primes q;
  q = p;	// to show that copy constructor and assignment are OK
  q.write();    // write primes in XML

  return 0;
}

// sieving primes in the simple_vector<> container:
void primes::sieve(int n)
{
  prime.clear();
  prime.insert(prime.end(), 1);
  prime.insert(prime.end(), 2);
  for (int i = 3; i <= n; i += 2)
  {
    bool composite = false;

    for (simple_vector<int>::const_iterator j = prime.begin(); j != prime.end(); ++j)
    {
      if (*j != 1 && i % *j == 0)
      { composite = true;
	break;
      }
    }
    if (!composite)
      prime.insert(prime.end(), i);
  }
}

// the writer uses the fact that the primes class inherits the context:
void primes::write()
{
  soap_set_omode(this, SOAP_XML_INDENT); // show with indentation please
  soap_write_primes(this, this); // soap_write_prime is generated
}

// the destructor cleans up the 'soap' context
primes::~primes()
{
  soap_destroy(this);
  soap_end(this);
}

// we need a dummy namespace table, even though we don't use XML namespaces:
SOAP_NMAC struct Namespace namespaces[] = { {NULL, NULL, NULL, NULL} };
