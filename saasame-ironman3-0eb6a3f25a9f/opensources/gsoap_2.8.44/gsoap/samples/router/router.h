/*
	router.h

	Simple Web Service message router (relay server)

--------------------------------------------------------------------------------
gSOAP XML Web services tools
Copyright (C) 2001-2016, Robert van Engelen, Genivia, Inc. All Rights Reserved.
This software is released under one of the following two licenses:
Genivia's license for commercial use.
--------------------------------------------------------------------------------
Product and source code licensed by Genivia, Inc., contact@genivia.com
--------------------------------------------------------------------------------
*/

struct t__Routing
{
  char *key;      // key matches SOAPAction or query string or endpoint URL
  char *endpoint;
  char *userid;	  // optional HTTP Authorization userid
  char *passwd;	  // optional HTTP Authorization passwd
};

struct t__RoutingTable
{
  int                __size; // size of array
  struct t__Routing *__ptr;  // array of Routing entries
};
