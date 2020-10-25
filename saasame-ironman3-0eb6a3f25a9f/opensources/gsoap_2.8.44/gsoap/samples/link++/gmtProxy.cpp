/* gmtProxy.cpp
   Generated by gSOAP 2.8.10 from gmt.h

Copyright(C) 2000-2012, Robert van Engelen, Genivia Inc. All Rights Reserved.
The generated code is released under one of the following licenses:
1) 2) Genivia's license for commercial use.
*/

#include "gmtProxy.h"

namespace gmt {

Proxy::Proxy()
{	Proxy_init(SOAP_IO_DEFAULT, SOAP_IO_DEFAULT);
}

Proxy::Proxy(const struct soap &_soap) : soap(_soap)
{ }

Proxy::Proxy(const char *url)
{	Proxy_init(SOAP_IO_DEFAULT, SOAP_IO_DEFAULT);
	soap_endpoint = url;
}

Proxy::Proxy(soap_mode iomode)
{	Proxy_init(iomode, iomode);
}

Proxy::Proxy(const char *url, soap_mode iomode)
{	Proxy_init(iomode, iomode);
	soap_endpoint = url;
}

Proxy::Proxy(soap_mode imode, soap_mode omode)
{	Proxy_init(imode, omode);
}

Proxy::~Proxy()
{ }

void Proxy::Proxy_init(soap_mode imode, soap_mode omode)
{	soap_imode(this, imode);
	soap_omode(this, omode);
	soap_endpoint = NULL;
	static const struct Namespace namespaces[] =
{
	{"SOAP-ENV", "http://schemas.xmlsoap.org/soap/envelope/", "http://www.w3.org/*/soap-envelope", NULL},
	{"SOAP-ENC", "http://schemas.xmlsoap.org/soap/encoding/", "http://www.w3.org/*/soap-encoding", NULL},
	{"xsi", "http://www.w3.org/2001/XMLSchema-instance", "http://www.w3.org/*/XMLSchema-instance", NULL},
	{"xsd", "http://www.w3.org/2001/XMLSchema", "http://www.w3.org/*/XMLSchema", NULL},
	{"t", "http://tempuri.org/t.xsd", NULL, NULL},
	{NULL, NULL, NULL, NULL}
};
	soap_set_namespaces(this, namespaces);
}

void Proxy::destroy()
{	soap_destroy(this);
	soap_end(this);
}

void Proxy::reset()
{	destroy();
	soap_done(this);
	soap_init(this);
	Proxy_init(SOAP_IO_DEFAULT, SOAP_IO_DEFAULT);
}

void Proxy::soap_noheader()
{	this->header = NULL;
}

const SOAP_ENV__Header *Proxy::soap_header()
{	return this->header;
}

const SOAP_ENV__Fault *Proxy::soap_fault()
{	return this->fault;
}

int Proxy::soap_close_socket()
{	return soap_closesock(this);
}

int Proxy::soap_force_close_socket()
{	return soap_force_closesock(this);
}

void Proxy::soap_print_fault(FILE *fd)
{	::soap_print_fault(this, fd);
}

#ifndef WITH_LEAN
#ifndef WITH_COMPAT
void Proxy::soap_stream_fault(std::ostream& os)
{	::soap_stream_fault(this, os);
}
#endif

char *Proxy::soap_sprint_fault(char *buf, size_t len)
{	return ::soap_sprint_fault(this, buf, len);
}
#endif

int Proxy::gmt(const char *endpoint, const char *soap_action, time_t *_param_1)
{	struct soap *soap = this;
	struct t__gmt soap_tmp_t__gmt;
	struct t__gmtResponse *soap_tmp_t__gmtResponse;
	if (endpoint)
		soap_endpoint = endpoint;
	soap->encodingStyle = NULL;
	soap_begin(soap);
	soap_serializeheader(soap);
	soap_serialize_t__gmt(soap, &soap_tmp_t__gmt);
	if (soap_begin_count(soap))
		return soap->error;
	if (soap->mode & SOAP_IO_LENGTH)
	{	if (soap_envelope_begin_out(soap)
		 || soap_putheader(soap)
		 || soap_body_begin_out(soap)
		 || soap_put_t__gmt(soap, &soap_tmp_t__gmt, "t:gmt", NULL)
		 || soap_body_end_out(soap)
		 || soap_envelope_end_out(soap))
			 return soap->error;
	}
	if (soap_end_count(soap))
		return soap->error;
	if (soap_connect(soap, soap_endpoint, soap_action)
	 || soap_envelope_begin_out(soap)
	 || soap_putheader(soap)
	 || soap_body_begin_out(soap)
	 || soap_put_t__gmt(soap, &soap_tmp_t__gmt, "t:gmt", NULL)
	 || soap_body_end_out(soap)
	 || soap_envelope_end_out(soap)
	 || soap_end_send(soap))
		return soap_closesock(soap);
	if (!_param_1)
		return soap_closesock(soap);
	soap_default_time(soap, _param_1);
	if (soap_begin_recv(soap)
	 || soap_envelope_begin_in(soap)
	 || soap_recv_header(soap)
	 || soap_body_begin_in(soap))
		return soap_closesock(soap);
	soap_tmp_t__gmtResponse = soap_get_t__gmtResponse(soap, NULL, "t:gmtResponse", "");
	if (soap->error)
		return soap_recv_fault(soap, 0);
	if (soap_body_end_in(soap)
	 || soap_envelope_end_in(soap)
	 || soap_end_recv(soap))
		return soap_closesock(soap);
	if (_param_1 && soap_tmp_t__gmtResponse->_param_1)
		*_param_1 = *soap_tmp_t__gmtResponse->_param_1;
	return soap_closesock(soap);
}

} // namespace gmt

/* End of client proxy code */
