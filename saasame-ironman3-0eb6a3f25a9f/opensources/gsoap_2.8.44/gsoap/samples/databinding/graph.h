/*
	graph.h

	Tree, digraph, and cyclic graph serialization example.

--------------------------------------------------------------------------------
gSOAP XML Web services tools
Copyright (C) 2000-2015, Robert van Engelen, Genivia, Inc. All Rights Reserved.
This software is released under one of the following two licenses:
Genivia's license for commercial use.
--------------------------------------------------------------------------------
Product and source code licensed by Genivia, Inc., contact@genivia.com
--------------------------------------------------------------------------------
*/

#import "stlvector.h"

// a gSOAP directive to produce a xmlns:g="urn:graph" namespace binding:
//gsoap g schema namespace: urn:graph

/** A recursive data type that is (invisibly) bound to the 'g' XML namespace */
class g:Graph
{
  public:
    std::vector<g:Graph*> edges; ///< public members are serializable
};
