/*

Copyright (c) 2005-2015, University of Oxford.
All rights reserved.

University of Oxford means the Chancellor, Masters and Scholars of the
University of Oxford, having an administrative office at Wellington
Square, Oxford OX1 2JD, UK.

This file is part of Chaste.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
 * Neither the name of the University of Oxford nor the names of its
   contributors may be used to endorse or promote products derived from this
   software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#ifndef IMMERSEDBOUNDARY2DARRAYS_HPP_
#define IMMERSEDBOUNDARY2DARRAYS_HPP_

#include <complex>
#include "ImmersedBoundaryArray.hpp"

/**
 * A class to store all arrays used by ImmersedBoundarySimulationModifier for 2D simulations.
 *
 * As these arrays will often be (very) large, it saves significant time to pre-allocate them and re-use during each
 * timestep, rather than creating them as needed.
 */
class ImmersedBoundary2dArrays
{
protected:

    /** Grid to store force acting on fluid */
    multi_array<double, 3> mForceGrids;

    /** Grid to calculate upwind scheme and store RHS of system */
    multi_array<double, 3> mRightHandSideGrids;

    /** Grid to store results of R2C fft */
    multi_array<std::complex<double>, 3> mFourierGrids;

    /** Grid to store the calculated pressure grid */
    multi_array<std::complex<double>, 2> mPressureGrid;

public:

    /**
     * Default constructor.
     *
     * @param numGridPtsX The number of grid points in the X direction
     * @param numGridPtsY The number of grid points in the X direction
     */
    ImmersedBoundary2dArrays(unsigned numGridPtsX, unsigned numGridPtsY);

    /**
     * Empty constructor
     */
    ImmersedBoundary2dArrays()
    {
    }

    /**
     * Destructor
     */
    virtual ~ImmersedBoundary2dArrays();

    /**
     * @return reference to modifiable force grids.
     */
    multi_array<double, 3>& rGetModifiableForceGrids();

    /**
     * @return reference to modifiable right-hand-side grids.
     */
    multi_array<double, 3>& rGetModifiableRightHandSideGrids();

    /**
     * @return reference to modifiable Fourier grids.
     */
    multi_array<std::complex<double>, 3>& rGetModifiableFourierGrids();

    /**
     * @return reference to modifiable pressure grid.
     */
    multi_array<std::complex<double>, 2>& rGetModifiablePressureGrid();

};

#endif /*IMMERSEDBOUNDARY2DARRAYS_HPP_*/
