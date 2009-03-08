/**
 * $Id:
 *
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
 * The Original Code is Copyright (C) 2009 Blender Foundation.
 * All rights reserved.
 *
 * 
 * Contributor(s): Blender Foundation, Joshua Leung
 *
 * ***** END GPL LICENSE BLOCK *****
 */

#include <string.h>
#include <stdio.h>
#include <math.h>
#include <float.h>

#include "DNA_anim_types.h"
#include "DNA_action_types.h"
#include "DNA_object_types.h"
#include "DNA_space_types.h"
#include "DNA_scene_types.h"
#include "DNA_screen_types.h"
#include "DNA_userdef_types.h"

#include "MEM_guardedalloc.h"

#include "BLI_arithb.h"
#include "BLI_blenlib.h"
#include "BLI_editVert.h"
#include "BLI_rand.h"

#include "BKE_animsys.h"
#include "BKE_action.h"
#include "BKE_context.h"
#include "BKE_curve.h"
#include "BKE_customdata.h"
#include "BKE_depsgraph.h"
#include "BKE_object.h"
#include "BKE_global.h"
#include "BKE_scene.h"
#include "BKE_screen.h"
#include "BKE_utildefines.h"

#include "BIF_gl.h"

#include "WM_api.h"
#include "WM_types.h"

#include "RNA_access.h"
#include "RNA_define.h"

#include "ED_anim_api.h"
#include "ED_keyframing.h"
#include "ED_screen.h"
#include "ED_types.h"
#include "ED_util.h"

#include "UI_interface.h"
#include "UI_resources.h"
#include "UI_view2d.h"

#include "graph_intern.h"	// own include


/* ******************* graph editor space & buttons ************** */

#define B_NOP		1
#define B_REDR		2

/* -------------- */

static void do_graph_region_buttons(bContext *C, void *arg, int event)
{
	//Scene *scene= CTX_data_scene(C);
	
	switch(event) {

	}
	
	/* default for now */
	//WM_event_add_notifier(C, NC_OBJECT|ND_TRANSFORM, ob);
}

static void graph_panel_properties(const bContext *C, ARegion *ar, short cntrl, bAnimListElem *ale)	
{
	FCurve *fcu= (FCurve *)ale->data;
	uiBlock *block;
	char name[128];

	block= uiBeginBlock(C, ar, "graph_panel_properties", UI_EMBOSS, UI_HELV);
	if (uiNewPanel(C, ar, block, "Properties", "Graph", 340, 30, 318, 254)==0) return;
	uiBlockSetHandleFunc(block, do_graph_region_buttons, NULL);

	/* to force height */
	uiNewPanelHeight(block, 204);

	/* Info - Active F-Curve */
	uiDefBut(block, LABEL, 1, "Active F-Curve:",					10, 200, 150, 19, NULL, 0.0, 0.0, 0, 0, "");
	
	if (ale->id) { 
		// icon of active blocktype - is this really necessary?
		int icon= geticon_anim_blocktype(GS(ale->id->name));
		
		// xxx type of icon-but is currently "LABEL", as that one is plain...
		uiDefIconBut(block, LABEL, 1, icon, 10, 180, 20, 19, NULL, 0, 0, 0, 0, "ID-type that F-Curve belongs to");
	}
	
	getname_anim_fcurve(name, ale->id, fcu);
	uiDefBut(block, LABEL, 1, name,	30, 180, 300, 19, NULL, 0.0, 0.0, 0, 0, "Name of Active F-Curve");
	
	/* TODO: the following settings could be added here
	 *	- F-Curve coloring mode - mode selector + color selector
	 *	- Access details (ID-block + RNA-Path + Array Index)
	 *	- ...
	 */
}

/* -------------- */

#define B_IPO_DEPCHANGE 	10

static void do_graph_region_driver_buttons(bContext *C, void *arg, int event)
{
	Scene *scene= CTX_data_scene(C);
	
	switch(event) {
		case B_IPO_DEPCHANGE:
		{
			/* rebuild depsgraph for the new deps */
			DAG_scene_sort(scene);
			
			/* TODO: which one? we need some way of sending these updates since curves from non-active ob could be being edited */
			//DAG_object_flush_update(G.scene, ob, OB_RECALC_DATA);
			//DAG_object_flush_update(scene, ob, OB_RECALC_OB);
		}
			break;
	}
	
	/* default for now */
	//WM_event_add_notifier(C, NC_OBJECT|ND_TRANSFORM, ob);
}

static void graph_panel_drivers(const bContext *C, ARegion *ar, short cntrl, bAnimListElem *ale)	
{
	FCurve *fcu= (FCurve *)ale->data;
	ChannelDriver *driver= fcu->driver;
	uiBlock *block;
	uiBut *but;
	int len;

	block= uiBeginBlock(C, ar, "graph_panel_drivers", UI_EMBOSS, UI_HELV);
	if (uiNewPanel(C, ar, block, "Drivers", "Graph", 340, 30, 318, 254)==0) return;
	uiBlockSetHandleFunc(block, do_graph_region_driver_buttons, NULL);

	/* to force height */
	uiNewPanelHeight(block, 204);
	
	/* type */
	uiDefBut(block, LABEL, 1, "Type:",					10, 200, 120, 20, NULL, 0.0, 0.0, 0, 0, "");
	uiDefButI(block, MENU, B_IPO_DEPCHANGE,
					"Driver Type%t|Transform Channel%x0|Scripted Expression%x1|Rotational Difference%x2", 
					130,200,180,20, &driver->type, 0, 0, 0, 0, "Driver type");
					
	/* buttons to draw depends on type of driver */
	if (driver->type == DRIVER_TYPE_PYTHON) { /* PyDriver */
		uiDefBut(block, TEX, B_REDR, "Expr: ", 10,160,300,20, driver->expression, 0, 255, 0, 0, "One-liner Python Expression to use as Scripted Expression");
		
		if (driver->flag & DRIVER_FLAG_INVALID) {
			uiDefIconBut(block, LABEL, 1, ICON_ERROR, 10, 140, 20, 19, NULL, 0, 0, 0, 0, "");
			uiDefBut(block, LABEL, 0, "Error: invalid Python expression",
					30,140,230,19, NULL, 0, 0, 0, 0, "");
		}
	}
	else { /* Channel or RotDiff - RotDiff just has extra settings */
		/* Driver Object */
		but= uiDefBut(block, TEX, B_IPO_DEPCHANGE, "OB: ",	10,160,150,20, driver->id->name+2, 0.0, 21.0, 0, 0, "Object that controls this Driver.");
		uiButSetFunc(but, test_idbutton_cb, driver->id->name, NULL);
		
		// XXX should we hide these technical details?
		if (driver->id) {
			/* Array Index */
			// XXX ideally this is grouped with the path, but that can get quite long...
			uiDefButI(block, NUM, B_IPO_DEPCHANGE, "Index: ", 170,160,140,20, &driver->array_index, 0, INT_MAX, 0, 0, "Index to the specific property used as Driver if applicable.");
			
			/* RNA-Path - allocate if non-existant */
			if (driver->rna_path == NULL) {
				driver->rna_path= MEM_callocN(256, "Driver RNA-Path");
				len= 255;
			}
			else
				len= strlen(driver->rna_path);
			uiDefBut(block, TEX, B_IPO_DEPCHANGE, "Path: ", 10,130,300,20, driver->rna_path, 0, len, 0, 0, "RNA Path (from Driver Object) to property used as Driver.");
		}
		
		/* for rotational difference, show second target... */
		if (driver->type == DRIVER_TYPE_ROTDIFF) {
			// TODO...
		}
	}
}

/* -------------- */

static void do_graph_region_modifier_buttons(bContext *C, void *arg, int event)
{
	//Scene *scene= CTX_data_scene(C);
	
	switch(event) {

	}
	
	/* default for now */
	//WM_event_add_notifier(C, NC_OBJECT|ND_TRANSFORM, ob);
}

static void graph_panel_modifiers(const bContext *C, ARegion *ar, short cntrl, bAnimListElem *ale)	
{
	//FCurve *fcu= (FCurve *)ale->data;
	//FModifier *fcm;
	uiBlock *block;

	block= uiBeginBlock(C, ar, "graph_panel_modifiers", UI_EMBOSS, UI_HELV);
	if (uiNewPanel(C, ar, block, "Modifiers", "Graph", 340, 30, 318, 254)==0) return;
	uiBlockSetHandleFunc(block, do_graph_region_modifier_buttons, NULL);

	/* to force height */
	uiNewPanelHeight(block, 204); // XXX variable height!
	
	
}

/* -------------- */

/* Find 'active' F-Curve. It must be editable, since that's the purpose of these buttons (subject to change).  
 * We return the 'wrapper' since it contains valuable context info (about hierarchy), which will need to be freed 
 * when the caller is done with it.
 */
// TODO: move this to anim api with another name?
static bAnimListElem *get_active_fcurve_channel (bAnimContext *ac)
{
	ListBase anim_data = {NULL, NULL};
	int filter= (ANIMFILTER_VISIBLE | ANIMFILTER_FOREDIT | ANIMFILTER_ACTIVE | ANIMFILTER_CURVESONLY);
	int items = ANIM_animdata_filter(ac, &anim_data, filter, ac->data, ac->datatype);
	
	/* We take the first F-Curve only, since some other ones may have had 'active' flag set
	 * if they were from linked data.
	 */
	if (items) {
		bAnimListElem *ale= (bAnimListElem *)anim_data.first;
		
		/* remove first item from list, then free the rest of the list and return the stored one */
		BLI_remlink(&anim_data, ale);
		BLI_freelistN(&anim_data);
		
		return ale;
	}
	
	/* no active F-Curve */
	return NULL;
}

void graph_region_buttons(const bContext *C, ARegion *ar)
{
	SpaceIpo *sipo= (SpaceIpo *)CTX_wm_space_data(C);
	bAnimContext ac;
	bAnimListElem *ale= NULL;
	
	/* for now, only draw if we could init the anim-context info (necessary for all animation-related tools) 
	 * to work correctly is able to be correctly retrieved. There's no point showing empty panels?
	 */
	if (ANIM_animdata_get_context(C, &ac) == 0) 
		return;
	
	
	/* try to find 'active' F-Curve */
	ale= get_active_fcurve_channel(&ac);
	if (ale == NULL) 
		return;	
		
	/* for now, the properties panel displays info about the selected channels */
	graph_panel_properties(C, ar, 0, ale);
	
	/* driver settings for active F-Curve (only for 'Drivers' mode) */
	if (sipo->mode == SIPO_MODE_DRIVERS)
		graph_panel_drivers(C, ar, 0, ale);
	
	/* modifiers */
	graph_panel_modifiers(C, ar, 0, ale);
	

	uiDrawPanels(C, 1);		/* 1 = align */
	uiMatchPanelsView2d(ar); /* sets v2d->totrct */
	
	/* free temp data */
	MEM_freeN(ale);
}


static int graph_properties(bContext *C, wmOperator *op)
{
	ScrArea *sa= CTX_wm_area(C);
	ARegion *ar= graph_has_buttons_region(sa);
	
	if(ar) {
		ar->flag ^= RGN_FLAG_HIDDEN;
		ar->v2d.flag &= ~V2D_IS_INITIALISED; /* XXX should become hide/unhide api? */
		
		ED_area_initialize(CTX_wm_manager(C), CTX_wm_window(C), sa);
		ED_area_tag_redraw(sa);
	}
	return OPERATOR_FINISHED;
}

void GRAPHEDIT_OT_properties(wmOperatorType *ot)
{
	ot->name= "Properties";
	ot->idname= "GRAPHEDIT_OT_properties";
	
	ot->exec= graph_properties;
	ot->poll= ED_operator_ipo_active; // xxx
 	
	/* flags */
	ot->flag= 0;
}
