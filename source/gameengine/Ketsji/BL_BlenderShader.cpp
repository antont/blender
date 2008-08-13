
#include "DNA_customdata_types.h"
#include "DNA_material_types.h"
#include "DNA_scene_types.h"

#include "BKE_global.h"
#include "BKE_main.h"

#include "BL_BlenderShader.h"
#include "BL_Material.h"

#include "GPU_extensions.h"
#include "GPU_material.h"

#include "RAS_BucketManager.h"
#include "RAS_MeshObject.h"
#include "RAS_IRasterizer.h"
 
 /* this is evil, but we need the scene to create materials with
  * lights from the correct scene .. */
static struct Scene *GetSceneForName(const STR_String& scenename)
{
	Scene *sce;

	for (sce= (Scene*)G.main->scene.first; sce; sce= (Scene*)sce->id.next)
		if (scenename == (sce->id.name+2))
			return sce;

	return (Scene*)G.main->scene.first;
}

bool BL_BlenderShader::Ok()
{
	VerifyShader();

	return (mMat && mMat->gpumaterial);
}

BL_BlenderShader::BL_BlenderShader(KX_Scene *scene, struct Material *ma, int lightlayer)
:
	mScene(scene),
	mMat(ma),
	mGPUMat(NULL),
	mBound(false),
	mLightLayer(lightlayer)
{
	mBlenderScene = GetSceneForName(scene->GetName());
	mBlendMode = GPU_BLEND_SOLID;

	if(mMat) {
		GPU_material_from_blender(mBlenderScene, mMat);
		mGPUMat = mMat->gpumaterial;
	}
}

BL_BlenderShader::~BL_BlenderShader()
{
	if(mMat && mMat->gpumaterial)
		GPU_material_unbind(mMat->gpumaterial);
}

bool BL_BlenderShader::VerifyShader()
{
	if(mMat && !mMat->gpumaterial)
		GPU_material_from_blender(mBlenderScene, mMat);

	mGPUMat = mMat->gpumaterial;
	
	return (mMat && mGPUMat);
}

void BL_BlenderShader::SetProg(bool enable)
{
	if(VerifyShader()) {
		if(enable) {
			GPU_material_bind(mGPUMat, mLightLayer);
			mBound = true;
		}
		else {
			GPU_material_unbind(mGPUMat);
			mBound = false;
		}
	}
}

int BL_BlenderShader::GetAttribNum()
{
	GPUVertexAttribs attribs;
	int i, enabled = 0;

	if(!VerifyShader())
		return enabled;

	GPU_material_vertex_attributes(mGPUMat, &attribs);

    for(i = 0; i < attribs.totlayer; i++)
		if(attribs.layer[i].glindex+1 > enabled)
			enabled= attribs.layer[i].glindex+1;
	
	if(enabled > BL_MAX_ATTRIB)
		enabled = BL_MAX_ATTRIB;

	return enabled;
}

void BL_BlenderShader::SetAttribs(RAS_IRasterizer* ras, const BL_Material *mat)
{
	GPUVertexAttribs attribs;
	int i, attrib_num;

	ras->SetAttribNum(0);

	if(!VerifyShader())
		return;

	if(ras->GetDrawingMode() == RAS_IRasterizer::KX_TEXTURED) {
		GPU_material_vertex_attributes(mGPUMat, &attribs);
		attrib_num = GetAttribNum();

		ras->SetTexCoordNum(0);
		ras->SetAttribNum(attrib_num);
		for(i=0; i<attrib_num; i++)
			ras->SetAttrib(RAS_IRasterizer::RAS_TEXCO_DISABLE, i);

		for(i = 0; i < attribs.totlayer; i++) {
			if(attribs.layer[i].glindex > attrib_num)
				continue;

			if(attribs.layer[i].type == CD_MTFACE) {
				if(!mat->uvName.IsEmpty() && strcmp(mat->uvName.ReadPtr(), attribs.layer[i].name) == 0)
					ras->SetAttrib(RAS_IRasterizer::RAS_TEXCO_UV1, attribs.layer[i].glindex);
				else if(!mat->uv2Name.IsEmpty() && strcmp(mat->uv2Name.ReadPtr(), attribs.layer[i].name) == 0)
					ras->SetAttrib(RAS_IRasterizer::RAS_TEXCO_UV2, attribs.layer[i].glindex);
				else
					ras->SetAttrib(RAS_IRasterizer::RAS_TEXCO_UV1, attribs.layer[i].glindex);
			}
			else if(attribs.layer[i].type == CD_TANGENT)
				ras->SetAttrib(RAS_IRasterizer::RAS_TEXTANGENT, attribs.layer[i].glindex);
			else if(attribs.layer[i].type == CD_ORCO)
				ras->SetAttrib(RAS_IRasterizer::RAS_TEXCO_ORCO, attribs.layer[i].glindex);
			else if(attribs.layer[i].type == CD_NORMAL)
				ras->SetAttrib(RAS_IRasterizer::RAS_TEXCO_NORM, attribs.layer[i].glindex);
			else if(attribs.layer[i].type == CD_MCOL)
				ras->SetAttrib(RAS_IRasterizer::RAS_TEXCO_VCOL, attribs.layer[i].glindex);
			else
				ras->SetAttrib(RAS_IRasterizer::RAS_TEXCO_DISABLE, attribs.layer[i].glindex);
		}
	}
}

void BL_BlenderShader::Update(const RAS_MeshSlot & ms, RAS_IRasterizer* rasty )
{
	float obmat[4][4], viewmat[4][4], viewinvmat[4][4], obcol[4];

	VerifyShader();

	if(!mGPUMat || !mBound)
		return;

	MT_Matrix4x4 model;
	model.setValue(ms.m_OpenGLMatrix);
	const MT_Matrix4x4& view = rasty->GetViewMatrix();
	const MT_Matrix4x4& viewinv = rasty->GetViewInvMatrix();

	// note: getValue gives back column major as needed by OpenGL
	model.getValue((float*)obmat);
	view.getValue((float*)viewmat);
	viewinv.getValue((float*)viewinvmat);

	if(ms.m_bObjectColor)
		ms.m_RGBAcolor.getValue((float*)obcol);
	else
		obcol[0]= obcol[1]= obcol[2]= obcol[3]= 1.0f;

	GPU_material_bind_uniforms(mGPUMat, obmat, viewmat, viewinvmat, obcol);

	mBlendMode = GPU_material_blend_mode(mGPUMat, obcol);
}

int BL_BlenderShader::GetBlendMode()
{
	return mBlendMode;
}

bool BL_BlenderShader::Equals(BL_BlenderShader *blshader)
{
	/* to avoid unneeded state switches */
	return (blshader && mGPUMat == blshader->mGPUMat && mLightLayer == blshader->mLightLayer);
}

// eof
