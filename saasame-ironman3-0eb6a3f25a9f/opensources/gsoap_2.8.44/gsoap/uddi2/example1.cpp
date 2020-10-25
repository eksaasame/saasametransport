/*

example1.cpp

Example UDDI V2 Client

--------------------------------------------------------------------------------
gSOAP XML Web services tools
Copyright (C) 2004-2005, Robert van Engelen, Genivia Inc. All Rights Reserved.
This software is released under one of the following two licenses:
Genivia's license for commercial use.
--------------------------------------------------------------------------------
Product and source code licensed by Genivia, Inc., contact@genivia.com
--------------------------------------------------------------------------------
*/

#include "inqH.h"

int main(int argc, char **argv)
{ 
  char *search_string = "magic";

  if (argc > 1)
    search_string = argv[1];

  // Create a gSOAP context
  struct soap *soap = soap_new();

  // Create an object to find a business
  uddi2__find_USCOREservice fs(soap, search_string);

  // Send the request
  uddi2__serviceList *sl = fs.send("http://uddi.xmethods.net/inquire");

  // Check if result is OK
  if (!sl)
    soap_print_fault(soap, stderr);

  // If OK, report the service name(s) and unique identification keys
  else if (sl->serviceInfos)
  {
    std::cout << "Search results on " << search_string << ":" << std::endl << std::endl;

    for (std::vector<uddi2__serviceInfo*>::const_iterator si = sl->serviceInfos->serviceInfo.begin(); si != sl->serviceInfos->serviceInfo.end(); ++si)
    {
      // Report serviceKey and businessKey
      std::cout << "serviceKey=" << (*si)->serviceKey << std::endl << "businessKey=" << (*si)->businessKey << std::endl;

      // Report names
      for (std::vector<uddi2__name*>::const_iterator n = (*si)->name.begin(); n != (*si)->name.end(); ++n)
        std::cout << "name=" << (*n)->__item << std::endl;

      std::cout << std::endl;
    }
  }

  // Remove deserialized objects
  soap_destroy(soap);

  // Remove temporary data
  soap_end(soap);

  // Detach and free context
  soap_done(soap);
  free(soap);

  return 0;
}
