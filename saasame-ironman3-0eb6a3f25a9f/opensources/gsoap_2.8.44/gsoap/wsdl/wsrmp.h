/*
	wsrmp.h

	WS-ReliableMessaging Policy

--------------------------------------------------------------------------------
gSOAP XML Web services tools
Copyright (C) 2001-2010, Robert van Engelen, Genivia Inc. All Rights Reserved.
This software is released under one of the following licenses:
Genivia's license for commercial use.
--------------------------------------------------------------------------------
Product and source code licensed by Genivia, Inc., contact@genivia.com
--------------------------------------------------------------------------------

*/

//gsoap wsrmp schema documentation:	WS-ReliableMessaging Policy binding
//gsoap wsrmp schema namespace:		http://docs.oasis-open.org/ws-rx/wsrm/200702
//gsoap wsrmp schema elementForm:	qualified             
//gsoap wsrmp schema attributeForm:	unqualified           

//gsoap wsrmp5 schema documentation:	WS-ReliableMessaging Policy binding
//gsoap wsrmp5 schema namespace:	http://schemas.xmlsoap.org/ws/2005/02/rm/policy
//gsoap wsrmp5 schema elementForm:	qualified             
//gsoap wsrmp5 schema attributeForm:	unqualified           

#import "imports.h"

class wsrmp__Timeout
{ public:
  @char *Milliseconds;
};

class wsrmp__RMAssertion : public wsp__Assertion
{ public:
  wsrmp__Timeout *InactivityTimeout;
  wsrmp__Timeout *BaseRetransmissionInterval;;
  wsrmp__Timeout *AcknowledgementInterval;
  char           *ExponentialBackoff;
// TODO: WCF netrmp extension elements go here, as necessary
};

class wsrmp5__Timeout
{ public:
  @char *Milliseconds;
};

class wsrmp5__RMAssertion : public wsp__Assertion
{ public:
  wsrmp5__Timeout *InactivityTimeout;
  wsrmp5__Timeout *BaseRetransmissionInterval;;
  wsrmp5__Timeout *AcknowledgementInterval;
  char           *ExponentialBackoff;
// TODO: WCF netrmp extension elements go here, as necessary
};

