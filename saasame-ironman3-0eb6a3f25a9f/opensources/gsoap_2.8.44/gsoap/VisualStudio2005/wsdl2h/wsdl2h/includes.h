/*
	includes.h

	Common project definitions

--------------------------------------------------------------------------------
gSOAP XML Web services tools
Copyright (C) 2000-2017, Robert van Engelen, Genivia Inc. All Rights Reserved.
This software is released under one of the following licenses:
Genivia's license for commercial use.
--------------------------------------------------------------------------------
Product and source code licensed by Genivia, Inc., contact@genivia.com
--------------------------------------------------------------------------------

*/

#ifndef INCLUDES_H
#define INCLUDES_H

#include "stdsoap2.h"

#ifdef WITH_OPENSSL
#include "httpda.h"
#endif

#define WSDL2H_VERSION "2.8.44"

#ifdef WIN32
# pragma warning(disable : 4996)
#endif

#include <utility>
#include <iterator>
#include <vector>
#include <set>
#include <map>

using namespace std;

struct ltstr
{ bool operator()(const char *s1, const char *s2) const
  { return strcmp(s1, s2) < 0;
  }
}; 

struct eqstr
{ const char *s;
  eqstr(const char *s) : s(s) { }
  bool operator()(const char *t) const
  { return strcmp(s, t) == 0;
  }
}; 

typedef set<const char*, ltstr> SetOfString;

typedef pair<const char*, const char*> Pair;

struct ltpair
{ bool operator()(Pair s1, Pair s2) const
  { int cmp = strcmp(s1.first, s2.first);
    if (cmp == 0)
      cmp = strcmp(s1.second, s2.second);
    return cmp < 0;
  }
};

typedef map<const char*, const char*, ltstr> MapOfStringToString;

typedef map<Pair, const char*, ltpair> MapOfPairToString;

typedef map<const char*, size_t, ltstr> MapOfStringToNum;

typedef vector<const char*> VectorOfString;

extern int _flag,
           aflag,
           bflag,
	   cflag,
	   c11flag,
	   dflag,
	   eflag,
	   fflag,
	   gflag,
	   iflag,
	   jflag,
	   kflag,
	   mflag,
	   Mflag,
	   pflag,
	   Pflag,
	   Rflag,
	   sflag,
	   Uflag,
	   uflag,
	   vflag,
	   wflag,
	   Wflag,
	   xflag,
	   yflag,
	   zflag;

extern FILE *stream;

extern SetOfString exturis;

#define MAXINFILES (1000)

extern int openfiles;
extern int infiles;
extern char *infile[MAXINFILES], *outfile, *proxy_host, *proxy_userid, *proxy_passwd, *auth_userid, *auth_passwd;
extern const char *mapfile, *import_path, *cwd_path, *cppnamespace;

extern int proxy_port;

extern const char *service_prefix;
extern const char *schema_prefix;

extern const char elementformat[];
extern const char pointerformat[];
extern const char attributeformat[];
extern const char templateformat_open[];
extern const char attrtemplateformat_open[];
extern const char templateformat[];
extern const char pointertemplateformat[];
extern const char arrayformat[];
extern const char arraysizeformat[];
extern const char arrayoffsetformat[];
extern const char sizeformat[];
extern const char choiceformat[];
extern const char schemaformat[];
extern const char serviceformat[];
extern const char paraformat[];
extern const char anonformat[];
extern const char sizeparaformat[];
extern const char pointertemplateparaformat[];

extern const char copyrightnotice[];
extern const char licensenotice[];

extern void *emalloc(size_t size);
extern char *estrdup(const char *s);
extern char *estrdupf(const char *s);

extern void text(const char*);

class Types;
class Message;
class Operation;
class Service;
class Definitions;

#endif
