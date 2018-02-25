#include "cblib/Rect.h"
#include <memory.h>

START_CB

//=================================================================================================

// template instantiation directive :

template RectTemplate<int>;
template RectTemplate<float>;


//=================================================================================================

bool RectI::operator == (const RectI & other) const
{
	return memcmp(this,&other,sizeof(RectI)) == 0;
}
	

bool RectF::IsValid() const
{
	ASSERT( fisvalid(Left()) );
	ASSERT( fisvalid(Right()) );
	ASSERT( fisvalid(Top()) );
	ASSERT( fisvalid(Bottom()) );
	ASSERT( parent_type::IsValid() );
	return true;
}

void RectF::SetScaled(const RectF & r,const float f)
{
	SetScaled(r,f,f);
}

void RectF::SetScaled(const RectF & r,const float xs,const float ys)
{
	const Vec2 center = r.GetCenter();
	const float w = r.Width();
	const float h = r.Height();
	
	Set(center.x - w*0.5f*xs , 
		center.x + w*0.5f*xs , 
		center.y - h*0.5f*ys , 
		center.y + h*0.5f*ys ); 
}
	
void RectF::GrowToSquare()
{
	float w = Width();
	float h = Height();

	if ( w < h )
	{
		// grow x
		float delta = (h-w)*0.5f;
		ASSERT( delta >= 0.f );
		SetX( LoX() - delta, HiX() + delta );
	}
	else
	{
		float delta = (w-h)*0.5f;
		ASSERT( delta >= 0.f );
		SetY( LoY() - delta, HiY() + delta );
	}

	// fequal(Width(), Height()) can sometimes fail if Width() &
	// Height() are very large, even though the dimensions are
	// relatively equal.
	ASSERT( (Height() < EPSILON && Width() < EPSILON)
			|| fequal(Width() / Height(), 1.0f, 0.05f) );
}

const Vec2 RectF::ScaleVectorIntoRect(const Vec2 & from) const
{
	ASSERT( Width() >= EPSILON );
	ASSERT( Height() >= EPSILON );
	return Vec2(
		(from.x - LoX()) / Width(),
		(from.y - LoY()) / Height()
	);
}

const Vec2 RectF::ScaleVectorFromRect(const Vec2 & from) const
{
	return Vec2(
		from.x * Width() + LoX(),
		from.y * Height() + LoY()
	);
}

/*static*/ const RectI RectI::zero(0,0,0,0);
/*static*/ const RectF RectF::zero(0.f,0.f,0.f,0.f);
/*static*/ const RectI RectI::unit(0,1,0,1);
/*static*/ const RectF RectF::unit(0.f,1.f,0.f,1.f);

END_CB
