// Reminder: Modify typemap.dat to customize the header file generated by wsdl2h
/* wssedemo.h
   Generated by wsdl2h 1.2.6 from wssedemo.wsdl and typemap.dat
   2005-08-26 12:54:17 GMT
   Copyright (C) 2001-2005 Robert van Engelen, Genivia Inc. All Rights Reserved.
   This part of the software is released under one of the following licenses:
   Genivia's license for commercial use.
*/

/* NOTE:

 - Compile this file with soapcpp2 to complete the code generation process.
 - Use soapcpp2 option -I to specify paths for #import
   To build with STL, 'stlvector.h' is imported from 'import' dir in package.
 - Use wsdl2h options -c and -s to generate pure C code or C++ code without STL.
 - Use 'typemap.dat' to control schema namespace bindings and type mappings.
   It is strongly recommended to customize the names of the namespace prefixes
   generated by wsdl2h. To do so, modify the prefix bindings in the Namespaces
   section below and add the modified lines to 'typemap.dat' to rerun wsdl2h.
 - Use Doxygen (www.doxygen.org) to browse this file.
 - Use wsdl2h option -l to view the software license terms.

*/

/******************************************************************************\
 *                                                                            *
 * http://www.genivia.com/wsdl/wssetest.wsdl                                  *
 *                                                                            *
\******************************************************************************/

//gsoapopt cw

/******************************************************************************\
 *                                                                            *
 * Import                                                                     *
 *                                                                            *
\******************************************************************************/

#import "wsse.h"	// wsse = <http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-secext-1.0.xsd>

/******************************************************************************\
 *                                                                            *
 * Schema Namespaces                                                          *
 *                                                                            *
\******************************************************************************/


/******************************************************************************\
 *                                                                            *
 * Schema Types                                                               *
 *                                                                            *
\******************************************************************************/


// Imported element "http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-secext-1.0.xsd":Security declared as _wsse__Security

/******************************************************************************\
 *                                                                            *
 * Services                                                                   *
 *                                                                            *
\******************************************************************************/


//gsoap ns1  service name:	wssetest 
//gsoap ns1  service type:	wssetestPortType 
//gsoap ns1  service port:	http://localhost:80 
//gsoap ns1  service namespace:	http://www.genivia.com/schemas/wssetest.xsd 
//gsoap ns1  service transport:	http://schemas.xmlsoap.org/soap/http 

/** @mainpage wssetest Definitions

@section wssetest_bindings Bindings
  - @ref wssetest

*/

/**

@page wssetest Binding "wssetest"

@section wssetest_operations Operations of Binding  "wssetest"
  - @ref ns1__add
  - @ref ns1__sub
  - @ref ns1__mul
  - @ref ns1__div

@section wssetest_ports Endpoints of Binding  "wssetest"
  - http://localhost:80

*/

/******************************************************************************\
 *                                                                            *
 * SOAP Header                                                                *
 *                                                                            *
\******************************************************************************/

/**

The SOAP Header is part of the gSOAP context and its content is accessed
through the soap.header variable. You may have to set the soap.actor variable
to serialize SOAP Headers with SOAP-ENV:actor or SOAP-ENV:role attributes.

*/
struct SOAP_ENV__Header
{
    mustUnderstand                       // must be understood by receiver
    _wsse__Security                     *wsse__Security                ;	///< TODO: Check element type (imported type)

};

/******************************************************************************\
 *                                                                            *
 * wssetest                                                                   *
 *                                                                            *
\******************************************************************************/


/******************************************************************************\
 *                                                                            *
 * ns1__add                                                                   *
 *                                                                            *
\******************************************************************************/


/// Operation "ns1__add" of service binding "wssetest"

/**

Operation details:

  - SOAP RPC encodingStyle="http://schemas.xmlsoap.org/soap/encoding/"
  - Request message has mandatory header part(s):
    - wsse__Security
  - Response message has mandatory header part(s):
    - wsse__Security

C stub function (defined in soapClient.c[pp]):
@code
  int soap_call_ns1__add(struct soap *soap,
    NULL, // char *endpoint = NULL selects default endpoint for this operation
    NULL, // char *action = NULL selects default action for this operation
    // request parameters:
    double                              a,
    double                              b,
    // response parameters:
    double                             *result
  );
@endcode

*/

//gsoap ns1  service method-style:	add rpc
//gsoap ns1  service method-encoding:	add http://schemas.xmlsoap.org/soap/encoding/
//gsoap ns1  service method-action:	add ""
//gsoap ns1  service method-input-header-part:	add wsse__Security
//gsoap ns1  service method-output-header-part:	add wsse__Security
int ns1__add(
    double                              a,
    double                              b,
    double                             *result ///< response parameter
);

/******************************************************************************\
 *                                                                            *
 * ns1__sub                                                                   *
 *                                                                            *
\******************************************************************************/


/// Operation "ns1__sub" of service binding "wssetest"

/**

Operation details:

  - SOAP RPC encodingStyle="http://schemas.xmlsoap.org/soap/encoding/"
  - Request message has mandatory header part(s):
    - wsse__Security
  - Response message has mandatory header part(s):
    - wsse__Security

C stub function (defined in soapClient.c[pp]):
@code
  int soap_call_ns1__sub(struct soap *soap,
    NULL, // char *endpoint = NULL selects default endpoint for this operation
    NULL, // char *action = NULL selects default action for this operation
    // request parameters:
    double                              a,
    double                              b,
    // response parameters:
    double                             *result
  );
@endcode

*/

//gsoap ns1  service method-style:	sub rpc
//gsoap ns1  service method-encoding:	sub http://schemas.xmlsoap.org/soap/encoding/
//gsoap ns1  service method-action:	sub ""
//gsoap ns1  service method-input-header-part:	sub wsse__Security
//gsoap ns1  service method-output-header-part:	sub wsse__Security
int ns1__sub(
    double                              a,
    double                              b,
    double                             *result ///< response parameter
);

/******************************************************************************\
 *                                                                            *
 * ns1__mul                                                                   *
 *                                                                            *
\******************************************************************************/


/// Operation "ns1__mul" of service binding "wssetest"

/**

Operation details:

  - SOAP RPC encodingStyle="http://schemas.xmlsoap.org/soap/encoding/"
  - Request message has mandatory header part(s):
    - wsse__Security
  - Response message has mandatory header part(s):
    - wsse__Security

C stub function (defined in soapClient.c[pp]):
@code
  int soap_call_ns1__mul(struct soap *soap,
    NULL, // char *endpoint = NULL selects default endpoint for this operation
    NULL, // char *action = NULL selects default action for this operation
    // request parameters:
    double                              a,
    double                              b,
    // response parameters:
    double                             *result
  );
@endcode

*/

//gsoap ns1  service method-style:	mul rpc
//gsoap ns1  service method-encoding:	mul http://schemas.xmlsoap.org/soap/encoding/
//gsoap ns1  service method-action:	mul ""
//gsoap ns1  service method-input-header-part:	mul wsse__Security
//gsoap ns1  service method-output-header-part:	mul wsse__Security
int ns1__mul(
    double                              a,
    double                              b,
    double                             *result ///< response parameter
);

/******************************************************************************\
 *                                                                            *
 * ns1__div                                                                   *
 *                                                                            *
\******************************************************************************/


/// Operation "ns1__div" of service binding "wssetest"

/**

Operation details:

  - SOAP RPC encodingStyle="http://schemas.xmlsoap.org/soap/encoding/"
  - Request message has mandatory header part(s):
    - wsse__Security
  - Response message has mandatory header part(s):
    - wsse__Security

C stub function (defined in soapClient.c[pp]):
@code
  int soap_call_ns1__div(struct soap *soap,
    NULL, // char *endpoint = NULL selects default endpoint for this operation
    NULL, // char *action = NULL selects default action for this operation
    // request parameters:
    double                              a,
    double                              b,
    // response parameters:
    double                             *result
  );
@endcode

*/

//gsoap ns1  service method-style:	div rpc
//gsoap ns1  service method-encoding:	div http://schemas.xmlsoap.org/soap/encoding/
//gsoap ns1  service method-action:	div ""
//gsoap ns1  service method-input-header-part:	div wsse__Security
//gsoap ns1  service method-output-header-part:	div wsse__Security
int ns1__div(
    double                              a,
    double                              b,
    double                             *result ///< response parameter
);

/* End of wssedemo.h */
