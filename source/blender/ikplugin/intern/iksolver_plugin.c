/**
 * $Id$
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
 * Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * The Original Code is Copyright (C) 2001-2002 by NaN Holding BV.
 * All rights reserved.
 *
 * The Original Code is: all of this file.
 *
 * Original author: Benoit Bolsee
 * Contributor(s): 
 *
 * ***** END GPL LICENSE BLOCK *****
 */

#include "MEM_guardedalloc.h"

#include "BIK_api.h"
#include "BLI_blenlib.h"
#include "BLI_arithb.h"

#include "BKE_armature.h"
#include "BKE_utildefines.h"
#include "DNA_object_types.h"
#include "DNA_action_types.h"
#include "DNA_constraint_types.h"
#include "DNA_armature_types.h"

#include "IK_solver.h"
#include "iksolver_plugin.h"

/* ********************** THE IK SOLVER ******************* */

/* allocates PoseTree, and links that to root bone/channel */
/* Note: detecting the IK chain is duplicate code... in drawarmature.c and in transform_conversions.c */
static void initialize_posetree(struct Object *ob, bPoseChannel *pchan_tip)
{
	bPoseChannel *curchan, *pchan_root=NULL, *chanlist[256], **oldchan;
	PoseTree *tree;
	PoseTarget *target;
	bConstraint *con;
	bKinematicConstraint *data;
	int a, segcount= 0, size, newsize, *oldparent, parent;
	
	/* find IK constraint, and validate it */
	for(con= pchan_tip->constraints.first; con; con= con->next) {
		if(con->type==CONSTRAINT_TYPE_KINEMATIC) {
			data=(bKinematicConstraint*)con->data;
			if (data->flag & CONSTRAINT_IK_AUTO) break;
			if (data->tar==NULL) continue;
			if (data->tar->type==OB_ARMATURE && data->subtarget[0]==0) continue;
			if ((con->flag & CONSTRAINT_DISABLE)==0 && (con->enforce!=0.0)) break;
		}
	}
	if(con==NULL) return;
	
	/* exclude tip from chain? */
	if(!(data->flag & CONSTRAINT_IK_TIP))
		pchan_tip= pchan_tip->parent;
	
	/* Find the chain's root & count the segments needed */
	for (curchan = pchan_tip; curchan; curchan=curchan->parent){
		pchan_root = curchan;
		
		curchan->flag |= POSE_CHAIN;	// don't forget to clear this
		chanlist[segcount]=curchan;
		segcount++;
		
		if(segcount==data->rootbone || segcount>255) break; // 255 is weak
	}
	if (!segcount) return;

	/* setup the chain data */
	
	/* we make tree-IK, unless all existing targets are in this chain */
	for(tree= pchan_root->iktree.first; tree; tree= tree->next) {
		for(target= tree->targets.first; target; target= target->next) {
			curchan= tree->pchan[target->tip];
			if(curchan->flag & POSE_CHAIN)
				curchan->flag &= ~POSE_CHAIN;
			else
				break;
		}
		if(target) break;
	}

	/* create a target */
	target= MEM_callocN(sizeof(PoseTarget), "posetarget");
	target->con= con;
	pchan_tip->flag &= ~POSE_CHAIN;

	if(tree==NULL) {
		/* make new tree */
		tree= MEM_callocN(sizeof(PoseTree), "posetree");

		tree->iterations= data->iterations;
		tree->totchannel= segcount;
		tree->stretch = (data->flag & CONSTRAINT_IK_STRETCH);
		
		tree->pchan= MEM_callocN(segcount*sizeof(void*), "ik tree pchan");
		tree->parent= MEM_callocN(segcount*sizeof(int), "ik tree parent");
		for(a=0; a<segcount; a++) {
			tree->pchan[a]= chanlist[segcount-a-1];
			tree->parent[a]= a-1;
		}
		target->tip= segcount-1;
		
		/* AND! link the tree to the root */
		BLI_addtail(&pchan_root->iktree, tree);
	}
	else {
		tree->iterations= MAX2(data->iterations, tree->iterations);
		tree->stretch= tree->stretch && !(data->flag & CONSTRAINT_IK_STRETCH);

		/* skip common pose channels and add remaining*/
		size= MIN2(segcount, tree->totchannel);
		for(a=0; a<size && tree->pchan[a]==chanlist[segcount-a-1]; a++);
		parent= a-1;

		segcount= segcount-a;
		target->tip= tree->totchannel + segcount - 1;

		if (segcount > 0) {
			/* resize array */
			newsize= tree->totchannel + segcount;
			oldchan= tree->pchan;
			oldparent= tree->parent;

			tree->pchan= MEM_callocN(newsize*sizeof(void*), "ik tree pchan");
			tree->parent= MEM_callocN(newsize*sizeof(int), "ik tree parent");
			memcpy(tree->pchan, oldchan, sizeof(void*)*tree->totchannel);
			memcpy(tree->parent, oldparent, sizeof(int)*tree->totchannel);
			MEM_freeN(oldchan);
			MEM_freeN(oldparent);

			/* add new pose channels at the end, in reverse order */
			for(a=0; a<segcount; a++) {
				tree->pchan[tree->totchannel+a]= chanlist[segcount-a-1];
				tree->parent[tree->totchannel+a]= tree->totchannel+a-1;
			}
			tree->parent[tree->totchannel]= parent;
			
			tree->totchannel= newsize;
		}

		/* move tree to end of list, for correct evaluation order */
		BLI_remlink(&pchan_root->iktree, tree);
		BLI_addtail(&pchan_root->iktree, tree);
	}

	/* add target to the tree */
	BLI_addtail(&tree->targets, target);
	/* mark root channel having an IK tree */
	pchan_root->flag |= POSE_IKTREE;
}


/* transform from bone(b) to bone(b+1), store in chan_mat */
static void make_dmats(bPoseChannel *pchan)
{
	if (pchan->parent) {
		float iR_parmat[4][4];
		Mat4Invert(iR_parmat, pchan->parent->pose_mat);
		Mat4MulMat4(pchan->chan_mat,  pchan->pose_mat, iR_parmat);	// delta mat
	}
	else Mat4CpyMat4(pchan->chan_mat, pchan->pose_mat);
}

/* applies IK matrix to pchan, IK is done separated */
/* formula: pose_mat(b) = pose_mat(b-1) * diffmat(b-1, b) * ik_mat(b) */
/* to make this work, the diffmats have to be precalculated! Stored in chan_mat */
static void where_is_ik_bone(bPoseChannel *pchan, float ik_mat[][3])   // nr = to detect if this is first bone
{
	float vec[3], ikmat[4][4];
	
	Mat4CpyMat3(ikmat, ik_mat);
	
	if (pchan->parent)
		Mat4MulSerie(pchan->pose_mat, pchan->parent->pose_mat, pchan->chan_mat, ikmat, NULL, NULL, NULL, NULL, NULL);
	else 
		Mat4MulMat4(pchan->pose_mat, ikmat, pchan->chan_mat);

	/* calculate head */
	VECCOPY(pchan->pose_head, pchan->pose_mat[3]);
	/* calculate tail */
	VECCOPY(vec, pchan->pose_mat[1]);
	VecMulf(vec, pchan->bone->length);
	VecAddf(pchan->pose_tail, pchan->pose_head, vec);

	pchan->flag |= POSE_DONE;
}


/* called from within the core where_is_pose loop, all animsystems and constraints
were executed & assigned. Now as last we do an IK pass */
static void execute_posetree(Object *ob, PoseTree *tree)
{
	float R_parmat[3][3], identity[3][3];
	float iR_parmat[3][3];
	float R_bonemat[3][3];
	float goalrot[3][3], goalpos[3];
	float rootmat[4][4], imat[4][4];
	float goal[4][4], goalinv[4][4];
	float irest_basis[3][3], full_basis[3][3];
	float end_pose[4][4], world_pose[4][4];
	float length, basis[3][3], rest_basis[3][3], start[3], *ikstretch=NULL;
	float resultinf=0.0f;
	int a, flag, hasstretch=0, resultblend=0;
	bPoseChannel *pchan;
	IK_Segment *seg, *parent, **iktree, *iktarget;
	IK_Solver *solver;
	PoseTarget *target;
	bKinematicConstraint *data, *poleangledata=NULL;
	Bone *bone;

	if (tree->totchannel == 0)
		return;
	
	iktree= MEM_mallocN(sizeof(void*)*tree->totchannel, "ik tree");

	for(a=0; a<tree->totchannel; a++) {
		pchan= tree->pchan[a];
		bone= pchan->bone;
		
		/* set DoF flag */
		flag= 0;
		if(!(pchan->ikflag & BONE_IK_NO_XDOF) && !(pchan->ikflag & BONE_IK_NO_XDOF_TEMP))
			flag |= IK_XDOF;
		if(!(pchan->ikflag & BONE_IK_NO_YDOF) && !(pchan->ikflag & BONE_IK_NO_YDOF_TEMP))
			flag |= IK_YDOF;
		if(!(pchan->ikflag & BONE_IK_NO_ZDOF) && !(pchan->ikflag & BONE_IK_NO_ZDOF_TEMP))
			flag |= IK_ZDOF;
		
		if(tree->stretch && (pchan->ikstretch > 0.0)) {
			flag |= IK_TRANS_YDOF;
			hasstretch = 1;
		}
		
		seg= iktree[a]= IK_CreateSegment(flag);
		
		/* find parent */
		if(a == 0)
			parent= NULL;
		else
			parent= iktree[tree->parent[a]];
			
		IK_SetParent(seg, parent);
			
		/* get the matrix that transforms from prevbone into this bone */
		Mat3CpyMat4(R_bonemat, pchan->pose_mat);
		
		/* gather transformations for this IK segment */
		
		if (pchan->parent)
			Mat3CpyMat4(R_parmat, pchan->parent->pose_mat);
		else
			Mat3One(R_parmat);
		
		/* bone offset */
		if (pchan->parent && (a > 0))
			VecSubf(start, pchan->pose_head, pchan->parent->pose_tail);
		else
			/* only root bone (a = 0) has no parent */
			start[0]= start[1]= start[2]= 0.0f;
		
		/* change length based on bone size */
		length= bone->length*VecLength(R_bonemat[1]);
		
		/* compute rest basis and its inverse */
		Mat3CpyMat3(rest_basis, bone->bone_mat);
		Mat3CpyMat3(irest_basis, bone->bone_mat);
		Mat3Transp(irest_basis);
		
		/* compute basis with rest_basis removed */
		Mat3Inv(iR_parmat, R_parmat);
		Mat3MulMat3(full_basis, iR_parmat, R_bonemat);
		Mat3MulMat3(basis, irest_basis, full_basis);
		
		/* basis must be pure rotation */
		Mat3Ortho(basis);
		
		/* transform offset into local bone space */
		Mat3Ortho(iR_parmat);
		Mat3MulVecfl(iR_parmat, start);
		
		IK_SetTransform(seg, start, rest_basis, basis, length);
		
		if (pchan->ikflag & BONE_IK_XLIMIT)
			IK_SetLimit(seg, IK_X, pchan->limitmin[0], pchan->limitmax[0]);
		if (pchan->ikflag & BONE_IK_YLIMIT)
			IK_SetLimit(seg, IK_Y, pchan->limitmin[1], pchan->limitmax[1]);
		if (pchan->ikflag & BONE_IK_ZLIMIT)
			IK_SetLimit(seg, IK_Z, pchan->limitmin[2], pchan->limitmax[2]);
		
		IK_SetStiffness(seg, IK_X, pchan->stiffness[0]);
		IK_SetStiffness(seg, IK_Y, pchan->stiffness[1]);
		IK_SetStiffness(seg, IK_Z, pchan->stiffness[2]);
		
		if(tree->stretch && (pchan->ikstretch > 0.0)) {
			float ikstretch = pchan->ikstretch*pchan->ikstretch;
			IK_SetStiffness(seg, IK_TRANS_Y, MIN2(1.0-ikstretch, 0.99));
			IK_SetLimit(seg, IK_TRANS_Y, 0.001, 1e10);
		}
	}

	solver= IK_CreateSolver(iktree[0]);

	/* set solver goals */

	/* first set the goal inverse transform, assuming the root of tree was done ok! */
	pchan= tree->pchan[0];
	if (pchan->parent)
		/* transform goal by parent mat, so this rotation is not part of the
		   segment's basis. otherwise rotation limits do not work on the
		   local transform of the segment itself. */
		Mat4CpyMat4(rootmat, pchan->parent->pose_mat);
	else
		Mat4One(rootmat);
	VECCOPY(rootmat[3], pchan->pose_head);
	
	Mat4MulMat4 (imat, rootmat, ob->obmat);
	Mat4Invert (goalinv, imat);
	
	for (target=tree->targets.first; target; target=target->next) {
		float polepos[3];
		int poleconstrain= 0;
		
		data= (bKinematicConstraint*)target->con->data;
		
		/* 1.0=ctime, we pass on object for auto-ik (owner-type here is object, even though
		 * strictly speaking, it is a posechannel)
		 */
		get_constraint_target_matrix(target->con, 0, CONSTRAINT_OBTYPE_OBJECT, ob, rootmat, 1.0);
		
		/* and set and transform goal */
		Mat4MulMat4(goal, rootmat, goalinv);
		
		VECCOPY(goalpos, goal[3]);
		Mat3CpyMat4(goalrot, goal);
		
		/* same for pole vector target */
		if(data->poletar) {
			get_constraint_target_matrix(target->con, 1, CONSTRAINT_OBTYPE_OBJECT, ob, rootmat, 1.0);
			
			if(data->flag & CONSTRAINT_IK_SETANGLE) {
				/* don't solve IK when we are setting the pole angle */
				break;
			}
			else {
				Mat4MulMat4(goal, rootmat, goalinv);
				VECCOPY(polepos, goal[3]);
				poleconstrain= 1;

				/* for pole targets, we blend the result of the ik solver
				 * instead of the target position, otherwise we can't get
				 * a smooth transition */
				resultblend= 1;
				resultinf= target->con->enforce;
				
				if(data->flag & CONSTRAINT_IK_GETANGLE) {
					poleangledata= data;
					data->flag &= ~CONSTRAINT_IK_GETANGLE;
				}
			}
		}

		/* do we need blending? */
		if (!resultblend && target->con->enforce!=1.0) {
			float q1[4], q2[4], q[4];
			float fac= target->con->enforce;
			float mfac= 1.0-fac;
			
			pchan= tree->pchan[target->tip];
			
			/* end effector in world space */
			Mat4CpyMat4(end_pose, pchan->pose_mat);
			VECCOPY(end_pose[3], pchan->pose_tail);
			Mat4MulSerie(world_pose, goalinv, ob->obmat, end_pose, 0, 0, 0, 0, 0);
			
			/* blend position */
			goalpos[0]= fac*goalpos[0] + mfac*world_pose[3][0];
			goalpos[1]= fac*goalpos[1] + mfac*world_pose[3][1];
			goalpos[2]= fac*goalpos[2] + mfac*world_pose[3][2];
			
			/* blend rotation */
			Mat3ToQuat(goalrot, q1);
			Mat4ToQuat(world_pose, q2);
			QuatInterpol(q, q1, q2, mfac);
			QuatToMat3(q, goalrot);
		}
		
		iktarget= iktree[target->tip];
		
		if(data->weight != 0.0) {
			if(poleconstrain)
				IK_SolverSetPoleVectorConstraint(solver, iktarget, goalpos,
					polepos, data->poleangle*M_PI/180, (poleangledata == data));
			IK_SolverAddGoal(solver, iktarget, goalpos, data->weight);
		}
		if((data->flag & CONSTRAINT_IK_ROT) && (data->orientweight != 0.0))
			if((data->flag & CONSTRAINT_IK_AUTO)==0)
				IK_SolverAddGoalOrientation(solver, iktarget, goalrot,
					data->orientweight);
	}

	/* solve */
	IK_Solve(solver, 0.0f, tree->iterations);

	if(poleangledata)
		poleangledata->poleangle= IK_SolverGetPoleAngle(solver)*180/M_PI;

	IK_FreeSolver(solver);

	/* gather basis changes */
	tree->basis_change= MEM_mallocN(sizeof(float[3][3])*tree->totchannel, "ik basis change");
	if(hasstretch)
		ikstretch= MEM_mallocN(sizeof(float)*tree->totchannel, "ik stretch");
	
	for(a=0; a<tree->totchannel; a++) {
		IK_GetBasisChange(iktree[a], tree->basis_change[a]);
		
		if(hasstretch) {
			/* have to compensate for scaling received from parent */
			float parentstretch, stretch;
			
			pchan= tree->pchan[a];
			parentstretch= (tree->parent[a] >= 0)? ikstretch[tree->parent[a]]: 1.0;
			
			if(tree->stretch && (pchan->ikstretch > 0.0)) {
				float trans[3], length;
				
				IK_GetTranslationChange(iktree[a], trans);
				length= pchan->bone->length*VecLength(pchan->pose_mat[1]);
				
				ikstretch[a]= (length == 0.0)? 1.0: (trans[1]+length)/length;
			}
			else
				ikstretch[a] = 1.0;
			
			stretch= (parentstretch == 0.0)? 1.0: ikstretch[a]/parentstretch;
			
			VecMulf(tree->basis_change[a][0], stretch);
			VecMulf(tree->basis_change[a][1], stretch);
			VecMulf(tree->basis_change[a][2], stretch);
		}

		if(resultblend && resultinf!=1.0f) {
			Mat3One(identity);
			Mat3BlendMat3(tree->basis_change[a], identity,
				tree->basis_change[a], resultinf);
		}
		
		IK_FreeSegment(iktree[a]);
	}
	
	MEM_freeN(iktree);
	if(ikstretch) MEM_freeN(ikstretch);
}

static void free_posetree(PoseTree *tree)
{
	BLI_freelistN(&tree->targets);
	if(tree->pchan) MEM_freeN(tree->pchan);
	if(tree->parent) MEM_freeN(tree->parent);
	if(tree->basis_change) MEM_freeN(tree->basis_change);
	MEM_freeN(tree);
}

///----------------------------------------
/// Plugin API for legacy iksolver

void iksolver_initialize_tree(struct Object *ob, float ctime)
{
	bPoseChannel *pchan;
	
	for(pchan= ob->pose->chanbase.first; pchan; pchan= pchan->next) {
		if(pchan->constflag & PCHAN_HAS_IK) // flag is set on editing constraints
			initialize_posetree(ob, pchan);	// will attach it to root!
	}
	ob->pose->flag &= ~POSE_WAS_REBUILT;
}

void iksolver_execute_tree(struct Object *ob,  struct bPoseChannel *pchan, float ctime)
{
	while(pchan->iktree.first) {
		PoseTree *tree= pchan->iktree.first;
		int a;
		
		/* 4. walk over the tree for regular solving */
		for(a=0; a<tree->totchannel; a++) {
			if(!(tree->pchan[a]->flag & POSE_DONE))	// successive trees can set the flag
				where_is_pose_bone(ob, tree->pchan[a], ctime);
		}
		/* 5. execute the IK solver */
		execute_posetree(ob, tree);
		
		/* 6. apply the differences to the channels, 
			  we need to calculate the original differences first */
		for(a=0; a<tree->totchannel; a++)
			make_dmats(tree->pchan[a]);
		
		for(a=0; a<tree->totchannel; a++)
			/* sets POSE_DONE */
			where_is_ik_bone(tree->pchan[a], tree->basis_change[a]);
		
		/* 7. and free */
		BLI_remlink(&pchan->iktree, tree);
		free_posetree(tree);
	}
}

