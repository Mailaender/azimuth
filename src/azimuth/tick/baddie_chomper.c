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

#include "azimuth/tick/baddie_chomper.h"

#include <assert.h>
#include <math.h>

#include "azimuth/state/baddie.h"
#include "azimuth/state/space.h"
#include "azimuth/tick/baddie_util.h"
#include "azimuth/tick/object.h"
#include "azimuth/util/audio.h"
#include "azimuth/util/bezier.h"
#include "azimuth/util/random.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

// Arrange the stalk segments along a cubic bezier curve from the head to the
// base.
static void realign_segments(
    az_baddie_t *baddie, int start_index, double scale,
    az_vector_t head_pos, double head_angle,
    az_vector_t base_pos, double base_angle) {
  const az_vector_t ctrl1 =
    az_vsub(head_pos, az_vpolar(60 * scale, head_angle));
  const az_vector_t ctrl2 =
    az_vadd(base_pos, az_vpolar(50 * scale, base_angle));
  const int num_segments = baddie->data->num_components - start_index;
  for (int i = 0; i < num_segments; ++i) {
    az_component_t *segment = &baddie->components[i + start_index];
    const double t = (0.5 + i) / (double)(num_segments + 1);
    segment->position = az_vrotate(az_vsub(
        az_cubic_bezier_point(head_pos, ctrl1, ctrl2, base_pos, t),
        baddie->position), -baddie->angle);
    segment->angle = az_mod2pi(
        az_cubic_bezier_angle(head_pos, ctrl1, ctrl2, base_pos, t) -
        baddie->angle);
  }
}

static void update_head_and_base(
    az_baddie_t *baddie, az_vector_t head_pos, double head_angle,
    az_vector_t base_pos, double base_angle) {
  // The baddie is centered on the head, so move the baddie to where we
  // want the head to be.
  baddie->position = head_pos;
  baddie->angle = head_angle;
  // Now that the head has moved, move the base component so that it
  // stays in the same absolute position as before.
  az_component_t *base = &baddie->components[0];
  base->position = az_vrotate(az_vsub(base_pos, baddie->position),
                              -baddie->angle);
  base->angle = az_mod2pi(base_angle - baddie->angle);
}

static void move_head(
    az_baddie_t *baddie, double time, double scale,
    az_vector_t head_pos, double head_angle,
    az_vector_t base_pos, double base_angle, bool open_jaws) {
  update_head_and_base(baddie, head_pos, head_angle, base_pos, base_angle);
  // Arrange the stalk segments along a cubic bezier curve from the head
  // to the base.
  realign_segments(baddie, 3, scale,
                   head_pos, head_angle, base_pos, base_angle);
  // Open/close the jaws.
  const double max_angle = AZ_DEG2RAD(55);
  const double old_angle = baddie->components[1].angle;
  const double new_angle =
    (open_jaws ? fmin(max_angle, old_angle + 2.0 * time) :
     fmax(0.0, old_angle - 5.0 * time));
  baddie->components[1].angle = new_angle;
  baddie->components[2].angle = -new_angle;
}

/*===========================================================================*/

void az_tick_bad_chomper_plant(
    az_space_state_t *state, az_baddie_t *baddie, double time) {
  assert(baddie->kind == AZ_BAD_CHOMPER_PLANT);
  az_component_t *base = &baddie->components[0];
  // Get the absolute position of the base.
  const az_vector_t base_pos =
    az_vadd(baddie->position, az_vrotate(base->position, baddie->angle));
  const double base_angle = az_mod2pi(baddie->angle + base->angle);
  // Pick a new position for the head.
  const bool line_of_sight = az_can_see_ship(state, baddie);
  const az_vector_t ship_pos = state->ship.position;
  const double head_angle = az_angle_towards(baddie->angle,
      (line_of_sight ? AZ_DEG2RAD(120) : AZ_DEG2RAD(60)) * time,
      (!line_of_sight ? base_angle :
       az_vtheta(az_vsub(ship_pos, baddie->position))));
  az_vector_t head_pos = baddie->position;
  // State 0: Move head in a circle.
  if (baddie->state == 0) {
    baddie->param += time * 400 / fmax(10, az_vdist(head_pos, ship_pos));
    const az_vector_t goal =
      az_vadd(az_vadd(base_pos, az_vpolar(140, base_angle)),
              az_vpolar(40, baddie->param));
    const double tracking_base = 0.1; // smaller = faster tracking
    const az_vector_t delta =
      az_vmul(az_vsub(goal, head_pos), 1.0 - pow(tracking_base, time));
    az_vpluseq(&head_pos, delta);
    if (baddie->cooldown <= 0.0 && line_of_sight &&
        az_vwithin(ship_pos, head_pos, 150)) {
      baddie->cooldown = az_random(0.35, 0.8);
      baddie->state = 1;
    }
  }
  // State 1: Rear back, then lunge at ship.
  else {
    if (!az_vwithin(head_pos, base_pos, 180) ||
        (line_of_sight && az_vwithin(head_pos, ship_pos, 20))) {
      baddie->cooldown = 1.5;
      baddie->state = 0;
    } else if (baddie->cooldown > 0) {
      const az_vector_t goal =
        az_vadd(base_pos, az_vpolar(50, base_angle));
      const double tracking_base = 0.001; // smaller = faster tracking
      const az_vector_t delta =
        az_vmul(az_vsub(goal, head_pos), 1.0 - pow(tracking_base, time));
      az_vpluseq(&head_pos, delta);
    } else {
      az_vpluseq(&head_pos, az_vpolar(600 * time, head_angle));
    }
  }
  // Update the baddie's position and components.
  move_head(baddie, time, 1.0, head_pos, head_angle, base_pos, base_angle,
            !(baddie->state == 0 ||
              (baddie->state == 1 && line_of_sight &&
               az_vwithin(head_pos, ship_pos, 50))));
}

void az_tick_bad_aquatic_chomper(
    az_space_state_t *state, az_baddie_t *baddie, double time) {
  assert(baddie->kind == AZ_BAD_AQUATIC_CHOMPER);
  az_component_t *base = &baddie->components[0];
  // Get the absolute position of the base.
  const az_vector_t base_pos =
    az_vadd(baddie->position, az_vrotate(base->position, baddie->angle));
  const double base_angle = az_mod2pi(baddie->angle + base->angle);
  // Pick a new position for the head.
  const bool line_of_sight = az_can_see_ship(state, baddie);
  const az_vector_t ship_pos = state->ship.position;
  double head_angle_goal = baddie->angle;
  az_vector_t head_pos = baddie->position;
  az_vector_t head_pos_goal = head_pos;
  if (line_of_sight) {
    head_pos_goal = az_vadd(az_vadd(base_pos, az_vpolar(60, base_angle)),
                            az_vwithlen(az_vsub(ship_pos, base_pos), 20));
    head_angle_goal = az_vtheta(az_vsub(ship_pos, head_pos));
    if (baddie->cooldown <= 0.0 &&
        baddie->components[1].angle >= AZ_DEG2RAD(35) &&
        az_ship_in_range(state, baddie, 400) &&
        az_ship_within_angle(state, baddie, 0.0, AZ_DEG2RAD(15))) {
      for (int i = -1; i <= 1; ++i) {
        az_fire_baddie_projectile(state, baddie,
            AZ_PROJ_FIREBALL_SLOW, baddie->data->main_body.bounding_radius,
            AZ_DEG2RAD(i * 5), 0.0);
      }
      az_play_sound(&state->soundboard, AZ_SND_FIRE_FIREBALL);
      baddie->cooldown = az_random(2.0, 4.0);
    }
  } else {
    // Sway head side to side.
    baddie->param += time * 300 / az_vdist(head_pos, ship_pos);
    head_pos_goal = az_vadd(base_pos, az_vpolar(
        80, base_angle + AZ_DEG2RAD(10) * sin(baddie->param)));
    head_angle_goal = az_vtheta(az_vsub(head_pos_goal, base_pos));
    baddie->cooldown = fmax(1.0, baddie->cooldown);
  }
  const double tracking_base = 0.1; // smaller = faster tracking
  const az_vector_t delta =
    az_vmul(az_vsub(head_pos_goal, head_pos), 1.0 - pow(tracking_base, time));
  az_vpluseq(&head_pos, delta);
  // Update the baddie's position and components.
  const double head_angle = az_angle_towards(baddie->angle,
      (line_of_sight ? AZ_DEG2RAD(120) : AZ_DEG2RAD(60)) * time,
      head_angle_goal);
  move_head(baddie, time, 0.5, head_pos, head_angle, base_pos, base_angle,
            (line_of_sight && baddie->cooldown < 0.75));
}

void az_tick_bad_jungle_chomper(
    az_space_state_t *state, az_baddie_t *baddie, double time) {
  assert(baddie->kind == AZ_BAD_JUNGLE_CHOMPER);
  az_component_t *base = &baddie->components[0];
  // Get the absolute position of the base.
  const az_vector_t base_pos =
    az_vadd(baddie->position, az_vrotate(base->position, baddie->angle));
  const double base_angle = az_mod2pi(baddie->angle + base->angle);
  // Pick a new position for the head.
  const bool line_of_sight = az_can_see_ship(state, baddie);
  const az_vector_t ship_pos = state->ship.position;
  const double head_angle = az_angle_towards(baddie->angle,
      (line_of_sight ? AZ_DEG2RAD(120) : AZ_DEG2RAD(60)) * time,
      (!line_of_sight ? base_angle :
       az_vtheta(az_vsub(ship_pos, baddie->position))));
  az_vector_t head_pos = baddie->position;
  // State 0: Move head in a circle.
  if (baddie->state <= 0) {
    baddie->param += time * 400 / fmax(10, az_vdist(head_pos, ship_pos));
    const az_vector_t goal =
      az_vadd(az_vadd(base_pos, az_vpolar(140, base_angle)),
              az_vpolar(40, baddie->param));
    const double tracking_base = 0.1; // smaller = faster tracking
    const az_vector_t delta =
      az_vmul(az_vsub(goal, head_pos), 1.0 - pow(tracking_base, time));
    az_vpluseq(&head_pos, delta);
    if (baddie->cooldown <= 0.0 && line_of_sight &&
        az_ship_in_range(state, baddie, 300)) {
      baddie->cooldown = 1.0;
      baddie->state = 12;
    }
  }
  // State 1-12: Fire a burst of fireballs.
  else {
    baddie->param += time * 600 / fmax(10, az_vdist(head_pos, ship_pos));
    const az_vector_t goal =
      az_vadd(az_vadd(base_pos, az_vpolar(130, base_angle)),
              az_vpolar(15, baddie->param));
    const double tracking_base = 0.001; // smaller = faster tracking
    const az_vector_t delta =
      az_vmul(az_vsub(goal, head_pos), 1.0 - pow(tracking_base, time));
    az_vpluseq(&head_pos, delta);
    if (baddie->cooldown <= 0.0) {
      az_fire_baddie_projectile(state, baddie,
          AZ_PROJ_FIREBALL_FAST, baddie->data->main_body.bounding_radius,
          0.0, az_random(-1, 1) * AZ_DEG2RAD(10));
      az_play_sound(&state->soundboard, AZ_SND_FIRE_FIREBALL);
      az_vpluseq(&head_pos, az_vpolar(-10, head_angle));
      if (line_of_sight) --baddie->state;
      else baddie->state = 0;
      baddie->cooldown = (baddie->state == 0 ? 2.0 : az_random(0.1, 0.15));
    }
  }
  // Update the baddie's position and components.
  move_head(baddie, time, 1.0, head_pos, head_angle, base_pos, base_angle,
            (baddie->state > 0));
}

void az_tick_bad_fire_chomper(
    az_space_state_t *state, az_baddie_t *baddie, double time) {
  assert(baddie->kind == AZ_BAD_FIRE_CHOMPER);
  az_component_t *base = &baddie->components[0];
  // Get the absolute position of the base.
  const az_vector_t base_pos =
    az_vadd(baddie->position, az_vrotate(base->position, baddie->angle));
  const double base_angle = az_mod2pi(baddie->angle + base->angle);
  // Pick a new position for the head.
  const bool line_of_sight = az_can_see_ship(state, baddie);
  const az_vector_t ship_pos = state->ship.position;
  double head_angle_goal = baddie->angle;
  az_vector_t head_pos = baddie->position;
  az_vector_t head_pos_goal = head_pos;
  if (line_of_sight) {
    head_pos_goal = az_vadd(az_vadd(base_pos, az_vpolar(60, base_angle)),
                            az_vwithlen(az_vsub(ship_pos, base_pos), 20));
    head_angle_goal = az_vtheta(az_vsub(ship_pos, head_pos));
    if (baddie->cooldown <= 0.0 &&
        baddie->components[1].angle >= AZ_DEG2RAD(35) &&
        az_ship_in_range(state, baddie, 400) &&
        az_ship_within_angle(state, baddie, 0.0, AZ_DEG2RAD(15))) {
      az_fire_baddie_projectile(state, baddie, AZ_PROJ_TRINE_TORPEDO_FIREBALL,
          baddie->data->main_body.bounding_radius, 0.0, 0.0);
      az_play_sound(&state->soundboard, AZ_SND_FIRE_ROCKET);
      az_vpluseq(&head_pos, az_vpolar(-10, baddie->angle));
      baddie->cooldown = az_random(2.0, 4.0);
    }
  } else {
    // Sway head side to side.
    baddie->param += time * 300 / az_vdist(head_pos, ship_pos);
    head_pos_goal = az_vadd(base_pos, az_vpolar(
        80, base_angle + AZ_DEG2RAD(10) * sin(baddie->param)));
    head_angle_goal = az_vtheta(az_vsub(head_pos_goal, base_pos));
    baddie->cooldown = fmax(1.0, baddie->cooldown);
  }
  const double tracking_base = 0.1; // smaller = faster tracking
  const az_vector_t delta =
    az_vmul(az_vsub(head_pos_goal, head_pos), 1.0 - pow(tracking_base, time));
  az_vpluseq(&head_pos, delta);
  // Update the baddie's position and components.
  const double head_angle = az_angle_towards(baddie->angle,
      (line_of_sight ? AZ_DEG2RAD(120) : AZ_DEG2RAD(60)) * time,
      head_angle_goal);
  move_head(baddie, time, 0.5, head_pos, head_angle, base_pos, base_angle,
            (line_of_sight && baddie->cooldown < 0.75));
}

void az_tick_bad_spiked_vine(
    az_space_state_t *state, az_baddie_t *baddie, double time) {
  assert(baddie->kind == AZ_BAD_SPIKED_VINE);
  // Grow longer over time.
  const double max_length_per_segment = 23.0;
  const double time_to_fully_extend = 14.0;  // seconds
  baddie->param = fmax(0.7, baddie->param + time);
  const double progress = fmin(1.0, baddie->param / time_to_fully_extend);
  const double length_per_segment =
    max_length_per_segment * (0.5 + 0.5 * progress);
  const double length_limit = 11.4 * max_length_per_segment * progress;
  // Get the absolute position of the base.
  az_component_t *base = &baddie->components[0];
  const az_vector_t base_pos =
    az_vadd(baddie->position, az_vrotate(base->position, baddie->angle));
  const double base_angle = az_mod2pi(baddie->angle + base->angle);
  // Pick a new position for the tip.
  const double ship_theta_offset =
    (az_ship_is_decloaked(&state->ship) &&
     az_vwithin(base_pos, state->ship.position, 1.1 * length_limit) ?
     fmin(fmax(az_mod2pi(az_vtheta(az_vsub(state->ship.position, base_pos)) -
                         base_angle), -AZ_DEG2RAD(20)), AZ_DEG2RAD(20)) : 0.0);
  const double head_angle_goal = base_angle + ship_theta_offset +
    AZ_DEG2RAD(30) * progress * cos(baddie->param * AZ_PI);
  const double head_angle =
    az_angle_towards(baddie->angle, AZ_DEG2RAD(120) * time, head_angle_goal);
  const az_vector_t head_pos_goal = az_vadd(base_pos, az_vrotate(
      az_vpolar(length_limit, base_angle),
      AZ_DEG2RAD(10) * progress *
      sin(baddie->param * AZ_PI) + ship_theta_offset));
  az_vector_t head_pos = baddie->position;
  const double tracking_base = 0.2; // smaller = faster tracking
  const az_vector_t delta =
    az_vmul(az_vsub(head_pos_goal, head_pos), 1.0 - pow(tracking_base, time));
  az_vpluseq(&head_pos, delta);
  // Update the baddie's position and components.
  update_head_and_base(baddie, head_pos, head_angle, base_pos, base_angle);
  const az_vector_t ctrl1 =
    az_vsub(head_pos, az_vpolar(60 * progress, head_angle));
  const az_vector_t ctrl2 =
    az_vadd(base_pos, az_vpolar(100 * progress, base_angle));
  double param = az_cubic_bezier_arc_param(
      head_pos, ctrl1, ctrl2, base_pos, 1000, 0.0, length_per_segment * 0.5);
  for (int i = 1; i < 12; ++i) {
    az_component_t *segment = &baddie->components[i];
    segment->position = az_vrotate(az_vsub(
        az_cubic_bezier_point(head_pos, ctrl1, ctrl2, base_pos, param),
        baddie->position), -baddie->angle);
    segment->angle = az_mod2pi(
        az_cubic_bezier_angle(head_pos, ctrl1, ctrl2, base_pos, param) -
        baddie->angle);
    param = az_cubic_bezier_arc_param(
        head_pos, ctrl1, ctrl2, base_pos, 1000, param, length_per_segment);
  }
}

/*===========================================================================*/
