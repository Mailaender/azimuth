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

#include "azimuth/view/ship.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>

#include <GL/gl.h>

#include "azimuth/constants.h"
#include "azimuth/state/node.h"
#include "azimuth/state/pickup.h"
#include "azimuth/state/player.h"
#include "azimuth/state/ship.h"
#include "azimuth/state/space.h"
#include "azimuth/util/misc.h"
#include "azimuth/util/vector.h"
#include "azimuth/view/util.h"

/*===========================================================================*/

void az_draw_ship(az_space_state_t *state) {
  const az_ship_t *ship = &state->ship;
  if (!az_ship_is_alive(ship)) return;

  // Draw the magnet sweep:
  if (az_has_upgrade(&ship->player, AZ_UPG_MAGNET_SWEEP)) {
    AZ_ARRAY_LOOP(pickup, state->pickups) {
      if (pickup->kind == AZ_PUP_NOTHING) continue;
      if (az_vwithin(pickup->position, ship->position,
                     AZ_MAGNET_SWEEP_ATTRACT_RANGE)) {
        glBegin(GL_TRIANGLE_STRIP); {
          const az_vector_t offset = az_vwithlen(az_vrot90ccw(
              az_vsub(pickup->position, ship->position)), 6.0);
          glColor4f(1, 1, 0.5, 0);
          az_gl_vertex(az_vadd(pickup->position, offset));
          glColor4f(1, 1, 0.5, 0.2);
          az_gl_vertex(ship->position);
          glColor4f(1, 1, 0.5, 0.1);
          az_gl_vertex(pickup->position);
          glColor4f(1, 1, 0.5, 0);
          az_gl_vertex(az_vsub(pickup->position, offset));
        } glEnd();
      }
    }
  }

  // Draw the tractor beam:
  if (ship->tractor_beam.active) {
    az_node_t *node;
    if (az_lookup_node(state, ship->tractor_beam.node_uid, &node)) {
      const az_vector_t start = node->position;
      const az_vector_t delta = az_vsub(ship->position, start);
      const double dist = az_vnorm(delta);
      const double thick = 4;
      const float green = az_clock_mod(2, 2, state->clock) == 0 ? 1.0 : 0.0;
      glPushMatrix(); {
        glTranslated(start.x, start.y, 0);
        glRotated(AZ_RAD2DEG(az_vtheta(delta)), 0, 0, 1);
        glBegin(GL_TRIANGLE_FAN); {
          glColor4f(1, green, 1, 0.5);
          glVertex2d(0, 0);
          glColor4f(1, green, 1, 0);
          for (int i = 4; i <= 12; ++i) {
            glVertex2d(thick * cos(i * AZ_PI_EIGHTHS),
                       thick * sin(i * AZ_PI_EIGHTHS));
          }
        } glEnd();
        glBegin(GL_QUAD_STRIP); {
          glColor4f(1, green, 1, 0);
          glVertex2d(0, thick);
          glVertex2d(dist, thick);
          glColor4f(1, green, 1, 0.5);
          glVertex2d(0, 0);
          glVertex2d(dist, 0);
          glColor4f(1, green, 1, 0);
          glVertex2d(0, -thick);
          glVertex2d(dist, -thick);
        } glEnd();
      } glPopMatrix();
    }
  }

  // Draw the milliwave radar:
  if (ship->radar.active_time >= AZ_MILLIWAVE_RADAR_WARMUP_TIME) {
    const double up_angle = az_vtheta(ship->position);
    const int segment =
      az_mod2pi_nonneg(ship->radar.angle + 0.125 * AZ_PI - up_angle) *
      (8.0 / AZ_TWO_PI);
    assert(segment >= 0 && segment < 8);
    glPushMatrix(); {
      az_gl_translated(ship->position);
      az_gl_rotated(up_angle);
      for (int i = 0; i < 8; ++i) {
        const bool lit =
          (i == segment || (!ship->radar.locked_on && i == (segment + 4) % 8));
        glBegin(GL_TRIANGLE_FAN); {
          glColor4f(1, (lit ? 1.0f : 0.0f), 0, (lit ? 0.4f : 0.2f));
          glVertex2f(35, 0);
          glColor4f(1, (lit ? 0.75f : 0.0f), (lit ? 0.5f : 0.0f), 0);
          const GLfloat y = (lit ? -5 : -3);
          glVertex2f(45, 0); glVertex2f(30, y);
          glVertex2f(25, 0); glVertex2f(30, -y); glVertex2f(45, 0);
        } glEnd();
        glRotated(45, 0, 0, 1);
      }
    } glPopMatrix();
  }

  // Draw the ship itself:
  az_draw_ship_body(ship, state->clock);
}

static void draw_cloak_glow(float alpha) {
  glBegin(GL_TRIANGLE_FAN); {
    glColor4f(0.5, 0, 1, 0.75f * alpha);
    glVertex2f(0, 0);
    glColor4f(0.5, 0, 1, 0);
    for (int i = 0; i < AZ_SHIP_POLYGON.num_vertices; ++i) {
      az_gl_vertex(AZ_SHIP_POLYGON.vertices[i]);
    }
    az_gl_vertex(AZ_SHIP_POLYGON.vertices[0]);
  } glEnd();
}

void az_draw_ship_body(const az_ship_t *ship, az_clock_t clock) {
  const az_ordnance_t ordnance = ship->player.ordnance;
  glPushMatrix(); {
    glTranslated(ship->position.x, ship->position.y, 0);
    glRotated(AZ_RAD2DEG(ship->angle), 0, 0, 1);

    if (ship->tractor_cloak.active && az_clock_mod(2, 1, clock) != 0) {
      draw_cloak_glow(1.0);
    } else if (ship->temp_invincibility <= 0.0 ||
               az_clock_mod(4, 1, clock) != 0) {
      // Exhaust:
      if (ship->thrusters != AZ_THRUST_NONE) {
        const float zig = az_clock_zigzag(10, 1, clock);
        // For forward thrusters:
        if (ship->thrusters == AZ_THRUST_FORWARD) {
          // From port engine:
          glBegin(GL_TRIANGLE_STRIP); {
            glColor4f(1, 0.5, 0, 0); // transparent orange
            glVertex2f(-10, 12);
            glColor4f(1, 0.75, 0, 0.9); // orange
            glVertex2f(-10, 9);
            glColor4f(1, 0.5, 0, 0); // transparent orange
            glVertex2f(-20 - zig, 9);
            glVertex2f(-10, 7);
          } glEnd();
          // From starboard engine:
          glBegin(GL_TRIANGLE_STRIP); {
            glColor4f(1, 0.5, 0, 0); // transparent orange
            glVertex2f(-10, -12);
            glColor4f(1, 0.75, 0, 0.9); // orange
            glVertex2f(-10, -9);
            glColor4f(1, 0.5, 0, 0); // transparent orange
            glVertex2f(-20 - zig, -9);
            glVertex2f(-10, -7);
          } glEnd();
        }
        // For reverse thrusters:
        else {
          assert(ship->thrusters == AZ_THRUST_REVERSE);
          // From port engine:
          glBegin(GL_TRIANGLE_STRIP); {
            glColor4f(1, 0.5, 0, 0); // transparent orange
            glVertex2f(6, 12);
            glColor4f(1, 0.75, 0, 0.9); // orange
            glVertex2f(6, 9);
            glColor4f(1, 0.5, 0, 0); // transparent orange
            glVertex2f(16 + zig, 9);
            glVertex2f(6, 7);
          } glEnd();
          // From starboard engine:
          glBegin(GL_TRIANGLE_STRIP); {
            glColor4f(1, 0.5, 0, 0); // transparent orange
            glVertex2f(6, -12);
            glColor4f(1, 0.75, 0, 0.9); // orange
            glVertex2f(6, -9);
            glColor4f(1, 0.5, 0, 0); // transparent orange
            glVertex2f(16 + zig, -9);
            glVertex2f(6, -7);
          } glEnd();
        }
      }
      // Ordnance charge:
      if (ship->ordn_charge > 0.0) {
        assert(ship->ordn_charge <= 1.0);
        glPushMatrix(); {
          glTranslated(ordnance == AZ_ORDN_BOMBS ? -15 : 20, 0, 0);
          const double mid_radius = ship->ordn_charge *
            (4.0 + 0.6 * az_clock_zigzag(10, 1, clock));
          const int offset = 6 * az_clock_mod(60, 1, clock);
          glBegin(GL_TRIANGLE_FAN); {
            if (ship->ordn_charge >= 1.0) glColor4f(1, 1, 0.25, 0.7);
            else glColor4f(1, 0.25, 0.25, 0.7);
            glVertex2d(0, 0);
            glColor4f(1, 1, 1, 0.0);
            for (int i = 0; i <= 8; ++i) {
              const double radius =
                (i % 2 ? 0.5 * mid_radius : 2.0 * mid_radius);
              const double theta = AZ_DEG2RAD(45 * i + offset) - ship->angle;
              glVertex2d(radius * cos(theta), radius * sin(theta));
            }
          } glEnd();
        } glPopMatrix();
      }
      // Gun charge:
      if (ship->gun_charge > 0.0) {
        assert(ship->gun_charge <= 1.0);
        glPushMatrix(); {
          glTranslated(20, 0, 0);
          const double radius = ship->gun_charge *
            (7.0 + 0.3 * az_clock_zigzag(10, 1, clock));
          const int offset = 6 * az_clock_mod(60, 1, clock);
          for (int n = 0; n < 2; ++n) {
            glBegin(GL_TRIANGLE_FAN); {
              if (ship->gun_charge >= 1.0) glColor4f(1, 1, 0.5, 0.4);
              else glColor4f(1, 0.5, 0.5, 0.4);
              glVertex2d(0, 0);
              glColor4f(1, 1, 1, 0.0);
              for (int i = 0; i <= 360; i += 60) {
                const double degrees = (n == 0 ? i + offset : i - offset);
                glVertex2d(radius * cos(AZ_DEG2RAD(degrees)),
                           radius * sin(AZ_DEG2RAD(degrees)));
              }
            } glEnd();
          }
        } glPopMatrix();
      }
      // Engines:
      glBegin(GL_QUADS); {
        // Struts:
        glColor3f(0.25, 0.25, 0.25); // dark gray
        glVertex2f( 1,  9); glVertex2f(-7,  9);
        glVertex2f(-7, -9); glVertex2f( 1, -9);
        // Port engine:
        glVertex2f(-10,  12); glVertex2f(  6,  12);
        glColor3f(0.75, 0.75, 0.75); // light gray
        glVertex2f(  8,   7); glVertex2f(-11,   7);
        // Starboard engine:
        glVertex2f(  8,  -7); glVertex2f(-11,  -7);
        glColor3f(0.25, 0.25, 0.25); // dark gray
        glVertex2f(-10, -12); glVertex2f(  6, -12);
      } glEnd();
      // Main body:
      if (ship->ordn_held) {
        if (ordnance == AZ_ORDN_ROCKETS) {
          glBegin(GL_TRIANGLE_STRIP); {
            glColor3f(0.15, 0.15, 0.15);
            glVertex2f(20,  3); glVertex2f(15,  3);
            glColor3f(0.65, 0.65, 0.65);
            glVertex2f(20,  0); glVertex2f(15,  0);
            glColor3f(0.15, 0.15, 0.15);
            glVertex2f(20, -3); glVertex2f(15, -3);
          } glEnd();
        } else if (ordnance == AZ_ORDN_BOMBS) {
          glBegin(GL_TRIANGLE_FAN); {
            glColor3f(0.5, 0.5, 0.5); glVertex2f(-14,  0);
            glColor3f(0.15, 0.15, 0.15);
            glVertex2f(-14,  3);
            glColor3f(0.15, 0.15, 0.5);
            glVertex2f(-17,  0);
            glColor3f(0.15, 0.15, 0.15);
            glVertex2f(-14, -3);
          } glEnd();
        }
      }
      glBegin(GL_QUAD_STRIP); {
        glColor3f(0.25, 0.25, 0.25); // dark gray
        glVertex2f( 15,  4); glVertex2f(-14,  4);
        glColor3f(0.75, 0.75, 0.75); // light gray
        glVertex2f( 20,  0); glVertex2f(-14,  0);
        glColor3f(0.25, 0.25, 0.25); // dark gray
        glVertex2f( 15, -4); glVertex2f(-14, -4);
      } glEnd();
      // Windshield:
      glBegin(GL_TRIANGLE_STRIP); {
        az_color_t inner = {0, 255, 255, 255};
        az_color_t outer = {0, 128, 128, 255};
        if (ship->ordn_held) {
          if (ordnance == AZ_ORDN_ROCKETS) {
            inner = (az_color_t){255, 0, 128, 255};
            outer = (az_color_t){128, 0, 64, 255};
          } else if (ordnance == AZ_ORDN_BOMBS) {
            inner = (az_color_t){64, 64, 255, 255};
            outer = (az_color_t){32, 32, 128, 255};
          }
        }
        az_gl_color(outer);
        glVertex2f(15,  2);
        az_gl_color(inner);
        glVertex2f(18,  0);
        glVertex2f(12,  0);
        az_gl_color(outer);
        glVertex2f(15, -2);
      } glEnd();
    }

    // Tractor cloak charging glow:
    if (!ship->tractor_cloak.active &&
        ship->tractor_cloak.charge > 0.0) {
      assert(ship->tractor_cloak.charge < 1.0);
      draw_cloak_glow(ship->tractor_cloak.charge);
    }

    // Reactive Armor flare:
    if (ship->reactive_flare > 0.0) {
      assert(ship->reactive_flare <= 1.0);
      glColor4f(1, 0.25, 0, 0.5 * ship->reactive_flare);
      glBegin(GL_POLYGON); {
        for (int i = 0; i < AZ_SHIP_POLYGON.num_vertices; ++i) {
          az_gl_vertex(AZ_SHIP_POLYGON.vertices[i]);
        }
      } glEnd();
    }

    // C-plus blink:
    if (ship->cplus.state == AZ_CPLUS_READY) {
      assert(ship->cplus.charge > 0.0);
      assert(ship->cplus.charge <= 1.0);
      glColor4f(0, 1, 0, (az_clock_mod(2, 3, clock) ? 0.25 : 0.5) *
                (1.0 - pow(ship->cplus.charge, 4.0)));
      glBegin(GL_POLYGON); {
        for (int i = 0; i < AZ_SHIP_POLYGON.num_vertices; ++i) {
          az_gl_vertex(AZ_SHIP_POLYGON.vertices[i]);
        }
      } glEnd();
    }

    // Shield flare:
    if (ship->shield_flare > 0.0) {
      assert(ship->shield_flare <= 1.0);
      glBegin(GL_QUAD_STRIP); {
        const GLfloat alpha = 0.6 * ship->shield_flare;
        const double scale = 1.0 + 0.7 * ship->shield_flare;
        for (int i = AZ_SHIP_POLYGON.num_vertices - 1, j = 0;
             j <= AZ_SHIP_POLYGON.num_vertices; i = j++) {
          glColor4f(0, 1, 1, alpha);
          const az_vector_t vertex = AZ_SHIP_POLYGON.vertices[i];
          az_gl_vertex(vertex);
          glColor4f(0, 1, 1, 0);
          az_gl_vertex(az_vmul(vertex, scale));
        }
      } glEnd();
    }
  } glPopMatrix();
}

/*===========================================================================*/
