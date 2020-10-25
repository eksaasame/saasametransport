/*
--------------------------------------------------------------------------------
gSOAP XML Web services tools
Copyright (C) 2001-2008, Robert van Engelen, Genivia, Inc. All Rights Reserved.
This software is released under one of the following two licenses:
Genivia's license for commercial use.
--------------------------------------------------------------------------------
Product and source code licensed by Genivia, Inc., contact@genivia.com
--------------------------------------------------------------------------------
*/

//gsoap ns service name:	Event Simple remote event handler
//gsoap ns service style:	rpc
//gsoap ns service encoding:	encoded
//gsoap ns service namespace:	http://www.cs.fsu.edu/~engelen/event.wsdl
//gsoap ns service location:	http://localhost:18000

//gsoap ns schema namespace:    urn:event

//gsoap ns schema type: event Set of four possible events
enum ns__event {
//gsoap ns schema type: event::EVENT_A first event
  EVENT_A,
//gsoap ns schema type: event::EVENT_B second event
  EVENT_B,
//gsoap ns schema type: event::EVENT_C third event
  EVENT_C,
//gsoap ns schema type: event::EVENT_Z final event
  EVENT_Z
};

//gsoap ns service method:              handle          Handles asynchronous events
//gsoap ns service method:              handle::event   EVENT_A, EVENT_B, EVENT_C, or EVENT_Z
//gsoap ns service method-action:       handle          "event"
int ns__handle(enum ns__event event, void);
