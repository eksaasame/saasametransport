/*
	ssl.h

	Example SSL-secure client and server.

	SSL-enabled services use the gSOAP SSL interface. See sslclient.c and
	sslserver.c for example code with instructions and the gSOAP
	documentation more details.

--------------------------------------------------------------------------------
gSOAP XML Web services tools
Copyright (C) 2001-2008, Robert van Engelen, Genivia, Inc. All Rights Reserved.
This software is released under one of the following two licenses:
Genivia's license for commercial use.
--------------------------------------------------------------------------------
Product and source code licensed by Genivia, Inc., contact@genivia.com
--------------------------------------------------------------------------------
*/

//gsoap ns service name:	ssl
//gsoap ns service style:	rpc
//gsoap ns service encoding:	encoded
//gsoap ns service namespace:	urn:ssl
//gsoap ns service location:	https://localhost:18081

typedef double xsd__double;

int ns__add(xsd__double a, xsd__double b, xsd__double *result);
