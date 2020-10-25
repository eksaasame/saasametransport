/*
        json.h
        
        JSON C/C++ supporting functions

--------------------------------------------------------------------------------
gSOAP XML Web services tools
Copyright (C) 2000-2015, Robert van Engelen, Genivia, Inc. All Rights Reserved.
This software is released under one of the following two licenses:
Genivia's license for commercial use.
--------------------------------------------------------------------------------
Product and source code licensed by Genivia, Inc., contact@genivia.com
--------------------------------------------------------------------------------
*/

#ifdef JSON_NAMESPACE
# include "jsonH.h"
#else
# include "soapH.h"
#endif

#ifdef JSON_NAMESPACE
namespace json {
#endif

/**
@brief Set value to JSON error property given the context's error code, as per Google JSON Style Guide
@param soap context with soap->error set
@param v value to set
@return error code
*/
extern int json_error(struct soap *soap, struct value *v);

/**
@brief Write JSON value to the context's output (socket, stream, FILE, or string)
@param soap context that manages IO
@param v value to write
@return SOAP_OK or error code
*/
extern int json_write(struct soap *soap, const struct value *v);

/**
@brief Send JSON value, requires soap_begin_send() before this call and soap_end_send() to finish, this function is used by json_write()
@param soap context that manages IO
@param v value to send
@return SOAP_OK or error code
*/
extern int json_send(struct soap *soap, const struct value *v);

#ifdef __cplusplus
extern int json_write(struct soap *soap, const value& v);
extern int json_send(struct soap *soap, const value& v);
extern std::ostream& operator<<(std::ostream&, const value&);
#endif

/**
@brief Read JSON value from context's input (socket, stream, FILE, or string)
@param soap context that manages IO
@param v value to read (non NULL)
@return SOAP_OK or error code
*/
extern int json_read(struct soap *soap, struct value *v);

/**
@brief Receive JSON value, requires soap_begin_recv() before this call and soap_end_recv() to finish, this function is used by json_read()
@param soap context that manages IO
@param v value to receive (non NULL)
@return SOAP_OK or error code
*/
extern int json_recv(struct soap *soap, struct value *v);

#ifdef __cplusplus
extern int json_read(struct soap *soap, value& v);
extern int json_recv(struct soap *soap, value& v);
extern std::istream& operator>>(std::istream&, value&);
#endif

/** Client-side JSON REST call to endpoint URL with optional in and out values (POST with in/out, GET with out, PUT with in, DELETE without in/out), returns SOAP_OK or HTTP code
@param soap context that manages IO
@param endpoint URL of the JSON REST/RPC service
@param in value to send, or NULL (when non-NULL: PUT or POST, when NULL: GET or DELETE)
@param out value to receive, or NULL (when non-NULL: GET or POST, when NULL: PUT or DELETE)
@return SOAP_OK or error code with out set to the JSON error property
*/
extern int json_call(struct soap *soap, const char *endpoint, const struct value *in, struct value *out);

#ifdef __cplusplus
extern int json_call(struct soap *soap, const char *endpoint, const struct value& in, struct value& out);
#endif

/**
@brief Convert string to JSON string and write it to context's output
@param soap context that manages IO
@param s string to send
@return SOAP_OK or error code
*/
extern int json_send_string(struct soap *soap, const char *s);

#ifdef __cplusplus
extern value json_add(const value&, const value&);
template<typename T> inline value operator+(const value& x, const T& y);
template<typename T> inline value operator+(const value& x, const T& y) { return json_add(x, value(x.soap, y)); }
template<> inline value operator+(const value& x, const value& y) { return json_add(x, y); }

extern value json_sub(const value&, const value&);
template<typename T> inline value operator-(const value& x, const T& y);
template<typename T> inline value operator-(const value& x, const T& y) { return json_sub(x, value(x.soap, y)); }
template<> inline value operator-(const value& x, const value& y) { return json_sub(x, y); }

extern value json_mul(const value&, const value&);
template<typename T> inline value operator*(const value& x, const T& y);
template<typename T> inline value operator*(const value& x, const T& y) { return json_mul(x, value(x.soap, y)); }
template<> inline value operator*(const value& x, const value& y) { return json_mul(x, y); }

extern value json_div(const value&, const value&);
template<typename T> inline value operator/(const value& x, const T& y);
template<typename T> inline value operator/(const value& x, const T& y) { return json_div(x, value(x.soap, y)); }
template<> inline value operator/(const value& x, const value& y) { return json_div(x, y); }

extern value json_mod(const value&, const value&);
template<typename T> inline value operator%(const value& x, const T& y);
template<typename T> inline value operator%(const value& x, const T& y) { return json_mod(x, value(x.soap, y)); }
template<> inline value operator%(const value& x, const value& y) { return json_mod(x, y); }

extern bool json_eqv(const value&, const value&);
template<typename T> inline bool operator==(const value& x, const T& y);
template<typename T> inline bool operator==(const value& x, const T& y) { return json_eqv(x, value(x.soap, y)); }
template<> inline bool operator==(const value& x, const value& y) { return json_eqv(x, y); }
template<typename T> inline bool operator!=(const value& x, const T& y);
template<typename T> inline bool operator!=(const value& x, const T& y) { return !json_eqv(x, value(x.soap, y)); }
template<> inline bool operator!=(const value& x, const value& y) { return !json_eqv(x, y); }

extern bool json_leq(const value&, const value&);
template<typename T> inline bool operator<=(const value& x, const T& y);
template<typename T> inline bool operator<=(const value& x, const T& y) { return json_leq(x, value(x.soap, y)); }
template<> inline bool operator<=(const value& x, const value& y) { return json_leq(x, y); }
template<typename T> inline bool operator>=(const value& x, const T& y);
template<typename T> inline bool operator>=(const value& x, const T& y) { return json_leq(value(x.soap, y), x); }
template<> inline bool operator>=(const value& x, const value& y) { return json_leq(y, x); }

extern bool json_lne(const value&, const value&);
template<typename T> inline bool operator<(const value& x, const T& y);
template<typename T> inline bool operator<(const value& x, const T& y) { return json_lne(x, value(x.soap, y)); }
template<> inline bool operator<(const value& x, const value& y) { return json_lne(x, y); }
template<typename T> inline bool operator>(const value& x, const T& y);
template<typename T> inline bool operator>(const value& x, const T& y) { return json_lne(value(x.soap, y), x); }
template<> inline bool operator>(const value& x, const value& y) { return json_lne(y, x); }
#endif

#ifdef JSON_NAMESPACE
} // namespace json
#endif