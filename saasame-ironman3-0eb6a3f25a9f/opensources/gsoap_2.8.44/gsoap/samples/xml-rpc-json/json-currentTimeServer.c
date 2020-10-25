/*
	json-currentTimeServer.c

	JSON currenTime server (C version)
	CGI or stand-alone multi-threaded server

	Returns JSON message with current time to client.

	Compile:
	soapcpp2 -c -CSL xml-rpc.h
	cc -o json-currentTimeServer json-currentTimeServer.c json.c xml-rpc.c stdsoap2.c soapC.c
	Install as CGI on Web server
	Or run as stand-alone server (e.g. on port 18000):
	./json-currentTimeServer 18000

--------------------------------------------------------------------------------
gSOAP XML Web services tools
Copyright (C) 2001-2008, Robert van Engelen, Genivia, Inc. All Rights Reserved.
This software is released under one of the following two licenses:
Genivia's license for commercial use.
--------------------------------------------------------------------------------
Product and source code licensed by Genivia, Inc., contact@genivia.com
--------------------------------------------------------------------------------
*/

#include "json.h"

#include <unistd.h>
#ifdef _POSIX_THREADS
#include <pthread.h>    // use Pthreads
#endif

#define BACKLOG (100)	// Max. request backlog
#define MAX_THR (8)	// Max. threads to serve requests

int serve_request(struct soap*);

int main(int argc, char **argv)
{
  struct soap *ctx = soap_new1(SOAP_C_UTFSTRING);
  ctx->send_timeout = 10; // 10 sec
  ctx->recv_timeout = 10; // 10 sec

  if (argc < 2)
    return serve_request(ctx);

  int port = atoi(argv[1]);

#ifdef _POSIX_THREADS
  pthread_t tid;
#endif

  if (!soap_valid_socket(soap_bind(ctx, NULL, port, BACKLOG)))
  {
    soap_print_fault(ctx, stderr);
    exit(1);
  }

  for (;;)
  {
    if (!soap_valid_socket(soap_accept(ctx)))
    {
      soap_print_fault(ctx, stderr);
    }
    else
    {
#ifdef _POSIX_THREADS
      pthread_create(&tid, NULL, (void*(*)(void*))serve_request, (void*)soap_copy(ctx));
#else
      serve_request(ctx);
#endif
    }
  }
  soap_destroy(ctx);
  soap_end(ctx);
  soap_free(ctx);

  return 0;
}

int serve_request(struct soap* ctx)
{
#ifdef _POSIX_THREADS
  pthread_detach(pthread_self());
#endif

  struct value *request = new_value(ctx);
    
  // HTTP keep-alive max number of iterations
  unsigned int k = ctx->max_keep_alive;

  do
  {
    if (ctx->max_keep_alive > 0 && !--k)
      ctx->keep_alive = 0;

    // receive HTTP header (optional) and JSON content
    if (soap_begin_recv(ctx)
     || json_recv(ctx, request)
     || soap_end_recv(ctx))
      soap_send_fault(ctx);
    else
    {
      struct value *response = new_value(ctx);
  
      if (is_string(request) && !strcmp(*string_of(request), "getCurrentTime"))
        // method name matches: first parameter of response is time
        *dateTime_of(response) = soap_dateTime2s(ctx, time(0));
      else
      { // otherwise, set fault
        *string_of(value_at(response, "fault")) = "Wrong method";
        *value_at(response, "detail") = *request;
      }

      // http content type
      ctx->http_content = "application/json; charset=utf-8";
      // http content length
      if (soap_begin_count(ctx)
       || ((ctx->mode & SOAP_IO_LENGTH) && json_send(ctx, response))
       || soap_end_count(ctx)
       || soap_response(ctx, SOAP_FILE)
       || json_send(ctx, response)
       || soap_end_send(ctx))
        soap_print_fault(ctx, stderr);
    }
    // close (keep-alive may keep socket open when client supports it)
    soap_closesock(ctx);

  } while (ctx->keep_alive);

  int err = ctx->error;

  // clean up
  soap_destroy(ctx);
  soap_end(ctx);

#ifdef _POSIX_THREADS
  // free the ctx copy for this thread
  soap_free(ctx);
#endif

  return err;
}

/* Don't need a namespace table. We put an empty one here to avoid link errors */
struct Namespace namespaces[] = { {NULL, NULL} };

