/*
	calc.h

	This is a gSOAP header file with web service definitions.

	The service operations and type definitions use familiar C/C++ syntax.

	The following methods are defined by this gSOAP service definition:

	add(a,b)
	sub(a,b)
	mul(a,b)
	div(a,b)
	pow(a,b)

	Compilation in C (see samples/calc):
	$ soapcpp2 -c calc.h
	$ cc -o calcclient calcclient.c stdsoap2.c soapC.c soapClient.c
	$ cc -o calcserver calcserver.c stdsoap2.c soapC.c soapServer.c

	Compilation in C++ (see samples/calc++):
	$ soapcpp2 -i calc.h
	$ cc -o calcclient++ calcclient.cpp stdsoap2.cpp soapC.cpp soapcalcProxy.cpp
	$ cc -o calcserver++ calcserver.cpp stdsoap2.cpp soapC.cpp soapcalcService.cpp

	Note that soapcpp2 option -i generates proxy and service classes, which
	encapsulate the method operations in the class instead of defining them
	as global functions as in C. 

	The //gsoap directives are used to bind XML namespaces and to define
	Web service properties:

	//gsoap <ns> service name: <WSDLserviceName> <documentationText>
	//gsoap <ns> service style: [rpc|document]
	//gsoap <ns> service encoding: [literal|encoded]
	//gsoap <ns> service namespace: <WSDLnamespaceURI>
	//gsoap <ns> service location: <WSDLserviceAddressLocationURI>

	Web service operation properties:

	//gsoap <ns> service method-style: <methodName> [rpc|document]
	//gsoap <ns> service method-encoding: <methodName> [literal|encoded]
	//gsoap <ns> service method-action: <methodName> <actionString>
	//gsoap <ns> service method-documentation: <methodName> <documentation>

	and type schema properties:

	//gsoap <ns> schema namespace: <schemaNamespaceURI>
	//gsoap <ns> schema elementForm: [qualified|unqualified]
	//gsoap <ns> schema attributeForm: [qualified|unqualified]
	//gsoap <ns> schema documentation: <documentationText>
	//gsoap <ns> schema type-documentation: <typeName> <documentationText>

	where <ns> is an XML namespace prefix, which is used in C/C++ operation
	names, e.g. ns__add(), and type names, e.g. xsd__int.

	See the documentation for the full list of //gsoap directives.

--------------------------------------------------------------------------------
gSOAP XML Web services tools
Copyright (C) 2001-2008, Robert van Engelen, Genivia, Inc. All Rights Reserved.
This software is released under one of the following two licenses:
Genivia's license for commercial use.
--------------------------------------------------------------------------------
Product and source code licensed by Genivia, Inc., contact@genivia.com
--------------------------------------------------------------------------------
*/

//gsoap ns service name:	calc Simple calculator service
//gsoap ns service style:	rpc
//gsoap ns service encoding:	encoded
//gsoap ns service namespace:	http://websrv.cs.fsu.edu/~engelen/calc.wsdl
//gsoap ns service location:	http://websrv.cs.fsu.edu/~engelen/calcserver.cgi

//gsoap ns schema namespace:	urn:calc

//gsoap ns service method-documentation: add Sums two values
int ns__add(double a, double b, double *result);

//gsoap ns service method-documentation: sub Subtracts two values
int ns__sub(double a, double b, double *result);

//gsoap ns service method-documentation: mul Multiplies two values
int ns__mul(double a, double b, double *result);

//gsoap ns service method-documentation: div Divides two values
int ns__div(double a, double b, double *result);

//gsoap ns service method-documentation: pow Raises a to b
int ns__pow(double a, double b, double *result);
