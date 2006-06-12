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
 *
 *
 * - here initialize and free and handling SPACE data
 */

#include <string.h>
#include <stdio.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "MEM_guardedalloc.h"
#include "MEM_CacheLimiterC-Api.h"

#ifdef INTERNATIONAL
#include "BIF_language.h"
#endif

#include "IMB_imbuf_types.h"
#include "IMB_imbuf.h"

#include "BLI_blenlib.h"
#include "BLI_arithb.h"
#include "BLI_linklist.h"

#include "DNA_action_types.h"
#include "DNA_armature_types.h"
#include "DNA_curve_types.h"
#include "DNA_group_types.h" /* used for select_same_group */
#include "DNA_image_types.h"
#include "DNA_ipo_types.h"
#include "DNA_mesh_types.h"
#include "DNA_object_types.h"
#include "DNA_scene_types.h"
#include "DNA_screen_types.h"
#include "DNA_sequence_types.h"
#include "DNA_sound_types.h"
#include "DNA_space_types.h"
#include "DNA_userdef_types.h"
#include "DNA_view2d_types.h"
#include "DNA_view3d_types.h"

#include "BKE_blender.h"
#include "BKE_colortools.h"
#include "BKE_curve.h"
#include "BKE_depsgraph.h"
#include "BKE_displist.h"
#include "BKE_global.h"
#include "BKE_group.h"
#include "BKE_ipo.h"
#include "BKE_main.h"
#include "BKE_node.h"
#include "BKE_scene.h"
#include "BKE_utildefines.h"

#include "BIF_spacetypes.h"  // first, nasty dependency with typedef

#include "BIF_butspace.h"
#include "BIF_drawimage.h"
#include "BIF_drawseq.h"
#include "BIF_drawtext.h"
#include "BIF_drawscript.h"
#include "BIF_editarmature.h"
#include "BIF_editconstraint.h"
#include "BIF_editfont.h"
#include "BIF_editgroup.h"
#include "BIF_editkey.h"
#include "BIF_editlattice.h"
#include "BIF_editmesh.h"
#include "BIF_editmode_undo.h"
#include "BIF_editnla.h"
#include "BIF_editoops.h"
#include "BIF_editseq.h"
#include "BIF_editsima.h"
#include "BIF_editsound.h"
#include "BIF_editview.h"
#include "BIF_gl.h"
#include "BIF_imasel.h"
#include "BIF_interface.h"
#include "BIF_meshtools.h"
#include "BIF_mywindow.h"
#include "BIF_oops.h"
#include "BIF_poseobject.h"
#include "BIF_outliner.h"
#include "BIF_resources.h"
#include "BIF_screen.h"
#include "BIF_space.h"
#include "BIF_toets.h"
#include "BIF_toolbox.h"
#include "BIF_usiblender.h"
#include "BIF_previewrender.h"

#include "BSE_edit.h"
#include "BSE_view.h"
#include "BSE_editipo.h"
#include "BSE_drawipo.h"
#include "BSE_drawview.h"
#include "BSE_drawnla.h"
#include "BSE_filesel.h"
#include "BSE_headerbuttons.h"
#include "BSE_editnla_types.h"

#include "BDR_vpaint.h"
#include "BDR_editmball.h"
#include "BDR_editobject.h"
#include "BDR_editcurve.h"
#include "BDR_editface.h"
#include "BDR_drawmesh.h"
#include "BDR_drawobject.h"
#include "BDR_imagepaint.h"
#include "BDR_unwrapper.h"

#include "BLO_readfile.h" /* for BLO_blendhandle_close */

#include "PIL_time.h"

#include "BPY_extern.h"

#include "butspace.h"
#include "mydevice.h"
#include "blendef.h"
#include "datatoc.h"

#include "BIF_transform.h"

#include "BKE_depsgraph.h"

#include "BSE_trans_types.h"
#include "IMG_Api.h"

#include "SYS_System.h" /* for the user def menu ... should move elsewhere. */


extern void StartKetsjiShell(ScrArea *area, char* startscenename, struct Main* maggie, struct SpaceIpo* sipo,int always_use_expand_framing);

/**
 * When the mipmap setting changes, we want to redraw the view right
 * away to reflect this setting.
 */
void space_mipmap_button_function(int event);

void free_soundspace(SpaceSound *ssound);

/* *************************************** */

/* don't know yet how the handlers will evolve, for simplicity
   i choose for an array with eventcodes, this saves in a file!
   */
void add_blockhandler(ScrArea *sa, short eventcode, short val)
{
	SpaceLink *sl= sa->spacedata.first;
	short a;
	
	// find empty spot
	for(a=0; a<SPACE_MAXHANDLER; a+=2) {
		if( sl->blockhandler[a]==eventcode ) {
			sl->blockhandler[a+1]= val;
			break;
		}
		else if( sl->blockhandler[a]==0) {
			sl->blockhandler[a]= eventcode;
			sl->blockhandler[a+1]= val;
			break;
		}
	}
	if(a==SPACE_MAXHANDLER) printf("error; max (4) blockhandlers reached!\n");
}

void rem_blockhandler(ScrArea *sa, short eventcode)
{
	SpaceLink *sl= sa->spacedata.first;
	short a;
	
	for(a=0; a<SPACE_MAXHANDLER; a+=2) {
		if( sl->blockhandler[a]==eventcode) {
			sl->blockhandler[a]= 0;
			
			/* specific free calls */
			if(eventcode==IMAGE_HANDLER_PREVIEW)
				image_preview_event(0);
			break;
		}
	}
}

void toggle_blockhandler(ScrArea *sa, short eventcode, short val)
{
	SpaceLink *sl= sa->spacedata.first;
	short a, addnew=1;
	
	// find if it exists
	for(a=0; a<SPACE_MAXHANDLER; a+=2) {
		if( sl->blockhandler[a]==eventcode ) {
			sl->blockhandler[a]= 0;
			
			/* specific free calls */
			if(eventcode==VIEW3D_HANDLER_PREVIEW)
				BIF_view3d_previewrender_free(sa->spacedata.first);
			else if(eventcode==IMAGE_HANDLER_PREVIEW)
				image_preview_event(0);
			
			addnew= 0;
		}
	}
	if(addnew) add_blockhandler(sa, eventcode, val);
}



/* ************* SPACE: VIEW3D  ************* */

/*  extern void drawview3dspace(ScrArea *sa, void *spacedata); BSE_drawview.h */


void copy_view3d_lock(short val)
{
	bScreen *sc;
	int bit;
	
	/* from G.scene copy to the other views */
	sc= G.main->screen.first;
	
	while(sc) {
		if(sc->scene==G.scene) {
			ScrArea *sa= sc->areabase.first;
			while(sa) {
				SpaceLink *sl= sa->spacedata.first;
				while(sl) {
					if(sl->spacetype==SPACE_OOPS && val==REDRAW) {
						if(sa->win) scrarea_queue_winredraw(sa);
					}
					else if(sl->spacetype==SPACE_VIEW3D) {
						View3D *vd= (View3D*) sl;
						if(vd->scenelock && vd->localview==0) {
							vd->lay= G.scene->lay;
							vd->camera= G.scene->camera;
							
							if(vd->camera==0 && vd->persp>1) vd->persp= 1;
							
							if( (vd->lay & vd->layact) == 0) {
								bit= 0;
								while(bit<32) {
									if(vd->lay & (1<<bit)) {
										vd->layact= 1<<bit;
										break;
									}
									bit++;
								}
							}
							
							if(val==REDRAW && vd==sa->spacedata.first) {
								if(sa->win) scrarea_queue_redraw(sa);
							}
						}
					}
					sl= sl->next;
				}
				sa= sa->next;
			}
		}
		sc= sc->id.next;
	}
}

void handle_view3d_around()
{
	bScreen *sc;
	
	if ((U.uiflag & USER_LOCKAROUND)==0) return;
	
	/* copies from G.vd->around to other view3ds */
	
	sc= G.main->screen.first;
	
	while(sc) {
		if(sc->scene==G.scene) {
			ScrArea *sa= sc->areabase.first;
			while(sa) {
				SpaceLink *sl= sa->spacedata.first;
				while(sl) {
					if(sl->spacetype==SPACE_VIEW3D) {
						View3D *vd= (View3D*) sl;
						if (vd != G.vd) {
							vd->around= G.vd->around;
							if (G.vd->flag & V3D_ALIGN)
								vd->flag |= V3D_ALIGN;
							else
								vd->flag &= ~V3D_ALIGN;
							scrarea_queue_headredraw(sa);
						}
					}
					sl= sl->next;
				}
				sa= sa->next;
			}
		}
		sc= sc->id.next;
	}
}

void handle_view3d_lock()
{
	if (G.vd != NULL) {
		if(G.vd->localview==0 && G.vd->scenelock && curarea->spacetype==SPACE_VIEW3D) {

			/* copy to scene */
			G.scene->lay= G.vd->lay;
			G.scene->camera= G.vd->camera;
	
			copy_view3d_lock(REDRAW);
		}
	}
}

void space_set_commmandline_options(void) {
	SYS_SystemHandle syshandle;
	int a;
		
	if ( (syshandle = SYS_GetSystem()) ) {
		/* User defined settings */
		a= (U.gameflags & USER_VERTEX_ARRAYS);
		SYS_WriteCommandLineInt(syshandle, "vertexarrays", a);

		a= (U.gameflags & USER_DISABLE_SOUND);
		SYS_WriteCommandLineInt(syshandle, "noaudio", a);

		a= (U.gameflags & USER_DISABLE_MIPMAP);
		set_mipmap(!a);
		SYS_WriteCommandLineInt(syshandle, "nomipmap", a);

		/* File specific settings: */
		/* Only test the first one. These two are switched
		 * simultaneously. */
		a= (G.fileflags & G_FILE_SHOW_FRAMERATE);
		SYS_WriteCommandLineInt(syshandle, "show_framerate", a);
		SYS_WriteCommandLineInt(syshandle, "show_profile", a);

		/* When in wireframe mode, always draw debug props. */
		if (G.vd) {
			a = ( (G.fileflags & G_FILE_SHOW_DEBUG_PROPS) 
				  || (G.vd->drawtype == OB_WIRE)          
				  || (G.vd->drawtype == OB_SOLID)         );
			SYS_WriteCommandLineInt(syshandle, "show_properties", a);
		}

		a= (G.fileflags & G_FILE_ENABLE_ALL_FRAMES);
		SYS_WriteCommandLineInt(syshandle, "fixedtime", a);

		a= (G.fileflags & G_FILE_GAME_TO_IPO);
		SYS_WriteCommandLineInt(syshandle, "game2ipo", a);

		a=(G.fileflags & G_FILE_GAME_MAT);
		SYS_WriteCommandLineInt(syshandle, "blender_material", a);
		a=(G.fileflags & G_FILE_DIAPLAY_LISTS);
		SYS_WriteCommandLineInt(syshandle, "displaylists", a);


	}
}

#if GAMEBLENDER == 1
	/**
	 * These two routines imported from the gameengine, 
	 * I suspect a lot of the resetting stuff is cruft
	 * and can be removed, but it should be checked.
	 */
static void SaveState(void)
{
	glPushAttrib(GL_ALL_ATTRIB_BITS);

	init_realtime_GL();
	init_gl_stuff();

	if(G.scene->camera==0 || G.scene->camera->type!=OB_CAMERA)
		error("no (correct) camera");

	waitcursor(1);
}

static void RestoreState(void)
{
	curarea->win_swap = 0;
	curarea->head_swap=0;
	allqueue(REDRAWVIEW3D, 1);
	allqueue(REDRAWBUTSALL, 0);
	reset_slowparents();
	waitcursor(0);
	G.qual= 0;
	glPopAttrib();
}

static LinkNode *save_and_reset_all_scene_cfra(void)
{
	LinkNode *storelist= NULL;
	Scene *sc;
	
	for (sc= G.main->scene.first; sc; sc= sc->id.next) {
		BLI_linklist_prepend(&storelist, (void*) (long) sc->r.cfra);

		//why is this reset to 1 ?
		//sc->r.cfra= 1;

		set_scene_bg(sc);
	}
	
	BLI_linklist_reverse(&storelist);
	
	return storelist;
}

static void restore_all_scene_cfra(LinkNode *storelist) {
	LinkNode *sc_store= storelist;
	Scene *sc;
	
	for (sc= G.main->scene.first; sc; sc= sc->id.next) {
		int stored_cfra= (int) sc_store->link;
		
		sc->r.cfra= stored_cfra;
		set_scene_bg(sc);
		
		sc_store= sc_store->next;
	}
	
	BLI_linklist_free(storelist, NULL);
}
#endif

void start_game(void)
{
#if GAMEBLENDER == 1
#ifndef NO_KETSJI
	Scene *sc, *startscene = G.scene;
	LinkNode *scene_cfra_store;

		/* XXX, silly code -  the game engine can
		 * access any scene through logic, so we try 
		 * to make sure each scene has a valid camera, 
		 * just in case the game engine tries to use it.
		 * 
		 * Better would be to make a better routine
		 * in the game engine for finding the camera.
		 *  - zr
		 * Note: yes, this is all very badly hacked! (ton)
		 */
	for (sc= G.main->scene.first; sc; sc= sc->id.next) {
		if (!sc->camera) {
			Base *base;
	
			for (base= sc->base.first; base; base= base->next)
				if (base->object->type==OB_CAMERA)
					break;
			
			sc->camera= base?base->object:NULL;
		}
	}

	/* these two lines make sure front and backbuffer are equal. for swapbuffers */
	markdirty_all();
	screen_swapbuffers();

	/* can start from header */
	mywinset(curarea->win);
    
	scene_cfra_store= save_and_reset_all_scene_cfra();
	

	/* game engine will do its own sounds. */
	sound_stop_all_sounds();
	sound_exit_audio();
	
	/* Before jumping into Ketsji, we configure some settings. */
	space_set_commmandline_options();

	SaveState();
	StartKetsjiShell(curarea, startscene->id.name+2, G.main,G.sipo, 1);
	RestoreState();

	/* Restart BPY - unload the game engine modules. */
	BPY_end_python();
	BPY_start_python(0, NULL); /* argc, argv stored there already */
	BPY_post_start_python(); /* userpref path and menus init */

	restore_all_scene_cfra(scene_cfra_store);
	set_scene_bg(startscene);
	scene_update_for_newframe(G.scene, G.scene->lay);

	if (G.flags & G_FILE_AUTOPLAY)
		exit_usiblender();

		/* groups could have changed ipo */
	allqueue(REDRAWNLA, 0);
	allqueue(REDRAWACTION, 0);
	allspace(REMAKEIPO, 0);
	allqueue(REDRAWIPO, 0);
#endif
#else
	notice("Game engine is disabled in this release!");
#endif
}

static void changeview3dspace(ScrArea *sa, void *spacedata)
{
	setwinmatrixview3d(sa->winx, sa->winy, NULL);	/* 0= no pick rect */
}

	/* Callable from editmode and faceselect mode from the
	 * moment, would be nice (and is easy) to generalize
	 * to any mode.
	 */
static void align_view_to_selected(View3D *v3d)
{
	int nr= pupmenu("Align View%t|To Selected (top)%x2|To Selected (front)%x1|To Selected (side)%x0");

	if (nr!=-1) {
		int axis= nr;

		if ((G.obedit) && (G.obedit->type == OB_MESH)) {
			editmesh_align_view_to_selected(v3d, axis);
			addqueue(v3d->area->win, REDRAW, 1);
		} else if (G.f & G_FACESELECT) {
			Object *obact= OBACT;
			if (obact && obact->type==OB_MESH) {
				Mesh *me= obact->data;

				if (me->tface) {
					faceselect_align_view_to_selected(v3d, me, axis);
					addqueue(v3d->area->win, REDRAW, 1);
				}
			}
		}
	}
}

static void select_children(Object *ob, int recursive)
{
	Base *base;

	for (base= FIRSTBASE; base; base= base->next)
		if (ob == base->object->parent) {
			base->flag |= SELECT;
			base->object->flag |= SELECT;
			if (recursive) select_children(base->object, 1);
		}
}

static void select_parent(void)	/* Makes parent active and de-selected OBACT */
{
	Base *base, *startbase, *basact=NULL, *oldbasact;
	
	if (!(OBACT) || !(OBACT->parent)) return;
	BASACT->flag &= (~SELECT);
	BASACT->object->flag &= (~SELECT);
	startbase=  FIRSTBASE;
	if(BASACT && BASACT->next) startbase= BASACT->next;
	base = startbase;
	while(base) {
		if(base->object==BASACT->object->parent) { basact=base; break; }
		base=base->next;
		if(base==NULL) base= FIRSTBASE;
		if(base==startbase) break;
	}
	/* can be NULL if parent in other scene */
	if(basact) {
		oldbasact = BASACT;
		BASACT = basact;
		basact->flag |= SELECT;		
		
		basact->object->flag= basact->flag;
		
		set_active_base(basact);
	}
}

static void select_same_group(Object *ob)	/* Select objects in the same group as the active */
{
	Base *base;
	Group *group= find_group(ob);
	if (!group || !ob)
		return;
	
	for (base= FIRSTBASE; base; base= base->next)
		if (object_in_group(base->object, group)) {
			base->flag |= SELECT;
			base->object->flag |= SELECT;
		}
}


static void select_same_parent(Object *ob)	/* Select objects woth the same parent as the active (siblings), parent can be NULL also */
{
	Base *base;
	if (!ob)
		return;
	
	for (base= FIRSTBASE; base; base= base->next)
		if (base->object->parent==ob->parent) {
			base->flag |= SELECT;
			base->object->flag |= SELECT;
		}
}

static void select_same_type(Object *ob)	/* Select objects woth the same parent as the active (siblings), parent can be NULL also */
{
	Base *base;
	if (!ob)
		return;
	
	for (base= FIRSTBASE; base; base= base->next)
		if (base->object->type==ob->type) {
			base->flag |= SELECT;
			base->object->flag |= SELECT;
		}
}

void select_object_grouped(short nr)
{
	Base *base;
	
	if(nr==6) {
		base= FIRSTBASE;
		while(base) {
			if (base->lay & OBACT->lay) {
				base->flag |= SELECT;
				base->object->flag |= SELECT;
			}
			base= base->next;
		}
	}
	else if(nr==1) select_children(OBACT, 1);
	else if(nr==2) select_children(OBACT, 0);
	else if(nr==3) select_parent();
	else if(nr==4) select_same_parent(OBACT);	
	else if(nr==5) select_same_type(OBACT);	
	else if(nr==7) select_same_group(OBACT);
	
	
	
	
	countall();
	allqueue(REDRAWVIEW3D, 0);
	allqueue(REDRAWBUTSOBJECT, 0);
	allspace(REMAKEIPO, 0);
	allqueue(REDRAWIPO, 0);
}

static void select_object_grouped_menu(void)
{
	char *str;
	short nr;

	/* make menu string */
	
	str= MEM_mallocN(180, "groupmenu");
	strcpy(str, "Select Grouped%t|Children%x1|"
	            "Immediate Children%x2|Parent%x3|"
	            "Siblings (Shared Parent)%x4|"
	            "Objects of Same Type%x5|"
				"Objects on Shared Layers%x6|"
                "Objects in Same Group%x7|");

	/* here we go */
	
	nr= pupmenu(str);
	MEM_freeN(str);
	
	select_object_grouped(nr);
}

void join_menu(void)
{
	Object *ob= OBACT;
	if (ob && !G.obedit) {
		if(ob->type == OB_MESH) {
			if(okee("Join selected meshes")==0) return;
			join_mesh();
		} else if(ob->type == OB_CURVE) {
			if(okee("Join selected curves")==0) return;
			join_curve(OB_CURVE);
		} else if(ob->type == OB_SURF) {
			if(okee("Join selected NURBS")==0) return;
			join_curve(OB_SURF);
		} else if(ob->type == OB_ARMATURE) {
			/*	Make sure the user wants to continue*/
			if(okee("Join selected armatures")==0) return;
			join_armature ();
		}
	}
}

static unsigned short convert_for_nonumpad(unsigned short event)
{
	if (event>=ZEROKEY && event<=NINEKEY) {
		return event - ZEROKEY + PAD0;
	} else if (event==MINUSKEY) {
		return PADMINUS;
	} else if (event==EQUALKEY) {
		return PADPLUSKEY;
	} else if (event==BACKSLASHKEY) {
		return PADSLASHKEY;
	} else {
		return event;
	}
}

/* *************** */

void BIF_undo_push(char *str)
{
	if(G.obedit) {
		if (U.undosteps == 0) return;

		if(G.obedit->type==OB_MESH)
			undo_push_mesh(str);
		else if ELEM(G.obedit->type, OB_CURVE, OB_SURF)
			undo_push_curve(str);
		else if (G.obedit->type==OB_FONT)
			undo_push_font(str);
		else if (G.obedit->type==OB_MBALL)
			undo_push_mball(str);
		else if (G.obedit->type==OB_LATTICE)
			undo_push_lattice(str);
		else if (G.obedit->type==OB_ARMATURE)
			undo_push_armature(str);
	}
	else {
		if(U.uiflag & USER_GLOBALUNDO) 
			BKE_write_undo(str);
	}
}

void BIF_undo(void)
{	
	if(G.obedit) {
		if ELEM7(G.obedit->type, OB_MESH, OB_FONT, OB_CURVE, OB_SURF, OB_MBALL, OB_LATTICE, OB_ARMATURE)
			undo_editmode_step(1);
	}
	else {
		if(G.f & G_WEIGHTPAINT)
			wpaint_undo();
		else if(G.f & G_VERTEXPAINT)
			vpaint_undo();
		else if(G.f & G_TEXTUREPAINT); /* no texture paint undo yet */
		else if(curarea->spacetype==SPACE_IMAGE && (G.sima->flag & SI_DRAWTOOL));
		else {
			/* now also in faceselect mode */
			if(U.uiflag & USER_GLOBALUNDO) {
				BKE_undo_step(1);
				sound_initialize_sounds();
			}
		}
	}
}

void BIF_redo(void)
{
	if(G.obedit) {
		if ELEM7(G.obedit->type, OB_MESH, OB_FONT, OB_CURVE, OB_SURF, OB_MBALL, OB_LATTICE, OB_ARMATURE)
			undo_editmode_step(-1);
	}
	else {
		if(G.f & G_WEIGHTPAINT)
			wpaint_undo();
		else if(G.f & G_VERTEXPAINT)
			vpaint_undo();
		else {
			/* includes faceselect now */
			if(U.uiflag & USER_GLOBALUNDO) {
				BKE_undo_step(-1);
				sound_initialize_sounds();
			}
		}
	}
}

void BIF_undo_menu(void)
{
	if(G.obedit) {
		if ELEM7(G.obedit->type, OB_MESH, OB_FONT, OB_CURVE, OB_SURF, OB_MBALL, OB_LATTICE, OB_ARMATURE)
			undo_editmode_menu();
		allqueue(REDRAWALL, 0);
	}
	else {
		if(G.f & G_WEIGHTPAINT)
			;
		else if(G.f & G_VERTEXPAINT)
			;
		else {
			if(U.uiflag & USER_GLOBALUNDO) {
				char *menu= BKE_undo_menu_string();
				if(menu) {
					short event= pupmenu_col(menu, 20);
					MEM_freeN(menu);
					if(event>0) BKE_undo_number(event);
				}
			}
		}
	}
}

/* *************** */

static void winqreadview3dspace(ScrArea *sa, void *spacedata, BWinEvent *evt)
{
	View3D *v3d= sa->spacedata.first;
	Object *ob= OBACT;	// do not change!
	float *curs;
	int doredraw= 0, pupval;
	unsigned short event= evt->event;
	short val= evt->val;
	char ascii= evt->ascii;
	
	if(curarea->win==0) return;	/* when it comes from sa->headqread() */
	
	if(val) {

		if( uiDoBlocks(&curarea->uiblocks, event)!=UI_NOTHING ) event= 0;
		if(event==MOUSEY || event==MOUSEX) return;
		
		if(event==UI_BUT_EVENT) do_butspace(val); // temporal, view3d deserves own queue?
		
		/* we consider manipulator a button, defaulting to leftmouse */
		if(event==LEFTMOUSE) if(BIF_do_manipulator(sa)) return;
		
		/* swap mouse buttons based on user preference */
		if (U.flag & USER_LMOUSESELECT) {
			if (event==LEFTMOUSE) event = RIGHTMOUSE;
			else if (event==RIGHTMOUSE) event = LEFTMOUSE;
		}

		/* run any view3d event handler script links */
		if (event && sa->scriptlink.totscript)
			if (BPY_do_spacehandlers(sa, event, SPACEHANDLER_VIEW3D_EVENT))
				return; /* return if event was processed (swallowed) by handler(s) */

		/* TEXTEDITING?? */
		if((G.obedit) && G.obedit->type==OB_FONT) {
			switch(event) {
			
			case LEFTMOUSE:
				mouse_cursor();
				break;
			case MIDDLEMOUSE:
				/* use '&' here, because of alt+leftmouse which emulates middlemouse */
				if(U.flag & USER_VIEWMOVE) {
					if((G.qual==LR_SHIFTKEY) || ((U.flag & USER_TWOBUTTONMOUSE) && (G.qual==(LR_ALTKEY|LR_SHIFTKEY))))
						viewmove(0);
					else if((G.qual==LR_CTRLKEY) || ((U.flag & USER_TWOBUTTONMOUSE) && (G.qual==(LR_ALTKEY|LR_CTRLKEY))))
						viewmove(2);
					else if((G.qual==0) || ((U.flag & USER_TWOBUTTONMOUSE) && (G.qual==LR_ALTKEY)))
						viewmove(1);
				}
				else {
					if((G.qual==LR_SHIFTKEY) || ((U.flag & USER_TWOBUTTONMOUSE) && (G.qual==(LR_ALTKEY|LR_SHIFTKEY))))
						viewmove(1);
					else if((G.qual==LR_CTRLKEY) || ((U.flag & USER_TWOBUTTONMOUSE) && (G.qual==(LR_ALTKEY|LR_CTRLKEY))))
						viewmove(2);
					else
						viewmove(0);
				}
				break;
				
			case WHEELUPMOUSE:
				/* Regular:   Zoom in */
				/* Shift:     Scroll up */
				/* Ctrl:      Scroll right */
				/* Alt-Shift: Rotate up */
				/* Alt-Ctrl:  Rotate right */

				if( G.qual & LR_SHIFTKEY ) {
					if( G.qual & LR_ALTKEY ) { 
						G.qual &= ~LR_SHIFTKEY;
						persptoetsen(PAD2);
						G.qual |= LR_SHIFTKEY;
					} else {
						persptoetsen(PAD2);
					}
				} else if( G.qual & LR_CTRLKEY ) {
					if( G.qual & LR_ALTKEY ) { 
						G.qual &= ~LR_CTRLKEY;
						persptoetsen(PAD4);
						G.qual |= LR_CTRLKEY;
					} else {
						persptoetsen(PAD4);
					}
				} else if(U.uiflag & USER_WHEELZOOMDIR) 
					persptoetsen(PADMINUS);
				else
					persptoetsen(PADPLUSKEY);

				doredraw= 1;
				break;

			case WHEELDOWNMOUSE:
				/* Regular:   Zoom out */
				/* Shift:     Scroll down */
				/* Ctrl:      Scroll left */
				/* Alt-Shift: Rotate down */
				/* Alt-Ctrl:  Rotate left */

				if( G.qual & LR_SHIFTKEY ) {
					if( G.qual & LR_ALTKEY ) { 
						G.qual &= ~LR_SHIFTKEY;
						persptoetsen(PAD8);
						G.qual |= LR_SHIFTKEY;
					} else {
						persptoetsen(PAD8);
					}
				} else if( G.qual & LR_CTRLKEY ) {
					if( G.qual & LR_ALTKEY ) { 
						G.qual &= ~LR_CTRLKEY;
						persptoetsen(PAD6);
						G.qual |= LR_CTRLKEY;
					} else {
						persptoetsen(PAD6);
					}
				} else if(U.uiflag & USER_WHEELZOOMDIR) 
					persptoetsen(PADPLUSKEY);
				else
					persptoetsen(PADMINUS);
				
				doredraw= 1;
				break;

			case UKEY:
				if(G.qual==LR_ALTKEY) {
					remake_editText();
					doredraw= 1;
				} 
				else {
					do_textedit(event, val, ascii);
				}
				break;
			case VKEY:
				if(G.qual==LR_ALTKEY) {
					paste_editText();
					doredraw= 1;
				} 
				else {
					do_textedit(event, val, ascii);
				}
				break;
			case PAD0: case PAD1: case PAD2: case PAD3: case PAD4:
			case PAD5: case PAD6: case PAD7: case PAD8: case PAD9:
			case PADENTER:
				persptoetsen(event);
				doredraw= 1;
				break;
				
			default:
				do_textedit(event, val, ascii);
				break;
			}
		}
		else {

			if (U.flag & USER_NONUMPAD) {
				event= convert_for_nonumpad(event);
			}

			switch(event) {
			
			/* Afterqueue events */
			case BACKBUFDRAW:
				backdrawview3d(1);
				break;
			case RENDERPREVIEW:
				BIF_view3d_previewrender(sa);
				break;
				
			/* LEFTMOUSE and RIGHTMOUSE event codes can be swapped above,
			 * based on user preference USER_LMOUSESELECT
			 */
			case LEFTMOUSE: 
				if ((G.obedit) || !(G.f&(G_VERTEXPAINT|G_WEIGHTPAINT|G_TEXTUREPAINT))) {
					mouse_cursor();
				} else if (G.f & G_VERTEXPAINT) {
					vertex_paint();
				}
				else if (G.f & G_WEIGHTPAINT){
					weight_paint();
				}
				else if (G.f & G_TEXTUREPAINT) {
					face_draw();
				}
				break;
			case MIDDLEMOUSE:
				/* use '&' here, because of alt+leftmouse which emulates middlemouse */
				if(U.flag & USER_VIEWMOVE) {
					if((G.qual==LR_SHIFTKEY) || ((U.flag & USER_TWOBUTTONMOUSE) && (G.qual==(LR_ALTKEY|LR_SHIFTKEY))))
						viewmove(0);
					else if((G.qual==LR_CTRLKEY) || ((U.flag & USER_TWOBUTTONMOUSE) && (G.qual==(LR_ALTKEY|LR_CTRLKEY))))
						viewmove(2);
					else if((G.qual==0) || ((U.flag & USER_TWOBUTTONMOUSE) && (G.qual==LR_ALTKEY)))
						viewmove(1);
				}
				else {
					if((G.qual==LR_SHIFTKEY) || ((U.flag & USER_TWOBUTTONMOUSE) && (G.qual==(LR_ALTKEY|LR_SHIFTKEY))))
						viewmove(1);
					else if((G.qual==LR_CTRLKEY) || ((U.flag & USER_TWOBUTTONMOUSE) && (G.qual==(LR_ALTKEY|LR_CTRLKEY))))
						viewmove(2);
					else if((G.qual==0) || ((U.flag & USER_TWOBUTTONMOUSE) && (G.qual==LR_ALTKEY)))
						viewmove(0);
				}
				break;
			case RIGHTMOUSE:
				if((G.obedit) && (G.qual & LR_CTRLKEY)==0) {
					if(G.obedit->type==OB_MESH)
						mouse_mesh();
					else if ELEM(G.obedit->type, OB_CURVE, OB_SURF)
						mouse_nurb();
					else if(G.obedit->type==OB_MBALL)
						mouse_mball();
					else if(G.obedit->type==OB_LATTICE)
						mouse_lattice();
					else if(G.obedit->type==OB_ARMATURE)
						mouse_armature();
				}
				else if((G.obedit && G.obedit->type==OB_MESH) && (G.qual == (LR_CTRLKEY|LR_ALTKEY)))
					mouse_mesh();	// loop select for 1 mousebutton dudes
				else if((G.obedit && G.obedit->type==OB_MESH) && (G.qual == (LR_CTRLKEY|LR_ALTKEY|LR_SHIFTKEY)))
					mouse_mesh();	// loop select for 1 mousebutton dudes
				else if(G.qual==LR_CTRLKEY)
					mouse_select();	// also allow in editmode, for vertex parenting
				else if(G.f & G_FACESELECT)
					face_select();
				else if( G.f & (G_VERTEXPAINT|G_TEXTUREPAINT))
					sample_vpaint();
				else
					mouse_select();	// does poses too
				break;
			case WHEELUPMOUSE:
				/* Regular:   Zoom in */
				/* Shift:     Scroll up */
				/* Ctrl:      Scroll right */
				/* Alt-Shift: Rotate up */
				/* Alt-Ctrl:  Rotate right */

				if( G.qual & LR_SHIFTKEY ) {
					if( G.qual & LR_ALTKEY ) { 
						G.qual &= ~LR_SHIFTKEY;
						persptoetsen(PAD2);
						G.qual |= LR_SHIFTKEY;
					} else {
						persptoetsen(PAD2);
					}
				} else if( G.qual & LR_CTRLKEY ) {
					if( G.qual & LR_ALTKEY ) { 
						G.qual &= ~LR_CTRLKEY;
						persptoetsen(PAD4);
						G.qual |= LR_CTRLKEY;
					} else {
						persptoetsen(PAD4);
					}
				} else if(U.uiflag & USER_WHEELZOOMDIR) 
					persptoetsen(PADMINUS);
				else
					persptoetsen(PADPLUSKEY);

				doredraw= 1;
				break;
			case WHEELDOWNMOUSE:
				/* Regular:   Zoom out */
				/* Shift:     Scroll down */
				/* Ctrl:      Scroll left */
				/* Alt-Shift: Rotate down */
				/* Alt-Ctrl:  Rotate left */

				if( G.qual & LR_SHIFTKEY ) {
					if( G.qual & LR_ALTKEY ) { 
						G.qual &= ~LR_SHIFTKEY;
						persptoetsen(PAD8);
						G.qual |= LR_SHIFTKEY;
					} else {
						persptoetsen(PAD8);
					}
				} else if( G.qual & LR_CTRLKEY ) {
					if( G.qual & LR_ALTKEY ) { 
						G.qual &= ~LR_CTRLKEY;
						persptoetsen(PAD6);
						G.qual |= LR_CTRLKEY;
					} else {
						persptoetsen(PAD6);
					}
				} else if(U.uiflag & USER_WHEELZOOMDIR) 
					persptoetsen(PADPLUSKEY);
				else
					persptoetsen(PADMINUS);
				
				doredraw= 1;
				break;
			
			case ONEKEY:
				if(G.qual==LR_CTRLKEY) {
					if(ob && ob->type == OB_MESH) {
						flip_subdivison(ob, 1);
					}
				}
				else do_layer_buttons(0); 
				break;
				
			case TWOKEY:
				if(G.qual==LR_CTRLKEY) {
					if(ob && ob->type == OB_MESH) {
						flip_subdivison(ob, 2);
					}
				}
				else do_layer_buttons(1); 
				break;
				
			case THREEKEY:
				if(G.qual==LR_CTRLKEY) {
					if(ob && ob->type == OB_MESH) {
						flip_subdivison(ob, 3);
					}
				}
				else if ( G.qual == (LR_SHIFTKEY | LR_ALTKEY | LR_CTRLKEY) ) {
					if ( (G.obedit) && (G.obedit->type==OB_MESH) )
						select_faces_by_numverts(3);
				}
				else do_layer_buttons(2); 
				break;
				
			case FOURKEY:
				if(G.qual==LR_CTRLKEY) {
					if(ob && ob->type == OB_MESH) {
						flip_subdivison(ob, 4);
					}
				}
				else if ( G.qual == (LR_SHIFTKEY | LR_ALTKEY | LR_CTRLKEY) ) {
					if ( (G.obedit) && (G.obedit->type==OB_MESH) )
						select_faces_by_numverts(4);
				}
				else do_layer_buttons(3); 
				break;
				
			case FIVEKEY:
				if ( G.qual == (LR_SHIFTKEY | LR_ALTKEY | LR_CTRLKEY) ) {
					if ( (G.obedit) && (G.obedit->type==OB_MESH) )
						select_faces_by_numverts(5);
				}
				else do_layer_buttons(4);
				break;

			case SIXKEY:
				do_layer_buttons(5); break;
			case SEVENKEY:
				do_layer_buttons(6); break;
			case EIGHTKEY:
				do_layer_buttons(7); break;
			case NINEKEY:
				do_layer_buttons(8); break;
			case ZEROKEY:
				do_layer_buttons(9); break;
			case MINUSKEY:
				do_layer_buttons(10); break;
			case EQUALKEY:
				do_layer_buttons(11); break;
			case ACCENTGRAVEKEY:
				do_layer_buttons(-1); break;
			
			case SPACEKEY:
				if(G.qual == LR_CTRLKEY) {
					val= pupmenu("Manipulator%t|Enable/Disable|Translate|Rotate|Scale|Combo");
					if(val>0) {
						if(val==1) v3d->twflag ^= V3D_USE_MANIPULATOR;
						else {
							if(val==2) v3d->twtype= V3D_MANIP_TRANSLATE;
							else if(val==3) v3d->twtype= V3D_MANIP_ROTATE;
							else if(val==4) v3d->twtype= V3D_MANIP_SCALE;
							else if(val==5) v3d->twtype= V3D_MANIP_TRANSLATE|V3D_MANIP_ROTATE|V3D_MANIP_SCALE;
							v3d->twflag |= V3D_USE_MANIPULATOR;
						}
						doredraw= 1;
					}
				}
				else if(G.qual == LR_ALTKEY) {
 					BIF_selectOrientation();
 					doredraw= 1;
				}

				break;
				
			case AKEY:
				if(G.qual & LR_CTRLKEY) apply_object();	// also with shift!
				else if((G.qual==LR_SHIFTKEY)) {
					toolbox_n_add();
				}
				else {
					if(G.obedit) {
						if(G.obedit->type==OB_MESH)
							deselectall_mesh();
						else if ELEM(G.obedit->type, OB_CURVE, OB_SURF)
							deselectall_nurb();
						else if(G.obedit->type==OB_MBALL)
							deselectall_mball();
						else if(G.obedit->type==OB_LATTICE)
							deselectall_Latt();
						else if(G.obedit->type==OB_ARMATURE)
							deselectall_armature(1);	// 1 == toggle
					}
					else if (ob && (ob->flag & OB_POSEMODE)){
						deselectall_posearmature(ob, 1);
					}
					else {
						if(G.f & G_FACESELECT) deselectall_tface();
						else {
							/* by design, the center of the active object 
							 * (which need not necessarily by selected) will
							 * still be drawn as if it were selected.
							 */
							deselectall();
						}
					}
				}
				break;
			case BKEY:
				if(G.qual==LR_ALTKEY)
					view3d_edit_clipping(v3d);
				else if(G.qual==LR_SHIFTKEY)
					set_render_border();
				else if(G.qual==LR_CTRLKEY) {
					if(okee("Bake all selected")) {
						extern void softbody_bake(Object *ob);
						extern void fluidsimBake(Object *ob);
						softbody_bake(NULL);
						// also bake first domain of selected objects...
						fluidsimBake(NULL);
					}
				}
				else if(G.qual==0)
					borderselect();
				break;
			case CKEY:
				if(G.qual==LR_CTRLKEY) {
					if(ob && (ob->flag & OB_POSEMODE))
						pose_copy_menu();	/* poseobject.c */
					else
						copy_attr_menu();
				}
				else if(G.qual==LR_ALTKEY) {
					if(ob && (ob->flag & OB_POSEMODE))
						pose_clear_constraints();	/* poseobject.c */
					else
						convertmenu();	/* editobject.c */
				}
				else if(G.qual==(LR_ALTKEY|LR_CTRLKEY)) 
					add_constraint(0);	/* editconstraint.c, generic for objects and posemode */
				else if((G.qual==LR_SHIFTKEY)) {
					view3d_home(1);
					curs= give_cursor();
					curs[0]=curs[1]=curs[2]= 0.0;
					allqueue(REDRAWVIEW3D, 0);
				}
				else if((G.obedit) && ELEM(G.obedit->type, OB_CURVE, OB_SURF) ) {
					makecyclicNurb();
					DAG_object_flush_update(G.scene, G.obedit, OB_RECALC_DATA);
					allqueue(REDRAWVIEW3D, 0);
				}
				else if((G.qual==0)){
					curs= give_cursor();
					G.vd->ofs[0]= -curs[0];
					G.vd->ofs[1]= -curs[1];
					G.vd->ofs[2]= -curs[2];
					scrarea_queue_winredraw(curarea);
				}
			
				break;
			case DKEY:
				if((G.qual==LR_SHIFTKEY)) {
					duplicate_context_selected();
				}
				else if(G.qual==LR_ALTKEY) {
					if(ob && (ob->flag & OB_POSEMODE))
						error ("Duplicate not possible in posemode.");
					else if((G.obedit==NULL))
						adduplicate(0, 0);
				}
				else if(G.qual==LR_CTRLKEY) {
					imagestodisplist();
				}
				else if((G.qual==0)){
					pupval= pupmenu("Draw mode%t|BoundBox %x1|Wire %x2|OpenGL Solid %x3|Shaded Solid %x4|Textured Solid %x5");
					if(pupval>0) {
						G.vd->drawtype= pupval;
						doredraw= 1;
					
					}
				}
				
				break;
			case EKEY:
				if (G.qual==0){
					if(G.obedit) {
						if(G.obedit->type==OB_MESH)
							extrude_mesh();
						else if(G.obedit->type==OB_CURVE)
							addvert_Nurb('e');
						else if(G.obedit->type==OB_SURF)
							extrude_nurb();
						else if(G.obedit->type==OB_ARMATURE)
							extrude_armature(0);
					}
				}
				else if (G.qual==LR_CTRLKEY) {
					if(G.obedit && G.obedit->type==OB_MESH)
						Edge_Menu();
					else if (G.f & G_FACESELECT)
						seam_mark_clear_tface(0);
				}
				else if (G.qual==LR_SHIFTKEY) {
					if (G.obedit && G.obedit->type==OB_MESH) {
						initTransform(TFM_CREASE, CTX_EDGE);
						Transform();
					}
					else if (G.obedit && G.obedit->type==OB_ARMATURE) {
						extrude_armature(1);
					}
				}
				break;
			case FKEY:
				if(G.obedit) {
					if(G.obedit->type==OB_MESH) {
						if((G.qual==LR_SHIFTKEY))
							fill_mesh();
						else if(G.qual==LR_ALTKEY)
							beauty_fill();
						else if(G.qual==LR_CTRLKEY)
							edge_flip();
						else if (G.qual==0)
							addedgeface_mesh();
						else if ( G.qual == 
							 (LR_SHIFTKEY | LR_ALTKEY | LR_CTRLKEY) ) {
							select_linked_flat_faces();
						}

					}
					else if ELEM(G.obedit->type, OB_CURVE, OB_SURF) addsegment_nurb();
				}
				else if(G.qual==LR_CTRLKEY)
					sort_faces();
				else if((G.qual==LR_SHIFTKEY)) {
					if(ob && (ob->flag & OB_POSEMODE))
					   pose_activate_flipped_bone();
					else if(G.f & G_WEIGHTPAINT)
						pose_activate_flipped_bone();
					else
						fly();
				}
				else {
					set_faceselect();
				}
				
				break;
			case GKEY:
				if(G.qual == LR_CTRLKEY) 
					group_operation_with_menu();
				else if((G.qual==LR_SHIFTKEY))
					if(G.obedit) {
						if(G.obedit->type==OB_MESH)
							select_mesh_group_menu();
					} else
						select_object_grouped_menu();
				else if(G.qual==LR_ALTKEY) {
					if(okee("Clear location")) {
						clear_object('g');
					}
				}
				else if(G.qual== (LR_CTRLKEY|LR_ALTKEY)) {
					v3d->twtype= V3D_MANIP_TRANSLATE;
					doredraw= 1;
				}
				else if((G.qual==0)) {
					initTransform(TFM_TRANSLATION, CTX_NONE);
					Transform();
				}
				break;
			case HKEY:
				if(G.obedit) {
					if(G.obedit->type==OB_MESH) {
						if(G.qual==LR_CTRLKEY)
							add_hook();
						else if(G.qual==LR_ALTKEY)
							reveal_mesh();
						else if((G.qual==LR_SHIFTKEY))
							hide_mesh(1);
						else if((G.qual==0)) 
							hide_mesh(0);
					}
					else if(G.obedit->type== OB_SURF) {
						if(G.qual==LR_CTRLKEY)
							add_hook();
						else if(G.qual==LR_ALTKEY)
							revealNurb();
						else if((G.qual==LR_SHIFTKEY))
							hideNurb(1);
						else if((G.qual==0))
							hideNurb(0);
					}
					else if(G.obedit->type==OB_CURVE) {
						if(G.qual==LR_CTRLKEY)
							add_hook();
						else {
							if(G.qual==LR_CTRLKEY)
								autocalchandlesNurb_all(1);	/* flag=1, selected */
							else if((G.qual==LR_SHIFTKEY))
								sethandlesNurb(1);
							else if((G.qual==0))
								sethandlesNurb(3);
							
							DAG_object_flush_update(G.scene, G.obedit, OB_RECALC_DATA);
							BIF_undo_push("Handle change");
							allqueue(REDRAWVIEW3D, 0);
						}
					}
					else if(G.obedit->type==OB_LATTICE) {
						if(G.qual==LR_CTRLKEY) add_hook();
					}
					else if(G.obedit->type==OB_MBALL) {
						if(G.qual==LR_ALTKEY)
							reveal_mball();
						else if((G.qual==LR_SHIFTKEY))
							hide_mball(1);
						else if((G.qual==0)) 
							hide_mball(0);
					}
					else if(G.obedit->type==OB_ARMATURE) {
						if (G.qual==0)
							hide_selected_armature_bones();
						else if (G.qual==LR_SHIFTKEY)
							hide_unselected_armature_bones();
						else if (G.qual==LR_ALTKEY)
							show_all_armature_bones();
					}
				}
				else if(G.f & G_FACESELECT)
					hide_tface();
				else if(ob && (ob->flag & OB_POSEMODE)) {
					if (G.qual==0)
						hide_selected_pose_bones();
					else if (G.qual==LR_SHIFTKEY)
						hide_unselected_pose_bones();
					else if (G.qual==LR_ALTKEY)
						show_all_pose_bones();
				}
				break;
			case IKEY:
				if(G.obedit);
				else if(G.qual==LR_CTRLKEY) {
					if(ob && ob->type==OB_ARMATURE) 
						if(ob->flag & OB_POSEMODE) 
							pose_add_IK();
				}
				else if(G.qual==LR_ALTKEY) {
					if(ob && ob->type==OB_ARMATURE) 
						if(ob->flag & OB_POSEMODE) 
							pose_clear_IK();
				}
				break;
				
			case JKEY:
				if(G.qual==LR_CTRLKEY) {
					if( ob ) {
						join_menu();
					}
					else if ((G.obedit) && ELEM(G.obedit->type, OB_CURVE, OB_SURF))
						addsegment_nurb();
				}
				else if(G.obedit) {
					if(G.obedit->type==OB_MESH) {
						join_triangles();
					}
				}

				break;
			case KKEY:
				if(G.obedit) {
					if (G.obedit->type==OB_MESH) {
						if (G.qual==LR_SHIFTKEY)
							KnifeSubdivide(KNIFE_PROMPT);
						else if (G.qual==0)
							LoopMenu();
					}
					else if(G.obedit->type==OB_SURF)
						printknots();
				}
				else {
					if((G.qual==LR_SHIFTKEY)) {
						if(G.f & G_FACESELECT)
							if (G.f & G_WEIGHTPAINT)
								clear_wpaint_selectedfaces();
							else
								clear_vpaint_selectedfaces();
						else if(G.f & G_VERTEXPAINT)
							clear_vpaint();
						else
							select_select_keys();
					}
					else if (G.qual==0)
						set_ob_ipoflags();
				}
				
				break;
			case LKEY:
				if(G.obedit) {
					if(G.obedit->type==OB_MESH)
						selectconnected_mesh(G.qual);
					if(G.obedit->type==OB_ARMATURE)
						selectconnected_armature();
					else if ELEM(G.obedit->type, OB_CURVE, OB_SURF)
						selectconnected_nurb();
				}
				else if(ob && (ob->flag & OB_POSEMODE)) {
					selectconnected_posearmature();
				}
				else {
					if(G.f & G_FACESELECT) {
						if((G.qual==0))
							select_linked_tfaces(0);
						else if((G.qual==LR_SHIFTKEY))
							select_linked_tfaces(1);
						else if(G.qual==LR_CTRLKEY)
							select_linked_tfaces(2);
					}
					else {
						if((G.qual==0))
							make_local_menu();
						else if((G.qual==LR_SHIFTKEY))
							selectlinks_menu();
						else if(G.qual==LR_CTRLKEY)
							make_links_menu();
					}
				}
				break;
 			case MKEY:
				if((G.obedit==0) && (G.f & G_FACESELECT) && (G.qual==0))
					mirror_uv_tface();
				else if(G.obedit){
					if(G.qual==LR_ALTKEY) {
						if(G.obedit->type==OB_MESH) {
							mergemenu();
							DAG_object_flush_update(G.scene, G.obedit, OB_RECALC_DATA);
						}
					}
					else if((G.qual==0) || (G.qual==LR_CTRLKEY)) {
						mirrormenu();
					}
					if ( G.qual == (LR_SHIFTKEY | LR_ALTKEY | LR_CTRLKEY) ) {
						if(G.obedit->type==OB_MESH) select_non_manifold();
					}
				}
				else if(G.qual & LR_CTRLKEY) {
					mirrormenu();
				}
				else if(G.qual==0) {
					if(ob && (ob->flag & OB_POSEMODE))
						pose_movetolayer();
					else
						movetolayer();
				}
 				break;
			case NKEY:
				if((G.qual==0)) {
					toggle_blockhandler(curarea, VIEW3D_HANDLER_OBJECT, UI_PNL_TO_MOUSE);
					allqueue(REDRAWVIEW3D, 0);
				}
				else if(G.obedit) {
					switch (G.obedit->type){
					case OB_ARMATURE:
						if(G.qual==LR_CTRLKEY){
							if (okee("Recalculate bone roll angles")) {
								auto_align_armature();
								allqueue(REDRAWVIEW3D, 0);
							}
						}
						break;
					case OB_MESH: 
						if(G.qual==(LR_SHIFTKEY|LR_CTRLKEY)) {
							if(okee("Recalculate normals inside")) {
								righthandfaces(2);
								allqueue(REDRAWVIEW3D, 0);
								BIF_undo_push("Recalculate normals inside");
							}
						}
						else if(G.qual==LR_CTRLKEY){
							if(okee("Recalculate normals outside")) {
								righthandfaces(1);
								allqueue(REDRAWVIEW3D, 0);
								BIF_undo_push("Recalculate normals outside");
							}
						}
						break;
					}
				}
				
				break;
			case OKEY:
				if (G.obedit) {
					if (G.qual==LR_SHIFTKEY) {
						G.scene->prop_mode = (G.scene->prop_mode+1)%6;
						allqueue(REDRAWHEADERS, 0);
					}
					else if((G.qual==LR_ALTKEY)) {
						if(G.scene->proportional==2) G.scene->proportional= 1;
						else G.scene->proportional= 2;
						allqueue(REDRAWHEADERS, 0);
					}
					else if((G.qual==0)) {
						G.scene->proportional= !G.scene->proportional;
						allqueue(REDRAWHEADERS, 0);
					}
				}
				else if((G.qual==LR_SHIFTKEY)) {
					if(ob && ob->type == OB_MESH) {
						flip_subdivison(ob, -1);
					}
				}
				else if(G.qual==LR_ALTKEY) {
					if(okee("Clear origin")) {
						clear_object('o');
					}
				}
				break;

			case PKEY:
				if(G.obedit) {
					if(G.qual==LR_CTRLKEY || G.qual==(LR_SHIFTKEY|LR_CTRLKEY)) {
						if(G.obedit->type==OB_ARMATURE)
							make_bone_parent();
						else
							make_parent();
					}
					else if(G.qual==LR_ALTKEY && G.obedit->type==OB_ARMATURE)
						clear_bone_parent();
					else if((G.qual==0) && G.obedit->type==OB_MESH)
						separatemenu();
					else if ((G.qual==0) && ELEM(G.obedit->type, OB_CURVE, OB_SURF))
						separate_nurb();
					else if (G.qual==LR_SHIFTKEY) {
						initTransform(TFM_PUSHPULL, CTX_NONE);
						Transform();
					}
				}
				else if(G.qual==LR_CTRLKEY || G.qual==(LR_SHIFTKEY|LR_CTRLKEY))
					make_parent();
				else if(G.qual==LR_SHIFTKEY) {
					if(G.obedit) {
						initTransform(TFM_PUSHPULL, CTX_NONE);
						Transform();
					}
					else {
						toggle_blockhandler(curarea, VIEW3D_HANDLER_PREVIEW, 0);
						doredraw= 1;
					}
				}
				else if(G.qual==LR_ALTKEY)
					clear_parent();
				else if((G.qual==0)) {
                	start_game();
				}
				break;				
			case RKEY:
				if((G.obedit==0) && (G.f & G_FACESELECT) && (G.qual==0))
					rotate_uv_tface();
				else if(G.qual==LR_ALTKEY) {
					if(okee("Clear rotation")) {
						clear_object('r');
					}
				} 
				else if(G.qual== (LR_CTRLKEY|LR_ALTKEY)) {
					v3d->twtype= V3D_MANIP_ROTATE;
					doredraw= 1;
				}
				else if (G.obedit) {
					if((G.qual==LR_SHIFTKEY)) {
						if ELEM(G.obedit->type,  OB_CURVE, OB_SURF)					
							selectrow_nurb();
					}
					else if(G.qual==LR_CTRLKEY) {
						if (G.obedit->type==OB_MESH)
							CutEdgeloop(1);
							BIF_undo_push("Cut Edgeloop");
					}
					else if((G.qual==0)) {
						initTransform(TFM_ROTATION, CTX_NONE);
						Transform();
					}
				}
				else if((G.qual==0)) {
					initTransform(TFM_ROTATION, CTX_NONE);
					Transform();
				}
				break;
			case SKEY:
				if(G.qual== (LR_CTRLKEY|LR_ALTKEY)) {
					v3d->twtype= V3D_MANIP_SCALE;
					doredraw= 1;
				}
				else if(G.obedit) {
					
					if(G.qual==LR_ALTKEY) {
						if(G.obedit->type==OB_ARMATURE) {
							initTransform(TFM_BONESIZE, CTX_NONE);
						}
						else
							initTransform(TFM_SHRINKFATTEN, CTX_NONE);
						Transform();
					}
					else if(G.qual==LR_CTRLKEY) {
						initTransform(TFM_SHEAR, CTX_NONE);
						Transform();
					}
					else if(G.qual==LR_SHIFTKEY)
						snapmenu();
					else if(G.qual==0) {
						if(G.obedit->type==OB_ARMATURE) {
							bArmature *arm= G.obedit->data;
							if(arm->drawtype==ARM_ENVELOPE)
								initTransform(TFM_BONE_ENVELOPE, CTX_NONE);
							else
								initTransform(TFM_RESIZE, CTX_NONE);
						}
						else
							initTransform(TFM_RESIZE, CTX_NONE);
						Transform();
					}
					else if(G.qual==(LR_SHIFTKEY|LR_CTRLKEY)){
						initTransform(TFM_TOSPHERE, CTX_NONE);
						Transform();
					}
					if ( G.qual == (LR_SHIFTKEY | LR_ALTKEY | LR_CTRLKEY) ) {
						if(G.obedit->type==OB_MESH) select_sharp_edges();
					}
				}
				else if(G.qual==LR_ALTKEY) {
					if(G.f & G_WEIGHTPAINT)
						ob= ob->parent;
					if(ob && (ob->flag & OB_POSEMODE)) {
						bArmature *arm= ob->data;
						if( ELEM(arm->drawtype, ARM_B_BONE, ARM_ENVELOPE)) {
							initTransform(TFM_BONESIZE, CTX_NONE);
							Transform();
							break;
						}
					}
					
					if(okee("Clear size")) {
						clear_object('s');
					}
				}
				else if(G.qual==LR_SHIFTKEY) {
					snapmenu();
				}
				else if((G.qual==0)) {
					initTransform(TFM_RESIZE, CTX_NONE);
					Transform();
				}
				else if(G.qual==(LR_SHIFTKEY|LR_CTRLKEY)) {
					initTransform(TFM_TOSPHERE, CTX_NONE);
					Transform();
				}
				else if(G.qual==(LR_CTRLKEY|LR_ALTKEY|LR_SHIFTKEY)) {
					initTransform(TFM_SHEAR, CTX_NONE);
					Transform();
				}
				break;
			case TKEY:
				if(G.obedit){
					if((G.qual & LR_CTRLKEY) && G.obedit->type==OB_MESH) {
						convert_to_triface(G.qual & LR_SHIFTKEY);
						allqueue(REDRAWVIEW3D, 0);
						countall();
						DAG_object_flush_update(G.scene, G.obedit, OB_RECALC_DATA);
					}
					if (G.obedit->type==OB_CURVE) {
						if (G.qual==LR_ALTKEY) {
							clear_tilt();
						}
						else if (G.qual==0) {
							initTransform(TFM_TILT, CTX_NONE);
							Transform();
						}
					}
				}
				else if(G.qual==LR_CTRLKEY) {
					if(ob && (ob->flag & OB_POSEMODE));
					else make_track();
				}
				else if(G.qual==LR_ALTKEY) {
					if(ob && (ob->flag & OB_POSEMODE));
					else clear_track();
				}
				else if((G.qual==0)){
					texspace_edit();
				}
				
				break;
			case UKEY:
				if(G.obedit) {
					if(G.obedit->type==OB_MESH) {
						if(G.qual==0) BIF_undo(); else BIF_redo();
					}
					else if ELEM5(G.obedit->type, OB_CURVE, OB_SURF, OB_MBALL, OB_LATTICE, OB_ARMATURE) {
						if(G.qual==0) BIF_undo(); else BIF_redo();
					}
				}
				else if((G.qual==0)) {
					if (G.f & G_FACESELECT)
						uv_autocalc_tface();
					else if(G.f & G_WEIGHTPAINT)
						wpaint_undo();
					else if(G.f & G_VERTEXPAINT)
						vpaint_undo();
					else {
						single_user();
					}
				}
					
				break;
			case VKEY:
				if((G.qual==LR_SHIFTKEY)) {
					if ((G.obedit) && G.obedit->type==OB_MESH) {
						align_view_to_selected(v3d);
					}
					else if (G.f & G_FACESELECT) {
						align_view_to_selected(v3d);
					}
				}
				else if(G.qual==LR_ALTKEY)
					image_aspect();
				else if (G.qual==0){
					if(G.obedit) {
						if(G.obedit->type==OB_MESH) {
							mesh_rip();
						}
						else if(G.obedit->type==OB_CURVE) {
							sethandlesNurb(2);
							DAG_object_flush_update(G.scene, G.obedit, OB_RECALC_DATA);
							allqueue(REDRAWVIEW3D, 0);
							BIF_undo_push("Handle change");
						}
					}
					else if(ob && ob->type == OB_MESH) 
						set_vpaint();
				}
				break;
			case WKEY:
				if((G.qual==LR_SHIFTKEY)) {
					initTransform(TFM_WARP, CTX_NONE);
					Transform();
				}
				else if(G.qual==LR_ALTKEY) {
					/* if(G.obedit && G.obedit->type==OB_MESH) write_videoscape(); */
				}
				else if(G.qual==LR_CTRLKEY) {
					if(G.obedit) {
						if ELEM(G.obedit->type,  OB_CURVE, OB_SURF) {
							switchdirectionNurb2();
						}
					}
				}
				else if((G.qual==0))
					special_editmenu();
				
				break;
			case XKEY:
			case DELKEY:
				if(G.qual==0)
					delete_context_selected();
				break;
			case YKEY:
				if((G.qual==0) && (G.obedit)) {
					if(G.obedit->type==OB_MESH) split_mesh();
				}
				break;
			case ZKEY:
				toggle_shading();
				
				scrarea_queue_headredraw(curarea);
				scrarea_queue_winredraw(curarea);
				break;
			
			case HOMEKEY:
				if(G.qual==0)
					view3d_home(0);
				break;
			case COMMAKEY:
				if(G.qual==LR_CTRLKEY) {
					G.vd->around= V3D_CENTROID;
				} else if(G.qual==LR_SHIFTKEY) {
					G.vd->around= V3D_CENTROID;
				} else if(G.qual==0) {
					G.vd->around= V3D_CENTRE;
				}
				handle_view3d_around();
				
				scrarea_queue_headredraw(curarea);
				scrarea_queue_winredraw(curarea);
				break;
				
			case PERIODKEY:
				if(G.qual==LR_CTRLKEY) {
					G.vd->around= V3D_LOCAL;
				} 	else if(G.qual==0) {
					G.vd->around= V3D_CURSOR;
				}
				handle_view3d_around();
				
				scrarea_queue_headredraw(curarea);
				scrarea_queue_winredraw(curarea);
				break;
			
			case PADSLASHKEY:
				if(G.qual==0) {
					if(G.vd->localview) {
						G.vd->localview= 0;
						endlocalview(curarea);
					}
					else {
						G.vd->localview= 1;
						initlocalview();
					}
					scrarea_queue_headredraw(curarea);
				}
				break;
			case PADASTERKEY:	/* '*' */
				if(G.qual==0) {
					if(ob) {
						if ((G.obedit) && (G.obedit->type == OB_MESH)) {
							editmesh_align_view_to_selected(G.vd, 2);
						} 
						else if (G.f & G_FACESELECT) {
							if(ob->type==OB_MESH) {
								Mesh *me= ob->data;
								faceselect_align_view_to_selected(G.vd, me, 2);
							}
						}
						else
							obmat_to_viewmat(ob);
						
						if(G.vd->persp==2) G.vd->persp= 1;
						scrarea_queue_winredraw(curarea);
					}
				}
				break;
			case PADPERIOD:	/* '.' */
				if(G.qual==0)
					centreview();
				break;
			
			case PAGEUPKEY:
				if(G.qual==LR_CTRLKEY)
					movekey_obipo(1);
				else if((G.qual==0))
					nextkey_obipo(1);	/* in editipo.c */
				break;

			case PAGEDOWNKEY:
				if(G.qual==LR_CTRLKEY)
					movekey_obipo(-1);
				else if((G.qual==0))
					nextkey_obipo(-1);
				break;
				
			case PAD0: case PAD1: case PAD2: case PAD3: case PAD4:
			case PAD5: case PAD6: case PAD7: case PAD8: case PAD9:
			case PADENTER:
				persptoetsen(event);
				doredraw= 1;
				break;
			case PADMINUS:
				if ( (G.qual==LR_CTRLKEY)
					 && (G.obedit) && (G.obedit->type==OB_MESH) )
					select_less();
				else {
					persptoetsen(event);
					doredraw= 1;
				}
				break;

			case PADPLUSKEY:
				if ( (G.qual==LR_CTRLKEY)
					 && (G.obedit) && (G.obedit->type==OB_MESH) )
					select_more();
				else {
					persptoetsen(event);
					doredraw= 1;
				}
				break;

			case ESCKEY:
				if(G.qual==0) {
					if (G.vd->flag & V3D_DISPIMAGE) {
						G.vd->flag &= ~V3D_DISPIMAGE;
						doredraw= 1;
					}
				}
				break;
			}
		}
	}
	
	if(doredraw) {
		scrarea_queue_winredraw(curarea);
		scrarea_queue_headredraw(curarea);
	}
}

static void initview3d(ScrArea *sa)
{
	View3D *vd;
	
	vd= MEM_callocN(sizeof(View3D), "initview3d");
	BLI_addhead(&sa->spacedata, vd);	/* addhead! not addtail */

	vd->spacetype= SPACE_VIEW3D;
	vd->blockscale= 0.7f;
	vd->viewquat[0]= 1.0f;
	vd->viewquat[1]= vd->viewquat[2]= vd->viewquat[3]= 0.0f;
	vd->persp= 1;
	vd->drawtype= OB_WIRE;
	vd->view= 7;
	vd->dist= 10.0;
	vd->lens= 35.0f;
	vd->near= 0.01f;
	vd->far= 500.0f;
	vd->grid= 1.0f;
	vd->gridlines= 16;
	vd->lay= vd->layact= 1;
	if(G.scene) {
		vd->lay= vd->layact= G.scene->lay;
		vd->camera= G.scene->camera;
	}
	vd->scenelock= 1;
	vd->gridflag |= V3D_SHOW_X;
	vd->gridflag |= V3D_SHOW_Y;
	vd->gridflag |= V3D_SHOW_FLOOR;
	vd->gridflag &= ~V3D_SHOW_Z;
}


/* ******************** SPACE: IPO ********************** */

static void changeview2dspace(ScrArea *sa, void *spacedata)
{
	if(G.v2d==0) return;

	test_view2d(G.v2d, curarea->winx, curarea->winy);
	myortho2(G.v2d->cur.xmin, G.v2d->cur.xmax, G.v2d->cur.ymin, G.v2d->cur.ymax);
}

static void winqreadipospace(ScrArea *sa, void *spacedata, BWinEvent *evt)
{
	extern void do_ipobuts(unsigned short event); 	// drawipo.c
	unsigned short event= evt->event;
	short val= evt->val;
	SpaceIpo *sipo= curarea->spacedata.first;
	View2D *v2d= &sipo->v2d;
	float dx, dy;
	int cfra, doredraw= 0;
	short mval[2];
	short mousebut = L_MOUSE;
	
	if(sa->win==0) return;

	if(val) {
		if( uiDoBlocks(&sa->uiblocks, event)!=UI_NOTHING ) event= 0;

		/* swap mouse buttons based on user preference */
		if (U.flag & USER_LMOUSESELECT) {
			if (event == LEFTMOUSE) {
				event = RIGHTMOUSE;
				mousebut = L_MOUSE;
			} else if (event == RIGHTMOUSE) {
				event = LEFTMOUSE;
				mousebut = R_MOUSE;
			}
		}

		switch(event) {
		case UI_BUT_EVENT:
			/* note: bad bad code, will be cleaned! is because event queues are all shattered */
			if(val>0 && val < 256) do_ipowin_buts(val-1);
			else do_ipobuts(val);
			break;
			
		case LEFTMOUSE:
			if( in_ipo_buttons() ) {
				do_ipo_selectbuttons();
				doredraw= 1;
			}
			else if(view2dmove(LEFTMOUSE));	// only checks for sliders
			else if(G.qual & LR_CTRLKEY) add_vert_ipo();
			else {
				do {
					getmouseco_areawin(mval);
					areamouseco_to_ipoco(v2d, mval, &dx, &dy);
					
					cfra= (int)dx;
					if(cfra< 1) cfra= 1;
					
					if( cfra!=CFRA ) {
						CFRA= cfra;
						update_for_newframe_nodraw(1);	/* 1 = nosound */
						force_draw_all(0); /* To make constraint sliders redraw */
					}
					else PIL_sleep_ms(30);
				
				} while(get_mbut() & mousebut);
			}
			break;
		case RIGHTMOUSE:
			mouse_select_ipo();
			allqueue (REDRAWACTION, 0);
			allqueue(REDRAWNLA, 0);
			break;
		case MIDDLEMOUSE:
			if(in_ipo_buttons()) {
				scroll_ipobuts();
			}
			else view2dmove(event);	/* in drawipo.c */
			break;
		case WHEELUPMOUSE:
		case WHEELDOWNMOUSE:
			view2dmove(event);	/* in drawipo.c */
			break;
		case PADPLUSKEY:
			view2d_zoom(v2d, 0.1154f, sa->winx, sa->winy);
			doredraw= 1;
			break;
		case PADMINUS:
			view2d_zoom(v2d, -0.15f, sa->winx, sa->winy);
			doredraw= 1;
			break;
		case PAGEUPKEY:
			if(G.qual==LR_CTRLKEY)
				movekey_ipo(1);
			else if((G.qual==0))
				nextkey_ipo(1);
			break;
		case PAGEDOWNKEY:
			if(G.qual==LR_CTRLKEY)
				movekey_ipo(-1);
			else if((G.qual==0))
				nextkey_ipo(-1);
			break;
		case HOMEKEY:
			if((G.qual==0))
				do_ipo_buttons(B_IPOHOME);
			break;
			
		case AKEY:
			if((G.qual==0)) {
				if(in_ipo_buttons()) {
					swap_visible_editipo();
				}
				else {
					swap_selectall_editipo();
				}
				allspace (REMAKEIPO, 0);
				allqueue (REDRAWNLA, 0);
				allqueue (REDRAWACTION, 0);
			}
			break;
		case BKEY:
			if((G.qual==0))
				borderselect_ipo();
			break;
		case CKEY:
			if((G.qual==0))
				move_to_frame();
			break;
		case DKEY:
			if((G.qual==LR_SHIFTKEY))
				add_duplicate_editipo();
			break;
		case GKEY:
			if((G.qual==0))
				transform_ipo('g');
			break;
		case HKEY:
			if(G.qual==LR_ALTKEY)
				sethandles_ipo(HD_AUTO_ANIM);
			if(G.qual==LR_SHIFTKEY)
				sethandles_ipo(HD_AUTO);
			else if(G.qual==0)
				sethandles_ipo(HD_ALIGN);
			break;
		case JKEY:
			if((G.qual==0))
				join_ipo_menu();
			break;
		case KKEY:
			if((G.qual==0)) {
				ipo_toggle_showkey();
				scrarea_queue_headredraw(curarea);
				allqueue(REDRAWVIEW3D, 0);
				doredraw= 1;
			}
			break;
		case NKEY:
			toggle_blockhandler(sa, IPO_HANDLER_PROPERTIES, UI_PNL_TO_MOUSE);
			doredraw= 1;
			break;
		case RKEY:
			if((G.qual==0))
				ipo_record();
			break;
		case SKEY:
			if((G.qual==LR_SHIFTKEY)) {		
				ipo_snap_menu();
			} else if((G.qual==0))
				transform_ipo('s');
			break;
		case TKEY:
			if((G.qual==0))
				set_ipotype();
			break;
		case VKEY:
			if((G.qual==0))
				sethandles_ipo(HD_VECT);
			break;
		case XKEY:
		case DELKEY:
			del_ipo();
			break;
		}
	}

	if(doredraw) scrarea_queue_winredraw(sa);
}

void initipo(ScrArea *sa)
{
	SpaceIpo *sipo;
	
	sipo= MEM_callocN(sizeof(SpaceIpo), "initipo");
	BLI_addhead(&sa->spacedata, sipo);

	sipo->spacetype= SPACE_IPO;
	sipo->blockscale= 0.7f;
	
	/* sipo space loopt van (0,-?) tot (??,?) */
	sipo->v2d.tot.xmin= 0.0;
	sipo->v2d.tot.ymin= -10.0;
	sipo->v2d.tot.xmax= G.scene->r.efra;
	sipo->v2d.tot.ymax= 10.0;

	sipo->v2d.cur= sipo->v2d.tot;

	sipo->v2d.min[0]= 0.01f;
	sipo->v2d.min[1]= 0.01f;

	sipo->v2d.max[0]= 15000.0f;
	sipo->v2d.max[1]= 10000.0f;
	
	sipo->v2d.scroll= L_SCROLL+B_SCROLL;
	sipo->v2d.keeptot= 0;

	sipo->blocktype= ID_OB;
}

/* ******************** SPACE: INFO ********************** */

void space_mipmap_button_function(int event) {
	set_mipmap(!(U.gameflags & USER_DISABLE_MIPMAP));

	allqueue(REDRAWVIEW3D, 0);
}

#if 0
static void space_sound_button_function(int event)
{
	int a;
	SYS_SystemHandle syshandle;

	if ((syshandle = SYS_GetSystem()))
	{
		a = (U.gameflags & USER_DISABLE_SOUND);
		SYS_WriteCommandLineInt(syshandle, "noaudio", a);
	}
}
#endif

// needed for event; choose new 'curmain' resets it...
static short th_curcol= TH_BACK;
static char *th_curcol_ptr= NULL;
static char th_curcol_arr[4]={0, 0, 0, 255};

static void info_user_themebuts(uiBlock *block, short y1, short y2, short y3)
{
	bTheme *btheme, *bt;
	int spacetype= 0;
	static short cur=1, curmain=2;
	short a, tot=0, isbuiltin= 0;
	char string[21*32], *strp, *col;
	
	y3= y2+23;	// exception!
	
	/* count total, max 16! */
	for(bt= U.themes.first; bt; bt= bt->next) tot++;
	
	/* if cur not is 1; move that to front of list */
	if(cur!=1) {
		a= 1;
		for(bt= U.themes.first; bt; bt= bt->next, a++) {
			if(a==cur) {
				BLI_remlink(&U.themes, bt);
				BLI_addhead(&U.themes, bt);
				allqueue(REDRAWALL, 0);
				cur= 1;
				break;
			}
		}
	}
	
	/* the current theme */
	btheme= U.themes.first;
	if(strcmp(btheme->name, "Default")==0) isbuiltin= 1;

	/* construct popup script */
	string[0]= 0;
	for(bt= U.themes.first; bt; bt= bt->next) {
		strcat(string, bt->name);
		if(btheme->next) strcat(string, "   |");
	}
	uiDefButS(block, MENU, B_UPDATE_THEME, string, 			45,y3,200,20, &cur, 0, 0, 0, 0, "Current theme");
	
	/* add / delete / name */

	if(tot<16)
		uiDefBut(block, BUT, B_ADD_THEME, "Add", 	45,y2,200,20, NULL, 0, 0, 0, 0, "Makes new copy of this theme");
	if(tot>1 && isbuiltin==0)
		uiDefBut(block, BUT, B_DEL_THEME, "Delete", 45,y1,200,20, NULL, 0, 0, 0, 0, "Delete theme");

	if(isbuiltin) return;
	
	/* name */
	uiDefBut(block, TEX, B_NAME_THEME, "", 			255,y3,200,20, btheme->name, 1.0, 30.0, 0, 0, "Rename theme");

	/* main choices pup */
	uiDefButS(block, MENU, B_CHANGE_THEME, "UI and Buttons %x1|%l|3D View %x2|%l|Ipo Curve Editor %x3|Action Editor %x4|"
		"NLA Editor %x5|%l|UV/Image Editor %x6|Video Sequence Editor %x7|Node Editor %x16|Timeline %x15|Audio Window %x8|Text Editor %x9|%l|User Preferences %x10|"
		"Outliner %x11|Buttons Window %x12|%l|File Browser %x13|Image Browser %x14",
													255,y2,200,20, &curmain, 0, 0, 0, 0, "Specify theme for...");
	if(curmain==1) spacetype= 0;
	else if(curmain==2) spacetype= SPACE_VIEW3D;
	else if(curmain==3) spacetype= SPACE_IPO;
	else if(curmain==4) spacetype= SPACE_ACTION;
	else if(curmain==5) spacetype= SPACE_NLA;
	else if(curmain==6) spacetype= SPACE_IMAGE;
	else if(curmain==7) spacetype= SPACE_SEQ;
	else if(curmain==8) spacetype= SPACE_SOUND;
	else if(curmain==9) spacetype= SPACE_TEXT;
	else if(curmain==10) spacetype= SPACE_INFO;
	else if(curmain==11) spacetype= SPACE_OOPS;
	else if(curmain==12) spacetype= SPACE_BUTS;
	else if(curmain==13) spacetype= SPACE_FILE;
	else if(curmain==14) spacetype= SPACE_IMASEL;
	else if(curmain==15) spacetype= SPACE_TIME;
	else if(curmain==16) spacetype= SPACE_NODE;
	else return; // only needed while coding... when adding themes for more windows
	
	/* color choices pup */
	if(curmain==1) {
		strp= BIF_ThemeColorsPup(0);
		if(th_curcol==TH_BACK) th_curcol= TH_BUT_OUTLINE;  // switching main choices...
	}
	else strp= BIF_ThemeColorsPup(spacetype);
	
	uiDefButS(block, MENU, B_REDR, strp, 			255,y1,200,20, &th_curcol, 0, 0, 0, 0, "Current color");
	MEM_freeN(strp);
	
	th_curcol_ptr= col= BIF_ThemeGetColorPtr(btheme, spacetype, th_curcol);
	if(col==NULL) return;
	
	/* first handle exceptions, special single values, row selection, etc */
	if(th_curcol==TH_VERTEX_SIZE) {
		uiDefButC(block, NUMSLI, B_UPDATE_THEME,"Vertex size ",	465,y3,200,20,  col, 1.0, 10.0, 0, 0, "");
	}
	else if(th_curcol==TH_FACEDOT_SIZE) {
		uiDefButC(block, NUMSLI, B_UPDATE_THEME,"Face dot size ",	465,y3,200,20,  col, 1.0, 10.0, 0, 0, "");
	}
	else if(th_curcol==TH_BUT_DRAWTYPE) {
		uiBlockBeginAlign(block);
		uiDefButC(block, ROW, B_UPDATE_THEME, "Minimal",	465,y3,100,20,  col, 2.0, 0.0, 0, 0, "");
		uiDefButC(block, ROW, B_UPDATE_THEME, "Shaded",	565,y3,100,20,  col, 2.0, 1.0, 0, 0, "");
		uiDefButC(block, ROW, B_UPDATE_THEME, "Rounded",	465,y2,100,20,  col, 2.0, 2.0, 0, 0, "");
		uiDefButC(block, ROW, B_UPDATE_THEME, "OldSkool",	565,y2,100,20,  col, 2.0, 3.0, 0, 0, "");
		uiBlockEndAlign(block);
	}
	else {
		uiBlockBeginAlign(block);
		if ELEM7(th_curcol, TH_PANEL, TH_LAMP, TH_FACE, TH_FACE_SELECT, TH_MENU_BACK, TH_MENU_HILITE, TH_MENU_ITEM) {
			uiDefButC(block, NUMSLI, B_UPDATE_THEME,"A ",	465,y3+25,200,20,  col+3, 0.0, 255.0, B_THEMECOL, 0, "");
		}
		uiDefButC(block, NUMSLI, B_UPDATE_THEME,"R ",	465,y3,200,20,  col, 0.0, 255.0, B_THEMECOL, 0, "");
		uiDefButC(block, NUMSLI, B_UPDATE_THEME,"G ",	465,y2,200,20,  col+1, 0.0, 255.0, B_THEMECOL, 0, "");
		uiDefButC(block, NUMSLI, B_UPDATE_THEME,"B ",	465,y1,200,20,  col+2, 0.0, 255.0, B_THEMECOL, 0, "");
		uiBlockEndAlign(block);

		uiDefButC(block, COL, B_UPDATE_THEME, "",		675,y1,50,y3-y1+20, col, 0, 0, 0, 0, "");
				
		/* copy paste */
		uiBlockBeginAlign(block);
		uiDefBut(block, BUT, B_THEME_COPY, "Copy Color", 	755,y2,120,20, NULL, 0, 0, 0, 0, "Stores current color in buffer");
		uiDefBut(block, BUT, B_THEME_PASTE, "Paste Color", 	755,y1,120,20, NULL, 0, 0, 0, 0, "Pastes buffer color");
		uiBlockEndAlign(block);
		
		uiDefButC(block, COL, 0, "",				885,y1,50,y2-y1+20, th_curcol_arr, 0, 0, 0, 0, "");
		
	}
}


void drawinfospace(ScrArea *sa, void *spacedata)
{
	uiBlock *block;
	static short cur_light=0, cur_light_var=0;
	float fac, col[3];
	short xpos, ypos, ypostab,  buth, rspace, dx, y1, y2, y3, y4, y5, y6;
	short y2label, y3label, y4label, y5label, y6label;
	short spref, mpref, lpref, smfileselbut;
	short edgsp, midsp;
	char naam[32];

	if(curarea->win==0 || curarea->winy<2) return;

	BIF_GetThemeColor3fv(TH_BACK, col);
	glClearColor(col[0], col[1], col[2], 0.0);
	glClear(GL_COLOR_BUFFER_BIT);

	if(curarea->winx<=1280.0) {
		fac= ((float)curarea->winx)/1280.0f;
		myortho2(0.375f, 1280.375f, 0.375f, curarea->winy/fac + 0.375f);
	}
	else {
		myortho2(0.375f, (float)curarea->winx + 0.375f, 0.375f, (float)curarea->winy + 0.375f);
	}
	
	sprintf(naam, "infowin %d", curarea->win);
	block= uiNewBlock(&curarea->uiblocks, naam, UI_EMBOSS, UI_HELV, curarea->win);


	/* Vars for nice grid alignment */ 
	dx= (1280-90)/7;	/* spacing for use in equally dividing 'tab' row */

	xpos = 45;		/* left padding */
	ypos = 50;		/* bottom padding for buttons */
	ypostab = 10;		/* bottom padding for 'tab' row */

	buth = 20;		/* standard button height */

	spref = 90;	/* standard size for small preferences button */
	mpref = 189;	/* standard size for medium preferences button */
	lpref = 288;	/* standard size for large preferences button */
	smfileselbut = buth;	/* standard size for fileselect button (square) */

	edgsp = 3;		/* space from edge of end 'tab' to edge of end button */
	midsp = 9;		/* horizontal space between buttons */

	rspace = 3;		/* default space between rows */

	y1 = ypos;		/* grid alignment for each row of buttons */
	y2 = ypos+buth+rspace;
	y3 = ypos+2*(buth+rspace);
	y4 = ypos+3*(buth+rspace);
	y5 = ypos+4*(buth+rspace);
	y6 = ypos+5*(buth+rspace);


	y2label = y2-2;		/* adjustments to offset the labels down to align better */
	y3label = y3-2;
	y4label = y4-2;
	y5label = y5-2;
	y6label = y6-2;


	/* set the colour to blue and draw the main 'tab' controls */

	uiBlockSetCol(block, TH_BUT_SETTING1);
	uiBlockBeginAlign(block);
	
	uiDefButS(block, ROW,B_USERPREF,"View & Controls",
		xpos,ypostab,(short)dx,buth,
		&U.userpref,1.0,0.0, 0, 0,"");
		
	uiDefButS(block, ROW,B_USERPREF,"Edit Methods",
		(short)(xpos+dx),ypostab,(short)dx,buth,
		&U.userpref,1.0,1.0, 0, 0,"");

	uiDefButS(block, ROW,B_USERPREF,"Language & Font",
		(short)(xpos+2*dx),ypostab,(short)dx,buth,
		&U.userpref,1.0,2.0, 0, 0,"");

	uiDefButS(block, ROW,B_USERPREF,"Themes",
		(short)(xpos+3*dx),ypostab,(short)dx,buth,
		&U.userpref,1.0,6.0, 0, 0,"");

	uiDefButS(block, ROW,B_USERPREF,"Auto Save",
		(short)(xpos+4*dx),ypostab,(short)dx,buth,
		&U.userpref,1.0,3.0, 0, 0,"");

	uiDefButS(block, ROW,B_USERPREF,"System & OpenGL",
		(short)(xpos+5*dx),ypostab,(short)dx,buth,
		&U.userpref,1.0,4.0, 0, 0,"");
		
	uiDefButS(block, ROW,B_USERPREF,"File Paths",
		(short)(xpos+6*dx),ypostab,(short)dx,buth,
		&U.userpref,1.0,5.0, 0, 0,"");

	uiBlockSetCol(block, TH_AUTO);
	uiBlockEndAlign(block);
	/* end 'tab' controls */

        /* line 2: left x co-ord, top y co-ord, width, height */

	if(U.userpref == 6) {
		info_user_themebuts(block, y1, y2, y3);
	}
	else if (U.userpref == 0) { /* view & controls */

		uiDefBut(block, LABEL,0,"Display:",
			xpos,y6label,spref,buth,
			0, 0, 0, 0, 0, "");	
		uiBlockBeginAlign(block);
		uiDefButBitI(block, TOG, USER_TOOLTIPS, 0, "Tool Tips",
			(xpos+edgsp),y5,spref,buth,
			&(U.flag), 0, 0, 0, 0,
			"Display tooltips (help tags) over buttons");
		uiDefButBitI(block, TOG, USER_DRAWVIEWINFO, B_DRAWINFO, "Object Info",
			(xpos+edgsp),y4,spref,buth,
			&(U.uiflag), 0, 0, 0, 0,
			"Display active object name and frame number in the 3D View");
		uiDefButBitI(block, TOG, USER_SCENEGLOBAL, 0, "Global Scene",
			(xpos+edgsp),y3,spref,buth,
			&(U.flag), 0, 0, 0, 0,
			"Forces the current Scene to be displayed in all Screens");
#ifndef __APPLE__	
		uiDefButBitS(block, TOG, 1, 0, "Large Cursors",
			(xpos+edgsp),y2,spref,buth,
			&(U.curssize), 0, 0, 0, 0,
			"Use large mouse cursors when available");
#else 
		U.curssize=0; /*Small Cursor always for OS X for now */
#endif
		uiDefButBitI(block, TOG, USER_SHOW_VIEWPORTNAME, B_DRAWINFO, "View Name",
			(xpos+edgsp),y1,spref,buth,
			&(U.uiflag), 0, 0, 0, 0,
			"Show the name of the view's direction in each 3D View");
		uiBlockEndAlign(block);

		uiDefBut(block, LABEL,0,"Menus:",
			(xpos+(2*edgsp)+spref),y6label,spref,buth,
			0, 0, 0, 0, 0, "");
		uiBlockBeginAlign(block);
		uiDefButBitI(block, TOG, USER_MENUOPENAUTO, 0, "Open on Mouse Over",
			(xpos+edgsp+spref+midsp),y5,mpref,buth,
			&(U.uiflag), 0, 0, 0, 0,
			"Open menu buttons and pulldowns automatically when the mouse is hovering");
		uiDefButS(block, NUM, 0, "Top Level:",
			(xpos+edgsp+spref+midsp),y4,spref+edgsp,buth,
			&(U.menuthreshold1), 1, 40, 0, 0,
			"Time delay in 1/10 seconds before automatically opening top level menus");
		uiDefButS(block, NUM, 0, "Sublevels:",
			(xpos+edgsp+(2*spref)+(2*midsp)-edgsp),y4,spref+edgsp,buth,
			&(U.menuthreshold2), 1, 40, 0, 0,
			"Time delay in 1/10 seconds before automatically opening menu sublevels");
		uiBlockEndAlign(block);

		uiDefBut(block, LABEL,0,"Toolbox click-hold delay:",
			(xpos+(2*edgsp)+spref),y3label,mpref,buth,
			0, 0, 0, 0, 0, "");
		uiBlockBeginAlign(block);
		uiDefButS(block, NUM, 0, "LMB:",
			(xpos+edgsp+spref+midsp),y2,spref+edgsp,buth,
			&(U.tb_leftmouse), 2, 40, 0, 0,
			"Time in 1/10 seconds to hold the Left Mouse Button before opening the toolbox");
		uiDefButS(block, NUM, 0, "RMB:",
			(xpos+edgsp+(2*spref)+(2*midsp)-edgsp),y2,spref+edgsp,buth,
			&(U.tb_rightmouse), 2, 40, 0, 0,
			"Time in 1/10 seconds to hold the Right Mouse Button before opening the toolbox");	
		uiBlockEndAlign(block);

		uiDefButBitI(block, TOG, USER_PANELPINNED, 0, "Pin Floating Panels",
			(xpos+edgsp+spref+midsp),y1,(mpref/2),buth,
			&(U.uiflag), 0, 0, 0, 0,
			"Make floating panels invoked by a hotkey (eg. N Key) open at the previous location");
		uiDefButBitI(block, TOG, USER_PLAINMENUS, B_PLAINMENUS, "Plain Menus",
			(xpos+edgsp+(2*spref)+(2*midsp)),y1,spref,buth,
			&(U.uiflag), 0, 0, 0, 0,
			"Use a column layout for toolbox and do not flip the contents of any menu");
		uiBlockEndAlign(block);
		
		uiDefBut(block, LABEL,0,"Snap to grid:",
			(xpos+(2*edgsp)+spref+midsp+mpref),y6label,mpref,buth,
			0, 0, 0, 0, 0, "");
		uiBlockBeginAlign(block);
		uiDefButBitI(block, TOG, USER_AUTOGRABGRID, 0, "Grab/Move",
			(xpos+edgsp+mpref+spref+(2*midsp)),y5,spref,buth,
			&(U.flag), 0, 0, 0, 0,
			"Snap objects and sub-objects to grid units when moving");
		uiDefButBitI(block, TOG, USER_AUTOROTGRID, 0, "Rotate",
			(xpos+edgsp+mpref+spref+(2*midsp)),y4,spref,buth,
			&(U.flag), 0, 0, 0, 0,
			"Snap objects and sub-objects to grid units when rotating");
		uiDefButBitI(block, TOG, USER_AUTOSIZEGRID, 0, "Scale",
			(xpos+edgsp+mpref+spref+(2*midsp)),y3,spref,buth,
			&(U.flag), 0, 0, 0, 0,
			"Snap objects and sub-objects to grid units when scaling");
		uiBlockEndAlign(block);
		
		uiDefButBitI(block, TOG, USER_LOCKAROUND, B_DRAWINFO, "Global Pivot",
			(xpos+edgsp+mpref+spref+(2*midsp)),y1,spref,buth,
			&(U.uiflag), 0, 0, 0, 0,
			"Lock the same rotation/scaling pivot in all 3D Views");	
		
		uiDefBut(block, LABEL,0,"View zoom:",
			(xpos+(2*edgsp)+mpref+(2*spref)+(2*midsp)),y6label,mpref,buth,
			0, 0, 0, 0, 0, "");
		uiBlockBeginAlign(block);
		uiBlockSetCol(block, TH_BUT_SETTING1);	/* mutually exclusive toggles, start color */
		uiDefButS(block, ROW, 0, "Continue",
			(xpos+edgsp+mpref+(2*spref)+(3*midsp)),y5,(mpref/3),buth,
			&(U.viewzoom), 40, USER_ZOOM_CONT, 0, 0,
			"Old style zoom, continues while moving mouse up or down");
		uiDefButS(block, ROW, 0, "Dolly",
			(xpos+edgsp+mpref+(2*spref)+(3*midsp)+(mpref/3)),y5,(mpref/3),buth,
			&(U.viewzoom), 40, USER_ZOOM_DOLLY, 0, 0,
			"Zooms in and out based on vertical mouse movement.");
		uiDefButS(block, ROW, 0, "Scale",
			(xpos+edgsp+mpref+(2*spref)+(3*midsp)+(2*mpref/3)),y5,(mpref/3),buth,
			&(U.viewzoom), 40, USER_ZOOM_SCALE, 0, 0,
			"Zooms in and out like scaling the view, mouse movements relative to center.");
		uiBlockSetCol(block, TH_AUTO);			/* end color */
		uiBlockEndAlign(block);
		
		uiDefBut(block, LABEL,0,"View rotation:",
			(xpos+(2*edgsp)+mpref+(2*spref)+(2*midsp)),y4label,mpref,buth,
			0, 0, 0, 0, 0, "");
		uiBlockBeginAlign(block);
		uiBlockSetCol(block, TH_BUT_SETTING1);	/* mutually exclusive toggles, start color */
		uiDefButBitI(block, TOG, USER_TRACKBALL, B_DRAWINFO, "Trackball",
			(xpos+edgsp+mpref+(2*spref)+(3*midsp)),y3,(mpref/2),buth,
			&(U.flag), 0, 0, 0, 0,
			"Allow the view to tumble freely when orbiting with the Middle Mouse Button");
		uiDefButBitI(block, TOGN, USER_TRACKBALL, B_DRAWINFO, "Turntable",
			(xpos+edgsp+mpref+(2*spref)+(3*midsp)+(mpref/2)),y3,(mpref/2),buth,
			&(U.flag), 0, 0, 0, 0,
			"Use fixed up axis for orbiting with Middle Mouse Button");
		uiBlockSetCol(block, TH_AUTO);			/* end color */
		uiDefButBitI(block, TOG, USER_AUTOPERSP, B_DRAWINFO, "Auto Perspective",
			(xpos+edgsp+mpref+(2*spref)+(3*midsp)),y2,(mpref/2),buth,
			&(U.uiflag), 0, 0, 0, 0,
			"Automatically switch between orthographic and perspective when changing from top/front/side views");
		uiDefButBitI(block, TOG, USER_ORBIT_SELECTION, B_DRAWINFO, "Around Active",
			(xpos+edgsp+mpref+(2*spref)+(3*midsp)+(mpref/2)),y2,(mpref/2),buth,
			&(U.uiflag), 0, 0, 0, 0,
			"Keep the active object in place when orbiting the views (Object Mode)");
		uiBlockEndAlign(block);
		

		uiBlockBeginAlign(block);
		uiDefButBitI(block, TOG, USER_SHOW_ROTVIEWICON, B_DRAWINFO, "Mini Axis",
			 (xpos+edgsp+(2*mpref)+(2*midsp)),y1,(mpref/3),buth,
			 &(U.uiflag), 0, 0, 0, 0,
			 "Show a small rotating 3D axis in the bottom left corner of the 3D View");
		uiDefButS(block, NUM, B_DRAWINFO, "Size:",
			(xpos+edgsp+(2*mpref)+(2*midsp)+(mpref/3)),y1,(mpref/3),buth,
			&U.rvisize, 10, 64, 0, 0,
			"The axis icon's size");
		uiDefButS(block, NUM, B_DRAWINFO, "Bright:",
			(xpos+edgsp+(2*mpref)+(2*midsp)+2*(mpref/3)),y1,(mpref/3),buth,
			&U.rvibright, 0, 10, 0, 0,
			"The brightness of the icon");
		uiBlockEndAlign(block);
		

		uiDefBut(block, LABEL,0,"Select with:",
			(xpos+(2*edgsp)+(3*mpref)+(3*midsp)),y6label,mpref,buth,
			0, 0, 0, 0, 0, "");
		uiBlockBeginAlign(block);
		uiBlockSetCol(block, TH_BUT_SETTING1);	/* mutually exclusive toggles, start color */
		uiDefButBitI(block, TOG, USER_LMOUSESELECT, B_DRAWINFO, "Left Mouse",
			(xpos+edgsp+(3*mpref)+(4*midsp)),y5,(mpref/2),buth,
			&(U.flag), 0, 0, 0, 0, "Use the Left Mouse Button for selection");
		uiDefButBitI(block, TOGN, USER_LMOUSESELECT, B_DRAWINFO, "Right Mouse",
			(xpos+edgsp+(3*mpref)+(4*midsp)+(mpref/2)),y5,(mpref/2),buth,
			&(U.flag), 0, 0, 0, 0, "Use the Right Mouse Button for selection");
		uiBlockSetCol(block, TH_AUTO);			/* end color */
		uiBlockEndAlign(block);
		
		
		if(U.flag & USER_LMOUSESELECT) {
			uiDefBut(block, LABEL,0,"Cursor with: Right Mouse",
				(xpos+(2*edgsp)+(3*mpref)+(3*midsp)),y4label+5,mpref,buth,
				0, 0, 0, 0, 0, "");
		} else {
			uiDefBut(block, LABEL,0,"Cursor with: Left Mouse",
				(xpos+(2*edgsp)+(3*mpref)+(3*midsp)),y4label+5,mpref,buth,
				0, 0, 0, 0, 0, "");
		}
		
		/* illegal combo... */
		if (U.flag & USER_LMOUSESELECT) 
			U.flag &= ~USER_TWOBUTTONMOUSE;
		
		uiDefButBitI(block, TOG, USER_TWOBUTTONMOUSE, B_DRAWINFO, "Emulate 3 Button Mouse",
			(xpos+edgsp+(3*mpref)+(4*midsp)),y3,mpref,buth,
			&(U.flag), 0, 0, 0, 0,
			"Emulates Middle Mouse with Alt+LeftMouse (doesnt work with Left Mouse Select option)");
		
			
		uiDefBut(block, LABEL,0,"Middle Mouse Button:",
			(xpos+(2*edgsp)+(4*mpref)+(4*midsp)),y6label,mpref,buth,
			0, 0, 0, 0, 0, "");
		uiBlockBeginAlign(block);
		uiBlockSetCol(block, TH_BUT_SETTING1);	/* mutually exclusive toggles, start color */
		uiDefButBitI(block, TOGN, USER_VIEWMOVE, B_DRAWINFO, "Rotate View",
			(xpos+edgsp+(4*mpref)+(5*midsp)),y5,(mpref/2),buth,
			&(U.flag), 0, 0, 0, 0, "Default action for the Middle Mouse Button");
		uiDefButBitI(block, TOG, USER_VIEWMOVE, B_DRAWINFO, "Pan View",
			(xpos+edgsp+(4*mpref)+(5*midsp)+(mpref/2)),y5,(mpref/2),buth,
			&(U.flag), 0, 0, 0, 0, "Default action for the Middle Mouse Button");
		uiBlockSetCol(block, TH_AUTO);			/* end color */
		uiBlockEndAlign(block);
			
		uiDefBut(block, LABEL,0,"Mouse Wheel:",
			(xpos+(2*edgsp)+(4*mpref)+(4*midsp)),y4label,mpref,buth,
			0, 0, 0, 0, 0, "");
		uiBlockBeginAlign(block);
		uiDefButBitI(block, TOG, USER_WHEELZOOMDIR, 0, "Invert Zoom",
			(xpos+edgsp+(4*mpref)+(5*midsp)),y3,spref,buth,
			&(U.uiflag), 0, 0, 0, 0,
			"Swap the Mouse Wheel zoom direction");
		uiDefButI(block, NUM, 0, "Scroll Lines:",
			(xpos+edgsp+(4*mpref)+(6*midsp)+spref-edgsp),y3,spref+edgsp,buth,
			&U.wheellinescroll, 0.0, 32.0, 0, 0,
			"The number of lines scrolled at a time with the mouse wheel");	
		uiBlockEndAlign(block);


		uiDefBut(block, LABEL,0,"3D Transform Widget:",
				 (xpos+(2*edgsp)+(5*mpref)+(5*midsp)),y6label,mpref,buth,
				 0, 0, 0, 0, 0, "");
		uiBlockBeginAlign(block);
		uiDefButS(block, NUM, B_REDRCURW3D, "Size:",
					 (xpos+edgsp+(5*mpref)+(6*midsp)),y5,(mpref/2),buth,
					 &(U.tw_size), 2, 40, 0, 0, "Diameter of widget, in 10 pixel units");
		uiDefButS(block, NUM, B_REDRCURW3D, "Handle:",
					 (xpos+edgsp+(5*mpref)+(6*midsp)+(mpref/2)),y5,(mpref/2),buth,
					 &(U.tw_handlesize), 2, 40, 0, 0, "Size of widget handles as percentage of widget radius");
		uiDefButS(block, NUM, B_REDRCURW3D, "Hotspot:",
				  (xpos+edgsp+(5*mpref)+(6*midsp)),y4,(mpref),buth,
				  &(U.tw_hotspot), 4, 40, 0, 0, "Hotspot in pixels for clicking widget handles");
		uiBlockEndAlign(block);
		
		uiDefButS(block, NUM, B_REDRCURW3D, "Object Center Size: ",
				   (xpos+edgsp+(5*mpref)+(6*midsp)),y3,mpref,buth,
				  &(U.obcenter_dia), 4, 10, 0, 0,
				  "Diameter in Pixels for Object/Lamp center display");
		
		
	} else if (U.userpref == 1) { /* edit methods */


		uiDefBut(block, LABEL,0,"Material linked to:",
			xpos,y3label,mpref,buth,
			0, 0, 0, 0, 0, "");
		uiBlockBeginAlign(block);
		uiDefButBitI(block, TOGN, USER_MAT_ON_OB, B_DRAWINFO, "ObData",
			(xpos+edgsp),y2,(mpref/2),buth,
			&(U.flag), 0, 0, 0, 0, "Link new objects' material to the obData block");
		uiDefButBitI(block, TOG, USER_MAT_ON_OB, B_DRAWINFO, "Object",
			(xpos+edgsp+(mpref/2)),y2,(mpref/2),buth,
			&(U.flag), 0, 0, 0, 0, "Link new objects' material to the object block");
		uiBlockEndAlign(block);


		uiDefBut(block, LABEL,0,"Undo:",
			(xpos+(2*edgsp)+mpref),y3label, mpref,buth,
			0, 0, 0, 0, 0, "");
		uiBlockBeginAlign(block);
		uiDefButS(block, NUMSLI, B_DRAWINFO, "Steps: ",
			(xpos+edgsp+mpref+midsp),y2,mpref,buth,
			&(U.undosteps), 0, 64, 0, 0, "Number of undo steps available (smaller values conserve memory)");

		uiDefButBitI(block, TOG, USER_GLOBALUNDO, B_DRAWINFO, "Global Undo",
			(xpos+edgsp+mpref+midsp),y1,mpref,buth,
			&(U.uiflag), 2, 64, 0, 0, "Global undo works by keeping a full copy of the file itself in memory, so takes extra memory");
		uiBlockEndAlign(block);


		uiDefBut(block, LABEL,0,"Auto keyframe",
			(xpos+(2*edgsp)+(2*mpref)+midsp),y3label,mpref,buth,
			0, 0, 0, 0, 0, "");

		uiDefButBitI(block, TOG, G_RECORDKEYS, REDRAWTIME, "Action and Object", 
					(xpos+edgsp+(2*mpref)+(2*midsp)),y2,mpref, buth,
					 &(G.flags), 0, 0, 0, 0, "Automatic keyframe insertion in Object and Action Ipo curves");

		uiDefButBitI(block, TOG, USER_KEYINSERTAVAI, REDRAWTIME, "Available", 
			(xpos+edgsp+(2*mpref)+(2*midsp)),y1,mpref, buth,
			&(U.uiflag), 0, 0, 0, 0, "Automatic keyframe insertion in available curves");

//		uiDefButBitS(block, TOG, USER_KEYINSERTACT, 0, "Action",
//			(xpos+edgsp+(2*mpref)+(2*midsp)),y2,(spref+edgsp),buth,
//			&(U.uiflag), 0, 0, 0, 0, "Automatic keyframe insertion in Action Ipo curve");
//		uiDefButBitS(block, TOG, USER_KEYINSERTOBJ, 0, "Object",
//			(xpos+edgsp+(2*mpref)+(3*midsp)+spref-edgsp),y2,(spref+edgsp),buth,
//			&(U.uiflag), 0, 0, 0, 0, "Automatic keyframe insertion in Object Ipo curve");


		uiDefBut(block, LABEL,0,"Duplicate with object:",
			(xpos+(2*edgsp)+(3*midsp)+(3*mpref)+spref),y3label,mpref,buth,
			0, 0, 0, 0, 0, "");

		uiBlockBeginAlign(block);
		uiDefButBitI(block, TOG, USER_DUP_MESH, 0, "Mesh",
			(xpos+edgsp+(4*midsp)+(3*mpref)+spref),y2,(spref+edgsp),buth,
			&(U.dupflag), 0, 0, 0, 0, "Causes mesh data to be duplicated with Shift+D");
		uiDefButBitI(block, TOG, USER_DUP_SURF, 0, "Surface",
			(xpos+edgsp+(5*midsp)+(3*mpref)+(2*spref)),y2,(spref+edgsp),buth,
			&(U.dupflag), 0, 0, 0, 0, "Causes surface data to be duplicated with Shift+D");
		uiDefButBitI(block, TOG, USER_DUP_CURVE, 0, "Curve",
			(xpos+edgsp+(6*midsp)+(3*mpref)+(3*spref)),y2,(spref+edgsp),buth,
			&(U.dupflag), 0, 0, 0, 0, "Causes curve data to be duplicated with Shift+D");
		uiDefButBitI(block, TOG, USER_DUP_FONT, 0, "Text",
			(xpos+edgsp+(7*midsp)+(3*mpref)+(4*spref)),y2,(spref+edgsp),buth,
			&(U.dupflag), 0, 0, 0, 0, "Causes text data to be duplicated with Shift+D");
		uiDefButBitI(block, TOG, USER_DUP_MBALL, 0, "Metaball",
			(xpos+edgsp+(8*midsp)+(3*mpref)+(5*spref)),y2,(spref+edgsp),buth,
			&(U.dupflag), 0, 0, 0, 0, "Causes metaball data to be duplicated with Shift+D");

		uiDefButBitI(block, TOG, USER_DUP_ARM, 0, "Armature",
			(xpos+edgsp+(4*midsp)+(3*mpref)+spref),y1,(spref+edgsp),buth,
			&(U.dupflag), 0, 0, 0, 0, "Causes armature data to be duplicated with Shift+D");
		uiDefButBitI(block, TOG, USER_DUP_LAMP, 0, "Lamp",
			(xpos+edgsp+(5*midsp)+(3*mpref)+(2*spref)),y1,(spref+edgsp),buth,
			&(U.dupflag), 0, 0, 0, 0, "Causes lamp data to be duplicated with Shift+D");
		uiDefButBitI(block, TOG, USER_DUP_MAT, 0, "Material",
			(xpos+edgsp+(6*midsp)+(3*mpref)+(3*spref)),y1,(spref+edgsp),buth,
			&(U.dupflag), 0, 0, 0, 0, "Causes material data to be duplicated with Shift+D");
		uiDefButBitI(block, TOG, USER_DUP_TEX, 0, "Texture",
			(xpos+edgsp+(7*midsp)+(3*mpref)+(4*spref)),y1,(spref+edgsp),buth,
			&(U.dupflag), 0, 0, 0, 0, "Causes texture data to be duplicated with Shift+D");
		uiDefButBitI(block, TOG, USER_DUP_IPO, 0, "Ipo",
			(xpos+edgsp+(8*midsp)+(3*mpref)+(5*spref)),y1,(spref+edgsp),buth,
			&(U.dupflag), 0, 0, 0, 0, "Causes ipo data to be duplicated with Shift+D");
		uiBlockEndAlign(block);
	
	} else if(U.userpref == 2) { /* language & colors */

#ifdef INTERNATIONAL
		uiDefButBitS(block, TOG, USER_DOTRANSLATE, B_DOLANGUIFONT, "International Fonts",
			xpos,y2,mpref,buth,
			&(U.transopts), 0, 0, 0, 0, "Activate international interface");

		if(U.transopts & USER_DOTRANSLATE) {
			char curfont[320];
			
			sprintf(curfont, "Interface Font: ");
			if(U.fontname[0]) strcat(curfont, U.fontname);
			else strcat(curfont, "Built-in");
			
			uiDefBut(block, LABEL,0,curfont,
				(xpos),y3,4*mpref,buth,
				0, 0, 0, 0, 0, "");

			uiDefBut(block, BUT, B_LOADUIFONT, "Select Font",
				xpos,y1,mpref,buth,
				0, 0, 0, 0, 0, "Select a new font for the interface");

			uiDefButI(block, BUT, B_RESTOREFONT, "Restore to default",
					  (xpos+edgsp+mpref+midsp),y2,mpref,buth,
					  &U.fontsize, 0, 0, 0, 0, "Restores to using the default included antialised font");
			
			uiDefButI(block, MENU, B_SETFONTSIZE, fontsize_pup(),
				(xpos+edgsp+mpref+midsp),y1,mpref,buth,
				&U.fontsize, 0, 0, 0, 0, "Current interface font size (points)");

/*
			uiDefButS(block, MENU, B_SETENCODING, encoding_pup(),
				(xpos+edgsp+mpref+midsp),y1,mpref,buth,
				&U.encoding, 0, 0, 0, 0, "Current interface font encoding");


			uiDefBut(block, LABEL,0,"Translate:",
				(xpos+edgsp+(2.1*mpref)+(2*midsp)),y3label,mpref,buth,
				0, 0, 0, 0, 0, "");
*/

			uiDefButBitS(block, TOG, USER_TR_TOOLTIPS, B_SETTRANSBUTS, "Tooltips",
				(xpos+edgsp+(2.2*mpref)+(3*midsp)),y1,spref,buth,
				&(U.transopts), 0, 0, 0, 0, "Translate tooltips");

			uiDefButBitS(block, TOG, USER_TR_BUTTONS, B_SETTRANSBUTS, "Buttons",
				(xpos+edgsp+(2.2*mpref)+(4*midsp)+spref),y1,spref,buth,
				&(U.transopts), 0, 0, 0, 0, "Translate button labels");

			uiDefButBitS(block, TOG, USER_TR_MENUS, B_SETTRANSBUTS, "Toolbox",
				(xpos+edgsp+(2.2*mpref)+(5*midsp)+(2*spref)),y1,spref,buth,
				&(U.transopts), 0, 0, 0, 0, "Translate toolbox menu");

			uiDefButI(block, MENU, B_SETLANGUAGE, language_pup(),
				(xpos+edgsp+(2.2*mpref)+(3*midsp)),y2,mpref+(0.5*mpref)+3,buth,
				&U.language, 0, 0, 0, 0, "Select interface language");
				
			uiDefButBitS(block, TOG, USER_USETEXTUREFONT, B_USETEXTUREFONT, "Use Textured Fonts",
				(xpos+edgsp+(4*mpref)+(4*midsp)),y2,mpref,buth,
				&(U.transopts), 0, 0, 0, 0,
				"Use Textured Fonts");
		}

/* end of INTERNATIONAL */
#endif

	} else if(U.userpref == 3) { /* auto save */


		uiDefButS(block, NUM, 0, "Save Versions:",
			(xpos+edgsp),y3,mpref,buth,
			&U.versions, 0.0, 32.0, 0, 0,
			"The number of old versions to maintain in the current directory, when manually saving");

		uiDefButBitI(block, TOG, USER_AUTOSAVE, B_RESETAUTOSAVE, "Auto Save Temp Files",
			(xpos+edgsp+mpref+midsp),y3,mpref,buth,
			&(U.flag), 0, 0, 0, 0,
			"Enables automatic saving of temporary files");

		if(U.flag & USER_AUTOSAVE) {

			uiDefButI(block, NUM, B_RESETAUTOSAVE, "Minutes:",
				(xpos+edgsp+mpref+midsp),y2,mpref,buth,
				&(U.savetime), 1.0, 60.0, 0, 0,
				"The time (in minutes) to wait between automatic temporary saves");

			uiDefBut(block, BUT, B_LOADTEMP, "Open Recent",
				(xpos+edgsp+mpref+midsp),y1,mpref,buth,
				0, 0, 0, 0, 0,"Open the most recently saved temporary file");
		}

	} else if (U.userpref == 4) { /* system & opengl */
		uiDefBut(block, LABEL,0,"Solid OpenGL light:",
			xpos+edgsp, y3label, mpref, buth,
			0, 0, 0, 0, 0, "");
		
		uiBlockBeginAlign(block);
		uiDefButS(block, MENU, B_REDR, "Light1 %x0|Light2 %x1|Light3 %x2",
			xpos+edgsp, y2, 2*mpref/6, buth, &cur_light, 0.0, 0.0, 0, 0, "");
		uiBlockSetCol(block, TH_BUT_SETTING1);
		uiDefButBitI(block, TOG, 1, B_RECALCLIGHT, "On",
			xpos+edgsp+2*mpref/6, y2, mpref/6, buth, 
			&U.light[cur_light].flag, 0.0, 0.0, 0, 0, "Enable this OpenGL light in Solid draw mode");
		
		uiBlockSetCol(block, TH_AUTO);
		uiDefButS(block, ROW, B_REDR, "Vec",
			xpos+edgsp+3*mpref/6, y2, mpref/6, buth, 
			&cur_light_var, 123.0, 0.0, 0, 0, "Lamp vector for OpenGL light");
		uiDefButS(block, ROW, B_REDR, "Col",
			xpos+edgsp+4*mpref/6, y2, mpref/6, buth, 
			&cur_light_var, 123.0, 1.0, 0, 0, "Diffuse Color for OpenGL light");
		uiDefButS(block, ROW, B_REDR, "Spec",
			xpos+edgsp+5*mpref/6, y2, mpref/6, buth, 
			&cur_light_var, 123.0, 2.0, 0, 0, "Specular color for OpenGL light");
		uiBlockEndAlign(block);
		
		uiBlockBeginAlign(block);
		if(cur_light_var==1) {
			uiDefButF(block, NUM, B_RECALCLIGHT, "R ",
				xpos+edgsp, y1, mpref/3, buth, 
				U.light[cur_light].col, 0.0, 1.0, 100, 2, "");
			uiDefButF(block, NUM, B_RECALCLIGHT, "G ",
				xpos+edgsp+mpref/3, y1, mpref/3, buth, 
				U.light[cur_light].col+1, 0.0, 1.0, 100, 2, "");
			uiDefButF(block, NUM, B_RECALCLIGHT, "B ",
				xpos+edgsp+2*mpref/3, y1, mpref/3, buth, 
				U.light[cur_light].col+2, 0.0, 1.0, 100, 2, "");
		}
		else if(cur_light_var==2) {
			uiDefButF(block, NUM, B_RECALCLIGHT, "sR ",
				xpos+edgsp, y1, mpref/3, buth, 
				U.light[cur_light].spec, 0.0, 1.0, 100, 2, "");
			uiDefButF(block, NUM, B_RECALCLIGHT, "sG ",
				xpos+edgsp+mpref/3, y1, mpref/3, buth, 
				U.light[cur_light].spec+1, 0.0, 1.0, 100, 2, "");
			uiDefButF(block, NUM, B_RECALCLIGHT, "sB ",
				xpos+edgsp+2*mpref/3, y1, mpref/3, buth, 
				U.light[cur_light].spec+2, 0.0, 1.0, 100, 2, "");
		}
		else if(cur_light_var==0) {
			uiDefButF(block, NUM, B_RECALCLIGHT, "X ",
				xpos+edgsp, y1, mpref/3, buth, 
				U.light[cur_light].vec, -1.0, 1.0, 100, 2, "");
			uiDefButF(block, NUM, B_RECALCLIGHT, "Y ",
				xpos+edgsp+mpref/3, y1, mpref/3, buth, 
				U.light[cur_light].vec+1, -1.0, 1.0, 100, 2, "");
			uiDefButF(block, NUM, B_RECALCLIGHT, "Z ",
				xpos+edgsp+2*mpref/3, y1, mpref/3, buth, 
				U.light[cur_light].vec+2, -1.0, 1.0, 100, 2, "");
		}
		uiBlockEndAlign(block);

/*
		uiDefButBitS(block, TOG, USER_EVTTOCONSOLE, 0, "Log Events to Console",
			(xpos+edgsp),y2,lpref,buth,
			&(U.uiflag), 0, 0, 0, 0, "Display a list of input events in the console");

		uiDefButS(block, MENU, B_CONSOLEOUT, consolemethod_pup(),
			(xpos+edgsp), y1, lpref,buth,
			&U.console_out, 0, 0, 0, 0, "Select console output method");

		uiDefButS(block, NUM, B_CONSOLENUMLINES, "Lines:",
			(xpos+edgsp+lpref+midsp),y1,spref,buth,
			&U.console_buffer, 1.0, 4000.0, 0, 0, "Maximum number of internal console lines");
*/

#ifdef _WIN32
		uiDefBut(block, LABEL,0,"Win Codecs:",
			(xpos+edgsp+(1*midsp)+(1*mpref)),y3label,mpref,buth,
			0, 0, 0, 0, 0, "");

		uiDefButBitI(block, TOG, USER_ALLWINCODECS, 0, "Enable all codecs",
			(xpos+edgsp+(1*mpref)+(1*midsp)),y2,mpref,buth,
			&(U.uiflag), 0, 0, 0, 0, "Allows all codecs for rendering (not guaranteed)");
#endif

		uiDefBut(block, LABEL,0,"Keyboard:",
			(xpos+edgsp+(3*midsp)+(3*mpref)),y3label,mpref,buth,
			0, 0, 0, 0, 0, "");

		uiDefButBitI(block, TOG, USER_NO_CAPSLOCK, B_U_CAPSLOCK, "Disable Caps Lock",
			(xpos+edgsp+(3*midsp)+(3*mpref)),y1,mpref,buth,
			&(U.flag), 0, 0, 0, 0,
			"Disables the Caps Lock key when entering text");

		uiDefButBitI(block, TOG, USER_NONUMPAD, 0, "Emulate Numpad",
			(xpos+edgsp+(3*midsp)+(3*mpref)),y2,mpref,buth,
			&(U.flag), 0, 0, 0, 0,
			"Causes the 1 to 0 keys to act as the numpad (useful for laptops)");


		uiDefBut(block, LABEL,0,"System:",
			(xpos+edgsp+(4*midsp)+(4*mpref)),y6label,mpref,buth,
			0, 0, 0, 0, 0, "");
		uiDefButI(block, NUM, B_MEMCACHELIMIT, "MEM Cache Limit ",
			  (xpos+edgsp+(4*mpref)+(4*midsp)), y5, mpref, buth, 
			  &U.memcachelimit, 0.0, 1024.0, 30, 2, 
			  "Memory cache limit in sequencer");
		uiDefButS(block, NUM, B_REDR, "Frameserver Port ",
			  (xpos+edgsp+(4*mpref)+(4*midsp)), y4, mpref, buth, 
			  &U.frameserverport, 0.0, 32727.0, 30, 2, 
			  "Frameserver Port for Framserver-Rendering");

		uiDefButBitI(block, TOG, USER_DISABLE_SOUND, B_SOUNDTOGGLE, "Disable Game Sound",
			(xpos+edgsp+(4*mpref)+(4*midsp)),y3,mpref,buth,
			&(U.gameflags), 0, 0, 0, 0, "Disables sounds from being played in games");

		uiDefButBitI(block, TOG, USER_FILTERFILEEXTS, 0, "Filter File Extensions",
			(xpos+edgsp+(4*mpref)+(4*midsp)),y2,mpref,buth,
			&(U.uiflag), 0, 0, 0, 0, "Display only files with extensions in the image select window");

		uiDefButBitI(block, TOG, USER_HIDE_DOT, 0, "Hide dot file/datablock",
			(xpos+edgsp+(4*mpref)+(4*midsp)),y1,mpref,buth,
			&(U.uiflag), 0, 0, 0, 0, "Hide files/datablocks that start with a dot(.*)");


		uiDefBut(block, LABEL,0,"OpenGL:",
			(xpos+edgsp+(5*midsp)+(5*mpref)),y5label,mpref,buth,
			0, 0, 0, 0, 0, "");
		uiDefButBitI(block, TOGN, USER_DISABLE_MIPMAP, B_MIPMAPCHANGED, "Mipmaps",
			(xpos+edgsp+(5*mpref)+(5*midsp)),y4,mpref,buth,
			&(U.gameflags), 0, 0, 0, 0, "Toggles between mipmap textures on (beautiful) and off (fast)");
		uiDefButBitI(block, TOG, USER_VERTEX_ARRAYS, 0, "Vertex Arrays",
			(xpos+edgsp+(5*mpref)+(5*midsp)),y3,mpref,buth,
			&(U.gameflags), 0, 0, 0, 0, "Toggles between vertex arrays on (less reliable) and off (more reliable)");

		uiDefButI(block, NUM, 0, "Time Out ",
			(xpos+edgsp+(5*mpref)+(5*midsp)), y2, mpref, buth, 
			&U.textimeout, 0.0, 3600.0, 30, 2, "Time since last access of a GL texture in seconds after which it is freed. (Set to 0 to keep textures allocated)");
		uiDefButI(block, NUM, 0, "Collect Rate ",
			(xpos+edgsp+(5*mpref)+(5*midsp)), y1, mpref, buth, 
			&U.texcollectrate, 1.0, 3600.0, 30, 2, "Number of seconds between each run of the GL texture garbage collector.");


		uiDefBut(block, LABEL,0,"Audio mixing buffer:",
			(xpos+edgsp+(2*midsp)+(2*mpref)),y3label,mpref,buth,
			0, 0, 0, 0, 0, "");
		uiBlockBeginAlign(block);
		uiDefButI(block, ROW, 0, "256",
			(xpos+edgsp+(2*midsp)+(2*mpref)),y2,(mpref/4),buth,
			&U.mixbufsize, 2.0, 256.0, 0, 0, "Set audio mixing buffer size to 256 samples");
		uiDefButI(block, ROW, 0, "512",
			(xpos+edgsp+(2*midsp)+(2*mpref)+(mpref/4)),y2,(mpref/4),buth,
			&U.mixbufsize, 2.0, 512.0, 0, 0, "Set audio mixing buffer size to 512 samples");	
		uiDefButI(block, ROW, 0, "1024",
			(xpos+edgsp+(2*midsp)+(2*mpref)+(2*mpref/4)),y2,(mpref/4),buth,
			&U.mixbufsize, 2.0, 1024.0, 0, 0, "Set audio mixing buffer size to 1024 samples");		
		uiDefButI(block, ROW, 0, "2048",
			(xpos+edgsp+(2*midsp)+(2*mpref)+(3*mpref/4)),y2,(mpref/4),buth,
			&U.mixbufsize, 2.0, 2048.0, 0, 0, "Set audio mixing buffer size to 2048 samples");			
		uiBlockEndAlign(block);

	} else if(U.userpref == 5) { /* file paths */

		/* yafray: (temporary) path button for yafray xml export, now with fileselect */
		uiBlockBeginAlign(block);
		uiDefBut(block, TEX, 0, "YFexport: ",
			(xpos+edgsp), y2+buth+rspace, lpref-smfileselbut, buth,
			U.yfexportdir, 1.0, 63.0, 0, 0,
			"The default directory for yafray xml export (must exist!)");
		uiDefIconBut(block, BUT, B_YAFRAYDIRFILESEL, ICON_FILESEL,
			(xpos+edgsp+lpref-smfileselbut), y2+buth+rspace, smfileselbut, buth,
			0, 0, 0, 0, 0, "Select the default yafray export directory");
		uiBlockEndAlign(block);
		
		uiBlockBeginAlign(block);
		uiDefBut(block, TEX, 0, "Fonts: ",
			(xpos+edgsp),y2,(lpref-smfileselbut),buth,
			U.fontdir, 1.0, 63.0, 0, 0,
			"The default directory to search for loading fonts");
		uiDefIconBut(block, BUT, B_FONTDIRFILESEL, ICON_FILESEL,
			(xpos+edgsp+lpref-smfileselbut),y2,smfileselbut,buth,
			0, 0, 0, 0, 0, "Select the default font directory");
		uiBlockEndAlign(block);
		
		uiBlockBeginAlign(block);
		uiDefBut(block, TEX, 0, "Textures: ",
			(xpos+edgsp+lpref+midsp),y2,(lpref-smfileselbut),buth,
			U.textudir, 1.0, 63.0, 0, 0, "The default directory to search for textures");
		uiDefIconBut(block, BUT, B_TEXTUDIRFILESEL, ICON_FILESEL,
			(xpos+edgsp+(2*lpref)+midsp-smfileselbut),y2,smfileselbut,buth,
			0, 0, 0, 0, 0, "Select the default texture location");
		uiBlockEndAlign(block);

		
		uiBlockBeginAlign(block);
		uiDefBut(block, TEX, 0, "Tex Plugins: ",
			(xpos+edgsp+(2*lpref)+(2*midsp)),y2,(lpref-smfileselbut),buth,
			U.plugtexdir, 1.0, 63.0, 0, 0, "The default directory to search for texture plugins");
		uiDefIconBut(block, BUT, B_PLUGTEXDIRFILESEL, ICON_FILESEL,
			(xpos+edgsp+(3*lpref)+(2*midsp)-smfileselbut),y2,smfileselbut,buth,
			0, 0, 0, 0, 0, "Select the default texture plugin location");
		uiBlockEndAlign(block);
		
		uiBlockBeginAlign(block);
		uiDefBut(block, TEX, 0, "Seq Plugins: ",
			(xpos+edgsp+(3*lpref)+(3*midsp)),y2,(lpref-smfileselbut),buth,
			U.plugseqdir, 1.0, 63.0, 0, 0, "The default directory to search for sequence plugins");
		uiDefIconBut(block, BUT, B_PLUGSEQDIRFILESEL, ICON_FILESEL,
			(xpos+edgsp+(4*lpref)+(3*midsp)-smfileselbut),y2,smfileselbut,buth,
			0, 0, 0, 0, 0, "Select the default sequence plugin location");
		uiBlockEndAlign(block);

		
		uiBlockBeginAlign(block);
		uiDefBut(block, TEX, 0, "Render: ",
			(xpos+edgsp),y1,(lpref-smfileselbut),buth,
			U.renderdir, 1.0, 63.0, 0, 0, "The default directory for rendering output");
		uiDefIconBut(block, BUT, B_RENDERDIRFILESEL, ICON_FILESEL,
			(xpos+edgsp+lpref-smfileselbut),y1,smfileselbut,buth,
			0, 0, 0, 0, 0, "Select the default render output location");
		uiBlockEndAlign(block);

		uiBlockBeginAlign(block);
		uiDefBut(block, TEX, 0, "Python: ",
			(xpos+edgsp+lpref+midsp),y1,(lpref-2*smfileselbut),buth,
			U.pythondir, 1.0, 63.0, 0, 0, "The default directory to search for Python scripts");
		uiDefIconBut(block, BUT, B_PYMENUEVAL, ICON_SCRIPT,
			(xpos+edgsp+(2*lpref)+midsp-2*smfileselbut),y1,smfileselbut,buth,
			0, 0, 0, 0, 0, "Re-evaluate scripts registration in menus");
		uiDefIconBut(block, BUT, B_PYTHONDIRFILESEL, ICON_FILESEL,
			(xpos+edgsp+(2*lpref)+midsp-smfileselbut),y1,smfileselbut,buth,
			0, 0, 0, 0, 0, "Select the default Python script location");
		uiBlockEndAlign(block);


		uiBlockBeginAlign(block);
		uiDefBut(block, TEX, 0, "Sounds: ",
			(xpos+edgsp+(2*lpref)+(2*midsp)),y1,(lpref-smfileselbut),buth,
			U.sounddir, 1.0, 63.0, 0, 0, "The default directory to search for sounds");
		uiDefIconBut(block, BUT, B_SOUNDDIRFILESEL, ICON_FILESEL,
			(xpos+edgsp+(3*lpref)+(2*midsp)-smfileselbut),y1,smfileselbut,buth,
			0, 0, 0, 0, 0, "Selet the default sound location");
		uiBlockEndAlign(block);

		uiBlockBeginAlign(block);
		uiDefBut(block, TEX, 0, "Temp: ",
			 (xpos+edgsp+(3*lpref)+(3*midsp)),y1,(lpref-smfileselbut),buth,
			 U.tempdir, 1.0, 63.0, 0, 0, "The directory for storing temporary save files");
		uiDefIconBut(block, BUT, B_TEMPDIRFILESEL, ICON_FILESEL,
			(xpos+edgsp+(4*lpref)+(3*midsp)-smfileselbut),y1,smfileselbut,buth,
			0, 0, 0, 0, 0, "Select the default temporary save file location");
		uiBlockEndAlign(block);

	}

	uiDrawBlock(block);
	
	myortho2(-0.375, (float)(sa->winx)-0.375, -0.375, (float)(sa->winy)-0.375);
	draw_area_emboss(sa);

	/* restore buttons transform */
	if(curarea->winx<=1280.0) {
		fac= ((float)curarea->winx)/1280.0f;
		myortho2(0.0, 1280.0, 0.0, curarea->winy/fac);
	}
	else {
		myortho2(0.0, (float)curarea->winx, 0.0, (float)curarea->winy);
	}
	sa->win_swap= WIN_BACK_OK;
	
}


static void winqreadinfospace(ScrArea *sa, void *spacedata, BWinEvent *evt)
{
	unsigned short event= evt->event;
	short val= evt->val;
	
	if(val) {
		if( uiDoBlocks(&curarea->uiblocks, event)!=UI_NOTHING ) event= 0;

		switch(event) {
		case UI_BUT_EVENT:
			if(val==REDRAWTIME) allqueue(REDRAWTIME, 0);
			else if(val==B_ADD_THEME) {
				bTheme *btheme, *new;
				
				btheme= U.themes.first;
				new= MEM_callocN(sizeof(bTheme), "theme");
				memcpy(new, btheme, sizeof(bTheme));
				BLI_addhead(&U.themes, new);
				strcpy(new->name, "New User Theme");
				addqueue(sa->win, REDRAW, 1);
			}
			else if(val==B_DEL_THEME) {
				bTheme *btheme= U.themes.first;
				BLI_remlink(&U.themes, btheme);
				MEM_freeN(btheme);
				BIF_SetTheme(sa); // prevent usage of old theme in calls	
				addqueue(sa->win, REDRAW, 1);
			}
			else if(val==B_NAME_THEME) {
				bTheme *btheme= U.themes.first;
				if(strcmp(btheme->name, "Default")==0) {
					strcpy(btheme->name, "New User Theme");
					addqueue(sa->win, REDRAW, 1);
				}
			}
			else if(val==B_UPDATE_THEME) {
				allqueue(REDRAWALL, 0);
			}
			else if(val==B_CHANGE_THEME) {
				th_curcol= TH_BACK;	// backdrop color is always there...
				addqueue(sa->win, REDRAW, 1);
			}
			else if(val==B_THEME_COPY) {
				if(th_curcol_ptr) {
					th_curcol_arr[0]= th_curcol_ptr[0];
					th_curcol_arr[1]= th_curcol_ptr[1];
					th_curcol_arr[2]= th_curcol_ptr[2];
					th_curcol_arr[3]= th_curcol_ptr[3];
					addqueue(sa->win, REDRAW, 1);
				}
			}
			else if(val==B_THEME_PASTE) {
				if(th_curcol_ptr) {
					th_curcol_ptr[0]= th_curcol_arr[0];
					th_curcol_ptr[1]= th_curcol_arr[1];
					th_curcol_ptr[2]= th_curcol_arr[2];
					th_curcol_ptr[3]= th_curcol_arr[3];
					allqueue(REDRAWALL, 0);
				}
			}
			else if(val==B_RECALCLIGHT) {
				if(U.light[0].flag==0 && U.light[1].flag==0 && U.light[2].flag==0)
					U.light[0].flag= 1;
				
				default_gl_light();
				addqueue(sa->win, REDRAW, 1);
				allqueue(REDRAWVIEW3D, 0);
			} 
			else if (val==B_MEMCACHELIMIT) {
				printf("Setting memcache limit to %d\n",
				       U.memcachelimit);
				MEM_CacheLimiter_set_maximum(
					U.memcachelimit * 1024 * 1024);
			}
			else do_global_buttons(val);
			
			break;	
		}
	}
}

static void init_infospace(ScrArea *sa)
{
	SpaceInfo *sinfo;
	
	sinfo= MEM_callocN(sizeof(SpaceInfo), "initinfo");
	BLI_addhead(&sa->spacedata, sinfo);

	sinfo->spacetype=SPACE_INFO;
}

/* ******************** SPACE: BUTS ********************** */

extern void drawbutspace(ScrArea *sa, void *spacedata);	/* buttons.c */

static void changebutspace(ScrArea *sa, void *spacedata)
{
	if(G.v2d==0) return;
	
	test_view2d(G.v2d, curarea->winx, curarea->winy);
	myortho2(G.v2d->cur.xmin, G.v2d->cur.xmax, G.v2d->cur.ymin, G.v2d->cur.ymax);
}

static void winqreadbutspace(ScrArea *sa, void *spacedata, BWinEvent *evt)
{
	unsigned short event= evt->event;
	short val= evt->val;
	SpaceButs *sbuts= curarea->spacedata.first;
	ScrArea *sa2, *sa3d;
	int nr, doredraw= 0;

	if(val) {
		
		if( uiDoBlocks(&curarea->uiblocks, event)!=UI_NOTHING ) event= 0;

		switch(event) {
		case UI_BUT_EVENT:
			do_butspace(val);
			break;
			
		case MIDDLEMOUSE:
		case WHEELUPMOUSE:
		case WHEELDOWNMOUSE:
			view2dmove(event);	/* in drawipo.c */
			break;
		case PAGEUPKEY:
			event= WHEELUPMOUSE;
			view2dmove(event);	/* in drawipo.c */
			break;
		case PAGEDOWNKEY:
			event= WHEELDOWNMOUSE;
			view2dmove(event);	/* in drawipo.c */
			break;
			
		case RIGHTMOUSE:
			nr= pupmenu("Panel Alignment%t|Horizontal%x1|Vertical%x2|Free %x0");
			if (nr>=0) {
				sbuts->align= nr;
				if(nr) {
					uiAlignPanelStep(sa, 1.0);
					do_buts_buttons(B_BUTSHOME);
				}
			}

			break;
		case PADPLUSKEY:
			view2d_zoom(&sbuts->v2d, 0.06f, curarea->winx, curarea->winy);
			scrarea_queue_winredraw(curarea);
			break;
		case PADMINUS:
			view2d_zoom(&sbuts->v2d, -0.075f, curarea->winx, curarea->winy);
			scrarea_queue_winredraw(curarea);
			break;
		case RENDERPREVIEW:
			BIF_previewrender_buts(sbuts);
			break;
		
		case HOMEKEY:
			do_buts_buttons(B_BUTSHOME);
			break;


		/* if only 1 view, also de persp, excluding arrowkeys */
		case PAD0: case PAD1: case PAD3:
		case PAD5: case PAD7: case PAD9:
		case PADENTER: case ZKEY: case PKEY:
			sa3d= 0;
			sa2= G.curscreen->areabase.first;
			while(sa2) {
				if(sa2->spacetype==SPACE_VIEW3D) {
					if(sa3d) return;
					sa3d= sa2;
				}
				sa2= sa2->next;
			}
			if(sa3d) {
				sa= curarea;
				areawinset(sa3d->win);
				
				if(event==PKEY) start_game();
				else if(event==ZKEY) toggle_shading();
				else persptoetsen(event);
				
				scrarea_queue_winredraw(sa3d);
				scrarea_queue_headredraw(sa3d);
				areawinset(sa->win);
			}
		}
	}

	if(doredraw) scrarea_queue_winredraw(curarea);
}

void set_rects_butspace(SpaceButs *buts)
{
	/* buts space goes from (0,0) to (1280, 228) */

	buts->v2d.tot.xmin= 0.0f;
	buts->v2d.tot.ymin= 0.0f;
	buts->v2d.tot.xmax= 1279.0f;
	buts->v2d.tot.ymax= 228.0f;
	
	buts->v2d.min[0]= 256.0f;
	buts->v2d.min[1]= 42.0f;

	buts->v2d.max[0]= 2048.0f;
	buts->v2d.max[1]= 450.0f;
	
	buts->v2d.minzoom= 0.5f;
	buts->v2d.maxzoom= 1.21f;
	
	buts->v2d.scroll= 0;
	buts->v2d.keepaspect= 1;
	buts->v2d.keepzoom= 1;
	buts->v2d.keeptot= 1;
	
}

void test_butspace(void)
{
	ScrArea *area= curarea;
	int blocksmin= uiBlocksGetYMin(&area->uiblocks)-10.0f;
	
	G.buts->v2d.tot.ymin= MIN2(0.0f, blocksmin-10.0f);
}

static void init_butspace(ScrArea *sa)
{
	SpaceButs *buts;
	
	buts= MEM_callocN(sizeof(SpaceButs), "initbuts");
	BLI_addhead(&sa->spacedata, buts);

	buts->spacetype= SPACE_BUTS;
	buts->scaflag= BUTS_SENS_LINK|BUTS_SENS_ACT|BUTS_CONT_ACT|BUTS_ACT_ACT|BUTS_ACT_LINK;

	/* set_rects only does defaults, so after reading a file the cur has not changed */
	set_rects_butspace(buts);
	buts->v2d.cur= buts->v2d.tot;

	buts->ri = NULL;
}

void extern_set_butspace(int fkey)
{
	ScrArea *sa;
	SpaceButs *sbuts;
	Object *ob= OBACT;
	
	/* when a f-key pressed: 'closest' button window is initialized */
	if(curarea->spacetype==SPACE_BUTS) sa= curarea;
	else {
		/* find area */
		sa= G.curscreen->areabase.first;
		while(sa) {
			if(sa->spacetype==SPACE_BUTS) break;
			sa= sa->next;
		}
	}
	
	if(sa==0) return;
	
	if(sa!=curarea) areawinset(sa->win);
	
	sbuts= sa->spacedata.first;
	
	if(fkey==F4KEY) {
		sbuts->mainb= CONTEXT_LOGIC;
	}
	else if(fkey==F5KEY) {
		/* if it's already in shading context, cycle between tabs with the same key */
		if (sbuts->oldkeypress == F5KEY) {

			if (sbuts->tab[CONTEXT_SHADING]==TAB_SHADING_LAMP)
				sbuts->tab[CONTEXT_SHADING]=TAB_SHADING_MAT;
			else if (sbuts->tab[CONTEXT_SHADING]==TAB_SHADING_MAT)
				sbuts->tab[CONTEXT_SHADING]=TAB_SHADING_TEX;
			else if (sbuts->tab[CONTEXT_SHADING]==1) {
				sbuts->tab[CONTEXT_SHADING]=TAB_SHADING_RAD;
			}
			else if (sbuts->tab[CONTEXT_SHADING]==TAB_SHADING_RAD)
				sbuts->tab[CONTEXT_SHADING]=TAB_SHADING_WORLD;
			else if (sbuts->tab[CONTEXT_SHADING]==TAB_SHADING_WORLD)
				sbuts->tab[CONTEXT_SHADING]=TAB_SHADING_LAMP;
		}
		/* if we're coming in from texture buttons, 
		or from outside the shading context, just go to the 'default' */
		else if (ob && ((sbuts->mainb!= CONTEXT_SHADING) || (sbuts->oldkeypress == F6KEY)) ) {
			sbuts->mainb= CONTEXT_SHADING;
			
			if(ob->type==OB_CAMERA) 
				sbuts->tab[CONTEXT_SHADING]= TAB_SHADING_WORLD;
			else if(ob->type==OB_LAMP) 
				sbuts->tab[CONTEXT_SHADING]= TAB_SHADING_LAMP;
			else  
				sbuts->tab[CONTEXT_SHADING]= TAB_SHADING_MAT;
		}
		else {
			sbuts->mainb= CONTEXT_SHADING;
			sbuts->tab[CONTEXT_SHADING]= TAB_SHADING_MAT;
		}
		BIF_preview_changed(ID_TE);
	}
	else if(fkey==F6KEY) {
		sbuts->mainb= CONTEXT_SHADING;
		sbuts->tab[CONTEXT_SHADING]= TAB_SHADING_TEX;
		BIF_preview_changed(ID_TE);
	}
	else if(fkey==F7KEY) {
		/* if it's already in object context, cycle between tabs with the same key */
		if (sbuts->oldkeypress == F7KEY) {

			if (sbuts->tab[CONTEXT_OBJECT]==TAB_OBJECT_OBJECT)
				sbuts->tab[CONTEXT_OBJECT]=TAB_OBJECT_PHYSICS;
			else if (sbuts->tab[CONTEXT_OBJECT]==TAB_OBJECT_PHYSICS)
				sbuts->tab[CONTEXT_OBJECT]=TAB_OBJECT_OBJECT;
		}
		else sbuts->mainb= CONTEXT_OBJECT;
		
	}
	else if(fkey==F8KEY) {
		sbuts->mainb= CONTEXT_SHADING;
		sbuts->tab[CONTEXT_SHADING]= TAB_SHADING_WORLD;
	}
	else if(fkey==F9KEY) sbuts->mainb= CONTEXT_EDITING;
	else if(fkey==F10KEY) {
		/* if it's already in scene context, cycle between tabs with the same key */
		if (sbuts->oldkeypress == F10KEY) {

			if (sbuts->tab[CONTEXT_SCENE]==TAB_SCENE_RENDER)
				sbuts->tab[CONTEXT_SCENE]=TAB_SCENE_ANIM;
			else if (sbuts->tab[CONTEXT_SCENE]==TAB_SCENE_ANIM)
				sbuts->tab[CONTEXT_SCENE]=TAB_SCENE_SOUND;
			else if (sbuts->tab[CONTEXT_SCENE]==TAB_SCENE_SOUND)
				sbuts->tab[CONTEXT_SCENE]=TAB_SCENE_RENDER;
		}
		else sbuts->mainb= CONTEXT_SCENE;
	}

	sbuts->oldkeypress = fkey;

	scrarea_queue_headredraw(sa);
	scrarea_queue_winredraw(sa);
}

/* ******************** SPACE: SEQUENCE ********************** */

/*  extern void drawseqspace(ScrArea *sa, void *spacedata); BIF_drawseq.h */

static void winqreadseqspace(ScrArea *sa, void *spacedata, BWinEvent *evt)
{
	unsigned short event= evt->event;
	short val= evt->val;
	SpaceSeq *sseq= curarea->spacedata.first;
	View2D *v2d= &sseq->v2d;
	Sequence *last_seq = get_last_seq();
	float dx, dy;
	int doredraw= 0, cfra, first;
	short mval[2];
	short nr;
	short mousebut = L_MOUSE;
	
	if(curarea->win==0) return;

	if(val) {
		if( uiDoBlocks(&curarea->uiblocks, event)!=UI_NOTHING ) event= 0;
		
		/* swap mouse buttons based on user preference */
		if (U.flag & USER_LMOUSESELECT) {
			if (event == LEFTMOUSE) {
				event = RIGHTMOUSE;
				mousebut = L_MOUSE;
			} else if (event == RIGHTMOUSE) {
				event = LEFTMOUSE;
				mousebut = R_MOUSE;
			}
		}
		
		switch(event) {
		case UI_BUT_EVENT:
			do_seqbuttons(val);			
			break;
		case LEFTMOUSE:
			if(sseq->mainb || view2dmove(event)==0) {
				
				first= 1;		
				set_special_seq_update(1);

				do {
					getmouseco_areawin(mval);
					areamouseco_to_ipoco(v2d, mval, &dx, &dy);
					
					cfra= (int)dx;
					if(cfra< 1) cfra= 1;
					/* else if(cfra> EFRA) cfra= EFRA; */
					
					if( cfra!=CFRA || first ) {
						first= 0;
				
						CFRA= cfra;
						force_draw(0);
						update_for_newframe();	/* for audio scrubbing */						
					}
					else PIL_sleep_ms(30);
				
				} while(get_mbut() & mousebut);
				
				set_special_seq_update(0);
				
				update_for_newframe();
			}
			break;
		case MIDDLEMOUSE:
			if(sseq->mainb) seq_viewmove(sseq);
			else view2dmove(event);	/* in drawipo.c */
			break;
		case RIGHTMOUSE:
			if(sseq->mainb) break;
			mouse_select_seq();
			break;
		case PADPLUSKEY:
		case WHEELUPMOUSE:
			if(sseq->mainb) {
				sseq->zoom++;
				if(sseq->zoom==-1) sseq->zoom= 1;
				if(sseq->zoom>8) sseq->zoom= 8;
			}
			else {
				if((G.qual==0)) {
					dx= 0.1154f*(v2d->cur.xmax-v2d->cur.xmin);
					v2d->cur.xmin+= dx;
					v2d->cur.xmax-= dx;
					test_view2d(G.v2d, sa->winx, sa->winy);
					view2d_do_locks(sa, V2D_LOCK_COPY);
				}
				else if((G.qual==LR_SHIFTKEY)) {
					insert_gap(25, CFRA);
					BIF_undo_push("Insert gaps Sequencer");
					allqueue(REDRAWSEQ, 0);
				}
				else if(G.qual==LR_ALTKEY) {
					insert_gap(250, CFRA);
					BIF_undo_push("Insert gaps Sequencer");
					allqueue(REDRAWSEQ, 0);
				}
			}
			doredraw= 1;
			break;
		case PADMINUS:
		case WHEELDOWNMOUSE:
			if(sseq->mainb) {
				sseq->zoom--;
				if(sseq->zoom==0) sseq->zoom= -2;
				if(sseq->zoom<-8) sseq->zoom= -8;
			}
			else {
				if((G.qual==LR_SHIFTKEY))
					no_gaps();
				else if((G.qual==0)) {
					dx= 0.15f*(v2d->cur.xmax-v2d->cur.xmin);
					v2d->cur.xmin-= dx;
					v2d->cur.xmax+= dx;
					test_view2d(G.v2d, sa->winx, sa->winy);
					view2d_do_locks(sa, V2D_LOCK_COPY);
				}
			}
			doredraw= 1;
			break;
		case HOMEKEY:
			if((G.qual==0))
				do_seq_buttons(B_SEQHOME);
			break;
		case PADPERIOD:	
			if(last_seq) {
				CFRA= last_seq->startdisp;
				v2d->cur.xmin= last_seq->startdisp- (last_seq->len/20);
				v2d->cur.xmax= last_seq->enddisp+ (last_seq->len/20);
				update_for_newframe();
			}
			break;
			
		case AKEY:
			if(sseq->mainb) break;
			if((G.qual==LR_SHIFTKEY)) {
				add_sequence(-1);
			}
			else if((G.qual==0))
				swap_select_seq();
			break;
		case SPACEKEY:
			if (G.qual==0) {
				if (sseq->mainb) {
					play_anim(1);
				} else {
					add_sequence(-1);
				}
			}
			break;
		case BKEY:
			if(sseq->mainb) break;
			if((G.qual==0))
				borderselect_seq();
			break;
		case CKEY:
			if((G.qual==0)) {
				if(last_seq && (last_seq->flag & (SEQ_LEFTSEL+SEQ_RIGHTSEL))) {
					if(last_seq->flag & SEQ_LEFTSEL) CFRA= last_seq->startdisp;
					else CFRA= last_seq->enddisp-1;
					
					dx= CFRA-(v2d->cur.xmax+v2d->cur.xmin)/2;
					v2d->cur.xmax+= dx;
					v2d->cur.xmin+= dx;
					update_for_newframe();
				}
				else
					change_sequence();
			}
			break;
		case DKEY:
			if(sseq->mainb) break;
			if((G.qual==LR_SHIFTKEY)) add_duplicate_seq();
			break;
		case EKEY:
			break;
		case FKEY:
			if((G.qual==0))
				set_filter_seq();
			break;
		case GKEY:
			if(sseq->mainb) break;
			if((G.qual==0))
				transform_seq('g', 0);
			break;
		case KKEY:
			if((G.qual==0)) { /* Cut at current frame */
				if(okee("Cut strips")) seq_cut(CFRA);
			}
			break;
		case MKEY:
			if(G.qual==LR_ALTKEY)
				un_meta();
			else if((G.qual==0)){
				if ((last_seq) && 
				    (last_seq->type == SEQ_RAM_SOUND
				     || last_seq->type == SEQ_HD_SOUND)) 
				{
					last_seq->flag ^= SEQ_MUTE;
					doredraw = 1;
				}
				else
					make_meta();
			}
			break;
		case NKEY:
			if(G.qual==0) {
				toggle_blockhandler(curarea, SEQ_HANDLER_PROPERTIES, UI_PNL_TO_MOUSE);
				scrarea_queue_winredraw(curarea);
			}
			break;
		case SKEY:
			if((G.qual==LR_SHIFTKEY))
				seq_snap_menu();
			break;
		case PKEY:
			if((G.qual==0))
				touch_seq_files();
			break;
		case TKEY: /* popup menu */
                       nr= pupmenu("Time value%t|Frames %x1|Seconds%x2");
                       if (nr>0) {
                               if(nr==1) sseq->flag |= SEQ_DRAWFRAMES;
                               else sseq->flag &= ~SEQ_DRAWFRAMES;
                               doredraw= 1;
                       }
                       break;
		case XKEY:
		case DELKEY:
			if(G.qual==0) {
				if(sseq->mainb) break;
				if((G.qual==0))
					del_seq();
			}
			break;
		}
	}

	if(doredraw) scrarea_queue_winredraw(curarea);
}


static void init_seqspace(ScrArea *sa)
{
	SpaceSeq *sseq;
	
	sseq= MEM_callocN(sizeof(SpaceSeq), "initseqspace");
	BLI_addhead(&sa->spacedata, sseq);

	sseq->spacetype= SPACE_SEQ;
	sseq->zoom= 4;
	sseq->blockscale= 0.7;
	sseq->chanshown = 0;
	
	/* seq space goes from (0,8) to (250, 0) */

	sseq->v2d.tot.xmin= 0.0;
	sseq->v2d.tot.ymin= 0.0;
	sseq->v2d.tot.xmax= 250.0;
	sseq->v2d.tot.ymax= 8.0;
	
	sseq->v2d.cur= sseq->v2d.tot;

	sseq->v2d.min[0]= 10.0;
	sseq->v2d.min[1]= 4.0;

	sseq->v2d.max[0]= 32000.0;
	sseq->v2d.max[1]= MAXSEQ;
	
	sseq->v2d.minzoom= 0.1f;
	sseq->v2d.maxzoom= 10.0;
	
	sseq->v2d.scroll= L_SCROLL+B_SCROLL;
	sseq->v2d.keepaspect= 0;
	sseq->v2d.keepzoom= 0;
	sseq->v2d.keeptot= 0;
}

/* ******************** SPACE: ACTION ********************** */
extern void drawactionspace(ScrArea *sa, void *spacedata);
extern void winqreadactionspace(struct ScrArea *sa, void *spacedata, struct BWinEvent *evt);

static void changeactionspace(ScrArea *sa, void *spacedata)
{
	if(G.v2d==0) return;

	/* this sets the sub-areas correct, for scrollbars */
	test_view2d(G.v2d, sa->winx, sa->winy);
	
	/* action space uses weird matrices... local calculated in a function */
	// myortho2(G.v2d->cur.xmin, G.v2d->cur.xmax, G.v2d->cur.ymin, G.v2d->cur.ymax);
}


static void init_actionspace(ScrArea *sa)
{
	SpaceAction *saction;
	
	saction= MEM_callocN(sizeof(SpaceAction), "initactionspace");
	BLI_addhead(&sa->spacedata, saction);

	saction->spacetype= SPACE_ACTION;
	saction->blockscale= 0.7;

	saction->v2d.tot.xmin= 1.0;
	saction->v2d.tot.ymin=	0.0;
	saction->v2d.tot.xmax= 1000.0;
	saction->v2d.tot.ymax= 1000.0;
	
	saction->v2d.cur.xmin= -5.0;
	saction->v2d.cur.ymin= 0.0;
	saction->v2d.cur.xmax= 65.0;
	saction->v2d.cur.ymax= 1000.0;

	saction->v2d.min[0]= 0.0;
	saction->v2d.min[1]= 0.0;

	saction->v2d.max[0]= 32000.0;
	saction->v2d.max[1]= 1000.0;
	
	saction->v2d.minzoom= 0.01;
	saction->v2d.maxzoom= 50;

	saction->v2d.scroll= R_SCROLL+B_SCROLL;
	saction->v2d.keepaspect= 0;
	saction->v2d.keepzoom= V2D_LOCKZOOM_Y;
	saction->v2d.keeptot= 0;
	
}

static void free_actionspace(SpaceAction *saction)
{
	/* don't free saction itself */
	
	/* __PINFAKE */
/*	if (saction->flag & SACTION_PIN)
		if (saction->action)
			saction->action->id.us --;

*/	/* end PINFAKE */
}


/* ******************** SPACE: FILE ********************** */

static void init_filespace(ScrArea *sa)
{
	SpaceFile *sfile;
	
	sfile= MEM_callocN(sizeof(SpaceFile), "initfilespace");
	BLI_addhead(&sa->spacedata, sfile);

	sfile->dir[0]= '/';
	sfile->type= FILE_UNIX;
	sfile->blockscale= 0.7;
	sfile->spacetype= SPACE_FILE;
}

static void init_imaselspace(ScrArea *sa)
{
	SpaceImaSel *simasel;
	
	simasel= MEM_callocN(sizeof(SpaceImaSel), "initimaselspace");
	BLI_addhead(&sa->spacedata, simasel);

	simasel->spacetype= SPACE_IMASEL;
	simasel->blockscale= 0.7;
	simasel->mode = 7;
	strcpy (simasel->dir,  U.textudir);	/* TON */
	strcpy (simasel->file, "");
	strcpy(simasel->fole, simasel->file);
	strcpy(simasel->dor,  simasel->dir);

	simasel->first_sel_ima	=  0;
	simasel->hilite_ima	    =  0;
	simasel->firstdir		=  0;
	simasel->firstfile		=  0;
	simasel->cmap           =  0;
	simasel->returnfunc     =  0;
	
	simasel->title[0]       =  0;
	
	clear_ima_dir(simasel);
	
	// simasel->cmap= IMB_loadiffmem((int*)datatoc_cmap_tga, IB_rect|IB_cmap);
	simasel->cmap= IMB_ibImageFromMemory((int *)datatoc_cmap_tga, datatoc_cmap_tga_size, IB_rect|IB_cmap);
	if (!simasel->cmap) {
		error("in console");
		printf("Image select cmap file not found \n");
	}
}

/* ******************** SPACE: SOUND ********************** */

extern void drawsoundspace(ScrArea *sa, void *spacedata);
extern void winqreadsoundspace(struct ScrArea *sa, void *spacedata, struct BWinEvent *evt);

static void init_soundspace(ScrArea *sa)
{
	SpaceSound *ssound;
	
	ssound= MEM_callocN(sizeof(SpaceSound), "initsoundspace");
	BLI_addhead(&sa->spacedata, ssound);

	ssound->spacetype= SPACE_SOUND;
	ssound->blockscale= 0.7;
	/* sound space goes from (0,8) to (250, 0) */

	ssound->v2d.tot.xmin= -4.0;
	ssound->v2d.tot.ymin= -4.0;
	ssound->v2d.tot.xmax= 250.0;
	ssound->v2d.tot.ymax= 255.0;
	
	ssound->v2d.cur.xmin= -4.0;
	ssound->v2d.cur.ymin= -4.0;
	ssound->v2d.cur.xmax= 50.0;
	ssound->v2d.cur.ymax= 255.0;

	ssound->v2d.min[0]= 1.0;
	ssound->v2d.min[1]= 259.0;

	ssound->v2d.max[0]= 32000.0;
	ssound->v2d.max[1]= 259;
	
	ssound->v2d.minzoom= 0.1f;
	ssound->v2d.maxzoom= 10.0;
	
	ssound->v2d.scroll= B_SCROLL;
	ssound->v2d.keepaspect= 0;
	ssound->v2d.keepzoom= 0;
	ssound->v2d.keeptot= 0;
	
}

void free_soundspace(SpaceSound *ssound)
{
	/* don't free ssound itself */
	
	
}

/* ******************** SPACE: IMAGE ********************** */

static void changeimagepace(ScrArea *sa, void *spacedata)
{
	image_preview_event(2);
}

static void winqreadimagespace(ScrArea *sa, void *spacedata, BWinEvent *evt)
{
	SpaceImage *sima= spacedata;
	unsigned short event= evt->event, origevent= evt->event;
	short val= evt->val;
	
	if(val==0) return;

	if(uiDoBlocks(&sa->uiblocks, event)!=UI_NOTHING ) event= 0;
	
	if (U.flag & USER_LMOUSESELECT) {
		if (event == LEFTMOUSE) {
			event = RIGHTMOUSE;
		} else if (event == RIGHTMOUSE) {
			event = LEFTMOUSE;
		}
	}

	if (sima->flag & SI_DRAWTOOL) {
		switch(event) {
			case CKEY:
			       	toggle_blockhandler(sa, IMAGE_HANDLER_PAINT, UI_PNL_UNSTOW);
				scrarea_queue_winredraw(sa);
				break;
			case LEFTMOUSE:
				imagepaint_paint(origevent==LEFTMOUSE? L_MOUSE: R_MOUSE);
				break;
			case RIGHTMOUSE:
				imagepaint_pick(origevent==LEFTMOUSE? L_MOUSE: R_MOUSE);
				break;
		}
	}
	else {
		/* Draw tool is inactive */
		switch(event) {
			case LEFTMOUSE:
				if(G.qual & LR_SHIFTKEY) {
					if(G.sima->image && G.sima->image->tpageflag & IMA_TILES)
						mouseco_to_curtile();
					else
						sima_sample_color();
				}
				else if(G.f & G_FACESELECT)
					gesture();
				else 
					sima_sample_color();
				break;
			case RIGHTMOUSE:
				if(G.f & G_FACESELECT)
					mouse_select_sima();
				else if(G.f & (G_VERTEXPAINT|G_TEXTUREPAINT))
					sample_vpaint();
				break;
			case AKEY:
				select_swap_tface_uv();
				break;
			case BKEY:
				if(G.qual==LR_SHIFTKEY)
					borderselect_sima(UV_SELECT_PINNED);
				else if((G.qual==0))
					borderselect_sima(UV_SELECT_ALL);
				break;
			case CKEY:
				if(G.qual==LR_CTRLKEY)
					toggle_uv_select('s');
				else if(G.qual==LR_SHIFTKEY)
					toggle_uv_select('l');
				else if(G.qual==LR_ALTKEY)
					toggle_uv_select('o');
				else
					toggle_uv_select('f');
				break;
			case EKEY :
				if(okee("Unwrap"))
					unwrap_lscm(0);
				break;
			case GKEY:
				if((G.qual==0) && is_uv_tface_editing_allowed()) {
					initTransform(TFM_TRANSLATION, CTX_NONE);
					Transform();
				}
				break;
			case HKEY:
				if(G.qual==LR_ALTKEY)
					reveal_tface_uv();
				else if((G.qual==LR_SHIFTKEY))
					hide_tface_uv(1);
				else if((G.qual==0))
					hide_tface_uv(0);
				break;
			case LKEY:
				if(G.qual==0)
					select_linked_tface_uv(0);
				else if(G.qual==LR_SHIFTKEY)
					select_linked_tface_uv(1);
				else if(G.qual==LR_CTRLKEY)
					select_linked_tface_uv(2);
				else if(G.qual==LR_ALTKEY)
					unlink_selection();
				break;
			case MKEY:
				if((G.qual==0))
					mirrormenu_tface_uv();
				break;
			case NKEY:
				if(G.qual==LR_CTRLKEY)
					replace_names_but();
				break;
			case OKEY:
				if (G.qual==LR_SHIFTKEY) {
					G.scene->prop_mode = (G.scene->prop_mode+1)%6;
					allqueue(REDRAWHEADERS, 0);
				}
				else if((G.qual==0)) {
					G.scene->proportional= !G.scene->proportional;
				}
				break;
			case PKEY:
				if(G.f & G_FACESELECT) {
					if(G.qual==LR_SHIFTKEY)
						select_pinned_tface_uv();
					else if(G.qual==LR_ALTKEY)
						pin_tface_uv(0);
					else
						pin_tface_uv(1);
				}
				else {
					if(G.qual==LR_SHIFTKEY) {
						toggle_blockhandler(sa, IMAGE_HANDLER_PREVIEW, 0);
						scrarea_queue_winredraw(sa);
					}
				}
				break;
			case RKEY:
				if((G.qual==0) && is_uv_tface_editing_allowed()) {
					initTransform(TFM_ROTATION, CTX_NONE);
					Transform();
				}
				break;
			case SKEY:
				if((G.qual==0) && is_uv_tface_editing_allowed()) {
					initTransform(TFM_RESIZE, CTX_NONE);
					Transform();
				}
				break;
			case VKEY:
				if((G.qual==0))
					stitch_uv_tface(0);
				else if(G.qual==LR_SHIFTKEY)
					stitch_uv_tface(1);
				else if(G.qual==LR_CTRLKEY)
					minimize_stretch_tface_uv();
				break;
			case WKEY:
				weld_align_menu_tface_uv();
				break;
			case PADPERIOD:
				if(G.qual==0)
					image_viewcentre();
				break;
		}
	}	


	/* least intrusive nonumpad hack, only for plus/minus */
	if (U.flag & USER_NONUMPAD) {
		event= convert_for_nonumpad(event);
	}

	/* Events handled always (whether the draw tool is active or not) */
	switch (event) {
	case UI_BUT_EVENT:
		do_imagebuts(val);	// drawimage.c
		break;
	case MIDDLEMOUSE:
		if((G.qual==LR_CTRLKEY) || ((U.flag & USER_TWOBUTTONMOUSE) && (G.qual==(LR_ALTKEY|LR_CTRLKEY))))
			image_viewmove(1);
		else
			image_viewmove(0);
		break;
	case WHEELUPMOUSE: case WHEELDOWNMOUSE: case PADPLUSKEY: case PADMINUS:
	case PAD1: case PAD2: case PAD4: case PAD8:
		image_viewzoom(event, (G.qual & LR_SHIFTKEY)==0);
		scrarea_queue_winredraw(sa);
		break;
	case HOMEKEY:
		if((G.qual==0))
			image_home();
			
		break;
	case NKEY:
		if(G.qual==0) {
			toggle_blockhandler(sa, IMAGE_HANDLER_PROPERTIES, UI_PNL_TO_MOUSE);
			scrarea_queue_winredraw(sa);
		}
		break;
	case ESCKEY:
		if(sima->flag & SI_PREVSPACE) {
			/* only allow ESC once */
			sima->flag &= ~SI_PREVSPACE;
			
			sima= sa->spacedata.first;
			if(sima->next) {
				SpaceLink *sl;
				
				BLI_remlink(&sa->spacedata, sima);
				BLI_addtail(&sa->spacedata, sima);
				
				sl= sa->spacedata.first;
				
				newspace(sa, sl->spacetype);
			}
		}
		if(sima->flag & SI_FULLWINDOW) {
			sima->flag &= ~SI_FULLWINDOW;
			if(sa->full)
				area_fullscreen();
		}
	}
}


static void init_imagespace(ScrArea *sa)
{
	SpaceImage *sima;
	
	sima= MEM_callocN(sizeof(SpaceImage), "initimaspace");
	BLI_addhead(&sa->spacedata, sima);

	sima->spacetype= SPACE_IMAGE;
	sima->zoom= 1;
	sima->blockscale= 0.7;
	sima->flag = SI_LOCALSTICKY;
}


/* ******************** SPACE: IMASEL ********************** */

extern void drawimaselspace(ScrArea *sa, void *spacedata);
extern void winqreadimaselspace(struct ScrArea *sa, void *spacedata, struct BWinEvent *evt);


/* everything to imasel.c */


/* ******************** SPACE: OOPS ********************** */

extern void drawoopsspace(ScrArea *sa, void *spacedata);

static void winqreadoopsspace(ScrArea *sa, void *spacedata, BWinEvent *evt)
{
	unsigned short event= evt->event;
	short val= evt->val;
	SpaceOops *soops= curarea->spacedata.first;
	View2D *v2d= &soops->v2d;
	float dx, dy;

	if(val==0) return;

	if( uiDoBlocks(&sa->uiblocks, event)!=UI_NOTHING ) event= 0;

	if (U.flag & USER_NONUMPAD) {
		event= convert_for_nonumpad(event);
	}
	
	/* keep leftmouse select for outliner, regardless of user pref */ 
	if(soops->type==SO_OUTLINER) {
		switch(event) {
		case LEFTMOUSE:
			outliner_mouse_event(sa, event);			
			break;
		case MIDDLEMOUSE:
		case WHEELUPMOUSE:
		case WHEELDOWNMOUSE:
			view2dmove(event);	/* in drawipo.c */
			break;
		case RIGHTMOUSE:
			outliner_operation_menu(sa);
			break;
			
		case AKEY:
			if(G.qual==LR_SHIFTKEY)
				outliner_toggle_selected(sa);
			else
				outliner_toggle_visible(sa);
			break;
		case WKEY:
			outliner_operation_menu(sa);
			break;
			
		case HOMEKEY:
			outliner_show_hierarchy(sa);
			break;
		case PAGEUPKEY:
			outliner_page_up_down(sa, 1);
			break;
		case PAGEDOWNKEY:
			outliner_page_up_down(sa, -1);
			break;
			
		case RETKEY:
		case PADENTER:
			outliner_mouse_event(sa, event);
			break;
		case PERIODKEY:
		case PADPERIOD:
			outliner_show_active(sa);
			break;
		case PADPLUSKEY:
			outliner_one_level(sa, 1);
			break;
		case PADMINUS:
			outliner_one_level(sa, -1);
			break;
		}
	}
	else {	
		/* swap mouse buttons based on user preference */
		if (U.flag & USER_LMOUSESELECT) {
			if (event==LEFTMOUSE) event = RIGHTMOUSE;
			else if (event==RIGHTMOUSE) event = LEFTMOUSE;
		}
		
		switch(event) {
		case LEFTMOUSE:
			gesture();
			break;
		case MIDDLEMOUSE:
		case WHEELUPMOUSE:
		case WHEELDOWNMOUSE:
			view2dmove(event);	/* in drawipo.c */
			break;
		case RIGHTMOUSE:
			mouse_select_oops();
			break;
		case PADPLUSKEY:
		
			dx= 0.1154*(v2d->cur.xmax-v2d->cur.xmin);
			dy= 0.1154*(v2d->cur.ymax-v2d->cur.ymin);
			v2d->cur.xmin+= dx;
			v2d->cur.xmax-= dx;
			v2d->cur.ymin+= dy;
			v2d->cur.ymax-= dy;
			test_view2d(G.v2d, curarea->winx, curarea->winy);
			scrarea_queue_winredraw(curarea);
			break;
		
		case PADMINUS:

			dx= 0.15*(v2d->cur.xmax-v2d->cur.xmin);
			dy= 0.15*(v2d->cur.ymax-v2d->cur.ymin);
			v2d->cur.xmin-= dx;
			v2d->cur.xmax+= dx;
			v2d->cur.ymin-= dy;
			v2d->cur.ymax+= dy;
			test_view2d(G.v2d, curarea->winx, curarea->winy);
			scrarea_queue_winredraw(curarea);
			break;
			
		case HOMEKEY:	
			if((G.qual==0))
				do_oops_buttons(B_OOPSHOME);
			break;
		
		case PADPERIOD:
			if((G.qual==0))
				do_oops_buttons(B_OOPSVIEWSEL);
			break;
			
		case AKEY:
			if((G.qual==0)) {
				swap_select_all_oops();
				scrarea_queue_winredraw(curarea);
			}
			break;
		case BKEY:
			if((G.qual==0))
				borderselect_oops();
			break;
		case GKEY:
			if((G.qual==0))
				transform_oops('g', 0);
			break;
		case LKEY:
			if((G.qual==LR_SHIFTKEY))
				select_backlinked_oops();
			else if((G.qual==0))
				select_linked_oops();
			break;
		case SKEY:
			if((G.qual==LR_ALTKEY)) {
				if (okee("Shrink blocks")) {
					shrink_oops();
				}
			} else if((G.qual==LR_SHIFTKEY)) {
				if (okee("Shuffle blocks")) {
					shuffle_oops();
				}
			} else if((G.qual==0)) {
				transform_oops('s', 0);
			}
			break;
		case PKEY:
			if((G.qual==LR_CTRLKEY)) {
				make_parent();
			} else if((G.qual==LR_ALTKEY)) {
				clear_parent();
			}
			break;


		case ONEKEY:
			do_layer_buttons(0); break;
		case TWOKEY:
			do_layer_buttons(1); break;
		case THREEKEY:
			do_layer_buttons(2); break;
		case FOURKEY:
			do_layer_buttons(3); break;
		case FIVEKEY:
			do_layer_buttons(4); break;
		case SIXKEY:
			do_layer_buttons(5); break;
		case SEVENKEY:
			do_layer_buttons(6); break;
		case EIGHTKEY:
			do_layer_buttons(7); break;
		case NINEKEY:
			do_layer_buttons(8); break;
		case ZEROKEY:
			do_layer_buttons(9); break;
		case MINUSKEY:
			do_layer_buttons(10); break;
		case EQUALKEY:
			do_layer_buttons(11); break;
		case ACCENTGRAVEKEY:
			do_layer_buttons(-1); break;
		
		}
	}
}

void init_v2d_oops(ScrArea *sa, SpaceOops *soops)
{
	View2D *v2d= &soops->v2d;
	
	if(soops->type==SO_OUTLINER) {
		/* outliner space is window size */
		calc_scrollrcts(sa, v2d, sa->winx, sa->winy);
		
		v2d->tot.xmax= 0.0;
		v2d->tot.ymax= 0.0;
		v2d->tot.xmin= -(v2d->mask.xmax-v2d->mask.xmin);
		v2d->tot.ymin= -(v2d->mask.ymax-v2d->mask.ymin);
		
		v2d->cur= v2d->tot;
		
		v2d->min[0]= v2d->tot.xmin;
		v2d->min[1]= v2d->tot.ymin;
		
		v2d->max[0]= v2d->tot.xmax;
		v2d->max[1]= v2d->tot.ymax;
		
		v2d->minzoom= 1.0;
		v2d->maxzoom= 1.0;
		
		v2d->scroll= L_SCROLL;
		v2d->keepaspect= 1;
		v2d->keepzoom= 1;
		v2d->keeptot= 1;
	}
	else {
		v2d->tot.xmin= -28.0;
		v2d->tot.xmax= 28.0;
		v2d->tot.ymin= -28.0;
		v2d->tot.ymax= 28.0;
		
		v2d->cur= v2d->tot;

		v2d->min[0]= 10.0;
		v2d->min[1]= 4.0;

		v2d->max[0]= 320.0;
		v2d->max[1]= 320.0;
		
		v2d->minzoom= 0.01f;
		v2d->maxzoom= 2.0;
		
		/* v2d->scroll= L_SCROLL+B_SCROLL; */
		v2d->scroll= 0;
		v2d->keepaspect= 1;
		v2d->keepzoom= 0;
		v2d->keeptot= 0;
	}
}

static void init_oopsspace(ScrArea *sa, int outliner)
{
	SpaceOops *soops;
	
	soops= MEM_callocN(sizeof(SpaceOops), "initoopsspace");
	BLI_addhead(&sa->spacedata, soops);

	soops->visiflag= OOPS_OB+OOPS_MA+OOPS_ME+OOPS_TE+OOPS_CU+OOPS_IP;
	if(outliner) soops->type= SO_OUTLINER;
	
	soops->spacetype= SPACE_OOPS;
	soops->type= SO_OUTLINER;	// default starts new ones in outliner mode
	soops->blockscale= 0.7;
	init_v2d_oops(sa, soops);
}

/* ******************** SPACE: NLA ********************** */

static void init_nlaspace(ScrArea *sa)
{
	SpaceNla *snla;
	
	snla= MEM_callocN(sizeof(SpaceNla), "initnlaspace");
	BLI_addhead(&sa->spacedata, snla);
	
	snla->spacetype= SPACE_NLA;
	snla->blockscale= 0.7;
	
	snla->v2d.tot.xmin= 1.0;
	snla->v2d.tot.ymin=	0.0;
	snla->v2d.tot.xmax= 1000.0;
	snla->v2d.tot.ymax= 1000.0;
	
	snla->v2d.cur.xmin= -5.0;
	snla->v2d.cur.ymin= 0.0;
	snla->v2d.cur.xmax= 65.0;
	snla->v2d.cur.ymax= 1000.0;
	
	snla->v2d.min[0]= 0.0;
	snla->v2d.min[1]= 0.0;
	
	snla->v2d.max[0]= 1000.0;
	snla->v2d.max[1]= 1000.0;
	
	snla->v2d.minzoom= 0.1F;
	snla->v2d.maxzoom= 50;
	
	snla->v2d.scroll= R_SCROLL+B_SCROLL;
	snla->v2d.keepaspect= 0;
	snla->v2d.keepzoom= V2D_LOCKZOOM_Y;
	snla->v2d.keeptot= 0;
	
	snla->lock = 0;
}



/* ******************** SPACE: Text ********************** */

extern void drawtextspace(ScrArea *sa, void *spacedata);
extern void winqreadtextspace(struct ScrArea *sa, void *spacedata, struct BWinEvent *evt);

static void init_textspace(ScrArea *sa)
{
	SpaceText *st;
	
	st= MEM_callocN(sizeof(SpaceText), "inittextspace");
	BLI_addhead(&sa->spacedata, st);
	
	st->spacetype= SPACE_TEXT;	
	st->blockscale= 0.7;
	st->text= NULL;
	st->flags= 0;
	
	st->font_id= 5;
	st->lheight= 12;
	st->showlinenrs= 0;
	st->tabnumber = 4;
	st->currtab_set = 0;
	
	st->top= 0;
}


/* ******************** SPACE: Script ********************** */

extern void drawscriptspace(ScrArea *sa, void *spacedata);
extern void winqreadscriptspace(struct ScrArea *sa, void *spacedata, struct BWinEvent *evt);

static void init_scriptspace(ScrArea *sa)
{
	SpaceScript *sc;
	
	sc = MEM_callocN(sizeof(SpaceScript), "initscriptspace");
	BLI_addhead(&sa->spacedata, sc);
	
	sc->spacetype = SPACE_SCRIPT;
	sc->blockscale= 0.7;
	sc->script = NULL;
	sc->flags = 0;
}


/* ******************** SPACE: Time ********************** */

extern void drawtimespace(ScrArea *sa, void *spacedata);
extern void winqreadtimespace(struct ScrArea *sa, void *spacedata, struct BWinEvent *evt);

static void init_timespace(ScrArea *sa)
{
	SpaceTime *stime;
	
	stime= MEM_callocN(sizeof(SpaceTime), "init timespace");
	BLI_addhead(&sa->spacedata, stime);
	
	stime->spacetype= SPACE_TIME;
	stime->blockscale= 0.7;
	stime->redraws= TIME_ALL_3D_WIN|TIME_ALL_ANIM_WIN;
	
	stime->v2d.tot.xmin= -4.0;
	stime->v2d.tot.ymin=  0.0;
	stime->v2d.tot.xmax= (float)EFRA + 4.0;
	stime->v2d.tot.ymax= (float)sa->winy;
	
	stime->v2d.cur= stime->v2d.tot;
	
	stime->v2d.min[0]= 1.0;
	stime->v2d.min[1]= (float)sa->winy;
	
	stime->v2d.max[0]= 32000.0;
	stime->v2d.max[1]= (float)sa->winy;
	
	stime->v2d.minzoom= 0.1f;
	stime->v2d.maxzoom= 10.0;
	
	stime->v2d.scroll= 0;
	stime->v2d.keepaspect= 0;
	stime->v2d.keepzoom= 0;
	stime->v2d.keeptot= 0;
	
	stime->flag |= TIME_DRAWFRAMES;
	
}

/* ******************** SPACE: Time ********************** */

extern void drawnodespace(ScrArea *sa, void *spacedata);
extern void winqreadnodespace(struct ScrArea *sa, void *spacedata, struct BWinEvent *evt);

static void init_nodespace(ScrArea *sa)
{
	SpaceNode *snode;
	
	snode= MEM_callocN(sizeof(SpaceNode), "init nodespace");
	BLI_addhead(&sa->spacedata, snode);
	
	snode->spacetype= SPACE_NODE;
	snode->blockscale= 0.7;
	
	snode->v2d.tot.xmin=  -10.0;
	snode->v2d.tot.ymin=  -10.0;
	snode->v2d.tot.xmax= (float)sa->winx + 10.0f;
	snode->v2d.tot.ymax= (float)sa->winy + 10.0f;
	
	snode->v2d.cur.xmin=  0.0;
	snode->v2d.cur.ymin=  0.0;
	snode->v2d.cur.xmax= (float)sa->winx;
	snode->v2d.cur.ymax= (float)sa->winy;
	
	snode->v2d.min[0]= 1.0;
	snode->v2d.min[1]= 1.0;
	
	snode->v2d.max[0]= 32000.0f;
	snode->v2d.max[1]= 32000.0f;
	
	snode->v2d.minzoom= 0.5f;
	snode->v2d.maxzoom= 1.21f;
	
	snode->v2d.scroll= 0;
	snode->v2d.keepaspect= 1;
	snode->v2d.keepzoom= 1;
	snode->v2d.keeptot= 0;
}



/* ******************** SPACE: GENERAL ********************** */

void newspace(ScrArea *sa, int type)
{
	int xtra= type & 256;	// hack to enforce outliner with hotkey from toets.c
	
	type &= ~256;
	
	if(type>=0) {
		if(sa->spacetype != type) {
			SpaceLink *sl;
			
			sa->spacetype= type;
			sa->headbutofs= 0;
			
			uiFreeBlocks(&sa->uiblocks);
			wich_cursor(sa);
			
			if (sa->headwin) addqueue(sa->headwin, CHANGED, 1);
			scrarea_queue_headredraw(sa);

			addqueue(sa->win, CHANGED, 1);
			scrarea_queue_winredraw(sa);

			areawinset(sa->win);

			bwin_clear_viewmat(sa->win);
			
			for (sl= sa->spacedata.first; sl; sl= sl->next)
				if(sl->spacetype==type)
					break;

			if (sl) {			
				BLI_remlink(&sa->spacedata, sl);
				BLI_addhead(&sa->spacedata, sl);
			} else {
				if(type==SPACE_VIEW3D)
					initview3d(sa);
				else if(type==SPACE_IPO)
					initipo(sa);
				else if(type==SPACE_INFO)
					init_infospace(sa);
				else if(type==SPACE_BUTS)
					init_butspace(sa);
				else if(type==SPACE_FILE)
					init_filespace(sa);
				else if(type==SPACE_SEQ)
					init_seqspace(sa);
				else if(type==SPACE_IMAGE)
					init_imagespace(sa);
				else if(type==SPACE_IMASEL)
					init_imaselspace(sa);
				else if(type==SPACE_OOPS)
					init_oopsspace(sa, xtra);
				else if(type==SPACE_ACTION)
					init_actionspace(sa);
				else if(type==SPACE_TEXT)
					init_textspace(sa);
				else if(type==SPACE_SCRIPT)
					init_scriptspace(sa);
				else if(type==SPACE_SOUND)
					init_soundspace(sa);
				else if(type==SPACE_NLA)
					init_nlaspace(sa);
				else if(type==SPACE_TIME)
					init_timespace(sa);
				else if(type==SPACE_NODE)
					init_nodespace(sa);

				sl= sa->spacedata.first;
				sl->area= sa;
			}
		}
	}

		
	/* exception: filespace */
	if(sa->spacetype==SPACE_FILE) {
		SpaceFile *sfile= sa->spacedata.first;
		
		if(sfile->type==FILE_MAIN) {
			freefilelist(sfile);
		} else {
			sfile->type= FILE_UNIX;
		}
		
		sfile->returnfunc= 0;
		sfile->title[0]= 0;
		if(sfile->filelist) test_flags_file(sfile);
	}
	/* exception: imasel space */
	else if(sa->spacetype==SPACE_IMASEL) {
		SpaceImaSel *simasel= sa->spacedata.first;
		simasel->returnfunc= 0;
		simasel->title[0]= 0;
	}
	else if(sa->spacetype==SPACE_OOPS) {
		SpaceOops *so= sa->spacedata.first;
		if(xtra && so->type!=SO_OUTLINER) {
			so->type= SO_OUTLINER;
			init_v2d_oops(sa, so);
			scrarea_queue_winredraw(sa);
			scrarea_queue_headredraw(sa);
		}
	}
}

void freespacelist(ScrArea *sa)
{
	SpaceLink *sl;

	for (sl= sa->spacedata.first; sl; sl= sl->next) {
		if(sl->spacetype==SPACE_FILE) {
			SpaceFile *sfile= (SpaceFile*) sl;
			if(sfile->libfiledata)	
				BLO_blendhandle_close(sfile->libfiledata);
			if(sfile->filelist)
				freefilelist(sfile);
		}
		else if(sl->spacetype==SPACE_BUTS) {
			SpaceButs *buts= (SpaceButs*) sl;
			if(buts->ri) { 
				if (buts->ri->rect) MEM_freeN(buts->ri->rect);
				MEM_freeN(buts->ri);
			}
			if(G.buts==buts) G.buts= NULL;
		}
		else if(sl->spacetype==SPACE_IPO) {
			SpaceIpo *si= (SpaceIpo*) sl;
			if(si->editipo) MEM_freeN(si->editipo);
			free_ipokey(&si->ipokey);
			if(G.sipo==si) G.sipo= NULL;
		}
		else if(sl->spacetype==SPACE_VIEW3D) {
			View3D *vd= (View3D*) sl;
			if(vd->bgpic) {
				if(vd->bgpic->rect) MEM_freeN(vd->bgpic->rect);
				if(vd->bgpic->ima) vd->bgpic->ima->id.us--;
				MEM_freeN(vd->bgpic);
			}
			if(vd->localvd) MEM_freeN(vd->localvd);
			if(vd->clipbb) MEM_freeN(vd->clipbb);
			if(G.vd==vd) G.vd= NULL;
			if(vd->ri) { 
				BIF_view3d_previewrender_free(vd);
			}
		}
		else if(sl->spacetype==SPACE_OOPS) {
			free_oopspace((SpaceOops *)sl);
		}
		else if(sl->spacetype==SPACE_IMASEL) {
			free_imasel((SpaceImaSel *)sl);
		}
		else if(sl->spacetype==SPACE_ACTION) {
			free_actionspace((SpaceAction*)sl);
		}
		else if(sl->spacetype==SPACE_NLA){
/*			free_nlaspace((SpaceNla*)sl);	*/
		}
		else if(sl->spacetype==SPACE_TEXT) {
			free_textspace((SpaceText *)sl);
		}
		else if(sl->spacetype==SPACE_SCRIPT) {
			free_scriptspace((SpaceScript *)sl);
		}
		else if(sl->spacetype==SPACE_SOUND) {
			free_soundspace((SpaceSound *)sl);
		}
		else if(sl->spacetype==SPACE_IMAGE) {
			SpaceImage *sima= (SpaceImage *)sl;
			if(sima->cumap)
				curvemapping_free(sima->cumap);
			if(sima->info_str)
				MEM_freeN(sima->info_str);
		}
		else if(sl->spacetype==SPACE_NODE) {
/*			SpaceNode *snode= (SpaceNode *)sl; */
		}
	}

	BLI_freelistN(&sa->spacedata);
}

void duplicatespacelist(ScrArea *newarea, ListBase *lb1, ListBase *lb2)
{
	SpaceLink *sl;

	duplicatelist(lb1, lb2);
	
	/* lb1 is copy from lb2, from lb2 we free stuff, rely on event system to properly re-alloc */
	
	sl= lb2->first;
	while(sl) {
		if(sl->spacetype==SPACE_FILE) {
			SpaceFile *sfile= (SpaceFile*) sl;
			sfile->libfiledata= 0;
			sfile->filelist= 0;
		}
		else if(sl->spacetype==SPACE_VIEW3D) {
			BIF_view3d_previewrender_free((View3D *)sl);
		}
		else if(sl->spacetype==SPACE_OOPS) {
			SpaceOops *so= (SpaceOops *)sl;
			so->oops.first= so->oops.last= NULL;
			so->tree.first= so->tree.last= NULL;
			so->treestore= NULL;
		}
		else if(sl->spacetype==SPACE_IMASEL) {
			check_imasel_copy((SpaceImaSel *) sl);
		}
		else if(sl->spacetype==SPACE_NODE) {
			SpaceNode *snode= (SpaceNode *)sl;
			snode->nodetree= NULL;
		}

		sl= sl->next;
	}
	
	sl= lb1->first;
	while(sl) {
		sl->area= newarea;

		if(sl->spacetype==SPACE_BUTS) {
			SpaceButs *buts= (SpaceButs *)sl;
			buts->ri= NULL;
		}
		else if(sl->spacetype==SPACE_IPO) {
			SpaceIpo *si= (SpaceIpo *)sl;
			si->editipo= NULL;
			si->ipokey.first= si->ipokey.last= NULL;
		}
		else if(sl->spacetype==SPACE_VIEW3D) {
			View3D *vd= (View3D *)sl;
			if(vd->bgpic) {
				vd->bgpic= MEM_dupallocN(vd->bgpic);
				vd->bgpic->rect= NULL;
				if(vd->bgpic->ima) vd->bgpic->ima->id.us++;
			}
			vd->clipbb= MEM_dupallocN(vd->clipbb);
			vd->ri= NULL;
		}
		else if(sl->spacetype==SPACE_IMAGE) {
			SpaceImage *sima= (SpaceImage *)sl;
			if(sima->cumap)
				sima->cumap= curvemapping_copy(sima->cumap);
			if(sima->info_str)
				sima->info_str= MEM_dupallocN(sima->info_str);
		}
		sl= sl->next;
	}

	/* again: from old View3D restore localview (because full) */
	sl= lb2->first;
	while(sl) {
		if(sl->spacetype==SPACE_VIEW3D) {
			View3D *v3d= (View3D*) sl;
			if(v3d->localvd) {
				restore_localviewdata(v3d);
				v3d->localvd= NULL;
				v3d->localview= 0;
				v3d->lay &= 0xFFFFFF;
			}
		}
		sl= sl->next;
	}
}

/* is called everywhere in blender */
void allqueue(unsigned short event, short val)
{
	ScrArea *sa;
	View3D *v3d;
	SpaceButs *buts;
	SpaceFile *sfile;

	sa= G.curscreen->areabase.first;
	while(sa) {

		if(event==REDRAWALL) {
			scrarea_queue_winredraw(sa);
			scrarea_queue_headredraw(sa);
		}
		else if(sa->win != val) {
			switch(event) {
				
			case REDRAWHEADERS:
				scrarea_queue_headredraw(sa);
				break;
			case REDRAWVIEW3D:
				if(sa->spacetype==SPACE_VIEW3D) {
					scrarea_queue_winredraw(sa);
					if(val) scrarea_queue_headredraw(sa);
				}
				break;
			case REDRAWVIEW3D_Z:
				if(sa->spacetype==SPACE_VIEW3D) {
					v3d= sa->spacedata.first;
					if(v3d->drawtype==OB_SOLID) {
						scrarea_queue_winredraw(sa);
						if(val) scrarea_queue_headredraw(sa);
					}
				}
				break;
			case REDRAWVIEWCAM:
				if(sa->spacetype==SPACE_VIEW3D) {
					v3d= sa->spacedata.first;
					if(v3d->persp>1) scrarea_queue_winredraw(sa);
				}
				break;
			case REDRAWINFO:
				if(sa->spacetype==SPACE_INFO) {
					scrarea_queue_winredraw(sa);
					scrarea_queue_headredraw(sa);
				}
				break;
			case REDRAWIMAGE:
				if(sa->spacetype==SPACE_IMAGE) {
					scrarea_queue_winredraw(sa);
					scrarea_queue_headredraw(sa);
				}
				break;
			case REDRAWIPO:
				if(sa->spacetype==SPACE_IPO) {
					SpaceIpo *si;
					scrarea_queue_winredraw(sa);
					scrarea_queue_headredraw(sa);
					if(val) {
						si= sa->spacedata.first;
						if (si->pin==0)				
							si->blocktype= val;
					}
				}
				else if(sa->spacetype==SPACE_OOPS) {
					scrarea_queue_winredraw(sa);
				}
				
				break;
				
			case REDRAWBUTSALL:
				if(sa->spacetype==SPACE_BUTS) {
					buts= sa->spacedata.first;
					buts->re_align= 1;
					scrarea_queue_winredraw(sa);
					scrarea_queue_headredraw(sa);
				}
				break;
			case REDRAWBUTSHEAD:
				if(sa->spacetype==SPACE_BUTS) {
					scrarea_queue_headredraw(sa);
				}
				break;
			case REDRAWBUTSSCENE:
				if(sa->spacetype==SPACE_BUTS) {
					buts= sa->spacedata.first;
					if(buts->mainb==CONTEXT_SCENE) {
						buts->re_align= 1;
						scrarea_queue_winredraw(sa);
						scrarea_queue_headredraw(sa);
					}
				}
				break;
			case REDRAWBUTSOBJECT:
				if(sa->spacetype==SPACE_BUTS) {
					buts= sa->spacedata.first;
					if(buts->mainb==CONTEXT_OBJECT) {
						buts->re_align= 1;
						scrarea_queue_winredraw(sa);
						scrarea_queue_headredraw(sa);
					}
				}
				break;
			case REDRAWBUTSSHADING:
				if(sa->spacetype==SPACE_BUTS) {
					buts= sa->spacedata.first;
					if(buts->mainb==CONTEXT_SHADING) {
						buts->re_align= 1;
						scrarea_queue_winredraw(sa);
						scrarea_queue_headredraw(sa);
					}
				}
				break;
			case REDRAWBUTSEDIT:
				if(sa->spacetype==SPACE_BUTS) {
					buts= sa->spacedata.first;
					if(buts->mainb==CONTEXT_EDITING) {
						buts->re_align= 1;
						scrarea_queue_winredraw(sa);
						scrarea_queue_headredraw(sa);
					}
				}
				break;
			case REDRAWBUTSSCRIPT:
				if(sa->spacetype==SPACE_BUTS) {
					buts= sa->spacedata.first;
					if(buts->mainb==CONTEXT_SCRIPT) {
						buts->re_align= 1;
						scrarea_queue_winredraw(sa);
						scrarea_queue_headredraw(sa);
					}
				}
				break;
			case REDRAWBUTSLOGIC:
				if(sa->spacetype==SPACE_BUTS) {
					buts= sa->spacedata.first;
					if(buts->mainb==CONTEXT_LOGIC) {
						scrarea_queue_winredraw(sa);
						scrarea_queue_headredraw(sa);
					}
				}
				break;
				
			case REDRAWDATASELECT:
				if(sa->spacetype==SPACE_FILE) {
					sfile= sa->spacedata.first;
					if(sfile->type==FILE_MAIN) {
						freefilelist(sfile);
						scrarea_queue_winredraw(sa);
					}
				}
				else if(sa->spacetype==SPACE_OOPS) {
					scrarea_queue_winredraw(sa);
				}
				break;
			case REDRAWSEQ:
				if(sa->spacetype==SPACE_SEQ) {
					addqueue(sa->win, CHANGED, 1);
					scrarea_queue_winredraw(sa);
					scrarea_queue_headredraw(sa);
				}
				break;
			case REDRAWOOPS:
				if(sa->spacetype==SPACE_OOPS) {
					scrarea_queue_winredraw(sa);
				}
				break;
			case REDRAWNLA:
				if(sa->spacetype==SPACE_NLA) {
					scrarea_queue_headredraw(sa);
					scrarea_queue_winredraw(sa);
				}
			case REDRAWACTION:
				if(sa->spacetype==SPACE_ACTION) {
					scrarea_queue_headredraw(sa);
					scrarea_queue_winredraw(sa);
				}
				break;
			case REDRAWTEXT:
				if(sa->spacetype==SPACE_TEXT) {
					scrarea_queue_winredraw(sa);
				}
				break;
			case REDRAWSCRIPT:
				if (sa->spacetype==SPACE_SCRIPT) {
					scrarea_queue_winredraw(sa);
				}
				break;
			case REDRAWSOUND:
				if(sa->spacetype==SPACE_SOUND) {
					scrarea_queue_headredraw(sa);
					scrarea_queue_winredraw(sa);
				}
				break;
			case REDRAWTIME:
				if(sa->spacetype==SPACE_TIME) {
					scrarea_queue_headredraw(sa);
					scrarea_queue_winredraw(sa);
				}
				break;
			case REDRAWNODE:
				if(sa->spacetype==SPACE_NODE) {
					scrarea_queue_headredraw(sa);
					scrarea_queue_winredraw(sa);
				}
				break;
			case RECALC_COMPOSITE:
				if(sa->spacetype==SPACE_NODE) {
					addqueue(sa->win, UI_BUT_EVENT, B_NODE_TREE_EXEC);
				}
				break;
			case REDRAWANIM:
				if ELEM6(sa->spacetype, SPACE_IPO, SPACE_SOUND, SPACE_TIME, SPACE_NLA, SPACE_ACTION, SPACE_SEQ) {
					scrarea_queue_winredraw(sa);
					if(val) scrarea_queue_headredraw(sa);
				}
			}
		}
		sa= sa->next;
	}
}

void allspace(unsigned short event, short val)
{
	bScreen *sc;

	sc= G.main->screen.first;
	while(sc) {
		ScrArea *sa= sc->areabase.first;
		while(sa) {
			SpaceLink *sl= sa->spacedata.first;
			while(sl) {
				switch(event) {
				case REMAKEIPO:
					if(sl->spacetype==SPACE_IPO) {
						SpaceIpo *si= (SpaceIpo *)sl;
						{
							if(si->editipo) MEM_freeN(si->editipo);
							si->editipo= 0;
							free_ipokey(&si->ipokey);
						}
					}
					break;
										
				case OOPS_TEST:
					if(sl->spacetype==SPACE_OOPS) {
						SpaceOops *so= (SpaceOops *)sl;
						so->flag |= SO_TESTBLOCKS;
					}
					break;
				}

				sl= sl->next;
			}
			sa= sa->next;
		}
		sc= sc->id.next;
	}
}

/* if header==1, then draw header for curarea too. Excepption for headerprint()... */
void force_draw(int header)
{
	/* draws all areas that show something identical to curarea */
	extern int afterqtest(short win, unsigned short evt);	//editscreen.c
	ScrArea *tempsa, *sa;

	scrarea_do_windraw(curarea);
	if(header) scrarea_do_headdraw(curarea);
		
	tempsa= curarea;
	sa= G.curscreen->areabase.first;
	while(sa) {
		if(sa!=tempsa && sa->spacetype==tempsa->spacetype) {
			areawinset(sa->win);
			scrarea_do_windraw(sa);
			scrarea_do_headdraw(sa);
		}
		sa= sa->next;
	}
	
	screen_swapbuffers();

#ifndef __APPLE__
	if(tempsa->spacetype==SPACE_VIEW3D) {
		/* de the afterqueuetest for backbuf draw */
		sa= G.curscreen->areabase.first;
		while(sa) {
			if(sa->spacetype==SPACE_VIEW3D) {
				if(afterqtest(sa->win, BACKBUFDRAW)) {
					areawinset(sa->win);
					backdrawview3d(0);
				}
			}
			sa= sa->next;
		}
	}
#endif
	if(curarea!=tempsa) areawinset(tempsa->win);
	
}

/* if header==1, then draw header for curarea too. Exception for headerprint()... */
void force_draw_plus(int type, int header)
{
	/* draws all areas that show something like curarea AND areas of 'type' */
	ScrArea *tempsa, *sa;

	scrarea_do_windraw(curarea); 
	if(header) scrarea_do_headdraw(curarea);

	tempsa= curarea;
	sa= G.curscreen->areabase.first;
	while(sa) {
		if(sa!=tempsa && (sa->spacetype==tempsa->spacetype || sa->spacetype==type)) {
			if(ELEM5(sa->spacetype, SPACE_VIEW3D, SPACE_IPO, SPACE_SEQ, SPACE_BUTS, SPACE_ACTION)) {
				areawinset(sa->win);
				scrarea_do_windraw(sa);
				scrarea_do_headdraw(sa);
			}
		}
		sa= sa->next;
	}
	if(curarea!=tempsa) areawinset(tempsa->win);

	screen_swapbuffers();
}

/* if header==1, then draw header for curarea too. Excepption for headerprint()... */
void force_draw_all(int header)
{
	/* redraws all */
	ScrArea *tempsa, *sa;

	tempsa= curarea;
	sa= G.curscreen->areabase.first;
	while(sa) {
		if(sa->headwin) {
			scrarea_do_headdraw(sa);
			if(sa!=curarea || header) scrarea_do_headchange(sa);
		}
		if(sa->win) {
			scrarea_do_windraw(sa);
		}
		sa= sa->next;
	}
	if(curarea!=tempsa) areawinset(tempsa->win);

	screen_swapbuffers();
}

/***/

SpaceType *spaceaction_get_type(void)
{
	static SpaceType *st= NULL;
	
	if (!st) {
		st= spacetype_new("Action");
		spacetype_set_winfuncs(st, drawactionspace, changeactionspace, winqreadactionspace);
	}

	return st;
}
SpaceType *spacebuts_get_type(void)
{
	static SpaceType *st= NULL;
	
	if (!st) {
		st= spacetype_new("Buts");
		spacetype_set_winfuncs(st, drawbutspace, changebutspace, winqreadbutspace);
	}

	return st;
}
SpaceType *spacefile_get_type(void)
{
	static SpaceType *st= NULL;
	
	if (!st) {
		st= spacetype_new("File");
		spacetype_set_winfuncs(st, drawfilespace, NULL, winqreadfilespace);
	}

	return st;
}
SpaceType *spaceimage_get_type(void)
{
	static SpaceType *st= NULL;
	
	if (!st) {
		st= spacetype_new("Image");
		spacetype_set_winfuncs(st, drawimagespace, changeimagepace, winqreadimagespace);
	}

	return st;
}
SpaceType *spaceimasel_get_type(void)
{
	static SpaceType *st= NULL;
	
	if (!st) {
		st= spacetype_new("Imasel");
		spacetype_set_winfuncs(st, drawimaselspace, NULL, winqreadimaselspace);
	}

	return st;
}
SpaceType *spaceinfo_get_type(void)
{
	static SpaceType *st= NULL;
	
	if (!st) {
		st= spacetype_new("Info");
		spacetype_set_winfuncs(st, drawinfospace, NULL, winqreadinfospace);
	}

	return st;
}
SpaceType *spaceipo_get_type(void)
{
	static SpaceType *st= NULL;
	
	if (!st) {
		st= spacetype_new("Ipo");
		spacetype_set_winfuncs(st, drawipospace, changeview2dspace, winqreadipospace);
	}

	return st;
}
SpaceType *spacenla_get_type(void)
{
	static SpaceType *st= NULL;
	
	if (!st) {
		st= spacetype_new("Nla");
		spacetype_set_winfuncs(st, drawnlaspace, changeview2dspace, winqreadnlaspace);
	}

	return st;
}
SpaceType *spaceoops_get_type(void)
{
	static SpaceType *st= NULL;
	
	if (!st) {
		st= spacetype_new("Oops");
		spacetype_set_winfuncs(st, drawoopsspace, changeview2dspace, winqreadoopsspace);
	}

	return st;
}
SpaceType *spaceseq_get_type(void)
{
	static SpaceType *st= NULL;
	
	if (!st) {
		st= spacetype_new("Sequence");
		spacetype_set_winfuncs(st, drawseqspace, changeview2dspace, winqreadseqspace);
	}

	return st;
}
SpaceType *spacesound_get_type(void)
{
	static SpaceType *st= NULL;
	
	if (!st) {
		st= spacetype_new("Sound");
		spacetype_set_winfuncs(st, drawsoundspace, changeview2dspace, winqreadsoundspace);
	}

	return st;
}
SpaceType *spacetext_get_type(void)
{
	static SpaceType *st= NULL;
	
	if (!st) {
		st= spacetype_new("Text");
		spacetype_set_winfuncs(st, drawtextspace, NULL, winqreadtextspace);
	}

	return st;
}
SpaceType *spacescript_get_type(void)
{
	static SpaceType *st = NULL;

	if (!st) {
		st = spacetype_new("Script");
		spacetype_set_winfuncs(st, drawscriptspace, NULL, winqreadscriptspace);
	}

	return st;
}
SpaceType *spaceview3d_get_type(void)
{
	static SpaceType *st= NULL;
	
	if (!st) {
		st= spacetype_new("View3D");
		spacetype_set_winfuncs(st, drawview3dspace, changeview3dspace, winqreadview3dspace);
	}

	return st;
}
SpaceType *spacetime_get_type(void)
{
	static SpaceType *st= NULL;
	
	if (!st) {
		st= spacetype_new("Time");
		spacetype_set_winfuncs(st, drawtimespace, NULL, winqreadtimespace);
	}
	
	return st;
}

SpaceType *spacenode_get_type(void)
{
	static SpaceType *st= NULL;
	
	if (!st) {
		st= spacetype_new("Node");
		spacetype_set_winfuncs(st, drawnodespace, changeview2dspace, winqreadnodespace);
	}
	
	return st;
}
