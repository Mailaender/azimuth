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

#include "azimuth/state/baddie.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h> // for NULL

#include "azimuth/state/pickup.h" // for AZ_PUPF_* macros
#include "azimuth/util/misc.h"
#include "azimuth/util/polygon.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

#define DECL_COMPONENTS(c) .num_components=AZ_ARRAY_SIZE(c), .components=(c)

static const az_vector_t turret_vertices[] = {
  {20, 0}, {10, 17.320508075688775}, {-10, 17.320508075688775},
  {-20, 0}, {-10, -17.320508075688775}, {10, -17.320508075688775}
};
static const az_vector_t turret_cannon_vertices[] = {
  {30, 5}, {0, 5}, {0, -5}, {30, -5}
};
static const az_vector_t double_turret_cannon_vertices[] = {
  {30, 10}, {0, 10}, {0, -10}, {30, -10}, {30, -2}, {0, -2}, {0, 2}, {30, 2}
};
static az_component_data_t turret_components[] = {
  { .polygon = AZ_INIT_POLYGON(turret_cannon_vertices), .impact_damage = 10.0 }
};
static az_component_data_t broken_turret_components[] = {
  { .polygon = AZ_INIT_POLYGON(turret_cannon_vertices), .impact_damage = 5.0 }
};
static az_component_data_t armored_turret_components[] = {
  { .polygon = AZ_INIT_POLYGON(turret_cannon_vertices),
    .immunities = (AZ_DMGF_NORMAL | AZ_DMGF_FLAME), .impact_damage = 10.0 }
};
static az_component_data_t crawling_turret_components[] = {
  { .polygon = AZ_INIT_POLYGON(turret_cannon_vertices), .impact_damage = 10.0 }
};
static az_component_data_t beam_turret_components[] = {
  { .polygon = AZ_INIT_POLYGON(turret_cannon_vertices),
    .immunities = (AZ_DMGF_NORMAL | AZ_DMGF_FLAME),
    .impact_damage = 15.0 }
};
static az_component_data_t rocket_turret_components[] = {
  { .polygon = AZ_INIT_POLYGON(turret_cannon_vertices),
    .immunities = (AZ_DMGF_NORMAL | AZ_DMGF_FLAME), .impact_damage = 15.0 }
};
static az_component_data_t crawling_mortar_components[] = {
  { .polygon = AZ_INIT_POLYGON(turret_cannon_vertices),
    .immunities = AZ_DMGF_FLAME, .impact_damage = 15.0 }
};
static az_component_data_t heavy_turret_components[] = {
  { .polygon = AZ_INIT_POLYGON(double_turret_cannon_vertices),
    .immunities = (AZ_DMGF_NORMAL | AZ_DMGF_FLAME), .impact_damage = 10.0 }
};

static const az_vector_t zipper_vertices[] = {
  {20, 3}, {15, 6}, {10, 7}, {-10, 4}, {-15, 2},
  {-15, -2}, {-10, -4}, {10, -7}, {15, -6}, {20, -3}
};

#define ATOM_ELECTRON_IMMUNITIES \
  (AZ_DMGF_NORMAL | AZ_DMGF_CHARGED | AZ_DMGF_FREEZE | \
   AZ_DMGF_FLAME | AZ_DMGF_BEAM)
static az_component_data_t atom_components[] = {
  { .init_position = {20, 0},
    .bounding_radius = 6.3, .immunities = ATOM_ELECTRON_IMMUNITIES,
    .impact_damage = 15.0 },
  { .init_position = {-1.675, -4.977}, .init_angle = AZ_DEG2RAD(100),
    .bounding_radius = 6.5, .immunities = ATOM_ELECTRON_IMMUNITIES,
    .impact_damage = 15.0 },
  { .init_position = {8.212, 16.96}, .init_angle = AZ_DEG2RAD(200),
    .bounding_radius = 6.7, .immunities = ATOM_ELECTRON_IMMUNITIES,
    .impact_damage = 15.0 }
};
static az_component_data_t red_atom_components[] = {
  { .init_position = {25, 0},
    .bounding_radius = 8, .immunities = ATOM_ELECTRON_IMMUNITIES,
    .impact_damage = 25.0 },
  { .init_position = {-2.09375, -6.22125}, .init_angle = AZ_DEG2RAD(100),
    .bounding_radius = 8, .immunities = ATOM_ELECTRON_IMMUNITIES,
    .impact_damage = 25.0 },
  { .init_position = {10.265, 21.2}, .init_angle = AZ_DEG2RAD(200),
    .bounding_radius = 8, .immunities = ATOM_ELECTRON_IMMUNITIES,
    .impact_damage = 25.0 }
};
#undef ATOM_ELECTRON_IMMUNITIES

static const az_vector_t box_vertices[] = {
  {16, 8}, {11, 13}, {-11, 13}, {-16, 8},
  {-16, -8}, {-11, -13}, {11, -13}, {16, -8}
};

static const az_vector_t clam_shell1_vertices[] = {
  {24, 0}, {27,  2}, {24,  8}, {20,  11}, {9,  14}, {-5, 10}, {-8,  6}, {-9, 0}
};
static const az_vector_t clam_shell2_vertices[] = {
  {24, 0}, {27, -2}, {24, -7}, {19, -10}, {7, -13}, {-6, -9}, {-8, -5}, {-9, 0}
};
static az_component_data_t clam_components[] = {
  { .init_position = {-4, 0}, .polygon = AZ_INIT_POLYGON(clam_shell1_vertices),
    .immunities = (AZ_DMGF_NORMAL | AZ_DMGF_CHARGED), .impact_damage = 10.0 },
  { .init_position = {-4, 0}, .polygon = AZ_INIT_POLYGON(clam_shell2_vertices),
    .immunities = (AZ_DMGF_NORMAL | AZ_DMGF_CHARGED), .impact_damage = 10.0 }
};

static const az_vector_t nightbug_vertices[] = {
  {17, 0}, {9, 10}, {-11, 5}, {-11, -5}, {9, -10}
};

static const az_vector_t zenith_core_main_body_vertices[] = {
  { 90, 0}, { 63.63961030678928,  63.63961030678928},
  {0,  90}, {-63.63961030678928,  63.63961030678928},
  {-90, 0}, {-63.63961030678928, -63.63961030678928},
  {0, -90}, { 63.63961030678928, -63.63961030678928}
};
static const az_vector_t zenith_core_plating_vertices[] = {
  {100, 0}, {70.71067811865476,  70.71067811865476},
  {49.49747468305833, 49.49747468305833}, {70, 0}
};
static az_component_data_t zenith_core_components[] = {
  { .polygon = AZ_INIT_POLYGON(zenith_core_plating_vertices),
    .init_angle = AZ_DEG2RAD(0), .immunities = ~0 },
  { .polygon = AZ_INIT_POLYGON(zenith_core_plating_vertices),
    .init_angle = AZ_DEG2RAD(45), .immunities = ~0 },
  { .polygon = AZ_INIT_POLYGON(zenith_core_plating_vertices),
    .init_angle = AZ_DEG2RAD(90), .immunities = ~0 },
  { .polygon = AZ_INIT_POLYGON(zenith_core_plating_vertices),
    .init_angle = AZ_DEG2RAD(135), .immunities = ~0 },
  { .polygon = AZ_INIT_POLYGON(zenith_core_plating_vertices),
    .init_angle = AZ_DEG2RAD(180), .immunities = ~0 },
  { .polygon = AZ_INIT_POLYGON(zenith_core_plating_vertices),
    .init_angle = AZ_DEG2RAD(225), .immunities = ~0 },
  { .polygon = AZ_INIT_POLYGON(zenith_core_plating_vertices),
    .init_angle = AZ_DEG2RAD(270), .immunities = ~0 },
  { .polygon = AZ_INIT_POLYGON(zenith_core_plating_vertices),
    .init_angle = AZ_DEG2RAD(315), .immunities = ~0 }
};

static const az_vector_t dragonfly_vertices[] = {
  {20, 2}, {10, 5}, {4, 10}, {0, 5}, {-15, 1},
  {-15, -1}, {0, -5}, {4, -10}, {10, -5}, {20, -2}
};

static const az_vector_t cave_crawler_vertices[] = {
  {10, -8}, {10, 8}, {-2, 18}, {-15, 10}, {-15, -10}, {-2, -18}
};

static const az_vector_t beam_sensor_target_vertices[] = {
  {8, -18}, {8, 18}, {0, 18}, {0, -18}
};
static const az_vector_t beam_sensor_casing_vertices[] = {
  {4, -5}, {4, 5}, {10, 17}, {10, 20}, {-15, 20},
  {-15, -20}, {10, -20}, {10, -17}
};
static az_component_data_t beam_sensor_components[] = {
  { .polygon = AZ_INIT_POLYGON(beam_sensor_casing_vertices), .immunities = ~0 }
};
static az_component_data_t beam_sensor_inv_components[] = {
  { .polygon = AZ_INIT_POLYGON(beam_sensor_casing_vertices), .immunities = ~0 }
};

static const az_vector_t rockwyrm_pincer1_vertices[] = {
  {40, -10}, {20, -7}, {0, -12}, {-15, -12}, {-30, -6},
  {-20,  18}, {0,  24}, {18,  20}, {30,  10}
};
static const az_vector_t rockwyrm_pincer2_vertices[] = {
  {40,  10}, {20,  7}, {0,  12}, {-15,  12}, {-30,  6},
  {-20, -18}, {0, -24}, {18, -20}, {30, -10}
};
static az_component_data_t rockwyrm_components[] = {
  { .polygon = AZ_INIT_POLYGON(rockwyrm_pincer1_vertices),
    .init_position = {-5,  10}, .immunities = ~0, .impact_damage = 8.0 },
  { .polygon = AZ_INIT_POLYGON(rockwyrm_pincer2_vertices),
    .init_position = {-5, -10}, .immunities = ~0, .impact_damage = 8.0 },
  { .init_position = { -40, 0}, .bounding_radius = 29.0,
    .immunities = ~0, .impact_damage = 3.0 },
  { .init_position = { -80, 0}, .bounding_radius = 30.0,
    .immunities = ~0, .impact_damage = 3.0 },
  { .init_position = {-120, 0}, .bounding_radius = 30.0,
    .immunities = ~0, .impact_damage = 3.0 },
  { .init_position = {-160, 0}, .bounding_radius = 30.0,
    .immunities = ~0, .impact_damage = 3.0 },
  { .init_position = {-200, 0}, .bounding_radius = 29.0,
    .immunities = ~0, .impact_damage = 3.0 },
  { .init_position = {-240, 0}, .bounding_radius = 28.0,
    .immunities = ~0, .impact_damage = 3.0 },
  { .init_position = {-280, 0}, .bounding_radius = 28.0,
    .immunities = ~0, .impact_damage = 3.0 },
  { .init_position = {-320, 0}, .bounding_radius = 27.0,
    .immunities = ~0, .impact_damage = 3.0 },
  { .init_position = {-360, 0}, .bounding_radius = 26.0,
    .immunities = ~0, .impact_damage = 3.0 },
  { .init_position = {-400, 0}, .bounding_radius = 25.0,
    .immunities = ~0, .impact_damage = 3.0 }
};

static az_component_data_t wyrmling_components[] = {
  { .init_position = {-5, 0}, .init_angle = AZ_DEG2RAD(0),
    .bounding_radius = 4.5, .impact_damage = 2.0 },
  { .init_position = {-5, 5}, .init_angle = AZ_DEG2RAD(-90),
    .bounding_radius = 4.5, .impact_damage = 2.0 },
  { .init_position = { 0, 5}, .init_angle = AZ_DEG2RAD(180),
    .bounding_radius = 4.0, .impact_damage = 2.0 },
  { .init_position = { 5, 5}, .init_angle = AZ_DEG2RAD(180),
    .bounding_radius = 3.5, .impact_damage = 2.0 },
  { .init_position = { 5, 0}, .init_angle = AZ_DEG2RAD(90),
    .bounding_radius = 3.0, .impact_damage = 2.0 }
};

static const az_vector_t trapdoor_hinge_vertices[] = {
  {-10, 8}, {-10, -8}, {0, -8}, {5, -5}, {7, 0}, {5, 5}, {0, 8}
};
static const az_vector_t trapdoor_door_vertices[] = {
  {10, -95}, {10, 10}, {-2, -2}, {4, -89}
};
static az_component_data_t trapdoor_components[] = {
  { .polygon = AZ_INIT_POLYGON(trapdoor_door_vertices),
    .immunities = (AZ_DMGF_NORMAL | AZ_DMGF_CHARGED | AZ_DMGF_PIERCE |
                   AZ_DMGF_ROCKET | AZ_DMGF_FLAME) }
};

static const az_vector_t swooper_vertices[] = {
  {8, 0}, {7, 3}, {0, 12}, {-10, 8}, {-16, 6}, {-14, 0}, {-16, -6},
  {-10, -8}, {0, -12}, {7, -3}
};
static const az_vector_t demon_swooper_tail_segment_vertices[] = {
  {3, -2}, {5, 0}, {3, 2}, {-3, 2}, {-5, 0}, {-3, -2}
};
static const az_vector_t demon_swooper_stinger_vertices[] = {
  {5, -2}, {7, 0}, {5, 2}, {1, 2}, {1, 4}, {-7, 0}, {1, -4}, {1, -2}
};
static az_component_data_t demon_swooper_components[] = {
#define TAIL_SEGMENT(x) \
  { .polygon = AZ_INIT_POLYGON(demon_swooper_tail_segment_vertices), \
    .immunities = AZ_DMGF_FLAME, .impact_damage = 8.0, \
    .init_position = {(x), 0} }

  TAIL_SEGMENT(-17), TAIL_SEGMENT(-23), TAIL_SEGMENT(-29), TAIL_SEGMENT(-35),
  TAIL_SEGMENT(-41),
#undef TAIL_SEGMENT
  { .polygon = AZ_INIT_POLYGON(demon_swooper_stinger_vertices),
    .immunities = AZ_DMGF_FLAME, .impact_damage = 8.0,
    .init_position = {-49, 0} }
};

static const az_vector_t ice_crawler_body_vertices[] = {
  {8, 0}, {-2, 15}, {-16, 10}, {-18, 0}, {-16, -10}, {-2, -15}
};
static const az_vector_t ice_crawler_shell_vertices[] = {
  {17, -4}, {11, 2}, {14, 12}, {6, 14}, {5, 17}, {-5, 18},
  {-14, 12}, {-16, 0}, {-13, -13}, {-2, -18}, {9, -16}, {10, -9}
};
static az_component_data_t ice_crawler_components[] = {
  { .polygon = AZ_INIT_POLYGON(ice_crawler_shell_vertices),
    .immunities = (AZ_DMGF_NORMAL | AZ_DMGF_CHARGED | AZ_DMGF_FREEZE),
    .impact_damage = 15.0 }
};

static const az_vector_t oth_brawler_vertices[] = {
  {0, 20}, {-29, 16}, {-10, 11.33}, {-15, 7}, {-15, -7}, {-10, -11.33},
  {-29, -16}, {0, -20}, {17, -30}, {32, -26}, {19, -22}, {10, -13.33},
  {15, -10}, {15, -8}, {25, -5}, {36, 0}, {25, 5}, {15, 8}, {15, 10},
  {10, 13.33}, {19, 22}, {32, 26}, {17, 30}
};

static const az_vector_t oth_crab_vertices[] = {
  {0, 20}, {-5, 16}, {-25, 22}, {-10, 11}, {-15, 7},
  {-15, -7}, {-10, -11}, {-25, -22}, {-5, -16}, {0, -20},
  {5, -16.66}, {22, -25}, {41, -12}, {24, -17}, {15, -10},
  {15, 10}, {24, 17}, {41, 12}, {22, 25}, {5, 16.66}
};

static const az_vector_t oth_minicrab_vertices[] = {
  {0, 14}, {-15, 7}, {-29, 9}, {-15, 0}, {-29, -9}, {-15, -7}, {0, -14},
  {30, -17}, {15, -9}, {15, 9}, {30, 17}
};

static const az_vector_t gun_sensor_casing_vertices[] = {
  {5, 5}, {10, 17}, {10, 20}, {-20, 20},
  {-20, -20}, {10, -20}, {10, -17}, {5, -5}
};
static az_component_data_t gun_sensor_components[] = {
  { .polygon = AZ_INIT_POLYGON(gun_sensor_casing_vertices), .immunities = ~0 }
};
static az_component_data_t bomb_sensor_components[] = {
  { .polygon = AZ_INIT_POLYGON(gun_sensor_casing_vertices), .immunities = ~0 }
};
static az_component_data_t rocket_sensor_components[] = {
  { .polygon = AZ_INIT_POLYGON(gun_sensor_casing_vertices), .immunities = ~0 }
};

static const az_vector_t security_drone_cannon_vertices[] = {
  {30, 10}, {-10.66, 10}, {-16, 15}, {-24, 0}, {-16, -15}, {-10.66, -10},
  {30, -10}, {30, -2}, {0, -2}, {0, 2}, {30, 2}
};
static az_component_data_t security_drone_components[] = {
  { .polygon = AZ_INIT_POLYGON(security_drone_cannon_vertices),
    .impact_damage = 10.0 }
};

static const az_vector_t small_truck_vertices[] = {
  {32, 12}, {10, 20}, {10, 14}, {-30, 14},
  {-30, -14}, {10, -14}, {10, -20}, {32, -12}
};

static const az_vector_t heat_ray_vertices[] = {
  {0, -13.6}, {1, -13.9}, {3.4, -13.9}, {5.5, -13}, {7.4, -11.5}, {8.5, -9.5},
  {9, -7}, {15, -7}, {15, 7}, {0, 7}, {0, 15}, {-10, 20}, {-10, -20}, {0, -15}
};

static const az_vector_t proxy_mine_vertices[] = {
  {15, -1.5}, {15, 1.5}, {6.837397, 1.5}, {5.649674, 4.132939},
  {3.5, 6.062178}, {0.754394, 6.959231}, {-2.119660, 6.671360},
  {-6.200962, 13.740381}, {-8.799038, 12.240381}, {-4.717737, 5.171360},
  {-6.404067, 2.826291}, {-7, 0}, {-6.404067, -2.826291},
  {-4.717737, -5.171360},
  {-8.799038, -12.240381}, {-6.200962, -13.740381}, {-2.119660, -6.671360},
  {0.754394, -6.959231}, {3.5, -6.062178}, {5.649674, -4.132939},
  {6.837397, -1.5},
};
static const az_vector_t nuclear_mine_vertices[] = {
  {11, -1.5}, {18, -1.5}, {18, 1.5}, {11, 1.5}, {6, 10.3923}, {-4.2, 10.2763},
  {-7.7, 16.3385}, {-10.299, 14.8385}, {-6.79904, 8.77628}, {-12, 0},
  {-6.79904, -8.77628}, {-10.299, -14.8385}, {-7.7, -16.3385},
  {-4.2, -10.2763}, {6, -10.3923}
};

static const az_vector_t beam_wall_vertices[] = {
  {52, 15}, {-52, 15}, {-52, 10}, {-50, 10}, {-50, -10}, {-52, -10},
  {-52, -15}, {52, -15}, {52, -10}, {50, -10}, {50, 10}, {52, 10}
};

static const az_vector_t gnat_vertices[] = {
  {10, 2}, {7.5, 4}, {5, 4.66}, {-5, 2.66}, {-7.5, 1.33},
  {-7.5, -1.33}, {-5, -2.66}, {5, -4.66}, {7.5, -4}, {10, -2}
};

static const az_vector_t forcefiend_left_pincer_vertices[] = {
  {30, -4}, {15, -3}, {0, -5}, {-6, -5}, {-11, -2},
  {-7,  7}, {0,  9}, {14,  8}, {22,  4}
};
static const az_vector_t forcefiend_right_pincer_vertices[] = {
  {30,  4}, {15,  3}, {0,  5}, {-6,  5}, {-11,  2},
  {-7, -7}, {0, -9}, {14, -8}, {22, -4}
};
static const az_vector_t forcefiend_upper_arm_vertices[] = {
  {0, 9}, {0, -9}, {40, -4}, {40, 4}
};
static const az_vector_t forcefiend_lower_arm_vertices[] = {
  {0, 4}, {-2, 3.4641}, {-3.4641, 2}, {-4, 0}, {-3.4641, -2}, {-2, -3.4641},
  {0, -4}, {34, -3}, {36, 0}, {34, 3}
};
static const az_vector_t forcefiend_left_claw_vertices[] = {
  {40, -1.5}, {20, -2}, {5, -3}, {-3, -4}, {-6, -2},
  {-4,  3}, {0,  4}, {18,  4}, {30,  2}
};
static const az_vector_t forcefiend_right_claw_vertices[] = {
  {40,  1.5}, {20,  2}, {5,  3}, {-3,  4}, {-6,  2},
  {-4, -3}, {0, -4}, {18, -4}, {30, -2}
};
static const az_vector_t forcefiend_segment0_vertices[] = {
  {7, -15}, {15, -12}, {23, 0}, {15, 12}, {7, 15},
  {-14, 14}, {-20, 0}, {-14, -14}
};
static const az_vector_t forcefiend_segment1_vertices[] = {
  {20, 0}, {12, 14}, {0, 15}, {-12, 12},
  {-20, 0}, {-12, -12}, {0, -15}, {12, -14}
};
static const az_vector_t forcefiend_segment2_vertices[] = {
  {20, 0}, {12, 12}, {-2, 11}, {-18, 8},
  {-20, 0}, {-18, -8}, {-2, -11}, {12, -12}
};
static const az_vector_t forcefiend_segment3_vertices[] = {
  {17, 0}, {12, 8}, {-1, 7}, {-16, 5},
  {-18, 0}, {-16, -5}, {-1, -7}, {12, -8}
};
static const az_vector_t forcefiend_stinger_vertices[] = {
  {14, 5}, {-20, 0}, {14, -5}
};
static az_component_data_t forcefiend_components[] = {
  // Pincers:
  { .polygon = AZ_INIT_POLYGON(forcefiend_left_pincer_vertices),
    .init_position = {15,  4}, .immunities = ~0, .impact_damage = 15.0 },
  { .polygon = AZ_INIT_POLYGON(forcefiend_right_pincer_vertices),
    .init_position = {15, -4}, .immunities = ~0, .impact_damage = 15.0 },
  // Left arm:
  { .polygon = AZ_INIT_POLYGON(forcefiend_upper_arm_vertices),
    .immunities = (AZ_DMGF_FREEZE | AZ_DMGF_CPLUS),
    .init_angle = AZ_DEG2RAD(120), .impact_damage = 4.0 },
  { .polygon = AZ_INIT_POLYGON(forcefiend_lower_arm_vertices),
    .init_position = {-20, 34.64}, .init_angle = AZ_DEG2RAD(180),
    .immunities = (AZ_DMGF_FREEZE | AZ_DMGF_CPLUS),
    .impact_damage = 4.0 },
  { .polygon = AZ_INIT_POLYGON(forcefiend_left_claw_vertices),
    .init_position = {-54, 34.64}, .init_angle = AZ_DEG2RAD(180),
    .immunities = ~0, .impact_damage = 12.0 },
  // Right arm:
  { .polygon = AZ_INIT_POLYGON(forcefiend_upper_arm_vertices),
    .immunities = (AZ_DMGF_FREEZE | AZ_DMGF_CPLUS),
    .init_angle = AZ_DEG2RAD(-120), .impact_damage = 4.0 },
  { .polygon = AZ_INIT_POLYGON(forcefiend_lower_arm_vertices),
    .init_position = {-20, -34.64}, .init_angle = AZ_DEG2RAD(180),
    .immunities = (AZ_DMGF_FREEZE | AZ_DMGF_CPLUS),
    .impact_damage = 4.0 },
  { .polygon = AZ_INIT_POLYGON(forcefiend_right_claw_vertices),
    .init_position = {-54, -34.64}, .init_angle = AZ_DEG2RAD(180),
    .immunities = ~0, .impact_damage = 12.0 },
  // Tail:
  { .polygon = AZ_INIT_POLYGON(forcefiend_segment1_vertices),
    .immunities = (AZ_DMGF_FREEZE | AZ_DMGF_CPLUS),
    .init_position = {-30, 0}, .impact_damage = 4.0 },
  { .polygon = AZ_INIT_POLYGON(forcefiend_segment2_vertices),
    .immunities = (AZ_DMGF_FREEZE | AZ_DMGF_CPLUS),
    .init_position = {-60, 0}, .impact_damage = 4.0 },
  { .polygon = AZ_INIT_POLYGON(forcefiend_segment3_vertices),
    .immunities = (AZ_DMGF_FREEZE | AZ_DMGF_CPLUS),
    .init_position = {-90, 0}, .impact_damage = 4.0 },
  { .polygon = AZ_INIT_POLYGON(forcefiend_stinger_vertices),
    .immunities = (AZ_DMGF_FREEZE | AZ_DMGF_CPLUS),
    .init_position = {-120, 0}, .impact_damage = 10.0 }
};

static const az_vector_t chomper_plant_core_vertices[] = {
  {-4, -5}, {8, -12}, {9, -10}, {9, -6}, {6, 0},
  {9, 6}, {9, 10}, {8, 12}, {-4, 5}
};
static az_vector_t chomper_plant_base_vertices[] = {
  {14, 4}, {8, 11}, {0, 14}, {0, -14}, {8, -11}, {14, -4}
};
static const az_vector_t chomper_plant_left_pincer_vertices[] = {
  {23, 0}, {26,  2}, {23,  8}, {20,  11}, {9,  14}, {-5, 10}, {-8,  6}, {-9, 0}
};
static const az_vector_t chomper_plant_right_pincer_vertices[] = {
  {23, 0}, {26, -2}, {23, -7}, {19, -10}, {7, -13}, {-6, -9}, {-8, -5}, {-9, 0}
};
static az_vector_t stalk_segment_vertices[] = {
  {15, 0}, {12, 4}, {-12, 4}, {-15, 0}, {-12, -4}, {12, -4}
};
static az_component_data_t chomper_plant_components[] = {
  // Base:
  { .polygon = AZ_INIT_POLYGON(chomper_plant_base_vertices),
    .immunities = (AZ_DMGF_NORMAL | AZ_DMGF_CHARGED),
    .init_position = {-190, 0}, .impact_damage = 4.0 },
  // Pincers:
  { .polygon = AZ_INIT_POLYGON(chomper_plant_left_pincer_vertices),
    .immunities = (AZ_DMGF_NORMAL | AZ_DMGF_CHARGED), .impact_damage = 12.0 },
  { .polygon = AZ_INIT_POLYGON(chomper_plant_right_pincer_vertices),
    .immunities = (AZ_DMGF_NORMAL | AZ_DMGF_CHARGED), .impact_damage = 12.0 },
  // Stalk:
#define STALK_SEGMENT(x) \
  { .polygon = AZ_INIT_POLYGON(stalk_segment_vertices), \
    .immunities = (AZ_DMGF_NORMAL | AZ_DMGF_CHARGED), \
    .init_position = {(x), 0}, .impact_damage = 4.0 }

  STALK_SEGMENT(-10), STALK_SEGMENT(-30), STALK_SEGMENT(-50),
  STALK_SEGMENT(-70), STALK_SEGMENT(-90), STALK_SEGMENT(-110),
  STALK_SEGMENT(-130), STALK_SEGMENT(-150), STALK_SEGMENT(-170)
#undef STALK_SEGMENT
};

static az_vector_t short_stalk_segment_vertices[] = {
  {7.5, 0}, {6, 4}, {-6, 4}, {-7.5, 0}, {-6, -4}, {6, -4}
};
static az_component_data_t aquatic_chomper_components[] = {
  // Base:
  { .polygon = AZ_INIT_POLYGON(chomper_plant_base_vertices),
    .immunities = (AZ_DMGF_NORMAL | AZ_DMGF_CHARGED),
    .init_position = {-95, 0}, .impact_damage = 4.0 },
  // Pincers:
  { .polygon = AZ_INIT_POLYGON(chomper_plant_left_pincer_vertices),
    .immunities = (AZ_DMGF_NORMAL | AZ_DMGF_CHARGED), .impact_damage = 12.0 },
  { .polygon = AZ_INIT_POLYGON(chomper_plant_right_pincer_vertices),
    .immunities = (AZ_DMGF_NORMAL | AZ_DMGF_CHARGED), .impact_damage = 12.0 },
  // Stalk:
#define STALK_SEGMENT(x) \
  { .polygon = AZ_INIT_POLYGON(short_stalk_segment_vertices), \
    .immunities = (AZ_DMGF_NORMAL | AZ_DMGF_CHARGED), \
    .init_position = {(x), 0}, .impact_damage = 4.0 }

  STALK_SEGMENT(-5), STALK_SEGMENT(-15), STALK_SEGMENT(-25),
  STALK_SEGMENT(-35), STALK_SEGMENT(-45), STALK_SEGMENT(-55),
  STALK_SEGMENT(-65), STALK_SEGMENT(-75), STALK_SEGMENT(-85)
#undef STALK_SEGMENT
};

static az_component_data_t jungle_chomper_components[] = {
  // Base:
  { .polygon = AZ_INIT_POLYGON(chomper_plant_base_vertices),
    .immunities = (AZ_DMGF_NORMAL | AZ_DMGF_CHARGED | AZ_DMGF_ROCKET),
    .init_position = {-190, 0}, .impact_damage = 6.0 },
  // Pincers:
  { .polygon = AZ_INIT_POLYGON(chomper_plant_left_pincer_vertices),
    .immunities = (AZ_DMGF_NORMAL | AZ_DMGF_CHARGED),
    .impact_damage = 18.0 },
  { .polygon = AZ_INIT_POLYGON(chomper_plant_right_pincer_vertices),
    .immunities = (AZ_DMGF_NORMAL | AZ_DMGF_CHARGED),
    .impact_damage = 18.0 },
  // Stalk:
#define STALK_SEGMENT(x) \
  { .polygon = AZ_INIT_POLYGON(stalk_segment_vertices), \
    .immunities = (AZ_DMGF_NORMAL | AZ_DMGF_CHARGED | AZ_DMGF_ROCKET), \
    .init_position = {(x), 0}, .impact_damage = 4.0 }

  STALK_SEGMENT(-10), STALK_SEGMENT(-30), STALK_SEGMENT(-50),
  STALK_SEGMENT(-70), STALK_SEGMENT(-90), STALK_SEGMENT(-110),
  STALK_SEGMENT(-130), STALK_SEGMENT(-150), STALK_SEGMENT(-170)
#undef STALK_SEGMENT
};

static az_component_data_t fire_chomper_components[] = {
  // Base:
  { .polygon = AZ_INIT_POLYGON(chomper_plant_base_vertices),
    .immunities = (AZ_DMGF_NORMAL | AZ_DMGF_CHARGED | AZ_DMGF_ROCKET),
    .init_position = {-95, 0}, .impact_damage = 4.0 },
  // Pincers:
  { .polygon = AZ_INIT_POLYGON(chomper_plant_left_pincer_vertices),
    .immunities = (AZ_DMGF_NORMAL | AZ_DMGF_CHARGED),
    .impact_damage = 18.0 },
  { .polygon = AZ_INIT_POLYGON(chomper_plant_right_pincer_vertices),
    .immunities = (AZ_DMGF_NORMAL | AZ_DMGF_CHARGED),
    .impact_damage = 18.0 },
  // Stalk:
#define STALK_SEGMENT(x) \
  { .polygon = AZ_INIT_POLYGON(short_stalk_segment_vertices), \
    .immunities = (AZ_DMGF_NORMAL | AZ_DMGF_CHARGED | AZ_DMGF_ROCKET), \
    .init_position = {(x), 0}, .impact_damage = 4.0 }

  STALK_SEGMENT(-5), STALK_SEGMENT(-15), STALK_SEGMENT(-25),
  STALK_SEGMENT(-35), STALK_SEGMENT(-45), STALK_SEGMENT(-55),
  STALK_SEGMENT(-65), STALK_SEGMENT(-75), STALK_SEGMENT(-85)
#undef STALK_SEGMENT
};

static az_vector_t copter_vertices[] = {
  {14, 21}, {-14, 21}, {-14, -21}, {14, -21}
};

static az_vector_t boss_door_body_vertices[] = {
  {-20, -50}, {-3, -41}, {7, -24}, {7, 24}, {-3, 41}, {-20, 50}
};
static az_vector_t boss_door_eyelid_vertices[] = {
  {0, 0}, {22, 0}, {21.6658, 3.82026}, {20.6732, 7.52444}, {19.0526, 11},
  {16.853, 14.1413}, {14.1413, 16.853}, {11, 19.0526}, {7.52444, 20.6732},
  {3.82026, 21.6658}, {0, 22}
};
static az_component_data_t boss_door_components[] = {
  // Eye:
  { .bounding_radius = 20.0, .impact_damage = 12.0,
    .immunities = AZ_DMGF_FREEZE },
  // Eyelids:
  { .polygon = AZ_INIT_POLYGON(boss_door_eyelid_vertices),
    .immunities = ~0, .impact_damage = 10.0, .init_angle = AZ_DEG2RAD(0) },
  { .polygon = AZ_INIT_POLYGON(boss_door_eyelid_vertices),
    .immunities = ~0, .impact_damage = 10.0, .init_angle = AZ_DEG2RAD(-90) }
};

static az_component_data_t creepy_eye_components[] = {
  // Eye:
  { .bounding_radius = 20.0,
    .immunities = (AZ_DMGF_NORMAL | AZ_DMGF_FREEZE | AZ_DMGF_PIERCE |
                   AZ_DMGF_BOMB | AZ_DMGF_MEGA_BOMB) },
  // Eyelids:
  { .polygon = AZ_INIT_POLYGON(boss_door_eyelid_vertices),
    .immunities = ~0, .init_angle = AZ_DEG2RAD(0) },
  { .polygon = AZ_INIT_POLYGON(boss_door_eyelid_vertices),
    .immunities = ~0, .init_angle = AZ_DEG2RAD(-90) }
};

static const az_vector_t mini_zipper_vertices[] = {
  {14, 2.1}, {10.5, 4.2}, {7, 4.9}, {-7, 2.8}, {-10.5, 1.4}, {-10.5, -1.4},
  {-7, -2.8}, {7, -4.9}, {10.5, -4.2}, {14, -2.1}
};

static const az_vector_t spined_crawler_vertices[] = {
  {-14.5, -11}, {-8, -16}, {-1.5, -13.8564}, {3.25833, -8}, {5, 0},
  {3.25833, 8}, {-1.5, 13.8564}, {-8, 16}, {-14.5, 11}, {-18, 0}
};

static const az_vector_t oth_gunship_vertices[] = {
  {20, 0}, {15, 4}, {6, 12}, {-10, 12}, {-14, 4},
  {-14, -4}, {-10, -12}, {6, -12}, {15, -4}
};

static const az_vector_t leaper_vertices[] = {
  {8, 0}, {12, 15}, {-10, 17}, {-13, 0}, {-10, -17}, {12, -15}
};

static const az_vector_t piston_head_vertices[] = {
  {27, 9}, {21, 10}, {21, 9}, {-15, 9},
  {-15, -9}, {21, -9}, {21, -10}, {27, -9}
};
static const az_vector_t piston_segment0_vertices[] = {
  {19, 10}, {-15, 10}, {-15, -10}, {19, -10}
};
static const az_vector_t piston_segment1_vertices[] = {
  {17, 11}, {-15, 11}, {-15, -11}, {17, -11}
};
static const az_vector_t piston_segment2_vertices[] = {
  {15, 12}, {-15, 12}, {-15, -12}, {15, -12}
};
static az_component_data_t piston_components[] = {
  { .polygon = AZ_INIT_POLYGON(piston_segment0_vertices),
    .immunities = (AZ_DMGF_NORMAL | AZ_DMGF_FLAME), .impact_damage = 10.0 },
  { .polygon = AZ_INIT_POLYGON(piston_segment1_vertices),
    .immunities = (AZ_DMGF_NORMAL | AZ_DMGF_FLAME), .impact_damage = 10.0 },
  { .polygon = AZ_INIT_POLYGON(piston_segment2_vertices),
    .immunities = (AZ_DMGF_NORMAL | AZ_DMGF_FLAME), .impact_damage = 10.0 }
};
static az_component_data_t armored_piston_components[] = {
  { .polygon = AZ_INIT_POLYGON(piston_segment0_vertices),
    .immunities = (AZ_DMGF_NORMAL | AZ_DMGF_CHARGED | AZ_DMGF_FLAME),
    .impact_damage = 10.0 },
  { .polygon = AZ_INIT_POLYGON(piston_segment1_vertices),
    .immunities = (AZ_DMGF_NORMAL | AZ_DMGF_CHARGED | AZ_DMGF_FLAME),
    .impact_damage = 10.0 },
  { .polygon = AZ_INIT_POLYGON(piston_segment2_vertices),
    .immunities = (AZ_DMGF_NORMAL | AZ_DMGF_CHARGED | AZ_DMGF_FLAME),
    .impact_damage = 10.0 }
};
static az_component_data_t armored_piston_ext_components[] = {
  { .polygon = AZ_INIT_POLYGON(piston_segment0_vertices),
    .init_position = {-30, 0}, .impact_damage = 10.0,
    .immunities = (AZ_DMGF_NORMAL | AZ_DMGF_CHARGED | AZ_DMGF_FLAME) },
  { .polygon = AZ_INIT_POLYGON(piston_segment1_vertices),
    .init_position = {-60, 0}, .impact_damage = 10.0,
    .immunities = (AZ_DMGF_NORMAL | AZ_DMGF_CHARGED | AZ_DMGF_FLAME) },
  { .polygon = AZ_INIT_POLYGON(piston_segment2_vertices),
    .init_position = {-90, 0}, .impact_damage = 10.0,
    .immunities = (AZ_DMGF_NORMAL | AZ_DMGF_CHARGED | AZ_DMGF_FLAME) }
};
static az_component_data_t incorporeal_piston_components[] = {
  { .polygon = AZ_INIT_POLYGON(piston_segment0_vertices) },
  { .polygon = AZ_INIT_POLYGON(piston_segment1_vertices) },
  { .polygon = AZ_INIT_POLYGON(piston_segment2_vertices) }
};
static az_component_data_t incorporeal_piston_ext_components[] = {
  { .polygon = AZ_INIT_POLYGON(piston_segment0_vertices),
    .init_position = {-30, 0} },
  { .polygon = AZ_INIT_POLYGON(piston_segment1_vertices),
    .init_position = {-60, 0} },
  { .polygon = AZ_INIT_POLYGON(piston_segment2_vertices),
    .init_position = {-90, 0} }
};

static const az_vector_t kilofuge_main_body_vertices[] = {
  {140, -20}, {144, 0}, {140, 20}, {90, 120}, {20, 140}, {-80, 145},
  {-123, 78}, {-150, 0}, {-123, -78}, {-80, -145}, {20, -140}, {90, -120}
};
static const az_vector_t kilofuge_left_pincer_vertices[] = {
  {50, 10}, {100, 0}, {50, 20}, {0, 6}, {-5, 0}, {0, -6}
};
static const az_vector_t kilofuge_right_pincer_vertices[] = {
  {50, -10}, {100, 0}, {50, -20}, {0, -6}, {-5, 0}, {0, 6}
};
static const az_vector_t kilofuge_left_leg_vertices[] = {
  {-100, -10}, {-50, -12}, {0, -2}, {2, 0}, {0, 2}, {-50, 1}, {-100, 10}
};
static const az_vector_t kilofuge_right_leg_vertices[] = {
  {-100, -10}, {-50, -1}, {0, -2}, {2, 0}, {0, 2}, {-50, 12}, {-100, 10}
};
static az_component_data_t kilofuge_components[] = {
  // Eyes:
  { .bounding_radius = 10, .init_position = {144, 0}, .impact_damage = 10,
    .immunities = (AZ_DMGF_FREEZE | AZ_DMGF_CPLUS) },
  { .bounding_radius = 10, .init_position = {140, 20}, .impact_damage = 10,
    .immunities = (AZ_DMGF_FREEZE | AZ_DMGF_CPLUS) },
  { .bounding_radius = 10, .init_position = {140, -20}, .impact_damage = 10,
    .immunities = (AZ_DMGF_FREEZE | AZ_DMGF_CPLUS) },
  // Pincers:
  { .polygon = AZ_INIT_POLYGON(kilofuge_left_pincer_vertices),
    .init_position = {110,  60}, .immunities = ~0, .impact_damage = 30 },
  { .polygon = AZ_INIT_POLYGON(kilofuge_right_pincer_vertices),
    .init_position = {110, -60}, .immunities = ~0, .impact_damage = 30 },
  // Legs:
#define KILOFUGE_LEG(side, x, y, theta) \
  { .polygon = AZ_INIT_POLYGON(kilofuge_##side##_leg_vertices), \
    .init_position = {(x), (y)}, .init_angle = (theta), \
    .immunities = ~0, .impact_damage = 20 }

  KILOFUGE_LEG(left,  70,  190,  1.5), KILOFUGE_LEG(right, 110, -195, -1.0),
  KILOFUGE_LEG(right, 20, -200, -1.5), KILOFUGE_LEG(left,   55,  190,  1.0),
  KILOFUGE_LEG(left, -50,  190,  1.5), KILOFUGE_LEG(right, -10, -200, -1.0)
#undef KILOFUGE_LEG
};

static const az_vector_t ice_crystal_vertices[] = {
  {20, 0}, {7, 12}, {-20, 0}, {0, -10}
};

static const az_vector_t switcher_vertices[] = {
  {12.5, 4}, {8.5, 8.5}, {4.5, 9.5}, {-10, 5.5}, {-12.5, 2.5},
  {-12.5, -2.5}, {-10, -5.5}, {4.5, -9.5}, {8.5, -8.5}, {12.5, -4}
};

static const az_vector_t nightshade_body_vertices[] = {
  {18.7, 0}, {9.9, 11}, {-12.1, 5.5}, {-12.1, -5.5}, {9.9, -11}
};
static const az_vector_t nightshade_left_mandible_vertices[] = {
  {0, 0}, {26, -1}, {20, 4}, {10, 5}, {0, 4}, {-2, 2}
};
static const az_vector_t nightshade_right_mandible_vertices[] = {
  {26, 1}, {0, 0}, {-2, -2}, {0, -4}, {10, -5}, {20, -4}
};
static az_component_data_t nightshade_components[] = {
  { .init_position = {15, 2}, .impact_damage = 25.0,
    .polygon = AZ_INIT_POLYGON(nightshade_left_mandible_vertices) },
  { .init_position = {15, -2}, .impact_damage = 25.0,
    .polygon = AZ_INIT_POLYGON(nightshade_right_mandible_vertices) }
};

static const az_vector_t small_fish_head_vertices[] = {
  {-8, -6}, {6, -8}, {10, -5}, {12, 0}, {10, 5}, {6, 8}, {-8, 6}, {-10, 0}
};
static const az_vector_t small_fish_middle_vertices[] = {
  {10, 0}, {8, 6}, {-8, 4}, {-10, 0}, {-8, -4}, {8, -6}
};
static const az_vector_t small_fish_tail_vertices[] = {
  {8, 4}, {-10, 0}, {8, -4}
};
static az_component_data_t small_fish_components[] = {
  { .polygon = AZ_INIT_POLYGON(small_fish_middle_vertices),
    .init_position = {-15, 0}, .impact_damage = 12.0 },
  { .polygon = AZ_INIT_POLYGON(small_fish_tail_vertices),
    .init_position = {-30, 0}, .impact_damage = 12.0 }
};

static const az_vector_t large_fish_head_vertices[] = {
  {-12, -10}, {9, -12}, {15, -8}, {18, 0}, {15, 8}, {9, 12}, {-12, 10},
  {-15, 0}
};
static const az_vector_t large_fish_mid1_vertices[] = {
  {15, 0}, {12, 10}, {-12, 7}, {-15, 0}, {-12, -7}, {12, -10}
};
static const az_vector_t large_fish_mid2_vertices[] = {
  {15, 0}, {12, 7}, {-12, 4}, {-15, 0}, {-12, -4}, {12, -7}
};
static const az_vector_t large_fish_tail_vertices[] = {
  {12, 4}, {-15, 0}, {12, -4}, {13, 0}
};
static az_component_data_t large_fish_components[] = {
  { .polygon = AZ_INIT_POLYGON(large_fish_mid1_vertices),
    .init_position = {-22.5, 0}, .impact_damage = 15.0 },
  { .polygon = AZ_INIT_POLYGON(large_fish_mid2_vertices),
    .init_position = {-45, 0}, .impact_damage = 15.0 },
  { .polygon = AZ_INIT_POLYGON(large_fish_tail_vertices),
    .init_position = {-67.5, 0}, .impact_damage = 15.0 }
};

// TODO: Update the various Nocturne component polygons once we have real
//   graphics for the Nocturne.
static const az_vector_t nocturne_main_body_vertices[] = {
  {45, 0}, {15, 60}, {-15, 70}, {-70, 45},
  {-105, 0}, {-70, -45}, {-15, -70}, {15, -60}
};
static const az_vector_t nocturne_inner_upper_arm_vertices[] = {
  {0, 5}, {-5, 0}, {0, -5}, {30, -4}, {30, 4}
};
static const az_vector_t nocturne_inner_lower_arm_vertices[] = {
  {0, 4}, {-4, 0}, {0, -4}, {30, -1}, {30, 1}
};
static const az_vector_t nocturne_outer_front_arm_vertices[] = {
  {0, 4}, {-4, 0}, {0, -4}, {50, -2}, {50, 2}
};
static const az_vector_t nocturne_outer_rear_arm_vertices[] = {
  {0, 4}, {-4, 0}, {0, -4}, {45, 0}
};
static az_component_data_t nocturne_components[] = {
  // Inner left upper arm:
  { .polygon = AZ_INIT_POLYGON(nocturne_inner_upper_arm_vertices),
    .init_position = {35, 20}, .init_angle = AZ_DEG2RAD(20),
    .immunities = ~0, .impact_damage = 30 },
  // Inner left lower arm:
  { .polygon = AZ_INIT_POLYGON(nocturne_inner_lower_arm_vertices),
    .init_position = {63.19, 30.26}, .init_angle = AZ_DEG2RAD(0),
    .immunities = ~0, .impact_damage = 30 },
  // Inner right upper arm:
  { .polygon = AZ_INIT_POLYGON(nocturne_inner_upper_arm_vertices),
    .init_position = {35, -20}, .init_angle = AZ_DEG2RAD(-20),
    .immunities = ~0, .impact_damage = 30 },
  // Inner right lower arm:
  { .polygon = AZ_INIT_POLYGON(nocturne_inner_lower_arm_vertices),
    .init_position = {63.19, -30.26}, .init_angle = AZ_DEG2RAD(0),
    .immunities = ~0, .impact_damage = 30 },
  // Outer front left arm:
  { .polygon = AZ_INIT_POLYGON(nocturne_outer_front_arm_vertices),
    .init_position = {25, 40}, .init_angle = AZ_DEG2RAD(40),
    .immunities = ~0, .impact_damage = 30 },
  // Outer front right arm:
  { .polygon = AZ_INIT_POLYGON(nocturne_outer_front_arm_vertices),
    .init_position = {25, -40}, .init_angle = AZ_DEG2RAD(-40),
    .immunities = ~0, .impact_damage = 30 },
  // Outer rear left arm:
  { .polygon = AZ_INIT_POLYGON(nocturne_outer_rear_arm_vertices),
    .init_position = {-47, 52}, .init_angle = AZ_DEG2RAD(135),
    .immunities = ~0, .impact_damage = 30 },
  // Outer rear right arm:
  { .polygon = AZ_INIT_POLYGON(nocturne_outer_rear_arm_vertices),
    .init_position = {-47, -52}, .init_angle = AZ_DEG2RAD(-135),
    .immunities = ~0, .impact_damage = 30 },
};

static const az_vector_t mycoflakker_vertices[] = {
  {-16, 9}, {-18, -1}, {-16, -11}, {1, -7.4}, {2, -14}, {5, -18},
  {14, -10}, {18, 0}, {14, 10}, {5, 18}, {2, 14}, {1, 7.5}
};

static const az_vector_t mycostalker_vertices[] = {
  {-16, 11.5}, {-20, -1}, {-16, -11.5}, {3, -6.3}, {4, -14}, {7, -18},
  {16, -10}, {20, 0}, {16, 10}, {7, 18}, {4, 14}, {3, 7.4}
};

static const az_vector_t fire_crawler_vertices[] = {
  {5, -8}, {5, 8}, {-4, 14}, {-15, 10}, {-15, -10}, {-4, -14}
};

static const az_vector_t forceling_head_vertices[] = {
  {-4, -3}, {3, -4}, {5, -3.5}, {9, 0}, {5, 3.5}, {3, 4}, {-4, 3}, {-5, 0}
};
static const az_vector_t forceling_middle_vertices[] = {
  {5, 0}, {4, 3}, {-4, 2}, {-5, 0}, {-4, -2}, {4, -3}
};
static const az_vector_t forceling_tail_vertices[] = {
  {4, 2}, {-7, 0}, {4, -2}
};
static az_component_data_t forceling_components[] = {
  { .polygon = AZ_INIT_POLYGON(forceling_middle_vertices),
    .init_position = {-7.5, 0}, .impact_damage = 6.0 },
  { .polygon = AZ_INIT_POLYGON(forceling_tail_vertices),
    .init_position = {-15, 0}, .impact_damage = 6.0 }
};

static const az_vector_t small_auv_vertices[] = {
  {30, 10}, {10, 16}, {10, 14}, {-24, 14}, {-30, 8},
  {-30, -8}, {-24, -14}, {10, -14}, {10, -16}, {30, -10}
};

static const az_vector_t sensor_laser_vertices[] = {
  {4, -3}, {6, 0}, {4, 3}, {10, 12}, {10, 20}, {-15, 20},
  {-15, -20}, {10, -20}, {10, -12}
};

static const az_vector_t grabber_plant_core_vertices[] = {
  {-4, -5}, {3, -11}, {5, -10}, {6.5, -6}, {4, 0},
  {6.5, 6}, {5, 10}, {3, 11}, {-4, 5}
};
static const az_vector_t grabber_plant_left_pincer_vertices[] = {
  {20, 0}, {23,  2}, {20,  9}, {17,  12}, {10,  15}, {1,  14}, {-5, 10},
  {-7,  7}, {-8, 0}
};
static const az_vector_t grabber_plant_right_pincer_vertices[] = {
  {20, 0}, {23, -2}, {20, -8}, {17, -12}, {10, -15}, {1, -14}, {-6, -9},
  {-7, -6}, {-8, 0}
};
static const az_vector_t grabber_plant_base_vertices[] = {
  {-22, -20}, {-12, -13}, {-4, -11}, {-2, -8}, {-1, -4}, {0.5, 0},
  {-1, 4}, {-2, 8}, {-4, 11}, {-12, 14}, {-22, 20}
};
static az_component_data_t grabber_plant_components[] = {
  { .polygon = AZ_INIT_POLYGON(grabber_plant_left_pincer_vertices),
    .immunities = (AZ_DMGF_NORMAL | AZ_DMGF_CHARGED), .impact_damage = 18.0 },
  { .polygon = AZ_INIT_POLYGON(grabber_plant_right_pincer_vertices),
    .immunities = (AZ_DMGF_NORMAL | AZ_DMGF_CHARGED), .impact_damage = 18.0 },
  { .polygon = AZ_INIT_POLYGON(grabber_plant_base_vertices),
    .immunities = (AZ_DMGF_NORMAL | AZ_DMGF_CHARGED), .impact_damage = 18.0 }
};

static const az_vector_t pop_open_turret_cannon_vertices[] = {
  {26, 5}, {0, 5}, {0, -5}, {26, -5}
};
static az_vector_t pop_open_turret_shell_vertices[] = {
  {0, 0}, {33, 0}, {32.4987, 5.73039}, {31.0098, 11.28666}, {28.5789, 16.5},
  {25.2795, 21.21195}, {21.21195, 25.2795}, {16.5, 28.5789},
  {11.28666, 31.0098}, {5.73039, 32.4987}, {0, 33}
};
static az_component_data_t pop_open_turret_components[] = {
  { .polygon = AZ_INIT_POLYGON(pop_open_turret_cannon_vertices),
    .impact_damage = 10.0 },
  { .polygon = AZ_INIT_POLYGON(pop_open_turret_shell_vertices),
    .immunities = ~(AZ_DMGF_HYPER_ROCKET | AZ_DMGF_CPLUS),
    .init_angle = AZ_DEG2RAD(0) },
  { .polygon = AZ_INIT_POLYGON(pop_open_turret_shell_vertices),
    .immunities = ~(AZ_DMGF_HYPER_ROCKET | AZ_DMGF_CPLUS),
    .init_angle = AZ_DEG2RAD(-90) }
};

static az_vector_t spiked_vine_base_vertices[] = {
  {14, 4}, {8, 11}, {0, 14}, {0, -14}, {8, -11}, {14, -4}
};
static az_vector_t spiked_vine_segment_vertices[] = {
  {15, 0}, {12, 4}, {-12, 4}, {-15, 0}, {-12, -4}, {12, -4}
};
static az_vector_t spiked_vine_tip_vertices[] = {
  {-4, 0}, {0, -4}, {20, 0}, {0, 4}
};
static az_component_data_t spiked_vine_components[] = {
  // Base:
  { .polygon = AZ_INIT_POLYGON(spiked_vine_base_vertices),
    .immunities = (AZ_DMGF_NORMAL | AZ_DMGF_CHARGED), .impact_damage = 10.0 },
  // Vine:
#define VINE_SEGMENT() \
  { .polygon = AZ_INIT_POLYGON(spiked_vine_segment_vertices), \
    .immunities = (AZ_DMGF_NORMAL | AZ_DMGF_CHARGED), .impact_damage = 10.0 }

  VINE_SEGMENT(), VINE_SEGMENT(), VINE_SEGMENT(), VINE_SEGMENT(),
  VINE_SEGMENT(), VINE_SEGMENT(), VINE_SEGMENT(), VINE_SEGMENT(),
  VINE_SEGMENT(), VINE_SEGMENT(), VINE_SEGMENT()
#undef VINE_SEGMENT
};

static az_vector_t magbeest_head_vertices[] = {
  {25, 16}, {16, 25}, {-16, 25}, {-25, 16},
  {-25, -16}, {-16, -25}, {16, -25}, {25, -16}
};
static az_vector_t magbeest_head_piston_0_vertices[] = {
  {0, 4}, {-80, 4}, {-80, -4}, {0, -4}
};
static az_vector_t magbeest_head_piston_1_vertices[] = {
  {-2, 5}, {-80, 5}, {-80, -5}, {-2, -5}
};
static az_vector_t magbeest_head_piston_2_vertices[] = {
  {-4, 6}, {-80, 6}, {-80, -6}, {-4, -6}
};
static az_vector_t magbeest_head_piston_3_vertices[] = {
  {-6, 7}, {-80, 7}, {-80, -7}, {-6, -7}
};
static az_vector_t magbeest_head_piston_4_vertices[] = {
  {-8, 8}, {-80, 8}, {-80, -8}, {-8, -8}
};
static az_component_data_t magbeest_head_components[] = {
  // First piston:
  { .polygon = AZ_INIT_POLYGON(magbeest_head_piston_0_vertices),
    .immunities = ~0, .init_position = {-60, 0}, .init_angle = AZ_PI },
  { .polygon = AZ_INIT_POLYGON(magbeest_head_piston_1_vertices),
    .immunities = ~0, .init_position = {-60, 0}, .init_angle = AZ_PI },
  { .polygon = AZ_INIT_POLYGON(magbeest_head_piston_2_vertices),
    .immunities = ~0, .init_position = {-60, 0}, .init_angle = AZ_PI },
  { .polygon = AZ_INIT_POLYGON(magbeest_head_piston_3_vertices),
    .immunities = ~0, .init_position = {-60, 0}, .init_angle = AZ_PI },
  { .polygon = AZ_INIT_POLYGON(magbeest_head_piston_4_vertices),
    .immunities = ~0, .init_position = {-60, 0}, .init_angle = AZ_PI },
  // Second piston:
  { .polygon = AZ_INIT_POLYGON(magbeest_head_piston_0_vertices),
    .immunities = ~0, .init_position = {-60, 0}, .init_angle = AZ_PI },
  { .polygon = AZ_INIT_POLYGON(magbeest_head_piston_1_vertices),
    .immunities = ~0, .init_position = {-60, 0}, .init_angle = AZ_PI },
  { .polygon = AZ_INIT_POLYGON(magbeest_head_piston_2_vertices),
    .immunities = ~0, .init_position = {-60, 0}, .init_angle = AZ_PI },
  { .polygon = AZ_INIT_POLYGON(magbeest_head_piston_3_vertices),
    .immunities = ~0, .init_position = {-60, 0}, .init_angle = AZ_PI },
  { .polygon = AZ_INIT_POLYGON(magbeest_head_piston_4_vertices),
    .immunities = ~0, .init_position = {-60, 0}, .init_angle = AZ_PI },
  // Eye:
  { .bounding_radius = 22.0,
    .immunities = (AZ_DMGF_FREEZE | AZ_DMGF_CPLUS | AZ_DMGF_FLAME) }
};
static az_vector_t magbeest_legs_l_base_vertices[] = {
  {10, -35}, {20, -25}, {20, 45}, {-20, 45}, {-30, 35}, {-30, -35}
};
static az_vector_t magbeest_legs_r_base_vertices[] = {
  {20, -45}, {20, 25}, {10, 35}, {-30, 35}, {-30, -35}, {-20, -45}
};
static az_vector_t magbeest_magnet_vertices[] = {
  {-7, 0}, {-4.9, -4.9}, {0, -7}, {4, -6}, {21, -17}, {24, -21.8},
  {30.2, -17.1}, {26, -10}, {24.5, 0}, {26, 10}, {30.2, 17.2}, {24, 21.7},
  {21, 17}, {4, 6}, {0, 7}, {-4.9, 4.9}
};
static az_vector_t magbeest_gatling_vertices[] = {
  {-20, 15}, {-20, -15}, {15, -15}, {15, -9}, {50, -9}, {50, 9}, {15, 9},
  {15, 15}
};
static az_vector_t magbeest_short_leg_vertices[] = {
  {0, 10}, {100, 10}, {107, 7}, {110, 0}, {107, -7}, {100, -10}, {0, -10}
};
static az_vector_t magbeest_long_leg_vertices[] = {
  {0, 10}, {180, 10}, {187, 7}, {190, 0}, {187, -7}, {180, -10}, {0, -10}
};
static az_component_data_t magbeest_legs_l_components[] = {
  { .polygon = AZ_INIT_POLYGON(magbeest_magnet_vertices), .immunities = ~0,
    .init_position = {20, 35}, .impact_damage = 20.0 },
  { .polygon = AZ_INIT_POLYGON(magbeest_long_leg_vertices), .immunities = ~0,
    .init_position = {-20, -25}, .init_angle = AZ_PI, .impact_damage = 20.0 },
  { .polygon = AZ_INIT_POLYGON(magbeest_short_leg_vertices), .immunities = ~0,
    .init_position = {-300, -25}, .impact_damage = 20.0 },
  { .polygon = AZ_INIT_POLYGON(magbeest_short_leg_vertices), .immunities = ~0,
    .init_position = {-20, 25}, .init_angle = AZ_PI, .impact_damage = 20.0 },
  { .polygon = AZ_INIT_POLYGON(magbeest_long_leg_vertices), .immunities = ~0,
    .init_position = {-300, 25}, .impact_damage = 20.0 }
};
static az_component_data_t magbeest_legs_r_components[] = {
  { .polygon = AZ_INIT_POLYGON(magbeest_gatling_vertices), .immunities = ~0,
    .init_position = {20, -35}, .impact_damage = 20.0 },
  { .polygon = AZ_INIT_POLYGON(magbeest_short_leg_vertices), .immunities = ~0,
    .init_position = {-20, -25}, .init_angle = AZ_PI, .impact_damage = 20.0 },
  { .polygon = AZ_INIT_POLYGON(magbeest_long_leg_vertices), .immunities = ~0,
    .init_position = {-300, -25}, .impact_damage = 20.0 },
  { .polygon = AZ_INIT_POLYGON(magbeest_long_leg_vertices), .immunities = ~0,
    .init_position = {-20, 25}, .init_angle = AZ_PI, .impact_damage = 20.0 },
  { .polygon = AZ_INIT_POLYGON(magbeest_short_leg_vertices), .immunities = ~0,
    .init_position = {-300, 25}, .impact_damage = 20.0 }
};

static const az_vector_t crab_crawler_body_vertices[] = {
  {-14.5, -11}, {-8, -16}, {-2, -13.8564}, {1.75833, -8}, {3.5, 0},
  {1.75833, 8}, {-2, 13.8564}, {-8, 16}, {-14.5, 11}, {-18, 0}
};
static az_vector_t crab_crawler_left_claw_vertices[] = {
  {12, 15}, {2, 13}, {-3, 10}, {-6, 7}, {-8, 0}, {-6, -6}, {-2, -5},
  {1, -1}, {4, 4}, {9, 10}, {5, 9}
};
static az_vector_t crab_crawler_right_claw_vertices[] = {
  {12, -15}, {2, -13}, {-3, -10}, {-6, -7}, {-8, 0}, {-6, 6}, {-2, 5},
  {1, 1}, {4, -4}, {9, -10}, {5, -9}
};
static az_component_data_t crab_crawler_components[] = {
  { .polygon = AZ_INIT_POLYGON(crab_crawler_left_claw_vertices),
    .init_position = {-8, 9}, .impact_damage = 12.0,
    .immunities = (AZ_DMGF_NORMAL | AZ_DMGF_CHARGED | AZ_DMGF_BEAM |
                   AZ_DMGF_FLAME) },
  { .polygon = AZ_INIT_POLYGON(crab_crawler_right_claw_vertices),
    .init_position = {-8, -9}, .impact_damage = 12.0,
    .immunities = (AZ_DMGF_NORMAL | AZ_DMGF_CHARGED | AZ_DMGF_BEAM |
                   AZ_DMGF_FLAME) }
};

static const az_vector_t scrap_metal_vertices[] = {
  {10, 2}, {0, 8}, {-3, 3}, {-10, 0}, {2, -6}, {3, -3}
};

static const az_vector_t oth_tentacle_segment_vertices[] = {
  {19, -3}, {19, 3}, {-19, 3}, {-19, -3}
};
static az_component_data_t oth_tentacle_components[] = {
#define TENTACLE_SEGMENT() \
  { .polygon = AZ_INIT_POLYGON(oth_tentacle_segment_vertices), \
    .immunities = (AZ_DMGF_BOMB | AZ_DMGF_MEGA_BOMB), .impact_damage = 20.0 }

  TENTACLE_SEGMENT(), TENTACLE_SEGMENT(), TENTACLE_SEGMENT(),
  TENTACLE_SEGMENT(), TENTACLE_SEGMENT(), TENTACLE_SEGMENT(),
  TENTACLE_SEGMENT(), TENTACLE_SEGMENT(), TENTACLE_SEGMENT(),
  TENTACLE_SEGMENT(), TENTACLE_SEGMENT(), TENTACLE_SEGMENT()
#undef TENTACLE_SEGMENT
};

static az_baddie_data_t baddie_datas[] = {
  [AZ_BAD_MARKER] = {
    .max_health = 1000000.0,
    .static_properties = (AZ_BADF_CARRIES_CARGO | AZ_BADF_INCORPOREAL |
                          AZ_BADF_NO_HOMING),
    .main_body = { .bounding_radius = 20.0 }
  },
  [AZ_BAD_NORMAL_TURRET] = {
    .max_health = 3.0, .overall_bounding_radius = 30.5,
    .color = {160, 160, 160, 255},
    .hurt_sound = AZ_SND_HURT_TURRET, .death_sound = AZ_SND_KILL_TURRET,
    .potential_pickups = ~AZ_PUPF_LARGE_SHIELDS,
    .main_body = { .polygon = AZ_INIT_POLYGON(turret_vertices),
                   .impact_damage = 10.0 },
    DECL_COMPONENTS(turret_components)
  },
  [AZ_BAD_ZIPPER] = {
    .max_health = 9.0, .color = {0, 255, 0, 255},
    .hurt_sound = AZ_SND_HURT_ZIPPER, .death_sound = AZ_SND_KILL_DRAGONFLY,
    .death_style = AZ_DEATH_EMBERS,
    .potential_pickups = AZ_PUPF_ALL, .static_properties = AZ_BADF_BOUNCE_PERP,
    .main_body = { .polygon = AZ_INIT_POLYGON(zipper_vertices),
                   .impact_damage = 10.0 }
  },
  [AZ_BAD_BOUNCER] = {
    .max_health = 3.0, .color = {255, 0, 0, 255},
    .hurt_sound = AZ_SND_HURT_BOUNCER, .death_sound = AZ_SND_KILL_BOUNCER,
    .death_style = AZ_DEATH_EMBERS,
    .potential_pickups = AZ_PUPF_ALL,
    .main_body = { .bounding_radius = 15.0, .impact_damage = 10.0 }
  },
  [AZ_BAD_ATOM] = {
    .max_health = 4.0, .overall_bounding_radius = 40.0,
    .potential_pickups = AZ_PUPF_ALL, .color = {64, 192, 64, 255},
    .death_sound = AZ_SND_KILL_ATOM, .armor_sound = AZ_SND_HIT_ARMOR,
    .hurt_sound = AZ_SND_HURT_FISH,
    .main_body = { .bounding_radius = 10.0, .impact_damage = 10.0 },
    DECL_COMPONENTS(atom_components)
  },
  [AZ_BAD_SPINER] = {
    .max_health = 6.0, .color = {128, 255, 0, 255},
    .hurt_sound = AZ_SND_HURT_BOUNCER, .death_sound = AZ_SND_KILL_BOUNCER,
    .death_style = AZ_DEATH_EMBERS, .potential_pickups = AZ_PUPF_ALL,
    .main_body = { .bounding_radius = 18.0, .impact_damage = 20.0 }
  },
  [AZ_BAD_BOX] = {
    .max_health = 6.0, .color = {160, 160, 160, 255},
    .potential_pickups = ~(AZ_PUPF_NOTHING | AZ_PUPF_LARGE_SHIELDS),
    .hurt_sound = AZ_SND_HURT_TURRET, .death_sound = AZ_SND_KILL_TURRET,
    .main_body = { .polygon = AZ_INIT_POLYGON(box_vertices),
                   .immunities = AZ_DMGF_FREEZE, .impact_damage = 0.0 }
  },
  [AZ_BAD_ARMORED_BOX] = {
    .max_health = 6.0, .color = {80, 80, 160, 255},
    .potential_pickups = ~(AZ_PUPF_NOTHING | AZ_PUPF_SMALL_SHIELDS),
    .hurt_sound = AZ_SND_HURT_TURRET, .armor_sound = AZ_SND_HIT_ARMOR,
    .death_sound = AZ_SND_KILL_TURRET,
    .static_properties = AZ_BADF_NO_HOMING_PHASE,
    .main_body = { .polygon = AZ_INIT_POLYGON(box_vertices),
                   .immunities = (AZ_DMGF_NORMAL | AZ_DMGF_CHARGED |
                                  AZ_DMGF_FREEZE | AZ_DMGF_FLAME),
                   .impact_damage = 0.0 }
  },
  [AZ_BAD_CLAM] = {
    .max_health = 6.0, .overall_bounding_radius = 31.5,
    .potential_pickups = AZ_PUPF_ALL, .color = {128, 0, 255, 255},
    .hurt_sound = AZ_SND_HURT_FISH, .armor_sound = AZ_SND_HIT_ARMOR,
    .death_sound = AZ_SND_KILL_TURRET, .static_properties = AZ_BADF_VULNERABLE,
    .main_body = { .bounding_radius = 10.0, .impact_damage = 10.0 },
    DECL_COMPONENTS(clam_components)
  },
  [AZ_BAD_NIGHTBUG] = {
    .max_health = 6.0, .color = {192, 96, 0, 255},
    .hurt_sound = AZ_SND_HURT_ZIPPER, .death_sound = AZ_SND_KILL_DRAGONFLY,
    .death_style = AZ_DEATH_EMBERS,
    .potential_pickups = ~AZ_PUPF_LARGE_SHIELDS,
    .main_body = { .polygon = AZ_INIT_POLYGON(nightbug_vertices),
                   .impact_damage = 10.0 }
  },
  [AZ_BAD_SPINE_MINE] = {
    .max_health = 3.0, .potential_pickups = ~AZ_PUPF_LARGE_SHIELDS,
    .color = {128, 255, 0, 255}, .death_sound = AZ_SND_KILL_BOUNCER,
    .main_body = { .bounding_radius = 9.0, .impact_damage = 20.0 }
  },
  [AZ_BAD_BROKEN_TURRET] = {
    .max_health = 2.0, .overall_bounding_radius = 30.5,
    .color = {160, 160, 160, 255},
    .potential_pickups = (AZ_PUPF_NOTHING | AZ_PUPF_SMALL_SHIELDS |
                          AZ_PUPF_ROCKETS | AZ_PUPF_BOMBS),
    .hurt_sound = AZ_SND_HURT_TURRET, .death_sound = AZ_SND_KILL_TURRET,
    .main_body = { .polygon = AZ_INIT_POLYGON(turret_vertices),
                   .impact_damage = 5.0 },
    DECL_COMPONENTS(broken_turret_components)
  },
  [AZ_BAD_ZENITH_CORE] = {
    .max_health = 1000.0, .overall_bounding_radius = 250.0,
    .hurt_sound = AZ_SND_HURT_TURRET,
    .static_properties = AZ_BADF_CARRIES_CARGO,
    .main_body = { .polygon = AZ_INIT_POLYGON(zenith_core_main_body_vertices),
                   .immunities = (AZ_DMGF_FREEZE | AZ_DMGF_CPLUS) },
    DECL_COMPONENTS(zenith_core_components)
  },
  [AZ_BAD_ARMORED_TURRET] = {
    .max_health = 5.0, .overall_bounding_radius = 30.5,
    .potential_pickups = ~AZ_PUPF_LARGE_SHIELDS, .color = {80, 80, 160, 255},
    .hurt_sound = AZ_SND_HURT_TURRET, .armor_sound = AZ_SND_HIT_ARMOR,
    .death_sound = AZ_SND_KILL_TURRET,
    .static_properties = AZ_BADF_NO_HOMING_PHASE,
    .main_body = { .polygon = AZ_INIT_POLYGON(turret_vertices),
                   .immunities = (AZ_DMGF_NORMAL | AZ_DMGF_FLAME),
                   .impact_damage = 10.0 },
    DECL_COMPONENTS(armored_turret_components)
  },
  [AZ_BAD_DRAGONFLY] = {
    .max_health = 5.0, .color = {255, 64, 0, 255},
    .potential_pickups = ~AZ_PUPF_LARGE_SHIELDS,
    .hurt_sound = AZ_SND_HURT_ZIPPER, .death_sound = AZ_SND_KILL_DRAGONFLY,
    .death_style = AZ_DEATH_EMBERS, .static_properties = AZ_BADF_WATER_BOUNCE,
    .main_body = { .polygon = AZ_INIT_POLYGON(dragonfly_vertices),
                   .impact_damage = 8.0 }
  },
  [AZ_BAD_CAVE_CRAWLER] = {
    .max_health = 4.0, .color = {128, 0, 128, 255},
    .potential_pickups = ~AZ_PUPF_LARGE_SHIELDS,
    .hurt_sound = AZ_SND_HURT_CRAWLER, .death_sound = AZ_SND_KILL_TURRET,
    .main_body = { .polygon = AZ_INIT_POLYGON(cave_crawler_vertices),
                   .impact_damage = 10.0 }
  },
  [AZ_BAD_CRAWLING_TURRET] = {
    .max_health = 7.0, .overall_bounding_radius = 30.5,
    .potential_pickups = ~AZ_PUPF_LARGE_SHIELDS, .color = {160, 160, 160, 255},
    .hurt_sound = AZ_SND_HURT_TURRET, .death_sound = AZ_SND_KILL_TURRET,
    .main_body = { .polygon = AZ_INIT_POLYGON(turret_vertices),
                   .impact_damage = 10.0 },
    DECL_COMPONENTS(crawling_turret_components)
  },
  [AZ_BAD_HORNET] = {
    .max_health = 3.0, .color = {192, 192, 0, 255},
    .hurt_sound = AZ_SND_HURT_ZIPPER, .death_sound = AZ_SND_KILL_DRAGONFLY,
    .death_style = AZ_DEATH_EMBERS, .static_properties = AZ_BADF_WATER_BOUNCE,
    .potential_pickups = ~AZ_PUPF_LARGE_SHIELDS,
    .main_body = { .polygon = AZ_INIT_POLYGON(dragonfly_vertices),
                   .impact_damage = 5.0 }
  },
  [AZ_BAD_BEAM_SENSOR] = {
    .max_health = 1000000.0, .overall_bounding_radius = 30.0,
    .color = {160, 160, 160, 255}, .death_sound = AZ_SND_KILL_TURRET,
    .static_properties = (AZ_BADF_DRAW_BG | AZ_BADF_NO_HOMING_PHASE |
                          AZ_BADF_NO_HOMING_PROJ | AZ_BADF_WALL_LIKE),
    .main_body = { .polygon = AZ_INIT_POLYGON(beam_sensor_target_vertices),
                   .immunities = ~AZ_DMGF_BEAM },
    DECL_COMPONENTS(beam_sensor_components)
  },
  [AZ_BAD_BEAM_SENSOR_INV] = {
    .max_health = 1000000.0, .overall_bounding_radius = 30.0,
    .color = {160, 160, 160, 255}, .death_sound = AZ_SND_KILL_TURRET,
    .static_properties = (AZ_BADF_DRAW_BG | AZ_BADF_NO_HOMING_PHASE |
                          AZ_BADF_NO_HOMING_PROJ | AZ_BADF_WALL_LIKE),
    .main_body = { .polygon = AZ_INIT_POLYGON(beam_sensor_target_vertices),
                   .immunities = ~AZ_DMGF_BEAM },
    DECL_COMPONENTS(beam_sensor_inv_components)
  },
  [AZ_BAD_ROCKWYRM] = {
    .max_health = 150.0, .overall_bounding_radius = 500.0,
    .color = {192, 255, 128, 255}, .hurt_sound = AZ_SND_HURT_ROCKWYRM,
    .potential_pickups = ~AZ_PUPF_NOTHING,
    .main_body = { .bounding_radius = 24.0, .impact_damage = 5.0,
                   .immunities = (AZ_DMGF_FREEZE | AZ_DMGF_CPLUS) },
    DECL_COMPONENTS(rockwyrm_components)
  },
  [AZ_BAD_WYRM_EGG] = {
    .max_health = 2.0,
    .potential_pickups = (AZ_PUPF_NOTHING | AZ_PUPF_SMALL_SHIELDS),
    .static_properties = AZ_BADF_DRAW_BG, .color = {128, 255, 255, 255},
    .death_sound = AZ_SND_KILL_TURRET, .death_style = AZ_DEATH_SHARDS,
    .main_body = { .bounding_radius = 10.0 }
  },
  [AZ_BAD_WYRMLING] = {
    .max_health = 0.1, .overall_bounding_radius = 30.0,
    .potential_pickups = ~(AZ_PUPF_MEDIUM_SHIELDS | AZ_PUPF_LARGE_SHIELDS),
    .color = {192, 255, 128, 255},
    .death_sound = AZ_SND_KILL_SWOOPER, .death_style = AZ_DEATH_EMBERS,
    .main_body = { .bounding_radius = 5.0, .impact_damage = 3.0,
                   .immunities = AZ_DMGF_FREEZE },
    DECL_COMPONENTS(wyrmling_components)
  },
  [AZ_BAD_TRAPDOOR] = {
    .max_health = 3.0, .overall_bounding_radius = 95.6,
    .static_properties = (AZ_BADF_DRAW_BG | AZ_BADF_NO_HOMING |
                          AZ_BADF_WALL_LIKE),
    .color = {160, 160, 160, 255},
    .death_sound = AZ_SND_KILL_TURRET, .death_style = AZ_DEATH_SHARDS,
    .main_body = { .polygon = AZ_INIT_POLYGON(trapdoor_hinge_vertices),
                   .immunities = (AZ_DMGF_NORMAL | AZ_DMGF_CHARGED |
                                  AZ_DMGF_PIERCE | AZ_DMGF_ROCKET |
                                  AZ_DMGF_FLAME) },
    DECL_COMPONENTS(trapdoor_components)
  },
  [AZ_BAD_CAVE_SWOOPER] = {
    .max_health = 3.0,
    .potential_pickups = ~AZ_PUPF_LARGE_SHIELDS, .color = {0, 128, 0, 255},
    .hurt_sound = AZ_SND_HURT_SWOOPER, .death_sound = AZ_SND_KILL_SWOOPER,
    .death_style = AZ_DEATH_EMBERS, .static_properties = AZ_BADF_WATER_BOUNCE,
    .main_body = { .polygon = AZ_INIT_POLYGON(swooper_vertices),
                   .impact_damage = 8.0 }
  },
  [AZ_BAD_ICE_CRAWLER] = {
    .max_health = 5.0, .overall_bounding_radius = 18.9,
    .potential_pickups = ~AZ_PUPF_LARGE_SHIELDS, .color = {0, 128, 128, 255},
    .hurt_sound = AZ_SND_HURT_CRAWLER, .death_sound = AZ_SND_KILL_TURRET,
    .main_body = { .polygon = AZ_INIT_POLYGON(ice_crawler_body_vertices),
                   .impact_damage = 10.0, .immunities = AZ_DMGF_FREEZE },
    DECL_COMPONENTS(ice_crawler_components)
  },
  [AZ_BAD_BEAM_TURRET] = {
    .max_health = 20.0, .overall_bounding_radius = 30.5,
    .potential_pickups = AZ_PUPF_ALL, .color = {80, 160, 120, 255},
    .hurt_sound = AZ_SND_HURT_TURRET, .death_sound = AZ_SND_KILL_TURRET,
    .armor_sound = AZ_SND_HIT_ARMOR,
    .static_properties = AZ_BADF_NO_HOMING_PHASE,
    .main_body = { .polygon = AZ_INIT_POLYGON(turret_vertices),
                   .immunities = (AZ_DMGF_NORMAL | AZ_DMGF_FLAME),
                   .impact_damage = 10.0 },
    DECL_COMPONENTS(beam_turret_components)
  },
  [AZ_BAD_OTH_CRAB_1] = {
    .max_health = 50.0, .color = {255, 255, 255, 255},
    .potential_pickups = ~AZ_PUPF_NOTHING,
    .hurt_sound = AZ_SND_HURT_OTH, .death_sound = AZ_SND_KILL_OTH,
    .death_style = AZ_DEATH_OTH,
    .main_body = { .polygon = AZ_INIT_POLYGON(oth_crab_vertices),
                   .impact_damage = 20.0 }
  },
  [AZ_BAD_OTH_ORB_1] = {
    .max_health = 30.0, .color = {255, 255, 255, 255},
    .potential_pickups = ~AZ_PUPF_NOTHING,
    .hurt_sound = AZ_SND_HURT_OTH, .death_sound = AZ_SND_KILL_OTH,
    .death_style = AZ_DEATH_OTH,
    .main_body = { .bounding_radius = 19.0, .impact_damage = 20.0 }
  },
  [AZ_BAD_OTH_SNAPDRAGON] = {
    .max_health = 60.0,  .color = {255, 255, 255, 255},
    .death_style = AZ_DEATH_OTH,
    .hurt_sound = AZ_SND_HURT_OTH, .death_sound = AZ_SND_KILL_OTH,
    .potential_pickups = ~AZ_PUPF_LARGE_SHIELDS,
    .main_body = { .bounding_radius = 30.0, .impact_damage = 6.0,
                   .immunities = (AZ_DMGF_FREEZE | AZ_DMGF_CPLUS) }
  },
  [AZ_BAD_OTH_RAZOR_1] = {
    .max_health = 0.1, .color = {255, 255, 255, 255},
    .death_sound = AZ_SND_KILL_OTH, .death_style = AZ_DEATH_OTH,
    .potential_pickups = AZ_PUPF_SMALL_SHIELDS,
    .static_properties = AZ_BADF_KAMIKAZE,
    .main_body = { .bounding_radius = 15.0, .impact_damage = 3.0 }
  },
  [AZ_BAD_GUN_SENSOR] = {
    .max_health = 1000000.0, .overall_bounding_radius = 30.0,
    .color = {160, 160, 160, 255}, .death_sound = AZ_SND_KILL_TURRET,
    .static_properties = (AZ_BADF_DRAW_BG | AZ_BADF_WALL_LIKE),
    .main_body = { .bounding_radius = 15.0,
                   .immunities = (AZ_DMGF_FREEZE | AZ_DMGF_CPLUS |
                                  AZ_DMGF_BOMB | AZ_DMGF_MEGA_BOMB |
                                  AZ_DMGF_REACTIVE) },
    DECL_COMPONENTS(gun_sensor_components)
  },
  [AZ_BAD_SECURITY_DRONE] = {
    .max_health = 8.0, .overall_bounding_radius = 31.7,
    .potential_pickups = ~(AZ_PUPF_NOTHING | AZ_PUPF_SMALL_SHIELDS),
    .color = {160, 160, 160, 255},
    .hurt_sound = AZ_SND_HURT_TURRET, .death_sound = AZ_SND_KILL_TURRET,
    .main_body = { .polygon = AZ_INIT_POLYGON(turret_vertices),
                   .impact_damage = 10.0 },
    DECL_COMPONENTS(security_drone_components)
  },
  [AZ_BAD_SMALL_TRUCK] = {
    .max_health = 30.0, .color = {160, 160, 160, 255},
    .potential_pickups = ~(AZ_PUPF_NOTHING | AZ_PUPF_SMALL_SHIELDS),
    .hurt_sound = AZ_SND_HURT_TURRET, .death_sound = AZ_SND_KILL_TURRET,
    .static_properties = (AZ_BADF_CARRIES_CARGO | AZ_BADF_NO_HOMING_PHASE),
    .main_body = { .polygon = AZ_INIT_POLYGON(small_truck_vertices),
                   .immunities = (AZ_DMGF_NORMAL | AZ_DMGF_CHARGED |
                                  AZ_DMGF_PIERCE | AZ_DMGF_BEAM |
                                  AZ_DMGF_FLAME),
                   .impact_damage = 8.0 }
  },
  [AZ_BAD_HEAT_RAY] = {
    .max_health = 40.0, .potential_pickups = ~AZ_PUPF_LARGE_SHIELDS,
    .color = {160, 160, 160, 255}, .death_sound = AZ_SND_KILL_TURRET,
    .static_properties = AZ_BADF_NO_HOMING,
    .main_body = { .polygon = AZ_INIT_POLYGON(heat_ray_vertices),
                   .immunities = ~(AZ_DMGF_CPLUS | AZ_DMGF_HYPER_ROCKET) }
  },
  [AZ_BAD_NUCLEAR_MINE] = {
    .max_health = 7.0, .potential_pickups = AZ_PUPF_ALL,
    .color = {160, 160, 128, 255},
    .hurt_sound = AZ_SND_HURT_TURRET, .death_sound = AZ_SND_KILL_TURRET,
    .main_body = { .polygon = AZ_INIT_POLYGON(nuclear_mine_vertices),
                   .immunities = AZ_DMGF_BOMB }
  },
  [AZ_BAD_BEAM_WALL] = {
    .max_health = 25.0, .potential_pickups = AZ_PUPF_NOTHING,
    .color = {160, 160, 160, 255}, .death_sound = AZ_SND_KILL_TURRET,
    .static_properties = (AZ_BADF_NO_HOMING | AZ_BADF_WALL_LIKE),
    .main_body = { .polygon = AZ_INIT_POLYGON(beam_wall_vertices),
                   .immunities = ~AZ_DMGF_BEAM }
  },
  [AZ_BAD_SPARK] = {
    .max_health = 1000000.0,
    .static_properties = (AZ_BADF_INCORPOREAL | AZ_BADF_NO_HOMING),
    .main_body = { .bounding_radius = 10.0 }
  },
  [AZ_BAD_MOSQUITO] = {
    .max_health = 0.75, .color = {255, 64, 0, 255},
    .hurt_sound = AZ_SND_HURT_ZIPPER, .death_sound = AZ_SND_KILL_DRAGONFLY,
    .death_style = AZ_DEATH_EMBERS,
    .potential_pickups = ~AZ_PUPF_LARGE_SHIELDS,
    .static_properties = (AZ_BADF_KAMIKAZE | AZ_BADF_WATER_BOUNCE),
    .main_body = { .polygon = AZ_INIT_POLYGON(mini_zipper_vertices),
                   .impact_damage = 4.0 }
  },
  [AZ_BAD_ARMORED_ZIPPER] = {
    .max_health = 9.0, .color = {128, 160, 128, 255},
    .hurt_sound = AZ_SND_HURT_ZIPPER, .armor_sound = AZ_SND_HIT_ARMOR,
    .death_sound = AZ_SND_KILL_DRAGONFLY, .death_style = AZ_DEATH_SHARDS,
    .potential_pickups = AZ_PUPF_ALL,
    .static_properties = (AZ_BADF_BOUNCE_PERP | AZ_BADF_NO_HOMING_PHASE),
    .main_body = { .polygon = AZ_INIT_POLYGON(zipper_vertices),
                   .immunities = (AZ_DMGF_NORMAL | AZ_DMGF_CHARGED |
                                  AZ_DMGF_FLAME),
                   .impact_damage = 10.0 }
  },
  [AZ_BAD_FORCEFIEND] = {
    .max_health = 450.0, .overall_bounding_radius = 150.0,
    .color = {192, 128, 255, 255}, .death_style = AZ_DEATH_EMBERS,
    .hurt_sound = AZ_SND_HURT_ROCKWYRM, .armor_sound = AZ_SND_HIT_ARMOR,
    .potential_pickups = ~AZ_PUPF_NOTHING,
    .main_body = { .polygon = AZ_INIT_POLYGON(forcefiend_segment0_vertices),
                   .immunities = (AZ_DMGF_FREEZE | AZ_DMGF_CPLUS),
                   .impact_damage = 4.0 },
    DECL_COMPONENTS(forcefiend_components)
  },
  [AZ_BAD_CHOMPER_PLANT] = {
    .max_health = 11.0, .overall_bounding_radius = 250.0,
    .potential_pickups = AZ_PUPF_ALL, .color = {32, 128, 0, 255},
    .hurt_sound = AZ_SND_HURT_PLANT, .death_sound = AZ_SND_KILL_PLANT,
    .death_style = AZ_DEATH_EMBERS, .static_properties = AZ_BADF_VULNERABLE,
    .main_body = { .polygon = AZ_INIT_POLYGON(chomper_plant_core_vertices),
                   .impact_damage = 10.0 },
    DECL_COMPONENTS(chomper_plant_components)
  },
  [AZ_BAD_COPTER_HORZ] = {
    .max_health = 30.0,
    .potential_pickups = ~(AZ_PUPF_NOTHING | AZ_PUPF_SMALL_SHIELDS),
    .color = {160, 160, 160, 255},
    .hurt_sound = AZ_SND_HURT_TURRET, .death_sound = AZ_SND_KILL_TURRET,
    .static_properties = (AZ_BADF_CARRIES_CARGO | AZ_BADF_NO_HOMING_PHASE),
    .main_body = { .polygon = AZ_INIT_POLYGON(copter_vertices),
                   .immunities = (AZ_DMGF_NORMAL | AZ_DMGF_CHARGED |
                                  AZ_DMGF_PIERCE | AZ_DMGF_BEAM |
                                  AZ_DMGF_FLAME),
                   .impact_damage = 8.0 }
  },
  [AZ_BAD_COPTER_VERT] = {
    .max_health = 30.0,
    .potential_pickups = ~(AZ_PUPF_NOTHING | AZ_PUPF_SMALL_SHIELDS),
    .color = {160, 160, 160, 255},
    .hurt_sound = AZ_SND_HURT_TURRET, .death_sound = AZ_SND_KILL_TURRET,
    .static_properties = (AZ_BADF_CARRIES_CARGO | AZ_BADF_NO_HOMING_PHASE),
    .main_body = { .polygon = AZ_INIT_POLYGON(copter_vertices),
                   .immunities = (AZ_DMGF_NORMAL | AZ_DMGF_CHARGED |
                                  AZ_DMGF_PIERCE | AZ_DMGF_BEAM |
                                  AZ_DMGF_FLAME),
                   .impact_damage = 8.0 }
  },
  [AZ_BAD_URCHIN] = {
    .max_health = 2.0, .potential_pickups = ~AZ_PUPF_LARGE_SHIELDS,
    .color = {128, 255, 0, 255}, .death_sound = AZ_SND_KILL_BOUNCER,
    .main_body = { .bounding_radius = 9.0, .impact_damage = 12.0 }
  },
  [AZ_BAD_BOSS_DOOR] = {
    .max_health = 24.0, .overall_bounding_radius = 55.0,
    .potential_pickups = AZ_PUPF_LARGE_SHIELDS,
    .color = {160, 160, 160, 255}, .death_sound = AZ_SND_KILL_TURRET,
    .main_body = { .polygon = AZ_INIT_POLYGON(boss_door_body_vertices),
                   .impact_damage = 10.0, .immunities = ~0 },
    DECL_COMPONENTS(boss_door_components)
  },
  [AZ_BAD_ROCKET_TURRET] = {
    .max_health = 20.0, .overall_bounding_radius = 30.5,
    .potential_pickups = AZ_PUPF_ALL, .color = {160, 80, 120, 255},
    .hurt_sound = AZ_SND_HURT_TURRET, .armor_sound = AZ_SND_HIT_ARMOR,
    .death_sound = AZ_SND_KILL_TURRET,
    .static_properties = AZ_BADF_NO_HOMING_PHASE,
    .main_body = { .polygon = AZ_INIT_POLYGON(turret_vertices),
                   .immunities = (AZ_DMGF_NORMAL | AZ_DMGF_FLAME),
                   .impact_damage = 10.0 },
    DECL_COMPONENTS(rocket_turret_components)
  },
  [AZ_BAD_MINI_ARMORED_ZIPPER] = {
    .max_health = 4.0, .color = {128, 160, 128, 255},
    .hurt_sound = AZ_SND_HURT_ZIPPER, .armor_sound = AZ_SND_HIT_ARMOR,
    .death_sound = AZ_SND_KILL_DRAGONFLY, .death_style = AZ_DEATH_SHARDS,
    .potential_pickups = AZ_PUPF_ALL,
    .static_properties = (AZ_BADF_BOUNCE_PERP | AZ_BADF_NO_HOMING_PHASE),
    .main_body = { .polygon = AZ_INIT_POLYGON(mini_zipper_vertices),
                   .immunities = (AZ_DMGF_NORMAL | AZ_DMGF_CHARGED |
                                  AZ_DMGF_FLAME),
                   .impact_damage = 10.0 }
  },
  [AZ_BAD_OTH_CRAB_2] = {
    .max_health = 60.0, .color = {255, 255, 255, 255},
    .potential_pickups = AZ_PUPF_LARGE_SHIELDS,
    .hurt_sound = AZ_SND_HURT_OTH, .death_sound = AZ_SND_KILL_OTH,
    .death_style = AZ_DEATH_OTH,
    .main_body = { .polygon = AZ_INIT_POLYGON(oth_crab_vertices),
                   .impact_damage = 20.0 }
  },
  [AZ_BAD_SPINED_CRAWLER] = {
    .max_health = 6.0, .potential_pickups = AZ_PUPF_ALL,
    .color = {32, 128, 64, 255},
    .hurt_sound = AZ_SND_HURT_CRAWLER, .death_sound = AZ_SND_KILL_TURRET,
    .main_body = { .polygon = AZ_INIT_POLYGON(spined_crawler_vertices),
                   .impact_damage = 12.0 }
  },
  [AZ_BAD_DEATH_RAY] = {
    .max_health = 40.0, .potential_pickups = AZ_PUPF_ALL,
    .color = {160, 160, 160, 255}, .death_sound = AZ_SND_KILL_TURRET,
    .static_properties = AZ_BADF_NO_HOMING,
    .main_body = { .polygon = AZ_INIT_POLYGON(heat_ray_vertices),
                   .immunities = ~AZ_DMGF_CPLUS }
  },
  [AZ_BAD_OTH_GUNSHIP] = {
    .max_health = 250.0, .color = {255, 255, 255, 255},
    .potential_pickups = ~AZ_PUPF_NOTHING, .hurt_sound = AZ_SND_HURT_OTH,
    .death_sound = AZ_SND_KILL_OTH, .death_style = AZ_DEATH_OTH,
    .main_body = { .polygon = AZ_INIT_POLYGON(oth_gunship_vertices),
                   .immunities = AZ_DMGF_FREEZE, .impact_damage = 10.0 }
  },
  [AZ_BAD_FIREBALL_MINE] = {
    .max_health = 10.0, .potential_pickups = ~AZ_PUPF_LARGE_SHIELDS,
    .color = {128, 128, 128, 255},
    .hurt_sound = AZ_SND_HURT_TURRET, .armor_sound = AZ_SND_HIT_ARMOR,
    .death_sound = AZ_SND_KILL_TURRET, .death_style = AZ_DEATH_SHARDS,
    .static_properties = (AZ_BADF_DRAW_BG | AZ_BADF_NO_HOMING_PHASE),
    .main_body = { .bounding_radius = 15.0, .impact_damage = 8.0,
                   .immunities = (AZ_DMGF_NORMAL | AZ_DMGF_FREEZE) }
  },
  [AZ_BAD_LEAPER] = {
    .max_health = 8.0, .color = {0, 255, 128, 255},
    .death_sound = AZ_SND_KILL_TURRET, .death_style = AZ_DEATH_EMBERS,
    .potential_pickups = AZ_PUPF_ALL,
    .main_body = { .polygon = AZ_INIT_POLYGON(leaper_vertices),
                   .impact_damage = 9.0 }
  },
  [AZ_BAD_BOUNCER_90] = {
    .max_health = 6.0, .color = {0, 255, 255, 255},
    .hurt_sound = AZ_SND_HURT_BOUNCER, .death_sound = AZ_SND_KILL_BOUNCER,
    .death_style = AZ_DEATH_EMBERS, .potential_pickups = AZ_PUPF_ALL,
    .main_body = { .bounding_radius = 15.0, .impact_damage = 12.0 }
  },
  [AZ_BAD_PISTON] = {
    .max_health = 6.0, .overall_bounding_radius = 110.0,
    .hurt_sound = AZ_SND_HURT_TURRET, .armor_sound = AZ_SND_HIT_ARMOR,
    .death_sound = AZ_SND_KILL_TURRET,
    .static_properties = (AZ_BADF_CARRIES_CARGO | AZ_BADF_DRAW_BG |
                          AZ_BADF_NO_HOMING),
    .color = {96, 96, 128, 255},
    .main_body = { .polygon = AZ_INIT_POLYGON(piston_head_vertices),
                   .immunities = (AZ_DMGF_NORMAL | AZ_DMGF_FLAME),
                   .impact_damage = 10.0 },
    DECL_COMPONENTS(piston_components)
  },
  [AZ_BAD_ARMORED_PISTON] = {
    .max_health = 19.0, .overall_bounding_radius = 110.0,
    .hurt_sound = AZ_SND_HURT_TURRET, .death_sound = AZ_SND_KILL_TURRET,
    .static_properties = (AZ_BADF_CARRIES_CARGO | AZ_BADF_DRAW_BG |
                          AZ_BADF_NO_HOMING),
    .color = {128, 96, 96, 255},
    .main_body = { .polygon = AZ_INIT_POLYGON(piston_head_vertices),
                   .immunities = (AZ_DMGF_NORMAL | AZ_DMGF_CHARGED |
                                  AZ_DMGF_FLAME),
                   .impact_damage = 10.0 },
    DECL_COMPONENTS(armored_piston_components)
  },
  [AZ_BAD_ARMORED_PISTON_EXT] = {
    .max_health = 19.0, .overall_bounding_radius = 110.0,
    .hurt_sound = AZ_SND_HURT_TURRET, .death_sound = AZ_SND_KILL_TURRET,
    .static_properties = (AZ_BADF_CARRIES_CARGO | AZ_BADF_DRAW_BG |
                          AZ_BADF_NO_HOMING),
    .color = {128, 96, 96, 255},
    .main_body = { .polygon = AZ_INIT_POLYGON(piston_head_vertices),
                   .immunities = (AZ_DMGF_NORMAL | AZ_DMGF_CHARGED |
                                  AZ_DMGF_FLAME),
                   .impact_damage = 10.0 },
    DECL_COMPONENTS(armored_piston_ext_components)
  },
  [AZ_BAD_INCORPOREAL_PISTON] = {
    .max_health = 1000000.0, .overall_bounding_radius = 110.0,
    .death_sound = AZ_SND_KILL_TURRET, .death_style = AZ_DEATH_SHARDS,
    .static_properties = (AZ_BADF_CARRIES_CARGO | AZ_BADF_DRAW_BG |
                          AZ_BADF_INCORPOREAL | AZ_BADF_NO_HOMING),
    .color = {96, 96, 96, 255},
    .main_body = { .polygon = AZ_INIT_POLYGON(piston_head_vertices) },
    DECL_COMPONENTS(incorporeal_piston_components)
  },
  [AZ_BAD_INCORPOREAL_PISTON_EXT] = {
    .max_health = 1000000.0, .overall_bounding_radius = 110.0,
    .death_sound = AZ_SND_KILL_TURRET, .death_style = AZ_DEATH_SHARDS,
    .static_properties = (AZ_BADF_CARRIES_CARGO | AZ_BADF_DRAW_BG |
                          AZ_BADF_INCORPOREAL | AZ_BADF_NO_HOMING),
    .color = {96, 96, 96, 255},
    .main_body = { .polygon = AZ_INIT_POLYGON(piston_head_vertices) },
    DECL_COMPONENTS(incorporeal_piston_ext_components)
  },
  [AZ_BAD_CRAWLING_MORTAR] = {
    .max_health = 12.0, .overall_bounding_radius = 30.5,
    .potential_pickups = AZ_PUPF_ALL, .color = {96, 96, 96, 255},
    .hurt_sound = AZ_SND_HURT_TURRET, .death_sound = AZ_SND_KILL_TURRET,
    .main_body = { .polygon = AZ_INIT_POLYGON(turret_vertices),
                   .immunities = AZ_DMGF_FLAME, .impact_damage = 15.0 },
    DECL_COMPONENTS(crawling_mortar_components)
  },
  [AZ_BAD_OTH_ORB_2] = {
    .max_health = 50.0, .color = {255, 255, 255, 255},
    .potential_pickups = ~(AZ_PUPF_NOTHING | AZ_PUPF_SMALL_SHIELDS),
    .hurt_sound = AZ_SND_HURT_OTH, .death_sound = AZ_SND_KILL_OTH,
    .death_style = AZ_DEATH_OTH,
    .main_body = { .bounding_radius = 19.0, .impact_damage = 20.0 }
  },
  [AZ_BAD_FIRE_ZIPPER] = {
    .max_health = 11.0, .color = {128, 0, 255, 255},
    .hurt_sound = AZ_SND_HURT_ZIPPER, .death_sound = AZ_SND_KILL_DRAGONFLY,
    .death_style = AZ_DEATH_EMBERS,
    .potential_pickups = AZ_PUPF_ALL, .static_properties = AZ_BADF_BOUNCE_PERP,
    .main_body = { .polygon = AZ_INIT_POLYGON(zipper_vertices),
                   .impact_damage = 12.0, .immunities = AZ_DMGF_FLAME }
  },
  [AZ_BAD_SUPER_SPINER] = {
    .max_health = 10.0, .color = {0, 192, 64, 255},
    .hurt_sound = AZ_SND_HURT_BOUNCER, .death_sound = AZ_SND_KILL_BOUNCER,
    .death_style = AZ_DEATH_EMBERS,
    .main_body = { .bounding_radius = 18.0, .impact_damage = 25.0 }
  },
  [AZ_BAD_HEAVY_TURRET] = {
    .max_health = 12.0, .overall_bounding_radius = 31.7,
    .potential_pickups = AZ_PUPF_ALL, .color = {80, 80, 160, 255},
    .hurt_sound = AZ_SND_HURT_TURRET, .armor_sound = AZ_SND_HIT_ARMOR,
    .death_sound = AZ_SND_KILL_TURRET, .death_style = AZ_DEATH_SHARDS,
    .static_properties = (AZ_BADF_DRAW_BG | AZ_BADF_NO_HOMING_PHASE),
    .main_body = { .polygon = AZ_INIT_POLYGON(turret_vertices),
                   .immunities = (AZ_DMGF_NORMAL | AZ_DMGF_FLAME),
                   .impact_damage = 10.0 },
    DECL_COMPONENTS(heavy_turret_components)
  },
  [AZ_BAD_ECHO_SWOOPER] = {
    .max_health = 7.0,
    .potential_pickups = AZ_PUPF_ALL, .color = {105, 0, 192, 255},
    .hurt_sound = AZ_SND_HURT_SWOOPER, .death_sound = AZ_SND_KILL_SWOOPER,
    .death_style = AZ_DEATH_EMBERS, .static_properties = AZ_BADF_WATER_BOUNCE,
    .main_body = { .polygon = AZ_INIT_POLYGON(swooper_vertices),
                   .impact_damage = 8.0 }
  },
  [AZ_BAD_SUPER_HORNET] = {
    .max_health = 8.0, .color = {128, 255, 0, 255},
    .hurt_sound = AZ_SND_HURT_ZIPPER, .death_sound = AZ_SND_KILL_DRAGONFLY,
    .death_style = AZ_DEATH_EMBERS, .potential_pickups = AZ_PUPF_ALL,
    .static_properties = AZ_BADF_WATER_BOUNCE,
    .main_body = { .polygon = AZ_INIT_POLYGON(dragonfly_vertices),
                   .impact_damage = 12.0 }
  },
  [AZ_BAD_KILOFUGE] = {
    .max_health = 500.0, .overall_bounding_radius = 350.0,
    .potential_pickups = ~AZ_PUPF_NOTHING,
    .hurt_sound = AZ_SND_HURT_KILOFUGE, .static_properties = AZ_BADF_DRAW_BG,
    .main_body = { .polygon = AZ_INIT_POLYGON(kilofuge_main_body_vertices),
                   .impact_damage = 25.0, .immunities = ~0 },
    DECL_COMPONENTS(kilofuge_components)
  },
  [AZ_BAD_ICE_CRYSTAL] = {
    .max_health = 6.0, .color = {64, 192, 192, 255},
    .death_sound = AZ_SND_KILL_TURRET, .death_style = AZ_DEATH_SHARDS,
    .static_properties = (AZ_BADF_NO_HOMING_PHASE | AZ_BADF_NO_HOMING_PROJ),
    .main_body = { .polygon = AZ_INIT_POLYGON(ice_crystal_vertices),
                   .impact_damage = 8.0,
                   .immunities = (AZ_DMGF_NORMAL | AZ_DMGF_FREEZE) }
  },
  [AZ_BAD_SWITCHER] = {
    .max_health = 9.0, .color = {0, 192, 192, 255},
    .hurt_sound = AZ_SND_HURT_ZIPPER, .death_sound = AZ_SND_KILL_DRAGONFLY,
    .death_style = AZ_DEATH_EMBERS,
    .potential_pickups = AZ_PUPF_ALL, .static_properties = AZ_BADF_BOUNCE_PERP,
    .main_body = { .polygon = AZ_INIT_POLYGON(switcher_vertices),
                   .impact_damage = 12.0 }
  },
  [AZ_BAD_FAST_BOUNCER] = {
    .max_health = 5.0, .color = {255, 96, 0, 255},
    .hurt_sound = AZ_SND_HURT_BOUNCER, .death_sound = AZ_SND_KILL_BOUNCER,
    .death_style = AZ_DEATH_EMBERS, .potential_pickups = AZ_PUPF_ALL,
    .main_body = { .bounding_radius = 15.0, .impact_damage = 15.0 }
  },
  [AZ_BAD_PROXY_MINE] = {
    .max_health = 5.0, .potential_pickups = AZ_PUPF_ALL,
    .color = {128, 128, 128, 255},
    .hurt_sound = AZ_SND_HURT_TURRET, .death_sound = AZ_SND_KILL_TURRET,
    .main_body = { .polygon = AZ_INIT_POLYGON(proxy_mine_vertices) }
  },
  [AZ_BAD_NIGHTSHADE] = {
    .max_health = 10.0, .overall_bounding_radius = 50.0,
    .hurt_sound = AZ_SND_HURT_ZIPPER, .death_sound = AZ_SND_KILL_DRAGONFLY,
    .death_style = AZ_DEATH_EMBERS,
    .color = {192, 96, 0, 255}, .potential_pickups = AZ_PUPF_ALL,
    .main_body = { .polygon = AZ_INIT_POLYGON(nightshade_body_vertices),
                   .impact_damage = 15.0 },
    DECL_COMPONENTS(nightshade_components)
  },
  [AZ_BAD_AQUATIC_CHOMPER] = {
    .max_health = 9.5, .overall_bounding_radius = 130.0,
    .potential_pickups = AZ_PUPF_ALL, .color = {96, 32, 192, 255},
    .hurt_sound = AZ_SND_HURT_FISH, .death_sound = AZ_SND_KILL_FISH,
    .death_style = AZ_DEATH_EMBERS,
    .static_properties = (AZ_BADF_DRAW_BG | AZ_BADF_VULNERABLE),
    .main_body = { .polygon = AZ_INIT_POLYGON(chomper_plant_core_vertices),
                   .impact_damage = 10.0 },
    DECL_COMPONENTS(aquatic_chomper_components)
  },
  [AZ_BAD_SMALL_FISH] = {
    .max_health = 7.0, .overall_bounding_radius = 45.0,
    .potential_pickups = ~AZ_PUPF_LARGE_SHIELDS, .color = {230, 0, 105, 255},
    .hurt_sound = AZ_SND_HURT_FISH, .death_sound = AZ_SND_KILL_FISH,
    .death_style = AZ_DEATH_EMBERS,
    .static_properties = (AZ_BADF_DRAW_BG | AZ_BADF_WATER_BOUNCE),
    .main_body = { .polygon = AZ_INIT_POLYGON(small_fish_head_vertices),
                   .impact_damage = 15.0 },
    DECL_COMPONENTS(small_fish_components)
  },
  [AZ_BAD_NOCTURNE] = {
    .max_health = 500.0, .overall_bounding_radius = 120.0,
    .color = {192, 96, 0, 255}, .potential_pickups = ~AZ_PUPF_NOTHING,
    .hurt_sound = AZ_SND_HURT_NOCTURNE, .armor_sound = AZ_SND_HIT_ARMOR,
    .main_body = { .polygon = AZ_INIT_POLYGON(nocturne_main_body_vertices),
                   .immunities = (AZ_DMGF_FREEZE | AZ_DMGF_CPLUS),
                   .impact_damage = 20.0 },
    DECL_COMPONENTS(nocturne_components)
  },
  [AZ_BAD_MYCOFLAKKER] = {
    .max_health = 5.0,
    .color = {160, 80, 255, 255}, .potential_pickups = AZ_PUPF_ALL,
    .hurt_sound = AZ_SND_HURT_PLANT, .death_sound = AZ_SND_KILL_PLANT,
    .death_style = AZ_DEATH_EMBERS,
    .main_body = { .polygon = AZ_INIT_POLYGON(mycoflakker_vertices),
                   .impact_damage = 10.0 }
  },
  [AZ_BAD_MYCOSTALKER] = {
    .max_health = 7.0,
    .color = {192, 80, 224, 255}, .potential_pickups = AZ_PUPF_ALL,
    .hurt_sound = AZ_SND_HURT_PLANT, .death_sound = AZ_SND_KILL_PLANT,
    .death_style = AZ_DEATH_EMBERS,
    .main_body = { .polygon = AZ_INIT_POLYGON(mycostalker_vertices),
                   .impact_damage = 12.0 }
  },
  [AZ_BAD_OTH_CRAWLER] = {
    .max_health = 20.0, .color = {255, 255, 255, 255},
    .potential_pickups = ~(AZ_PUPF_NOTHING | AZ_PUPF_SMALL_SHIELDS),
    .hurt_sound = AZ_SND_HURT_OTH, .death_sound = AZ_SND_KILL_OTH,
    .death_style = AZ_DEATH_OTH,
    .main_body = { .bounding_radius = 16.0, .impact_damage = 20.0 }
  },
  [AZ_BAD_FIRE_CRAWLER] = {
    .max_health = 8.0,
    .potential_pickups = AZ_PUPF_ALL, .color = {255, 96, 0, 255},
    .hurt_sound = AZ_SND_HURT_CRAWLER, .death_sound = AZ_SND_KILL_FIRE_CRAWLER,
    .death_style = AZ_DEATH_EMBERS,
    .main_body = { .polygon = AZ_INIT_POLYGON(fire_crawler_vertices),
                   .immunities = (AZ_DMGF_FREEZE | AZ_DMGF_FLAME),
                   .impact_damage = 20.0 }
  },
  [AZ_BAD_JUNGLE_CRAWLER] = {
    .max_health = 7.0, .color = {0, 128, 0, 255},
    .potential_pickups = AZ_PUPF_ALL,
    .hurt_sound = AZ_SND_HURT_CRAWLER, .death_sound = AZ_SND_KILL_TURRET,
    .main_body = { .polygon = AZ_INIT_POLYGON(cave_crawler_vertices),
                   .impact_damage = 15.0 }
  },
  [AZ_BAD_FORCE_EGG] = {
    .max_health = 5.0, .potential_pickups = ~AZ_PUPF_LARGE_SHIELDS,
    .static_properties = AZ_BADF_DRAW_BG, .color = {168, 142, 102, 255},
    .hurt_sound = AZ_SND_HURT_TURRET, .death_sound = AZ_SND_KILL_BOUNCER,
    .main_body = { .bounding_radius = 12.0, .impact_damage = 5.0,
                   .immunities = AZ_DMGF_FREEZE }
  },
  [AZ_BAD_FORCELING] = {
    .max_health = 0.75, .overall_bounding_radius = 22.5,
    .potential_pickups = ~(AZ_PUPF_MEDIUM_SHIELDS | AZ_PUPF_LARGE_SHIELDS),
    .color = {204, 25, 76, 255},
    .hurt_sound = AZ_SND_HURT_FISH, .death_sound = AZ_SND_KILL_FISH,
    .death_style = AZ_DEATH_EMBERS, .static_properties = AZ_BADF_DRAW_BG,
    .main_body = { .polygon = AZ_INIT_POLYGON(forceling_head_vertices),
                   .impact_damage = 6.0 },
    DECL_COMPONENTS(forceling_components)
  },
  [AZ_BAD_JUNGLE_CHOMPER] = {
    .max_health = 10.0, .overall_bounding_radius = 250.0,
    .potential_pickups = AZ_PUPF_ALL, .color = {128, 192, 0, 255},
    .hurt_sound = AZ_SND_HURT_PLANT, .death_sound = AZ_SND_KILL_PLANT,
    .death_style = AZ_DEATH_EMBERS, .static_properties = AZ_BADF_VULNERABLE,
    .main_body = { .polygon = AZ_INIT_POLYGON(chomper_plant_core_vertices),
                   .impact_damage = 10.0 },
    DECL_COMPONENTS(jungle_chomper_components)
  },
  [AZ_BAD_SMALL_AUV] = {
    .max_health = 30.0, .color = {160, 160, 160, 255},
    .potential_pickups = ~(AZ_PUPF_NOTHING | AZ_PUPF_SMALL_SHIELDS),
    .hurt_sound = AZ_SND_HURT_TURRET, .death_sound = AZ_SND_KILL_TURRET,
    .static_properties = (AZ_BADF_CARRIES_CARGO | AZ_BADF_NO_HOMING_PHASE),
    .main_body = { .polygon = AZ_INIT_POLYGON(small_auv_vertices),
                   .immunities = (AZ_DMGF_NORMAL | AZ_DMGF_CHARGED |
                                  AZ_DMGF_PIERCE | AZ_DMGF_BEAM |
                                  AZ_DMGF_FLAME),
                   .impact_damage = 8.0 }
  },
  [AZ_BAD_SENSOR_LASER] = {
    .max_health = 1000000.0, .color = {160, 160, 160, 255},
    .death_sound = AZ_SND_KILL_TURRET, .death_style = AZ_DEATH_SHARDS,
    .static_properties = (AZ_BADF_DRAW_BG | AZ_BADF_INVINCIBLE |
                          AZ_BADF_NO_HOMING | AZ_BADF_WALL_LIKE),
    .main_body = { .polygon = AZ_INIT_POLYGON(sensor_laser_vertices),
                   .immunities = ~0 }
  },
  [AZ_BAD_ERUPTION] = {
    .max_health = 1000000.0,
    .static_properties = (AZ_BADF_INCORPOREAL | AZ_BADF_NO_HOMING),
    .main_body = { .bounding_radius = 10.0 }
  },
  [AZ_BAD_PYROFLAKKER] = {
    .max_health = 12.0,
    .color = {255, 160, 80, 255}, .potential_pickups = AZ_PUPF_ALL,
    .hurt_sound = AZ_SND_HURT_PLANT,
    .death_sound = AZ_SND_KILL_FIRE_CRAWLER, .death_style = AZ_DEATH_EMBERS,
    .main_body = { .polygon = AZ_INIT_POLYGON(mycoflakker_vertices),
                   .immunities = AZ_DMGF_FLAME, .impact_damage = 10.0 }
  },
  [AZ_BAD_PYROSTALKER] = {
    .max_health = 15.0,
    .color = {255, 192, 80, 255}, .potential_pickups = AZ_PUPF_ALL,
    .hurt_sound = AZ_SND_HURT_PLANT,
    .death_sound = AZ_SND_KILL_FIRE_CRAWLER, .death_style = AZ_DEATH_EMBERS,
    .main_body = { .polygon = AZ_INIT_POLYGON(mycostalker_vertices),
                   .immunities = AZ_DMGF_FLAME, .impact_damage = 12.0 }
  },
  [AZ_BAD_DEMON_SWOOPER] = {
    .max_health = 15.0, .overall_bounding_radius = 60.0,
    .potential_pickups = AZ_PUPF_ALL, .color = {140, 0, 0, 255},
    .hurt_sound = AZ_SND_HURT_SWOOPER, .death_sound = AZ_SND_KILL_SWOOPER,
    .death_style = AZ_DEATH_EMBERS, .static_properties = AZ_BADF_WATER_BOUNCE,
    .main_body = { .polygon = AZ_INIT_POLYGON(swooper_vertices),
                   .immunities = AZ_DMGF_FLAME, .impact_damage = 8.0 },
    DECL_COMPONENTS(demon_swooper_components)
  },
  [AZ_BAD_FIRE_CHOMPER] = {
    .max_health = 14.0, .overall_bounding_radius = 130.0,
    .potential_pickups = AZ_PUPF_ALL, .color = {192, 96, 32, 255},
    .hurt_sound = AZ_SND_HURT_PLANT, .death_sound = AZ_SND_KILL_PLANT,
    .death_style = AZ_DEATH_EMBERS,
    .static_properties = (AZ_BADF_DRAW_BG | AZ_BADF_VULNERABLE),
    .main_body = { .polygon = AZ_INIT_POLYGON(chomper_plant_core_vertices),
                   .impact_damage = 10.0 },
    DECL_COMPONENTS(fire_chomper_components)
  },
  [AZ_BAD_GRABBER_PLANT] = {
    .max_health = 14.0, .overall_bounding_radius = 30.0,
    .potential_pickups = AZ_PUPF_ALL, .color = {90, 160, 40, 255},
    .hurt_sound = AZ_SND_HURT_PLANT, .death_sound = AZ_SND_KILL_PLANT,
    .static_properties = AZ_BADF_DRAW_BG, .death_style = AZ_DEATH_EMBERS,
    .main_body = { .polygon = AZ_INIT_POLYGON(grabber_plant_core_vertices),
                   .impact_damage = 10.0 },
    DECL_COMPONENTS(grabber_plant_components)
  },
  [AZ_BAD_POP_OPEN_TURRET] = {
    .max_health = 8.0, .overall_bounding_radius = 33.5,
    .potential_pickups = ~AZ_PUPF_LARGE_SHIELDS, .color = {160, 160, 80, 255},
    .hurt_sound = AZ_SND_HURT_TURRET, .death_sound = AZ_SND_KILL_TURRET,
    .static_properties = AZ_BADF_DRAW_BG,
    .main_body = { .bounding_radius = 16.0, .impact_damage = 10.0 },
    DECL_COMPONENTS(pop_open_turret_components)
  },
  [AZ_BAD_GNAT] = {
    .max_health = 0.5, .color = {128, 0, 255, 255},
    .hurt_sound = AZ_SND_HURT_ZIPPER, .death_sound = AZ_SND_KILL_DRAGONFLY,
    .death_style = AZ_DEATH_EMBERS,
    .potential_pickups = ~AZ_PUPF_LARGE_SHIELDS,
    .static_properties = (AZ_BADF_KAMIKAZE | AZ_BADF_WATER_BOUNCE),
    .main_body = { .polygon = AZ_INIT_POLYGON(gnat_vertices),
                   .impact_damage = 6.0 }
  },
  [AZ_BAD_CREEPY_EYE] = {
    .max_health = 35.0, .overall_bounding_radius = 25.0,
    .color = {160, 160, 160, 255}, .death_sound = AZ_SND_KILL_TURRET,
    .static_properties = (AZ_BADF_DRAW_BG | AZ_BADF_NO_HOMING),
    .main_body = { .bounding_radius = 1.0, .immunities = ~0 },
    DECL_COMPONENTS(creepy_eye_components)
  },
  [AZ_BAD_BOMB_SENSOR] = {
    .max_health = 1000000.0, .overall_bounding_radius = 30.0,
    .color = {160, 160, 160, 255}, .death_sound = AZ_SND_KILL_TURRET,
    .static_properties = (AZ_BADF_DRAW_BG | AZ_BADF_NO_HOMING |
                          AZ_BADF_WALL_LIKE),
    .main_body = { .bounding_radius = 15.0,
                   .immunities = ~(AZ_DMGF_BOMB | AZ_DMGF_MEGA_BOMB) },
    DECL_COMPONENTS(bomb_sensor_components)
  },
  [AZ_BAD_ROCKET_SENSOR] = {
    .max_health = 1000000.0, .overall_bounding_radius = 30.0,
    .color = {160, 160, 160, 255}, .death_sound = AZ_SND_KILL_TURRET,
    .static_properties = (AZ_BADF_DRAW_BG | AZ_BADF_NO_HOMING |
                          AZ_BADF_WALL_LIKE),
    .main_body = { .bounding_radius = 15.0,
                   .immunities = ~(AZ_DMGF_ROCKET | AZ_DMGF_HYPER_ROCKET) },
    DECL_COMPONENTS(rocket_sensor_components)
  },
  [AZ_BAD_SPIKED_VINE] = {
    .max_health = 20.0, .overall_bounding_radius = 300.0,
    .color = {48, 192, 96, 255}, .hurt_sound = AZ_SND_HURT_PLANT,
    .death_sound = AZ_SND_KILL_PLANT, .death_style = AZ_DEATH_EMBERS,
    .static_properties = (AZ_BADF_DRAW_BG | AZ_BADF_NO_HOMING),
    .main_body = { .polygon = AZ_INIT_POLYGON(spiked_vine_tip_vertices),
                   .immunities = (AZ_DMGF_NORMAL | AZ_DMGF_CHARGED),
                   .impact_damage = 10.0 },
    DECL_COMPONENTS(spiked_vine_components)
  },
  [AZ_BAD_MAGBEEST_HEAD] = {
    .max_health = 750.0, .overall_bounding_radius = 500.0,
    .color = {128, 128, 128, 255}, .hurt_sound = AZ_SND_HURT_TURRET,
    .armor_sound = AZ_SND_HIT_ARMOR, .static_properties = AZ_BADF_DRAW_BG,
    .potential_pickups = ~AZ_PUPF_NOTHING,
    .main_body = { .polygon = AZ_INIT_POLYGON(magbeest_head_vertices),
                   .immunities = (AZ_DMGF_FREEZE | AZ_DMGF_CPLUS |
                                  AZ_DMGF_FLAME),
                   .impact_damage = 20.0 },
    DECL_COMPONENTS(magbeest_head_components)
  },
  [AZ_BAD_MAGBEEST_LEGS_L] = {
    .max_health = 1000000.0, .overall_bounding_radius = 500.0,
    .color = {128, 128, 128, 255}, .death_style = AZ_DEATH_SHARDS,
    .armor_sound = AZ_SND_HIT_ARMOR,
    .static_properties = (AZ_BADF_DRAW_BG | AZ_BADF_INVINCIBLE |
                          AZ_BADF_NO_HOMING),
    .main_body = { .polygon = AZ_INIT_POLYGON(magbeest_legs_l_base_vertices),
                   .immunities = ~0, .impact_damage = 20.0 },
    DECL_COMPONENTS(magbeest_legs_l_components)
  },
  [AZ_BAD_MAGBEEST_LEGS_R] = {
    .max_health = 1000000.0, .overall_bounding_radius = 500.0,
    .color = {128, 128, 128, 255}, .death_style = AZ_DEATH_SHARDS,
    .armor_sound = AZ_SND_HIT_ARMOR,
    .static_properties = (AZ_BADF_DRAW_BG | AZ_BADF_INVINCIBLE |
                          AZ_BADF_NO_HOMING),
    .main_body = { .polygon = AZ_INIT_POLYGON(magbeest_legs_r_base_vertices),
                   .immunities = ~0, .impact_damage = 20.0 },
    DECL_COMPONENTS(magbeest_legs_r_components)
  },
  [AZ_BAD_MAGMA_BOMB] = {
    .max_health = 8.0, .color = {80, 80, 80, 255},
    .hurt_sound = AZ_SND_HURT_TURRET, .armor_sound = AZ_SND_HIT_ARMOR,
    .death_sound = AZ_SND_KILL_TURRET, .death_style = AZ_DEATH_SHARDS,
    .static_properties = AZ_BADF_NO_HOMING_PHASE,
    .main_body = { .bounding_radius = 12.0, .impact_damage = 10.0,
                   .immunities = (AZ_DMGF_NORMAL | AZ_DMGF_FREEZE |
                                  AZ_DMGF_FLAME) },
  },
  [AZ_BAD_OTH_BRAWLER] = {
    .max_health = 90.0, .color = {255, 255, 255, 255},
    .potential_pickups = AZ_PUPF_LARGE_SHIELDS, .death_style = AZ_DEATH_OTH,
    .hurt_sound = AZ_SND_HURT_OTH, .death_sound = AZ_SND_KILL_OTH,
    .main_body = { .polygon = AZ_INIT_POLYGON(oth_brawler_vertices),
                   .impact_damage = 20.0 }
  },
  [AZ_BAD_LARGE_FISH] = {
    .max_health = 18.0, .overall_bounding_radius = 90.0,
    .potential_pickups = AZ_PUPF_ALL, .color = {230, 0, 105, 255},
    .hurt_sound = AZ_SND_HURT_FISH, .death_sound = AZ_SND_KILL_FISH,
    .death_style = AZ_DEATH_EMBERS,
    .static_properties = (AZ_BADF_DRAW_BG | AZ_BADF_WATER_BOUNCE),
    .main_body = { .polygon = AZ_INIT_POLYGON(large_fish_head_vertices),
                   .impact_damage = 20.0 },
    DECL_COMPONENTS(large_fish_components)
  },
  [AZ_BAD_CRAB_CRAWLER] = {
    .max_health = 4.0, .overall_bounding_radius = 32.0,
    .potential_pickups = AZ_PUPF_ALL, .color = {128, 64, 32, 255},
    .hurt_sound = AZ_SND_HURT_CRAWLER, .armor_sound = AZ_SND_HIT_ARMOR,
    .death_sound = AZ_SND_KILL_TURRET, .death_style = AZ_DEATH_SHARDS,
    .main_body = { .polygon = AZ_INIT_POLYGON(crab_crawler_body_vertices),
                   .impact_damage = 12.0 },
    DECL_COMPONENTS(crab_crawler_components)
  },
  [AZ_BAD_SCRAP_METAL] = {
    .max_health = 0.1, .color = {160, 128, 192, 255},
    .potential_pickups = ~AZ_PUPF_LARGE_SHIELDS,
    .death_sound = AZ_SND_KILL_TURRET, .death_style = AZ_DEATH_SHARDS,
    .static_properties = (AZ_BADF_DRAW_BG | AZ_BADF_KAMIKAZE),
    .main_body = { .polygon = AZ_INIT_POLYGON(scrap_metal_vertices),
                   .impact_damage = 10.0,
                   .immunities = (AZ_DMGF_FREEZE | AZ_DMGF_FLAME) }
  },
  [AZ_BAD_RED_ATOM] = {
    .max_health = 16.0, .overall_bounding_radius = 60.0,
    .potential_pickups = AZ_PUPF_ALL, .color = {192, 64, 64, 255},
    .death_sound = AZ_SND_KILL_ATOM, .armor_sound = AZ_SND_HIT_ARMOR,
    .hurt_sound = AZ_SND_HURT_FISH,
    .main_body = { .bounding_radius = 12.0, .impact_damage = 25.0,
                   .immunities = AZ_DMGF_FLAME },
    DECL_COMPONENTS(red_atom_components)
  },
  [AZ_BAD_REFLECTION] = {
    .max_health = 1000000.0,
    .static_properties = (AZ_BADF_DRAW_BG | AZ_BADF_INCORPOREAL |
                          AZ_BADF_NO_HOMING),
    .main_body = { .bounding_radius = 20.0 }
  },
  [AZ_BAD_OTH_MINICRAB] = {
    .max_health = 30.0, .color = {255, 255, 255, 255},
    .potential_pickups = ~AZ_PUPF_NOTHING,
    .hurt_sound = AZ_SND_HURT_OTH, .death_sound = AZ_SND_KILL_OTH,
    .death_style = AZ_DEATH_OTH,
    .main_body = { .polygon = AZ_INIT_POLYGON(oth_minicrab_vertices),
                   .impact_damage = 15.0 }
  },
  [AZ_BAD_OTH_RAZOR_2] = {
    .max_health = 0.1, .color = {255, 255, 255, 255},
    .death_sound = AZ_SND_KILL_OTH, .death_style = AZ_DEATH_OTH,
    .potential_pickups = ~(AZ_PUPF_NOTHING | AZ_PUPF_LARGE_SHIELDS),
    .static_properties = AZ_BADF_KAMIKAZE,
    .main_body = { .bounding_radius = 15.0, .impact_damage = 7.0 }
  },
  [AZ_BAD_OTH_SUPERGUNSHIP] = {
    .max_health = 600.0, .color = {255, 255, 255, 255},
    .potential_pickups = ~AZ_PUPF_NOTHING, .hurt_sound = AZ_SND_HURT_OTH,
    .death_sound = AZ_SND_KILL_OTH, .death_style = AZ_DEATH_OTH,
    .main_body = { .polygon = AZ_INIT_POLYGON(oth_gunship_vertices),
                   .immunities = AZ_DMGF_FREEZE, .impact_damage = 15.0 }
  },
  [AZ_BAD_OTH_DECOY] = {
    .max_health = 25.0, .color = {255, 255, 255, 255},
    .potential_pickups = ~AZ_PUPF_NOTHING, .hurt_sound = AZ_SND_HURT_OTH,
    .death_sound = AZ_SND_KILL_OTH, .death_style = AZ_DEATH_OTH,
    .main_body = { .polygon = AZ_INIT_POLYGON(oth_gunship_vertices),
                   .impact_damage = 15.0 }
  },
  [AZ_BAD_CENTRAL_NETWORK_NODE] = {
    .max_health = 1000000.0,
    .static_properties = (AZ_BADF_INCORPOREAL | AZ_BADF_NO_HOMING),
    .main_body = { .bounding_radius = 20.0, .immunities = ~0 }
  },
  [AZ_BAD_OTH_TENTACLE] = {
    .max_health = 25.0, .overall_bounding_radius = 500.0,
    .color = {255, 255, 255, 255}, .hurt_sound = AZ_SND_HURT_OTH,
    .death_sound = AZ_SND_KILL_OTH, .death_style = AZ_DEATH_OTH,
    .potential_pickups = ~AZ_PUPF_NOTHING,
    .static_properties = AZ_BADF_NO_HOMING,
    .main_body = { .bounding_radius = 3, .impact_damage = 20.0,
                   .immunities = (AZ_DMGF_BOMB | AZ_DMGF_MEGA_BOMB) },
    DECL_COMPONENTS(oth_tentacle_components)
  }
};

AZ_STATIC_ASSERT(AZ_ARRAY_SIZE(baddie_datas) == AZ_NUM_BADDIE_KINDS + 1);

/*===========================================================================*/

static double polygon_bounding_radius(az_polygon_t polygon) {
  double radius = 0.0;
  for (int i = 0; i < polygon.num_vertices; ++i) {
    radius = fmax(radius, az_vnorm(polygon.vertices[i]));
  }
  return radius + 0.01; // small safety margin
}

static void init_component_data(az_component_data_t *component) {
  if (component->polygon.num_vertices > 0) {
    assert(component->bounding_radius == 0.0);
    component->bounding_radius =
      polygon_bounding_radius(component->polygon);
  } else assert(component->bounding_radius > 0.0);
}

static bool baddie_data_initialized = false;

void az_init_baddie_datas(void) {
  assert(!baddie_data_initialized);
  AZ_ARRAY_LOOP(data, baddie_datas) {
    if (data == &baddie_datas[AZ_BAD_NOTHING]) continue;
    // Check that the number of components is valid.
    assert(data->num_components >= 0);
    assert(data->num_components <= AZ_MAX_BADDIE_COMPONENTS);
    // Set bounding radius for all components.
    for (int i = 0; i < data->num_components; ++i) {
      // N.B. We need to cast away the const-ness of the data->components
      // pointer here, so that we can initialize its bounding_radius.  We could
      // avoid needing a const-cast by accessing the component data arrays
      // directly by name (as they are each declared non-const above), but it's
      // far more convenient to go through the data->components pointer.
      az_component_data_t *component =
        (az_component_data_t *)(&data->components[i]);
      init_component_data(component);
    }
    // Set main body bounding radius.
    init_component_data(&data->main_body);
    // Set overall bounding radius.
    if (data->num_components == 0) {
      assert(data->overall_bounding_radius == 0.0);
      data->overall_bounding_radius = data->main_body.bounding_radius;
    }
    // Sanity-check the overall bounding radius.
    assert(data->overall_bounding_radius >= data->main_body.bounding_radius);
    for (int i = 0; i < data->num_components; ++i) {
      assert(data->overall_bounding_radius >=
             data->components[i].bounding_radius +
             az_vnorm(data->components[i].init_position));
    }
    // Sanity-check other fields.
    assert(data->max_health > 0.0);
  }
  baddie_data_initialized = true;
}

const az_baddie_data_t *az_get_baddie_data(az_baddie_kind_t kind) {
  assert(baddie_data_initialized);
  assert(kind != AZ_BAD_NOTHING);
  const int data_index = (int)kind;
  assert(0 <= data_index && data_index < AZ_ARRAY_SIZE(baddie_datas));
  return &baddie_datas[data_index];
}

void az_init_baddie(az_baddie_t *baddie, az_baddie_kind_t kind,
                    az_vector_t position, double angle) {
  assert(kind != AZ_BAD_NOTHING);
  const az_uid_t uid = baddie->uid;
  AZ_ZERO_OBJECT(baddie);
  baddie->kind = kind;
  baddie->data = az_get_baddie_data(kind);
  baddie->uid = uid;
  baddie->position = position;
  baddie->angle = angle;
  baddie->health = baddie->data->max_health;
  baddie->temp_properties = AZ_BADF_INVINCIBLE;
  for (int i = 0; i < baddie->data->num_components; ++i) {
    assert(i < AZ_ARRAY_SIZE(baddie->components));
    baddie->components[i].position = baddie->data->components[i].init_position;
    baddie->components[i].angle = baddie->data->components[i].init_angle;
  }
}

/*===========================================================================*/

bool az_baddie_has_flag(const az_baddie_t *baddie, az_baddie_flags_t flag) {
  assert(baddie->kind != AZ_BAD_NOTHING);
  return (flag & (baddie->data->static_properties | baddie->temp_properties));
}

/*===========================================================================*/

static bool point_touches_component(
    const az_component_data_t *component, az_vector_t point) {
  if (component->polygon.num_vertices > 0) {
    return (az_vwithin(point, AZ_VZERO, component->bounding_radius) &&
            az_polygon_contains(component->polygon, point));
  } else {
    return az_vwithin(point, AZ_VZERO, component->bounding_radius);
  }
}

bool az_point_touches_baddie(const az_baddie_t *baddie, az_vector_t point,
                             const az_component_data_t **component_out,
                             az_vector_t *component_pos_out) {
  assert(baddie->kind != AZ_BAD_NOTHING);
  const az_baddie_data_t *data = baddie->data;

  // Common case: if point definitely doesn't touch baddie, return early.
  if (!az_vwithin(point, baddie->position, data->overall_bounding_radius)) {
    return false;
  }

  // Calculate point relative to the positioning of the baddie.
  const az_vector_t rel_point =
    az_vrotate(az_vsub(point, baddie->position), -baddie->angle);

  // Check if we hit the main body of the baddie.
  if (point_touches_component(&data->main_body, rel_point)) {
    if (component_out != NULL) *component_out = &data->main_body;
    if (component_pos_out != NULL) *component_pos_out = baddie->position;
    return true;
  }

  // Now check if we hit any of the baddie's components.
  for (int i = 0; i < data->num_components; ++i) {
    assert(i < AZ_ARRAY_SIZE(baddie->components));
    const az_component_data_t *component = &data->components[i];
    const az_vector_t rel_rel_point =
      az_vrotate(az_vsub(rel_point, baddie->components[i].position),
                 -baddie->components[i].angle);
    if (point_touches_component(component, rel_rel_point)) {
      if (component_out != NULL) *component_out = component;
      if (component_pos_out != NULL) {
        *component_pos_out = az_vadd(az_vrotate(baddie->components[i].position,
                                                baddie->angle),
                                     baddie->position);
      }
      return true;
    }
  }

  return false;
}

/*===========================================================================*/

static bool circle_touches_component(
    const az_component_data_t *component, az_vector_t position, double angle,
    double circle_radius, az_vector_t circle_center) {
  if (!az_vwithin(position, circle_center,
                  component->bounding_radius + circle_radius)) return false;
  if (component->polygon.num_vertices == 0) return true;
  return az_circle_touches_polygon_trans(component->polygon, position, angle,
                                         circle_radius, circle_center);
}

bool az_circle_touches_baddie(
    const az_baddie_t *baddie, double radius, az_vector_t center,
    const az_component_data_t **component_out) {
  assert(baddie->kind != AZ_BAD_NOTHING);
  // Common case: if circle definitely doesn't touch the baddie, return early.
  if (!az_vwithin(baddie->position, center, radius +
                  baddie->data->overall_bounding_radius)) return false;

  // Semi-common case: cast a ray towards the baddie center, returning the
  // first component hit.
  if (az_ray_hits_baddie(
          baddie, center,
          az_vwithlen(az_vsub(baddie->position, center), radius),
          NULL, NULL, component_out)) return true;

  // Okay, now we have to exhaustively check each component.
  const az_baddie_data_t *data = baddie->data;
  // Calculate center relative to the positioning of the baddie.
  const az_vector_t rel_center =
    az_vrotate(az_vsub(center, baddie->position), -baddie->angle);
  // Check the non-main components first.
  for (int i = 0; i < data->num_components; ++i) {
    const az_component_data_t *component = &data->components[i];
    if (circle_touches_component(
            component, baddie->components[i].position,
            baddie->components[i].angle, radius, rel_center)) {
      if (component_out != NULL) *component_out = component;
      return true;
    }
  }
  // Now check the main body.
  if (circle_touches_component(&data->main_body, AZ_VZERO, 0.0,
                               radius, rel_center)) {
    if (component_out != NULL) *component_out = &data->main_body;
    return true;
  }

  return false;
}

/*===========================================================================*/

static bool ray_hits_component(
    const az_component_data_t *component, az_vector_t position, double angle,
    az_vector_t start, az_vector_t delta, az_vector_t *point_out,
    az_vector_t *normal_out) {
  if (component->polygon.num_vertices > 0) {
    return (az_ray_hits_bounding_circle(start, delta, position,
                                        component->bounding_radius) &&
            az_ray_hits_polygon_trans(component->polygon, position,
                                      angle, start, delta,
                                      point_out, normal_out));
  } else {
    return az_ray_hits_circle(component->bounding_radius, position,
                              start, delta, point_out, normal_out);
  }
}

bool az_ray_hits_baddie(
    const az_baddie_t *baddie, az_vector_t start, az_vector_t delta,
    az_vector_t *point_out, az_vector_t *normal_out,
    const az_component_data_t **component_out) {
  assert(baddie->kind != AZ_BAD_NOTHING);
  const az_baddie_data_t *data = baddie->data;

  // Common case: if ray definitely misses baddie, return early.
  if (!az_ray_hits_bounding_circle(start, delta, baddie->position,
                                   data->overall_bounding_radius)) {
    return false;
  }

  // Calculate start and delta relative to the positioning of the baddie.
  const az_vector_t rel_start = az_vrotate(az_vsub(start, baddie->position),
                                           -baddie->angle);
  az_vector_t rel_delta = az_vrotate(delta, -baddie->angle);
  const az_component_data_t *hit_component = NULL;
  az_vector_t point = AZ_VZERO;

  // Check if we hit the main body of the baddie.
  if (ray_hits_component(&data->main_body, AZ_VZERO, 0.0, rel_start,
                         rel_delta, &point, normal_out)) {
    hit_component = &data->main_body;
    rel_delta = az_vsub(point, rel_start);
  }

  // Now check if we hit any of the baddie's components.
  for (int i = 0; i < data->num_components; ++i) {
    assert(i < AZ_ARRAY_SIZE(baddie->components));
    const az_component_data_t *component = &data->components[i];
    if (ray_hits_component(component, baddie->components[i].position,
                           baddie->components[i].angle, rel_start, rel_delta,
                           &point, normal_out)) {
      hit_component = component;
      rel_delta = az_vsub(point, rel_start);
    }
  }

  // Fix up *point_out and *normal_out and return.
  if (hit_component != NULL) {
    if (point_out != NULL) {
      *point_out = az_vadd(az_vrotate(point, baddie->angle), baddie->position);
    }
    if (normal_out != NULL) {
      *normal_out = az_vrotate(*normal_out, baddie->angle);
    }
    if (component_out != NULL) *component_out = hit_component;
    return true;
  }
  return false;
}

/*===========================================================================*/

static bool circle_hits_component(
    const az_component_data_t *component, az_vector_t position, double angle,
    double radius, az_vector_t start, az_vector_t delta,
    az_vector_t *pos_out, az_vector_t *normal_out) {
  if (component->polygon.num_vertices > 0) {
    return (az_ray_hits_bounding_circle(start, delta, position,
                                        component->bounding_radius + radius) &&
            az_circle_hits_polygon_trans(component->polygon, position, angle,
                                         radius, start, delta,
                                         pos_out, normal_out));
  } else {
    return az_circle_hits_circle(component->bounding_radius, position,
                                 radius, start, delta, pos_out, normal_out);
  }
}

bool az_circle_hits_baddie(
    const az_baddie_t *baddie, double radius, az_vector_t start,
    az_vector_t delta, az_vector_t *pos_out, az_vector_t *normal_out,
    const az_component_data_t **component_out) {
  assert(baddie->kind != AZ_BAD_NOTHING);
  const az_baddie_data_t *data = baddie->data;

  // Common case: if circle definitely misses baddie, return early.
  if (!az_ray_hits_bounding_circle(start, delta, baddie->position,
                                   data->overall_bounding_radius + radius)) {
    return false;
  }

  // Calculate start and delta relative to the positioning of the baddie.
  const az_vector_t rel_start = az_vrotate(az_vsub(start, baddie->position),
                                           -baddie->angle);
  az_vector_t rel_delta = az_vrotate(delta, -baddie->angle);
  const az_component_data_t *hit_component = NULL;
  az_vector_t pos = AZ_VZERO;

  // Check if we hit the main body of the baddie.
  if (circle_hits_component(&data->main_body, AZ_VZERO, 0.0, radius, rel_start,
                            rel_delta, &pos, normal_out)) {
    hit_component = &data->main_body;
    rel_delta = az_vsub(pos, rel_start);
  }

  // Now check if we hit any of the baddie's components.
  for (int i = 0; i < data->num_components; ++i) {
    assert(i < AZ_ARRAY_SIZE(baddie->components));
    const az_component_data_t *component = &data->components[i];
    if (circle_hits_component(component, baddie->components[i].position,
                              baddie->components[i].angle, radius, rel_start,
                              rel_delta, &pos, normal_out)) {
      hit_component = component;
      rel_delta = az_vsub(pos, rel_start);
    }
  }

  // Fix up *pos_out and *normal_out and return.
  if (hit_component != NULL) {
    if (pos_out != NULL) {
      *pos_out = az_vadd(az_vrotate(pos, baddie->angle), baddie->position);
    }
    if (normal_out != NULL) {
      *normal_out = az_vrotate(*normal_out, baddie->angle);
    }
    if (component_out != NULL) *component_out = hit_component;
    return true;
  }
  return false;
}

/*===========================================================================*/

static bool arc_circle_hits_component(
    const az_component_data_t *component, az_vector_t component_position,
    double component_angle, double circle_radius, az_vector_t circle_start,
    az_vector_t spin_center, double spin_angle,
    double *angle_out, az_vector_t *pos_out, az_vector_t *normal_out) {
  if (component->polygon.num_vertices > 0) {
    return (az_arc_ray_might_hit_bounding_circle(
                circle_start, spin_center, spin_angle, component_position,
                component->bounding_radius + circle_radius) &&
            az_arc_circle_hits_polygon_trans(
                component->polygon, component_position, component_angle,
                circle_radius, circle_start, spin_center, spin_angle,
                angle_out, pos_out, normal_out));
  } else {
    return az_arc_circle_hits_circle(
        component->bounding_radius, component_position, circle_radius,
        circle_start, spin_center, spin_angle, angle_out, pos_out, normal_out);
  }
}

bool az_arc_circle_hits_baddie(
    const az_baddie_t *baddie, double circle_radius,
    az_vector_t start, az_vector_t spin_center, double spin_angle,
    double *angle_out, az_vector_t *pos_out, az_vector_t *normal_out,
    const az_component_data_t **component_out) {
  assert(baddie->kind != AZ_BAD_NOTHING);
  const az_baddie_data_t *data = baddie->data;

  // Common case: if circle definitely misses baddie, return early.
  if (!az_arc_ray_might_hit_bounding_circle(
          start, spin_center, spin_angle, baddie->position,
          data->overall_bounding_radius + circle_radius)) {
    return false;
  }

  // Calculate start and spin_center relative to the positioning of the baddie.
  const az_vector_t rel_start =
    az_vrotate(az_vsub(start, baddie->position), -baddie->angle);
  const az_vector_t rel_spin_center =
    az_vrotate(az_vsub(spin_center, baddie->position), -baddie->angle);
  const az_component_data_t *hit_component = NULL;

  // Check if we hit the main body of the baddie.
  if (arc_circle_hits_component(
          &data->main_body, AZ_VZERO, 0.0, circle_radius, rel_start,
          rel_spin_center, spin_angle, &spin_angle, pos_out, normal_out)) {
    hit_component = &data->main_body;
  }

  // Now check if we hit any of the baddie's components.
  for (int i = 0; i < data->num_components; ++i) {
    assert(i < AZ_ARRAY_SIZE(baddie->components));
    const az_component_data_t *component = &data->components[i];
    if (arc_circle_hits_component(
            component, baddie->components[i].position,
            baddie->components[i].angle, circle_radius, rel_start,
            rel_spin_center, spin_angle, &spin_angle, pos_out, normal_out)) {
      hit_component = component;
    }
  }

  // Fix up *pos_out and *normal_out and return.
  if (hit_component != NULL) {
    if (angle_out != NULL) *angle_out = spin_angle;
    if (pos_out != NULL) {
      *pos_out = az_vadd(az_vrotate(*pos_out, baddie->angle),
                         baddie->position);
    }
    if (normal_out != NULL) {
      *normal_out = az_vrotate(*normal_out, baddie->angle);
    }
    if (component_out != NULL) *component_out = hit_component;
    return true;
  }
  return false;
}

/*===========================================================================*/
