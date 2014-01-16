/*
  Copyright (C) 2009-2013 by Andreas Lauser

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
 *
 * \copydoc Ewoms::NcpVolumeVariables
 */
#ifndef EWOMS_NCP_VOLUME_VARIABLES_HH
#define EWOMS_NCP_VOLUME_VARIABLES_HH

#include "ncpproperties.hh"

#include <ewoms/models/common/energymodule.hh>
#include <ewoms/models/common/diffusionmodule.hh>
#include <opm/material/constraintsolvers/NcpFlash.hpp>
#include <opm/material/fluidstates/CompositionalFluidState.hpp>

#include <dune/common/fvector.hh>
#include <dune/common/fmatrix.hh>

namespace Ewoms {
/*!
 * \ingroup NcpModel
 * \ingroup VolumeVariables
 *
 * \brief Contains the quantities which are are constant within a
 *        finite volume in the compositional multi-phase NCP model.
 */
template <class TypeTag>
class NcpVolumeVariables
    : public GET_PROP_TYPE(TypeTag, DiscVolumeVariables)
    , public DiffusionVolumeVariables<TypeTag, GET_PROP_VALUE(TypeTag, EnableDiffusion) >
    , public EnergyVolumeVariables<TypeTag, GET_PROP_VALUE(TypeTag, EnableEnergy) >
    , public GET_PROP_TYPE(TypeTag, VelocityModule)::VelocityVolumeVariables
{
    typedef typename GET_PROP_TYPE(TypeTag, DiscVolumeVariables) ParentType;

    typedef typename GET_PROP_TYPE(TypeTag, Scalar) Scalar;
    typedef typename GET_PROP_TYPE(TypeTag, FluidSystem) FluidSystem;
    typedef typename GET_PROP_TYPE(TypeTag, MaterialLaw) MaterialLaw;
    typedef typename GET_PROP_TYPE(TypeTag, MaterialLawParams) MaterialLawParams;
    typedef typename GET_PROP_TYPE(TypeTag, ElementContext) ElementContext;
    typedef typename GET_PROP_TYPE(TypeTag, Indices) Indices;
    typedef typename GET_PROP_TYPE(TypeTag, NcpCompositionFromFugacitiesSolver)
        CompositionFromFugacitiesSolver;
    typedef typename GET_PROP_TYPE(TypeTag, GridView) GridView;
    typedef typename GET_PROP_TYPE(TypeTag, VelocityModule) VelocityModule;

    enum { numPhases = GET_PROP_VALUE(TypeTag, NumPhases) };
    enum { numComponents = GET_PROP_VALUE(TypeTag, NumComponents) };
    enum { enableEnergy = GET_PROP_VALUE(TypeTag, EnableEnergy) };
    enum { enableDiffusion = GET_PROP_VALUE(TypeTag, EnableDiffusion) };
    enum { fugacity0Idx = Indices::fugacity0Idx };
    enum { saturation0Idx = Indices::saturation0Idx };
    enum { pressure0Idx = Indices::pressure0Idx };
    enum { dimWorld = GridView::dimensionworld };

    typedef Opm::CompositionalFluidState<Scalar, FluidSystem,
                                         /*storeEnthalpy=*/enableEnergy> FluidState;
    typedef Dune::FieldVector<Scalar, numComponents> ComponentVector;
    typedef Dune::FieldMatrix<Scalar, dimWorld, dimWorld> DimMatrix;
    typedef Ewoms::DiffusionVolumeVariables<TypeTag, enableDiffusion> DiffusionVolumeVariables;
    typedef Ewoms::EnergyVolumeVariables<TypeTag, enableEnergy> EnergyVolumeVariables;
    typedef typename VelocityModule::VelocityVolumeVariables VelocityVolumeVariables;

public:
    NcpVolumeVariables()
    {}

    /*!
     * \brief VolumeVariables::update
     */
    void update(const ElementContext &elemCtx,
                int dofIdx,
                int timeIdx)
    {
        ParentType::update(elemCtx,
                           dofIdx,
                           timeIdx);
        ParentType::checkDefined();

        typename FluidSystem::ParameterCache paramCache;
        const auto &priVars = elemCtx.primaryVars(dofIdx, timeIdx);

        // set the phase saturations
        Scalar sumSat = 0;
        for (int phaseIdx = 0; phaseIdx < numPhases - 1; ++phaseIdx) {
            sumSat += priVars[saturation0Idx + phaseIdx];
            fluidState_.setSaturation(phaseIdx,
                                      priVars[saturation0Idx + phaseIdx]);
        }
        fluidState_.setSaturation(numPhases - 1, 1.0 - sumSat);
        Valgrind::CheckDefined(sumSat);

        // set the fluid phase temperature
        EnergyVolumeVariables::updateTemperatures_(fluidState_, elemCtx, dofIdx, timeIdx);

        // retrieve capillary pressure parameters
        const auto &problem = elemCtx.problem();
        const MaterialLawParams &materialParams =
            problem.materialLawParams(elemCtx, dofIdx, timeIdx);
        // calculate capillary pressures
        Scalar capPress[numPhases];
        MaterialLaw::capillaryPressures(capPress, materialParams, fluidState_);
        // add to the pressure of the first fluid phase
        Scalar pressure0 = priVars[pressure0Idx];
        for (int phaseIdx = 0; phaseIdx < numPhases; ++phaseIdx)
            fluidState_.setPressure(phaseIdx, pressure0 + (capPress[phaseIdx]
                                                           - capPress[0]));

        ComponentVector fug;
        // retrieve component fugacities
        for (int compIdx = 0; compIdx < numComponents; ++compIdx)
            fug[compIdx] = priVars[fugacity0Idx + compIdx];

        // calculate phase compositions
        const auto *hint = elemCtx.hint(dofIdx, timeIdx);
        for (int phaseIdx = 0; phaseIdx < numPhases; ++phaseIdx) {
            // initial guess
            if (hint) {
                for (int compIdx = 0; compIdx < numComponents; ++compIdx) {
                    // use the hint for the initial mole fraction!
                    Scalar moleFracIJ
                        = hint->fluidState().moleFraction(phaseIdx, compIdx);

                    // set initial guess of the component's mole fraction
                    fluidState_.setMoleFraction(phaseIdx, compIdx, moleFracIJ);
                }
            }
            else // !hint
                CompositionFromFugacitiesSolver::guessInitial(fluidState_,
                                                              paramCache,
                                                              phaseIdx, fug);

            // calculate the phase composition from the component
            // fugacities
            CompositionFromFugacitiesSolver::solve(fluidState_, paramCache,
                                                   phaseIdx, fug);
        }

        // porosity
        porosity_ = problem.porosity(elemCtx, dofIdx, timeIdx);
        Valgrind::CheckDefined(porosity_);

        // relative permeabilities
        MaterialLaw::relativePermeabilities(relativePermeability_,
                                            materialParams, fluidState_);

        // dynamic viscosities
        for (int phaseIdx = 0; phaseIdx < numPhases; ++phaseIdx) {
            // viscosities
            Scalar mu
                = FluidSystem::viscosity(fluidState_, paramCache, phaseIdx);
            fluidState_.setViscosity(phaseIdx, mu);
        }

        // intrinsic permeability
        intrinsicPerm_ = problem.intrinsicPermeability(elemCtx, dofIdx, timeIdx);

        // update the quantities specific for the velocity model
        VelocityVolumeVariables::update_(elemCtx, dofIdx, timeIdx);

        // energy related quantities
        EnergyVolumeVariables::update_(fluidState_, paramCache, elemCtx, dofIdx, timeIdx);

        // update the diffusion specific quantities of the volume variables
        DiffusionVolumeVariables::update_(fluidState_, paramCache, elemCtx, dofIdx, timeIdx);

        checkDefined();
    }

    /*!
     * \brief ImmiscibleVolumeVariables::fluidState
     */
    const FluidState &fluidState() const
    { return fluidState_; }

    /*!
     * \brief ImmiscibleVolumeVariables::intrinsicPermeability
     */
    const DimMatrix &intrinsicPermeability() const
    { return intrinsicPerm_; }

    /*!
     * \brief ImmiscibleVolumeVariables::relativePermeability
     */
    Scalar relativePermeability(int phaseIdx) const
    { return relativePermeability_[phaseIdx]; }

    /*!
     * \brief ImmiscibleVolumeVariables::mobility
     */
    Scalar mobility(int phaseIdx) const
    { return relativePermeability(phaseIdx) / fluidState_.viscosity(phaseIdx); }

    /*!
     * \brief ImmiscibleVolumeVariables::porosity
     */
    Scalar porosity() const
    { return porosity_; }

    /*!
     * \brief VolumeVariables::checkDefined
     */
    void checkDefined() const
    {
#if !defined NDEBUG && HAVE_VALGRIND
        ParentType::checkDefined();

        Valgrind::CheckDefined(porosity_);
        Valgrind::CheckDefined(relativePermeability_);

        fluidState_.checkDefined();
#endif
    }

private:
    FluidState fluidState_;
    Scalar porosity_;
    DimMatrix intrinsicPerm_;
    Scalar relativePermeability_[numPhases];
};

} // namespace Ewoms

#endif
