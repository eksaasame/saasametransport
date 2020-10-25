/*

httpmd5test.c

gSOAP HTTP Content-MD5 digest plugin example application.

gSOAP XML Web services tools
Copyright (C) 2000-2004, Robert van Engelen, Genivia, Inc., All Rights Reserved.

--------------------------------------------------------------------------------
gSOAP public license.

The contents of this file are subject to the gSOAP Public License Version 1.3
(the "License"); you may not use this file except in compliance with the
License. You may obtain a copy of the License at
http://www.cs.fsu.edu/~engelen/soaplicense.html
Software distributed under the License is distributed on an "AS IS" basis,
WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
for the specific language governing rights and limitations under the License.

The Initial Developer of the Original Code is Robert A. van Engelen.
Copyright (C) 2000-2004, Robert van Engelen, Genivia, Inc., All Rights Reserved.
--------------------------------------------------------------------------------

Requires OpenSSL

Compile:

soapcpp2 -c httpmd5test.h
cc -DWITH_OPENSSL -o httpmd5test httpmd5test.c soapC.c soapClient.c soapServer.c httpmd5.c md5evp.c stdsoap2.c -lssl -lcrypto -lz

*/

#include "httpmd5.h"
#include "soapH.h"
#include "ns.nsmap"

int main(int argc, char **argv)
{ struct soap soap;
  struct ns__echoString r;
  soap_init(&soap);
  soap_register_plugin(&soap, http_md5);
  if (argc < 2)
    soap_serve(&soap);
  else if (soap_call_ns__echoString(&soap, "http://", NULL, argv[1], &r))
    soap_print_fault(&soap, stderr);
  soap_end(&soap);
  soap_done(&soap);
  return 0;
}

int ns__echoString(struct soap *soap, char *arg, struct ns__echoString *response)
{ response->arg = arg;
  return SOAP_OK;
}
