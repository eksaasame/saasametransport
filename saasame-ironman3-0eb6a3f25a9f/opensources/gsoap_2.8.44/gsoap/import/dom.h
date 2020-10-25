/*

        dom.h

        gSOAP DOM API v5
         
        Use #import "dom.h" in gSOAP header files to use DOM-based xsd__anyType
        for XML element and xsd__anyAttribute for XML attributes.
        
        This import is automatic with wsdl2h option -d.

        Compile with dom.c[pp] or link with libgsoapssl.a (for C) or
        libgsoapssl++.a (for C++)

        See gsoap/doc/dom/html/index.html for the new DOM API v5 documentation
        Also located in /gsoap/samples/dom/README.md

gSOAP XML Web services tools
Copyright (C) 2000-2016, Robert van Engelen, Genivia, Inc. All Rights Reserved.
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
Copyright (C) 2000-2016, Robert van Engelen, Genivia Inc., All Rights Reserved.
--------------------------------------------------------------------------------
Product and source code licensed by Genivia, Inc., contact@genivia.com
--------------------------------------------------------------------------------
*/

// The custom serializer for DOM element nodes is represented by xsd__anyType.
extern typedef struct soap_dom_element xsd__anyType;

// The custom serializer for DOM attribute nodes is represented by xsd__anyAttribute.
extern typedef struct soap_dom_attribute xsd__anyAttribute;
