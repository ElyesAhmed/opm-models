/*
  Copyright (C) 2011-2013 by Andreas Lauser

  This file is part of the Open Porous Media project (OPM).

  OPM is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 2 of the License, or
  (at your option) any later version.

  OPM is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with OPM.  If not, see <http://www.gnu.org/licenses/>.
*/
/*!
 * \file
 * \ingroup FlashModel
 *
 * \brief Declares the properties required by the compositional
 *        multi-phase model based on flash calculations.
 */
#ifndef EWOMS_FLASH_PROPERTIES_HH
#define EWOMS_FLASH_PROPERTIES_HH

#include <ewoms/models/common/multiphasebaseproperties.hh>
#include <ewoms/vtk/vtkcompositionmodule.hh>
#include <ewoms/vtk/vtkenergymodule.hh>
#include <ewoms/vtk/vtkdiffusionmodule.hh>

namespace Opm {
namespace Properties {
NEW_PROP_TAG(FluidSystem); //!< Provides the thermodynamic relations
NEW_PROP_TAG(FlashSolver); //!< The type of the flash constraint solver
NEW_PROP_TAG(FlashTolerance); //!< The maximum accepted error of the flash solver

NEW_PROP_TAG(HeatConductionLaw);   //!< The heat conduction law which ought to be used
NEW_PROP_TAG(HeatConductionLawParams); //!< The parameters of the heat conduction law

NEW_PROP_TAG(EnableEnergy); //!< Specifies whether energy should be considered as a conservation quantity or not
NEW_PROP_TAG(EnableDiffusion); //!< Enable diffusive fluxes?
}} // namespace Properties, Opm

#endif
