/**
 * $Id$
 *
 * ***** BEGIN GPL/BL DUAL LICENSE BLOCK *****
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version. The Blender
 * Foundation also sells licenses for use in proprietary software under
 * the Blender License.  See http://www.blender.org/BL/ for information
 * about this.
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
 * Contributor(s): none yet.
 *
 * ***** END GPL/BL DUAL LICENSE BLOCK *****
 */

#include <stdio.h> 
#include <string.h>
#include <math.h>

#include "MEM_guardedalloc.h"
#include "nla.h"

#include "BLI_blenlib.h"
#include "BLI_arithb.h"

#include "DNA_armature_types.h"
#include "DNA_constraint_types.h"
#include "DNA_object_types.h"
#include "DNA_action_types.h"
#include "DNA_curve_types.h"
#include "DNA_scene_types.h"

#include "BKE_utildefines.h"
#include "BKE_action.h"
#include "BKE_anim.h" // for the curve calculation part
#include "BKE_armature.h"
#include "BKE_blender.h"
#include "BKE_constraint.h"
#include "BKE_displist.h"
#include "BKE_object.h"
#include "BKE_ipo.h"
#include "BKE_global.h"
#include "BKE_library.h"

#include "blendef.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifndef M_PI
#define M_PI		3.14159265358979323846
#endif

/* used by object.c */
void Mat4BlendMat4(float [][4], float [][4], float [][4], float );

/* Local function prototypes */

/* ********************* Data level ****************** */

void free_constraint_data (bConstraint *con)
{
	if (con->data){
		switch (con->type){
			default:
				break;
		};
		
		MEM_freeN (con->data);
	}
}

void free_constraints (ListBase *conlist)
{
	bConstraint *con;
	
	/* Do any specific freeing */
	for (con=conlist->first; con; con=con->next) {
		free_constraint_data (con);
	}
	
	/* Free the whole list */
	BLI_freelistN(conlist);
}

void free_constraint_channels (ListBase *chanbase)
{
	bConstraintChannel *chan;
	
	for (chan=chanbase->first; chan; chan=chan->next)
	{
		if (chan->ipo){
			chan->ipo->id.us--;
		}
	}
	
	BLI_freelistN(chanbase);
}

void relink_constraints (struct ListBase *list)
{
	bConstraint *con;
	
	for (con = list->first; con; con=con->next){
		switch (con->type){
			case CONSTRAINT_TYPE_KINEMATIC:
			{
				bKinematicConstraint *data;
				data = con->data;
				
				ID_NEW(data->tar);
			}
				break;
			case CONSTRAINT_TYPE_NULL:
			{
			}
				break;
			case CONSTRAINT_TYPE_TRACKTO:
			{
				bTrackToConstraint *data;
				data = con->data;
				
				ID_NEW(data->tar);
			}
				break;
			case CONSTRAINT_TYPE_MINMAX:
			{
				bMinMaxConstraint *data;
				data = con->data;
				
				ID_NEW(data->tar);
			}
				break;
			case CONSTRAINT_TYPE_LOCKTRACK:
			{
				bLockTrackConstraint *data;
				data = con->data;
				
				ID_NEW(data->tar);
			}
				break;
			case CONSTRAINT_TYPE_ACTION:
			{
				bActionConstraint *data;
				data = con->data;
				
				ID_NEW(data->tar);
			}
				break;
			case CONSTRAINT_TYPE_LOCLIKE:
			{
				bLocateLikeConstraint *data;
				data = con->data;
				
				ID_NEW(data->tar);
			}
				break;
			case CONSTRAINT_TYPE_ROTLIKE:
			{
				bRotateLikeConstraint *data;
				data = con->data;
				
				ID_NEW(data->tar);
			}
				break;
	 		case CONSTRAINT_TYPE_SIZELIKE:
 			{
 				bSizeLikeConstraint *data;
 				data = con->data;
 
 				ID_NEW(data->tar);
 			}
 				break;
			case CONSTRAINT_TYPE_FOLLOWPATH:
			{
				bFollowPathConstraint *data;
				data = con->data;
				
				ID_NEW(data->tar);
			}
				break;
			case CONSTRAINT_TYPE_STRETCHTO:
			{
				bStretchToConstraint *data;
				data = con->data;
				
				ID_NEW(data->tar);
			}
				break;
			case CONSTRAINT_TYPE_LOCLIMIT:
			{
				bLocLimitConstraint *data;
				data = con->data;
			}
				break;
			case CONSTRAINT_TYPE_ROTLIMIT:
			{
				bRotLimitConstraint *data;
				data = con->data;
			}
				break;
			case CONSTRAINT_TYPE_SIZELIMIT:
			{
				bSizeLimitConstraint *data;
				data = con->data;
			}
				break;
		}
	}
}

void copy_constraint_channels (ListBase *dst, ListBase *src)
{
	bConstraintChannel *dchan, *schan;
	
	dst->first=dst->last=NULL;
	duplicatelist(dst, src);
	
	for (dchan=dst->first, schan=src->first; dchan; dchan=dchan->next, schan=schan->next){
		dchan->ipo = copy_ipo(schan->ipo);
	}
}

void clone_constraint_channels (ListBase *dst, ListBase *src)
{
	bConstraintChannel *dchan, *schan;
	
	dst->first=dst->last=NULL;
	duplicatelist(dst, src);
	
	for (dchan=dst->first, schan=src->first; dchan; dchan=dchan->next, schan=schan->next){
		id_us_plus((ID *)dchan->ipo);
	}
}

void copy_constraints (ListBase *dst, ListBase *src)
{
	bConstraint *con;
	
	dst->first= dst->last= NULL;
	
	duplicatelist (dst, src);
	
	for (con = dst->first; con; con=con->next) {
		con->data = MEM_dupallocN (con->data);
		/* removed a whole lot of useless code here (ton) */
	}
}


/* **************** Editor Functions **************** */

char constraint_has_target (bConstraint *con) 
{
	switch (con->type){
	case CONSTRAINT_TYPE_TRACKTO:
		{
			bTrackToConstraint *data = con->data;
			if (data->tar)
				return 1;
		}
		break;
	case CONSTRAINT_TYPE_KINEMATIC:
		{
			bKinematicConstraint *data = con->data;
			if (data->tar)
				return 1;
		}
		break;
	case CONSTRAINT_TYPE_FOLLOWPATH:
		{
			bFollowPathConstraint *data = con->data;
			if (data->tar)
				return 1;
		}
		break;
	case CONSTRAINT_TYPE_ROTLIKE:
		{
			bRotateLikeConstraint *data = con->data;
			if (data->tar)
				return 1;
		}
		break;
	case CONSTRAINT_TYPE_LOCLIKE:
		{
			bLocateLikeConstraint *data = con->data;
			if (data->tar)
				return 1;
		}
		break;
	case CONSTRAINT_TYPE_SIZELIKE:
		{
			bSizeLikeConstraint *data = con->data;
			if (data->tar)
				return 1;
		}
		break;
	case CONSTRAINT_TYPE_MINMAX:
		{
			bMinMaxConstraint *data = con->data;
			if (data->tar)
				return 1;
		}
		break;
	case CONSTRAINT_TYPE_ACTION:
		{
			bActionConstraint *data = con->data;
			if (data->tar)
				return 1;
		}
		break;
	case CONSTRAINT_TYPE_LOCKTRACK:
		{
			bLockTrackConstraint *data = con->data;
			if (data->tar)
				return 1;
		}
	case CONSTRAINT_TYPE_STRETCHTO:
		{
			bStretchToConstraint *data = con->data;
			if (data->tar)
				return 1;
		}
		break;
	}
	// Unknown types or CONSTRAINT_TYPE_NULL or no target
	return 0;
}

Object *get_constraint_target(bConstraint *con, char **subtarget)
{
/*
* If the target for this constraint is target, return a pointer 
* to the name for this constraints subtarget ... NULL otherwise
	*/
	switch (con->type) {
	case CONSTRAINT_TYPE_ACTION:
		{
			bActionConstraint *data = con->data;
			*subtarget= data->subtarget;
			return data->tar;
		}
		break;
	case CONSTRAINT_TYPE_LOCLIKE:
		{
			bLocateLikeConstraint *data = con->data;
			*subtarget= data->subtarget;
			return data->tar;
		}
		break;
	case CONSTRAINT_TYPE_ROTLIKE:
		{
			bRotateLikeConstraint *data = con->data;
			*subtarget= data->subtarget;
			return data->tar;
		}
		break;
	case CONSTRAINT_TYPE_SIZELIKE:
		{
			bSizeLikeConstraint *data = con->data;
			*subtarget= data->subtarget;
			return data->tar;
		}
		break;
	case CONSTRAINT_TYPE_KINEMATIC:
		{
			bKinematicConstraint *data = con->data;
			*subtarget= data->subtarget;
			return data->tar;
		}
		break;
	case CONSTRAINT_TYPE_TRACKTO:
		{
			bTrackToConstraint *data = con->data;
			*subtarget= data->subtarget;
			return data->tar;
		}
		break;
	case CONSTRAINT_TYPE_MINMAX:
		{
			bMinMaxConstraint *data = con->data;
			*subtarget= data->subtarget;
			return data->tar;
		}
		break;
	case CONSTRAINT_TYPE_LOCKTRACK:
		{
			bLockTrackConstraint *data = con->data;
			*subtarget= data->subtarget;
			return data->tar;
		}
		break;
	case CONSTRAINT_TYPE_FOLLOWPATH: 
		{
			bFollowPathConstraint *data = con->data;
			*subtarget= NULL;
			return data->tar;
		}
		break;
	case CONSTRAINT_TYPE_STRETCHTO:
		{
			bStretchToConstraint *data = con->data;
			*subtarget= data->subtarget;
			return (data->tar);
		}
		break;
	default:
		*subtarget= NULL;
		break;
	}
	
	return NULL;  
}

void set_constraint_target(bConstraint *con, Object *ob, char *subtarget)
{
	/*
	 * Set the target for this constraint  
	 */
	switch (con->type) {
		case CONSTRAINT_TYPE_ACTION:
		{
			bActionConstraint *data = con->data;
			data->tar= ob;
			if(subtarget) BLI_strncpy(data->subtarget, subtarget, 32);
		}
			break;
		case CONSTRAINT_TYPE_LOCLIKE:
		{
			bLocateLikeConstraint *data = con->data;
			data->tar= ob;
			if(subtarget) BLI_strncpy(data->subtarget, subtarget, 32);
		}
			break;
		case CONSTRAINT_TYPE_ROTLIKE:
		{
			bRotateLikeConstraint *data = con->data;
			data->tar= ob;
			if(subtarget) BLI_strncpy(data->subtarget, subtarget, 32);
		}
			break;
		case CONSTRAINT_TYPE_SIZELIKE:
		{
			bSizeLikeConstraint *data = con->data;
			data->tar= ob;
			if(subtarget) BLI_strncpy(data->subtarget, subtarget, 32);
		}
			break;
		case CONSTRAINT_TYPE_KINEMATIC:
		{
			bKinematicConstraint *data = con->data;
			data->tar= ob;
			if(subtarget) BLI_strncpy(data->subtarget, subtarget, 32);
		}
			break;
		case CONSTRAINT_TYPE_TRACKTO:
		{
			bTrackToConstraint *data = con->data;
			data->tar= ob;
			if(subtarget) BLI_strncpy(data->subtarget, subtarget, 32);
		}
			break;
		case CONSTRAINT_TYPE_LOCKTRACK:
		{
			bLockTrackConstraint *data = con->data;
			data->tar= ob;
			if(subtarget) BLI_strncpy(data->subtarget, subtarget, 32);
		}
			break;
		case CONSTRAINT_TYPE_FOLLOWPATH: 
		{
			bFollowPathConstraint *data = con->data;
			data->tar= ob;
		}
			break;
		case CONSTRAINT_TYPE_STRETCHTO:
		{
			bStretchToConstraint *data = con->data;
			data->tar= ob;
			if(subtarget) BLI_strncpy(data->subtarget, subtarget, 32);
		}
			break;
		case CONSTRAINT_TYPE_MINMAX:
		{
			bMinMaxConstraint *data = (bMinMaxConstraint*)con->data;
			data->tar= ob;
			if(subtarget) BLI_strncpy(data->subtarget, subtarget, 32);
		}
			break;
	}
}

void unique_constraint_name (bConstraint *con, ListBase *list)
{
	char		tempname[64];
	int			number;
	char		*dot;
	int exists = 0;
	bConstraint *curcon;
	
	/* See if we even need to do this */
	for (curcon = list->first; curcon; curcon=curcon->next){
		if (curcon!=con){
			if (!strcmp(curcon->name, con->name)){
				exists = 1;
				break;
			}
		}
	}
	
	if (!exists)
		return;

	/*	Strip off the suffix */
	dot=strchr(con->name, '.');
	if (dot)
		*dot=0;
	
	for (number = 1; number <=999; number++){
		sprintf (tempname, "%s.%03d", con->name, number);
		
		exists = 0;
		for (curcon=list->first; curcon; curcon=curcon->next){
			if (con!=curcon){
				if (!strcmp (curcon->name, tempname)){
					exists = 1;
					break;
				}
			}
		}
		if (!exists){
			strcpy (con->name, tempname);
			return;
		}
	}
}

void *new_constraint_data (short type)
{
	void *result;
	
	switch (type){
	case CONSTRAINT_TYPE_KINEMATIC:
		{
			bKinematicConstraint *data;
			data = MEM_callocN(sizeof(bKinematicConstraint), "kinematicConstraint");

			data->tolerance = (float)0.001;
			data->weight= (float)1.0;
			data->iterations = 500;
			data->flag= CONSTRAINT_IK_TIP;
			
			result = data;
		}
		break;
	case CONSTRAINT_TYPE_NULL:
		{
			result = NULL;
		}
		break;
	case CONSTRAINT_TYPE_TRACKTO:
		{
			bTrackToConstraint *data;
			data = MEM_callocN(sizeof(bTrackToConstraint), "tracktoConstraint");


			data->reserved1 = TRACK_Y;
			data->reserved2 = UP_Z;

			result = data;

		}
		break;
	case CONSTRAINT_TYPE_MINMAX:
		{
			bMinMaxConstraint *data;
			data = MEM_callocN(sizeof(bMinMaxConstraint), "minmaxConstraint");


			data->minmaxflag = TRACK_Z;
			data->offset = 0.0f;
			data->cache[0] = data->cache[1] = data->cache[2] = 0.0f;
			data->flag = 0;

			result = data;

		}
		break;
	case CONSTRAINT_TYPE_ROTLIKE:
		{
			bRotateLikeConstraint *data;
			data = MEM_callocN(sizeof(bRotateLikeConstraint), "rotlikeConstraint");
			data->flag = ROTLIKE_X|ROTLIKE_Y|ROTLIKE_Z;
			result = data;
		}
		break;
	case CONSTRAINT_TYPE_LOCLIKE:
		{
			bLocateLikeConstraint *data;
			data = MEM_callocN(sizeof(bLocateLikeConstraint), "loclikeConstraint");
			data->flag = LOCLIKE_X|LOCLIKE_Y|LOCLIKE_Z;
			result = data;
		}
		break;
	case CONSTRAINT_TYPE_SIZELIKE:
		{
			bSizeLikeConstraint *data;
			data = MEM_callocN(sizeof(bLocateLikeConstraint), "sizelikeConstraint");

			data->flag |= SIZELIKE_X|SIZELIKE_Y|SIZELIKE_Z;
			result = data;
		}
		break;
	case CONSTRAINT_TYPE_ACTION:
		{
			bActionConstraint *data;
			data = MEM_callocN(sizeof(bActionConstraint), "actionConstraint");
			data->local= 1;
			
			result = data;
		}
		break;
	case CONSTRAINT_TYPE_LOCKTRACK:
		{
			bLockTrackConstraint *data;
			data = MEM_callocN(sizeof(bLockTrackConstraint), "locktrackConstraint");

			data->trackflag = TRACK_Y;
			data->lockflag = LOCK_Z;

			result = data;
		}
		break;
	case CONSTRAINT_TYPE_FOLLOWPATH:
		{
			bFollowPathConstraint *data;
			data = MEM_callocN(sizeof(bFollowPathConstraint), "followpathConstraint");

			data->trackflag = TRACK_Y;
			data->upflag = UP_Z;
			data->offset = 0;
			data->followflag = 0;

			result = data;
		}
		break;
	case CONSTRAINT_TYPE_STRETCHTO:
		{
			bStretchToConstraint *data;
			data = MEM_callocN(sizeof(bStretchToConstraint), "StretchToConstraint");

			data->volmode = 0;
			data->plane = 0;
			data->orglength = 0.0; 
			data->bulge = 1.0;
			result = data;
		}
		break; 
	case CONSTRAINT_TYPE_LOCLIMIT:
		{
			bLocLimitConstraint *data;
			data = MEM_callocN(sizeof(bLocLimitConstraint), "LocLimitConstraint");
			
			data->flag = 0;
			data->flag2 = 0;
			data->xmin = 0.0f;
			data->xmax = 0.0f;
			data->ymin = 0.0f;
			data->ymax = 0.0f;
			data->zmin = 0.0f;
			data->zmax = 0.0f;
			
			result = data;
		}
		break;
	case CONSTRAINT_TYPE_ROTLIMIT:
		{
			bRotLimitConstraint *data;
			data = MEM_callocN(sizeof(bRotLimitConstraint), "RotLimitConstraint");
			
			data->flag = 0;
			data->xmin = 0.0f;
			data->xmax = 0.0f;
			data->ymin = 0.0f;
			data->ymax = 0.0f;
			data->zmin = 0.0f;
			data->zmax = 0.0f;
			
			result = data;
		}
		break;
	case CONSTRAINT_TYPE_SIZELIMIT:
		{
			bSizeLimitConstraint *data;
			data = MEM_callocN(sizeof(bSizeLimitConstraint), "SizeLimitConstraint");
			
			data->flag = 0;
			data->xmin = 0.0f;
			data->xmax = 0.0f;
			data->ymin = 0.0f;
			data->ymax = 0.0f;
			data->zmin = 0.0f;
			data->zmax = 0.0f;
			
			result = data;
		}
		break;
	default:
		result = NULL;
		break;
	}

	return result;
}

bConstraintChannel *get_constraint_channel (ListBase *list, const char *name)
{
	bConstraintChannel *chan;

	for (chan = list->first; chan; chan=chan->next) {
		if (!strcmp(name, chan->name)) {
			return chan;
		}
	}
	return NULL;
}

/* finds or creates new constraint channel */
bConstraintChannel *verify_constraint_channel (ListBase *list, const char *name)
{
	bConstraintChannel *chan;
	
	chan= get_constraint_channel (list, name);
	if(chan==NULL) {
		chan= MEM_callocN(sizeof(bConstraintChannel), "new constraint chan");
		BLI_addtail(list, chan);
		strcpy(chan->name, name);
	}
	
	return chan;
}


/* ***************** Evaluating ********************* */

/* does ipos only */
void do_constraint_channels (ListBase *conbase, ListBase *chanbase, float ctime)
{
	bConstraint *con;
	bConstraintChannel *chan;
	IpoCurve *icu=NULL;
	
	for (con=conbase->first; con; con=con->next) {
		chan = get_constraint_channel(chanbase, con->name);
		if (chan && chan->ipo){
			calc_ipo(chan->ipo, ctime);
			for (icu=chan->ipo->curve.first; icu; icu=icu->next){
				switch (icu->adrcode){
				case CO_ENFORCE:
					con->enforce = icu->curval;
					if (con->enforce<0.0f) con->enforce= 0.0f;
					else if (con->enforce>1.0f) con->enforce= 1.0f;
					break;
				}
			}
		}
	}
}

void Mat4BlendMat4(float out[][4], float dst[][4], float src[][4], float srcweight)
{
	float squat[4], dquat[4], fquat[4];
	float ssize[3], dsize[3], fsize[4];
	float sloc[3], dloc[3], floc[3];
	float mat3[3][3], dstweight;
	float qmat[3][3], smat[3][3];
	int i;

	dstweight = 1.0F-srcweight;

	Mat3CpyMat4(mat3, dst);
	Mat3ToQuat(mat3, dquat);
	Mat3ToSize(mat3, dsize);
	VECCOPY (dloc, dst[3]);

	Mat3CpyMat4(mat3, src);
	Mat3ToQuat(mat3, squat);
	Mat3ToSize(mat3, ssize);
	VECCOPY (sloc, src[3]);
	
	/* Do the actual blend */
	for (i=0; i<3; i++){
		floc[i] = (dloc[i]*dstweight) + (sloc[i]*srcweight);
		fsize[i] = 1.0f + ((dsize[i]-1.0f)*dstweight) + ((ssize[i]-1.0f)*srcweight);
		fquat[i+1] = (dquat[i+1]*dstweight) + (squat[i+1]*srcweight);
	}
	
	/* Do one more iteration for the quaternions only and normalize the quaternion if needed */
	fquat[0] = 1.0f + ((dquat[0]-1.0f)*dstweight) + ((squat[0]-1.0f)*srcweight);
	NormalQuat (fquat);

	QuatToMat3(fquat, qmat);
	SizeToMat3(fsize, smat);

	Mat3MulMat3(mat3, qmat, smat);
	Mat4CpyMat3(out, mat3);
	VECCOPY (out[3], floc);
}

static void constraint_target_to_mat4 (Object *ob, const char *substring, float mat[][4], float size[3])
{

	/*	Case OBJECT */
	if (!strlen(substring)) {
		Mat4CpyMat4 (mat, ob->obmat);
		VECCOPY (size, ob->size);  // whats this for, hack! (ton)
	}
	/*	Case BONE */
	else {
		bPoseChannel *pchan;
		float	bsize[3]={1, 1, 1};

		pchan = get_pose_channel(ob->pose, substring);
		if (pchan){
			/**
			 *	Multiply the objectspace bonematrix by the skeletons's global
			 *	transform to obtain the worldspace transformation of the target
			 */
			Mat4MulMat4 (mat, pchan->pose_mat, ob->obmat);
		} 
		else
			Mat4CpyMat4 (mat, ob->obmat);

		VECCOPY(size, bsize);   // whats this for, hack! (ton)
	}
}

/* called during solve_constraints */
/* also for make_parent, to find correct inverse of "follow path" */
/* warning, ownerdata is void... is not Bone anymore, but PoseChannel or Object */
/* ctime is global time, uncorrected for local bsystem_time */
short get_constraint_target_matrix (bConstraint *con, short ownertype, void* ownerdata, float mat[][4], float size[3], float ctime)
{
	short valid=0;

	switch (con->type){
	case CONSTRAINT_TYPE_NULL:
		{
			Mat4One(mat);
		}
		break;
	case CONSTRAINT_TYPE_ACTION:
		{
			if (ownertype == TARGET_BONE) {
				extern void chan_calc_mat(bPoseChannel *chan);
				bActionConstraint *data = (bActionConstraint*)con->data;
				bPose *pose;
				bPoseChannel *pchan, *tchan;
				float tempmat3[3][3];
				float eul[3];
				float s,t;
				
				Mat4One(mat);	// return mat
				
				if (data->tar==NULL) return 0;
				
				/* need proper check for bone... */
				if(data->subtarget[0]) {
					pchan = get_pose_channel(data->tar->pose, data->subtarget);
					if (pchan) {
						float arm_mat[3][3], pose_mat[3][3];		/* arm mat should be bone mat! bug... */
						
						Mat3CpyMat4(arm_mat, pchan->bone->arm_mat);
						Mat3CpyMat4(pose_mat, pchan->pose_mat);
						
						/* new; true local rotation constraint */
						if(data->local) {
							float diff_mat[3][3], par_mat[3][3], ipar_mat[3][3];
							/* we need the local rotation = current rotation - (parent rotation + restpos) */
							
							if (pchan->parent) {
								Mat3CpyMat4(par_mat, pchan->parent->pose_mat);
								Mat3MulMat3(diff_mat, par_mat, arm_mat);
								
								Mat3Inv(ipar_mat, diff_mat);
							}
							else {
								Mat3Inv(ipar_mat, arm_mat);
							}
							
							Mat3MulMat3(tempmat3, ipar_mat, pose_mat);
						}
						else {	/* we use the deform mat, for backwards compatibility */
							float imat[3][3];
							
							Mat3Inv(imat, arm_mat);
							Mat3MulMat3(tempmat3, pose_mat, imat);
						}
					}
					else Mat3One(tempmat3);
				}
				else {
					float ans[4][4];
					
					constraint_target_to_mat4(data->tar, data->subtarget, ans, size);
					/* extract rotation, is in global world coordinates */
					Mat3CpyMat4(tempmat3, ans);
				}
				
				Mat3ToEul(tempmat3, eul);
				eul[0]*=(float)(180.0/M_PI);
				eul[1]*=(float)(180.0/M_PI);
				eul[2]*=(float)(180.0/M_PI);
				
				/* Target defines the animation */
				s = (eul[data->type]-data->min)/(data->max-data->min);
				if (s<0)
					s=0;
				if (s>1)
					s=1;

				t = ( s * (data->end-data->start)) + data->start;

				/* Get the appropriate information from the action, we make temp pose */
				pose = MEM_callocN(sizeof(bPose), "pose");
				
				pchan = ownerdata;
				tchan= verify_pose_channel(pose, pchan->name);
				extract_pose_from_action (pose, data->act, t);
				
				chan_calc_mat(tchan);
				
				Mat4CpyMat4(mat, tchan->chan_mat);

				/* Clean up */
				free_pose_channels(pose);
				MEM_freeN(pose);
			}
			
		}
		break;
	case CONSTRAINT_TYPE_LOCLIKE:
		{
			bLocateLikeConstraint *data = (bLocateLikeConstraint*)con->data;

			if (data->tar){
				constraint_target_to_mat4(data->tar, data->subtarget, mat, size);
				valid=1;
			}
			else
				Mat4One (mat);
		} 
		break;
	case CONSTRAINT_TYPE_MINMAX:
		{
			bMinMaxConstraint *data = (bMinMaxConstraint*)con->data;

			if (data->tar){
				constraint_target_to_mat4(data->tar, data->subtarget, mat, size);
				valid=1;
			}
			else
				Mat4One (mat);
		} 
		break;
	case CONSTRAINT_TYPE_ROTLIKE:
		{
			bRotateLikeConstraint *data;
			data = (bRotateLikeConstraint*)con->data;

			if (data->tar){
				constraint_target_to_mat4(data->tar, data->subtarget, mat, size);
				valid=1;
			}
			else
				Mat4One (mat);
		} 
		break;
	case CONSTRAINT_TYPE_SIZELIKE:
		{
			bSizeLikeConstraint *data;
			data = (bSizeLikeConstraint*)con->data;

			if (data->tar){
				constraint_target_to_mat4(data->tar, data->subtarget, mat, size);
				valid=1;
			}
			else
				Mat4One (mat);
		} 
		break;
	case CONSTRAINT_TYPE_TRACKTO:
		{
			bTrackToConstraint *data;
			data = (bTrackToConstraint*)con->data;

			if (data->tar){
				constraint_target_to_mat4(data->tar, data->subtarget, mat, size);
				valid=1;
			}
			else
				Mat4One (mat);
		}
		break;
	case CONSTRAINT_TYPE_KINEMATIC:
		{
			bKinematicConstraint *data;
			data = (bKinematicConstraint*)con->data;

			if (data->tar){
				constraint_target_to_mat4(data->tar, data->subtarget, mat, size);
				valid=1;
			}
			else if (data->flag & CONSTRAINT_IK_AUTO) {
				Object *ob= ownerdata;
				
				if(ob==NULL)
					Mat4One(mat);
				else {
					float vec[3];
					/* move grabtarget into world space */
					VECCOPY(vec, data->grabtarget);
					Mat4MulVecfl(ob->obmat, vec);
					Mat4CpyMat4(mat, ob->obmat);
					VECCOPY(mat[3], vec);
				}
			}
			else
				Mat4One (mat);
		} 
		break;
	case CONSTRAINT_TYPE_LOCKTRACK:
		{
			bLockTrackConstraint *data;
			data = (bLockTrackConstraint*)con->data;

			if (data->tar){
				constraint_target_to_mat4(data->tar, data->subtarget, mat, size);
				valid=1;
			}
			else
				Mat4One (mat);
		} 
		break;
	case CONSTRAINT_TYPE_FOLLOWPATH:
		{
			bFollowPathConstraint *data;
			data = (bFollowPathConstraint*)con->data;

			if (data->tar){
				Curve *cu;
				float q[4], vec[4], dir[3], *quat, x1, totmat[4][4];
				float curvetime;

				Mat4One (totmat);
				Mat4One (mat);

				cu= data->tar->data;

				/* note; when creating constraints that follow path, the curve gets the CU_PATH set now,
					currently for paths to work it needs to go through the bevlist/displist system (ton) */
				
				if(cu->path==NULL || cu->path->data==NULL) /* only happens on reload file, but violates depsgraph still... fix! */
					makeDispListCurveTypes(data->tar, 0);
				if(cu->path && cu->path->data) {
					
					curvetime= bsystem_time(data->tar, data->tar->parent, (float)ctime, 0.0) - data->offset;

					if(calc_ipo_spec(cu->ipo, CU_SPEED, &curvetime)==0) {
						curvetime /= cu->pathlen;
						CLAMP(curvetime, 0.0, 1.0);
					}

					if(where_on_path(data->tar, curvetime, vec, dir) ) {

						if(data->followflag){
							quat= vectoquat(dir, (short) data->trackflag, (short) data->upflag);

							Normalise(dir);
							q[0]= (float)cos(0.5*vec[3]);
							x1= (float)sin(0.5*vec[3]);
							q[1]= -x1*dir[0];
							q[2]= -x1*dir[1];
							q[3]= -x1*dir[2];
							QuatMul(quat, q, quat);
							

							QuatToMat4(quat, totmat);
						}
						VECCOPY(totmat[3], vec);

						Mat4MulSerie(mat, data->tar->obmat, totmat, NULL, NULL, NULL, NULL, NULL, NULL);
					}
				}
				valid=1;
			}
			else
				Mat4One (mat);
		}
		break;
	case CONSTRAINT_TYPE_STRETCHTO:
		{
			bStretchToConstraint *data;
			data = (bStretchToConstraint*)con->data;

			if (data->tar){
				constraint_target_to_mat4(data->tar, data->subtarget, mat, size);
				valid = 1;
			}
			else
				Mat4One (mat);
		}
		break;

	default:
		Mat4One(mat);
		break;
	}

	return valid;
}


/* only called during solve_constraints */
/* bone constraints create a fake object to work on, then ob is a workob */
/* if ownerdata is set, it's the posechannel */
void evaluate_constraint (bConstraint *constraint, Object *ob, short ownertype, void *ownerdata, float targetmat[][4])
{
	float	M_oldmat[4][4];
	float	M_identity[4][4];
	
	if (!constraint || !ob)
		return;

	Mat4One (M_identity);
	
	switch (constraint->type){
	case CONSTRAINT_TYPE_ACTION:
		{
			float temp[4][4];
			bActionConstraint *data;
			
			data = constraint->data;
			Mat4CpyMat4 (temp, ob->obmat);

			Mat4MulMat4(ob->obmat, targetmat, temp);
		}
		break;
	case CONSTRAINT_TYPE_LOCLIKE:
		{
			bLocateLikeConstraint *data;

			data = constraint->data;
			
			if (data->flag & LOCLIKE_X)
				ob->obmat[3][0] = targetmat[3][0];
			if (data->flag & LOCLIKE_Y)
				ob->obmat[3][1] = targetmat[3][1];
			if (data->flag & LOCLIKE_Z)
				ob->obmat[3][2] = targetmat[3][2];
		}
		break;
	case CONSTRAINT_TYPE_ROTLIKE:
		{
			bRotateLikeConstraint *data;
			float	tmat[3][3];
			float	size[3];
			
			data = constraint->data;
			
			/* old files stuff only... version patch is too much code! */
			if(data->flag==0) data->flag = ROTLIKE_X|ROTLIKE_Y|ROTLIKE_Z;

			Mat4ToSize(ob->obmat, size);
			
			Mat3CpyMat4 (tmat, targetmat);
			Mat3Ortho(tmat);
			
			if(data->flag != (ROTLIKE_X|ROTLIKE_Y|ROTLIKE_Z)) {
				float obeul[3], eul[3], obmat[3][3];
				
				Mat3ToEul(tmat, eul);
				Mat3CpyMat4(obmat, ob->obmat);
				Mat3ToEul(obmat, obeul);
				if(!(data->flag & ROTLIKE_X)) eul[0]= obeul[0];
				if(!(data->flag & ROTLIKE_Y)) eul[1]= obeul[1];
				if(!(data->flag & ROTLIKE_Z)) eul[2]= obeul[2];
				compatible_eul(eul, obeul);
				EulToMat3(eul, tmat);
			}

			ob->obmat[0][0] = tmat[0][0]*size[0];
			ob->obmat[0][1] = tmat[0][1]*size[1];
			ob->obmat[0][2] = tmat[0][2]*size[2];

			ob->obmat[1][0] = tmat[1][0]*size[0];
			ob->obmat[1][1] = tmat[1][1]*size[1];
			ob->obmat[1][2] = tmat[1][2]*size[2];

			ob->obmat[2][0] = tmat[2][0]*size[0];
			ob->obmat[2][1] = tmat[2][1]*size[1];
			ob->obmat[2][2] = tmat[2][2]*size[2];
		}
		break;
 	case CONSTRAINT_TYPE_SIZELIKE:
 		{
 			float obsize[3], size[3];
 			bSizeLikeConstraint *data;
 
 			data = constraint->data;
 
 			Mat4ToSize(targetmat, size);
 			Mat4ToSize(ob->obmat, obsize);
 			
 			if (data->flag & SIZELIKE_X && obsize[0] != 0)
 				VecMulf(ob->obmat[0], size[0] / obsize[0]);
 			if (data->flag & SIZELIKE_Y && obsize[1] != 0)
 				VecMulf(ob->obmat[1], size[1] / obsize[1]);
 			if (data->flag & SIZELIKE_Z && obsize[2] != 0)
 				VecMulf(ob->obmat[2], size[2] / obsize[2]);
  		}
  		break;
	case CONSTRAINT_TYPE_NULL:
		{
		}
		break;
	case CONSTRAINT_TYPE_MINMAX:
		{
			float val1, val2;
			int index;
			bMinMaxConstraint *data;
			float obmat[4][4],imat[4][4],tarmat[4][4],tmat[4][4];

			data = constraint->data;
			
			Mat4CpyMat4(obmat,ob->obmat);
			Mat4CpyMat4(tarmat,targetmat);
			
			if (data->flag&MINMAX_USEROT) {
			/* take rotation of target into account by doing the transaction in target's localspace */
				Mat4Invert(imat,tarmat);
				Mat4MulMat4(tmat,obmat,imat);
				Mat4CpyMat4(obmat,tmat);
				Mat4One(tarmat);
			}

			switch (data->minmaxflag){
			case TRACK_Z:
				val1 = tarmat[3][2];
				val2 = obmat[3][2]-data->offset;
				index = 2;
				break;
			case TRACK_Y:
				val1 = tarmat[3][1];
				val2 = obmat[3][1]-data->offset;
				index = 1;
				break;
			case TRACK_X:
				val1 = tarmat[3][0];
				val2 = obmat[3][0]-data->offset;
				index = 0;
				break;
			case TRACK_nZ:
				val2 = tarmat[3][2];
				val1 = obmat[3][2]-data->offset;
				index = 2;
				break;
			case TRACK_nY:
				val2 = tarmat[3][1];
				val1 = obmat[3][1]-data->offset;
				index = 1;
				break;
			case TRACK_nX:
				val2 = tarmat[3][0];
				val1 = obmat[3][0]-data->offset;
				index = 0;
				break;
			default:
				return;
			}
			
			if (val1 > val2) {
				obmat[3][index] = tarmat[3][index] + data->offset;
				if (data->flag&MINMAX_STICKY) {
					if (data->flag&MINMAX_STUCK) {
						VECCOPY(obmat[3], data->cache);
					} else {
						VECCOPY(data->cache, obmat[3]);
						data->flag|=MINMAX_STUCK;
					}
				}
				if (data->flag&MINMAX_USEROT) {
					/* get out of localspace */
					Mat4MulMat4(tmat,obmat,targetmat);
					Mat4CpyMat4(ob->obmat,tmat);
				} else {			
					VECCOPY(ob->obmat[3],obmat[3]);
				}

			} else {
				data->flag&=~MINMAX_STUCK;
			}
			
		}
		break;
	case CONSTRAINT_TYPE_TRACKTO:
		{
			bTrackToConstraint *data;
			float size[3];
			float *quat;
			float vec[3];
			float totmat[3][3];
			float tmat[4][4];

			data=(bTrackToConstraint*)constraint->data;			
			
			if (data->tar){
					
				/* Get size property, since ob->size is only the object's own relative size, not its global one */
				Mat4ToSize (ob->obmat, size);
	
				Mat4CpyMat4 (M_oldmat, ob->obmat);

				// Clear the object's rotation 	
				ob->obmat[0][0]=size[0];
				ob->obmat[0][1]=0;
				ob->obmat[0][2]=0;
				ob->obmat[1][0]=0;
				ob->obmat[1][1]=size[1];
				ob->obmat[1][2]=0;
				ob->obmat[2][0]=0;
				ob->obmat[2][1]=0;
				ob->obmat[2][2]=size[2];
	
			
				VecSubf(vec, ob->obmat[3], targetmat[3]);
				quat= vectoquat(vec, (short)data->reserved1, (short)data->reserved2);
				QuatToMat3(quat, totmat);

				Mat4CpyMat4(tmat, ob->obmat);
				
				Mat4MulMat34(ob->obmat, totmat, tmat);
			}
		}
		break;
	case CONSTRAINT_TYPE_KINEMATIC:
		{
			/* removed */
		}
		break;
	case CONSTRAINT_TYPE_LOCKTRACK:
		{
			bLockTrackConstraint *data;
			float vec[3],vec2[3];
			float totmat[3][3];
			float tmpmat[3][3];
			float invmat[3][3];
			float tmat[4][4];
			float mdet;


			data=(bLockTrackConstraint*)constraint->data;			

			if (data->tar){
	
				Mat4CpyMat4 (M_oldmat, ob->obmat);

				/* Vector object -> target */
				VecSubf(vec, targetmat[3], ob->obmat[3]);
				switch (data->lockflag){
				case LOCK_X: /* LOCK X */
					{
					switch (data->trackflag){
					case TRACK_Y: /* LOCK X TRACK Y */
						{
						/* Projection of Vector on the plane */
						Projf(vec2, vec, ob->obmat[0]);
						VecSubf(totmat[1], vec, vec2);
						Normalise(totmat[1]);

						/* the x axis is fixed*/
						totmat[0][0] = ob->obmat[0][0];
						totmat[0][1] = ob->obmat[0][1];
						totmat[0][2] = ob->obmat[0][2];
						Normalise(totmat[0]);
				
						/* the z axis gets mapped onto
						a third orthogonal vector */
						Crossf(totmat[2], totmat[0], totmat[1]);
						}
						break;
					case TRACK_Z: /* LOCK X TRACK Z */
						{
						/* Projection of Vector on the plane */
						Projf(vec2, vec, ob->obmat[0]);
						VecSubf(totmat[2], vec, vec2);
						Normalise(totmat[2]);

						/* the x axis is fixed*/
						totmat[0][0] = ob->obmat[0][0];
						totmat[0][1] = ob->obmat[0][1];
						totmat[0][2] = ob->obmat[0][2];
						Normalise(totmat[0]);
				
						/* the z axis gets mapped onto
						a third orthogonal vector */
						Crossf(totmat[1], totmat[2], totmat[0]);
						}
						break;
					case TRACK_nY: /* LOCK X TRACK -Y */
						{
						/* Projection of Vector on the plane */
						Projf(vec2, vec, ob->obmat[0]);
						VecSubf(totmat[1], vec, vec2);
						Normalise(totmat[1]);
						VecMulf(totmat[1],-1);

						/* the x axis is fixed*/
						totmat[0][0] = ob->obmat[0][0];
						totmat[0][1] = ob->obmat[0][1];
						totmat[0][2] = ob->obmat[0][2];
						Normalise(totmat[0]);
				
						/* the z axis gets mapped onto
						a third orthogonal vector */
						Crossf(totmat[2], totmat[0], totmat[1]);
						}
						break;
					case TRACK_nZ: /* LOCK X TRACK -Z */
						{
						/* Projection of Vector on the plane */
						Projf(vec2, vec, ob->obmat[0]);
						VecSubf(totmat[2], vec, vec2);
						Normalise(totmat[2]);
						VecMulf(totmat[2],-1);

						/* the x axis is fixed*/
						totmat[0][0] = ob->obmat[0][0];
						totmat[0][1] = ob->obmat[0][1];
						totmat[0][2] = ob->obmat[0][2];
						Normalise(totmat[0]);
				
						/* the z axis gets mapped onto
						a third orthogonal vector */
						Crossf(totmat[1], totmat[2], totmat[0]);
						}
						break;
					default:
						{
							totmat[0][0] = 1;totmat[0][1] = 0;totmat[0][2] = 0;
							totmat[1][0] = 0;totmat[1][1] = 1;totmat[1][2] = 0;
							totmat[2][0] = 0;totmat[2][1] = 0;totmat[2][2] = 1;
						}
						break;
					}
					}
					break;
				case LOCK_Y: /* LOCK Y */
					{
					switch (data->trackflag){
					case TRACK_X: /* LOCK Y TRACK X */
						{
						/* Projection of Vector on the plane */
						Projf(vec2, vec, ob->obmat[1]);
						VecSubf(totmat[0], vec, vec2);
						Normalise(totmat[0]);

						/* the y axis is fixed*/
						totmat[1][0] = ob->obmat[1][0];
						totmat[1][1] = ob->obmat[1][1];
						totmat[1][2] = ob->obmat[1][2];
						Normalise(totmat[1]);
						
						/* the z axis gets mapped onto
						a third orthogonal vector */
						Crossf(totmat[2], totmat[0], totmat[1]);
						}
						break;
					case TRACK_Z: /* LOCK Y TRACK Z */
						{
						/* Projection of Vector on the plane */
						Projf(vec2, vec, ob->obmat[1]);
						VecSubf(totmat[2], vec, vec2);
						Normalise(totmat[2]);

						/* the y axis is fixed*/
						totmat[1][0] = ob->obmat[1][0];
						totmat[1][1] = ob->obmat[1][1];
						totmat[1][2] = ob->obmat[1][2];
						Normalise(totmat[1]);
						
						/* the z axis gets mapped onto
						a third orthogonal vector */
						Crossf(totmat[0], totmat[1], totmat[2]);
						}
						break;
					case TRACK_nX: /* LOCK Y TRACK -X */
						{
						/* Projection of Vector on the plane */
						Projf(vec2, vec, ob->obmat[1]);
						VecSubf(totmat[0], vec, vec2);
						Normalise(totmat[0]);
						VecMulf(totmat[0],-1);

						/* the y axis is fixed*/
						totmat[1][0] = ob->obmat[1][0];
						totmat[1][1] = ob->obmat[1][1];
						totmat[1][2] = ob->obmat[1][2];
						Normalise(totmat[1]);
						
						/* the z axis gets mapped onto
						a third orthogonal vector */
						Crossf(totmat[2], totmat[0], totmat[1]);
						}
						break;
					case TRACK_nZ: /* LOCK Y TRACK -Z */
						{
						/* Projection of Vector on the plane */
						Projf(vec2, vec, ob->obmat[1]);
						VecSubf(totmat[2], vec, vec2);
						Normalise(totmat[2]);
						VecMulf(totmat[2],-1);

						/* the y axis is fixed*/
						totmat[1][0] = ob->obmat[1][0];
						totmat[1][1] = ob->obmat[1][1];
						totmat[1][2] = ob->obmat[1][2];
						Normalise(totmat[1]);
						
						/* the z axis gets mapped onto
						a third orthogonal vector */
						Crossf(totmat[0], totmat[1], totmat[2]);
						}
						break;
					default:
						{
							totmat[0][0] = 1;totmat[0][1] = 0;totmat[0][2] = 0;
							totmat[1][0] = 0;totmat[1][1] = 1;totmat[1][2] = 0;
							totmat[2][0] = 0;totmat[2][1] = 0;totmat[2][2] = 1;
						}
						break;
					}
					}
					break;
				case LOCK_Z: /* LOCK Z */
					{
					switch (data->trackflag){
					case TRACK_X: /* LOCK Z TRACK X */
						{
						/* Projection of Vector on the plane */
						Projf(vec2, vec, ob->obmat[2]);
						VecSubf(totmat[0], vec, vec2);
						Normalise(totmat[0]);

						/* the z axis is fixed*/
						totmat[2][0] = ob->obmat[2][0];
						totmat[2][1] = ob->obmat[2][1];
						totmat[2][2] = ob->obmat[2][2];
						Normalise(totmat[2]);
						
						/* the x axis gets mapped onto
						a third orthogonal vector */
						Crossf(totmat[1], totmat[2], totmat[0]);
						}
						break;
					case TRACK_Y: /* LOCK Z TRACK Y */
						{
						/* Projection of Vector on the plane */
						Projf(vec2, vec, ob->obmat[2]);
						VecSubf(totmat[1], vec, vec2);
						Normalise(totmat[1]);

						/* the z axis is fixed*/
						totmat[2][0] = ob->obmat[2][0];
						totmat[2][1] = ob->obmat[2][1];
						totmat[2][2] = ob->obmat[2][2];
						Normalise(totmat[2]);
						
						/* the x axis gets mapped onto
						a third orthogonal vector */
						Crossf(totmat[0], totmat[1], totmat[2]);
						}
						break;
					case TRACK_nX: /* LOCK Z TRACK -X */
						{
						/* Projection of Vector on the plane */
						Projf(vec2, vec, ob->obmat[2]);
						VecSubf(totmat[0], vec, vec2);
						Normalise(totmat[0]);
						VecMulf(totmat[0],-1);

						/* the z axis is fixed*/
						totmat[2][0] = ob->obmat[2][0];
						totmat[2][1] = ob->obmat[2][1];
						totmat[2][2] = ob->obmat[2][2];
						Normalise(totmat[2]);
						
						/* the x axis gets mapped onto
						a third orthogonal vector */
						Crossf(totmat[1], totmat[2], totmat[0]);
						}
						break;
					case TRACK_nY: /* LOCK Z TRACK -Y */
						{
						/* Projection of Vector on the plane */
						Projf(vec2, vec, ob->obmat[2]);
						VecSubf(totmat[1], vec, vec2);
						Normalise(totmat[1]);
						VecMulf(totmat[1],-1);

						/* the z axis is fixed*/
						totmat[2][0] = ob->obmat[2][0];
						totmat[2][1] = ob->obmat[2][1];
						totmat[2][2] = ob->obmat[2][2];
						Normalise(totmat[2]);
						
						/* the x axis gets mapped onto
						a third orthogonal vector */
						Crossf(totmat[0], totmat[1], totmat[2]);
						}
						break;
					default:
						{
							totmat[0][0] = 1;totmat[0][1] = 0;totmat[0][2] = 0;
							totmat[1][0] = 0;totmat[1][1] = 1;totmat[1][2] = 0;
							totmat[2][0] = 0;totmat[2][1] = 0;totmat[2][2] = 1;
						}
						break;
					}
					}
					break;
				default:
					{
						totmat[0][0] = 1;totmat[0][1] = 0;totmat[0][2] = 0;
						totmat[1][0] = 0;totmat[1][1] = 1;totmat[1][2] = 0;
						totmat[2][0] = 0;totmat[2][1] = 0;totmat[2][2] = 1;
					}
					break;
				}
				/* Block to keep matrix heading */
				tmpmat[0][0] = ob->obmat[0][0];tmpmat[0][1] = ob->obmat[0][1];tmpmat[0][2] = ob->obmat[0][2];
				tmpmat[1][0] = ob->obmat[1][0];tmpmat[1][1] = ob->obmat[1][1];tmpmat[1][2] = ob->obmat[1][2];
				tmpmat[2][0] = ob->obmat[2][0];tmpmat[2][1] = ob->obmat[2][1];tmpmat[2][2] = ob->obmat[2][2];
				Normalise(tmpmat[0]);
				Normalise(tmpmat[1]);
				Normalise(tmpmat[2]);
				Mat3Inv(invmat,tmpmat);
				Mat3MulMat3(tmpmat,totmat,invmat);
				totmat[0][0] = tmpmat[0][0];totmat[0][1] = tmpmat[0][1];totmat[0][2] = tmpmat[0][2];
				totmat[1][0] = tmpmat[1][0];totmat[1][1] = tmpmat[1][1];totmat[1][2] = tmpmat[1][2];
				totmat[2][0] = tmpmat[2][0];totmat[2][1] = tmpmat[2][1];totmat[2][2] = tmpmat[2][2];

				Mat4CpyMat4(tmat, ob->obmat);

				mdet = Det3x3(	totmat[0][0],totmat[0][1],totmat[0][2],
								totmat[1][0],totmat[1][1],totmat[1][2],
								totmat[2][0],totmat[2][1],totmat[2][2]);
				if (mdet==0)
				{
					totmat[0][0] = 1;totmat[0][1] = 0;totmat[0][2] = 0;
					totmat[1][0] = 0;totmat[1][1] = 1;totmat[1][2] = 0;
					totmat[2][0] = 0;totmat[2][1] = 0;totmat[2][2] = 1;
				}

				/* apply out transformaton to the object */
				Mat4MulMat34(ob->obmat, totmat, tmat);
			}
		}
		break;
	case CONSTRAINT_TYPE_FOLLOWPATH:
		{
			bFollowPathConstraint *data;
			float obmat[4][4];

			data=(bFollowPathConstraint*)constraint->data;			

			if (data->tar) {
				// weird, this is needed? doesnt work for workob (ton)
				object_to_mat4(ob, obmat);

				Mat4MulSerie(ob->obmat, targetmat, obmat, NULL, NULL, NULL, NULL, NULL, NULL);
			}
		}
		break;
	case CONSTRAINT_TYPE_STRETCHTO:
        {
            bStretchToConstraint *data;
            float size[3],scale[3],vec[3],xx[3],zz[3],orth[3];
            float totmat[3][3];
            float tmat[4][4];
            float dist;
            data=(bStretchToConstraint*)constraint->data;            
            Mat4ToSize (ob->obmat, size);

            
            if (data->tar){
                
                /* store X orientation before destroying obmat */
                xx[0] = ob->obmat[0][0];
                xx[1] = ob->obmat[0][1];
                xx[2] = ob->obmat[0][2];
                Normalise(xx);

                /* store Z orientation before destroying obmat */
                zz[0] = ob->obmat[2][0];
                zz[1] = ob->obmat[2][1];
                zz[2] = ob->obmat[2][2];
                Normalise(zz);

				VecSubf(vec, ob->obmat[3], targetmat[3]);
				vec[0] /= size[0];
				vec[1] /= size[1];
				vec[2] /= size[2];

				dist = Normalise(vec);
                //dist = VecLenf( ob->obmat[3], targetmat[3]);

                if (data->orglength == 0)  data->orglength = dist;
                if (data->bulge ==0) data->bulge = 1.0;

                scale[1] = dist/data->orglength;
                switch (data->volmode){
                /* volume preserving scaling */
                case VOLUME_XZ :
                    scale[0] = 1.0f - (float)sqrt(data->bulge) + (float)sqrt(data->bulge*(data->orglength/dist));
                    scale[2] = scale[0];
                    break;
                case VOLUME_X:
                    scale[0] = 1.0f + data->bulge * (data->orglength /dist - 1);
                    scale[2] = 1.0;
                    break;
                case VOLUME_Z:
                    scale[0] = 1.0;
                    scale[2] = 1.0f + data->bulge * (data->orglength /dist - 1);
                    break;
                    /* don't care for volume */
                case NO_VOLUME:
                    scale[0] = 1.0;
                    scale[2] = 1.0;
                    break;
                default: /* should not happen, but in case*/
                    return;    
                } /* switch (data->volmode) */

                /* Clear the object's rotation and scale */
                ob->obmat[0][0]=size[0]*scale[0];
                ob->obmat[0][1]=0;
                ob->obmat[0][2]=0;
                ob->obmat[1][0]=0;
                ob->obmat[1][1]=size[1]*scale[1];
                ob->obmat[1][2]=0;
                ob->obmat[2][0]=0;
                ob->obmat[2][1]=0;
                ob->obmat[2][2]=size[2]*scale[2];
                
                VecSubf(vec, ob->obmat[3], targetmat[3]);
                Normalise(vec);
                /* new Y aligns  object target connection*/
                totmat[1][0] = -vec[0];
                totmat[1][1] = -vec[1];
                totmat[1][2] = -vec[2];
                switch (data->plane){
                case PLANE_X:
                    /* build new Z vector */
                    /* othogonal to "new Y" "old X! plane */
                    Crossf(orth, vec, xx);
                    Normalise(orth);
                    
                    /* new Z*/
                    totmat[2][0] = orth[0];
                    totmat[2][1] = orth[1];
                    totmat[2][2] = orth[2];
                    
                    /* we decided to keep X plane*/
                    Crossf(xx,orth, vec);
                    Normalise(xx);
                    totmat[0][0] = xx[0];
                    totmat[0][1] = xx[1];
                    totmat[0][2] = xx[2];
                    break;
                case PLANE_Z:
                    /* build new X vector */
                    /* othogonal to "new Y" "old Z! plane */
                    Crossf(orth, vec, zz);
                    Normalise(orth);
                    
                    /* new X*/
                    totmat[0][0] = -orth[0];
                    totmat[0][1] = -orth[1];
                    totmat[0][2] = -orth[2];
                    
                    /* we decided to keep Z */
                    Crossf(zz,orth, vec);
                    Normalise(zz);
                    totmat[2][0] = zz[0];
                    totmat[2][1] = zz[1];
                    totmat[2][2] = zz[2];
                    break;
                } /* switch (data->plane) */
                

                Mat4CpyMat4(tmat, ob->obmat);
                
                Mat4MulMat34(ob->obmat, totmat, tmat);

            }
        }
        break;
	case CONSTRAINT_TYPE_LOCLIMIT:
		{
			bLocLimitConstraint *data;

			data = constraint->data;
			
			/* limit location relative to origin or parent   */
			if (data->flag2 & LIMIT_NOPARENT) {
				/* limiting relative to parent */
				float parmat[4][4]; /* matrix of parent */
				float objLoc[3], parLoc[3]; /* location of object, and location of parent */
				float relLoc[3]; /* objLoc  - parLoc*/
				
				/* get matrix of parent */
				Mat4CpyMat4(parmat, ob->parent->obmat);
				
				/* get locations as vectors */
				objLoc[0] = ob->obmat[3][0];
				objLoc[1] = ob->obmat[3][1];
				objLoc[2] = ob->obmat[3][2];
				
				parLoc[0] = parmat[3][0];
				parLoc[1] = parmat[3][1];
				parLoc[2] = parmat[3][2];
				
				/* get relative location of obj from parent */
				VecSubf(relLoc, objLoc, parLoc);
				
				/* limiting location */
				if (data->flag & LIMIT_XMIN) {
					if(relLoc[0] < data->xmin) 
						ob->obmat[3][0] = (parLoc[0]+data->xmin);
				}
				if (data->flag & LIMIT_XMAX) {
					if (relLoc[0] > data->xmax) 
						ob->obmat[3][0] = (parLoc[0]+data->xmax);
				}
				if (data->flag & LIMIT_YMIN) {
					if(relLoc[1] < data->ymin) 
						ob->obmat[3][1] = (parLoc[1]+data->ymin);
				}
				if (data->flag & LIMIT_YMAX) {
					if (relLoc[1] > data->ymax) 
						ob->obmat[3][1] = (parLoc[1]+data->ymax);
				}
				if (data->flag & LIMIT_ZMIN) {
					if(relLoc[2] < data->zmin) 
						ob->obmat[3][2] = (parLoc[2]+data->zmin);
				}
				if (data->flag & LIMIT_ZMAX) {
					if (relLoc[2] > data->zmax) 
						ob->obmat[3][2] = (parLoc[2]+data->zmax);
				}
			} else {
				/* limiting relative to origin */
				if (data->flag & LIMIT_XMIN) {
					if(ob->obmat[3][0] < data->xmin)
						ob->obmat[3][0] = data->xmin;
				}
				if (data->flag & LIMIT_XMAX) {
					if (ob->obmat[3][0] > data->xmax)
						ob->obmat[3][0] = data->xmax;
				}
				if (data->flag & LIMIT_YMIN) {
					if(ob->obmat[3][1] < data->ymin)
						ob->obmat[3][1] = data->ymin;
				}
				if (data->flag & LIMIT_YMAX) {
					if (ob->obmat[3][1] > data->ymax)
						ob->obmat[3][1] = data->ymax;
				}
				if (data->flag & LIMIT_ZMIN) {
					if(ob->obmat[3][2] < data->zmin) 
						ob->obmat[3][2] = data->zmin;
				}
				if (data->flag & LIMIT_ZMAX) {
					if (ob->obmat[3][2] > data->zmax)
						ob->obmat[3][2] = data->zmax;
				}
			}
		}
		break;
	case CONSTRAINT_TYPE_ROTLIMIT:
		{
			bRotLimitConstraint *data;
			float tmat[3][3];
			float eul[3];
			float size[3];
			
			data = constraint->data;
			
			Mat4ToSize(ob->obmat, size);
			
			Mat3CpyMat4(tmat, ob->obmat);
			Mat3Ortho(tmat); 
			Mat3ToEul(tmat, eul);
			
			/* eulers: radians to degrees! */
			eul[0] = (eul[0] / M_PI * 180);
			eul[1] = (eul[1] / M_PI * 180);
			eul[2] = (eul[2] / M_PI * 180);
			
			/* limiting of euler values... */
			if (data->flag & LIMIT_XROT) {
				if (eul[0] < data->xmin) 
					eul[0] = data->xmin;
					
				if (eul[0] > data->xmax)
					eul[0] = data->xmax;
			}
			if (data->flag & LIMIT_YROT) {
				if (eul[1] < data->ymin)
					eul[1] = data->ymin;
					
				if (eul[1] > data->ymax)
					eul[1] = data->ymax;
			}
			if (data->flag & LIMIT_ZROT) {
				if (eul[2] < data->zmin)
					eul[2] = data->zmin;
					
				if (eul[2] > data->zmax)
					eul[2] = data->zmax;
			}
				
			/* eulers: degrees to radians ! */
			eul[0] = (eul[0] / 180 * M_PI); 
			eul[1] = (eul[1] / 180 * M_PI);
			eul[2] = (eul[2] / 180 * M_PI);
			
			EulToMat3(eul, tmat);
			
			ob->obmat[0][0] = tmat[0][0]*size[0];
			ob->obmat[0][1] = tmat[0][1]*size[1];
			ob->obmat[0][2] = tmat[0][2]*size[2];

			ob->obmat[1][0] = tmat[1][0]*size[0];
			ob->obmat[1][1] = tmat[1][1]*size[1];
			ob->obmat[1][2] = tmat[1][2]*size[2];

			ob->obmat[2][0] = tmat[2][0]*size[0];
			ob->obmat[2][1] = tmat[2][1]*size[1];
			ob->obmat[2][2] = tmat[2][2]*size[2];
		}
		break;
	case CONSTRAINT_TYPE_SIZELIMIT:
		{
			bSizeLimitConstraint *data;
			float obsize[3], size[3];
			int clearNegScale=0;
			
			data = constraint->data;
			
			Mat4ToSize(ob->obmat, size);
			Mat4ToSize(ob->obmat, obsize);
			
			if (data->flag & LIMIT_XMIN) {
				if (ob->transflag & OB_NEG_SCALE) {
					size[0] *= -1;
					
					if (size[0] < data->xmin) { 
						size[0] = data->xmin;
						clearNegScale += 1;
					}
				} else {
					if (size[0] < data->xmin) 
						size[0] = data->xmin;	
				}	
			}
			if (data->flag & LIMIT_XMAX) {
				if (size[0] > data->xmax) 
					size[0] = data->xmax;
			}
			if (data->flag & LIMIT_YMIN) {
				if (ob->transflag & OB_NEG_SCALE) {
					size[1] *= -1;
					
					if (size[1] < data->ymin) { 
						size[1] = data->ymin;
						clearNegScale += 1;
					}
				} else {
					if (size[1] < data->ymin) 
						size[1] = data->ymin;	
				}	
			}
			if (data->flag & LIMIT_YMAX) {
				if (size[1] > data->ymax) 
					size[1] = data->ymax;
			}
			if (data->flag & LIMIT_ZMIN) {
				if (ob->transflag & OB_NEG_SCALE) {
					size[2] *= -1;
					
					if (size[2] < data->zmin) { 
						size[2] = data->zmin;
						clearNegScale += 1;
					}
				} else {
					if (size[2] < data->zmin) 
						size[2] = data->zmin;	
				}	
			}
			if (data->flag & LIMIT_ZMAX) {
				if (size[2] > data->zmax) 
					size[2] = data->zmax;
			}
			
			if (clearNegScale != 0) {
				ob->transflag &= ~OB_NEG_SCALE;  /* is this how we remove that flag? */	
			}
			
			VecMulf(ob->obmat[0], size[0]/obsize[0]);
			VecMulf(ob->obmat[1], size[1]/obsize[1]);
			VecMulf(ob->obmat[2], size[2]/obsize[2]);
		}
		break;
	default:
		printf ("Error: Unknown constraint type\n");
		break;
	}
}
