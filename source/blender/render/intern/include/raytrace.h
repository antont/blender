
#ifndef __RENDER_RAYTRACE_H__
#define __RENDER_RAYTRACE_H__

struct LampRen;
struct ObjectInstanceRen;
struct RayObject;
struct Render;
struct RenderDB;
struct ShadeInput;
struct ShadeResult;
struct World;

#include "result.h"

/* RayTree Create/Free */

void raytree_create(struct Render *re);
void raytree_free(struct RenderDB *rdb);

struct RayObject* raytree_create_object(struct Render *re, struct ObjectInstanceRen *obi);

/* Raytraced Shading */

void ray_shadow_single(float lashdw[3],
	struct Render *re, struct ShadeInput *shi, struct LampRen *lar,
	float from[3], float to[3]);

void ray_trace_specular(struct Render *re, struct ShadeInput *shi,
	struct ShadeResult *shr);

void ray_ao_env_indirect(struct Render *re, struct ShadeInput *shi,
	float *ao, float env[3], float indirect[3],
	float dir_ao[3], float dir_env[3][3], float dir_indirect[3][3],
	float *Rmean, int for_cache);

/* Irradiance Cache Helper */

void ray_cache_post_apply(struct Render *re, struct ShadeInput *shi,
	float *ao, float env[3], float indirect[3],
	float dir_ao[3], float dir_env[3][3], float dir_indirect[3][3]);

#endif /* __RENDER_RAYTRACE_H__ */

