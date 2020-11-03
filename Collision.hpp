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
		AABB();
		AABB(glm::vec3 c_, glm::vec3 r_) : Primitive(PrimitiveType::AABB), c(c_), r(r_) {}
		glm::vec3 c; // center
		glm::vec3 r; // radius in x,y,z direction
	};

	bool testAABBAABB(const AABB& a, const AABB& b);

	// this function only checks for overlapping in X and Y direction
	bool testAABBAABBXY(const AABB& a, const AABB& b);

	bool testCollision(const Primitive& a, const Primitive& b);

	bool testCollisionXY(const Primitive& a, const Primitive& b);
}



