/*

udp.h

SOAP-over-UDP demo specification for udpclient and udpserver examples

Requires WS-Addressing from import/wsa.h

--------------------------------------------------------------------------------
gSOAP XML Web services tools
Copyright (C) 2001-2008, Robert van Engelen, Genivia, Inc. All Rights Reserved.
This software is released under one of the following two licenses:
Genivia's license for commercial use.
--------------------------------------------------------------------------------
Product and source code licensed by Genivia, Inc., contact@genivia.com
--------------------------------------------------------------------------------
*/

////////////////////////////////////////////////////////////////////////////////
//
//	Import WS-Addressing
//
////////////////////////////////////////////////////////////////////////////////

//gsoap wsa schema import: http://schemas.xmlsoap.org/ws/2004/08/addressing

#import "wsa.h"		// wsa.h is in the import directory

////////////////////////////////////////////////////////////////////////////////
//
//	Demo UDP service
//
////////////////////////////////////////////////////////////////////////////////

//gsoap ns service name:	udp
//gsoap ns service namespace:	urn:gsoap-udp-demo
//gsoap ns service location:	soap.udp://localhost:10000

//	I would have assumed UDP transport to be defined in the WSDL bindings,
//	but this doesn't appear to be possible, so we will use the default HTTP
//	transport binding and omit the transport directive below.
// ---> //gsoap ns service transport:	... ???

////////////////////////////////////////////////////////////////////////////////
//
//	SOAP-over-UDP SOAP Headers with WS-Addressing
//
////////////////////////////////////////////////////////////////////////////////

struct SOAP_ENV__Header
{
  mustUnderstand _wsa__MessageID      wsa__MessageID 0; ///< WS-Addressing
  mustUnderstand _wsa__RelatesTo     *wsa__RelatesTo 0; ///< WS-Addressing
  mustUnderstand _wsa__From          *wsa__From      0; ///< WS-Addressing
  mustUnderstand _wsa__ReplyTo       *wsa__ReplyTo   0; ///< WS-Addressing
  mustUnderstand _wsa__FaultTo       *wsa__FaultTo   0; ///< WS-Addressing
  mustUnderstand _wsa__To             wsa__To        0; ///< WS-Addressing
  mustUnderstand _wsa__Action         wsa__Action    0; ///< WS-Addressing
};

////////////////////////////////////////////////////////////////////////////////
//
//	echoString (anonymous) request-response MEP over UDP
//
////////////////////////////////////////////////////////////////////////////////

//gsoap ns service method-documentation: echoString Demo (anonymous) request-response MEP over UDP

//gsoap ns service method-header-part:		echoString wsa__MessageID
//gsoap ns service method-header-part:		echoString wsa__To
//gsoap ns service method-header-part:		echoString wsa__Action
//gsoap ns service method-input-header-part:	sendString wsa__ReplyTo
//gsoap ns service method-output-header-part:	echoString wsa__RelatesTo

int ns__echoString(char *str, char **res);

////////////////////////////////////////////////////////////////////////////////
//
//	sendString one-way MEP over UDP
//
////////////////////////////////////////////////////////////////////////////////

//gsoap ns service method-documentation: sendString Demo one-way MEP over UDP

//gsoap ns service method-header-part:		sendString wsa__MessageID
//gsoap ns service method-header-part:		sendString wsa__To
//gsoap ns service method-header-part:		sendString wsa__Action

int ns__sendString(char *str, void);
