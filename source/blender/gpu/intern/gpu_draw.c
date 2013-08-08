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
 * The Original Code is Copyright (C) 2005 Blender Foundation.
 * All rights reserved.
 *
 * The Original Code is: all of this file.
 *
 * Contributor(s): Brecht Van Lommel.
 *
 * ***** END GPL LICENSE BLOCK *****
 */

/** \file blender/gpu/intern/gpu_draw.c
 *  \ingroup gpu
 */

/* my interface */
#include "GPU_draw.h"

/* internal */
#include "intern/gpu_lighting.h"
#include "intern/gpu_common.h"
#include "intern/gpu_extension_wrapper.h"
#include "intern/gpu_aspect.h"
#include "intern/gpu_aspectfuncs.h"
#include "intern/gpu_raster.h"

/* my library */
#include "GPU_compatibility.h"
#include "GPU_colors.h"
#include "GPU_buffers.h"
#include "GPU_extensions.h"
#include "GPU_material.h"
#include "GPU_basic_shader.h"

/* external */

#include "BLI_math.h"
#include "BLI_utildefines.h"

#include "DNA_lamp_types.h"
#include "DNA_material_types.h"
#include "DNA_meshdata_types.h"
#include "DNA_modifier_types.h"
#include "DNA_node_types.h"
#include "DNA_object_types.h"
#include "DNA_scene_types.h"
#include "DNA_smoke_types.h"
#include "DNA_view3d_types.h"

#include "MEM_guardedalloc.h"

#include "IMB_imbuf.h"
#include "IMB_imbuf_types.h"

#include "BLI_threads.h"
#include "BLI_blenlib.h"

#include "BKE_bmfont.h"
#include "BKE_global.h"
#include "BKE_image.h"
#include "BKE_main.h"
#include "BKE_material.h"
#include "BKE_node.h"
#include "BKE_object.h"
#include "BKE_scene.h"
#include "BKE_DerivedMesh.h"

#include "smoke_API.h"

/* standard */

#include <string.h>


extern Material defmaterial; /* from material.c */

/* These are some obscure rendering functions shared between the
 * game engine and the blender, in this module to avoid duplicaten
 * and abstract them away from the rest a bit */

/* Text Rendering */

static void gpu_mcol(unsigned int ucol)
{
	/* mcol order is swapped */
	char *cp= (char *)&ucol;
	gpuColor3ub(cp[3], cp[2], cp[1]);
}

void GPU_render_text(MTFace *tface, int mode,
	const char *textstr, int textlen, unsigned int *col,
	float *v1, float *v2, float *v3, float *v4, int glattrib)
{
	if ((mode & GEMAT_TEXT) && (textlen>0) && tface->tpage) {
		Image* ima = (Image *)tface->tpage;
		int index, character;
		float centerx, centery, sizex, sizey, transx, transy, movex, movey, advance;
		float advance_tab;
		
		/* multiline */
		float line_start= 0.0f, line_height;
		
		if (v4)
			line_height = max_ffff(v1[1], v2[1], v3[1], v4[2]) - min_ffff(v1[1], v2[1], v3[1], v4[2]);
		else
			line_height = max_fff(v1[1], v2[1], v3[1]) - min_fff(v1[1], v2[1], v3[1]);
		line_height *= 1.2f; /* could be an option? */
		/* end multiline */

		
		/* color has been set */
		if (tface->mode & TF_OBCOL)
			col= NULL;
		else if (!col)
			gpuColor3P(CPACK_WHITE);

		gpuPushMatrix();
		
		/* get the tab width */
		matrixGlyph((ImBuf *)ima->ibufs.first, ' ', & centerx, &centery,
			&sizex, &sizey, &transx, &transy, &movex, &movey, &advance);
		
		advance_tab= advance * 4; /* tab width could also be an option */
		
		gpuImmediateFormat_V2();

		for (index = 0; index < textlen; index++) {
			float uv[4][2];

			// lets calculate offset stuff
			character = textstr[index];
			
			if (character=='\n') {
				gpuTranslate(line_start, -line_height, 0.0);
				line_start = 0.0f;
				continue;
			}
			else if (character=='\t') {
				gpuTranslate(advance_tab, 0.0, 0.0);
				line_start -= advance_tab; /* so we can go back to the start of the line */
				continue;
				
			}
			
			// space starts at offset 1
			// character = character - ' ' + 1;
			matrixGlyph((ImBuf *)ima->ibufs.first, character, & centerx, &centery,
				&sizex, &sizey, &transx, &transy, &movex, &movey, &advance);

			uv[0][0] = (tface->uv[0][0] - centerx) * sizex + transx;
			uv[0][1] = (tface->uv[0][1] - centery) * sizey + transy;
			uv[1][0] = (tface->uv[1][0] - centerx) * sizex + transx;
			uv[1][1] = (tface->uv[1][1] - centery) * sizey + transy;
			uv[2][0] = (tface->uv[2][0] - centerx) * sizex + transx;
			uv[2][1] = (tface->uv[2][1] - centery) * sizey + transy;
			
			gpuBegin(GL_TRIANGLE_FAN);
			if (glattrib >= 0) gpuVertexAttrib2fv(glattrib, uv[0]);
			else gpuTexCoord2fv(uv[0]);
			if (col) gpu_mcol(col[0]);
			gpuVertex3f(sizex * v1[0] + movex, sizey * v1[1] + movey, v1[2]);
			
			if (glattrib >= 0) gpuVertexAttrib2fv(glattrib, uv[1]);
			else gpuTexCoord2fv(uv[1]);
			if (col) gpu_mcol(col[1]);
			gpuVertex3f(sizex * v2[0] + movex, sizey * v2[1] + movey, v2[2]);

			if (glattrib >= 0) gpuVertexAttrib2fv(glattrib, uv[2]);
			else gpuTexCoord2fv(uv[2]);
			if (col) gpu_mcol(col[2]);
			gpuVertex3f(sizex * v3[0] + movex, sizey * v3[1] + movey, v3[2]);

			if (v4) {
				uv[3][0] = (tface->uv[3][0] - centerx) * sizex + transx;
				uv[3][1] = (tface->uv[3][1] - centery) * sizey + transy;

				if (glattrib >= 0) gpuVertexAttrib2fv(glattrib, uv[3]);
				else gpuTexCoord2fv(uv[3]);
				if (col) gpu_mcol(col[3]);
				gpuVertex3f(sizex * v4[0] + movex, sizey * v4[1] + movey, v4[2]);
			}
			gpuEnd();

			gpuTranslate(advance, 0.0, 0.0);
			line_start -= advance; /* so we can go back to the start of the line */
		}

		gpuImmediateUnformat();

		gpuPopMatrix();
	}
}

/* Checking powers of two for images since opengl 1.x requires it */

static bool is_power_of_2_resolution(int w, int h)
{
	return is_power_of_2_i(w) && is_power_of_2_i(h);
}

static bool is_over_resolution_limit(int w, int h)
{
	int reslimit = (U.glreslimit != 0)?
		min_ii(U.glreslimit, GPU_max_texture_size()) :
		GPU_max_texture_size();

	return (w > reslimit || h > reslimit);
}

static int smaller_power_of_2_limit(int num)
{
	int reslimit = (U.glreslimit != 0)?
		min_ii(U.glreslimit, GPU_max_texture_size()) :
		GPU_max_texture_size();
	/* take texture clamping into account */
	if (num > reslimit)
		return reslimit;

	return power_of_2_min_i(num);
}

/* Current OpenGL state caching for GPU_set_tpage */

static struct GPUTextureState {
	int curtile, tile;
	int curtilemode, tilemode;
	int curtileXRep, tileXRep;
	int curtileYRep, tileYRep;
	Image *ima, *curima;

	int domipmap, linearmipmap;
	int texpaint; /* store this so that new images created while texture painting won't be set to mipmapped */

	int alphablend;
	float anisotropic;
	int gpu_mipmap;
	MTFace *lasttface;
} GTS = {0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, 1, 0, 0, -1, 1.f, 0, NULL};

/* Mipmap settings */

void GPU_set_gpu_mipmapping(int gpu_mipmap)
{
	int old_value = GTS.gpu_mipmap;

	/* only actually enable if it's supported */
	GTS.gpu_mipmap = gpu_mipmap && GLEW_EXT_framebuffer_object;

	if (old_value != GTS.gpu_mipmap) {
		GPU_free_images();
	}
}

void GPU_set_mipmap(int mipmap)
{
	if (GTS.domipmap != (mipmap != 0)) {
		GPU_free_images();
		GTS.domipmap = mipmap != 0;
	}
}

void GPU_set_linear_mipmap(int linear)
{
	if (GTS.linearmipmap != (linear != 0)) {
		GPU_free_images();
		GTS.linearmipmap = linear != 0;
	}
}

int GPU_get_mipmap(void)
{
	return GTS.domipmap && !GTS.texpaint;
}

int GPU_get_linear_mipmap(void)
{
	return GTS.linearmipmap;
}

static GLenum gpu_get_mipmap_filter(int mag)
{
	/* linearmipmap is off by default *when mipmapping is off,
	 * use unfiltered display */
	if (mag) {
		if (GTS.linearmipmap || GTS.domipmap)
			return GL_LINEAR;
		else
			return GL_NEAREST;
	}
	else {
		if (GTS.linearmipmap)
			return GL_LINEAR_MIPMAP_LINEAR;
		else if (GTS.domipmap)
			return GL_LINEAR_MIPMAP_NEAREST;
		else
			return GL_NEAREST;
	}
}

/* Anisotropic filtering settings */
void GPU_set_anisotropic(float value)
{
	if (GTS.anisotropic != value) {
		GPU_free_images();

		/* Clamp value to the maximum value the graphics card supports */
		if (value > GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT)
			value = GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT;

		GTS.anisotropic = value;
	}
}

float GPU_get_anisotropic(void)
{
	return GTS.anisotropic;
}

/* Set OpenGL state for an MTFace */

static void gpu_make_repbind(Image *ima)
{
	ImBuf *ibuf;
	
	ibuf = BKE_image_acquire_ibuf(ima, NULL, NULL);
	if (ibuf==NULL)
		return;

	if (ima->repbind) {
		glDeleteTextures(ima->totbind, (GLuint *)ima->repbind);
		MEM_freeN(ima->repbind);
		ima->repbind= NULL;
		ima->tpageflag &= ~IMA_MIPMAP_COMPLETE;
	}

	ima->totbind= ima->xrep*ima->yrep;

	if (ima->totbind>1)
		ima->repbind= MEM_callocN(sizeof(int)*ima->totbind, "repbind");

	BKE_image_release_ibuf(ima, ibuf, NULL);
}

static void reset_default_alphablend_state(void);

static void gpu_clear_tpage(void)
{
	if (GTS.lasttface==NULL)
		return;
	
	GTS.lasttface= NULL;
	GTS.curtile= 0;
	GTS.curima= NULL;
	if (GTS.curtilemode!=0) {
		gpuMatrixMode(GL_TEXTURE);
		gpuLoadIdentity();
		gpuMatrixMode(GL_MODELVIEW);
	}
	GTS.curtilemode= 0;
	GTS.curtileXRep=0;
	GTS.curtileYRep=0;
	GTS.alphablend= -1;

#if defined(WITH_GL_PROFILE_COMPAT)
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_TEXTURE_GEN_S);
	glDisable(GL_TEXTURE_GEN_T);
#endif

	reset_default_alphablend_state();
}

static void disable_blend(void);
static void enable_blendfunc_blend(void);
static void enable_blendfunc_add(void);
static void disable_alphatest(void);
static void enable_alphatest(GLfloat alpharef);
static void user_defined_blend(void);

static void gpu_set_alpha_blend(GPUBlendMode alphablend)
{
	switch (alphablend) {
		case GPU_BLEND_SOLID:
			disable_blend();
			disable_alphatest();
			break;

		/* for OpenGL render we use the alpha channel, this makes alpha blend correct */
		if (GLEW_VERSION_1_4)
			glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		else

		case GPU_BLEND_ADD:
			enable_blendfunc_add();
			disable_alphatest();
			break;

		case GPU_BLEND_ALPHA:
		case GPU_BLEND_ALPHA_SORT:
			enable_blendfunc_blend();
			enable_alphatest(U.glalphaclip);
			break;

		case GPU_BLEND_CLIP:
			disable_blend();
			enable_alphatest(0.5f);
			break;

		default:
			user_defined_blend();
			break;
	}
}

static void gpu_verify_alpha_blend(int alphablend)
{
	/* verify alpha blending modes */
	if (GTS.alphablend == alphablend)
		return;

	gpu_set_alpha_blend(alphablend);
	GTS.alphablend= alphablend;
}

static void gpu_verify_reflection(Image *ima)
{
#if defined(WITH_GL_PROFILE_COMPAT)
	if (ima && (ima->flag & IMA_REFLECT)) {
		/* enable reflection mapping */
		glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
		glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);

		glEnable(GL_TEXTURE_GEN_S);
		glEnable(GL_TEXTURE_GEN_T);
	}
	else {
		/* disable reflection mapping */
		glDisable(GL_TEXTURE_GEN_S);
		glDisable(GL_TEXTURE_GEN_T);
	}
#endif
}


// XXX jwilkins: I don't think this box filter is gamma correct (should take into account color correction)

#define BOX_FILTER_HALF_ONLY \
	for(y=0; y<hightout; y++)\
	{\
		for(x=0; x<widthout; x++)\
		{\
			for(sizei=0; sizei<size; sizei++)\
			{\
				int p;\
				wf = scw*x;\
				hf = sch*y;\
				win = wf;\
				hin = hf;\
				\
				mix[0][1] = wf-win;\
				mix[0][0] = 1.0f - mix[0][1];\
				\
				mix[1][1] = hf-hin;\
				mix[1][0] = 1.0f - mix[1][1];\
				\
				p = (hin*widthin+win)*size+sizei;\
				col[0][0]=p<max?imgin[p]:0;\
				p = (hin*widthin+win+1)*size+sizei;\
				col[0][1]=p<max?imgin[p]:0;\
				p = ((hin+1)*widthin+win)*size+sizei;\
				col[1][0]=p<max?imgin[p]:0;\
				p = ((hin+1)*widthin+win+1)*size+sizei;\
				col[1][1]=p<max?imgin[p]:0;\
			\
				imgout[(y*widthout+x)*size+sizei] = mix[0][0]*mix[1][0]*col[0][0]+\
													mix[0][1]*mix[1][0]*col[0][1]+\
													mix[0][0]*mix[1][1]*col[1][0]+\
													mix[0][1]*mix[1][1]*col[1][1];\
			}}}

static void ScaleHalfImageChar(int size, unsigned char *imgin, int widthin, int hightin, unsigned char* imgout, int widthout, int hightout)
{
	int sizei, x, y;
	float scw = (float)(widthin-1)/(widthout-1);
	float sch = (float)(hightin-1)/(hightout-1);
	int hin, win;
	float hf, wf;
	float mix[2][2];
	char col[2][2];
	int max = hightin*widthin*size;
	
	BOX_FILTER_HALF_ONLY
}

static void ScaleHalfImageFloat(int size, float *imgin, int widthin, int hightin, float* imgout, int widthout, int hightout)
{
	int sizei, x, y;
	float scw = (float)(widthin-1)/(widthout-1);
	float sch = (float)(hightin-1)/(hightout-1);
	int hin, win;
	float hf, wf;
	float mix[2][2];
	float col[2][2];
	int max = hightin*widthin*size;
	
	BOX_FILTER_HALF_ONLY
}

#undef BOX_FILTER_HALF_ONLY

GLenum GPU_mipmap_2D(GLboolean genmip, GLenum internalFormat, int w, int h, GLenum type, void* data)
{
	while (glGetError() != GL_NO_ERROR) {}

	if (GLEW_VERSION_3_0            ||
		GLEW_ARB_framebuffer_object ||
		GLEW_EXT_framebuffer_object ||
		GLEW_ES_VERSION_2_0         ||
		GLEW_OES_framebuffer_object)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, w, h, 0, GL_RGBA, type, data);

		if (genmip) {
			glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);
			gpu_glGenerateMipmap(GL_TEXTURE_2D);
			glHint(GL_GENERATE_MIPMAP_HINT, GL_DONT_CARE); // return to default value
		}

		return glGetError();
	}

#if defined(WITH_GL_PROFILE_COMPAT)
	if (GLEW_VERSION_1_4) {
		if (genmip) {
			glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);
			glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
		}

		glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, w, h, 0, GL_RGBA, type, data);

		if (genmip) {
			glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_FALSE);
			glHint(GL_GENERATE_MIPMAP_HINT, GL_DONT_CARE); // return to default value
		}

		return glGetError();
	}
#endif

#if defined(WITH_GLU)
	if (genmip) {
		return gluBuild2DMipmaps(GL_TEXTURE_2D, internalFormat, w, h, GL_RGBA, type, data);
	}
	else {
		glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, w, h, 0, GL_RGBA, type, data);
		return glGetError();
	}
#else
	glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, w, h, 0, GL_RGBA, type, data);
	return glGetError();
#endif
}

int GPU_verify_image(Image *ima, ImageUser *iuser, int tftile, int compare, int mipmap, bool is_data)
{
	ImBuf *ibuf = NULL;
	unsigned int *bind = NULL;
	int rectw, recth, tpx=0, tpy=0, y;
	unsigned int *tilerect= NULL, *rect= NULL;
	float *ftilerect= NULL, *frect = NULL;
	float *srgb_frect = NULL;
	short texwindx, texwindy, texwinsx, texwinsy;
	/* flag to determine whether high resolution format is used */
	int use_high_bit_depth = FALSE, do_color_management = FALSE;

	/* initialize tile mode and number of repeats */
	GTS.ima = ima;
	GTS.tilemode= (ima && (ima->tpageflag & (IMA_TILES|IMA_TWINANIM)));
	GTS.tileXRep = 0;
	GTS.tileYRep = 0;

	/* setting current tile according to frame */
	if (ima && (ima->tpageflag & IMA_TWINANIM))
		GTS.tile= ima->lastframe;
	else
		GTS.tile= tftile;

	GTS.tile = MAX2(0, GTS.tile);

	if (ima) {
		GTS.tileXRep = ima->xrep;
		GTS.tileYRep = ima->yrep;
	}

	/* if same image & tile, we're done */
	if (compare && ima == GTS.curima && GTS.curtile == GTS.tile &&
	    GTS.tilemode == GTS.curtilemode && GTS.curtileXRep == GTS.tileXRep &&
	    GTS.curtileYRep == GTS.tileYRep)
	{
		return (ima != NULL);
	}

	/* if tiling mode or repeat changed, change texture matrix to fit */
	if (GTS.tilemode!=GTS.curtilemode || GTS.curtileXRep!=GTS.tileXRep ||
	    GTS.curtileYRep != GTS.tileYRep)
	{
		gpuMatrixMode(GL_TEXTURE);
		gpuLoadIdentity();

		if (ima && (ima->tpageflag & IMA_TILES))
			gpuScale(ima->xrep, ima->yrep, 1.0);

		gpuMatrixMode(GL_MODELVIEW);
	}

	/* check if we have a valid image */
	if (ima==NULL || ima->ok==0)
		return 0;

	/* check if we have a valid image buffer */
	ibuf= BKE_image_acquire_ibuf(ima, iuser, NULL);

	if (ibuf==NULL)
		return 0;

	if (ibuf->rect_float) {
		if (U.use_16bit_textures) {
			/* use high precision textures. This is relatively harmless because OpenGL gives us
			 * a high precision format only if it is available */
			use_high_bit_depth = TRUE;
		}

		/* TODO unneeded when float images are correctly treated as linear always */
		if (!is_data)
			do_color_management = TRUE;

		if (ibuf->rect==NULL)
			IMB_rect_from_float(ibuf);
	}

	/* currently, tpage refresh is used by ima sequences */
	if (ima->tpageflag & IMA_TPAGE_REFRESH) {
		GPU_free_image(ima);
		ima->tpageflag &= ~IMA_TPAGE_REFRESH;
	}
	
	if (GTS.tilemode) {
		/* tiled mode */
		if (ima->repbind==NULL) gpu_make_repbind(ima);
		if (GTS.tile>=ima->totbind) GTS.tile= 0;
		
		/* this happens when you change repeat buttons */
		if (ima->repbind) bind= &ima->repbind[GTS.tile];
		else bind= &ima->bindcode;
		
		if (*bind==0) {
			
			texwindx= ibuf->x/ima->xrep;
			texwindy= ibuf->y/ima->yrep;
			
			if (GTS.tile>=ima->xrep*ima->yrep)
				GTS.tile= ima->xrep*ima->yrep-1;
	
			texwinsy= GTS.tile / ima->xrep;
			texwinsx= GTS.tile - texwinsy*ima->xrep;
	
			texwinsx*= texwindx;
			texwinsy*= texwindy;
	
			tpx= texwindx;
			tpy= texwindy;

			if (use_high_bit_depth) {
				if (do_color_management) {
					srgb_frect = MEM_mallocN(ibuf->x*ibuf->y*sizeof(float)*4, "floar_buf_col_cor");
					IMB_buffer_float_from_float(srgb_frect, ibuf->rect_float,
						ibuf->channels, IB_PROFILE_SRGB, IB_PROFILE_LINEAR_RGB, TRUE,
						ibuf->x, ibuf->y, ibuf->x, ibuf->x);
					IMB_buffer_float_unpremultiply(srgb_frect, ibuf->x, ibuf->y);
					/* clamp buffer colors to 1.0 to avoid artifacts due to glu for hdr images */
					IMB_buffer_float_clamp(srgb_frect, ibuf->x, ibuf->y);
					frect= srgb_frect + texwinsy*ibuf->x + texwinsx;
				}
				else
					frect= ibuf->rect_float + texwinsy*ibuf->x + texwinsx;
			}
			else
				rect= ibuf->rect + texwinsy*ibuf->x + texwinsx;
		}
	}
	else {
		/* regular image mode */
		bind= &ima->bindcode;

		if (*bind==0) {
			tpx= ibuf->x;
			tpy= ibuf->y;
			rect= ibuf->rect;
			if (use_high_bit_depth) {
				if (do_color_management) {
					frect = srgb_frect = MEM_mallocN(ibuf->x*ibuf->y*sizeof(*srgb_frect)*4, "floar_buf_col_cor");
					IMB_buffer_float_from_float(srgb_frect, ibuf->rect_float,
							ibuf->channels, IB_PROFILE_SRGB, IB_PROFILE_LINEAR_RGB, TRUE,
							ibuf->x, ibuf->y, ibuf->x, ibuf->x);
					IMB_buffer_float_unpremultiply(srgb_frect, ibuf->x, ibuf->y);
					/* clamp buffer colors to 1.0 to avoid artifacts due to glu for hdr images */
					IMB_buffer_float_clamp(srgb_frect, ibuf->x, ibuf->y);
				}
				else
					frect= ibuf->rect_float;
			}
		}
	}

	if (*bind != 0) {
		/* enable opengl drawing with textures */

		gpuBindTexture(GL_TEXTURE_2D, *bind);

		BKE_image_release_ibuf(ima, ibuf, NULL);
		return *bind;
	}

	rectw = tpx;
	recth = tpy;

	/* for tiles, copy only part of image into buffer */
	if (GTS.tilemode) {
		if (use_high_bit_depth) {
			float *frectrow, *ftilerectrow;

			ftilerect= MEM_mallocN(rectw*recth*sizeof(*ftilerect), "tilerect");

			for (y=0; y<recth; y++) {
				frectrow= &frect[y*ibuf->x];
				ftilerectrow= &ftilerect[y*rectw];

				memcpy(ftilerectrow, frectrow, tpx*sizeof(*frectrow));
			}

			frect= ftilerect;
		}
		else {
			unsigned int *rectrow, *tilerectrow;

			tilerect= MEM_mallocN(rectw*recth*sizeof(*tilerect), "tilerect");

			for (y=0; y<recth; y++) {
				rectrow= &rect[y*ibuf->x];
				tilerectrow= &tilerect[y*rectw];

				memcpy(tilerectrow, rectrow, tpx*sizeof(*rectrow));
			}
			
			rect= tilerect;
		}
	}

#ifdef WITH_DDS
	if (ibuf->ftype & DDS)
		GPU_create_gl_tex_compressed(bind, rect, rectw, recth, mipmap, ima, ibuf);
	else
#endif
		GPU_create_gl_tex(bind, rect, frect, rectw, recth, mipmap, use_high_bit_depth, ima);
	
	/* mark as non-color data texture */
	if (*bind) {
		if (is_data)
			ima->tpageflag |= IMA_GLBIND_IS_DATA;	
		else
			ima->tpageflag &= ~IMA_GLBIND_IS_DATA;	
	}

	/* clean up */
	if (tilerect)
		MEM_freeN(tilerect);
	if (ftilerect)
		MEM_freeN(ftilerect);
	if (srgb_frect)
		MEM_freeN(srgb_frect);

	BKE_image_release_ibuf(ima, ibuf, NULL);

	return *bind;
}

/* Image *ima can be NULL */
void GPU_create_gl_tex(unsigned int *bind, unsigned int *pix, float *frect, int rectw, int recth, int mipmap, int use_high_bit_depth, Image *ima)
{
	unsigned int *scalerect = NULL;
	float *fscalerect = NULL;

	int tpx = rectw;
	int tpy = recth;

	/* scale if not a power of two. this is not strictly necessary for newer
	 * GPUs (OpenGL version >= 2.0) since they support non-power-of-two-textures 
	 * Then don't bother scaling for hardware that supports NPOT textures! */
	if ((!GPU_non_power_of_two_support() && !is_power_of_2_resolution(rectw, recth)) ||
		is_over_resolution_limit(rectw, recth)) {
		rectw= smaller_power_of_2_limit(rectw);
		recth= smaller_power_of_2_limit(recth);
		
		if (use_high_bit_depth) {
			fscalerect = MEM_mallocN(rectw*recth*sizeof(*fscalerect)*4, "fscalerect");
			ScaleHalfImageFloat(4, (float*)pix, tpx, tpy, (float*)fscalerect, rectw, recth);
			frect = fscalerect;
		}
		else {
			scalerect = MEM_mallocN(rectw*recth*sizeof(*scalerect), "scalerect");
			ScaleHalfImageChar(4, (unsigned char*)pix, tpx, tpy, (unsigned char*)scalerect, rectw, recth);

			pix = scalerect;
		}
	}

	/* create image */
	glGenTextures(1, (GLuint *)bind);
	gpuBindTexture(GL_TEXTURE_2D, *bind);

	mipmap = mipmap && GPU_get_mipmap();

	if (use_high_bit_depth) {
		if (GLEW_VERSION_3_0 || GLEW_ARB_texture_float || GLEW_EXT_texture_storage || GLEW_EXT_color_buffer_half_float) {
			GPU_mipmap_2D(mipmap, GL_RGBA16F, rectw, recth, GL_FLOAT, frect);
		}
		else {
			// XXX jwilkins: not sure if falling back to RGBA internal format will work
			GPU_mipmap_2D(mipmap,  (GLEW_VERSION_1_1 || GLEW_OES_required_internalformat) ? GL_RGBA8 : GL_RGBA, rectw, recth, GL_FLOAT, frect);
		}
	}
	else {
		GPU_mipmap_2D(mipmap, GL_RGBA, rectw, recth, GL_UNSIGNED_BYTE, pix);
	}

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gpu_get_mipmap_filter(0));
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gpu_get_mipmap_filter(1));

	if (ima && mipmap)
		ima->tpageflag |= IMA_MIPMAP_COMPLETE;

	if (GLEW_EXT_texture_filter_anisotropic)
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, GPU_get_anisotropic());

	if (scalerect)
		MEM_freeN(scalerect);

	if (fscalerect)
		MEM_freeN(fscalerect);
}

/**
 * GPU_upload_dxt_texture() assumes that the texture is already bound and ready to go.
 * This is so the viewport and the BGE can share some code.
 * Returns FALSE if the provided ImBuf doesn't have a supported DXT compression format
 */
int GPU_upload_dxt_texture(ImBuf *ibuf)
{
#ifdef WITH_DDS
	GLint format = 0;
	int blocksize, height, width, i, size, offset = 0;

	width = ibuf->x;
	height = ibuf->y;

	if (GLEW_EXT_texture_compression_s3tc) {
		if (ibuf->dds_data.fourcc == FOURCC_DXT1)
			format = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
		else if (ibuf->dds_data.fourcc == FOURCC_DXT3)
			format = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
		else if (ibuf->dds_data.fourcc == FOURCC_DXT5)
			format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
	}

	if (format == 0) {
		printf("Unable to find a suitable DXT compression, falling back to uncompressed\n");
		return FALSE;
	}

	if (!is_power_of_2_resolution(width, height)) {
		printf("Unable to load non-power-of-two DXT image resolution, falling back to uncompressed\n");
		return FALSE;
	}

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gpu_get_mipmap_filter(0));
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gpu_get_mipmap_filter(1));

#if defined(WITH_GL_PROFILE_COMPAT)
	//glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE); // XXX jwilkins: blender never changes the TEXTURE_ENV_MODE
#endif

	if (GLEW_EXT_texture_filter_anisotropic)
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, GPU_get_anisotropic());

	blocksize = (ibuf->dds_data.fourcc == FOURCC_DXT1) ? 8 : 16;
	for (i=0; i<ibuf->dds_data.nummipmaps && (width||height); ++i) {
		if (width == 0)
			width = 1;
		if (height == 0)
			height = 1;

		size = ((width+3)/4)*((height+3)/4)*blocksize;

		glCompressedTexImage2D(GL_TEXTURE_2D, i, format, width, height,
			0, size, ibuf->dds_data.data + offset);

		offset += size;
		width >>= 1;
		height >>= 1;
	}

	/* set number of mipmap levels we have, needed in case they don't go down to 1x1 */
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, i-1);

	return TRUE;
#else
	(void)ibuf;
	return FALSE;
#endif
}

void GPU_create_gl_tex_compressed(unsigned int *bind, unsigned int *pix, int x, int y, int mipmap, Image *ima, ImBuf *ibuf)
{
#ifndef WITH_DDS
	(void)ibuf;
	/* Fall back to uncompressed if DDS isn't enabled */
	GPU_create_gl_tex(bind, pix, NULL, x, y, mipmap, 0, ima);
#else


	glGenTextures(1, (GLuint *)bind);
	gpuBindTexture(GL_TEXTURE_2D, *bind);

	if (GPU_upload_dxt_texture(ibuf) == 0) {
		glDeleteTextures(1, (GLuint *)bind);
		GPU_create_gl_tex(bind, pix, NULL, x, y, mipmap, 0, ima);
	}
#endif
}
static void gpu_verify_repeat(Image *ima)
{
	/* set either clamp or repeat in X/Y */
	if (ima->tpageflag & IMA_CLAMP_U)
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	else
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);

	if (ima->tpageflag & IMA_CLAMP_V)
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	else
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

int GPU_set_tpage(MTFace *tface, int mipmap, int alphablend)
{
	Image *ima;
	
	/* check if we need to clear the state */
	if (tface==NULL) {
		gpu_clear_tpage();
		return 0;
	}

	ima= tface->tpage;
	GTS.lasttface= tface;

	gpu_verify_alpha_blend(alphablend);
	gpu_verify_reflection(ima);

	if (GPU_verify_image(ima, NULL, tface->tile, 1, mipmap, FALSE)) {
		GTS.curtile= GTS.tile;
		GTS.curima= GTS.ima;
		GTS.curtilemode= GTS.tilemode;
		GTS.curtileXRep = GTS.tileXRep;
		GTS.curtileYRep = GTS.tileYRep;

		// SSS Enable Texturing
		GPU_aspect_enable(GPU_ASPECT_BASIC, GPU_BASIC_TEXTURE_2D);
	}
	else {
		// SSS Disable Texturing
		GPU_aspect_disable(GPU_ASPECT_BASIC, GPU_BASIC_TEXTURE_2D);
		
		GTS.curtile= 0;
		GTS.curima= NULL;
		GTS.curtilemode= 0;
		GTS.curtileXRep = 0;
		GTS.curtileYRep = 0;

		return 0;
	}
	
	gpu_verify_repeat(ima);
	
	/* Did this get lost in the image recode? */
	/* BKE_image_tag_time(ima);*/

	return 1;
}

/* these two functions are called on entering and exiting texture paint mode,
 * temporary disabling/enabling mipmapping on all images for quick texture
 * updates with glTexSubImage2D. images that didn't change don't have to be
 * re-uploaded to OpenGL */
void GPU_paint_set_mipmap(int mipmap)
{
	Image* ima;
	
	if (!GTS.domipmap)
		return;

	GTS.texpaint = !mipmap;

	if (mipmap) {
		for (ima=G.main->image.first; ima; ima=ima->id.next) {
			if (ima->bindcode) {
				if (ima->tpageflag & IMA_MIPMAP_COMPLETE) {
					gpuBindTexture(GL_TEXTURE_2D, ima->bindcode);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gpu_get_mipmap_filter(0));
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gpu_get_mipmap_filter(1));
				}
				else
					GPU_free_image(ima);
			}
			else
				ima->tpageflag &= ~IMA_MIPMAP_COMPLETE;
		}

	}
	else {
		for (ima=G.main->image.first; ima; ima=ima->id.next) {
			if (ima->bindcode) {
				gpuBindTexture(GL_TEXTURE_2D, ima->bindcode);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gpu_get_mipmap_filter(1));
			}
			else
				ima->tpageflag &= ~IMA_MIPMAP_COMPLETE;
		}
	}
}

void GPU_paint_update_image(Image *ima, int x, int y, int w, int h)
{
	ImBuf *ibuf;
	
	ibuf = BKE_image_acquire_ibuf(ima, NULL, NULL);
	
	if (ima->repbind || (GPU_get_mipmap() && !GTS.gpu_mipmap) || !ima->bindcode || !ibuf ||
		(w == 0) || (h == 0))
	{
		/* these cases require full reload still */
		GPU_free_image(ima);
	}
	else {
		/* for the special case, we can do a partial update
		 * which is much quicker for painting */

		/* if color correction is needed, we must update the part that needs updating. */
		if (ibuf->rect_float) {
			float *buffer = (float*)MEM_mallocN(w*h*sizeof(float)*4, "temp_texpaint_float_buf");
			int is_data = (ima->tpageflag & IMA_GLBIND_IS_DATA);
			IMB_partial_rect_from_float(ibuf, buffer, x, y, w, h, is_data);

			gpuBindTexture(GL_TEXTURE_2D, ima->bindcode);
			glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, w, h, GL_RGBA,
					GL_FLOAT, buffer);

			MEM_freeN(buffer);

			/* we have already accounted for the case where GTS.gpu_mipmap is false
			 * so we will be using GPU mipmap generation here */
			if (GPU_get_mipmap()) {
				gpu_glGenerateMipmap(GL_TEXTURE_2D);
			}
			else {
				ima->tpageflag &= ~IMA_MIPMAP_COMPLETE;
			}

			BKE_image_release_ibuf(ima, ibuf, NULL);
			return;
		}
		
		gpuBindTexture(GL_TEXTURE_2D, ima->bindcode);

		if (ibuf->x != 0)
			glPixelStorei(GL_UNPACK_ROW_LENGTH,  ibuf->x);

		if (x != 0)
			glPixelStorei(GL_UNPACK_SKIP_PIXELS, x);

		if (y != 0)
			glPixelStorei(GL_UNPACK_SKIP_ROWS,   y);

		glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, w, h, GL_RGBA,
			GL_UNSIGNED_BYTE, ibuf->rect);

		if (ibuf->x != 0)
			glPixelStorei(GL_UNPACK_ROW_LENGTH,  0); /* restore default value */

		if (x != 0)
			glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0); /* restore default value */

		if (y != 0)
			glPixelStorei(GL_UNPACK_SKIP_ROWS,   0); /* restore default value */

		/* see comment above as to why we are using gpu mipmap generation here */
		if (GPU_get_mipmap()) {
			gpu_glGenerateMipmap(GL_TEXTURE_2D);
		}
		else {
			ima->tpageflag &= ~IMA_MIPMAP_COMPLETE;
		}
	}

	BKE_image_release_ibuf(ima, ibuf, NULL);
}



void GPU_update_images_framechange(void)
{
	Image *ima;
	
	for (ima=G.main->image.first; ima; ima=ima->id.next) {
		if (ima->tpageflag & IMA_TWINANIM) {
			if (ima->twend >= ima->xrep*ima->yrep)
				ima->twend= ima->xrep*ima->yrep-1;
		
			/* check: is bindcode not in the array? free. (to do) */
			
			ima->lastframe++;
			if (ima->lastframe > ima->twend)
				ima->lastframe= ima->twsta;
		}
	}
}

int GPU_update_image_time(Image *ima, double time)
{
	int	inc = 0;
	float	diff;
	int	newframe;

	if (!ima)
		return 0;

	if (ima->lastupdate<0)
		ima->lastupdate = 0;

	if (ima->lastupdate > (float)time)
		ima->lastupdate=(float)time;

	if (ima->tpageflag & IMA_TWINANIM) {
		if (ima->twend >= ima->xrep*ima->yrep) ima->twend= ima->xrep*ima->yrep-1;
		
		/* check: is the bindcode not in the array? Then free. (still to do) */
		
		diff = (float)((float)time - ima->lastupdate);
		inc = (int)(diff*(float)ima->animspeed);

		ima->lastupdate+=((float)inc/(float)ima->animspeed);

		newframe = ima->lastframe+inc;

		if (newframe > (int)ima->twend) {
			if (ima->twend-ima->twsta != 0)
				newframe = (int)ima->twsta-1 + (newframe-ima->twend)%(ima->twend-ima->twsta);
			else
				newframe = ima->twsta;
		}

		ima->lastframe = newframe;
	}

	return inc;
}


void GPU_free_smoke(SmokeModifierData *smd)
{
	if (smd->type & MOD_SMOKE_TYPE_DOMAIN && smd->domain) {
		if (smd->domain->tex)
			GPU_texture_free(smd->domain->tex);
		smd->domain->tex = NULL;

		if (smd->domain->tex_shadow)
			GPU_texture_free(smd->domain->tex_shadow);
		smd->domain->tex_shadow = NULL;

		if (smd->domain->tex_flame)
			GPU_texture_free(smd->domain->tex_flame);
		smd->domain->tex_flame = NULL;
	}
}

void GPU_create_smoke(SmokeModifierData *smd, int highres)
{
#ifdef WITH_SMOKE
	if (smd->type & MOD_SMOKE_TYPE_DOMAIN) {
		SmokeDomainSettings *sds = smd->domain;
		if (!sds->tex && !highres) {
			/* rgba texture for color + density */
			if (smoke_has_colors(sds->fluid)) {
				float *data = MEM_callocN(sizeof(float)*sds->total_cells*4, "smokeColorTexture");
				smoke_get_rgba(sds->fluid, data, 0);
				sds->tex = GPU_texture_create_3D(sds->res[0], sds->res[1], sds->res[2], 4, data);
				MEM_freeN(data);
			}
			/* density only */
			else {
				sds->tex = GPU_texture_create_3D(sds->res[0], sds->res[1], sds->res[2], 1, smoke_get_density(sds->fluid));
			}
			sds->tex_flame = (smoke_has_fuel(sds->fluid)) ? GPU_texture_create_3D(sds->res[0], sds->res[1], sds->res[2], 1, smoke_get_flame(sds->fluid)) : NULL;
		}
		else if (!sds->tex && highres) {
			/* rgba texture for color + density */
			if (smoke_turbulence_has_colors(sds->wt)) {
				float *data = MEM_callocN(sizeof(float)*smoke_turbulence_get_cells(sds->wt)*4, "smokeColorTexture");
				smoke_turbulence_get_rgba(sds->wt, data, 0);
				sds->tex = GPU_texture_create_3D(sds->res_wt[0], sds->res_wt[1], sds->res_wt[2], 4, data);
				MEM_freeN(data);
			}
			/* density only */
			else {
				sds->tex = GPU_texture_create_3D(sds->res_wt[0], sds->res_wt[1], sds->res_wt[2], 1, smoke_turbulence_get_density(sds->wt));
			}
			sds->tex_flame = (smoke_turbulence_has_fuel(sds->wt)) ? GPU_texture_create_3D(sds->res_wt[0], sds->res_wt[1], sds->res_wt[2], 1, smoke_turbulence_get_flame(sds->wt)) : NULL;
		}

		sds->tex_shadow = GPU_texture_create_3D(sds->res[0], sds->res[1], sds->res[2], 1, sds->shadow);
	}
#else // WITH_SMOKE
	(void)highres;
	smd->domain->tex= NULL;
	smd->domain->tex_flame= NULL;
	smd->domain->tex_shadow= NULL;
#endif // WITH_SMOKE
}

static ListBase image_free_queue = {NULL, NULL};

static void gpu_queue_image_for_free(Image *ima)
{
	Image *cpy = MEM_dupallocN(ima);

	BLI_lock_thread(LOCK_OPENGL);
	BLI_addtail(&image_free_queue, cpy);
	BLI_unlock_thread(LOCK_OPENGL);
}

void GPU_free_unused_buffers(void)
{
	Image *ima;

	if (!BLI_thread_is_main())
		return;

	BLI_lock_thread(LOCK_OPENGL);

	/* images */
	for (ima=image_free_queue.first; ima; ima=ima->id.next)
		GPU_free_image(ima);

	BLI_freelistN(&image_free_queue);

	/* vbo buffers */
	/* it's probably not necessary to free all buffers every frame */
	/* GPU_buffer_pool_free_unused(0); */

	BLI_unlock_thread(LOCK_OPENGL);
}

void GPU_free_image(Image *ima)
{
	if (!BLI_thread_is_main()) {
		gpu_queue_image_for_free(ima);
		return;
	}

	/* free regular image binding */
	if (ima->bindcode) {
		glDeleteTextures(1, (GLuint *)&ima->bindcode);
		ima->bindcode= 0;
	}

	/* free glsl image binding */
	if (ima->gputexture) {
		GPU_texture_free(ima->gputexture);
		ima->gputexture= NULL;
	}

	/* free repeated image binding */
	if (ima->repbind) {
		glDeleteTextures(ima->totbind, (GLuint *)ima->repbind);
	
		MEM_freeN(ima->repbind);
		ima->repbind= NULL;
	}

	ima->tpageflag &= ~(IMA_MIPMAP_COMPLETE|IMA_GLBIND_IS_DATA);
}

void GPU_free_images(void)
{
	Image* ima;

	if (G.main)
		for (ima=G.main->image.first; ima; ima=ima->id.next)
			GPU_free_image(ima);
}

/* same as above but only free animated images */
void GPU_free_images_anim(void)
{
	Image* ima;

	if (G.main)
		for (ima=G.main->image.first; ima; ima=ima->id.next)
			if (ELEM(ima->source, IMA_SRC_SEQUENCE, IMA_SRC_MOVIE))
				GPU_free_image(ima);
}

/* OpenGL Materials */

#define FIXEDMAT	8

/* OpenGL state caching for materials */

typedef struct GPUMaterialFixed {
	float diff[4];
	float spec[4];
	int hard;
} GPUMaterialFixed; 

static struct GPUMaterialState {
	GPUMaterialFixed (*matbuf);
	GPUMaterialFixed matbuf_fixed[FIXEDMAT];
	int totmat;

	Material **gmatbuf;
	Material *gmatbuf_fixed[FIXEDMAT];
	Material *gboundmat;
	Object *gob;
	Scene *gscene;
	int glay;
	float (*gviewmat)[4];
	float (*gviewinv)[4];

	int backface_culling;

	GPUBlendMode *alphablend;
	GPUBlendMode alphablend_fixed[FIXEDMAT];
	int use_alpha_pass, is_alpha_pass;

	int lastmatnr, lastretval;
	GPUBlendMode lastalphablend;

	GLboolean lastblendenabled;
	GLboolean lastblendfuncdefault;
	GLboolean lastalphatestenabled;
	GLfloat   lastalphatestref;
} GMS = {NULL};


static void disable_blend(void)
{
	if (GMS.lastblendenabled) {
		glDisable(GL_BLEND);
		GMS.lastblendenabled = GL_FALSE;
	}
}

static void enable_blend(void)
{
	if (!GMS.lastblendenabled) {
		glEnable(GL_BLEND);
		GMS.lastblendenabled = GL_TRUE;
	}
}

static void enable_blendfunc_blend(void)
{
	enable_blend();

	if (!GMS.lastblendfuncdefault) {
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  /* set blend function default */
		GMS.lastblendfuncdefault = GL_TRUE;
	}
}

static void enable_blendfunc_add(void)
{
	enable_blend();

	if (GMS.lastblendfuncdefault) {
		glBlendFunc(GL_ONE, GL_ONE); /* non-standard blend function */
		GMS.lastblendfuncdefault = GL_FALSE;
	}
}



static void disable_alphatest(void)
{
	if (GMS.lastalphatestenabled) {
		glDisable(GL_ALPHA_TEST);
		GMS.lastalphatestenabled = GL_FALSE;
	}
}

static void enable_alphatest(GLfloat alphatestref)
{
#if defined(WITH_GL_PROFILE_COMPAT) // XXX jwilkins: It might be better to figure out an alpha test replacement on a case by case basis
	/* if ref == 1, some cards go bonkers; turn off alpha test in this case */
	GLboolean enable    = alphatestref < 1.0f;
	GLboolean refchange = enable && (alphatestref != GMS.lastalphatestref);

	if (refchange || enable != GMS.lastalphatestenabled) {
		if (enable) {
			glEnable(GL_ALPHA_TEST);

			if (refchange) {
				glAlphaFunc(GL_GREATER, alphatestref);
				GMS.lastalphatestref = alphatestref;
			}
		}
		else {
			glDisable(GL_ALPHA_TEST);
		}

		GMS.lastalphatestenabled = enable;
	}
#endif
}

/* For when some external code needs to set up a custom blend mode
   disables alpha test, enables blending, and marks blend mode as non-default */
static void user_defined_blend()
{
	enable_blend();
	disable_alphatest();
	GMS.lastblendfuncdefault = GL_FALSE; 
}

static void reset_default_alphablend_state(void)
{
#if defined(WITH_GL_PROFILE_COMPAT) // XXX jwilkins: It might be better to figure out an alpha test replacement on a case by case basis
	disable_alphatest();
	disable_blend();

	if (!GMS.lastblendfuncdefault) {
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); /* reset blender default */
		GMS.lastblendfuncdefault = GL_TRUE;
	}

	if (GMS.lastalphatestref != 0.5f) {
		glAlphaFunc(GL_GREATER, 0.5f); /* reset blender default */
		GMS.lastalphatestref = 0.5f;
	}
#endif
}

/* fixed function material, alpha handed by caller */
static void gpu_material_to_fixed(GPUMaterialFixed *smat, const Material *bmat, const int gamma, const Object *ob, const int new_shading_nodes)
{
	if (new_shading_nodes || bmat->mode & MA_SHLESS) {
		copy_v3_v3(smat->diff, &bmat->r);
		smat->diff[3]= 1.0;

		if (gamma)
			linearrgb_to_srgb_v3_v3(smat->diff, smat->diff);

		zero_v4(smat->spec);
		smat->hard= 0;
	}
	else {
		mul_v3_v3fl(smat->diff, &bmat->r, bmat->ref + bmat->emit);
		smat->diff[3]= 1.0; /* caller may set this to bmat->alpha */

		if (bmat->shade_flag & MA_OBCOLOR)
			mul_v3_v3(smat->diff, ob->col);
		
		mul_v3_v3fl(smat->spec, &bmat->specr, bmat->spec);
		smat->spec[3]= 1.0; /* always 1 */
		smat->hard= CLAMPIS(bmat->har, 0, 128);

		if (gamma) {
			linearrgb_to_srgb_v3_v3(smat->diff, smat->diff);
			linearrgb_to_srgb_v3_v3(smat->spec, smat->spec);
		}
	}
}

static Material *gpu_active_node_material(Material *ma)
{
	if (ma && ma->use_nodes && ma->nodetree) {
		bNode *node = nodeGetActiveID(ma->nodetree, ID_MA);

		if (node)
			return (Material *)node->id;
		else
			return NULL;
	}

	return ma;
}

void GPU_begin_object_materials(View3D *v3d, RegionView3D *rv3d, Scene *scene, Object *ob, bool glsl, bool *do_alpha_after)
{
	Material *ma;
	GPUMaterial *gpumat;
	GPUBlendMode alphablend;
	int a;
	int gamma = BKE_scene_check_color_management_enabled(scene);
	int new_shading_nodes = BKE_scene_use_new_shading_nodes(scene);
	int use_matcap = (v3d->flag2 & V3D_SHOW_SOLID_MATCAP); /* assumes v3d->defmaterial->preview is set */
	
	/* initialize state */
	memset(&GMS, 0, sizeof(GMS));

	GMS.lastmatnr  = -1;
	GMS.lastretval = -1;

	GMS.lastalphablend        = GPU_BLEND_SOLID;

	GMS.lastalphatestenabled  = GL_FALSE;
	GMS.lastalphatestref      = 0.5f;
	GMS.lastblendenabled      = GL_FALSE;
	GMS.lastblendfuncdefault  = GL_TRUE;

	GMS.backface_culling = (v3d->flag2 & V3D_BACKFACE_CULLING);

	GMS.gob = ob;
	GMS.gscene = scene;
	GMS.totmat = use_matcap ? 1 : ob->totcol + 1;  /* materials start from 1, default material is 0 */
	GMS.glay= (v3d->localvd)? v3d->localvd->lay: v3d->lay; /* keep lamps visible in local view */
	GMS.gviewmat= rv3d->viewmat;
	GMS.gviewinv= rv3d->viewinv;

	/* alpha pass setup. there's various cases to handle here:
	 * - object transparency on: only solid materials draw in the first pass,
	 * and only transparent in the second 'alpha' pass.
	 * - object transparency off: for glsl we draw both in a single pass, and
	 * for solid we don't use transparency at all. */
	GMS.use_alpha_pass = (do_alpha_after != NULL);
	GMS.is_alpha_pass = (v3d->transp != FALSE);
	if (GMS.use_alpha_pass)
		*do_alpha_after = false;
	
	if (GMS.totmat > FIXEDMAT) {
		GMS.matbuf= MEM_callocN(sizeof(GPUMaterialFixed)*GMS.totmat, "GMS.matbuf");
		GMS.gmatbuf= MEM_callocN(sizeof(*GMS.gmatbuf)*GMS.totmat, "GMS.matbuf");
		GMS.alphablend= MEM_callocN(sizeof(*GMS.alphablend)*GMS.totmat, "GMS.matbuf");
	}
	else {
		GMS.matbuf= GMS.matbuf_fixed;
		GMS.gmatbuf= GMS.gmatbuf_fixed;
		GMS.alphablend= GMS.alphablend_fixed;
	}

	/* viewport material, setup in space_view3d, defaults to matcap using ma->preview now */
	if (use_matcap) {
		GMS.gmatbuf[0] = v3d->defmaterial;
		GPU_material_matcap(scene, v3d->defmaterial);
		
		/* do material 1 too, for displists! */
		memcpy(&GMS.matbuf[1], &GMS.matbuf[0], sizeof(GPUMaterialFixed));
	
		GMS.alphablend[0]= GPU_BLEND_SOLID;
	}
	else {
	
		/* no materials assigned? */
		if (ob->totcol==0) {
			gpu_material_to_fixed(&GMS.matbuf[0], &defmaterial, 0, ob, new_shading_nodes);

			/* do material 1 too, for displists! */
			memcpy(&GMS.matbuf[1], &GMS.matbuf[0], sizeof(GPUMaterialFixed));

			if (glsl) {
				GMS.gmatbuf[0]= &defmaterial;
				GPU_material_from_blender(GMS.gscene, &defmaterial);
			}

			GMS.alphablend[0]= GPU_BLEND_SOLID;
		}
		
		/* setup materials */
		for (a=1; a<=ob->totcol; a++) {
			/* find a suitable material */
			ma= give_current_material(ob, a);
			if (!glsl && !new_shading_nodes) ma= gpu_active_node_material(ma);
			if (ma==NULL) ma= &defmaterial;

			/* create glsl material if requested */
			gpumat = (glsl)? GPU_material_from_blender(GMS.gscene, ma): NULL;

			if (gpumat) {
				/* do glsl only if creating it succeed, else fallback */
				GMS.gmatbuf[a]= ma;
				alphablend = GPU_material_alpha_blend(gpumat, ob->col);
			}
			else {
				/* fixed function opengl materials */
				gpu_material_to_fixed(&GMS.matbuf[a], ma, gamma, ob, new_shading_nodes);

				if (GMS.use_alpha_pass) {
					GMS.matbuf[a].diff[3]= ma->alpha;
					alphablend = (ma->alpha == 1.0f)? GPU_BLEND_SOLID: GPU_BLEND_ALPHA;
				}
				else {
					GMS.matbuf[a].diff[3]= 1.0f;
					alphablend = GPU_BLEND_SOLID;
				}
			}

			/* setting 'do_alpha_after = TRUE' indicates this object needs to be
			 * drawn in a second alpha pass for improved blending */
			if (do_alpha_after && !GMS.is_alpha_pass)
				if (ELEM3(alphablend, GPU_BLEND_ALPHA, GPU_BLEND_ADD, GPU_BLEND_ALPHA_SORT))
					*do_alpha_after = true;

			GMS.alphablend[a]= alphablend;
		}
	}
	
	/* let's start with a clean state */
	GPU_disable_material();
}

int GPU_enable_material(int nr, void *attribs)
{
	GPUVertexAttribs *gattribs = attribs;
	GPUMaterial *gpumat;
	GPUBlendMode alphablend;

	/* no GPU_begin_object_materials, use default material */
	if (!GMS.matbuf) {
		float diff[4], spec[4];

		memset(&GMS, 0, sizeof(GMS));

		GMS.lastalphatestenabled  = GL_FALSE;
		GMS.lastalphatestref      = 0.5f;
		GMS.lastblendenabled      = GL_FALSE;
		GMS.lastblendfuncdefault  = GL_TRUE;

		mul_v3_v3fl(diff, &defmaterial.r, defmaterial.ref + defmaterial.emit);
		diff[3]= 1.0;

		mul_v3_v3fl(spec, &defmaterial.specr, defmaterial.spec);
		spec[3]= 1.0;

		// SSS Material
		gpuColor4fv(diff);
		GPU_set_basic_material_specular (spec);
		GPU_set_basic_material_shininess(35); /* blender default */

		return 0;
	}

	/* prevent index to use un-initialized array items */
	if (nr>=GMS.totmat)
		nr= 0;

	if (gattribs)
		memset(gattribs, 0, sizeof(*gattribs));

	/* keep current material */
	if (nr==GMS.lastmatnr)
		return GMS.lastretval;

	/* unbind glsl material */
	if (GMS.gboundmat) {
		if (GMS.is_alpha_pass) gpuDepthMask(GL_FALSE);
		GPU_material_unbind(GPU_material_from_blender(GMS.gscene, GMS.gboundmat));
		GMS.gboundmat= NULL;
	}

	/* draw materials with alpha in alpha pass */
	GMS.lastmatnr = nr;
	GMS.lastretval = 1;

	if (GMS.use_alpha_pass) {
		GMS.lastretval = ELEM(GMS.alphablend[nr], GPU_BLEND_SOLID, GPU_BLEND_CLIP);
		if (GMS.is_alpha_pass)
			GMS.lastretval = !GMS.lastretval;
	}
	else
		GMS.lastretval = !GMS.is_alpha_pass;

	if (GMS.lastretval) {
		/* for alpha pass, use alpha blend */
		alphablend = GMS.alphablend[nr];

		if (gattribs && GMS.gmatbuf[nr]) {
			/* bind glsl material and get attributes */
			Material *mat = GMS.gmatbuf[nr];
			float auto_bump_scale;

			gpumat = GPU_material_from_blender(GMS.gscene, mat);
			GPU_material_vertex_attributes(gpumat, gattribs);
			GPU_material_bind(gpumat, GMS.gob->lay, GMS.glay, 1.0, !(GMS.gob->mode & OB_MODE_TEXTURE_PAINT), GMS.gviewmat, GMS.gviewinv);

			auto_bump_scale = GMS.gob->derivedFinal != NULL ? GMS.gob->derivedFinal->auto_bump_scale : 1.0f;
			GPU_material_bind_uniforms(gpumat, GMS.gob->obmat, GMS.gob->col, auto_bump_scale);
			GMS.gboundmat= mat;

			/* for glsl use alpha blend mode, unless it's set to solid and
			 * we are already drawing in an alpha pass */
			if (mat->game.alpha_blend != GPU_BLEND_SOLID)
				alphablend= mat->game.alpha_blend;

			if (GMS.is_alpha_pass) gpuDepthMask(GL_TRUE);

			if (GMS.backface_culling) {
				if (mat->game.flag)
					glEnable(GL_CULL_FACE);
				else
					glDisable(GL_CULL_FACE);
			}
		}
		else {
			/* or do a basic material */

			// SSS Material
			gpuColor4fv(GMS.matbuf[nr].diff);
			GPU_set_basic_material_specular (GMS.matbuf[nr].spec);
			GPU_set_basic_material_shininess(GMS.matbuf[nr].hard);
		}

		/* set (alpha) blending mode */
		GPU_set_material_alpha_blend(alphablend);
	}

	return GMS.lastretval;
}

void GPU_set_material_alpha_blend(int alphablend)
{
	if (GMS.lastalphablend != alphablend) {
		gpu_set_alpha_blend(alphablend);
		GMS.lastalphablend = alphablend;
	}
}

int GPU_get_material_alpha_blend(void)
{
	return GMS.lastalphablend;
}

void GPU_disable_material(void)
{
	GMS.lastmatnr= -1;
	GMS.lastretval= 1;

	if (GMS.gboundmat) {
		if (GMS.backface_culling)
			glDisable(GL_CULL_FACE);

		if (GMS.is_alpha_pass) gpuDepthMask(GL_FALSE);
		GPU_material_unbind(GPU_material_from_blender(GMS.gscene, GMS.gboundmat));
		GMS.gboundmat= NULL;
	}

	reset_default_alphablend_state();
}

void GPU_material_diffuse_get(int nr, float diff[4])
{
	/* prevent index to use un-initialized array items */
	if (nr >= GMS.totmat)
		nr = 0;

	/* no GPU_begin_object_materials, use default material */
	if (!GMS.matbuf) {
		mul_v3_v3fl(diff, &defmaterial.r, defmaterial.ref + defmaterial.emit);
	}
	else {
		copy_v4_v4(diff, GMS.matbuf[nr].diff);
	}
}

void GPU_end_object_materials(void)
{
	GPU_disable_material();

	if (GMS.matbuf && GMS.matbuf != GMS.matbuf_fixed) {
		MEM_freeN(GMS.matbuf);
		MEM_freeN(GMS.gmatbuf);
		MEM_freeN(GMS.alphablend);
	}

	GMS.matbuf= NULL;
	GMS.gmatbuf= NULL;
	GMS.alphablend= NULL;

	/* resetting the texture matrix after the scaling needed for tiled textures */
	if (GTS.tilemode) {
		gpuMatrixMode(GL_TEXTURE);
		gpuLoadIdentity();
		gpuMatrixMode(GL_MODELVIEW);
	}
}

/* Lights */

int GPU_default_lights(void)
{
	int a;
	GPUbasiclight lights[GPU_MAX_COMMON_LIGHTS];

	if (!(U.light[0].flag || U.light[1].flag || U.light[2].flag)) {
		/* initialize */

		U.light[0].flag = 1;
		U.light[1].flag = 0;
		U.light[2].flag = 0;

		VEC4D(U.light[0].vec,  -0.3f,  0.3f,  0.9f,  0.0f);
		VEC4D(U.light[0].col,   0.8f,  0.8f,  0.8f,  1.0f);
		VEC4D(U.light[0].spec,  0.5f,  0.5f,  0.5f,  1.0f);

		VEC4D(U.light[1].vec,   0.5f,  0.5f,  0.1f,  0.0f);
		VEC4D(U.light[1].col,   0.4f,  0.4f,  0.8f,  1.0f);
		VEC4D(U.light[1].spec,  0.3f,  0.3f,  0.5f,  1.0f);

		VEC4D(U.light[2].vec,   0.3f, -0.3f, -0.2f,  0.0f);
		VEC4D(U.light[2].col,   0.8f,  0.5f,  0.4f,  1.0f);
		VEC4D(U.light[2].spec,  0.5f,  0.4f,  0.3f,  1.0f);
	}

	for (a = 0; a < 3; a++) {
		if (a < 3 && U.light[a].flag) {
			lights[a] = GPU_DEFAULT_LIGHT;

			normalize_v3_v3(lights[a].position, U.light[a].vec);
			lights[a].position[3]= 0.0f;

			copy_v4_v4(lights[a].diffuse,  U.light[a].col);
			copy_v4_v4(lights[a].specular, U.light[a].spec);
		}
	}

	GPU_set_basic_lights(3, lights);

	return 3;
}



int GPU_scene_object_lights(Scene *scene, Object *ob, int lay, float viewmat[4][4], int ortho)
{
	GPUbasiclight lights[GPU_MAX_COMMON_LIGHTS];

	Base *base;
	Lamp *la;
	int count = 0;

	/* disable all lights */
	GPU_set_basic_lights(0, NULL);

	/* view direction for specular is not compute correct by default in
	 * opengl, so we set the settings ourselfs */

	// SSS Enable/Disable Local Viewer
	if (ortho)
		GPU_aspect_disable(GPU_ASPECT_BASIC, GPU_BASIC_LOCAL_VIEWER);
	else
		GPU_aspect_enable (GPU_ASPECT_BASIC, GPU_BASIC_LOCAL_VIEWER);

	for (base=(Base*)(scene->base.first); base; base=base->next) {
		if (base->object->type!=OB_LAMP)
			continue;

		if (!(base->lay & lay) || !(base->lay & ob->lay))
			continue;

		la = (Lamp*)(base->object->data);
		
		/* setup lamp transform */
		gpuPushMatrix();
		gpuLoadMatrix((float *)viewmat);

		BKE_object_where_is_calc_simul(scene, base->object);

		lights[count] = GPU_DEFAULT_LIGHT;

		if (la->type==LA_SUN) {
			/* sun lamp */
			copy_v3_v3(lights[count].position, base->object->obmat[2]);
			lights[count].position[3] = 0;
		}
		else {
			/* other lamps with attenuation */
			copy_v3_v3(lights[count].position, base->object->obmat[3]);
			lights[count].position[3]= 1.0f;

			lights[count].constant_attenuation  = 1;
			lights[count].linear_attenuation    = la->att1/la->dist;
			lights[count].quadratic_attenuation = la->att2/(la->dist*la->dist);

			if (la->type==LA_SPOT) {
				/* spot lamp */
				negate_v3_v3(lights[count].spot_direction, base->object->obmat[2]);
				lights[count].spot_cutoff   = la->spotsize/2.0f;
				lights[count].spot_exponent = 128.0f*la->spotblend;
			}
		}

		/* setup energy */

		mul_v3_v3fl(lights[count].diffuse, &la->r, la->energy);
		lights[count].diffuse[3]= 1.0;

		copy_v4_v4(lights[count].specular, lights[count].diffuse);

		gpuPopMatrix();

		count++;

		if (count == GPU_MAX_COMMON_LIGHTS)
			break;
	}

	GPU_set_basic_lights(count, lights);

	return count;
}



/* Default OpenGL State */
// XXX jwilkins: is this meant to return OpenGL to a Blender default state no matter what?  if so then there needs to be more in here
// if it is meant to put OpenGL into an initial state for Blender just once then it could be smaller.
void GPU_state_init(void)
{
	/* also called when doing opengl rendering and in the game engine */

	static const float init_color[] = { 0.5, 0.5, 0.5, 1.0 };

	// SSS Material
	GPU_set_basic_material_specular (init_color);
	GPU_set_basic_material_shininess(35);

	GPU_default_lights();

	glDepthFunc(GL_LEQUAL);

#if defined(WITH_GL_PROFILE_COMPAT)
	/* scaling matrices */
	if (GPU_PROFILE_COMPAT) {
		glEnable(GL_NORMALIZE);
		glDisable(GL_ALPHA_TEST);
		glDisable(GL_FOG);
		glDisable(GL_TEXTURE_2D);
	}
#endif

	/* make sure double side isn't used by default and only getting enabled in places where it's
	 * really needed to prevent different unexpected behaviors like with intel gme965 card (sergey) */
	// SSS Disable Everything
	GPU_aspect_disable(GPU_ASPECT_BASIC, 0xFFFFFF);

	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_STENCIL_TEST);

#if defined(WITH_GL_PROFILE_COMPAT)
	if (GPU_PROFILE_COMPAT) {
		glDisable(GL_LOGIC_OP);
		glDisable(GL_TEXTURE_1D);

		/* default disabled, enable should be local per function */
		glDisableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_NORMAL_ARRAY);
		glDisableClientState(GL_COLOR_ARRAY);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);

		glPixelTransferi(GL_MAP_COLOR, GL_FALSE);
		glPixelTransferi(GL_RED_SCALE, 1);
		glPixelTransferi(GL_RED_BIAS, 0);
		glPixelTransferi(GL_GREEN_SCALE, 1);
		glPixelTransferi(GL_GREEN_BIAS, 0);
		glPixelTransferi(GL_BLUE_SCALE, 1);
		glPixelTransferi(GL_BLUE_BIAS, 0);
		glPixelTransferi(GL_ALPHA_SCALE, 1);
		glPixelTransferi(GL_ALPHA_BIAS, 0);
	
		glPixelTransferi(GL_DEPTH_BIAS, 0);
		glPixelTransferi(GL_DEPTH_SCALE, 1);

		// XXX jwilkins: It might be better to figure out an alpha test replacement on a case by case basis
		glAlphaFunc(GL_GREATER, 0.5f); /* init blender default */
	}
#endif

	gpuDepthRange(0.0, 1.0);

	gpu_init_stipple();

	gpuMatrixMode(GL_TEXTURE);
	gpuLoadIdentity();
	gpuMatrixMode(GL_MODELVIEW);

	glFrontFace(GL_CCW);
	glCullFace(GL_BACK);
	glDisable(GL_CULL_FACE);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); /* init blender default */

	/* calling this makes drawing very slow when AA is not set up in ghost
	 * on Linux/NVIDIA. */
	// glDisable(GL_MULTISAMPLE);
}
