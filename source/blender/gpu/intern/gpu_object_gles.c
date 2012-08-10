 
#ifdef GLES

#include <GLES2/gl2.h>
#include "gpu_object_gles.h"

#include "GPU_functions.h"

#include REAL_GL_MODE
struct GPUGLSL_ES_info *curglslesi = 0;



void gpuVertexPointer_gles(int size, int type, int stride, const void *pointer)
{
	if(curglslesi && (curglslesi->vertexloc!=-1))
	{
		glEnableVertexAttribArray(curglslesi->vertexloc);
		glVertexAttribPointer(curglslesi->vertexloc, size, type, 0, stride, pointer);
	}
}

void gpuNormalPointer_gles(int type, int stride, const void *pointer)
{

	if(curglslesi && (curglslesi->normalloc!=-1))
	{
		glEnableVertexAttribArray(curglslesi->normalloc);
		glVertexAttribPointer(curglslesi->normalloc, 3, type, 0, stride, pointer);
	}
}

void gpuColorPointer_gles (int size, int type, int stride, const void *pointer)
{






}

void gpuTexCoordPointer_gles(int size, int type, int stride, const void *pointer)
{



}


void gpuClientActiveTexture_gles(int texture)
{

}

void gpuCleanupAfterDraw_gles(void)
{


}

void gpu_set_shader_es(struct GPUGLSL_ES_info * s, int update)
{
	curglslesi = s;
//	if(update)
//		GPU_matrix_forced_update();
}


void gpu_assign_gles_loc(GPUGLSL_ES_info * glslesinfo, unsigned int program)
{
		glslesinfo->normalmatloc = gpu_glGetUniformLocation(program, "b_NormalMatrix");	
		glslesinfo->viewmatloc = gpu_glGetUniformLocation(program, "b_ModelViewMatrix");	
		glslesinfo->projectionmatloc = gpu_glGetUniformLocation(program, "b_ProjectionMatrix");
		
		glslesinfo->vertexloc = gpu_glGetAttribLocation(program, "b_Vertex");
		glslesinfo->normalloc = gpu_glGetAttribLocation(program, "b_Normal");
}



#endif
