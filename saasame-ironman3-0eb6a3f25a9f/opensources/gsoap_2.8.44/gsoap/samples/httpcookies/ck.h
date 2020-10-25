/*
	ck.h

	This is a gSOAP header file with web service definitions.

	This demo illustrates the use of HTTP cookies for stateful services.
	This code base can be used to implement your own cookie-based stateful
	services.

	gSOAP's cookie handling is fully automatic at the client-side. A
	database of cookies is kept and returned to the appropriate servers. In
	this demo, the value (int) of the (invisible) cookie is returned as an
	output parameter by the service to demonstrate that each call uses a
	unique and updated cookie. Cookies are not automatically saved to a
	file by the client. So the internal cookie database is discarded when
	the program terminates.

	The server runs as CGI or stand alone with:
	./ckserver <port>
	For example:
	./ckserver 18000

	The client runs from the command line with an optional endpoint:
	./ckclient <endpoint>
	For example:
	./ckclient http://localhost:18000

	By default, the client connects to our CGI-based server.

	Compilation in C++:
	$ soapcpp2 ck.h
	$ cc -o ckclient ckclient.cpp stdsoap2.cpp soapC.cpp soapClient.cpp
	$ cc -o ckserver ckserver.cpp stdsoap2.cpp soapC.cpp soapServer.cpp

--------------------------------------------------------------------------------
gSOAP XML Web services tools
Copyright (C) 2001-2008, Robert van Engelen, Genivia, Inc. All Rights Reserved.
This software is released under one of the following two licenses:
Genivia's license for commercial use.
--------------------------------------------------------------------------------
Product and source code licensed by Genivia, Inc., contact@genivia.com
--------------------------------------------------------------------------------
*/

//gsoap ck service name:	ck
//gsoap ck service style:	rpc
//gsoap ck service encoding:	encoded
//gsoap ck service location:	http://www.cs.fsu.edu/~engelen/ck.cgi
//gsoap ck service namespace:	http://www.cs.fsu.edu/~engelen/ck.wsdl

//gsoap ck schema  namespace:	urn:ck
int ck__demo(char **r);
