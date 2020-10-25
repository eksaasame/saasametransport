/*
	mashupclient.cpp

	Example mashup service client in C++

	soapcpp2 -i mashup.hpp
	cc -o mashupclient mashupclient.cpp stdsoap2.cpp soapC.cpp soapmashupProxy.cpp

--------------------------------------------------------------------------------
gSOAP XML Web services tools
Copyright (C) 2001-2008, Robert van Engelen, Genivia, Inc. All Rights Reserved.
This software is released under one of the following two licenses:
Genivia's license for commercial use.
--------------------------------------------------------------------------------
Product and source code licensed by Genivia, Inc., contact@genivia.com
--------------------------------------------------------------------------------
*/

#include "soapmashupProxy.h"
#include "mashup.nsmap"

int main()
{
  mashupProxy proxy;
  _ns3__commingtotown response;

  if (proxy.dtx((char*)"", response))
    proxy.soap_stream_fault(std::cerr);
  else if (response.days == 0)
    std::cout << "Today is the day!" << std::endl;
  else
    std::cout << "Wait " << response.days << " more days to xmas" << std::endl;

  proxy.destroy(); // delete deserialized data

  return 0;
}
