#include "Collision.hpp"


// test collision between two AABB boxes
bool Collision::testAABBAABB(const AABB& a, const AABB& b)
{
	//  Two AABBs only overlap if they overlap on all three axes
	if (std::abs(a.c.x - b.c.x) > a.r.x + b.r.x) { return false; }
	if (std::abs(a.c.y - b.c.y) > a.r.y + b.r.y) { return false; }
	if (std::abs(a.c.z - b.c.z) > a.r.z + b.r.z) { return false; }
	return true;
}

// general function, can add more primitive types here
bool Collision::testCollision(const Primitive& a, const Primitive& b)
{
	if (a.type == PrimitiveType::AABB && b.type == PrimitiveType::AABB)
	{
		return testAABBAABB(static_cast<const AABB&>(a), static_cast<const AABB&>(b));
	}
	return false;
}
