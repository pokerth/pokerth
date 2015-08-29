/*****************************************************************************
 * Independent Chip Model (ICM) calculation                                  *
 * Copyright (C) 2015 Daniel Steinhauer                                      *
 *                                                                           *
 * This file is part of:                                                     *
 * PokerTH - The open source texas holdem engine                             *
 * Copyright (C) 2006-2012 Felix Hammer, Florian Thauer, Lothar May          *
 *                                                                           *
 * This program is free software: you can redistribute it and/or modify      *
 * it under the terms of the GNU Affero General Public License as            *
 * published by the Free Software Foundation, either version 3 of the        *
 * License, or (at your option) any later version.                           *
 *                                                                           *
 * This program is distributed in the hope that it will be useful,           *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
 * GNU Affero General Public License for more details.                       *
 *                                                                           *
 * You should have received a copy of the GNU Affero General Public License  *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.     *
 *                                                                           *
 *                                                                           *
 * Additional permission under GNU AGPL version 3 section 7                  *
 *                                                                           *
 * If you modify this program, or any covered work, by linking or            *
 * combining it with the OpenSSL project's OpenSSL library (or a             *
 * modified version of that library), containing parts covered by the        *
 * terms of the OpenSSL or SSLeay licenses, the authors of PokerTH           *
 * (Felix Hammer, Florian Thauer, Lothar May) grant you additional           *
 * permission to convey the resulting work.                                  *
 * Corresponding Source for a non-source form of such a combination          *
 * shall include the source code for the parts of OpenSSL used as well       *
 * as that of the covered work.                                              *
 *****************************************************************************/

#include "icm.h"

#include <algorithm>

using namespace independent_chip_model;
using namespace std;

/* 
 * These are the default point pay-outs for the
 * poker-heroes.com ranking tournaments.
 */
uint32_t Table::pay_outs[10] = {
  24, 16, 10, 6, 3, 2, 1, 0, 0, 0
};

Table::Table ()
  : players (0)
{}

Table::Table (const Table& other)
  : players (other.players)
{}

Table::~Table ()
{}

uint32_t Table::getStack (const uint16_t id) const
{
  uint32_t RET = 0;
  for (uint32_t ii = 0; ii < players.size (); ++ii)  {
    if (players[ii].id == id)  {
      RET = players[ii].stack;
      break;
    }
  }
  
  return RET;
}

void Table::setStack (const uint16_t id, const uint32_t stack)
{
  int16_t pos = -1;
  for (uint16_t ii = 0; ii < players.size (); ++ii)  {
    if (players[ii].id == id)  {
      pos = ii;
      break;
    }
  }
  
  if (pos >= 0)  {
    if (stack)  {
      players[pos].stack = stack;
    } else  {
      players.erase (players.begin () + pos);
    }
  } else  {
    if (stack)  {
      Player np;
      np.id = id;
      np.stack = stack;
      np.ICM_EV = 0.0;
      players.push_back (np);
    }
  }
}

void Table::calc_ICM ()
{
  const uint16_t p_count = players.size ();
  
  /* Player -> Place */
  uint16_t places[p_count];
  
  /*
   * Inverse for places.
   * Place -> Player
   */
  uint16_t partc[p_count];
  for (uint16_t ii = 0; ii < p_count; ++ii)  {
    places[ii] = ii;
    players[ii].ICM_EV = 0.0;
  }
  
  /* Iterate through the placement permutations. */
  do {
    /* Build the inverse array */
    for (uint16_t ii = 0; ii < p_count; ++ii)  {
      partc[places[ii]] = ii;
    }
    
    /* Calculate the propability for this permutation. */
    double p = 1.0;
    for (uint16_t ia = 0; ia < p_count; ++ia)  {
      int32_t stack_sum = 0;
      for (uint16_t ib = ia; ib < p_count; ++ib)  {
        stack_sum += players[partc[ib]].stack;
      }
    
      p *= (double) players[partc[ia]].stack / stack_sum;
    }

    /* 
     * Sum up the expectation values for each player for this
     * permutation.
     */
    for (uint16_t ii = 0; ii < p_count; ++ii)  {
      players[ii].ICM_EV += p * pay_outs[places[ii]];
    }
  } while (next_permutation (places, places + p_count));
}

double Table::get_EV (const uint16_t id) const
{
  double RET = 0.0;
  for (uint32_t ii = 0; ii < players.size (); ++ii)  {
    if (players[ii].id == id)  {
      RET = players[ii].ICM_EV;
      break;
    }
  }
  
  return RET;
}