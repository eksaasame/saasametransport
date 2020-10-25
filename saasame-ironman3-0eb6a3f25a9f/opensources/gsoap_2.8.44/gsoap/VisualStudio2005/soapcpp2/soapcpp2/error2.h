/*
	error2.h

	Error handling.

gSOAP XML Web services tools
Copyright (C) 2000-2008, Robert van Engelen, Genivia Inc. All Rights Reserved.
This part of the software is released under one of the following licenses:
Genivia's license for commercial use.
--------------------------------------------------------------------------------
Product and source code licensed by Genivia, Inc., contact@genivia.com
--------------------------------------------------------------------------------
*/

extern char errbuf[];

#ifdef WIN32_WITHOUT_SOLARIS_FLEX
extern void soapcpp2error(const char*);
#else
extern void yyerror(const char*);
#endif

extern void lexerror(const char*);
extern void synerror(const char *);
extern void semerror(const char *);
extern void semwarn(const char *);
extern void compliancewarn(const char *);
extern void typerror(const char*);
extern void execerror(const char*);
extern void progerror(const char*, const char*, int);
extern int errstat(void);
