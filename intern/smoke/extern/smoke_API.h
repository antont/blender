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
 * The Original Code is Copyright (C) 2009 by Daniel Genrich
 * All rights reserved.
 *
 * Contributor(s): Daniel Genrich
 *
 * ***** END GPL LICENSE BLOCK *****
 */

/** \file smoke/extern/smoke_API.h
 *  \ingroup smoke
 */


#ifndef SMOKE_API_H_
#define SMOKE_API_H_

#ifdef __cplusplus
extern "C" {
#endif

struct FLUID_3D;

// export
void smoke_export(struct FLUID_3D *fluid, float *dt, float *dx, float **dens, float **flame, float **fuel, float **heat, float **heatold,
				  float **vx, float **vy, float **vz, unsigned char **obstacles);

// low res
struct FLUID_3D *smoke_init(int *res, float *p0, float dtdef);
void smoke_free(struct FLUID_3D *fluid);

void smoke_initBlenderRNA(struct FLUID_3D *fluid, float *alpha, float *beta, float *dt_factor, float *vorticity, int *border_colli,
						  float *burning_rate, float *flame_smoke, float *flame_vorticity, float *flame_ignition_temp, float *flame_max_temp);
void smoke_step(struct FLUID_3D *fluid, float dtSubdiv);

float *smoke_get_density(struct FLUID_3D *fluid);
float *smoke_get_flame(struct FLUID_3D *fluid);
float *smoke_get_fuel(struct FLUID_3D *fluid);
float *smoke_get_heat(struct FLUID_3D *fluid);
float *smoke_get_velocity_x(struct FLUID_3D *fluid);
float *smoke_get_velocity_y(struct FLUID_3D *fluid);
float *smoke_get_velocity_z(struct FLUID_3D *fluid);

/* Moving obstacle velocity provided by blender */
void smoke_get_ob_velocity(struct FLUID_3D *fluid, float **x, float **y, float **z);

float *smoke_get_force_x(struct FLUID_3D *fluid);
float *smoke_get_force_y(struct FLUID_3D *fluid);
float *smoke_get_force_z(struct FLUID_3D *fluid);

unsigned char *smoke_get_obstacle(struct FLUID_3D *fluid);

size_t smoke_get_index(int x, int max_x, int y, int max_y, int z);
size_t smoke_get_index2d(int x, int max_x, int y);

void smoke_dissolve(struct FLUID_3D *fluid, int speed, int log);

// wavelet turbulence functions
struct WTURBULENCE *smoke_turbulence_init(int *res, int amplify, int noisetype);
void smoke_turbulence_free(struct WTURBULENCE *wt);
void smoke_turbulence_step(struct WTURBULENCE *wt, struct FLUID_3D *fluid);

float *smoke_turbulence_get_density(struct WTURBULENCE *wt);
float *smoke_turbulence_get_flame(struct WTURBULENCE *wt);
float *smoke_turbulence_get_fuel(struct WTURBULENCE *wt);
void smoke_turbulence_get_res(struct WTURBULENCE *wt, int *res);
void smoke_turbulence_set_noise(struct WTURBULENCE *wt, int type);
void smoke_initWaveletBlenderRNA(struct WTURBULENCE *wt, float *strength);

void smoke_dissolve_wavelet(struct WTURBULENCE *wt, int speed, int log);

// export
void smoke_turbulence_export(struct WTURBULENCE *wt, float **dens, float **flame, float **fuel, float **tcu, float **tcv, float **tcw);

void flame_get_spectrum(unsigned char *spec, int width, float t1, float t2);

#ifdef __cplusplus
}
#endif

#endif /* SMOKE_API_H_ */
