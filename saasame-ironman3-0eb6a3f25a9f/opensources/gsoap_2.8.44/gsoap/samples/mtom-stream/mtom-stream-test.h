/*
	mtom-stream-test.h

	Example streaming MTOM client and server.

	Copyright (C) 2000-2006 Robert A. van Engelen, Genivia, Inc.
	All Rights Reserved.

	Usage (CGI server):

		Install as CGI application, e.g. under cgi-bin

	Usage (server):

	mtom-stream-test 8085 &

		Starts a server on your host at port 8085.

	Usage (client):

	mtom-stream-test -p file1 file2 file3 ...

		Stores files file1, file2, etc. at the server side. The server
		saves them locally under a key. The storage keys are printed at
		the client side. The keys provide access to the data using
		option -g (get).

	mtom-stream-test -g name1 name2 name3 ...

		Retrieves files stored under keys name1, name2, etc.
		The keys must correspond to the keys returned when storing
		files. Files are stored by the server locally under the key
		name.

	Unix/Linux: add a sigpipe handler to avoid broken pipes.

--------------------------------------------------------------------------------
gSOAP XML Web services tools
Copyright (C) 2001-2008, Robert van Engelen, Genivia, Inc. All Rights Reserved.
This software is released under one of the following two licenses:
Genivia's license for commercial use.
--------------------------------------------------------------------------------
Product and source code licensed by Genivia, Inc., contact@genivia.com
--------------------------------------------------------------------------------
*/

#import "soap12.h"
#import "xop.h"
#import "xmime5.h"

//gsoap m service name:		mtom_stream_test
//gsoap m service namespace:	http://www.genivia.com/wsdl/mtom_stream_test.wsdl

//gsoap x schema namespace:	http://www.genivia.com/schemas/mtom_stream_test.xsd
//gsoap x schema elementForm:	qualified

//gsoap x schema type-documentation: Data holds a MIME attachment
struct x__Data
{ _xop__Include xop__Include;
  @char *xmime5__contentType;
};

//gsoap x schema type-documentation: DataSet holds a set of MIME attachments
struct x__DataSet
{ int __size;
  struct x__Data *item;
};

//gsoap x schema type-documentation: Keys holds a set of strings to access the data collection at the server side
struct x__Keys
{ int __size;
  char **key;
};

//gsoap m service method-mime-type: PutData */*
//gsoap m service method-documentation: PutData stores a data set at the server side and returns a set of unique keys to access that data
int m__PutData(
  struct x__DataSet *x__data,
  struct m__PutDataResponse
  { struct x__Keys x__keys;
  }*
);

//gsoap m service method-mime-type: GetData */*
//gsoap m service method-documentation: GetData gets a set of data items from the server given a set of keys
int m__GetData(
  struct x__Keys *x__keys,
  struct m__GetDataResponse
  { struct x__DataSet x__data;
  }*
);
