/*
--------------------------------------------------------------------------------
gSOAP XML Web services tools
Copyright (C) 2001-2008, Robert van Engelen, Genivia, Inc. All Rights Reserved.
This software is released under one of the following two licenses:
Genivia's license for commercial use.
--------------------------------------------------------------------------------
Product and source code licensed by Genivia, Inc., contact@genivia.com
--------------------------------------------------------------------------------
*/

//gsoap ns1 service name:	magic Magic squares
//gsoap ns1 service style:	rpc
//gsoap ns1 service encoding:	encoded
//gsoap ns1 service namespace:	http://websrv.cs.fsu.edu/~engelen/magic.wsdl
//gsoap ns1 service location:	http://websrv.cs.fsu.edu/~engelen/magicserver.cgi

//gsoap ns1 schema namespace: urn:MagicSquare

typedef int xsd__int;

// SOAP encoded array of ints
class vector
{ public:
  xsd__int *__ptr;
  int __size;
  struct soap *soap;
  vector();
  vector(int);
  virtual ~vector();
  void resize(int);
  int& operator[](int) const;
};

// SOAP encoded array of arrays of ints
class matrix
{ public:
  vector *__ptr;
  int __size;
  struct soap *soap;
  matrix();
  matrix(int, int);
  virtual ~matrix();
  void resize(int, int);
  vector& operator[](int) const;
};

//gsoap ns1 service method: magic Compute magic square of given rank
//gsoap ns1 service method: magic::rank magic square matrix rank
int ns1__magic(xsd__int rank, matrix *result);
