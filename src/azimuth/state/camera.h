/*=============================================================================
| Copyright 2012 Matthew D. Steele <mdsteele@alum.mit.edu>                    |
|                                                                             |
| This file is part of Azimuth.                                               |
|                                                                             |
| Azimuth is free software: you can redistribute it and/or modify it under    |
| the terms of the GNU General Public License as published by the Free        |
| Software Foundation, either version 3 of the License, or (at your option)   |
| any later version.                                                          |
|                                                                             |
| Azimuth is distributed in the hope that it will be useful, but WITHOUT      |
| ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or       |
| FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for   |
| more details.                                                               |
|                                                                             |
| You should have received a copy of the GNU General Public License along     |
| with Azimuth.  If not, see <http://www.gnu.org/licenses/>.                  |
=============================================================================*/

#pragma once
#ifndef AZIMUTH_STATE_CAMERA_H_
#define AZIMUTH_STATE_CAMERA_H_

#include <stdbool.h>

#include "azimuth/util/clock.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

typedef struct {
  double min_r;
  double r_span;
  double min_theta;
  double theta_span;
} az_camera_bounds_t;

// Return the center point of the camera bounds.
az_vector_t az_bounds_center(const az_camera_bounds_t *bounds);

// Clamp a vector to be within the given camera bounds.
az_vector_t az_clamp_to_bounds(const az_camera_bounds_t *bounds,
                               az_vector_t vec);
az_vector_t az_clamp_to_bounds_with_override(
    const az_camera_bounds_t *bounds, az_vector_t vec, double r_max_override);

// Determine whether a position can be seen based on the given camera bounds.
// This function is conservative in that it may return false for certain
// positions that are actually possible to see in the periphery, but will never
// return true for positions that are impossible to see.
bool az_position_visible(const az_camera_bounds_t *bounds, az_vector_t vec);

/*===========================================================================*/

typedef struct {
  az_vector_t center;
  // Camera shake/quake shakes the screen.  A "shake" dies down over a short
  // time, and use used for e.g. rocket impacts.  A "quake" persists until we
  // turn it off (e.g. via a script) or until we change rooms.
  double shake_horz, shake_vert;
  double quake_vert;
  // Camera wobble is used to wobble the screen for NPS portals.
  double wobble_intensity;
  double wobble_goal;
  double wobble_theta;
  // Effective camera bounds r_max is the greater of the actual r_max and this:
  double r_max_override;
} az_camera_t;

az_vector_t az_camera_shake_offset(const az_camera_t *camera,
                                   az_clock_t clock);

void az_track_camera_towards(az_camera_t *camera, az_vector_t goal,
                             double time);

// Apply shake to the camera.
void az_shake_camera(az_camera_t *camera, double horz, double vert);

// Determine if a ray, travelling delta from start, will intersect the
// rectangular view of the camera.
bool az_ray_intersects_camera_rectangle(
    const az_camera_t *camera, az_vector_t start, az_vector_t delta);

/*===========================================================================*/

#endif // AZIMUTH_STATE_CAMERA_H_
