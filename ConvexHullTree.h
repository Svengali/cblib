#pragma once

#include "cblib/Rect.h"
#include "cblib/Vec2.h"
#include "cblib/Vec2i.h"
#include "cblib/vector.h"

START_CB

//---------------------------------------------------------------------------------------

namespace ConvexHullTree
{
	struct Node
	{
		Node *			parent;
		Node *			children;
		Node *			sibling;
		bool			isLeaf;
		int64			area;
		RectI			bound;
		vector<int>	outline;
		
		Node() : parent(NULL), children(NULL), sibling(NULL), area(0), isLeaf(false), bound(0,0)
		{
		}
	};


	// Build makes the ConvexHullTree and returns the Root
	Node * Build(const vector<Vec2i> & verts,vector< vector<int> > & polygons);

	// Delete to get rid of the tree
	void Delete(Node * pNode);
	
	// find the Node if any that the query point is in
	//	returns NULL if none
	const Node * Descend(const Node * pRoot,const vector<Vec2i> & verts,const Vec2i & query);
};

//---------------------------------------------------------------------------------------
// Utils for indexed 2d polygons :

int64 Area(const vector<Vec2i> & verts,const vector<int> & polygon);
RectI Bound(const vector<Vec2i> & verts,const vector<int> & polygon);


bool Contains( const vector<Vec2i> & verts, const vector<int> & poly, const Vec2i & v );
bool Contains( const vector<Vec2i> & verts, const vector<int> & p1, const vector<int> & p2 );

//---------------------------------------------------------------------------------------

END_CB

