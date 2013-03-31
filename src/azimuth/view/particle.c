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

#include "azimuth/view/particle.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>

#include <GL/gl.h>

#include "azimuth/state/particle.h"
#include "azimuth/state/space.h"
#include "azimuth/util/misc.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

static void with_color_alpha(az_color_t color, double alpha_factor) {
  glColor4ub(color.r, color.g, color.b, color.a * alpha_factor);
}

static void draw_particle(const az_particle_t *particle) {
  assert(particle->age <= particle->lifetime);
  switch (particle->kind) {
    case AZ_PAR_NOTHING: AZ_ASSERT_UNREACHABLE();
    case AZ_PAR_BOOM:
      glBegin(GL_TRIANGLE_FAN); {
        with_color_alpha(particle->color, 0);
        glVertex2d(0, 0);
        const double ratio = particle->age / particle->lifetime;
        with_color_alpha(particle->color, 1 - ratio * ratio);
        const double radius = particle->param1 * ratio;
        for (int i = 0; i <= 16; ++i) {
          glVertex2d(radius * cos(i * AZ_PI_EIGHTHS),
                     radius * sin(i * AZ_PI_EIGHTHS));
        }
      } glEnd();
      break;
    case AZ_PAR_BEAM:
      glBegin(GL_QUAD_STRIP); {
        const double alpha = (particle->lifetime <= 0.0 ? 1.0 :
                              1.0 - particle->age / particle->lifetime);
        with_color_alpha(particle->color, 0);
        glVertex2d(0, particle->param2);
        glVertex2d(particle->param1, particle->param2);
        with_color_alpha(particle->color, alpha);
        glVertex2d(0, 0);
        glVertex2d(particle->param1, 0);
        with_color_alpha(particle->color, 0);
        glVertex2d(0, -particle->param2);
        glVertex2d(particle->param1, -particle->param2);
      } glEnd();
      break;
    case AZ_PAR_EMBER:
      glBegin(GL_TRIANGLE_FAN); {
        with_color_alpha(particle->color, 1);
        glVertex2f(0, 0);
        with_color_alpha(particle->color, 0);
        const double radius =
          particle->param1 * (1.0 - particle->age / particle->lifetime);
        for (int i = 0; i <= 360; i += 30) {
          glVertex2d(radius * cos(AZ_DEG2RAD(i)), radius * sin(AZ_DEG2RAD(i)));
        }
      } glEnd();
      break;
    case AZ_PAR_NPS_PORTAL:
      glBegin(GL_TRIANGLE_FAN); {
        with_color_alpha(particle->color, 1);
        glVertex2f(0, 0);
        with_color_alpha(particle->color, 0.5);
        const double radius = particle->param1 *
          (1.0 - 2.0 * fabs((particle->age / particle->lifetime) - 0.5));
        for (int i = 0; i <= 360; i += 5) {
          glVertex2d(radius * cos(AZ_DEG2RAD(i)), radius * sin(AZ_DEG2RAD(i)));
        }
      } glEnd();
      break;
    case AZ_PAR_SHARD:
      glScaled(particle->param1, particle->param1, 1);
      glRotated(particle->age * AZ_RAD2DEG(particle->param2), 0, 0, 1);
      glBegin(GL_TRIANGLES); {
        az_color_t color = particle->color;
        const double alpha = 1.0 - particle->age / particle->lifetime;
        with_color_alpha(color, alpha);
        glVertex2f(2, 3);
        color.r *= 0.6; color.g *= 0.6; color.b *= 0.6;
        with_color_alpha(color, alpha);
        glVertex2f(-2, 4);
        color.r *= 0.6; color.g *= 0.6; color.b *= 0.6;
        with_color_alpha(color, alpha);
        glVertex2f(0, -4);
      } glEnd();
      break;
    case AZ_PAR_SPLOOSH:
      glBegin(GL_TRIANGLE_FAN); {
        with_color_alpha(particle->color, 1);
        glVertex2f(0, 0);
        with_color_alpha(particle->color, 0);
        const GLfloat height =
          particle->param1 * sin(AZ_PI * particle->age / particle->lifetime);
        glVertex2f(0, 14);
        glVertex2f(0.3f * height, 7);
        glVertex2f(height, 0);
        glVertex2f(0.3f * height, -7);
        glVertex2f(0, -14);
        glVertex2f(-0.05f * height, 0);
        glVertex2f(0, 14);
      } glEnd();
      break;
  }
}

void az_draw_particles(const az_space_state_t* state) {
  AZ_ARRAY_LOOP(particle, state->particles) {
    if (particle->kind == AZ_PAR_NOTHING) continue;
    glPushMatrix(); {
      glTranslated(particle->position.x, particle->position.y, 0);
      glRotated(AZ_RAD2DEG(particle->angle), 0, 0, 1);
      draw_particle(particle);
    } glPopMatrix();
  }
}

/*===========================================================================*/
