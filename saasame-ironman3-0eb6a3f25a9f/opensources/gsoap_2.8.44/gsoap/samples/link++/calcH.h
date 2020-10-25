/* calcH.h
   Generated by gSOAP 2.8.10 from calc.h

Copyright(C) 2000-2012, Robert van Engelen, Genivia Inc. All Rights Reserved.
The generated code is released under one of the following licenses:
1) 2) Genivia's license for commercial use.
*/

#ifndef calcH_H
#define calcH_H
#include "calcStub.h"

namespace calc {
#ifndef WITH_NOIDREF
SOAP_FMAC3 void SOAP_FMAC4 soap_markelement(struct soap*, const void*, int);
SOAP_FMAC3 int SOAP_FMAC4 soap_putelement(struct soap*, const void*, const char*, int, int);
SOAP_FMAC3 void *SOAP_FMAC4 soap_getelement(struct soap*, int*);
SOAP_FMAC3 int SOAP_FMAC4 soap_putindependent(struct soap*);
SOAP_FMAC3 int SOAP_FMAC4 soap_getindependent(struct soap*);
#endif
SOAP_FMAC3 int SOAP_FMAC4 soap_ignore_element(struct soap*);

SOAP_FMAC3 const char ** SOAP_FMAC4 soap_faultcode(struct soap *soap);

SOAP_FMAC3 void * SOAP_FMAC4 calc_instantiate(struct soap*, int, const char*, const char*, size_t*);
SOAP_FMAC3 int SOAP_FMAC4 calc_fdelete(struct soap_clist*);
SOAP_FMAC3 void* SOAP_FMAC4 soap_class_id_enter(struct soap*, const char*, void*, int, size_t, const char*, const char*);

#ifndef SOAP_TYPE_calc_byte
#define SOAP_TYPE_calc_byte (3)
#endif
SOAP_FMAC3 void SOAP_FMAC4 soap_default_byte(struct soap*, char *);
SOAP_FMAC3 int SOAP_FMAC4 soap_out_byte(struct soap*, const char*, int, const char *, const char*);
SOAP_FMAC3 char * SOAP_FMAC4 soap_in_byte(struct soap*, const char*, char *, const char*);

#ifndef soap_write_byte
#define soap_write_byte(soap, data) ( soap_begin_send(soap) || calc::soap_put_byte(soap, data, "byte", NULL) || soap_end_send(soap) )
#endif


SOAP_FMAC3 int SOAP_FMAC4 soap_put_byte(struct soap*, const char *, const char*, const char*);

#ifndef soap_read_byte
#define soap_read_byte(soap, data) ( soap_begin_recv(soap) || !calc::soap_get_byte(soap, data, NULL, NULL) || soap_end_recv(soap) )
#endif

SOAP_FMAC3 char * SOAP_FMAC4 soap_get_byte(struct soap*, char *, const char*, const char*);

#ifndef SOAP_TYPE_calc_int
#define SOAP_TYPE_calc_int (1)
#endif
SOAP_FMAC3 void SOAP_FMAC4 soap_default_int(struct soap*, int *);
SOAP_FMAC3 int SOAP_FMAC4 soap_out_int(struct soap*, const char*, int, const int *, const char*);
SOAP_FMAC3 int * SOAP_FMAC4 soap_in_int(struct soap*, const char*, int *, const char*);

#ifndef soap_write_int
#define soap_write_int(soap, data) ( soap_begin_send(soap) || calc::soap_put_int(soap, data, "int", NULL) || soap_end_send(soap) )
#endif


SOAP_FMAC3 int SOAP_FMAC4 soap_put_int(struct soap*, const int *, const char*, const char*);

#ifndef soap_read_int
#define soap_read_int(soap, data) ( soap_begin_recv(soap) || !calc::soap_get_int(soap, data, NULL, NULL) || soap_end_recv(soap) )
#endif

SOAP_FMAC3 int * SOAP_FMAC4 soap_get_int(struct soap*, int *, const char*, const char*);

#ifndef SOAP_TYPE_calc_double
#define SOAP_TYPE_calc_double (7)
#endif
SOAP_FMAC3 void SOAP_FMAC4 soap_default_double(struct soap*, double *);
SOAP_FMAC3 int SOAP_FMAC4 soap_out_double(struct soap*, const char*, int, const double *, const char*);
SOAP_FMAC3 double * SOAP_FMAC4 soap_in_double(struct soap*, const char*, double *, const char*);

#ifndef soap_write_double
#define soap_write_double(soap, data) ( soap_begin_send(soap) || calc::soap_put_double(soap, data, "double", NULL) || soap_end_send(soap) )
#endif


SOAP_FMAC3 int SOAP_FMAC4 soap_put_double(struct soap*, const double *, const char*, const char*);

#ifndef soap_read_double
#define soap_read_double(soap, data) ( soap_begin_recv(soap) || !calc::soap_get_double(soap, data, NULL, NULL) || soap_end_recv(soap) )
#endif

SOAP_FMAC3 double * SOAP_FMAC4 soap_get_double(struct soap*, double *, const char*, const char*);

#ifndef WITH_NOGLOBAL

#ifndef SOAP_TYPE_calc_SOAP_ENV__Fault
#define SOAP_TYPE_calc_SOAP_ENV__Fault (31)
#endif
SOAP_FMAC3 void SOAP_FMAC4 soap_default_SOAP_ENV__Fault(struct soap*, struct SOAP_ENV__Fault *);
SOAP_FMAC3 void SOAP_FMAC4 soap_serialize_SOAP_ENV__Fault(struct soap*, const struct SOAP_ENV__Fault *);
SOAP_FMAC3 int SOAP_FMAC4 soap_out_SOAP_ENV__Fault(struct soap*, const char*, int, const struct SOAP_ENV__Fault *, const char*);
SOAP_FMAC3 struct SOAP_ENV__Fault * SOAP_FMAC4 soap_in_SOAP_ENV__Fault(struct soap*, const char*, struct SOAP_ENV__Fault *, const char*);

#ifndef soap_write_SOAP_ENV__Fault
#define soap_write_SOAP_ENV__Fault(soap, data) ( soap_begin_send(soap) || (calc::soap_serialize_SOAP_ENV__Fault(soap, data), 0) || calc::soap_put_SOAP_ENV__Fault(soap, data, "SOAP-ENV:Fault", NULL) || soap_end_send(soap) )
#endif


SOAP_FMAC3 int SOAP_FMAC4 soap_put_SOAP_ENV__Fault(struct soap*, const struct SOAP_ENV__Fault *, const char*, const char*);

#ifndef soap_read_SOAP_ENV__Fault
#define soap_read_SOAP_ENV__Fault(soap, data) ( soap_begin_recv(soap) || !calc::soap_get_SOAP_ENV__Fault(soap, data, NULL, NULL) || soap_end_recv(soap) )
#endif

SOAP_FMAC3 struct SOAP_ENV__Fault * SOAP_FMAC4 soap_get_SOAP_ENV__Fault(struct soap*, struct SOAP_ENV__Fault *, const char*, const char*);

#define soap_new_SOAP_ENV__Fault(soap, n) soap_instantiate_SOAP_ENV__Fault(soap, n, NULL, NULL, NULL)


#define soap_delete_SOAP_ENV__Fault(soap, p) soap_delete(soap, p)

SOAP_FMAC1 struct SOAP_ENV__Fault * SOAP_FMAC2 soap_instantiate_SOAP_ENV__Fault(struct soap*, int, const char*, const char*, size_t*);
SOAP_FMAC3 void SOAP_FMAC4 soap_copy_SOAP_ENV__Fault(struct soap*, int, int, void*, size_t, const void*, size_t);

#endif

#ifndef WITH_NOGLOBAL

#ifndef SOAP_TYPE_calc_SOAP_ENV__Reason
#define SOAP_TYPE_calc_SOAP_ENV__Reason (30)
#endif
SOAP_FMAC3 void SOAP_FMAC4 soap_default_SOAP_ENV__Reason(struct soap*, struct SOAP_ENV__Reason *);
SOAP_FMAC3 void SOAP_FMAC4 soap_serialize_SOAP_ENV__Reason(struct soap*, const struct SOAP_ENV__Reason *);
SOAP_FMAC3 int SOAP_FMAC4 soap_out_SOAP_ENV__Reason(struct soap*, const char*, int, const struct SOAP_ENV__Reason *, const char*);
SOAP_FMAC3 struct SOAP_ENV__Reason * SOAP_FMAC4 soap_in_SOAP_ENV__Reason(struct soap*, const char*, struct SOAP_ENV__Reason *, const char*);

#ifndef soap_write_SOAP_ENV__Reason
#define soap_write_SOAP_ENV__Reason(soap, data) ( soap_begin_send(soap) || (calc::soap_serialize_SOAP_ENV__Reason(soap, data), 0) || calc::soap_put_SOAP_ENV__Reason(soap, data, "SOAP-ENV:Reason", NULL) || soap_end_send(soap) )
#endif


SOAP_FMAC3 int SOAP_FMAC4 soap_put_SOAP_ENV__Reason(struct soap*, const struct SOAP_ENV__Reason *, const char*, const char*);

#ifndef soap_read_SOAP_ENV__Reason
#define soap_read_SOAP_ENV__Reason(soap, data) ( soap_begin_recv(soap) || !calc::soap_get_SOAP_ENV__Reason(soap, data, NULL, NULL) || soap_end_recv(soap) )
#endif

SOAP_FMAC3 struct SOAP_ENV__Reason * SOAP_FMAC4 soap_get_SOAP_ENV__Reason(struct soap*, struct SOAP_ENV__Reason *, const char*, const char*);

#define soap_new_SOAP_ENV__Reason(soap, n) soap_instantiate_SOAP_ENV__Reason(soap, n, NULL, NULL, NULL)


#define soap_delete_SOAP_ENV__Reason(soap, p) soap_delete(soap, p)

SOAP_FMAC1 struct SOAP_ENV__Reason * SOAP_FMAC2 soap_instantiate_SOAP_ENV__Reason(struct soap*, int, const char*, const char*, size_t*);
SOAP_FMAC3 void SOAP_FMAC4 soap_copy_SOAP_ENV__Reason(struct soap*, int, int, void*, size_t, const void*, size_t);

#endif

#ifndef WITH_NOGLOBAL

#ifndef SOAP_TYPE_calc_SOAP_ENV__Detail
#define SOAP_TYPE_calc_SOAP_ENV__Detail (27)
#endif
SOAP_FMAC3 void SOAP_FMAC4 soap_default_SOAP_ENV__Detail(struct soap*, struct SOAP_ENV__Detail *);
SOAP_FMAC3 void SOAP_FMAC4 soap_serialize_SOAP_ENV__Detail(struct soap*, const struct SOAP_ENV__Detail *);
SOAP_FMAC3 int SOAP_FMAC4 soap_out_SOAP_ENV__Detail(struct soap*, const char*, int, const struct SOAP_ENV__Detail *, const char*);
SOAP_FMAC3 struct SOAP_ENV__Detail * SOAP_FMAC4 soap_in_SOAP_ENV__Detail(struct soap*, const char*, struct SOAP_ENV__Detail *, const char*);

#ifndef soap_write_SOAP_ENV__Detail
#define soap_write_SOAP_ENV__Detail(soap, data) ( soap_begin_send(soap) || (calc::soap_serialize_SOAP_ENV__Detail(soap, data), 0) || calc::soap_put_SOAP_ENV__Detail(soap, data, "SOAP-ENV:Detail", NULL) || soap_end_send(soap) )
#endif


SOAP_FMAC3 int SOAP_FMAC4 soap_put_SOAP_ENV__Detail(struct soap*, const struct SOAP_ENV__Detail *, const char*, const char*);

#ifndef soap_read_SOAP_ENV__Detail
#define soap_read_SOAP_ENV__Detail(soap, data) ( soap_begin_recv(soap) || !calc::soap_get_SOAP_ENV__Detail(soap, data, NULL, NULL) || soap_end_recv(soap) )
#endif

SOAP_FMAC3 struct SOAP_ENV__Detail * SOAP_FMAC4 soap_get_SOAP_ENV__Detail(struct soap*, struct SOAP_ENV__Detail *, const char*, const char*);

#define soap_new_SOAP_ENV__Detail(soap, n) soap_instantiate_SOAP_ENV__Detail(soap, n, NULL, NULL, NULL)


#define soap_delete_SOAP_ENV__Detail(soap, p) soap_delete(soap, p)

SOAP_FMAC1 struct SOAP_ENV__Detail * SOAP_FMAC2 soap_instantiate_SOAP_ENV__Detail(struct soap*, int, const char*, const char*, size_t*);
SOAP_FMAC3 void SOAP_FMAC4 soap_copy_SOAP_ENV__Detail(struct soap*, int, int, void*, size_t, const void*, size_t);

#endif

#ifndef WITH_NOGLOBAL

#ifndef SOAP_TYPE_calc_SOAP_ENV__Code
#define SOAP_TYPE_calc_SOAP_ENV__Code (25)
#endif
SOAP_FMAC3 void SOAP_FMAC4 soap_default_SOAP_ENV__Code(struct soap*, struct SOAP_ENV__Code *);
SOAP_FMAC3 void SOAP_FMAC4 soap_serialize_SOAP_ENV__Code(struct soap*, const struct SOAP_ENV__Code *);
SOAP_FMAC3 int SOAP_FMAC4 soap_out_SOAP_ENV__Code(struct soap*, const char*, int, const struct SOAP_ENV__Code *, const char*);
SOAP_FMAC3 struct SOAP_ENV__Code * SOAP_FMAC4 soap_in_SOAP_ENV__Code(struct soap*, const char*, struct SOAP_ENV__Code *, const char*);

#ifndef soap_write_SOAP_ENV__Code
#define soap_write_SOAP_ENV__Code(soap, data) ( soap_begin_send(soap) || (calc::soap_serialize_SOAP_ENV__Code(soap, data), 0) || calc::soap_put_SOAP_ENV__Code(soap, data, "SOAP-ENV:Code", NULL) || soap_end_send(soap) )
#endif


SOAP_FMAC3 int SOAP_FMAC4 soap_put_SOAP_ENV__Code(struct soap*, const struct SOAP_ENV__Code *, const char*, const char*);

#ifndef soap_read_SOAP_ENV__Code
#define soap_read_SOAP_ENV__Code(soap, data) ( soap_begin_recv(soap) || !calc::soap_get_SOAP_ENV__Code(soap, data, NULL, NULL) || soap_end_recv(soap) )
#endif

SOAP_FMAC3 struct SOAP_ENV__Code * SOAP_FMAC4 soap_get_SOAP_ENV__Code(struct soap*, struct SOAP_ENV__Code *, const char*, const char*);

#define soap_new_SOAP_ENV__Code(soap, n) soap_instantiate_SOAP_ENV__Code(soap, n, NULL, NULL, NULL)


#define soap_delete_SOAP_ENV__Code(soap, p) soap_delete(soap, p)

SOAP_FMAC1 struct SOAP_ENV__Code * SOAP_FMAC2 soap_instantiate_SOAP_ENV__Code(struct soap*, int, const char*, const char*, size_t*);
SOAP_FMAC3 void SOAP_FMAC4 soap_copy_SOAP_ENV__Code(struct soap*, int, int, void*, size_t, const void*, size_t);

#endif

#ifndef WITH_NOGLOBAL

#ifndef SOAP_TYPE_calc_SOAP_ENV__Header
#define SOAP_TYPE_calc_SOAP_ENV__Header (24)
#endif
SOAP_FMAC3 void SOAP_FMAC4 soap_default_SOAP_ENV__Header(struct soap*, struct SOAP_ENV__Header *);
SOAP_FMAC3 void SOAP_FMAC4 soap_serialize_SOAP_ENV__Header(struct soap*, const struct SOAP_ENV__Header *);
SOAP_FMAC3 int SOAP_FMAC4 soap_out_SOAP_ENV__Header(struct soap*, const char*, int, const struct SOAP_ENV__Header *, const char*);
SOAP_FMAC3 struct SOAP_ENV__Header * SOAP_FMAC4 soap_in_SOAP_ENV__Header(struct soap*, const char*, struct SOAP_ENV__Header *, const char*);

#ifndef soap_write_SOAP_ENV__Header
#define soap_write_SOAP_ENV__Header(soap, data) ( soap_begin_send(soap) || (calc::soap_serialize_SOAP_ENV__Header(soap, data), 0) || calc::soap_put_SOAP_ENV__Header(soap, data, "SOAP-ENV:Header", NULL) || soap_end_send(soap) )
#endif


SOAP_FMAC3 int SOAP_FMAC4 soap_put_SOAP_ENV__Header(struct soap*, const struct SOAP_ENV__Header *, const char*, const char*);

#ifndef soap_read_SOAP_ENV__Header
#define soap_read_SOAP_ENV__Header(soap, data) ( soap_begin_recv(soap) || !calc::soap_get_SOAP_ENV__Header(soap, data, NULL, NULL) || soap_end_recv(soap) )
#endif

SOAP_FMAC3 struct SOAP_ENV__Header * SOAP_FMAC4 soap_get_SOAP_ENV__Header(struct soap*, struct SOAP_ENV__Header *, const char*, const char*);

#define soap_new_SOAP_ENV__Header(soap, n) soap_instantiate_SOAP_ENV__Header(soap, n, NULL, NULL, NULL)


#define soap_delete_SOAP_ENV__Header(soap, p) soap_delete(soap, p)

SOAP_FMAC1 struct SOAP_ENV__Header * SOAP_FMAC2 soap_instantiate_SOAP_ENV__Header(struct soap*, int, const char*, const char*, size_t*);
SOAP_FMAC3 void SOAP_FMAC4 soap_copy_SOAP_ENV__Header(struct soap*, int, int, void*, size_t, const void*, size_t);

#endif

#ifndef SOAP_TYPE_calc_ns__pow
#define SOAP_TYPE_calc_ns__pow (23)
#endif
SOAP_FMAC3 void SOAP_FMAC4 soap_default_ns__pow(struct soap*, struct ns__pow *);
SOAP_FMAC3 void SOAP_FMAC4 soap_serialize_ns__pow(struct soap*, const struct ns__pow *);
SOAP_FMAC3 int SOAP_FMAC4 soap_out_ns__pow(struct soap*, const char*, int, const struct ns__pow *, const char*);
SOAP_FMAC3 struct ns__pow * SOAP_FMAC4 soap_in_ns__pow(struct soap*, const char*, struct ns__pow *, const char*);

#ifndef soap_write_ns__pow
#define soap_write_ns__pow(soap, data) ( soap_begin_send(soap) || (calc::soap_serialize_ns__pow(soap, data), 0) || calc::soap_put_ns__pow(soap, data, "ns:pow", NULL) || soap_end_send(soap) )
#endif


SOAP_FMAC3 int SOAP_FMAC4 soap_put_ns__pow(struct soap*, const struct ns__pow *, const char*, const char*);

#ifndef soap_read_ns__pow
#define soap_read_ns__pow(soap, data) ( soap_begin_recv(soap) || !calc::soap_get_ns__pow(soap, data, NULL, NULL) || soap_end_recv(soap) )
#endif

SOAP_FMAC3 struct ns__pow * SOAP_FMAC4 soap_get_ns__pow(struct soap*, struct ns__pow *, const char*, const char*);

#define soap_new_ns__pow(soap, n) soap_instantiate_ns__pow(soap, n, NULL, NULL, NULL)


#define soap_delete_ns__pow(soap, p) soap_delete(soap, p)

SOAP_FMAC1 struct ns__pow * SOAP_FMAC2 soap_instantiate_ns__pow(struct soap*, int, const char*, const char*, size_t*);
SOAP_FMAC3 void SOAP_FMAC4 soap_copy_ns__pow(struct soap*, int, int, void*, size_t, const void*, size_t);

#ifndef SOAP_TYPE_calc_ns__powResponse
#define SOAP_TYPE_calc_ns__powResponse (22)
#endif
SOAP_FMAC3 void SOAP_FMAC4 soap_default_ns__powResponse(struct soap*, struct ns__powResponse *);
SOAP_FMAC3 void SOAP_FMAC4 soap_serialize_ns__powResponse(struct soap*, const struct ns__powResponse *);
SOAP_FMAC3 int SOAP_FMAC4 soap_out_ns__powResponse(struct soap*, const char*, int, const struct ns__powResponse *, const char*);
SOAP_FMAC3 struct ns__powResponse * SOAP_FMAC4 soap_in_ns__powResponse(struct soap*, const char*, struct ns__powResponse *, const char*);

#ifndef soap_write_ns__powResponse
#define soap_write_ns__powResponse(soap, data) ( soap_begin_send(soap) || (calc::soap_serialize_ns__powResponse(soap, data), 0) || calc::soap_put_ns__powResponse(soap, data, "ns:powResponse", NULL) || soap_end_send(soap) )
#endif


SOAP_FMAC3 int SOAP_FMAC4 soap_put_ns__powResponse(struct soap*, const struct ns__powResponse *, const char*, const char*);

#ifndef soap_read_ns__powResponse
#define soap_read_ns__powResponse(soap, data) ( soap_begin_recv(soap) || !calc::soap_get_ns__powResponse(soap, data, NULL, NULL) || soap_end_recv(soap) )
#endif

SOAP_FMAC3 struct ns__powResponse * SOAP_FMAC4 soap_get_ns__powResponse(struct soap*, struct ns__powResponse *, const char*, const char*);

#define soap_new_ns__powResponse(soap, n) soap_instantiate_ns__powResponse(soap, n, NULL, NULL, NULL)


#define soap_delete_ns__powResponse(soap, p) soap_delete(soap, p)

SOAP_FMAC1 struct ns__powResponse * SOAP_FMAC2 soap_instantiate_ns__powResponse(struct soap*, int, const char*, const char*, size_t*);
SOAP_FMAC3 void SOAP_FMAC4 soap_copy_ns__powResponse(struct soap*, int, int, void*, size_t, const void*, size_t);

#ifndef SOAP_TYPE_calc_ns__div
#define SOAP_TYPE_calc_ns__div (20)
#endif
SOAP_FMAC3 void SOAP_FMAC4 soap_default_ns__div(struct soap*, struct ns__div *);
SOAP_FMAC3 void SOAP_FMAC4 soap_serialize_ns__div(struct soap*, const struct ns__div *);
SOAP_FMAC3 int SOAP_FMAC4 soap_out_ns__div(struct soap*, const char*, int, const struct ns__div *, const char*);
SOAP_FMAC3 struct ns__div * SOAP_FMAC4 soap_in_ns__div(struct soap*, const char*, struct ns__div *, const char*);

#ifndef soap_write_ns__div
#define soap_write_ns__div(soap, data) ( soap_begin_send(soap) || (calc::soap_serialize_ns__div(soap, data), 0) || calc::soap_put_ns__div(soap, data, "ns:div", NULL) || soap_end_send(soap) )
#endif


SOAP_FMAC3 int SOAP_FMAC4 soap_put_ns__div(struct soap*, const struct ns__div *, const char*, const char*);

#ifndef soap_read_ns__div
#define soap_read_ns__div(soap, data) ( soap_begin_recv(soap) || !calc::soap_get_ns__div(soap, data, NULL, NULL) || soap_end_recv(soap) )
#endif

SOAP_FMAC3 struct ns__div * SOAP_FMAC4 soap_get_ns__div(struct soap*, struct ns__div *, const char*, const char*);

#define soap_new_ns__div(soap, n) soap_instantiate_ns__div(soap, n, NULL, NULL, NULL)


#define soap_delete_ns__div(soap, p) soap_delete(soap, p)

SOAP_FMAC1 struct ns__div * SOAP_FMAC2 soap_instantiate_ns__div(struct soap*, int, const char*, const char*, size_t*);
SOAP_FMAC3 void SOAP_FMAC4 soap_copy_ns__div(struct soap*, int, int, void*, size_t, const void*, size_t);

#ifndef SOAP_TYPE_calc_ns__divResponse
#define SOAP_TYPE_calc_ns__divResponse (19)
#endif
SOAP_FMAC3 void SOAP_FMAC4 soap_default_ns__divResponse(struct soap*, struct ns__divResponse *);
SOAP_FMAC3 void SOAP_FMAC4 soap_serialize_ns__divResponse(struct soap*, const struct ns__divResponse *);
SOAP_FMAC3 int SOAP_FMAC4 soap_out_ns__divResponse(struct soap*, const char*, int, const struct ns__divResponse *, const char*);
SOAP_FMAC3 struct ns__divResponse * SOAP_FMAC4 soap_in_ns__divResponse(struct soap*, const char*, struct ns__divResponse *, const char*);

#ifndef soap_write_ns__divResponse
#define soap_write_ns__divResponse(soap, data) ( soap_begin_send(soap) || (calc::soap_serialize_ns__divResponse(soap, data), 0) || calc::soap_put_ns__divResponse(soap, data, "ns:divResponse", NULL) || soap_end_send(soap) )
#endif


SOAP_FMAC3 int SOAP_FMAC4 soap_put_ns__divResponse(struct soap*, const struct ns__divResponse *, const char*, const char*);

#ifndef soap_read_ns__divResponse
#define soap_read_ns__divResponse(soap, data) ( soap_begin_recv(soap) || !calc::soap_get_ns__divResponse(soap, data, NULL, NULL) || soap_end_recv(soap) )
#endif

SOAP_FMAC3 struct ns__divResponse * SOAP_FMAC4 soap_get_ns__divResponse(struct soap*, struct ns__divResponse *, const char*, const char*);

#define soap_new_ns__divResponse(soap, n) soap_instantiate_ns__divResponse(soap, n, NULL, NULL, NULL)


#define soap_delete_ns__divResponse(soap, p) soap_delete(soap, p)

SOAP_FMAC1 struct ns__divResponse * SOAP_FMAC2 soap_instantiate_ns__divResponse(struct soap*, int, const char*, const char*, size_t*);
SOAP_FMAC3 void SOAP_FMAC4 soap_copy_ns__divResponse(struct soap*, int, int, void*, size_t, const void*, size_t);

#ifndef SOAP_TYPE_calc_ns__mul
#define SOAP_TYPE_calc_ns__mul (17)
#endif
SOAP_FMAC3 void SOAP_FMAC4 soap_default_ns__mul(struct soap*, struct ns__mul *);
SOAP_FMAC3 void SOAP_FMAC4 soap_serialize_ns__mul(struct soap*, const struct ns__mul *);
SOAP_FMAC3 int SOAP_FMAC4 soap_out_ns__mul(struct soap*, const char*, int, const struct ns__mul *, const char*);
SOAP_FMAC3 struct ns__mul * SOAP_FMAC4 soap_in_ns__mul(struct soap*, const char*, struct ns__mul *, const char*);

#ifndef soap_write_ns__mul
#define soap_write_ns__mul(soap, data) ( soap_begin_send(soap) || (calc::soap_serialize_ns__mul(soap, data), 0) || calc::soap_put_ns__mul(soap, data, "ns:mul", NULL) || soap_end_send(soap) )
#endif


SOAP_FMAC3 int SOAP_FMAC4 soap_put_ns__mul(struct soap*, const struct ns__mul *, const char*, const char*);

#ifndef soap_read_ns__mul
#define soap_read_ns__mul(soap, data) ( soap_begin_recv(soap) || !calc::soap_get_ns__mul(soap, data, NULL, NULL) || soap_end_recv(soap) )
#endif

SOAP_FMAC3 struct ns__mul * SOAP_FMAC4 soap_get_ns__mul(struct soap*, struct ns__mul *, const char*, const char*);

#define soap_new_ns__mul(soap, n) soap_instantiate_ns__mul(soap, n, NULL, NULL, NULL)


#define soap_delete_ns__mul(soap, p) soap_delete(soap, p)

SOAP_FMAC1 struct ns__mul * SOAP_FMAC2 soap_instantiate_ns__mul(struct soap*, int, const char*, const char*, size_t*);
SOAP_FMAC3 void SOAP_FMAC4 soap_copy_ns__mul(struct soap*, int, int, void*, size_t, const void*, size_t);

#ifndef SOAP_TYPE_calc_ns__mulResponse
#define SOAP_TYPE_calc_ns__mulResponse (16)
#endif
SOAP_FMAC3 void SOAP_FMAC4 soap_default_ns__mulResponse(struct soap*, struct ns__mulResponse *);
SOAP_FMAC3 void SOAP_FMAC4 soap_serialize_ns__mulResponse(struct soap*, const struct ns__mulResponse *);
SOAP_FMAC3 int SOAP_FMAC4 soap_out_ns__mulResponse(struct soap*, const char*, int, const struct ns__mulResponse *, const char*);
SOAP_FMAC3 struct ns__mulResponse * SOAP_FMAC4 soap_in_ns__mulResponse(struct soap*, const char*, struct ns__mulResponse *, const char*);

#ifndef soap_write_ns__mulResponse
#define soap_write_ns__mulResponse(soap, data) ( soap_begin_send(soap) || (calc::soap_serialize_ns__mulResponse(soap, data), 0) || calc::soap_put_ns__mulResponse(soap, data, "ns:mulResponse", NULL) || soap_end_send(soap) )
#endif


SOAP_FMAC3 int SOAP_FMAC4 soap_put_ns__mulResponse(struct soap*, const struct ns__mulResponse *, const char*, const char*);

#ifndef soap_read_ns__mulResponse
#define soap_read_ns__mulResponse(soap, data) ( soap_begin_recv(soap) || !calc::soap_get_ns__mulResponse(soap, data, NULL, NULL) || soap_end_recv(soap) )
#endif

SOAP_FMAC3 struct ns__mulResponse * SOAP_FMAC4 soap_get_ns__mulResponse(struct soap*, struct ns__mulResponse *, const char*, const char*);

#define soap_new_ns__mulResponse(soap, n) soap_instantiate_ns__mulResponse(soap, n, NULL, NULL, NULL)


#define soap_delete_ns__mulResponse(soap, p) soap_delete(soap, p)

SOAP_FMAC1 struct ns__mulResponse * SOAP_FMAC2 soap_instantiate_ns__mulResponse(struct soap*, int, const char*, const char*, size_t*);
SOAP_FMAC3 void SOAP_FMAC4 soap_copy_ns__mulResponse(struct soap*, int, int, void*, size_t, const void*, size_t);

#ifndef SOAP_TYPE_calc_ns__sub
#define SOAP_TYPE_calc_ns__sub (14)
#endif
SOAP_FMAC3 void SOAP_FMAC4 soap_default_ns__sub(struct soap*, struct ns__sub *);
SOAP_FMAC3 void SOAP_FMAC4 soap_serialize_ns__sub(struct soap*, const struct ns__sub *);
SOAP_FMAC3 int SOAP_FMAC4 soap_out_ns__sub(struct soap*, const char*, int, const struct ns__sub *, const char*);
SOAP_FMAC3 struct ns__sub * SOAP_FMAC4 soap_in_ns__sub(struct soap*, const char*, struct ns__sub *, const char*);

#ifndef soap_write_ns__sub
#define soap_write_ns__sub(soap, data) ( soap_begin_send(soap) || (calc::soap_serialize_ns__sub(soap, data), 0) || calc::soap_put_ns__sub(soap, data, "ns:sub", NULL) || soap_end_send(soap) )
#endif


SOAP_FMAC3 int SOAP_FMAC4 soap_put_ns__sub(struct soap*, const struct ns__sub *, const char*, const char*);

#ifndef soap_read_ns__sub
#define soap_read_ns__sub(soap, data) ( soap_begin_recv(soap) || !calc::soap_get_ns__sub(soap, data, NULL, NULL) || soap_end_recv(soap) )
#endif

SOAP_FMAC3 struct ns__sub * SOAP_FMAC4 soap_get_ns__sub(struct soap*, struct ns__sub *, const char*, const char*);

#define soap_new_ns__sub(soap, n) soap_instantiate_ns__sub(soap, n, NULL, NULL, NULL)


#define soap_delete_ns__sub(soap, p) soap_delete(soap, p)

SOAP_FMAC1 struct ns__sub * SOAP_FMAC2 soap_instantiate_ns__sub(struct soap*, int, const char*, const char*, size_t*);
SOAP_FMAC3 void SOAP_FMAC4 soap_copy_ns__sub(struct soap*, int, int, void*, size_t, const void*, size_t);

#ifndef SOAP_TYPE_calc_ns__subResponse
#define SOAP_TYPE_calc_ns__subResponse (13)
#endif
SOAP_FMAC3 void SOAP_FMAC4 soap_default_ns__subResponse(struct soap*, struct ns__subResponse *);
SOAP_FMAC3 void SOAP_FMAC4 soap_serialize_ns__subResponse(struct soap*, const struct ns__subResponse *);
SOAP_FMAC3 int SOAP_FMAC4 soap_out_ns__subResponse(struct soap*, const char*, int, const struct ns__subResponse *, const char*);
SOAP_FMAC3 struct ns__subResponse * SOAP_FMAC4 soap_in_ns__subResponse(struct soap*, const char*, struct ns__subResponse *, const char*);

#ifndef soap_write_ns__subResponse
#define soap_write_ns__subResponse(soap, data) ( soap_begin_send(soap) || (calc::soap_serialize_ns__subResponse(soap, data), 0) || calc::soap_put_ns__subResponse(soap, data, "ns:subResponse", NULL) || soap_end_send(soap) )
#endif


SOAP_FMAC3 int SOAP_FMAC4 soap_put_ns__subResponse(struct soap*, const struct ns__subResponse *, const char*, const char*);

#ifndef soap_read_ns__subResponse
#define soap_read_ns__subResponse(soap, data) ( soap_begin_recv(soap) || !calc::soap_get_ns__subResponse(soap, data, NULL, NULL) || soap_end_recv(soap) )
#endif

SOAP_FMAC3 struct ns__subResponse * SOAP_FMAC4 soap_get_ns__subResponse(struct soap*, struct ns__subResponse *, const char*, const char*);

#define soap_new_ns__subResponse(soap, n) soap_instantiate_ns__subResponse(soap, n, NULL, NULL, NULL)


#define soap_delete_ns__subResponse(soap, p) soap_delete(soap, p)

SOAP_FMAC1 struct ns__subResponse * SOAP_FMAC2 soap_instantiate_ns__subResponse(struct soap*, int, const char*, const char*, size_t*);
SOAP_FMAC3 void SOAP_FMAC4 soap_copy_ns__subResponse(struct soap*, int, int, void*, size_t, const void*, size_t);

#ifndef SOAP_TYPE_calc_ns__add
#define SOAP_TYPE_calc_ns__add (11)
#endif
SOAP_FMAC3 void SOAP_FMAC4 soap_default_ns__add(struct soap*, struct ns__add *);
SOAP_FMAC3 void SOAP_FMAC4 soap_serialize_ns__add(struct soap*, const struct ns__add *);
SOAP_FMAC3 int SOAP_FMAC4 soap_out_ns__add(struct soap*, const char*, int, const struct ns__add *, const char*);
SOAP_FMAC3 struct ns__add * SOAP_FMAC4 soap_in_ns__add(struct soap*, const char*, struct ns__add *, const char*);

#ifndef soap_write_ns__add
#define soap_write_ns__add(soap, data) ( soap_begin_send(soap) || (calc::soap_serialize_ns__add(soap, data), 0) || calc::soap_put_ns__add(soap, data, "ns:add", NULL) || soap_end_send(soap) )
#endif


SOAP_FMAC3 int SOAP_FMAC4 soap_put_ns__add(struct soap*, const struct ns__add *, const char*, const char*);

#ifndef soap_read_ns__add
#define soap_read_ns__add(soap, data) ( soap_begin_recv(soap) || !calc::soap_get_ns__add(soap, data, NULL, NULL) || soap_end_recv(soap) )
#endif

SOAP_FMAC3 struct ns__add * SOAP_FMAC4 soap_get_ns__add(struct soap*, struct ns__add *, const char*, const char*);

#define soap_new_ns__add(soap, n) soap_instantiate_ns__add(soap, n, NULL, NULL, NULL)


#define soap_delete_ns__add(soap, p) soap_delete(soap, p)

SOAP_FMAC1 struct ns__add * SOAP_FMAC2 soap_instantiate_ns__add(struct soap*, int, const char*, const char*, size_t*);
SOAP_FMAC3 void SOAP_FMAC4 soap_copy_ns__add(struct soap*, int, int, void*, size_t, const void*, size_t);

#ifndef SOAP_TYPE_calc_ns__addResponse
#define SOAP_TYPE_calc_ns__addResponse (10)
#endif
SOAP_FMAC3 void SOAP_FMAC4 soap_default_ns__addResponse(struct soap*, struct ns__addResponse *);
SOAP_FMAC3 void SOAP_FMAC4 soap_serialize_ns__addResponse(struct soap*, const struct ns__addResponse *);
SOAP_FMAC3 int SOAP_FMAC4 soap_out_ns__addResponse(struct soap*, const char*, int, const struct ns__addResponse *, const char*);
SOAP_FMAC3 struct ns__addResponse * SOAP_FMAC4 soap_in_ns__addResponse(struct soap*, const char*, struct ns__addResponse *, const char*);

#ifndef soap_write_ns__addResponse
#define soap_write_ns__addResponse(soap, data) ( soap_begin_send(soap) || (calc::soap_serialize_ns__addResponse(soap, data), 0) || calc::soap_put_ns__addResponse(soap, data, "ns:addResponse", NULL) || soap_end_send(soap) )
#endif


SOAP_FMAC3 int SOAP_FMAC4 soap_put_ns__addResponse(struct soap*, const struct ns__addResponse *, const char*, const char*);

#ifndef soap_read_ns__addResponse
#define soap_read_ns__addResponse(soap, data) ( soap_begin_recv(soap) || !calc::soap_get_ns__addResponse(soap, data, NULL, NULL) || soap_end_recv(soap) )
#endif

SOAP_FMAC3 struct ns__addResponse * SOAP_FMAC4 soap_get_ns__addResponse(struct soap*, struct ns__addResponse *, const char*, const char*);

#define soap_new_ns__addResponse(soap, n) soap_instantiate_ns__addResponse(soap, n, NULL, NULL, NULL)


#define soap_delete_ns__addResponse(soap, p) soap_delete(soap, p)

SOAP_FMAC1 struct ns__addResponse * SOAP_FMAC2 soap_instantiate_ns__addResponse(struct soap*, int, const char*, const char*, size_t*);
SOAP_FMAC3 void SOAP_FMAC4 soap_copy_ns__addResponse(struct soap*, int, int, void*, size_t, const void*, size_t);

#ifndef WITH_NOGLOBAL

#ifndef SOAP_TYPE_calc_PointerToSOAP_ENV__Reason
#define SOAP_TYPE_calc_PointerToSOAP_ENV__Reason (33)
#endif
SOAP_FMAC3 void SOAP_FMAC4 soap_serialize_PointerToSOAP_ENV__Reason(struct soap*, struct SOAP_ENV__Reason *const*);
SOAP_FMAC3 int SOAP_FMAC4 soap_out_PointerToSOAP_ENV__Reason(struct soap*, const char *, int, struct SOAP_ENV__Reason *const*, const char *);
SOAP_FMAC3 struct SOAP_ENV__Reason ** SOAP_FMAC4 soap_in_PointerToSOAP_ENV__Reason(struct soap*, const char*, struct SOAP_ENV__Reason **, const char*);

#ifndef soap_write_PointerToSOAP_ENV__Reason
#define soap_write_PointerToSOAP_ENV__Reason(soap, data) ( soap_begin_send(soap) || (calc::soap_serialize_PointerToSOAP_ENV__Reason(soap, data), 0) || calc::soap_put_PointerToSOAP_ENV__Reason(soap, data, "SOAP-ENV:Reason", NULL) || soap_end_send(soap) )
#endif

SOAP_FMAC3 int SOAP_FMAC4 soap_put_PointerToSOAP_ENV__Reason(struct soap*, struct SOAP_ENV__Reason *const*, const char*, const char*);

#ifndef soap_read_PointerToSOAP_ENV__Reason
#define soap_read_PointerToSOAP_ENV__Reason(soap, data) ( soap_begin_recv(soap) || !calc::soap_get_PointerToSOAP_ENV__Reason(soap, data, NULL, NULL) || soap_end_recv(soap) )
#endif

SOAP_FMAC3 struct SOAP_ENV__Reason ** SOAP_FMAC4 soap_get_PointerToSOAP_ENV__Reason(struct soap*, struct SOAP_ENV__Reason **, const char*, const char*);

#endif

#ifndef WITH_NOGLOBAL

#ifndef SOAP_TYPE_calc_PointerToSOAP_ENV__Detail
#define SOAP_TYPE_calc_PointerToSOAP_ENV__Detail (32)
#endif
SOAP_FMAC3 void SOAP_FMAC4 soap_serialize_PointerToSOAP_ENV__Detail(struct soap*, struct SOAP_ENV__Detail *const*);
SOAP_FMAC3 int SOAP_FMAC4 soap_out_PointerToSOAP_ENV__Detail(struct soap*, const char *, int, struct SOAP_ENV__Detail *const*, const char *);
SOAP_FMAC3 struct SOAP_ENV__Detail ** SOAP_FMAC4 soap_in_PointerToSOAP_ENV__Detail(struct soap*, const char*, struct SOAP_ENV__Detail **, const char*);

#ifndef soap_write_PointerToSOAP_ENV__Detail
#define soap_write_PointerToSOAP_ENV__Detail(soap, data) ( soap_begin_send(soap) || (calc::soap_serialize_PointerToSOAP_ENV__Detail(soap, data), 0) || calc::soap_put_PointerToSOAP_ENV__Detail(soap, data, "SOAP-ENV:Detail", NULL) || soap_end_send(soap) )
#endif

SOAP_FMAC3 int SOAP_FMAC4 soap_put_PointerToSOAP_ENV__Detail(struct soap*, struct SOAP_ENV__Detail *const*, const char*, const char*);

#ifndef soap_read_PointerToSOAP_ENV__Detail
#define soap_read_PointerToSOAP_ENV__Detail(soap, data) ( soap_begin_recv(soap) || !calc::soap_get_PointerToSOAP_ENV__Detail(soap, data, NULL, NULL) || soap_end_recv(soap) )
#endif

SOAP_FMAC3 struct SOAP_ENV__Detail ** SOAP_FMAC4 soap_get_PointerToSOAP_ENV__Detail(struct soap*, struct SOAP_ENV__Detail **, const char*, const char*);

#endif

#ifndef WITH_NOGLOBAL

#ifndef SOAP_TYPE_calc_PointerToSOAP_ENV__Code
#define SOAP_TYPE_calc_PointerToSOAP_ENV__Code (26)
#endif
SOAP_FMAC3 void SOAP_FMAC4 soap_serialize_PointerToSOAP_ENV__Code(struct soap*, struct SOAP_ENV__Code *const*);
SOAP_FMAC3 int SOAP_FMAC4 soap_out_PointerToSOAP_ENV__Code(struct soap*, const char *, int, struct SOAP_ENV__Code *const*, const char *);
SOAP_FMAC3 struct SOAP_ENV__Code ** SOAP_FMAC4 soap_in_PointerToSOAP_ENV__Code(struct soap*, const char*, struct SOAP_ENV__Code **, const char*);

#ifndef soap_write_PointerToSOAP_ENV__Code
#define soap_write_PointerToSOAP_ENV__Code(soap, data) ( soap_begin_send(soap) || (calc::soap_serialize_PointerToSOAP_ENV__Code(soap, data), 0) || calc::soap_put_PointerToSOAP_ENV__Code(soap, data, "SOAP-ENV:Code", NULL) || soap_end_send(soap) )
#endif

SOAP_FMAC3 int SOAP_FMAC4 soap_put_PointerToSOAP_ENV__Code(struct soap*, struct SOAP_ENV__Code *const*, const char*, const char*);

#ifndef soap_read_PointerToSOAP_ENV__Code
#define soap_read_PointerToSOAP_ENV__Code(soap, data) ( soap_begin_recv(soap) || !calc::soap_get_PointerToSOAP_ENV__Code(soap, data, NULL, NULL) || soap_end_recv(soap) )
#endif

SOAP_FMAC3 struct SOAP_ENV__Code ** SOAP_FMAC4 soap_get_PointerToSOAP_ENV__Code(struct soap*, struct SOAP_ENV__Code **, const char*, const char*);

#endif

#ifndef SOAP_TYPE_calc_PointerTodouble
#define SOAP_TYPE_calc_PointerTodouble (8)
#endif
SOAP_FMAC3 void SOAP_FMAC4 soap_serialize_PointerTodouble(struct soap*, double *const*);
SOAP_FMAC3 int SOAP_FMAC4 soap_out_PointerTodouble(struct soap*, const char *, int, double *const*, const char *);
SOAP_FMAC3 double ** SOAP_FMAC4 soap_in_PointerTodouble(struct soap*, const char*, double **, const char*);

#ifndef soap_write_PointerTodouble
#define soap_write_PointerTodouble(soap, data) ( soap_begin_send(soap) || (calc::soap_serialize_PointerTodouble(soap, data), 0) || calc::soap_put_PointerTodouble(soap, data, "double", NULL) || soap_end_send(soap) )
#endif

SOAP_FMAC3 int SOAP_FMAC4 soap_put_PointerTodouble(struct soap*, double *const*, const char*, const char*);

#ifndef soap_read_PointerTodouble
#define soap_read_PointerTodouble(soap, data) ( soap_begin_recv(soap) || !calc::soap_get_PointerTodouble(soap, data, NULL, NULL) || soap_end_recv(soap) )
#endif

SOAP_FMAC3 double ** SOAP_FMAC4 soap_get_PointerTodouble(struct soap*, double **, const char*, const char*);

#ifndef SOAP_TYPE_calc__QName
#define SOAP_TYPE_calc__QName (5)
#endif

#define soap_default__QName(soap, a) soap_default_string(soap, a)


#define soap_serialize__QName(soap, a) soap_serialize_string(soap, a)

SOAP_FMAC3 int SOAP_FMAC4 soap_out__QName(struct soap*, const char*, int, char*const*, const char*);
SOAP_FMAC3 char * * SOAP_FMAC4 soap_in__QName(struct soap*, const char*, char **, const char*);

#ifndef soap_write__QName
#define soap_write__QName(soap, data) ( soap_begin_send(soap) || (calc::soap_serialize__QName(soap, data), 0) || calc::soap_put__QName(soap, data, "byte", NULL) || soap_end_send(soap) )
#endif

SOAP_FMAC3 int SOAP_FMAC4 soap_put__QName(struct soap*, char *const*, const char*, const char*);

#ifndef soap_read__QName
#define soap_read__QName(soap, data) ( soap_begin_recv(soap) || !calc::soap_get__QName(soap, data, NULL, NULL) || soap_end_recv(soap) )
#endif

SOAP_FMAC3 char ** SOAP_FMAC4 soap_get__QName(struct soap*, char **, const char*, const char*);

#ifndef SOAP_TYPE_calc_string
#define SOAP_TYPE_calc_string (4)
#endif
SOAP_FMAC3 void SOAP_FMAC4 soap_default_string(struct soap*, char **);
SOAP_FMAC3 void SOAP_FMAC4 soap_serialize_string(struct soap*, char *const*);
SOAP_FMAC3 int SOAP_FMAC4 soap_out_string(struct soap*, const char*, int, char*const*, const char*);
SOAP_FMAC3 char * * SOAP_FMAC4 soap_in_string(struct soap*, const char*, char **, const char*);

#ifndef soap_write_string
#define soap_write_string(soap, data) ( soap_begin_send(soap) || (calc::soap_serialize_string(soap, data), 0) || calc::soap_put_string(soap, data, "byte", NULL) || soap_end_send(soap) )
#endif

SOAP_FMAC3 int SOAP_FMAC4 soap_put_string(struct soap*, char *const*, const char*, const char*);

#ifndef soap_read_string
#define soap_read_string(soap, data) ( soap_begin_recv(soap) || !calc::soap_get_string(soap, data, NULL, NULL) || soap_end_recv(soap) )
#endif

SOAP_FMAC3 char ** SOAP_FMAC4 soap_get_string(struct soap*, char **, const char*, const char*);

} // namespace calc


#endif

/* End of calcH.h */
