#if defined(USE_SOLID_LIGHTING) || defined(USE_SCENE_LIGHTING)

varying vec3 varying_normal;

#ifndef USE_SOLID_LIGHTING
varying vec3 varying_position;
#endif

#endif

varying vec4 varying_vertex_color;

#ifdef USE_TEXTURE
varying vec2 varying_texture_coord;
#endif

void main()
{
	vec4 co = b_ModelViewMatrix * b_Vertex;

#if defined(USE_SOLID_LIGHTING) || defined(USE_SCENE_LIGHTING)
	varying_normal = normalize(b_NormalMatrix * b_Normal);

#ifndef USE_SOLID_LIGHTING
	varying_position = co.xyz;
#endif
#endif

	gl_Position = b_ProjectionMatrix * co;

// XXX jwilkins: gl_ClipVertex is deprecated
//#ifdef GPU_NVIDIA 
//	// Setting gl_ClipVertex is necessary to get glClipPlane working on NVIDIA
//	// graphic cards, while on ATI it can cause a software fallback.
//	gl_ClipVertex = b_ModelViewMatrix * b_Vertex; 
//#endif 

#ifdef USE_COLOR
	varying_vertex_color = b_Color;
#endif

#ifdef USE_TEXTURE
	varying_texture_coord = (b_TextureMatrix[0] * b_MultiTexCoord0).st;
#endif
}
