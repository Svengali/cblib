#include "ConvexHullTree.h"
#include "ConvexHullBuilder2d.h"

START_CB

//---------------------------------------------------------------------------------------

int64 Area(const vector<Vec2i> & verts,const vector<int> & polygon)
{
	int64 ret = 0;
	for(int i=2;i<polygon.size();i++)
	{
		int64 a = Area( verts[ polygon[0] ], verts[ polygon[ i-1] ], verts[ polygon[i] ] );
		ret += a;
	}
	return ret;
}

RectI Bound(const vector<Vec2i> & verts,const vector<int> & polygon)
{
	RectI ret( verts[ polygon[0] ].x, verts[polygon[0]].y );
	for(int i=1;i<polygon.size();i++)
	{
		ret.ExtendToPoint( verts[ polygon[i] ].x , verts[ polygon[i] ].y );
	}
	return ret;
}

bool Contains( const vector<Vec2i> & verts, const vector<int> & poly, const Vec2i & v )
{
	// does poly contain v ?
	// do "ray-shooting"
	//	(poly is convex)
	// "on" is counted as "in"

	bool inY = false;

	int s = poly.size();
	for(int i=0;i<s;i++)
	{
		int n = i+1;
		if ( n == s ) n = 0;
		const Vec2i & A = verts[poly[i]];
		const Vec2i & B = verts[poly[n]];

		// am I in the Y range of the segment ?
		int loY = MIN(A.y,B.y);
		int hiY = MAX(A.y,B.y);

		if ( v.y < loY || v.y > hiY )
			continue;

		inY = true;

		// special case horizontals
		if ( loY == hiY )
		{
			if ( v.y == loY )
			{
				if ( v.x >= MIN(A.x,B.x) && v.x <= MAX(A.x,B.x) )
					return true;
			}

			continue;
		}

		int64 a = Area(A,B,v);
		if ( a == 0 )
			return true; // I'm on the segment, count that as contains

		if ( a < 0 )
			return false; // I'm outside
	}

	return inY;
}

bool Contains( const vector<Vec2i> & verts, const vector<int> & p1, const vector<int> & p2 )
{
	// does p1 contain p2 ?
	// just check if any verts of p2 are not in p1
	// "on" is counted as "in"

	for(int i=0;i<p2.size();i++)
	{
		if ( ! Contains(verts,p1, verts[p2[i]]) )
			return false;
	}

	return true;
}

void MergeConvex( vector<int> * pTo, const vector<Vec2i> & verts, const vector<int> & p1, const vector<int> & p2 )
{
	//@@@@ TODO : there should be fast way to do this, right?

	vector<Vec2i> v;
	{for(int i=0;i<p1.size();i++)
	{
		v.push_back( verts[p1[i]] );
	}}
	{for(int i=0;i<p2.size();i++)
	{
		v.push_back( verts[p2[i]] );
	}}

	vector<Vec2i> hull;
	ConvexHullBuilder2d::Make2d(v.data(),v.size(), hull);

	pTo->resize(hull.size());

	// re-index it :
	{for(int i=0;i<hull.size();i++)
	{
		int index = verts.find( hull[i] ) - verts.begin();
		ASSERT( index != verts.size() );
		pTo->at(i) = index;
	}}
}

//---------------------------------------------------------------------------------------

namespace ConvexHullTree
{

double RateMerge(const vector<Vec2i> & verts,const Node * p1,const Node * p2, double)
{
	// rate based on :
	//	1) area of result (smaller is better)
	//	2) compare area of result to sum of two area;
	//		area of the merged <= sum of the two is good

	vector<int> merged;
	MergeConvex(&merged, verts,p1->outline,p2->outline);

	int64 area_merged = Area(verts,merged);

	// try this :
	//	rating = Sum[kids]/Area[parent]
	// if the kids overlap, this will be > 1 , otherwise it's < 1
	// for the same kids, the closer they are together the better the rating
	//
	// another option would be
	//	rating = Sum[kids] - Area[parent]

	//double rating = (double(p1->area) + double(p2->area)) / double(area_merged);
	
	double rating = double( p1->area + p2->area - area_merged );

	return rating;
}

void Delete(Node * pNode)
{
	ASSERT( pNode != NULL );
	if ( pNode->children )
		Delete(pNode->children);
	if ( pNode->sibling )
		Delete(pNode->sibling);
	delete pNode;
}

Node * Build(const vector<Vec2i> & verts,vector< vector<int> > & polygons)
{	
	// make all the leaves :
	// I assume all the leaves don't overlap each over at all

	vector<Node *>	nodes;
	nodes.resize( polygons.size() );

	for( int i=0;i<polygons.size();i++ )
	{
		nodes[i] = new Node;
		nodes[i]->outline.assignv( polygons[i] );
		nodes[i]->area = Area(verts,polygons[i]);
		nodes[i]->bound = Bound(verts,polygons[i]);
		nodes[i]->isLeaf = true;
	}

	// now do a recursive build :

	while ( nodes.size() > 1 )
	{
		// pick something to put under something else

		// first see if any node contains another one :
		//	(this will keep going while any node contains any other one)

		{for(int i=0;i<nodes.size();i++)
		{
			for(int j=0;j<nodes.size();j++)
			{
				// check for i contains j
				if ( nodes[i]->area <= nodes[j]->area )
					continue; // must have bigger area to contain !!
				if ( ! nodes[i]->bound.Contains( nodes[j]->bound ) )
					continue; // must contain the bound !!

				if ( Contains(verts, nodes[i]->outline, nodes[j]->outline ) )
				{
					// yup, it contains
					// just make it a child of i
					// both i and j may already have kids,
					// but neither has a parent or siblings
					ASSERT( nodes[i]->parent == NULL );
					ASSERT( nodes[j]->parent == NULL );
					ASSERT( nodes[i]->sibling == NULL );
					ASSERT( nodes[j]->sibling == NULL );
					nodes[j]->sibling = nodes[i]->children;
					nodes[i]->children = nodes[j];
					nodes[j]->parent = nodes[i];
					
					nodes.erase(nodes.begin()+j);
					if ( j < i )
						i--; // did an erase, so go back

					i--; // don't step i
					break; // cancel j loop, continue i loop
				}
			}
		}}

		// now try to pick two nodes to make another node on top of

		double bestRating = -FLT_MAX;
		int bestI=-1,bestJ=-1;
		{for(int i=0;i<nodes.size();i++)
		{
			for(int j=i+1;j<nodes.size();j++)
			{
				double rating = RateMerge(verts, nodes[i],nodes[j], bestRating);
				if ( rating > bestRating )
				{
					bestRating = rating;
					bestI = i;
					bestJ = j;
				}
			}
		}}

		// make the new node and put it on top :
		ASSERT( bestI != -1 && bestJ != -1 );
		Node * p1 = nodes[bestI];
		Node * p2 = nodes[bestJ];
		// must to the larger one first :
		ASSERT( bestJ > bestI );
		nodes.erase_u(bestJ);
		nodes.erase_u(bestI);

		Node * pNew = new Node;
		nodes.push_back(pNew);

		ASSERT( p1->parent == NULL );
		ASSERT( p2->parent == NULL );
		ASSERT( p1->sibling == NULL );
		ASSERT( p2->sibling == NULL );

		pNew->children = p1;
		p1->sibling = p2;
		p1->parent = pNew;
		p2->parent = pNew;

		MergeConvex( &(pNew->outline), verts, p1->outline, p2->outline );
		pNew->bound.SetEnclosing( p1->bound, p2->bound );
		pNew->area = Area(verts,pNew->outline);
	}

	ASSERT( nodes.size() == 1 );
	Node * pRoot = nodes[0];
	return pRoot;
}

const Node * Descend(const Node * pRoot,const vector<Vec2i> & verts,const Vec2i & query)
{
	vector<const Node *> stack;

	stack.push_back(pRoot);

	while(!stack.empty())
	{
		const Node * pNode = stack.back();
		stack.pop_back();

		if ( ! Contains(verts,pNode->outline,query) )
		{
			continue;
		}

		if ( pNode->isLeaf )
		{
			return pNode;
		}

		if ( pNode->sibling )
			stack.push_back(pNode->sibling);
		if ( pNode->children )
			stack.push_back(pNode->children);
	}

	return NULL;
}


}; // gConvexHullTree

//---------------------------------------------------------------------------------------

END_CB
