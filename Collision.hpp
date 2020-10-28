#pragma once

/*
 * Library to manage collisions between primitives
 * based on: Real-time Collision Detection by Christer Ericson
 */

// example on how to use:
// glm::vec3 c1(0,0,0); glm::vec3 r1(1,1,1); 
// glm::vec3 c2(1,1,1); glm::vec3 r2(1,1,1); 
// Collision::AABB a(c1,r1); // box 2 by 2 by 2, centered at origin
// Collision::AABB b(c2,r2); // box 2 by 2 by 2, centered at (1,1,1)
// if (Collision::testCollision(a,b)) { std::cout << "a and b collide" << std::endl; } // should be true 


#include <glm/glm.hpp>
#include <cmath>        // std::abs

namespace Collision
{
	enum class PrimitiveType {
		AABB = 0,
		OBB
	};

	struct Primitive
	{
		Primitive(PrimitiveType _type) : type(_type) { }
		PrimitiveType type;
	};

	// axis aligned bounding box
	struct AABB : Primitive
	{
		AABB(glm::vec3 c_, glm::vec3 r_) : Primitive(PrimitiveType::AABB), c(c_), r(r_) {}
		glm::vec3 c; // center
		glm::vec3 r; // radius in x,y,z direction
	};

	bool testAABBAABB(const AABB& a, const AABB& b);

	bool testCollision(const Primitive& a, const Primitive& b);

	// test collision between two AABB boxes
	bool testAABBAABB(const AABB& a, const AABB& b)
	{
		//  Two AABBs only overlap if they overlap on all three axes
		if (std::abs(a.c.x - b.c.x) > a.r.x + b.r.x) { return false; }
		if (std::abs(a.c.y - b.c.y) > a.r.y + b.r.y) { return false; }
		if (std::abs(a.c.z - b.c.z) > a.r.z + b.r.z) { return false; }
		return true;
	}

	// general function, can add more primitive types here
	bool testCollision(const Primitive& a, const Primitive& b)
	{
		if (a.type == PrimitiveType::AABB && b.type == PrimitiveType::AABB)
		{
			return testAABBAABB(static_cast<const AABB&>(a), static_cast<const AABB&>(b));
		}
		return false;
	}
}



