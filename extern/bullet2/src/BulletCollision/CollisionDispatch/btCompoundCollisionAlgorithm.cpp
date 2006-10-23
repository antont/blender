/*
Bullet Continuous Collision Detection and Physics Library
Copyright (c) 2003-2006 Erwin Coumans  http://continuousphysics.com/Bullet/

This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising from the use of this software.
Permission is granted to anyone to use this software for any purpose, 
including commercial applications, and to alter it and redistribute it freely, 
subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/

#include "BulletCollision/CollisionDispatch/btCompoundCollisionAlgorithm.h"
#include "BulletCollision/CollisionDispatch/btCollisionObject.h"
#include "BulletCollision/CollisionShapes/btCompoundShape.h"


btCompoundCollisionAlgorithm::btCompoundCollisionAlgorithm( const btCollisionAlgorithmConstructionInfo& ci,btCollisionObject* body0,btCollisionObject* body1,bool isSwapped)
:m_isSwapped(isSwapped)
{
	btCollisionObject* colObj = m_isSwapped? body1 : body0;
	btCollisionObject* otherObj = m_isSwapped? body0 : body1;
	assert (colObj->m_collisionShape->isCompound());
	
	btCompoundShape* compoundShape = static_cast<btCompoundShape*>(colObj->m_collisionShape);
	int numChildren = compoundShape->getNumChildShapes();
	int i;
	
	m_childCollisionAlgorithms.resize(numChildren);
	for (i=0;i<numChildren;i++)
	{
		btCollisionShape* childShape = compoundShape->getChildShape(i);
		btCollisionShape* orgShape = colObj->m_collisionShape;
		colObj->m_collisionShape = childShape;
		m_childCollisionAlgorithms[i] = ci.m_dispatcher->findAlgorithm(colObj,otherObj);
		colObj->m_collisionShape =orgShape;
	}
}


btCompoundCollisionAlgorithm::~btCompoundCollisionAlgorithm()
{
	int numChildren = m_childCollisionAlgorithms.size();
	int i;
	for (i=0;i<numChildren;i++)
	{
		delete m_childCollisionAlgorithms[i];
	}
}

void btCompoundCollisionAlgorithm::processCollision (btCollisionObject* body0,btCollisionObject* body1,const btDispatcherInfo& dispatchInfo,btManifoldResult* resultOut)
{
	btCollisionObject* colObj = m_isSwapped? body1 : body0;
	btCollisionObject* otherObj = m_isSwapped? body0 : body1;

	assert (colObj->m_collisionShape->isCompound());
	btCompoundShape* compoundShape = static_cast<btCompoundShape*>(colObj->m_collisionShape);

	//We will use the OptimizedBVH, AABB tree to cull potential child-overlaps
	//If both proxies are Compound, we will deal with that directly, by performing sequential/parallel tree traversals
	//given Proxy0 and Proxy1, if both have a tree, Tree0 and Tree1, this means:
	//determine overlapping nodes of Proxy1 using Proxy0 AABB against Tree1
	//then use each overlapping node AABB against Tree0
	//and vise versa.

	int numChildren = m_childCollisionAlgorithms.size();
	int i;
	for (i=0;i<numChildren;i++)
	{
		//temporarily exchange parent btCollisionShape with childShape, and recurse
		btCollisionShape* childShape = compoundShape->getChildShape(i);

		//backup
		btTransform	orgTrans = colObj->m_worldTransform;
		btCollisionShape* orgShape = colObj->m_collisionShape;

		btTransform childTrans = compoundShape->getChildTransform(i);
		btTransform	newChildWorldTrans = orgTrans*childTrans ;
		colObj->m_worldTransform = newChildWorldTrans;
		//the contactpoint is still projected back using the original inverted worldtrans
		colObj->m_collisionShape = childShape;
		m_childCollisionAlgorithms[i]->processCollision(colObj,otherObj,dispatchInfo,resultOut);
		//revert back
		colObj->m_collisionShape =orgShape;
		colObj->m_worldTransform = orgTrans;
	}
}

float	btCompoundCollisionAlgorithm::calculateTimeOfImpact(btCollisionObject* body0,btCollisionObject* body1,const btDispatcherInfo& dispatchInfo,btManifoldResult* resultOut)
{

	btCollisionObject* colObj = m_isSwapped? body1 : body0;
	btCollisionObject* otherObj = m_isSwapped? body0 : body1;

	assert (colObj->m_collisionShape->isCompound());
	
	btCompoundShape* compoundShape = static_cast<btCompoundShape*>(colObj->m_collisionShape);

	//We will use the OptimizedBVH, AABB tree to cull potential child-overlaps
	//If both proxies are Compound, we will deal with that directly, by performing sequential/parallel tree traversals
	//given Proxy0 and Proxy1, if both have a tree, Tree0 and Tree1, this means:
	//determine overlapping nodes of Proxy1 using Proxy0 AABB against Tree1
	//then use each overlapping node AABB against Tree0
	//and vise versa.

	float hitFraction = 1.f;

	int numChildren = m_childCollisionAlgorithms.size();
	int i;
	for (i=0;i<numChildren;i++)
	{
		//temporarily exchange parent btCollisionShape with childShape, and recurse
		btCollisionShape* childShape = compoundShape->getChildShape(i);

		//backup
		btTransform	orgTrans = colObj->m_worldTransform;
		btCollisionShape* orgShape = colObj->m_collisionShape;

		btTransform childTrans = compoundShape->getChildTransform(i);
		btTransform	newChildWorldTrans = orgTrans*childTrans ;
		colObj->m_worldTransform = newChildWorldTrans;

		colObj->m_collisionShape = childShape;
		float frac = m_childCollisionAlgorithms[i]->calculateTimeOfImpact(colObj,otherObj,dispatchInfo,resultOut);
		if (frac<hitFraction)
		{
			hitFraction = frac;
		}
		//revert back
		colObj->m_collisionShape =orgShape;
		colObj->m_worldTransform = orgTrans;
	}
	return hitFraction;

}


