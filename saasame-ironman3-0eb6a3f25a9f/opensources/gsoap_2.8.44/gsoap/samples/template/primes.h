/*
	primes.h

	Declarations for the soapcpp2 compiler to define a primes class derived
	from the 'soap' context and to define a simple_vector template.

--------------------------------------------------------------------------------
gSOAP XML Web services tools
Copyright (C) 2001-2010, Robert van Engelen, Genivia, Inc. All Rights Reserved.
This software is released under one of the following two licenses:
Genivia's license for commercial use.
--------------------------------------------------------------------------------
Product and source code licensed by Genivia, Inc., contact@genivia.com
--------------------------------------------------------------------------------
*/

// declare the simple_vector<> template:
template <class T> class simple_vector;

// declare a primes class that inherits a context for SOAP/XML serialization:
class primes: struct soap
{ public:
    simple_vector<int> prime; // container of ints
    void sieve(int n);
    void write();
    virtual ~primes();
};

// #include is deferred to the generated code, which will then include the defs:
#include "simple_vector.h" // defines simple_vector<>
