/**
 * $Id: 
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

#include <time.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef WIN32
#include "BLI_winstuff.h"
#endif

#include "MEM_guardedalloc.h"
#include "DNA_screen_types.h"
#include "DNA_space_types.h"
#include "DNA_scene_types.h"
#include "DNA_curve_types.h"
#include "DNA_material_types.h"
#include "DNA_texture_types.h"
#include "DNA_object_types.h"


#include "BKE_global.h"
#include "BKE_main.h"
#include "BKE_library.h"
#include "BKE_utildefines.h"
#include "BKE_material.h"
#include "BKE_texture.h"

#include "BLI_blenlib.h"

#include "BSE_filesel.h"

#include "BIF_gl.h"
#include "BIF_graphics.h"
#include "BIF_keyval.h"
#include "BIF_mainqueue.h"
#include "BIF_resources.h"
#include "BIF_screen.h"
#include "BIF_mywindow.h"
#include "BIF_space.h"
#include "BIF_glutil.h"
#include "BIF_interface.h"
#include "BIF_toolbox.h"
#include "BIF_space.h"
#include "BIF_previewrender.h"
#include "BIF_butspace.h"

#include "interface.h"
#include "mydevice.h"
#include "blendef.h"

/* -----includes for this file specific----- */

#include "butspace.h" // own module

static MTex mtexcopybuf;
static MTex emptytex;

void do_matbuts(unsigned short event)
{
	static short mtexcopied=0;
	Material *ma;
	MTex *mtex;

	switch(event) {		
	case B_ACTCOL:
		scrarea_queue_headredraw(curarea);
		allqueue(REDRAWBUTSMAT, 0);
		allqueue(REDRAWIPO, 0);
		BIF_preview_changed(G.buts);
		break;
	case B_MATFROM:
		scrarea_queue_headredraw(curarea);
		allqueue(REDRAWBUTSMAT, 0);
		// BIF_previewdraw();  push/pop!
		break;
	case B_MATPRV:
		/* this event also used by lamp, tex and sky */
		BIF_preview_changed(G.buts);
		break;
	case B_MATPRV_DRAW:
		BIF_preview_changed(G.buts);
		allqueue(REDRAWBUTSMAT, 0);
		break;
	case B_TEXCLEAR:
		ma= G.buts->lockpoin;
		mtex= ma->mtex[(int) ma->texact ];
		if(mtex) {
			if(mtex->tex) mtex->tex->id.us--;
			MEM_freeN(mtex);
			ma->mtex[ (int) ma->texact ]= 0;
			allqueue(REDRAWBUTSMAT, 0);
			allqueue(REDRAWOOPS, 0);
			BIF_preview_changed(G.buts);
		}
		break;
	case B_MTEXCOPY:
		ma= G.buts->lockpoin;
		if(ma && ma->mtex[(int)ma->texact] ) {
			mtex= ma->mtex[(int)ma->texact];
			if(mtex->tex==0) {
				error("No texture available");
			}
			else {
				memcpy(&mtexcopybuf, ma->mtex[(int)ma->texact], sizeof(MTex));
				notice("copied!");
				mtexcopied= 1;
			}
		}
		break;
	case B_MTEXPASTE:
		ma= G.buts->lockpoin;
		if(ma && mtexcopied && mtexcopybuf.tex) {
			if(ma->mtex[(int)ma->texact]==0 ) ma->mtex[(int)ma->texact]= MEM_mallocN(sizeof(MTex), "mtex"); 
			memcpy(ma->mtex[(int)ma->texact], &mtexcopybuf, sizeof(MTex));
			
			id_us_plus((ID *)mtexcopybuf.tex);
			notice("pasted!");
			BIF_preview_changed(G.buts);
			scrarea_queue_winredraw(curarea);
		}
		break;
	case B_MATLAY:
		ma= G.buts->lockpoin;
		if(ma && ma->lay==0) {
			ma->lay= 1;
			scrarea_queue_winredraw(curarea);
		}
	}
}

void matbuts(void)
{
	Object *ob;
	Material *ma;
	ID *id, *idn;
	MTex *mtex;
	uiBlock *block;
	uiBut *but;
	float *colpoin = NULL, min;
	int rgbsel = 0, a, loos;
	char str[30], *strp;
	short xco;
	
	ob= OBACT;
	if(ob==0 || ob->data==0) return;

	sprintf(str, "buttonswin %d", curarea->win);
	block= uiNewBlock(&curarea->uiblocks, str, UI_EMBOSSX, UI_HELV, curarea->win);

	if(ob->actcol==0) ob->actcol= 1;	/* because of TOG|BIT button */
	
	/* indicate which one is linking a material */
	uiBlockSetCol(block, BUTSALMON);
	uiDefButS(block, TOG|BIT|(ob->actcol-1), B_MATFROM, "OB",	342, 195, 33, 20, &ob->colbits, 0, 0, 0, 0, "Link material to object");
	idn= ob->data;
	strncpy(str, idn->name, 2);
	str[2]= 0;
	uiBlockSetCol(block, BUTGREEN);
	uiDefButS(block, TOGN|BIT|(ob->actcol-1), B_MATFROM, str,		380, 195, 33, 20, &ob->colbits, 0, 0, 0, 0, "Show the block the material is linked to");
	uiBlockSetCol(block, BUTGREY);
	
	/* id is the block from which the material is used */
	if( BTST(ob->colbits, ob->actcol-1) ) id= (ID *)ob;
	else id= ob->data;

	sprintf(str, "%d Mat", ob->totcol);
	if(ob->totcol) min= 1.0; else min= 0.0;
	uiDefButC(block, NUM, B_ACTCOL, str,	415,195,150,20, &(ob->actcol), min, (float)ob->totcol, 0, 0, "Number of materials on object / Active material");
	
	uiSetButLock(id->lib!=0, "Can't edit library data");
	
	strncpy(str, id->name, 2);
	str[2]= ':'; str[3]= 0;
	but= uiDefBut(block, TEX, B_IDNAME, str,		200,195,140,20, id->name+2, 0.0, 18.0, 0, 0, "Show the block the material is linked to");
	uiButSetFunc(but, test_idbutton_cb, id->name, NULL);

	if(ob->totcol==0) {
		uiDrawBlock(block);
		return;
	}
	
	ma= give_current_material(ob, ob->actcol);
	
	if(ma==0) {
		uiDrawBlock(block);
		return;
	}
	uiSetButLock(ma->id.lib!=0, "Can't edit library data");
	
	uiBlockSetCol(block, BUTGREY);
	uiDefButS(block, ROW, REDRAWBUTSMAT, "RGB",			200,166,35,22, &(ma->colormodel), 1.0, (float)MA_RGB, 0, 0, "Create colour by red, green and blue");
	uiDefButS(block, ROW, REDRAWBUTSMAT, "HSV",			200,143,35,22, &(ma->colormodel), 1.0, (float)MA_HSV, 0, 0, "Mix colour with hue, saturation and value");
	uiDefButS(block, TOG|BIT|0, REDRAWBUTSMAT, "DYN",	200,120,35,22, &(ma->dynamode), 0.0, 0.0, 0, 0, "Adjust parameters for dynamics options");
	
	if(ma->dynamode & MA_DRAW_DYNABUTS) {
		uiDefButF(block, NUMSLI, 0, "Restitut ",		390,168,175,21, &ma->reflect, 0.0, 1.0, 0, 0, "Elasticity of collisions");
		uiDefButF(block, NUMSLI, 0, "Friction ",  		390,144,175,21, &ma->friction, 0.0, 100.0, 0, 0,   "Coulomb friction coefficient");

		uiDefButF(block, NUMSLI, 0, "Fh Force ",		390,120,175,21, &ma->fh, 0.0, 1.0, 0, 0, "Upward spring force within the Fh area");
		uiDefButF(block, NUM, 0,	 "Fh Damp ",		260,144,120,21, &ma->xyfrict, 0.0, 1.0, 10, 0, "Damping of the Fh spring force");
		uiDefButF(block, NUM, 0, "Fh Dist ",			260,120,120,21, &ma->fhdist, 0.0, 20.0, 10, 0, "Height of the Fh area");
		uiBlockSetCol(block, BUTGREEN);
		uiDefButS(block, TOG|BIT|1, 0, "Fh Norm",		260,168,120,21, &ma->dynamode, 0.0, 0.0, 0, 0, "Add a horizontal spring force on slopes");
		uiBlockSetCol(block, BUTGREY);
	}
	else {
		uiDefButF(block, COL, B_MIRCOL, "",		235,143,30,45, &(ma->mirr), 0, 0, 0, 0, "");
		uiDefButF(block, COL, B_SPECCOL, "",	265,143,39,45, &(ma->specr), 0, 0, 0, 0, "");
		uiDefButF(block, COL, B_MATCOL, "",		304,143,39,45, &(ma->r), 0, 0, 0, 0, "");
	
		if(ma->mode & MA_HALO) {
			uiDefButC(block, ROW, REDRAWBUTSMAT, "Ring",		235,120,30,22, &(ma->rgbsel), 2.0, 2.0, 0, 0, "Mix the colour of the rings with the RGB sliders");
			uiDefButC(block, ROW, REDRAWBUTSMAT, "Line",		265,120,39,22, &(ma->rgbsel), 2.0, 1.0, 0, 0, "Mix the colour of the lines with the RGB sliders");
			uiDefButC(block, ROW, REDRAWBUTSMAT, "Halo",		304,120,39,22, &(ma->rgbsel), 2.0, 0.0, 0, 0, "Mix the colour of the halo with the RGB sliders");
		}
		else {
			uiDefButC(block, ROW, REDRAWBUTSMAT, "Mir",			235,120,30,22, &(ma->rgbsel), 2.0, 2.0, 0, 0, "Use mirror colour");
			uiDefButC(block, ROW, REDRAWBUTSMAT, "Spe",			265,120,39,22, &(ma->rgbsel), 2.0, 1.0, 0, 0, "Set the colour of the specularity");
			uiDefButC(block, ROW, REDRAWBUTSMAT, "Col",			304,120,39,22, &(ma->rgbsel), 2.0, 0.0, 0, 0, "Set the basic colour of the material");
		}
		if(ma->rgbsel==0) {colpoin= &(ma->r); rgbsel= B_MATCOL;}
		else if(ma->rgbsel==1) {colpoin= &(ma->specr); rgbsel= B_SPECCOL;}
		else if(ma->rgbsel==2) {colpoin= &(ma->mirr); rgbsel= B_MIRCOL;}
		
		if(ma->rgbsel==0 && (ma->mode & (MA_VERTEXCOLP|MA_FACETEXTURE) && !(ma->mode & MA_HALO)));
		else if(ma->colormodel==MA_HSV) {
			uiBlockSetCol(block, BUTPURPLE);
			uiDefButF(block, HSVSLI, B_MATPRV, "H ",			350,168,150,21, colpoin, 0.0, 0.9999, rgbsel, 0, "");
			uiBlockSetCol(block, BUTPURPLE);
			uiDefButF(block, HSVSLI, B_MATPRV, "S ",			350,144,150,21, colpoin, 0.0001, 1.0, rgbsel, 0, "");
			uiBlockSetCol(block, BUTPURPLE);
			uiDefButF(block, HSVSLI, B_MATPRV, "V ",			350,120,150,21, colpoin, 0.0001, 1.0, rgbsel, 0, "");
			uiBlockSetCol(block, BUTGREY);
		}
		else {
			uiDefButF(block, NUMSLI, B_MATPRV, "R ",			350,168,150,21, colpoin, 0.0, 1.0, rgbsel, 0, "");
			uiDefButF(block, NUMSLI, B_MATPRV, "G ",			350,144,150,21, colpoin+1, 0.0, 1.0, rgbsel, 0, "");
			uiDefButF(block, NUMSLI, B_MATPRV, "B ",			350,120,150,21, colpoin+2, 0.0, 1.0, rgbsel, 0, "");
		}
		if(!(ma->mode & MA_HALO)) {
			uiBlockSetCol(block, BUTBLUE);
			uiDefButI(block, TOG|BIT|4, B_REDR,	"VCol Light",		505, 168, 60, 21, &(ma->mode), 0, 0, 0, 0, "Add vertex colours as extra light");
			uiDefButI(block, TOG|BIT|7, B_REDR, "VCol Paint",		505, 144, 60, 21, &(ma->mode), 0, 0, 0, 0, "Replace basic colours with vertex colours");
			uiDefButI(block, TOG|BIT|11, B_REDR, "TexFace",			505, 120, 60, 21, &(ma->mode), 0, 0, 0, 0, "UV-Editor assigned texture gives color and texture info for the faces");
		}
	}
	if(ma->mode & MA_HALO) {
		uiBlockSetCol(block, BUTGREY);
		uiDefButF(block, NUM, B_MATPRV, "HaloSize: ",		200,90,175,18, &(ma->hasize), 0.0, 100.0, 10, 0, "Set the dimension of the halo");
		uiDefButF(block, NUMSLI, B_MATPRV, "Alpha ",		200,50,175,18, &(ma->alpha), 0.0, 1.0, 0, 0, "Set the degree of coverage");
		uiDefButS(block, NUMSLI, B_MATPRV, "Hard ",			200,30,175,18, &(ma->har), 1.0, 127.0, 0, 0, "Set the hardness of the halo");
		uiDefButF(block, NUMSLI, B_MATPRV, "Add  ",			200,10,175,18, &(ma->add), 0.0, 1.0, 0, 0, "Strength of the add effect");
		
		uiDefButS(block, NUM, B_MATPRV, "Rings: ",			380,90,85,18, &(ma->ringc), 0.0, 24.0, 0, 0, "Set the number of rings rendered over the basic halo");
		uiDefButS(block, NUM, B_MATPRV, "Lines: ",			465,90,90,18, &(ma->linec), 0.0, 250.0, 0, 0, "Set the number of star shaped lines rendered over the halo");
		uiDefButS(block, NUM, B_MATPRV, "Star: ",			380,70,85,18, &(ma->starc), 3.0, 50.0, 0, 0, "Set the number of points on the star shaped halo");
		uiDefButC(block, NUM, B_MATPRV, "Seed: ",			465,70,90,18, &(ma->seed1), 0.0, 255.0, 0, 0, "Use random values for ring dimension and line location");
		
		uiDefButF(block, NUM, B_MATPRV, "FlareSize: ",		380,50,85,18, &(ma->flaresize), 0.1, 25.0, 10, 0, "Set the factor the flare is larger than the halo");
		uiDefButF(block, NUM, B_MATPRV, "Sub Size: ",		465,50,90,18, &(ma->subsize), 0.1, 25.0, 10, 0, "Set the dimension of the subflares, dots and circles");
		uiDefButF(block, NUM, B_MATPRV, "FlareBoost: ",		380,30,175,18, &(ma->flareboost), 0.1, 10.0, 10, 0, "Give the flare extra strength");
		uiDefButC(block, NUM, B_MATPRV, "Fl.seed: ",		380,10,85,18, &(ma->seed2), 0.0, 255.0, 0, 0, "Specify an offset in the seed table");
		uiDefButS(block, NUM, B_MATPRV, "Flares: ",			465,10,90,18, &(ma->flarec), 1.0, 32.0, 0, 0, "Set the nuber of subflares");

		uiBlockSetCol(block, BUTBLUE);
		
		uiDefButI(block, TOG|BIT|15, B_MATPRV, "Flare",		576, 181, 77, 36, &(ma->mode), 0, 0, 0, 0, "Render halo as a lensflare");
		uiDefButI(block, TOG|BIT|8, B_MATPRV, "Rings",		576, 143, 77, 18, &(ma->mode), 0, 0, 0, 0, "Render rings over basic halo");
		uiDefButI(block, TOG|BIT|9, B_MATPRV, "Lines",		576, 124, 77, 18, &(ma->mode), 0, 0, 0, 0, "Render star shaped lines over the basic halo");
		uiDefButI(block, TOG|BIT|11, B_MATPRV, "Star",		576, 105, 77, 18, &(ma->mode), 0, 0, 0, 0, "Render halo as a star");
		uiDefButI(block, TOG|BIT|5, B_MATPRV_DRAW, "Halo",	576, 86, 77, 18, &(ma->mode), 0, 0, 0, 0, "Render as a halo");
		
		uiDefButI(block, TOG|BIT|12, B_MATPRV, "HaloTex",	576, 67, 77, 18, &(ma->mode), 0, 0, 0, 0, "Give halo a texture");
		uiDefButI(block, TOG|BIT|13, B_MATPRV, "HaloPuno",	576, 48, 77, 18, &(ma->mode), 0, 0, 0, 0, "Use the vertex normal to specify the dimension of the halo");
		uiDefButI(block, TOG|BIT|10, B_MATPRV, "X Alpha",	576, 28, 77, 18, &(ma->mode), 0, 0, 0, 0, "Use extreme alpha");
		uiDefButI(block, TOG|BIT|14, B_MATPRV, "Shaded",	576, 10, 77, 18, &(ma->mode), 0, 0, 0, 0, "Let halo receive light");
	}
	else {
		char *str1= "Diffuse Shader%t|Lambert %x0|Oren-Nayar %x1|Toon %x2";
		char *str2= "Specular Shader%t|CookTorr %x0|Phong %x1|Blinn %x2|Toon %x3";
		
		/* shader buttons */
		uiBlockSetCol(block, BUTGREY);
		uiDefButS(block, MENU, B_MATPRV_DRAW, str1,		200,90,95,18, &(ma->diff_shader), 0.0, 0.0, 0, 0, "Set a diffuse shader");
		uiDefButS(block, MENU, B_MATPRV_DRAW, str2,		295,90,90,18, &(ma->spec_shader), 0.0, 0.0, 0, 0, "Set a specular shader");

		if(ma->diff_shader==MA_DIFF_ORENNAYAR)
			uiDefButF(block, NUM, B_MATPRV, "Rough:",		200, 70, 95,18, &(ma->roughness), 0.0, 3.14, 0, 0, "Oren Nayar Roughness");
		else if(ma->diff_shader==MA_DIFF_TOON) {
			uiDefButF(block, NUM, B_MATPRV, "Size:",		200, 70, 95,18, &(ma->param[0]), 0.0, 3.14, 0, 0, "Size of diffuse toon area");
			uiDefButF(block, NUM, B_MATPRV, "Smooth:",		200, 50, 95,18, &(ma->param[1]), 0.0, 1.0, 0, 0, "Smoothness of diffuse toon area");
		}
		
		if ELEM3(ma->spec_shader, MA_SPEC_COOKTORR, MA_SPEC_PHONG, MA_SPEC_BLINN) {
			uiDefButS(block, NUM, B_MATPRV, "Hard:",		295, 70, 90,18, &(ma->har), 1.0, 255, 0, 0, "Set the hardness of the specularity");
		}
		if(ma->spec_shader==MA_SPEC_BLINN)
			uiDefButF(block, NUM, B_MATPRV, "Refr:",		295, 50, 90,18, &(ma->refrac), 1.0, 10.0, 0, 0, "Refraction index");
		if(ma->spec_shader==MA_SPEC_TOON) {
			uiDefButF(block, NUM, B_MATPRV, "Size:",		295, 70, 90,18, &(ma->param[2]), 0.0, 1.53, 0, 0, "Size of specular toon area");
			uiDefButF(block, NUM, B_MATPRV, "Smooth:",		295, 50, 90,18, &(ma->param[3]), 0.0, 1.0, 0, 0, "Smoothness of specular toon area");
		}

		uiDefButF(block, NUMSLI, B_MATPRV, "Ref   ",		200,30,185,18, &(ma->ref), 0.0, 1.0, 0, 0, "Set the amount of reflection");
		uiDefButF(block, NUMSLI, B_MATPRV, "Spec ",			200,10,185,18, &(ma->spec), 0.0, 2.0, 0, 0, "Set the degree of specularity");
	
		/* default shading variables */
		uiDefButF(block, NUMSLI, B_MATPRV, "Alpha ",		390,90,175,18, &(ma->alpha), 0.0, 1.0, 0, 0, "Set the amount of coverage, to make materials transparent");
		uiDefButF(block, NUMSLI, B_MATPRV, "SpecTra ",		390,70,175,18, &(ma->spectra), 0.0, 1.0, 0, 0, "Make specular areas opaque");
		uiDefButF(block, NUMSLI, B_MATPRV, "Add  ",			390,50,175,18, &(ma->add), 0.0, 1.0, 0, 0, "Glow factor for transparant");
		uiDefButF(block, NUMSLI, B_MATPRV, "Emit  ",		390,30,175,18, &(ma->emit), 0.0, 1.0, 0, 0, "Set the amount of emitting light");
		uiDefButF(block, NUMSLI, B_MATPRV, "Amb   ",		390,10,175,18, &(ma->amb), 0.0, 1.0, 0, 0, "Set the amount of global ambient color");
	
		uiBlockSetCol(block, BUTBLUE);
	
		uiDefButI(block, TOG|BIT|0, 0,	"Traceable",		576,200,77,18, &(ma->mode), 0, 0, 0, 0, "Make material visible for shadow lamps");
		uiDefButI(block, TOG|BIT|1, 0,	"Shadow",			576,181,77,18, &(ma->mode), 0, 0, 0, 0, "Enable material for shadows");
		uiDefButI(block, TOG|BIT|16, 0,	"Radio",			576, 162, 77,18, &(ma->mode), 0, 0, 0, 0, "Set the material insensitive to mist");
		uiDefButI(block, TOG|BIT|2, B_MATPRV, "Shadeless",	576, 143, 77, 18, &(ma->mode), 0, 0, 0, 0, "Make material insensitive to light or shadow");
		uiDefButI(block, TOG|BIT|3, 0,	"Wire",				576, 124, 77, 18, &(ma->mode), 0, 0, 0, 0, "Render only the edges of faces");
		uiDefButI(block, TOG|BIT|6, 0,	"ZTransp",			576, 105, 77, 18, &(ma->mode), 0, 0, 0, 0, "Z-Buffer transparent faces");
		uiDefButI(block, TOG|BIT|5, B_MATPRV_DRAW, "Halo",	576, 86, 77, 18, &(ma->mode), 0, 0, 0, 0, "Render as a halo");
		uiDefButI(block, TOG|BIT|9, 0,	"Env",				576, 67, 77, 18, &(ma->mode), 0, 0, 0, 0, "Do not render material");
		uiDefButI(block, TOG|BIT|10, 0,	"OnlyShadow",		576, 48, 77, 18, &(ma->mode), 0, 0, 0, 0, "Let alpha be determined on the degree of shadow");
		uiDefButI(block, TOG|BIT|14, 0,	"No Mist",			576, 29, 77,18, &(ma->mode), 0, 0, 0, 0, "Set the material insensitive to mist");
		uiDefButI(block, TOG|BIT|8, 0,	"ZInvert",			576, 10, 77, 18, &(ma->mode), 0, 0, 0, 0, "Render with inverted Z Buffer");
		uiBlockSetCol(block, BUTGREY);
		uiDefButF(block, NUM, 0, "Zoffs:",					576, -9, 77,18, &(ma->zoffs), 0.0, 10.0, 0, 0, "Give face an artificial offset");
	}

	/* TEX CHANNELS */
	uiBlockSetCol(block, BUTGREY);
	xco= 665;
	for(a= 0; a<8; a++) {
		mtex= ma->mtex[a];
		if(mtex && mtex->tex) splitIDname(mtex->tex->id.name+2, str, &loos);
		else strcpy(str, "");
		str[10]= 0;
		uiDefButC(block, ROW, B_MATPRV_DRAW, str,	xco, 195, 63, 20, &(ma->texact), 3.0, (float)a, 0, 0, "");
		xco+= 65;
	}
	
	uiDefIconBut(block, BUT, B_MTEXCOPY, ICON_COPYUP,	(short)xco,195,20,21, 0, 0, 0, 0, 0, "Copy the material settings to the buffer");
	uiDefIconBut(block, BUT, B_MTEXPASTE, ICON_PASTEUP,	(short)(xco+20),195,20,21, 0, 0, 0, 0, 0, "Paste the material settings from the buffer");

	
	uiBlockSetCol(block, BUTGREEN);
	uiDefButC(block, TOG, B_MATPRV, "SepT", (short)(xco+40), 195, 40, 20, &(ma->septex), 0, 0, 0, 0, "Render only use active texture channel");
	uiBlockSetCol(block, BUTGREY);

	mtex= ma->mtex[ ma->texact ];
	if(mtex==0) {
		mtex= &emptytex;
		default_mtex(mtex);
	}
	
	/* TEXCO */
	uiBlockSetCol(block, BUTGREEN);
	uiDefButS(block, ROW, B_MATPRV, "Object",		694,166,49,18, &(mtex->texco), 4.0, (float)TEXCO_OBJECT, 0, 0, "Use linked object's coordinates for texture coordinates");
	uiDefIDPoinBut(block, test_obpoin_but, B_MATPRV, "",		745,166,133,18, &(mtex->object), "");
	uiDefButS(block, ROW, B_MATPRV, "UV",			664,166,29,18, &(mtex->texco), 4.0, (float)TEXCO_UV, 0, 0, "Use UV coordinates for texture coordinates");

	uiDefButS(block, ROW, B_MATPRV, "Glob",			665,146,35,18, &(mtex->texco), 4.0, (float)TEXCO_GLOB, 0, 0, "Use global coordinates for the texture coordinates");
	uiDefButS(block, ROW, B_MATPRV, "Orco",			701,146,38,18, &(mtex->texco), 4.0, (float)TEXCO_ORCO, 0, 0, "Use the original coordinates of the mesh");
	uiDefButS(block, ROW, B_MATPRV, "Stick",			739,146,38,18, &(mtex->texco), 4.0, (float)TEXCO_STICKY, 0, 0, "Use mesh sticky coordaintes for the texture coordinates");
	uiDefButS(block, ROW, B_MATPRV, "Win",			779,146,31,18, &(mtex->texco), 4.0, (float)TEXCO_WINDOW, 0, 0, "Use screen coordinates as texture coordinates");
	uiDefButS(block, ROW, B_MATPRV, "Nor",			811,146,32,18, &(mtex->texco), 4.0, (float)TEXCO_NORM, 0, 0, "Use normal vector as texture coordinates");
	uiDefButS(block, ROW, B_MATPRV, "Refl",			844,146,33,18, &(mtex->texco), 4.0, (float)TEXCO_REFL, 0, 0, "Use reflection vector as texture coordinates");
	
	uiBlockSetCol(block, BUTGREY);
	
	/* COORDS */
	uiDefButC(block, ROW, B_MATPRV, "Flat",			666,114,48,18, &(mtex->mapping), 5.0, (float)MTEX_FLAT, 0, 0, "Map X and Y coordinates directly");
	uiDefButC(block, ROW, B_MATPRV, "Cube",			717,114,50,18, &(mtex->mapping), 5.0, (float)MTEX_CUBE, 0, 0, "Map using the normal vector");
	uiDefButC(block, ROW, B_MATPRV, "Tube",			666,94,48,18, &(mtex->mapping), 5.0, (float)MTEX_TUBE, 0, 0, "Map with Z as central axis (tube-like)");
	uiDefButC(block, ROW, B_MATPRV, "Sphe",			716,94,50,18, &(mtex->mapping), 5.0, (float)MTEX_SPHERE, 0, 0, "Map with Z as central axis (sphere-like)");

	xco= 665;
	for(a=0; a<4; a++) {
		if(a==0) strcpy(str, "");
		else if(a==1) strcpy(str, "X");
		else if(a==2) strcpy(str, "Y");
		else strcpy(str, "Z");
		
		uiDefButC(block, ROW, B_MATPRV, str,			(short)xco, 50, 24, 18, &(mtex->projx), 6.0, (float)a, 0, 0, "");
		uiDefButC(block, ROW, B_MATPRV, str,			(short)xco, 30, 24, 18, &(mtex->projy), 7.0, (float)a, 0, 0, "");
		uiDefButC(block, ROW, B_MATPRV, str,			(short)xco, 10, 24, 18, &(mtex->projz), 8.0, (float)a, 0, 0, "");
		xco+= 26;
	}
	
	uiDefButF(block, NUM, B_MATPRV, "ofsX",		778,114,100,18, mtex->ofs, -10.0, 10.0, 10, 0, "Fine tune X coordinate");
	uiDefButF(block, NUM, B_MATPRV, "ofsY",		778,94,100,18, mtex->ofs+1, -10.0, 10.0, 10, 0, "Fine tune Y coordinate");
	uiDefButF(block, NUM, B_MATPRV, "ofsZ",		778,74,100,18, mtex->ofs+2, -10.0, 10.0, 10, 0, "Fine tune Z coordinate");
	uiDefButF(block, NUM, B_MATPRV, "sizeX",	778,50,100,18, mtex->size, -100.0, 100.0, 10, 0, "Set an extra scaling for the texture coordinate");
	uiDefButF(block, NUM, B_MATPRV, "sizeY",	778,30,100,18, mtex->size+1, -100.0, 100.0, 10, 0, "Set an extra scaling for the texture coordinate");
	uiDefButF(block, NUM, B_MATPRV, "sizeZ",	778,10,100,18, mtex->size+2, -100.0, 100.0, 10, 0, "Set an extra scaling for the texture coordinate");
	
	/* TEXTUREBLOK SELECT */
	if(G.main->tex.first==0)
		id= NULL;
	else
		id= (ID*) mtex->tex;
	IDnames_to_pupstring(&strp, NULL, "ADD NEW %x32767", &(G.main->tex), id, &(G.buts->texnr));
	uiDefButS(block, MENU, B_EXTEXBROWSE, strp, 900,146,20,19, &(G.buts->texnr), 0, 0, 0, 0, "The name of the texture");
	MEM_freeN(strp);

	if(id) {
		uiDefBut(block, TEX, B_IDNAME, "TE:",	900,166,163,19, id->name+2, 0.0, 18.0, 0, 0, "The name of the texture block");
		sprintf(str, "%d", id->us);
		uiDefBut(block, BUT, 0, str,				996,146,21,19, 0, 0, 0, 0, 0, "");
		uiDefIconBut(block, BUT, B_AUTOTEXNAME, ICON_AUTO, 1041,146,21,19, 0, 0, 0, 0, 0, "Auto-assign name to texture");
		if(id->lib) {
			if(ma->id.lib) uiDefIconBut(block, BUT, 0, ICON_DATALIB,	1019,146,21,19, 0, 0, 0, 0, 0, "");
			else uiDefIconBut(block, BUT, 0, ICON_PARLIB,	1019,146,21,19, 0, 0, 0, 0, 0, "");		
		}
		uiBlockSetCol(block, BUTSALMON);
		uiDefBut(block, BUT, B_TEXCLEAR, "Clear", 922, 146, 72, 19, 0, 0, 0, 0, 0, "Erase link to datablock");
		uiBlockSetCol(block, BUTGREY);
	}
	
	/* TEXTURE OUTPUT */
	uiDefButS(block, TOG|BIT|1, B_MATPRV, "Stencil",	900,114,52,18, &(mtex->texflag), 0, 0, 0, 0, "Set the mapping to stencil mode");
	uiDefButS(block, TOG|BIT|2, B_MATPRV, "Neg",		954,114,38,18, &(mtex->texflag), 0, 0, 0, 0, "Reverse the effect of the texture");
	uiDefButS(block, TOG|BIT|0, B_MATPRV, "No RGB",	994,114,69,18, &(mtex->texflag), 0, 0, 0, 0, "Use an RGB texture as an intensity texture");
	
	uiDefButF(block, COL, B_MTEXCOL, "",				900,100,163,12, &(mtex->r), 0, 0, 0, 0, "Browse datablocks");

	if(ma->colormodel==MA_HSV) {
		uiBlockSetCol(block, BUTPURPLE);
		uiDefButF(block, HSVSLI, B_MATPRV, "H ",			900,80,163,18, &(mtex->r), 0.0, 0.9999, B_MTEXCOL, 0, "");
		uiBlockSetCol(block, BUTPURPLE);
		uiDefButF(block, HSVSLI, B_MATPRV, "S ",			900,60,163,18, &(mtex->r), 0.0001, 1.0, B_MTEXCOL, 0, "");
		uiBlockSetCol(block, BUTPURPLE);
		uiDefButF(block, HSVSLI, B_MATPRV, "V ",			900,40,163,18, &(mtex->r), 0.0001, 1.0, B_MTEXCOL, 0, "");
		uiBlockSetCol(block, BUTGREY);
	}
	else {
		uiDefButF(block, NUMSLI, B_MATPRV, "R ",			900,80,163,18, &(mtex->r), 0.0, 1.0, B_MTEXCOL, 0, "Set the amount of red the intensity texture blends with");
		uiDefButF(block, NUMSLI, B_MATPRV, "G ",			900,60,163,18, &(mtex->g), 0.0, 1.0, B_MTEXCOL, 0, "Set the amount of green the intensity texture blends with");
		uiDefButF(block, NUMSLI, B_MATPRV, "B ",			900,40,163,18, &(mtex->b), 0.0, 1.0, B_MTEXCOL, 0, "Set the amount of blue the intensity texture blends with");
	}
	
	uiDefButF(block, NUMSLI, B_MATPRV, "DVar ",		900,10,163,18, &(mtex->def_var), 0.0, 1.0, 0, 0, "Set the value the texture blends with the current value");
	
	/* MAP TO */
	uiBlockSetCol(block, BUTGREEN);
	uiDefButS(block, TOG|BIT|0, B_MATPRV, "Col",		1087,166,35,18, &(mtex->mapto), 0, 0, 0, 0, "Let the texture affect basic colour of the material");
	uiDefButS(block, TOG3|BIT|1, B_MATPRV, "Nor",		1126,166,31,18, &(mtex->mapto), 0, 0, 0, 0, "Let the texture affect the rendered normal");
	uiDefButS(block, TOG|BIT|2, B_MATPRV, "Csp",		1160,166,34,18, &(mtex->mapto), 0, 0, 0, 0, "Let the texture affect the specularity colour");
	uiDefButS(block, TOG|BIT|3, B_MATPRV, "Cmir",		1196,166,35,18, &(mtex->mapto), 0, 0, 0, 0, "Let the texture affext the mirror colour");
	uiDefButS(block, TOG3|BIT|4, B_MATPRV, "Ref",		1234,166,31,18, &(mtex->mapto), 0, 0, 0, 0, "Let the texture affect the value of the materials reflectivity");
	uiDefButS(block, TOG3|BIT|5, B_MATPRV, "Spec",	1087,146,36,18, &(mtex->mapto), 0, 0, 0, 0, "Let the texture affect the value of specularity");
	uiDefButS(block, TOG3|BIT|8, B_MATPRV, "Hard",	1126,146,44,18, &(mtex->mapto), 0, 0, 0, 0, "Let the texture affect the hardness value");
	uiDefButS(block, TOG3|BIT|7, B_MATPRV, "Alpha",	1172,146,45,18, &(mtex->mapto), 0, 0, 0, 0, "Let the texture affect the alpha value");
	uiDefButS(block, TOG3|BIT|6, B_MATPRV, "Emit",	1220,146,45,18, &(mtex->mapto), 0, 0, 0, 0, "Let the texture affect the emit value");
	
/* 	uiDefButS(block, TOG|BIT|3, B_MATPRV, "Alpha Mix",1087,114,100,18, &(mtex->texflag), 0, 0, 0, 0); ,""*/

	uiBlockSetCol(block, BUTGREY);
	uiDefButS(block, ROW, B_MATPRV, "Mix",			1087,94,48,18, &(mtex->blendtype), 9.0, (float)MTEX_BLEND, 0, 0, "The texture blends the values or colour");
	uiDefButS(block, ROW, B_MATPRV, "Mul",			1136,94,44,18, &(mtex->blendtype), 9.0, (float)MTEX_MUL, 0, 0, "The texture multiplies the values or colour");
	uiDefButS(block, ROW, B_MATPRV, "Add",			1182,94,41,18, &(mtex->blendtype), 9.0, (float)MTEX_ADD, 0, 0, "The texture adds the values or colour");
	uiDefButS(block, ROW, B_MATPRV, "Sub",			1226,94,40,18, &(mtex->blendtype), 9.0, (float)MTEX_SUB, 0, 0, "The texture subtracts the values or colour");
	
	uiDefButF(block, NUMSLI, B_MATPRV, "Col ",		1087,50,179,18, &(mtex->colfac), 0.0, 1.0, 0, 0, "Set the amount the texture affects colour");
	uiDefButF(block, NUMSLI, B_MATPRV, "Nor ",		1087,30,179,18, &(mtex->norfac), 0.0, 5.0, 0, 0, "Set the amount the texture affects the normal");
	uiDefButF(block, NUMSLI, B_MATPRV, "Var ",		1087,10,179,18, &(mtex->varfac), 0.0, 1.0, 0, 0, "Set the amount the texture affects a value");
	
	uiDrawBlock(block);
}

void material_panel_preview(Material *ma)
{
	uiBlock *block;
	
	/* name "Preview" is abused to detect previewrender offset panel */
	block= uiNewBlock(&curarea->uiblocks, "material_panel_preview", UI_EMBOSSX, UI_HELV, curarea->win);
	if(uiNewPanel(curarea, block, "Preview", "Material", 0, 0, 318, 204)==0) return;
	
	uiBlockSetDrawExtraFunc(block, BIF_previewdraw);

	// labels to force a boundbox for buttons not to be centered
	uiDefBut(block, LABEL, 0, " ",	20,20,10,10, 0, 0, 0, 0, 0, "");
	uiDefBut(block, LABEL, 0, " ",	280,180,10, 10, 0, 0, 0, 0, 0, "");

	uiDefIconButC(block, ROW, B_MATPRV, ICON_MATPLANE,		230,180,25,20, &(ma->pr_type), 10, 0, 0, 0, "");
	uiDefIconButC(block, ROW, B_MATPRV, ICON_MATSPHERE,		230,150,25,20, &(ma->pr_type), 10, 1, 0, 0, "");
	uiDefIconButC(block, ROW, B_MATPRV, ICON_MATCUBE,		230,120,25,20, &(ma->pr_type), 10, 2, 0, 0, "");
	uiDefIconButS(block, ICONTOG|BIT|0, B_MATPRV, ICON_TRANSP_HLT,	230,80,25,20, &(ma->pr_back), 0, 0, 0, 0, "");
	uiDefIconBut(block, BUT, B_MATPRV, ICON_EYE,			230,10, 25,20, 0, 0, 0, 0, 0, "");

}

void material_panels()
{
	Material *ma;
	Object *ob= OBACT;
	
	if(ob==0) return;
	ma= give_current_material(ob, ob->actcol);

	if(ma) {
	
		material_panel_preview(ma);
	}
	
}



	