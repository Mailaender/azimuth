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
#ifndef AZIMUTH_STATE_SCRIPT_H_
#define AZIMUTH_STATE_SCRIPT_H_

#include <stdbool.h>
#include <stdio.h> // for FILE

#include "azimuth/util/rw.h"

/*===========================================================================*/

typedef enum {
  AZ_OP_NOP = 0, // does nothing
  // Stack manipulation:
  AZ_OP_PUSH, // push i onto the stack
  AZ_OP_POP, // pop i values from the stack (minimum of 1)
  AZ_OP_DUP, // duplicate top i values on the stack (minimum of 1)
  AZ_OP_SWAP, // cycle top i values on the stack (defaults to 2)
  // Arithmetic:
  AZ_OP_ADD, // pop top two, push (a + b)
  AZ_OP_ADDI, // pop top, push (a + i)
  AZ_OP_SUB, // pop top two, push (a - b)
  AZ_OP_SUBI, // pop top, push (a - i)
  AZ_OP_ISUB, // pop top, push (i - a)
  AZ_OP_MUL, // pop top two, push (a * b)
  AZ_OP_MULI, // pop top, push (a * i)
  AZ_OP_DIV, // pop top two, push (a / b)
  AZ_OP_DIVI, // pop top, push (a / i)
  AZ_OP_IDIV, // pop top, push (i / a)
  AZ_OP_MOD, // pop top two, push (a mod b)
  AZ_OP_MODI, // pop top, push (a mod i)
  AZ_OP_MIN, // pop top two, push min(a, b)
  AZ_OP_MINI, // pop top, push min(a, i)
  AZ_OP_MAX, // pop top two, push max(a, b)
  AZ_OP_MAXI, // pop top, push max(a, i)
  // Math:
  AZ_OP_ABS, // pop top, push fabs(a)
  AZ_OP_MTAU, // pop top, push az_mod2pi(a)
  AZ_OP_RAND, // push random number from [0, 1)
  AZ_OP_SQRT, // pop top, push sqrt(a)
  // Vectors:
  AZ_OP_VADD, // pop top four, push (a + c) and (b + d)
  AZ_OP_VSUB, // pop top four, push (a - c) and (b - d)
  AZ_OP_VMUL, // pop top three, push (a * c) and (b * c)
  AZ_OP_VMULI, // pop top two, push (a * i) and (b * i)
  AZ_OP_VNORM, // pop top two, push hypot(a, b)
  AZ_OP_VTHETA, // pop top two, push atan2(b, a)
  AZ_OP_VPOLAR, // pop top two, push a*cos(b) and a*sin(b)
  // Comparisons:
  AZ_OP_EQ, // pop top two, push (a == b ? 1 : 0)
  AZ_OP_EQI, // pop top, push (a == i ? 1 : 0)
  AZ_OP_NE, // pop top two, push (a != b ? 1 : 0)
  AZ_OP_NEI, // pop top, push (a != i ? 1 : 0)
  AZ_OP_LT, // pop top two, push (a < b ? 1 : 0)
  AZ_OP_LTI, // pop top, push (a < i ? 1 : 0)
  AZ_OP_GT, // pop top two, push (a > b ? 1 : 0)
  AZ_OP_GTI, // pop top, push (a > i ? 1 : 0)
  AZ_OP_LE, // pop top two, push (a <= b ? 1 : 0)
  AZ_OP_LEI, // pop top, push (a <= i ? 1 : 0)
  AZ_OP_GE, // pop top two, push (a >= b ? 1 : 0)
  AZ_OP_GEI, // pop top, push (a >= i ? 1 : 0)
  // Flags:
  AZ_OP_TEST, // push 1 if flag i is set, else 0
  AZ_OP_SET, // set flag i
  AZ_OP_CLR, // clear flag i
  AZ_OP_HAS, // push 1 if player has upgrade i, else 0
  AZ_OP_MAP, // grant map data for zone i
  // Objects:
  AZ_OP_NIX, // remove object i (of any non-ship type)
  AZ_OP_KILL, // kill/break object i (of any type)
  AZ_OP_GHEAL, // push health of object i (ship or baddie)
  AZ_OP_SHEAL, // pop top, set health of object i (ship or baddie) to a
  AZ_OP_GPOS, // push (x, y) position of object i (of any type)
  AZ_OP_SPOS, // pop top two, set position of obj i (of any type) to (a, b)
  AZ_OP_GANG, // push angle of object i (of any type)
  AZ_OP_SANG, // pop top, set angle of obj i (of any type) to a
  AZ_OP_GSTAT, // push state of object i (meaning of "state" depends on type)
  AZ_OP_SSTAT, // pop top, set state of object i to a
  AZ_OP_ACTIV, // set state of object i to 1
  AZ_OP_DEACT, // set state of object i to 0
  // Ship:
  AZ_OP_GVEL, // push (x, y) velocity of ship
  AZ_OP_SVEL, // pop top two, set velocity of ship to (a, b)
  AZ_OP_AUTOP, // enable (if i != 0) or disable (if i == 0) autopilot mode
  AZ_OP_TURN, // pop top, set autopilot goal angle to a
  AZ_OP_THRUST, // set autopilot engine thrust to sign(i)
  AZ_OP_CPLUS, // engage autopilot C-plus drive
  // Baddies:
  AZ_OP_BAD, // pop top four, add baddie of kind a, at (b, c), with angle d
  AZ_OP_SBADK, // pop top, transform baddie i to kind a
  AZ_OP_BOSS, // designate baddie i as the current boss
  // Doors:
  AZ_OP_OPEN, // open door i (but don't run its script)
  AZ_OP_CLOSE, // close door i
  AZ_OP_LOCK, // close and lock door i
  AZ_OP_UNLOCK, // unlock (but do not open) door i
  // Gravfields:
  AZ_OP_GSTR, // push strength of gravfield i
  AZ_OP_SSTR, // pop top, set strength of gravfield i to a
  // Camera:
  AZ_OP_GCAM, // push (x, y) position of camera center
  AZ_OP_RCAM, // pop top, set camera r_max_override to a
  AZ_OP_DARK, // set room darkness goal to i
  AZ_OP_DARKS, // pop top, set room darkness goal to a
  AZ_OP_BLINK, // set room darkness (not goal) to i
  AZ_OP_SHAKE, // momentarily shake camera by amplitude i
  AZ_OP_QUAKE, // set camera quake amplitude to i
  // Pyrotechnics:
  AZ_OP_BOOM, // pop top two, create large explosion at position (a, b)
  AZ_OP_NUKE, // create ginormous explosion that destroys everything
  AZ_OP_BOLT, // pop top four, add bolt from (a, b) to (c, d) for i seconds
  AZ_OP_NPS, // pop top 2, create intensity-i NPS portal at (a, b)
  // Cutscenes:
  AZ_OP_FADO, // fade out whole screen (including HUD) and hide dialogue
  AZ_OP_FADI, // fade in screen
  AZ_OP_FLASH, // flash screen white, then fade back to normal
  AZ_OP_SCENE, // transition to cutscene i (suspend script until scene starts)
  AZ_OP_SCTXT, // display text i as a cutscene
  AZ_OP_SKIP, // enable (if i != 0) or disable (if i == 0) skip.allowed
  // Messages/dialogue:
  AZ_OP_MSG, // display text i at bottom of screen
  AZ_OP_DLOG, // begin dialogue
  AZ_OP_PT, // set top speaker to portrait i
  AZ_OP_PB, // set bottom speaker to portrait i
  AZ_OP_TT, // have top speaker say text i
  AZ_OP_TB, // have bottom speaker say text i
  AZ_OP_DEND, // end dialogue
  AZ_OP_MLOG, // begin monologue
  AZ_OP_TM, // have monologue say text i
  AZ_OP_MEND, // end monologue
  // Music/sound:
  AZ_OP_MUS, // start music i
  AZ_OP_MUSF, // set music flag to i
  AZ_OP_SND, // play sound i
  // Timers:
  AZ_OP_WAIT, // suspend script; add timer to resume after i seconds
  AZ_OP_WAITS, // pop top, suspend script; add timer to resume after a seconds
  AZ_OP_DOOM, // suspend script; set global countdown for i seconds
  AZ_OP_SAFE, // cancel global countdown
  // Control flow:
  AZ_OP_JUMP, // add i to PC
  AZ_OP_BEQZ, // pop top, add i to PC if a is zero
  AZ_OP_BNEZ, // pop top, add i to PC if a is not zero
  AZ_OP_HALT, // halt script successfully
  AZ_OP_HEQZ, // pop top, halt script successfully if a is zero
  AZ_OP_HNEZ, // pop top, halt script successfully if a is not zero
  AZ_OP_VICT, // halt script and end the game in victory
  AZ_OP_ERROR // halt script and printf execution state
} az_opcode_t;

const char *az_opcode_name(az_opcode_t opcode);

/*===========================================================================*/

typedef struct {
  az_opcode_t opcode;
  double immediate;
} az_instruction_t;

typedef struct {
  int num_instructions;
  az_instruction_t *instructions;
} az_script_t;

typedef struct {
  const az_script_t *script;
  int pc;
  int stack_size;
  double stack[20];
} az_script_vm_t;

typedef struct {
  double time_remaining;
  az_script_vm_t vm; // if script is NULL, this timer is not present
} az_timer_t;

// Serialize the script and return true, or return false on error.
bool az_write_script(const az_script_t *script, az_writer_t *writer);
bool az_sprint_script(const az_script_t *script, char *buffer, int length);

// Parse, allocate, and return the script, or return NULL on error.
az_script_t *az_read_script(az_reader_t *reader);
az_script_t *az_sscan_script(const char *string, int length);

// Allocate and return a copy of the given script.  Returns NULL if given NULL.
az_script_t *az_clone_script(const az_script_t *script);

// Free the script object and its data array.  Does nothing if given NULL.
void az_free_script(az_script_t *script);

/*===========================================================================*/

#endif // AZIMUTH_STATE_SCRIPT_H_
