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

#ifndef ICM_HPP
#define ICM_HPP

#include <stdint.h>
#include <vector>

namespace independent_chip_model  {
  /**
   * \struct Player
   * \brief All the necessary stuff for a player.
   */
  struct Player
  {
    uint16_t id;
    uint32_t stack;
    double ICM_EV;
  };
  
  /**
   * \class Table
   * \brief This class contains the player's stacks at the table to do the ICM calculation with.
   */
  class Table
  {
  public:
    /**
     * \var pay_outs
     * \brief This array contains the pay outs for the SNG.
     */
    static uint32_t pay_outs[10];
  
  private:
    std::vector<Player> players;
    bool calculated;
    
  public:
    /**
     * \brief The constructor.
     */
    Table ();
    
    /**
     * \brief The copy constructor.
     * \param other The other Table to copy.
     */
    Table (const Table&);
    
    /**
     * \brief The destructor.
     */
    virtual ~Table ();
    
    /**
     * \brief Return a player's stack.
     * \param id The player's number.
     */
    uint32_t getStack (const uint16_t) const;
    
    /**
     * \brief Set a player's stack.
     * \param id The player's number.
     * \param stack The amount to set the player's stack to.
     */
    void setStack (const uint16_t, const uint32_t);
    
    /**
     * \brief Do the ICM calculation.
     */
    void calc_ICM ();
    
    /**
     * \brief Get a player's ICM expectation value.
     * \param id The player's number.
     */
    double get_EV (const uint16_t);
    
    /**
     * \brief Get the EV of the amount to call.
     * \param id The player's number.
     * \param dollars The amount to call.
     */
    double get_Call_EV (const uint16_t, const uint32_t);
  };
}

#endif