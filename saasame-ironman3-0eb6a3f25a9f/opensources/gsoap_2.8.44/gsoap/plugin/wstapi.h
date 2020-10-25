/*
        wstapi.h

        WS-Trust plugin.

        See wstapi.c for documentation and details.

gSOAP XML Web services tools
Copyright (C) 2000-2016, Robert van Engelen, Genivia Inc., All Rights Reserved.
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
Copyright (C) 2000-2016, Robert van Engelen, Genivia Inc., All Rights Reserved.
--------------------------------------------------------------------------------
Product and source code licensed by Genivia, Inc., contact@genivia.com
--------------------------------------------------------------------------------
*/

#ifndef WSTAPI_H
#define WSTAPI_H

#include "wsaapi.h"     /* also includes stdsoap2.h, soapH.h (replace with soapcpp2-generated *H.h file) */
#include "wsseapi.h"
#include "smdevp.h"     /* digest algos */
#include "threads.h"    /* mutex for sequence database */

extern const char *soap_wst_rst_action;
extern const char *soap_wst_rstr_action;
extern const char *soap_wst_rstc_action;
extern const char *soap_wst_rstrc_action;

#ifdef __cplusplus
extern "C" {
#endif

SOAP_FMAC1 int SOAP_FMAC2 soap_wst_request_saml_token(struct soap *soap, const char *endpoint, int soapver, const char *applyto, const char *username, const char *password, saml1__AssertionType **saml1, saml2__AssertionType **saml2);
SOAP_FMAC1 int SOAP_FMAC2 soap_wst_request_psha1_token(struct soap *soap, const char *endpoint, int soapver, const char *applyto, const char *username, const char *password, char *psha1, size_t psha1len);

#ifdef __cplusplus
}
#endif

SOAP_FMAC5 int SOAP_FMAC6 soap_call___wst__RequestSecurityToken(struct soap *soap, const char *soap_endpoint, const char *soap_action, struct wst__RequestSecurityTokenType *wst__RequestSecurityToken, struct wst__RequestSecurityTokenResponseType *wst__RequestSecurityTokenResponse);

SOAP_FMAC5 int SOAP_FMAC6 soap_call___wst__RequestSecurityTokenCollection(struct soap *soap, const char *soap_endpoint, const char *soap_action, struct wst__RequestSecurityTokenCollectionType *wst__RequestSecurityTokenCollection, struct wst__RequestSecurityTokenResponseCollectionType *wst__RequestSecurityTokenResponseCollection);

#endif
