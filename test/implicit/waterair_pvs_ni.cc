// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
/*****************************************************************************
 *   Copyright (C) 2008-2012 by Andreas Lauser                               *
 *   Copyright (C) 2009-2012 by Bernd Flemisch                               *
 *                                                                           *
 *   This program is free software: you can redistribute it and/or modify    *
 *   it under the terms of the GNU General Public License as published by    *
 *   the Free Software Foundation, either version 2 of the License, or       *
 *   (at your option) any later version.                                     *
 *                                                                           *
 *   This program is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 *   GNU General Public License for more details.                            *
 *                                                                           *
 *   You should have received a copy of the GNU General Public License       *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 *****************************************************************************/
/*!
 * \file
 *
 * \brief test for the 2p2cni VCVF discretization
 */
#include "config.h"

#include "problems/waterairproblem.hh"

#include <ewoms/models/pvs/pvsmodel.hh>
#include <ewoms/common/start.hh>

namespace Ewoms {
namespace Properties {
NEW_TYPE_TAG(WaterAirProblem, INHERITS_FROM(VcfvPvs, WaterAirBaseProblem));

SET_BOOL_PROP(WaterAirProblem, EnableEnergy, true);
} }

int main(int argc, char** argv)
{
    typedef TTAG(WaterAirProblem) ProblemTypeTag;
    return Ewoms::start<ProblemTypeTag>(argc, argv);
}