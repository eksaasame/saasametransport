/*

httpdatest.h

gSOAP HTTP Digest Authentication example application.

gSOAP XML Web services tools
Copyright (C) 2000-2005, Robert van Engelen, Genivia, Inc., All Rights Reserved.

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
Copyright (C) 2000-2005, Robert van Engelen, Genivia, Inc., All Rights Reserved.
--------------------------------------------------------------------------------

Compile:

soapcpp2 -c httpdatest.h
cc -DWITH_OPENSSL -o httpdatest httpdatest.c soapC.c soapClient.c soapServer.c httpda.c smdevp.c threads.c stdsoap2.c -lssl -lcrypto -lz

*/

int ns__echoString(char *arg, struct ns__echoString*);