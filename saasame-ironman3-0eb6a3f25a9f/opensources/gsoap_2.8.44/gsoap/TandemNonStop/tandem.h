/*
	tandem.h

  Tandem NonStop bridge for gSOAP 2.8.0 and higher:

    tandem.h	Tandem IO
    tandem.c	Tandem blocking IO
    tandem_nw.c	Tandem nonblocking IO (no wait)

  Usage:

    Add to your client/server code the following set up:

    #include "tandem.h"
    struct soap *soap = soap_new();
    tandem_init(soap, "ProcName");
    socket_set_inet_name(argv[0]); // See Tandem docs
    ... // client or server code goes here as per gSOAP documentation
    soap_destroy(soap);
    soap_end(soap);
    tandem_done(soap);
    soap_free(soap);

  Compile flags for C/C++:

    -DTANDEM_NONSTOP

  Linkage requirements:

    Compile and link with tandem.c or tandem_nw.c

  Example:

    cc -DTANDEM_NONSTOP -o calcclient calcclient.c soapC.c soapClient.c stdsoap2.c tandem.c
      or
    cc -DTANDEM_NONSTOP -o calcclient calcclient.c soapC.c soapClient.c stdsoap2.c tandem_nw.c

    cc -DTANDEM_NONSTOP -o calcserver calcserver.c soapC.c soapServer.c stdsoap2.c tandem.c
      or
    cc -DTANDEM_NONSTOP -o calcserver calcserver.c soapC.c soapServer.c stdsoap2.c tandem_nw.c

gSOAP XML Web services tools
Copyright (C) 2000-2010, Robert van Engelen, Genivia Inc., All Rights Reserved.
This part of the software is released under ONE of the following licenses:
Genivia's license for commercial use.
--------------------------------------------------------------------------------
Product and source code licensed by Genivia, Inc., contact@genivia.com
--------------------------------------------------------------------------------
*/

void tandem_init(struct soap *soap, const char *procname);
void tandem_done(struct soap *soap);

SOAP_SOCKET soap_bind(struct soap *soap, const char *host, int port, int backlog);
SOAP_SOCKET soap_accept(struct soap *soap);
