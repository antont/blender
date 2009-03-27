/* $Id$
-----------------------------------------------------------------------------

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the Free Software
Foundation; either version 2 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along with
this program; if not, write to the Free Software Foundation, Inc., 59 Temple
Place - Suite 330, Boston, MA 02111-1307, USA, or go to
http://www.gnu.org/copyleft/lesser.txt.

Contributor(s): Dalai Felinto

This source uses some of the ideas and code from Paul Bourke.
Developed as part of a Research and Development project for SAT - La Soci�t� des arts technologiques.
-----------------------------------------------------------------------------
*/

#if !defined KX_DOME_H
#define KX_DOME_H

#include "KX_Scene.h"
#include "KX_Camera.h"
#include "DNA_screen_types.h"
#include "RAS_ICanvas.h"
#include "RAS_IRasterizer.h"
#include "RAS_IRenderTools.h"
#include "KX_KetsjiEngine.h"

#include <BIF_gl.h>
#include <vector>

//Dome modes: limit hardcoded in buttons_scene.c
#define DOME_FISHEYE		1
#define DOME_TRUNCATED		2
#define DOME_PANORAM_SPH	3
#define DOME_WARPED			4
#define DOME_OFFSET			5
#define DOME_NUM_MODES		6


/// class for render 3d scene
class KX_Dome
{
public:
	/// constructor
	KX_Dome (
	RAS_ICanvas* m_canvas,
    /// rasterizer
    RAS_IRasterizer* m_rasterizer,
    /// render tools
    RAS_IRenderTools* m_rendertools,
    /// engine
    KX_KetsjiEngine* m_engine,
	
	float size,
	short res,
	short mode,
	short angle,
	float resbuf
	);

	/// destructor
	virtual ~KX_Dome (void);

	//openGL checks:
	bool	dlistSupported;

	//openGL names:
	GLuint domefacesId[6];		// ID of the images -- room for 6 images, using only 4 for 180� x 360� dome
	GLuint dlistId;				// ID of the Display Lists of the images (used as an offset)
	
	typedef struct {
		double u[3], v[3];
		MT_Vector3 verts[3]; //three verts
	} DomeFace;

	vector <DomeFace> cubetop, cubebottom, cuberight, cubeleft, cubefront, cubeback; //for dome
	vector <DomeFace> cubeleftback, cuberightback; //for panorama
	
	int nfacestop, nfacesbottom, nfacesleft, nfacesright, nfacesfront, nfacesback;
	int nfacesleftback, nfacesrightback;

	int GetNumberRenders(){return m_numfaces;};

	void RenderDome(void);
	void RenderDomeFrame(KX_Scene* scene, KX_Camera* cam, int i);
	void BindImages(int i);
//	void clearBuffer(int i);

	void SetViewPort(GLuint viewport[4]);
	void CalculateFrustum(KX_Camera* m_camera);
	void RotateCamera(KX_Camera* m_camera, int i);

	//Mesh  Creating Functions
	void CreateMeshDome180(void);
	void CreateMeshDome250(void);
	void CreateMeshPanorama(void);

	void SplitFace(vector <DomeFace>& face, int *nfaces);

	void FlattenDome(MT_Vector3 verts[3]);
	void FlattenPanorama(MT_Vector3 verts[3]);

	//Draw functions
	void GLDrawTriangles(vector <DomeFace>& face, int nfaces);
	void Draw(void);

	void DrawDomeFisheye(void);
	void DrawPanorama(void);
	void DrawDomeTmp(void); //XXX
	//	void DrawDomeWarped(void); //not using yet

	//setting up openGL
	void CreateGLImages(void);
	void ClearGLImages(void);//called on resize
	void CreateDL(void); //create Display Lists
	void ClearDL(void);  //remove Display Lists 

	void CalculateCameraOrientation();
	void CalculateImageSize(); //set m_imagesize

	int canvaswidth;
	int canvasheight;
	int m_buffersize;

protected:
	int m_drawingmode;

	int m_imagesize;
	int m_numfaces;		// 4 to 6 depending on the kind of dome image
	
	float m_size;		// size to adjust
	short m_resolution;	//resolution to tesselate the mesh
	short m_mode;		// the mode (truncated, warped, panoramic,...)
	short m_angle;		//the angle of the fisheye
	float m_resbuffer;	//the resolution of the buffer
	
	RAS_Rect m_viewport;
	
	/// rendered scene
	KX_Scene * m_scene;
	/// camera for render
	KX_Camera * m_camera;

	float m_clip;		// clipping distance
    /// canvas
    RAS_ICanvas* m_canvas;
    /// rasterizer
    RAS_IRasterizer* m_rasterizer;
    /// render tools
    RAS_IRenderTools* m_rendertools;
    /// engine
    KX_KetsjiEngine* m_engine;

	MT_Matrix4x4 m_projmat; // this is nice

	MT_Matrix3x3 m_locRot [6];//the rotation matrix - room for 6 elements, using only 4 sometimes
};

#endif