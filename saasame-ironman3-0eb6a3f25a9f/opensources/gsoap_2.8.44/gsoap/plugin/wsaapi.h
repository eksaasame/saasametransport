/*
	wsaapi.h

	WS-Addressing plugin

gSOAP XML Web services tools
Copyright (C) 2000-2008, Robert van Engelen, Genivia Inc., All Rights Reserved.
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
Copyright (C) 2000-2008, Robert van Engelen, Genivia Inc., All Rights Reserved.
--------------------------------------------------------------------------------
Product and source code licensed by Genivia, Inc., contact@genivia.com
--------------------------------------------------------------------------------
*/

#ifndef WSAAPI_H
#define WSAAPI_H

#include "soapH.h"	/* replace with soapcpp2-generated *H.h file */

#ifdef __cplusplus
extern "C" {
#endif

/** Plugin identification for plugin registry */
#define SOAP_WSA_ID "SOAP-WSA-1.3"

/** Plugin identification for plugin registry */
extern const char soap_wsa_id[];

extern const char *soap_wsa_noneURI;
extern const char *soap_wsa_anonymousURI;
extern const char *soap_wsa_faultAction;

#if defined(SOAP_WSA_2003)
# define SOAP_WSA(member) wsa3__##member
# define SOAP_WSA_(prefix,member) prefix##_wsa3__##member
# define SOAP_WSA__(prefix,member) prefix##wsa3__##member
#elif defined(SOAP_WSA_2004)
# define SOAP_WSA(member) wsa4__##member
# define SOAP_WSA_(prefix,member) prefix##_wsa4__##member
# define SOAP_WSA__(prefix,member) prefix##wsa4__##member
#elif defined(SOAP_WSA_2005)
# define SOAP_WSA(member) wsa5__##member
# define SOAP_WSA_(prefix,member) prefix##_wsa5__##member
# define SOAP_WSA__(prefix,member) prefix##wsa5__##member
#else
# define SOAP_WSA(member) wsa__##member
# define SOAP_WSA_(prefix,member) prefix##_wsa__##member
# define SOAP_WSA__(prefix,member) prefix##wsa__##member
#endif

/**
@brief plugin data to override callbacks
*/
struct soap_wsa_data
{ /** fheader callback is invoked immediately after parsing a SOAP Header */
  int (*fheader)(struct soap*);
  /** fseterror callback is used to inspect and change gSOAP error codes */
  void (*fseterror)(struct soap*, const char**, const char**);
  /** fresponse callback is used to change a HTTP response into a HTTP POST */
  int (*fresponse)(struct soap*, int, size_t);
  /** fdisconnect callback is used to accept HTTP 202 */
  int (*fdisconnect)(struct soap*);
};

SOAP_FMAC1 int SOAP_FMAC2 soap_wsa(struct soap *soap, struct soap_plugin *p, void *arg);

SOAP_FMAC1 const char * SOAP_FMAC2 soap_wsa_rand_uuid(struct soap *soap);

SOAP_FMAC1 int SOAP_FMAC2 soap_wsa_request(struct soap *soap, const char *id, const char *to, const char *action);
SOAP_FMAC1 int SOAP_FMAC2 soap_wsa_add_From(struct soap *soap, const char *endpoint);
SOAP_FMAC1 int SOAP_FMAC2 soap_wsa_add_NoReply(struct soap *soap);
SOAP_FMAC1 int SOAP_FMAC2 soap_wsa_add_ReplyTo(struct soap *soap, const char *endpoint);
SOAP_FMAC1 int SOAP_FMAC2 soap_wsa_add_FaultTo(struct soap *soap, const char *endpoint);
SOAP_FMAC1 int SOAP_FMAC2 soap_wsa_add_RelatesTo(struct soap *soap, const char *endpoint);
SOAP_FMAC1 const char * SOAP_FMAC2 soap_wsa_From(struct soap *soap);
SOAP_FMAC1 const char * SOAP_FMAC2 soap_wsa_ReplyTo(struct soap *soap);
SOAP_FMAC1 const char * SOAP_FMAC2 soap_wsa_FaultTo(struct soap *soap);
SOAP_FMAC1 const char * SOAP_FMAC2 soap_wsa_RelatesTo(struct soap *soap);

SOAP_FMAC1 int SOAP_FMAC2 soap_wsa_check(struct soap *soap);
SOAP_FMAC1 int SOAP_FMAC2 soap_wsa_reply(struct soap *soap, const char *id, const char *action);
SOAP_FMAC1 int SOAP_FMAC2 soap_wsa_fault_subcode(struct soap *soap, int flag, const char *faultsubcode, const char *faultstring, const char *faultdetail);
SOAP_FMAC1 int SOAP_FMAC2 soap_wsa_fault_subcode_action(struct soap *soap, int flag, const char *faultsubcode, const char *faultstring, const char *faultdetail, const char *action);
SOAP_FMAC1 int SOAP_FMAC2 soap_wsa_sender_fault_subcode(struct soap *soap, const char *faultsubcode, const char *faultstring, const char *faultdetail);
SOAP_FMAC1 int SOAP_FMAC2 soap_wsa_sender_fault_subcode_action(struct soap *soap, const char *faultsubcode, const char *faultstring, const char *faultdetail, const char *action);
SOAP_FMAC1 int SOAP_FMAC2 soap_wsa_receiver_fault_subcode(struct soap *soap, const char *faultsubcode, const char *faultstring, const char *faultdetail);
SOAP_FMAC1 int SOAP_FMAC2 soap_wsa_receiver_fault_subcode_action(struct soap *soap, const char *faultsubcode, const char *faultstring, const char *faultdetail, const char *action);
SOAP_FMAC1 int SOAP_FMAC2 soap_wsa_sender_fault(struct soap *soap, const char *faultstring, const char *faultdetail);
SOAP_FMAC1 int SOAP_FMAC2 soap_wsa_receiver_fault(struct soap *soap, const char *faultstring, const char *faultdetail);

#if defined(SOAP_WSA_2005)
SOAP_FMAC1 int SOAP_FMAC2 soap_wsa_check_fault(struct soap *soap, SOAP_WSA(FaultCodesType) *fault, const char **info);
SOAP_FMAC1 int SOAP_FMAC2 soap_wsa_error(struct soap *soap, SOAP_WSA(FaultCodesType) fault, const char *info);
#elif defined(SOAP_WSA_2003)
SOAP_FMAC1 int SOAP_FMAC2 soap_wsa_check_fault(struct soap *soap, char **fault);
SOAP_FMAC1 int SOAP_FMAC2 soap_wsa_error(struct soap *soap, const char *fault);
#else
SOAP_FMAC1 int SOAP_FMAC2 soap_wsa_check_fault(struct soap *soap, SOAP_WSA(FaultSubcodeValues) *fault);
SOAP_FMAC1 int SOAP_FMAC2 soap_wsa_error(struct soap *soap, SOAP_WSA(FaultSubcodeValues) fault);
#endif

#ifdef __cplusplus
}
#endif

#endif
