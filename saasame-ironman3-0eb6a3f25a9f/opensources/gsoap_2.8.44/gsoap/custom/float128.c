/*
	float128.c

	Custom serializer for the <quadmath.h> __float128 quad precision float
	type as xsd:decimal.

	Compile this file and link it with your code.

gSOAP XML Web services tools
Copyright (C) 2000-2015, Robert van Engelen, Genivia Inc., All Rights Reserved.
This part of the software is released under ONE of the following licenses:
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
Copyright (C) 2000-2015, Robert van Engelen, Genivia, Inc., All Rights Reserved.
--------------------------------------------------------------------------------
Product and source code licensed by Genivia, Inc., contact@genivia.com
--------------------------------------------------------------------------------
*/

/* soapH.h generated by soapcpp2 from .h file containing #import "float128.h" */
#include "soapH.h"

SOAP_FMAC3 void SOAP_FMAC4 soap_default_xsd__decimal(struct soap *soap, __float128 *a)
{
  (void)soap;
  *a = 0.0;
}

SOAP_FMAC3 void SOAP_FMAC4 soap_serialize_xsd__decimal(struct soap *soap, const __float128 *a)
{
  (void)soap; (void)a;
}

SOAP_FMAC3 int SOAP_FMAC4 soap_s2xsd__decimal(struct soap *soap, const char *s, __float128 *a)
{
  if (s)
  {
    if (!*s)
      return soap->error = SOAP_TYPE;
    if (!soap_tag_cmp(s, "INF"))
      *a = (__float128)DBL_PINFTY;
    else if (!soap_tag_cmp(s, "+INF"))
      *a = (__float128)DBL_PINFTY;
    else if (!soap_tag_cmp(s, "-INF"))
      *a = (__float128)DBL_NINFTY;
    else if (!soap_tag_cmp(s, "NaN"))
      *a = (__float128)DBL_NAN;
    else
    {
      char *r;
      *a = strtoflt128(s, &r);
      if (*r)
        soap->error = SOAP_TYPE;
    }
  }
  return soap->error;
}

SOAP_FMAC3 const char * SOAP_FMAC4 soap_xsd__decimal2s(struct soap *soap, __float128 a)
{
  char *s;
  if (isnanq(a))
    return "NaN";
  if (isinfq(a))
  {
    if (a >= 0)
      return "INF";
    return "-INF";
  }
  quadmath_snprintf(soap->tmpbuf, sizeof(soap->tmpbuf), "%Qg", a);
  s = strchr(soap->tmpbuf, ',');	/* convert decimal comma to DP */
  if (s)
    *s = '.';
  return soap->tmpbuf;
}

SOAP_FMAC3 int SOAP_FMAC4 soap_out_xsd__decimal(struct soap *soap, const char *tag, int id, const __float128 *p, const char *type)
{
  if (soap_element_begin_out(soap, tag, soap_embedded_id(soap, id, p, SOAP_TYPE_xsd__decimal), type)
   || soap_string_out(soap, soap_xsd__decimal2s(soap, *p), 0))
    return soap->error;
  return soap_element_end_out(soap, tag);
}

SOAP_FMAC3 __float128 * SOAP_FMAC4 soap_in_xsd__decimal(struct soap *soap, const char *tag, __float128 *a, const char *type)
{
  if (soap_element_begin_in(soap, tag, 0, type))
    return NULL;
  a = (__float128*)soap_id_enter(soap, soap->id, a, SOAP_TYPE_xsd__decimal, sizeof(__float128), NULL, NULL, NULL, NULL);
  if (*soap->href)
    a = (__float128*)soap_id_forward(soap, soap->href, a, 0, SOAP_TYPE_xsd__decimal, 0, sizeof(__float128), 0, NULL, NULL);
  else if (a)
  {
    if (soap_s2xsd__decimal(soap, soap_value(soap), a))
      return NULL;
  }
  if (soap->body && soap_element_end_in(soap, tag))
    return NULL;
  return a;
}
