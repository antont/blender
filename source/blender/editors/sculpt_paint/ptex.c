#include "MEM_guardedalloc.h"

#include "DNA_object_types.h"
#include "DNA_mesh_types.h"
#include "DNA_meshdata_types.h"
#include "DNA_space_types.h"

#include "RNA_access.h"
#include "RNA_define.h"

#include "BKE_context.h"
#include "BKE_customdata.h"
#include "BKE_DerivedMesh.h"
#include "BKE_report.h"
#include "BKE_subsurf.h"

#include "BLI_math.h"

#include "WM_api.h"
#include "WM_types.h"

#include "ptex.h"

#include <assert.h>
#include <stdlib.h>

static int next_power_of_two(int n)
{
	n--;
	n = (n >> 1) | n;
	n = (n >> 2) | n;
	n = (n >> 4) | n;
	n = (n >> 8) | n;
	n = (n >> 16) | n;
	n++;

	return n;
}

static const void *ptex_default_data(PtexDataType type) {
	static const unsigned char ptex_def_val_uc[] = {255, 255, 255, 255};
	static const unsigned short ptex_def_val_us[] = {65535, 65535, 65535, 65535};
	static const float ptex_def_val_f[] = {1, 1, 1, 1};

	switch(type) {
	case PTEX_DT_UINT8:
		return ptex_def_val_uc;
	case PTEX_DT_UINT16:
		return ptex_def_val_us;
	case PTEX_DT_FLOAT:
		return ptex_def_val_f;
	default:
		return NULL;
	};
}

/* add a new ptex layer
   automatically sets resolution based on face area */
static int ptex_layer_add_exec(bContext *C, wmOperator *op)
{
	Object *ob = CTX_data_active_object(C);
	Mesh *me = ob->data;
	MPtex *mptex;
	float (*limit_pos)[3], *face_area, totarea;
	float density;
	float largest_face_area = 0;
	const void *def_val;
	PtexDataType type;
	int totchannel;
	int layer_size;
	int tottexel = 0;
	int i, j;

	type = RNA_enum_get(op->ptr, "type");
	totchannel = RNA_int_get(op->ptr, "channels");
	layer_size = ptex_data_size(type) * totchannel;
	def_val = ptex_default_data(type);

	mptex = CustomData_add_layer(&me->fdata, CD_MPTEX, CD_CALLOC,
				     NULL, me->totface);

	/* TODO: for now i'm allocating texels based on limit surface area;
	   according to ptex paper it's better to use surface derivatives */

	limit_pos = MEM_callocN(sizeof(float)*3*me->totvert, "limit_pos");
	face_area = MEM_callocN(sizeof(float)*me->totface, "face_area");
	subsurf_calculate_limit_positions(me, limit_pos);
	for(i = 0; i < me->totface; ++i) {
		MFace *f = &me->mface[i];
		if(f->v4) {
			face_area[i] = area_quad_v3(limit_pos[f->v1], limit_pos[f->v2],
						    limit_pos[f->v3], limit_pos[f->v4]);
		}
		else {
			face_area[i] = area_tri_v3(limit_pos[f->v1], limit_pos[f->v2],
						   limit_pos[f->v3]);
		}
		largest_face_area = MAX2(largest_face_area, face_area[i]);
		totarea += face_area[i];
	}

	/* try to make the density factor less dependent on mesh size */
	density = RNA_float_get(op->ptr, "density") * 1000 / largest_face_area;
	
	for(i = 0; i < me->totface; ++i) {
		int S = me->mface[i].v4 ? 4 : 3;
		int ures;
		int vres;
		int gridsize;
		int texels;
		char *data;

		if(S == 4) {
			/* adjust u and v resolution by the ration
			   between the average edge size in u and v
			   directions */
			float len1 = (len_v3v3(limit_pos[me->mface[i].v1],
					       limit_pos[me->mface[i].v2]) +
				      len_v3v3(limit_pos[me->mface[i].v3],
					       limit_pos[me->mface[i].v4])) * 0.5f;
			float len2 = (len_v3v3(limit_pos[me->mface[i].v2],
					       limit_pos[me->mface[i].v3]) +
				      len_v3v3(limit_pos[me->mface[i].v4],
					       limit_pos[me->mface[i].v1])) * 0.5f;
			float r = len2/len1;

			ures = next_power_of_two(sqrtf((face_area[i] * density) * r));
			vres = next_power_of_two(sqrtf((face_area[i] * density) / r));
		}
		else {
			/* do triangles uniform (subfaces) */
			ures = sqrtf(face_area[i] * (density / 3.0f));
			vres = ures = next_power_of_two(ures);
		}

		ures = MAX2(ures, 1);
		vres = MAX2(vres, 1);
		gridsize = ures * vres;

		mptex[i].res[0][0] = ures;
		mptex[i].res[0][1] = vres;
		mptex[i].type = type;
		mptex[i].channels = totchannel;

		if(S == 4) {
			mptex[i].subfaces = 1;
			texels = gridsize;
		}
		else {
			mptex[i].res[1][0] = mptex[i].res[2][0] = ures;
			mptex[i].res[1][1] = mptex[i].res[2][1] = vres;
			mptex[i].subfaces = S;
			texels = S*gridsize;
		}
		
		data = mptex[i].data = MEM_callocN(layer_size * texels, "PTex colors");
		tottexel += texels;
		
		for(j = 0; j < texels; ++j) {
			memcpy(data, def_val, layer_size);
			data += layer_size;
		}
	}

	printf("total texels = %d, sqrt(texels)=%.1f\n", tottexel, sqrtf(tottexel));

	MEM_freeN(face_area);
	MEM_freeN(limit_pos);

	return OPERATOR_FINISHED;
}

void PTEX_OT_layer_add(wmOperatorType *ot)
{
	static EnumPropertyItem type_items[]= {
		{PTEX_DT_UINT8, "PTEX_DT_UINT8", 0, "8-bit channels", ""},
		{PTEX_DT_UINT16, "PTEX_DT_UINT16", 0, "16-bit channels", ""},
		{PTEX_DT_FLOAT, "PTEX_DT_FLOAT", 0, "32-bit floating-point channels", ""},

		{0, NULL, 0, NULL, NULL}};
	
	/* identifiers */
	ot->name= "Add Layer";
	ot->description= "Add a new ptex layer";
	ot->idname= "PTEX_OT_layer_add";
	
	/* api callbacks */
	ot->exec= ptex_layer_add_exec;
	
	/* flags */
	ot->flag= OPTYPE_REGISTER|OPTYPE_UNDO;

	/* properties */
	RNA_def_float(ot->srna, "density", 10, 0, 6000, "Density", "Density of texels to generate", 0, 6000);
	RNA_def_int(ot->srna, "channels", 3, 1, 4, "Channels", "", 1, 4);
	RNA_def_enum(ot->srna, "type", type_items, PTEX_DT_FLOAT, "Type", "Layer channels and data type");
}

/* loads a .ptx file
   makes some fairly strict assumptions that could
   be relaxed later as our ptex implementation is refined

   on the other hand, some unsupported ptex features
   are not checked for yet
*/
int ptex_open_exec(bContext *C, wmOperator *op)
{
	Object *ob = CTX_data_active_object(C);
	Mesh *me = ob->data;
	MPtex *mptex;

	PtexTextureHandle *ptex_texture;
	PtexDataType ptex_data_type;
	int totchannel, layersize;

	char *path;
	int i, j;
	
	path = RNA_string_get_alloc(op->ptr, "filepath", NULL, 0);

	ptex_texture = ptex_open(path, 1, 0);
	MEM_freeN(path);

	/* check if loader worked */
	if(!ptex_texture) {
		BKE_report(op->reports, RPT_ERROR, "Error loading ptex file (see stdout for now, TODO)");
		return OPERATOR_CANCELLED;
	}

	/* data type */
	ptex_data_type = ptex_texture_data_type(ptex_texture);
	if(ptex_data_type == PTEX_DT_UNSUPPORTED) {
		BKE_report(op->reports, RPT_ERROR, "Ptex format unsupported");
		ptex_texture_release(ptex_texture);
		return OPERATOR_CANCELLED;
	}

	/* data channels */
	totchannel = ptex_texture_num_channels(ptex_texture);
	if(totchannel == 2 || totchannel > 4) {
		BKE_report(op->reports, RPT_ERROR, "Ptex channel count unsupported");
		ptex_texture_release(ptex_texture);
		return OPERATOR_CANCELLED;
	}

	/* check that ptex file matches mesh topology */
	for(i = 0, j = 0; i < me->totface; ++i) {
		MFace *f = &me->mface[i];
		PtexFaceInfoHandle *ptex_face = ptex_texture_get_face_info(ptex_texture, j);
		int subface;

		if(!ptex_face) {
			BKE_report(op->reports, RPT_ERROR, "Ptex/mesh topology mismatch");
			ptex_texture_release(ptex_texture);
			return OPERATOR_CANCELLED;
		}

		subface = ptex_face_info_is_subface(ptex_face);

		if(subface != (f->v4 == 0)) {
			BKE_report(op->reports, RPT_ERROR, "Ptex/mesh topology mismatch");
			ptex_texture_release(ptex_texture);
			return OPERATOR_CANCELLED;
		}

		j += (f->v4 ? 1 : 3);
	}

	/* number of bytes for one ptex element */
	layersize = ptex_data_size(ptex_data_type) * totchannel;

	mptex = CustomData_add_layer(&me->fdata, CD_MPTEX, CD_CALLOC,
				     NULL, me->totface);

	for(i = 0, j = 0; i < me->totface; ++i) {
		int S = me->mface[i].v4 ? 4 : 3;
		int k, texels, faceid;
		char *data;

		mptex[i].type = ptex_data_type;
		mptex[i].channels = totchannel;

		mptex[i].subfaces = (S==4? 1 : S);

		/* get quad resolution or per-subface resolutions */
		for(k = 0, texels = 0; k < mptex[i].subfaces; ++k) {
			PtexFaceInfoHandle *ptex_face;
			PtexResHandle *ptex_res;

			faceid = j+k;

			ptex_face = ptex_texture_get_face_info(ptex_texture, faceid);
			ptex_res = ptex_face_get_res(ptex_face);
			
			mptex[i].res[k][0] = ptex_res_u(ptex_res);
			mptex[i].res[k][1] = ptex_res_v(ptex_res);

			texels += mptex[i].res[k][0] * mptex[i].res[k][1];
		}

		data = mptex[i].data = MEM_callocN(layersize * texels, "Ptex data from file");

		for(k = 0; k < mptex[i].subfaces; ++k, ++j) {
			PtexFaceInfoHandle *ptex_face;
			PtexResHandle *ptex_res;

			ptex_face = ptex_texture_get_face_info(ptex_texture, j);
			ptex_res = ptex_face_get_res(ptex_face);

			ptex_texture_get_data(ptex_texture, j, data, 0, ptex_res);

			data += layersize * mptex[i].res[k][0] * mptex[i].res[k][1];
		}
	}

	/* data is all copied, can release ptex file */
	ptex_texture_release(ptex_texture);
	
	return OPERATOR_FINISHED;
}

static int ptex_open_invoke(bContext *C, wmOperator *op, wmEvent *event)
{
	WM_event_add_fileselect(C, op);
	return OPERATOR_RUNNING_MODAL;
}

void PTEX_OT_open(wmOperatorType *ot)
{
	/* identifiers */
	ot->name= "Open";
	ot->idname= "PTEX_OT_open";
	
	/* api callbacks */
	ot->exec= ptex_open_exec;
	ot->invoke= ptex_open_invoke;

	/* flags */
	ot->flag= OPTYPE_REGISTER|OPTYPE_UNDO;

	/* properties */
	WM_operator_properties_filesel(ot, 0, FILE_SPECIAL, FILE_OPENFILE, WM_FILESEL_FILEPATH|WM_FILESEL_RELPATH);
}
