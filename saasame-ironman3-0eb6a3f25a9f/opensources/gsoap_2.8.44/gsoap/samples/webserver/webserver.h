/*

webserver.h

--------------------------------------------------------------------------------
gSOAP XML Web services tools
Copyright (C) 2001-2004, Robert van Engelen, Genivia, Inc. All Rights Reserved.
This software is released under one of the following two licenses:
Genivia's license for commercial use.
--------------------------------------------------------------------------------
Product and source code licensed by Genivia, Inc., contact@genivia.com
--------------------------------------------------------------------------------

*/

//gsoap ns service name:	webserver
//gsoap ns service namespace:	http://websrv.cs.fsu.edu/~engelen/calc.wsdl
//gsoap ns service location:	http://localhost:8080
//gsoap ns service style:	rpc
//gsoap ns service encoding:	encoded

//gsoap ns schema namespace:	urn:calc

int ns__add(double a, double b, double *result); // HTTP POST request-response
int ns__sub(double a, double b, double *result); // HTTP POST request-response
int ns__mul(double a, double b, double *result); // HTTP POST request-response
int ns__div(double a, double b, double *result); // HTTP POST request-response

//gsoap f schema namespace: urn:form

int f__form1(void);	// one-way MEP

int f__form2(struct f__formResponse { double result; } *);

