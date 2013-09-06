/*
 * ***** BEGIN GPL LICENSE BLOCK *****
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) 2013 Blender Foundation
 * All rights reserved.
 *
 * The Original Code is: all of this file.
 *
 * Contributor(s): Joshua Leung, Sergej Reich
 *
 * ***** END GPL LICENSE BLOCK *****
 */

/** \file rb_bullet_api.cpp
 *  \ingroup RigidBody
 *  \brief Rigid Body API implementation for Bullet
 */

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
 
/* This file defines the "RigidBody interface" for the 
 * Bullet Physics Engine. This API is designed to be used
 * from C-code in Blender as part of the Rigid Body simulation
 * system.
 *
 * It is based on the Bullet C-API, but is heavily modified to 
 * give access to more data types and to offer a nicer interface.
 *
 * -- Joshua Leung, June 2010
 */

#include <stdio.h>
#include <errno.h>

#include "GPU_draw.h"

#include "RBI_api.h"

#include "btBulletDynamicsCommon.h"

#include "LinearMath/btVector3.h"
#include "LinearMath/btScalar.h"	
#include "LinearMath/btMatrix3x3.h"
#include "LinearMath/btTransform.h"
#include "LinearMath/btConvexHullComputer.h"

#include "BulletCollision/Gimpact/btGImpactShape.h"
#include "BulletCollision/Gimpact/btGImpactCollisionAlgorithm.h"
#include "BulletCollision/CollisionShapes/btScaledBvhTriangleMeshShape.h"

#include "hacdHACD.h"

struct rbDynamicsWorld {
	btDiscreteDynamicsWorld *dynamicsWorld;
	btDefaultCollisionConfiguration *collisionConfiguration;
	btDispatcher *dispatcher;
	btBroadphaseInterface *pairCache;
	btConstraintSolver *constraintSolver;
	btOverlapFilterCallback *filterCallback;
};

enum ActivationType {
	ACTIVATION_COLLISION = 0,
	ACTIVATION_TRIGGER,
	ACTIVATION_TIME
};

struct rbRigidBody {
	btRigidBody *body;
	btDiscreteDynamicsWorld *world;
	int col_groups;
	bool is_trigger;
	bool suspended;
	float saved_mass;
	int activation_type;
};

struct rbCollisionShape {
	btCollisionShape *cshape;
	btCollisionShape *compound;
	btTriangleMesh *mesh;
};

struct rbFilterCallback : public btOverlapFilterCallback
{
	virtual bool needBroadphaseCollision(btBroadphaseProxy *proxy0, btBroadphaseProxy *proxy1) const
	{
		rbRigidBody *rb0 = (rbRigidBody *)((btRigidBody *)proxy0->m_clientObject)->getUserPointer();
		rbRigidBody *rb1 = (rbRigidBody *)((btRigidBody *)proxy1->m_clientObject)->getUserPointer();
		
		bool collides;
		collides = (proxy0->m_collisionFilterGroup & proxy1->m_collisionFilterMask) != 0;
		collides = collides && (proxy1->m_collisionFilterGroup & proxy0->m_collisionFilterMask);
		collides = collides && (rb0->col_groups & rb1->col_groups);
		
		return collides;
	}
};

void nearCallback(btBroadphasePair &collisionPair, btCollisionDispatcher &dispatcher, const btDispatcherInfo &dispatchInfo)
{
	rbRigidBody *rb0 = (rbRigidBody *)((btRigidBody *)collisionPair.m_pProxy0->m_clientObject)->getUserPointer();
	rbRigidBody *rb1 = (rbRigidBody *)((btRigidBody *)collisionPair.m_pProxy1->m_clientObject)->getUserPointer();
	
	if (rb1->suspended && !(rb1->activation_type == ACTIVATION_TRIGGER && !rb0->is_trigger)) {
		btRigidBody *body = rb1->body;
		rb1->suspended = false;
		rb1->world->removeRigidBody(body);
		RB_body_set_mass(rb1, rb1->saved_mass);
		rb1->world->addRigidBody(body);
		body->activate();
	}
	
	dispatcher.defaultNearCallback(collisionPair, dispatcher, dispatchInfo);
}

class rbDebugDraw : public btIDebugDraw {
private:
	int debug_mode;
public:
	rbDebugDraw() { }
	virtual void drawLine(const btVector3& from,const btVector3& to,const btVector3& color)
	{
		float root[3] = {from.x(), from.y(), from.z()};
		float tip[3] = {to.x(), to.y(), to.z()};
		float col[3] = {color.x(), color.y(), color.z()};
		GPU_debug_add_line(root, tip, col);
	}
	virtual void drawContactPoint(const btVector3& PointOnB,const btVector3& normalOnB,btScalar distance,int lifeTime,const btVector3& color)
	{
		float root[3] = {PointOnB.x(), PointOnB.y(), PointOnB.z()};
		btVector3 to = PointOnB + normalOnB;
		float nor_tip[3] = {to.x(), to.y(), to.z()};
		
		float col[3] = {0.0f, 1.0f, 1.0f};
		GPU_debug_add_point(root, col);
		col[0] = 1.0f;
		col[1] = 0.0f;
		col[2] = 1.0f;
		GPU_debug_add_line(root, nor_tip, col);
	}
	virtual void reportErrorWarning(const char *warningString)
	{
		
	}
	virtual void draw3dText(const btVector3& location,const char *textString)
	{
		
	}
	virtual void setDebugMode(int debugMode)
	{
		debug_mode = debugMode;
	}
	virtual int getDebugMode() const
	{
		return debug_mode;
	}
};

static inline void copy_v3_btvec3(float vec[3], const btVector3 &btvec)
{
	vec[0] = (float)btvec[0];
	vec[1] = (float)btvec[1];
	vec[2] = (float)btvec[2];
}
static inline void copy_quat_btquat(float quat[4], const btQuaternion &btquat)
{
	quat[0] = btquat.getW();
	quat[1] = btquat.getX();
	quat[2] = btquat.getY();
	quat[3] = btquat.getZ();
}

/* ********************************** */
/* Dynamics World Methods */

/* Setup ---------------------------- */

rbDynamicsWorld *RB_dworld_new(const float gravity[3])
{
	rbDynamicsWorld *world = new rbDynamicsWorld;
	
	/* collision detection/handling */
	world->collisionConfiguration = new btDefaultCollisionConfiguration();
	
	world->dispatcher = new btCollisionDispatcher(world->collisionConfiguration);
	btGImpactCollisionAlgorithm::registerAlgorithm((btCollisionDispatcher *)world->dispatcher);
	((btCollisionDispatcher *)world->dispatcher)->setNearCallback(nearCallback);
	
	world->pairCache = new btDbvtBroadphase();
	
	world->filterCallback = new rbFilterCallback();
	world->pairCache->getOverlappingPairCache()->setOverlapFilterCallback(world->filterCallback);

	/* constraint solving */
	world->constraintSolver = new btSequentialImpulseConstraintSolver();

	/* world */
	world->dynamicsWorld = new btDiscreteDynamicsWorld(world->dispatcher,
	                                                   world->pairCache,
	                                                   world->constraintSolver,
	                                                   world->collisionConfiguration);

	RB_dworld_set_gravity(world, gravity);
	
	// HACK set debug drawer, this is only temporary
	btIDebugDraw *debugDrawer = new rbDebugDraw();
	debugDrawer->setDebugMode(btIDebugDraw::DBG_DrawWireframe |
	                          btIDebugDraw::DBG_DrawAabb |
	                          btIDebugDraw::DBG_DrawContactPoints |
	                          btIDebugDraw::DBG_DrawText |
	                          btIDebugDraw::DBG_DrawConstraintLimits |
	                          btIDebugDraw::DBG_DrawConstraints |
	                          btIDebugDraw::DBG_DrawConstraintLimits
	);
	world->dynamicsWorld->setDebugDrawer(debugDrawer);
	
	return world;
}

void RB_dworld_delete(rbDynamicsWorld *world)
{
	/* bullet doesn't like if we free these in a different order */
	delete world->dynamicsWorld;
	delete world->constraintSolver;
	delete world->pairCache;
	delete world->dispatcher;
	delete world->collisionConfiguration;
	delete world->filterCallback;
	delete world;
}

/* Settings ------------------------- */

/* Gravity */
void RB_dworld_get_gravity(rbDynamicsWorld *world, float g_out[3])
{
	copy_v3_btvec3(g_out, world->dynamicsWorld->getGravity());
}

void RB_dworld_set_gravity(rbDynamicsWorld *world, const float g_in[3])
{
	world->dynamicsWorld->setGravity(btVector3(g_in[0], g_in[1], g_in[2]));
}

/* Constraint Solver */
void RB_dworld_set_solver_iterations(rbDynamicsWorld *world, int num_solver_iterations)
{
	btContactSolverInfo& info = world->dynamicsWorld->getSolverInfo();
	
	info.m_numIterations = num_solver_iterations;
}

/* Split Impulse */
void RB_dworld_set_split_impulse(rbDynamicsWorld *world, int split_impulse)
{
	btContactSolverInfo& info = world->dynamicsWorld->getSolverInfo();
	
	info.m_splitImpulse = split_impulse;
}

/* Simulation ----------------------- */

void RB_dworld_step_simulation(rbDynamicsWorld *world, float timeStep, int maxSubSteps, float timeSubStep)
{
	world->dynamicsWorld->stepSimulation(timeStep, maxSubSteps, timeSubStep);
	
	// draw debug information
	GPU_debug_reset();
	world->dynamicsWorld->debugDrawWorld();
}

/* Export -------------------------- */

/* Exports entire dynamics world to Bullet's "*.bullet" binary format
 * which is similar to Blender's SDNA system...
 * < rbDynamicsWorld: dynamics world to write to file
 * < filename: assumed to be a valid filename, with .bullet extension 
 */
void RB_dworld_export(rbDynamicsWorld *world, const char *filename)
{
	//create a large enough buffer. There is no method to pre-calculate the buffer size yet.
	int maxSerializeBufferSize = 1024 * 1024 * 5;
	
	btDefaultSerializer *serializer = new btDefaultSerializer(maxSerializeBufferSize);
	world->dynamicsWorld->serialize(serializer);
	
	FILE *file = fopen(filename, "wb");
	if (file) {
		fwrite(serializer->getBufferPointer(), serializer->getCurrentBufferSize(), 1, file);
		fclose(file);
	}
	else {
		 fprintf(stderr, "RB_dworld_export: %s\n", strerror(errno));
	}
}

/* ********************************** */
/* Rigid Body Methods */

/* Setup ---------------------------- */

void RB_dworld_add_body(rbDynamicsWorld *world, rbRigidBody *object, int col_groups)
{
	btRigidBody *body = object->body;
	object->col_groups = col_groups;
	object->world = world->dynamicsWorld;
	
	world->dynamicsWorld->addRigidBody(body);
}

void RB_dworld_remove_body(rbDynamicsWorld *world, rbRigidBody *object)
{
	btRigidBody *body = object->body;
	
	world->dynamicsWorld->removeRigidBody(body);
}

/* Collision detection */

void RB_world_convex_sweep_test(rbDynamicsWorld *world, rbRigidBody *object, const float loc_start[3], const float loc_end[3], float v_location[3],  float v_hitpoint[3],  float v_normal[3], int *r_hit)
{
	btRigidBody *body = object->body;
	btCollisionShape *collisionShape = body->getCollisionShape();
	/* only convex shapes are supported, but user can specify a non convex shape */
	if (collisionShape->isConvex()) {
		btCollisionWorld::ClosestConvexResultCallback result(btVector3(loc_start[0], loc_start[1], loc_start[2]), btVector3(loc_end[0], loc_end[1], loc_end[2]));

		btQuaternion obRot = body->getWorldTransform().getRotation();
		
		btTransform rayFromTrans;
		rayFromTrans.setIdentity();
		rayFromTrans.setRotation(obRot);
		rayFromTrans.setOrigin(btVector3(loc_start[0], loc_start[1], loc_start[2]));

		btTransform rayToTrans;
		rayToTrans.setIdentity();
		rayToTrans.setRotation(obRot);
		rayToTrans.setOrigin(btVector3(loc_end[0], loc_end[1], loc_end[2]));
		
		world->dynamicsWorld->convexSweepTest((btConvexShape*) collisionShape, rayFromTrans, rayToTrans, result, 0);
		
		if (result.hasHit()) {
			*r_hit = 1;
			
			v_location[0] = result.m_convexFromWorld[0]+(result.m_convexToWorld[0]-result.m_convexFromWorld[0])*result.m_closestHitFraction;
			v_location[1] = result.m_convexFromWorld[1]+(result.m_convexToWorld[1]-result.m_convexFromWorld[1])*result.m_closestHitFraction;
			v_location[2] = result.m_convexFromWorld[2]+(result.m_convexToWorld[2]-result.m_convexFromWorld[2])*result.m_closestHitFraction;
			
			v_hitpoint[0] = result.m_hitPointWorld[0];
			v_hitpoint[1] = result.m_hitPointWorld[1];
			v_hitpoint[2] = result.m_hitPointWorld[2];
			
			v_normal[0] = result.m_hitNormalWorld[0];
			v_normal[1] = result.m_hitNormalWorld[1];
			v_normal[2] = result.m_hitNormalWorld[2];
			
		}
		else {
			*r_hit = 0;
		}
	}
	else{
		/* we need to return a value if user passes non convex body, to report */
		*r_hit = -2;
	}
}

/* ............ */

rbRigidBody *RB_body_new(rbCollisionShape *shape, const float loc[3], const float rot[4])
{
	rbRigidBody *object = new rbRigidBody;
	/* current transform */
	btTransform trans;
	trans.setOrigin(btVector3(loc[0], loc[1], loc[2]));
	trans.setRotation(btQuaternion(rot[1], rot[2], rot[3], rot[0]));
	
	/* create motionstate, which is necessary for interpolation (includes reverse playback) */
	btDefaultMotionState *motionState = new btDefaultMotionState(trans);
	
	/* make rigidbody */
	btRigidBody::btRigidBodyConstructionInfo rbInfo(1.0f, motionState, shape->compound);
	
	object->body = new btRigidBody(rbInfo);
	
	object->body->setUserPointer(object);
	
	object->is_trigger = false;
	object->suspended = false;
	
	return object;
}

void RB_body_delete(rbRigidBody *object)
{
	btRigidBody *body = object->body;
	
	/* motion state */
	btMotionState *ms = body->getMotionState();
	if (ms)
		delete ms;
	
	/* collision shape is done elsewhere... */
	
	/* body itself */
	
	/* manually remove constraint refs of the rigid body, normally this happens when removing constraints from the world
	 * but since we delete everything when the world is rebult, we need to do it manually here */
	for (int i = body->getNumConstraintRefs() - 1; i >= 0; i--) {
		btTypedConstraint *con = body->getConstraintRef(i);
		body->removeConstraintRef(con);
	}
	
	delete body;
	delete object;
}

/* Settings ------------------------- */

void RB_body_set_collision_shape(rbRigidBody *object, rbCollisionShape *shape)
{
	btRigidBody *body = object->body;
	
	/* set new collision shape */
	body->setCollisionShape(shape->compound);
	
	/* recalculate inertia, since that depends on the collision shape... */
	RB_body_set_mass(object, RB_body_get_mass(object));
}

/* ............ */

float RB_body_get_mass(rbRigidBody *object)
{
	btRigidBody *body = object->body;
	
	/* there isn't really a mass setting, but rather 'inverse mass'  
	 * which we convert back to mass by taking the reciprocal again 
	 */
	float value = (float)body->getInvMass();
	
	if (value)
		value = 1.0 / value;
		
	return value;
}

void RB_body_set_mass(rbRigidBody *object, float value)
{
	btRigidBody *body = object->body;
	btVector3 localInertia(0, 0, 0);
	
	/* calculate new inertia if non-zero mass */
	if (value) {
		btCollisionShape *shape = body->getCollisionShape();
		shape->calculateLocalInertia(value, localInertia);
	}
	if (!object->suspended) {
		body->setMassProps(value, localInertia);
		body->updateInertiaTensor();
	}
}


float RB_body_get_friction(rbRigidBody *object)
{
	btRigidBody *body = object->body;
	return body->getFriction();
}

void RB_body_set_friction(rbRigidBody *object, float value)
{
	btRigidBody *body = object->body;
	body->setFriction(value);
}


float RB_body_get_restitution(rbRigidBody *object)
{
	btRigidBody *body = object->body;
	return body->getRestitution();
}

void RB_body_set_restitution(rbRigidBody *object, float value)
{
	btRigidBody *body = object->body;
	body->setRestitution(value);
}


float RB_body_get_linear_damping(rbRigidBody *object)
{
	btRigidBody *body = object->body;
	return body->getLinearDamping();
}

void RB_body_set_linear_damping(rbRigidBody *object, float value)
{
	RB_body_set_damping(object, value, RB_body_get_linear_damping(object));
}

float RB_body_get_angular_damping(rbRigidBody *object)
{
	btRigidBody *body = object->body;
	return body->getAngularDamping();
}

void RB_body_set_angular_damping(rbRigidBody *object, float value)
{
	RB_body_set_damping(object, RB_body_get_linear_damping(object), value);
}

void RB_body_set_damping(rbRigidBody *object, float linear, float angular)
{
	btRigidBody *body = object->body;
	body->setDamping(linear, angular);
}


float RB_body_get_linear_sleep_thresh(rbRigidBody *object)
{
	btRigidBody *body = object->body;
	return body->getLinearSleepingThreshold();
}

void RB_body_set_linear_sleep_thresh(rbRigidBody *object, float value)
{
	RB_body_set_sleep_thresh(object, value, RB_body_get_angular_sleep_thresh(object));
}

float RB_body_get_angular_sleep_thresh(rbRigidBody *object)
{
	btRigidBody *body = object->body;
	return body->getAngularSleepingThreshold();
}

void RB_body_set_angular_sleep_thresh(rbRigidBody *object, float value)
{
	RB_body_set_sleep_thresh(object, RB_body_get_linear_sleep_thresh(object), value);
}

void RB_body_set_sleep_thresh(rbRigidBody *object, float linear, float angular)
{
	btRigidBody *body = object->body;
	body->setSleepingThresholds(linear, angular);
}

/* ............ */

void RB_body_get_linear_velocity(rbRigidBody *object, float v_out[3])
{
	btRigidBody *body = object->body;
	
	copy_v3_btvec3(v_out, body->getLinearVelocity());
}

void RB_body_set_linear_velocity(rbRigidBody *object, const float v_in[3])
{
	btRigidBody *body = object->body;
	
	body->setLinearVelocity(btVector3(v_in[0], v_in[1], v_in[2]));
}


void RB_body_get_angular_velocity(rbRigidBody *object, float v_out[3])
{
	btRigidBody *body = object->body;
	
	copy_v3_btvec3(v_out, body->getAngularVelocity());
}

void RB_body_set_angular_velocity(rbRigidBody *object, const float v_in[3])
{
	btRigidBody *body = object->body;
	
	body->setAngularVelocity(btVector3(v_in[0], v_in[1], v_in[2]));
}

void RB_body_set_linear_factor(rbRigidBody *object, float x, float y, float z)
{
	btRigidBody *body = object->body;
	body->setLinearFactor(btVector3(x, y, z));
}

void RB_body_set_angular_factor(rbRigidBody *object, float x, float y, float z)
{
	btRigidBody *body = object->body;
	body->setAngularFactor(btVector3(x, y, z));
}

/* ............ */

void RB_body_set_kinematic_state(rbRigidBody *object, int kinematic)
{
	if (object->suspended)
		return;
	btRigidBody *body = object->body;
	if (kinematic)
		body->setCollisionFlags(body->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
	else
		body->setCollisionFlags(body->getCollisionFlags() & ~btCollisionObject::CF_KINEMATIC_OBJECT);
}

void RB_body_suspend(rbRigidBody *object)
{
	object->saved_mass = RB_body_get_mass(object);
	RB_body_set_mass(object, 0.0f);
	object->suspended = true;
}

void RB_body_set_trigger(rbRigidBody *object, int trigger)
{
	object->is_trigger = trigger;
}

void RB_body_set_ghost(rbRigidBody *object, int ghost)
{
	btRigidBody *body = object->body;
	if (ghost)
		body->setCollisionFlags(body->getCollisionFlags() | btCollisionObject::CF_NO_CONTACT_RESPONSE);
	else
		body->setCollisionFlags(body->getCollisionFlags() & ~btCollisionObject::CF_NO_CONTACT_RESPONSE);
}

void RB_body_set_activation_type(rbRigidBody *object, int type)
{
	object->activation_type = type;
}

/* ............ */

void RB_body_set_activation_state(rbRigidBody *object, int use_deactivation)
{
	btRigidBody *body = object->body;
	if (use_deactivation)
		body->forceActivationState(ACTIVE_TAG);
	else
		body->setActivationState(DISABLE_DEACTIVATION);
}
void RB_body_activate(rbRigidBody *object)
{
	btRigidBody *body = object->body;
	body->setActivationState(ACTIVE_TAG);
}
void RB_body_deactivate(rbRigidBody *object)
{
	btRigidBody *body = object->body;
	body->setActivationState(ISLAND_SLEEPING);
}

/* ............ */



/* Simulation ----------------------- */

/* The transform matrices Blender uses are OpenGL-style matrices, 
 * while Bullet uses the Right-Handed coordinate system style instead.
 */

void RB_body_get_transform_matrix(rbRigidBody *object, float m_out[4][4])
{
	btRigidBody *body = object->body;
	btMotionState *ms = body->getMotionState();
	
	btTransform trans;
	ms->getWorldTransform(trans);
	
	trans.getOpenGLMatrix((btScalar *)m_out);
}

void RB_body_set_loc_rot(rbRigidBody *object, const float loc[3], const float rot[4])
{
	btRigidBody *body = object->body;
	btMotionState *ms = body->getMotionState();
	
	/* set transform matrix */
	btTransform trans;
	trans.setOrigin(btVector3(loc[0], loc[1], loc[2]));
	trans.setRotation(btQuaternion(rot[1], rot[2], rot[3], rot[0]));
	
	ms->setWorldTransform(trans);
}

void RB_body_set_scale(rbRigidBody *object, const float scale[3])
{
	btRigidBody *body = object->body;
	
	/* apply scaling factor from matrix above to the collision shape */
	btCollisionShape *cshape = body->getCollisionShape();
	if (cshape) {
		cshape->setLocalScaling(btVector3(scale[0], scale[1], scale[2]));
		
		/* GIimpact shapes have to be updated to take scaling into account */
		if (cshape->getShapeType() == GIMPACT_SHAPE_PROXYTYPE)
			((btGImpactMeshShape *)cshape)->updateBound();
	}
}

/* ............ */
/* Read-only state info about status of simulation */

void RB_body_get_position(rbRigidBody *object, float v_out[3])
{
	btRigidBody *body = object->body;
	
	copy_v3_btvec3(v_out, body->getWorldTransform().getOrigin());
}

void RB_body_get_pos_orn(rbRigidBody *object, float pos_out[3], float orn_out[4])
{
	btRigidBody *body = object->body;
	
	copy_v3_btvec3(pos_out, body->getWorldTransform().getOrigin());
	copy_quat_btquat(orn_out, body->getWorldTransform().getRotation());
}

void RB_body_get_compound_pos_orn(rbRigidBody *parent_object, rbCollisionShape *child_shape, float pos_out[3], float orn_out[4])
{
	btRigidBody *body = parent_object->body;
	btCompoundShape *compound = (btCompoundShape*)body->getCollisionShape();
	btTransform transform = body->getWorldTransform();
	btTransform child_transform;
	
	for (int i = 0; i < compound->getNumChildShapes(); i++) {
		if (child_shape->compound == compound->getChildShape(i)) {
			child_transform = compound->getChildTransform(i);
			break;
		}
	}
	transform *= child_transform;
	
	copy_v3_btvec3(pos_out, transform.getOrigin());
	copy_quat_btquat(orn_out, transform.getRotation());
}

/* ............ */
/* Overrides for simulation */

void RB_body_apply_central_force(rbRigidBody *object, const float v_in[3])
{
	btRigidBody *body = object->body;
	
	body->applyCentralForce(btVector3(v_in[0], v_in[1], v_in[2]));
}

/* ********************************** */
/* Collision Shape Methods */

/* Setup (Standard Shapes) ----------- */

static rbCollisionShape *prepare_primitive_shape(btCollisionShape *cshape, const float loc[3], const float rot[4], const float center[3])
{
	rbCollisionShape *shape = new rbCollisionShape;
	shape->cshape = cshape;
	shape->mesh = NULL;
	
	btTransform world_transform = btTransform();
	world_transform.setIdentity();
	world_transform.setOrigin(btVector3(loc[0], loc[1], loc[2]));
	world_transform.setRotation(btQuaternion(rot[1], rot[2], rot[3], rot[0]));
	
	shape->compound = new btCompoundShape();
	btTransform compound_transform;
	compound_transform.setIdentity();
	/* adjust center of mass */
	compound_transform.setOrigin(btVector3(center[0], center[1], center[2]));
	compound_transform.setRotation(btQuaternion(rot[1], rot[2], rot[3], rot[0]));
	compound_transform = world_transform.inverse() * compound_transform;
	
	((btCompoundShape*)shape->compound)->addChildShape(compound_transform, shape->cshape);
	
	return shape;
}

rbCollisionShape *RB_shape_new_box(float x, float y, float z, const float loc[3], const float rot[4], const float center[3])
{
	return prepare_primitive_shape(new btBoxShape(btVector3(x, y, z)), loc, rot, center);
}

rbCollisionShape *RB_shape_new_sphere(float radius, const float loc[3], const float rot[4], const float center[3])
{
	return prepare_primitive_shape(new btSphereShape(radius), loc, rot, center);
}

rbCollisionShape *RB_shape_new_capsule(float radius, float height, const float loc[3], const float rot[4], const float center[3])
{
	return prepare_primitive_shape(new btCapsuleShapeZ(radius, height), loc, rot, center);
}

rbCollisionShape *RB_shape_new_cone(float radius, float height, const float loc[3], const float rot[4], const float center[3])
{
	return prepare_primitive_shape(new btConeShapeZ(radius, height), loc, rot, center);
}

rbCollisionShape *RB_shape_new_cylinder(float radius, float height, const float loc[3], const float rot[4], const float center[3])
{
	return prepare_primitive_shape(new btCylinderShapeZ(btVector3(radius, radius, height)), loc, rot, center);
}

/* Setup (Convex Hull) ------------ */

rbCollisionShape *RB_shape_new_convex_hull(float *verts, int stride, int count, float margin, bool *can_embed)
{
	btConvexHullComputer hull_computer = btConvexHullComputer();
	
	// try to embed the margin, if that fails don't shrink the hull
	if (hull_computer.compute(verts, stride, count, margin, 0.0f) < 0.0f) {
		hull_computer.compute(verts, stride, count, 0.0f, 0.0f);
		*can_embed = false;
	}
	
	rbCollisionShape *shape = new rbCollisionShape;
	btConvexHullShape *hull_shape = new btConvexHullShape(&(hull_computer.vertices[0].getX()), hull_computer.vertices.size());
	
	shape->cshape = hull_shape;
	shape->mesh = NULL;
	
	shape->compound = new btCompoundShape();
	btTransform compound_transform;
	compound_transform.setIdentity();
	((btCompoundShape*)shape->compound)->addChildShape(compound_transform, shape->cshape);
	
	return shape;
}

/* Setup (Triangle Mesh) ---------- */

/* Need to call rbTriMeshNewData() followed by rbTriMeshAddTriangle() several times 
 * to set up the mesh buffer BEFORE calling rbShapeNewTriMesh(). Otherwise,
 * we get nasty crashes...
 */

rbMeshData *RB_trimesh_data_new(int num_tris, int num_verts)
{
	// XXX: welding threshold?
	btTriangleMesh *mesh = new btTriangleMesh(true, false);
	
	mesh->preallocateIndices(num_tris * 3);
	mesh->preallocateVertices(num_verts);
	
	return (rbMeshData *) mesh;
}
 
void RB_trimesh_add_triangle(rbMeshData *mesh, const float v1[3], const float v2[3], const float v3[3])
{
	btTriangleMesh *meshData = reinterpret_cast<btTriangleMesh*>(mesh);
	
	/* cast vertices to usable forms for Bt-API */
	btVector3 vtx1((btScalar)v1[0], (btScalar)v1[1], (btScalar)v1[2]);
	btVector3 vtx2((btScalar)v2[0], (btScalar)v2[1], (btScalar)v2[2]);
	btVector3 vtx3((btScalar)v3[0], (btScalar)v3[1], (btScalar)v3[2]);
	
	/* add to the mesh 
	 *	- remove duplicated verts is enabled
	 */
	meshData->addTriangle(vtx1, vtx2, vtx3, false);
}
 
rbCollisionShape *RB_shape_new_trimesh(rbMeshData *mesh)
{
	rbCollisionShape *shape = new rbCollisionShape;
	btTriangleMesh *tmesh = reinterpret_cast<btTriangleMesh*>(mesh);
	
	/* triangle-mesh we create is a BVH wrapper for triangle mesh data (for faster lookups) */
	// RB_TODO perhaps we need to allow saving out this for performance when rebuilding?
	btBvhTriangleMeshShape *unscaledShape = new btBvhTriangleMeshShape(tmesh, true, true);
	
	shape->cshape = new btScaledBvhTriangleMeshShape(unscaledShape, btVector3(1.0f, 1.0f, 1.0f));
	shape->mesh = tmesh;
	
	shape->compound = new btCompoundShape();
	btTransform compound_transform;
	compound_transform.setIdentity();
	((btCompoundShape*)shape->compound)->addChildShape(compound_transform, shape->cshape);
	
	return shape;
}

rbCollisionShape *RB_shape_new_gimpact_mesh(rbMeshData *mesh)
{
	rbCollisionShape *shape = new rbCollisionShape;
	/* interpret mesh buffer as btTriangleIndexVertexArray (i.e. an impl of btStridingMeshInterface) */
	btTriangleMesh *tmesh = reinterpret_cast<btTriangleMesh*>(mesh);
	
	btGImpactMeshShape *gimpactShape = new btGImpactMeshShape(tmesh);
	gimpactShape->updateBound(); // TODO: add this to the update collision margin call?
	
	shape->cshape = gimpactShape;
	shape->mesh = tmesh;
	
	shape->compound = new btCompoundShape();
	btTransform compound_transform;
	compound_transform.setIdentity();
	((btCompoundShape*)shape->compound)->addChildShape(compound_transform, shape->cshape);
	
	return shape;
}

struct rbHACDMeshData {
	std::vector< HACD::Vec3<HACD::Real> > verts;
	std::vector< HACD::Vec3<long> > tris;
};

rbHACDMeshData *RB_hacd_mesh_new(int num_tris, int num_verts)
{
	rbHACDMeshData *mesh = new rbHACDMeshData;
	
	mesh->verts.reserve(num_verts);
	mesh->tris.reserve(num_tris);
	
	return mesh;
}
 
void RB_hacd_mesh_add_triangle(rbHACDMeshData *mesh, const float v1[3], const float v2[3], const float v3[3])
{
	mesh->tris.push_back(HACD::Vec3<long>(mesh->verts.size(), mesh->verts.size()+1, mesh->verts.size()+2));
	mesh->verts.push_back(HACD::Vec3<HACD::Real>(v1[0], v1[1], v1[2]));
	mesh->verts.push_back(HACD::Vec3<HACD::Real>(v2[0], v2[1], v2[2]));
	mesh->verts.push_back(HACD::Vec3<HACD::Real>(v3[0], v3[1], v3[2]));
}

rbCollisionShape *RB_shape_new_convex_decomp(rbHACDMeshData *mesh)
{
	rbCollisionShape *shape = new rbCollisionShape;
	btCompoundShape *compound = new btCompoundShape();
	
	HACD::HACD hacd;
	hacd.SetPoints(&mesh->verts[0]);
	hacd.SetNPoints(mesh->verts.size());
	hacd.SetTriangles(&mesh->tris[0]);
	hacd.SetNTriangles(mesh->tris.size());

	hacd.Compute();
	
	int num_clusters = hacd.GetNClusters();
	
	for (int i = 0; i < num_clusters; i++) {
		int num_verts = hacd.GetNPointsCH(i);
		int num_tris = hacd.GetNTrianglesCH(i);
		HACD::Vec3<HACD::Real> verts[num_verts];
		HACD::Vec3<long> tris[num_tris];
		hacd.GetCH(i, verts, tris);
		
		btVector3 hull_verts[num_verts];
		for (int i = 0; i < num_verts; i++) {
			hull_verts[i].setX(verts[i].X());
			hull_verts[i].setY(verts[i].Y());
			hull_verts[i].setZ(verts[i].Z());
		}
		
		btTransform transform;
		transform.setIdentity();
		btConvexHullShape *hull = new btConvexHullShape(&hull_verts[0].getX(), num_verts);
		compound->addChildShape(transform, hull);
	}
	
	shape->mesh = NULL;
	shape->compound = compound;
	shape->cshape = compound;
	
	delete mesh;
	
	return shape;
}

void RB_shape_add_compound_child(rbRigidBody *parent, rbCollisionShape *child, float child_pos[3], float child_orn[4])
{
	btCompoundShape *compound = (btCompoundShape*)parent->body->getCollisionShape();
	btTransform parent_transform = parent->body->getCenterOfMassTransform(); // TODO center of mass?
	btTransform transform;
	transform.setOrigin(btVector3(child_pos[0], child_pos[1], child_pos[2]));
	transform.setRotation(btQuaternion(child_orn[1], child_orn[2], child_orn[3], child_orn[0]));
	transform = parent_transform.inverse() * transform;
	compound->addChildShape(transform, child->compound);
}

/* Cleanup --------------------------- */

void RB_shape_delete(rbCollisionShape *shape)
{
	if (shape->cshape->getShapeType() == SCALED_TRIANGLE_MESH_SHAPE_PROXYTYPE) {
		btBvhTriangleMeshShape *child_shape = ((btScaledBvhTriangleMeshShape *)shape->cshape)->getChildShape();
		if (child_shape)
			delete child_shape;
	}
	if (shape->mesh)
		delete shape->mesh;
	if (shape->cshape != shape->compound)
		delete shape->cshape;
	delete shape->compound;
	delete shape;
}

/* Settings --------------------------- */

float RB_shape_get_margin(rbCollisionShape *shape)
{
	return shape->cshape->getMargin();
}

void RB_shape_set_margin(rbCollisionShape *shape, float value)
{
	shape->cshape->setMargin(value);
}

/* ********************************** */
/* Constraints */

/* Setup ----------------------------- */

void RB_dworld_add_constraint(rbDynamicsWorld *world, rbConstraint *con, int disable_collisions)
{
	btTypedConstraint *constraint = reinterpret_cast<btTypedConstraint*>(con);
	
	world->dynamicsWorld->addConstraint(constraint, disable_collisions);
}

void RB_dworld_remove_constraint(rbDynamicsWorld *world, rbConstraint *con)
{
	btTypedConstraint *constraint = reinterpret_cast<btTypedConstraint*>(con);
	
	world->dynamicsWorld->removeConstraint(constraint);
}

/* ............ */

static void make_constraint_transforms(btTransform &transform1, btTransform &transform2, btRigidBody *body1, btRigidBody *body2, float pivot[3], float orn[4])
{
	btTransform pivot_transform = btTransform();
	pivot_transform.setOrigin(btVector3(pivot[0], pivot[1], pivot[2]));
	pivot_transform.setRotation(btQuaternion(orn[1], orn[2], orn[3], orn[0]));
	
	transform1 = body1->getWorldTransform().inverse() * pivot_transform;
	transform2 = body2->getWorldTransform().inverse() * pivot_transform;
}

rbConstraint *RB_constraint_new_point(float pivot[3], rbRigidBody *rb1, rbRigidBody *rb2)
{
	btRigidBody *body1 = rb1->body;
	btRigidBody *body2 = rb2->body;
	
	btVector3 pivot1 = body1->getWorldTransform().inverse() * btVector3(pivot[0], pivot[1], pivot[2]);
	btVector3 pivot2 = body2->getWorldTransform().inverse() * btVector3(pivot[0], pivot[1], pivot[2]);
	
	btTypedConstraint *con = new btPoint2PointConstraint(*body1, *body2, pivot1, pivot2);
	
	return (rbConstraint *)con;
}

rbConstraint *RB_constraint_new_fixed(float pivot[3], float orn[4], rbRigidBody *rb1, rbRigidBody *rb2)
{
	btRigidBody *body1 = rb1->body;
	btRigidBody *body2 = rb2->body;
	btTransform transform1;
	btTransform transform2;
	
	make_constraint_transforms(transform1, transform2, body1, body2, pivot, orn);
	
	btGeneric6DofConstraint *con = new btGeneric6DofConstraint(*body1, *body2, transform1, transform2, true);
	
	/* lock all axes */
	for (int i = 0; i < 6; i++)
		con->setLimit(i, 0, 0);
	
	return (rbConstraint *)con;
}

rbConstraint *RB_constraint_new_hinge(float pivot[3], float orn[4], rbRigidBody *rb1, rbRigidBody *rb2)
{
	btRigidBody *body1 = rb1->body;
	btRigidBody *body2 = rb2->body;
	btTransform transform1;
	btTransform transform2;
	
	make_constraint_transforms(transform1, transform2, body1, body2, pivot, orn);
	
	btHingeConstraint *con = new btHingeConstraint(*body1, *body2, transform1, transform2);
	
	return (rbConstraint *)con;
}

rbConstraint *RB_constraint_new_slider(float pivot[3], float orn[4], rbRigidBody *rb1, rbRigidBody *rb2)
{
	btRigidBody *body1 = rb1->body;
	btRigidBody *body2 = rb2->body;
	btTransform transform1;
	btTransform transform2;
	
	make_constraint_transforms(transform1, transform2, body1, body2, pivot, orn);
	
	btSliderConstraint *con = new btSliderConstraint(*body1, *body2, transform1, transform2, true);
	
	return (rbConstraint *)con;
}

rbConstraint *RB_constraint_new_piston(float pivot[3], float orn[4], rbRigidBody *rb1, rbRigidBody *rb2)
{
	btRigidBody *body1 = rb1->body;
	btRigidBody *body2 = rb2->body;
	btTransform transform1;
	btTransform transform2;
	
	make_constraint_transforms(transform1, transform2, body1, body2, pivot, orn);
	
	btSliderConstraint *con = new btSliderConstraint(*body1, *body2, transform1, transform2, true);
	con->setUpperAngLimit(-1.0f); // unlock rotation axis
	
	return (rbConstraint *)con;
}

rbConstraint *RB_constraint_new_6dof(float pivot[3], float orn[4], rbRigidBody *rb1, rbRigidBody *rb2)
{
	btRigidBody *body1 = rb1->body;
	btRigidBody *body2 = rb2->body;
	btTransform transform1;
	btTransform transform2;
	
	make_constraint_transforms(transform1, transform2, body1, body2, pivot, orn);
	
	btTypedConstraint *con = new btGeneric6DofConstraint(*body1, *body2, transform1, transform2, true);
	
	return (rbConstraint *)con;
}

rbConstraint *RB_constraint_new_6dof_spring(float pivot[3], float orn[4], rbRigidBody *rb1, rbRigidBody *rb2)
{
	btRigidBody *body1 = rb1->body;
	btRigidBody *body2 = rb2->body;
	btTransform transform1;
	btTransform transform2;
	
	make_constraint_transforms(transform1, transform2, body1, body2, pivot, orn);
	
	btTypedConstraint *con = new btGeneric6DofSpringConstraint(*body1, *body2, transform1, transform2, true);
	
	return (rbConstraint *)con;
}

rbConstraint *RB_constraint_new_motor(float pivot[3], float orn[4], rbRigidBody *rb1, rbRigidBody *rb2)
{
	btRigidBody *body1 = rb1->body;
	btRigidBody *body2 = rb2->body;
	btTransform transform1;
	btTransform transform2;
	
	make_constraint_transforms(transform1, transform2, body1, body2, pivot, orn);
	
	btGeneric6DofConstraint *con = new btGeneric6DofConstraint(*body1, *body2, transform1, transform2, true);
	
	/* unlock constraint axes */
	for (int i = 0; i < 6; i++) {
		con->setLimit(i, 0.0f, -1.0f);
	}
	/* unlock motor axes */
	con->getTranslationalLimitMotor()->m_upperLimit.setValue(-1.0f, -1.0f, -1.0f);
	
	return (rbConstraint*)con;
}

/* Cleanup ----------------------------- */

void RB_constraint_delete(rbConstraint *con)
{
	btTypedConstraint *constraint = reinterpret_cast<btTypedConstraint*>(con);
	delete constraint;
}

/* Settings ------------------------- */

void RB_constraint_set_enabled(rbConstraint *con, int enabled)
{
	btTypedConstraint *constraint = reinterpret_cast<btTypedConstraint*>(con);
	
	constraint->setEnabled(enabled);
}

void RB_constraint_set_limits_hinge(rbConstraint *con, float lower, float upper)
{
	btHingeConstraint *constraint = reinterpret_cast<btHingeConstraint*>(con);
	
	// RB_TODO expose these
	float softness = 0.9f;
	float bias_factor = 0.3f;
	float relaxation_factor = 1.0f;
	
	constraint->setLimit(lower, upper, softness, bias_factor, relaxation_factor);
}

void RB_constraint_set_limits_slider(rbConstraint *con, float lower, float upper)
{
	btSliderConstraint *constraint = reinterpret_cast<btSliderConstraint*>(con);
	
	constraint->setLowerLinLimit(lower);
	constraint->setUpperLinLimit(upper);
}

void RB_constraint_set_limits_piston(rbConstraint *con, float lin_lower, float lin_upper, float ang_lower, float ang_upper)
{
	btSliderConstraint *constraint = reinterpret_cast<btSliderConstraint*>(con);
	
	constraint->setLowerLinLimit(lin_lower);
	constraint->setUpperLinLimit(lin_upper);
	constraint->setLowerAngLimit(ang_lower);
	constraint->setUpperAngLimit(ang_upper);
}

void RB_constraint_set_limits_6dof(rbConstraint *con, int axis, float lower, float upper)
{
	btGeneric6DofConstraint *constraint = reinterpret_cast<btGeneric6DofConstraint*>(con);
	
	constraint->setLimit(axis, lower, upper);
}

void RB_constraint_set_stiffness_6dof_spring(rbConstraint *con, int axis, float stiffness)
{
	btGeneric6DofSpringConstraint *constraint = reinterpret_cast<btGeneric6DofSpringConstraint*>(con);
	
	constraint->setStiffness(axis, stiffness);
}

void RB_constraint_set_damping_6dof_spring(rbConstraint *con, int axis, float damping)
{
	btGeneric6DofSpringConstraint *constraint = reinterpret_cast<btGeneric6DofSpringConstraint*>(con);
	
	// invert damping range so that 0 = no damping
	constraint->setDamping(axis, 1.0f - damping);
}

void RB_constraint_set_spring_6dof_spring(rbConstraint *con, int axis, int enable)
{
	btGeneric6DofSpringConstraint *constraint = reinterpret_cast<btGeneric6DofSpringConstraint*>(con);
	
	constraint->enableSpring(axis, enable);
}

void RB_constraint_set_equilibrium_6dof_spring(rbConstraint *con)
{
	btGeneric6DofSpringConstraint *constraint = reinterpret_cast<btGeneric6DofSpringConstraint*>(con);
	
	constraint->setEquilibriumPoint();
}

void RB_constraint_set_solver_iterations(rbConstraint *con, int num_solver_iterations)
{
	btTypedConstraint *constraint = reinterpret_cast<btTypedConstraint*>(con);
	
	constraint->setOverrideNumSolverIterations(num_solver_iterations);
}

void RB_constraint_set_breaking_threshold(rbConstraint *con, float threshold)
{
	btTypedConstraint *constraint = reinterpret_cast<btTypedConstraint*>(con);
	
	constraint->setBreakingImpulseThreshold(threshold);
}

void RB_constraint_set_enable_motor(rbConstraint *con, int enable_lin, int enable_ang)
{
	btGeneric6DofConstraint *constraint = reinterpret_cast<btGeneric6DofConstraint*>(con);
	
	constraint->getTranslationalLimitMotor()->m_enableMotor[0] = enable_lin;
	constraint->getRotationalLimitMotor(0)->m_enableMotor = enable_ang;
}

void RB_constraint_set_max_impulse_motor(rbConstraint *con, float max_impulse_lin, float max_impulse_ang)
{
	btGeneric6DofConstraint *constraint = reinterpret_cast<btGeneric6DofConstraint*>(con);
	
	constraint->getTranslationalLimitMotor()->m_maxMotorForce.setX(max_impulse_lin);
	constraint->getRotationalLimitMotor(0)->m_maxMotorForce = max_impulse_ang;
}

void RB_constraint_set_target_velocity_motor(rbConstraint *con, float velocity_lin, float velocity_ang)
{
	btGeneric6DofConstraint *constraint = reinterpret_cast<btGeneric6DofConstraint*>(con);
	
	constraint->getTranslationalLimitMotor()->m_targetVelocity.setX(velocity_lin);
	constraint->getRotationalLimitMotor(0)->m_targetVelocity = velocity_ang;
}

/* ********************************** */
