/*
	wsseapi-lite.h

	WS-Security, lite version (time stamp and user name token only).

	See wsseapi-lite.c for documentation and details.

gSOAP XML Web services tools
Copyright (C) 2000-2015, Robert van Engelen, Genivia Inc., All Rights Reserved.
This part of the software is released under one of the following licenses:
 Genivia's license for commercial use.
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
Copyright (C) 2000-2015, Robert van Engelen, Genivia Inc., All Rights Reserved.
--------------------------------------------------------------------------------
Product and source code licensed by Genivia, Inc., contact@genivia.com
--------------------------------------------------------------------------------
*/

#ifndef WSSEAPI_H
#define WSSEAPI_H

#include "soapH.h"	/* replace with soapcpp2-generated *H.h file */

#ifdef __cplusplus
extern "C" {
#endif

extern const char *wsse_PasswordTextURI;

SOAP_FMAC1 struct _wsse__Security * SOAP_FMAC2 soap_wsse_add_Security(struct soap *soap);
SOAP_FMAC1 struct _wsse__Security * SOAP_FMAC2 soap_wsse_add_Security_actor(struct soap *soap, const char *actor);
SOAP_FMAC1 void SOAP_FMAC2 soap_wsse_delete_Security(struct soap *soap);
SOAP_FMAC1 struct _wsse__Security* SOAP_FMAC2 soap_wsse_Security(struct soap *soap);

SOAP_FMAC1 struct ds__SignatureType * SOAP_FMAC2 soap_wsse_add_Signature(struct soap *soap);
SOAP_FMAC1 void SOAP_FMAC2 soap_wsse_delete_Signature(struct soap *soap);
SOAP_FMAC1 struct ds__SignatureType * SOAP_FMAC2 soap_wsse_Signature(struct soap *soap);

SOAP_FMAC1 int SOAP_FMAC2 soap_wsse_add_Timestamp(struct soap *soap, const char *id, time_t lifetime);
SOAP_FMAC1 struct _wsu__Timestamp * SOAP_FMAC2 soap_wsse_Timestamp(struct soap *soap);
SOAP_FMAC1 int SOAP_FMAC2 soap_wsse_verify_Timestamp(struct soap *soap);

SOAP_FMAC1 int SOAP_FMAC2 soap_wsse_add_UsernameTokenText(struct soap *soap, const char *id, const char *username, const char *password);
SOAP_FMAC1 struct _wsse__UsernameToken * SOAP_FMAC2 soap_wsse_UsernameToken(struct soap *soap, const char *id);
SOAP_FMAC1 const char * SOAP_FMAC2 soap_wsse_get_Username(struct soap *soap);
SOAP_FMAC1 int SOAP_FMAC2 soap_wsse_verify_Password(struct soap *soap, const char *password);

SOAP_FMAC1 int SOAP_FMAC2 soap_wsse_sender_fault_subcode(struct soap *soap, const char *faultsubcode, const char *faultstring, const char *faultdetail);
SOAP_FMAC1 int SOAP_FMAC2 soap_wsse_receiver_fault_subcode(struct soap *soap, const char *faultsubcode, const char *faultstring, const char *faultdetail);
SOAP_FMAC1 int SOAP_FMAC2 soap_wsse_sender_fault(struct soap *soap, const char *faultstring, const char *faultdetail);
SOAP_FMAC1 int SOAP_FMAC2 soap_wsse_receiver_fault(struct soap *soap, const char *faultstring, const char *faultdetail);
SOAP_FMAC1 int SOAP_FMAC2 soap_wsse_fault(struct soap *soap, enum wsse__FaultcodeEnum fault, const char *detail);

SOAP_FMAC1 int SOAP_FMAC2 soap_wsse_set_wsu_id(struct soap *soap, const char *tags);

#ifdef __cplusplus
}
#endif

#endif
