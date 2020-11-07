#include "Collision.hpp"


// test collision between two AABB boxes
int Collision::testAABBAABB(const AABB& a, const AABB& b)
{
	//  Two AABBs only overlap if they overlap on all three axes
	if (std::abs(a.c.x - b.c.x) > a.r.x + b.r.x) { return false; }
	if (std::abs(a.c.y - b.c.y) > a.r.y + b.r.y) { return false; }
	if (std::abs(a.c.z - b.c.z) > a.r.z + b.r.z) { return false; }

	// ASSUMPTION: b is the player (a is usually wider)
	// did the player collide more on the x side or the y side?
	// if b.c.x is contained between sides x1 x2 of a -> collision in y
	// if b.c.y is contained between sides y1 y2 of a -> collision in x
	if (b.c.x > a.c.x - a.r.x && b.c.x < a.c.x + a.r.x)
	{
		return 2; // collision in y
	}
	else if (b.c.y > a.c.y - a.r.y && b.c.y < a.c.y + a.r.y)
	{
		return 1;
	}
	else
	{
		return 3;
	}
	
	

	return true;
}

bool Collision::testAABBAABBXY(const AABB& a, const AABB& b)
{
	if (std::abs(a.c.x - b.c.x) > a.r.x + b.r.x + 0.2f) { return false; }
	if (std::abs(a.c.y - b.c.y) > a.r.y + b.r.y + 0.2f) { return false; }
	return true;
}

// general function, can add more primitive types here
int Collision::testCollision(const Primitive& a, const Primitive& b)
{
	if (a.type == PrimitiveType::AABB && b.type == PrimitiveType::AABB)
	{
		return testAABBAABB(static_cast<const AABB&>(a), static_cast<const AABB&>(b));
	}
	return 0;
}

bool Collision::testCollisionXY(const Primitive& a, const Primitive& b)
{
	if (a.type == PrimitiveType::AABB && b.type == PrimitiveType::AABB)
	{
		return testAABBAABBXY(static_cast<const AABB&>(a), static_cast<const AABB&>(b));
	}
	return false;	
}