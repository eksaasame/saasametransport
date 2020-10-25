/*
	rss.h

	Supports RSS 0.91, 0.92 and 2.0

	Retrieve RSS feeds and formats the messages in HTML.

	Use CSS to format the HTML layout.

--------------------------------------------------------------------------------
gSOAP XML Web services tools
Copyright (C) 2001-2008, Robert van Engelen, Genivia, Inc. All Rights Reserved.
This software is released under one of the following two licenses:
Genivia's license for commercial use.
--------------------------------------------------------------------------------
Product and source code licensed by Genivia, Inc., contact@genivia.com
--------------------------------------------------------------------------------
*/

//gsoap dc schema namespace: http://purl.org/dc/elements/1.1/

/// Container for mixed XML content (XML gSOAP built-in)
typedef char *XML;

/// <channel> element
struct channel
{
  char *title;          ///< channel title element
  char *link;           ///< channel link element
  char *language;
  char *copyright;
  XML description;	///< description may contain XHTML that is preserved in XML
  struct image *image;  ///< optional image element
  int __size;           ///< an array of items of size __size
  struct item *item;    ///< an array of item elements
  time_t *dc__date;	///< RSS 2.0 dc schema element (optional)
};

/// <item> element
struct item
{
  char *title;		///< item title element
  char *link;		///< item link element
  XML description;	///< description may contain XHTML that is preserved in XML
  char *pubDate;
  time_t *dc__date;	///< RSS 2.0 dc schema element
};

/// <image> element
struct image
{
  char *title;		///< image title element
  char *url;		///< image URL element
  char *link;		///< image link element
  int width  0:1 = 0;	///< optional, default value = 0
  int height 0:1 = 0;	///< optional, default value = 0
  XML description;	///< description may contain XHTML that is preserved in XML
};

/// <rss> element
struct rss
{
  @char *version = "2.0";	///< version attribute (optional, default="2.0")
  struct channel channel;	///< channel element
};
