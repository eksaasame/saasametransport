/*
	xml-rpc-weblogs.cpp

	XML-RPC weblogUpdates.ping example in C++

	See http://xmlrpc.scripting.com/weblogsCom for more details.

	Requires xml-rpc.h and xml-rpc.cpp

	Compile:
	soapcpp2 xml-rpc.h
	cc xml-rpc-weblogs.cpp xml-rpc.cpp xml-rpc-io.cpp stdsoap2.cpp soapC.cpp

--------------------------------------------------------------------------------
gSOAP XML Web services tools
Copyright (C) 2001-2008, Robert van Engelen, Genivia, Inc. All Rights Reserved.
This software is released under one of the following two licenses:
Genivia's license for commercial use.
--------------------------------------------------------------------------------
Product and source code licensed by Genivia, Inc., contact@genivia.com
--------------------------------------------------------------------------------
*/

#include "soapH.h"
#include "xml-rpc-io.h"

using namespace std;

#define ENDPOINT "http://rpc.weblogs.com/RPC2"

int main()
{
  soap *ctx = soap_new1(SOAP_C_UTFSTRING);
  // define the weblogsUpdates.ping method
  methodCall ping(ctx, ENDPOINT, "weblogUpdates.ping");
  // set the two parameters of the method
  ping[0] = "Scripting News";
  ping[1] = "http://www.scripting.com/";
  // make the call
  params output = ping();
  // error?
  if (ping.error())
    soap_print_fault(ctx, stderr);
  else if (output.empty())
    cout << ping.fault() << endl;
  else
  { if (output[0].is_struct())
    { if (output[0]["flerror"].is_false())
        cout << "Success:" << endl;
      else
        cout << "Failure:" << endl;
      cout << output[0]["message"] << endl;
    }
    else
      cout << "Message format error:" << endl << output[0] << endl;
  }
  // delete all data
  soap_destroy(ctx);
  soap_end(ctx);
  // free context
  soap_free(ctx);
  return 0;
}

/* Don't need a namespace table. We put an empty one here to avoid link errors */
struct Namespace namespaces[] = { {NULL, NULL} };
