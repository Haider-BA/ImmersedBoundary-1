/*

Copyright (c) 2005-2014, University of Oxford.
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

// Needed for test framework
#include <cxxtest/TestSuite.h>

// Needed for Immersed Boundary simulations
#include <complex>
#include <fftw3.h>
#include <boost/multi_array.hpp>

#include "ImmersedBoundaryArray.hpp"
#include "RandomNumberGenerator.hpp"
#include "Debug.hpp"

// This test is never run in parallel
#include "FakePetscSetup.hpp"

class TestGenerateFftwWisdom : public CxxTest::TestSuite
{
public:

    void TestGenerateManyR2CWisdomOneThread() throw(Exception)
    {
        // We first forget all wisdom and re-load, as threaded wisdom doesn't play well with un-threaded
        void fftw_forget_wisdom(void);

        /*
         * This test generates an fftw wisdom file telling fftw how to efficiently compute fourier transforms of a
         * given size, using two threads.  We generate wisdom for two DFTs on data contiguous in memory:
         *    * 2d forward R2C and backward C2R transforms (16x16 --> 4096x4096)
         *    * 3d forward R2C and backward C2R transforms (16x16x16 --> 256x256x256)
         *
         * This test takes a LONG time to run if there is currently no wisdom (around 4 hours).
         */

        std::string filename = "./projects/ImmersedBoundary/src/fftw.wisdom";
        int wisdom_flag = fftw_import_wisdom_from_filename(filename.c_str());

        // 1 means it's read correctly, 0 indicates a failure
        TS_ASSERT_EQUALS(wisdom_flag, 1);

        // Create 3D arrays that will represent two 2D arrays
        typedef boost::multi_array<std::complex<double>, 3> complex_array_2d;

        typedef boost::multi_array<double, 3> real_array_2d;

        // Create 2D wisdom
        for (unsigned i = 16 ; i < 5000 ; i*=2)
        {
            real_array_2d input(boost::extents[2][i][i]);
            complex_array_2d output(boost::extents[2][i][(i/2) + 1]);
            real_array_2d check(boost::extents[2][i][i]);

            double* fftw_input = &input[0][0][0];
            fftw_complex* fftw_output = reinterpret_cast<fftw_complex*>(&output[0][0][0]);
            double* fftw_check = &check[0][0][0];

            // Plan variables
            int rank = 2;                   // Number of dimensions for each array
            int real_dims[] = {i, i};       // Dimensions of each real array
            int comp_dims[] = {i, 1 + i/2}; // Dimensions of each complex array
            int how_many = 2;               // Number of transforms
            int real_sep = i * i;           // How many doubles between start of first array and start of second
            int comp_sep = i * (1 + i/2);   // How many fftw_complex between start of first array and start of second
            int real_stride = 1;            // Each real array is contiguous in memory
            int comp_stride = 1;            // Each complex array is contiguous in memory
            int* real_nembed = real_dims;
            int* comp_nembed = comp_dims;

            fftw_plan plan_f;
            plan_f = fftw_plan_many_dft_r2c(rank, real_dims, how_many,
                                            fftw_input,  real_nembed, real_stride, real_sep,
                                            fftw_output, comp_nembed, comp_stride, comp_sep,
                                            FFTW_EXHAUSTIVE);

            fftw_plan plan_b;
            plan_b = fftw_plan_many_dft_c2r(rank, real_dims, how_many,
                                            fftw_output, comp_nembed, comp_stride, comp_sep,
                                            fftw_check,  real_nembed, real_stride, real_sep,
                                            FFTW_EXHAUSTIVE);

            // We now verify that the forward followed by inverse transform produces the correct result
            for (unsigned dim = 0 ; dim < 2 ; dim++)
            {
                for (unsigned x = 0 ; x < i ; x++)
                {
                    for (unsigned y = 0 ; y < i ; y++)
                    {
                        input[dim][x][y] = RandomNumberGenerator::Instance()->ranf();
                    }
                }
            }

            fftw_execute(plan_f);
            fftw_execute(plan_b);

            for (unsigned dim = 0 ; dim < 2 ; dim++)
            {
                for (unsigned x = 0 ; x < i ; x++)
                {
                    for (unsigned y = 0 ; y < i ; y++)
                    {
                        TS_ASSERT_DELTA(input[dim][x][y], check[dim][x][y]/(i*i), 1e-10);
                    }
                }
            }

            // Export each step of the loop so if the test is stopped progress is kept
            fftw_export_wisdom_to_filename(filename.c_str());
            std::cout << "Exported 1-thread wisdom for two contiguous arrays of size " << i << " by " << i << std::endl;
        }
    }

    void TestGenerateManyR2CWisdomTwoThreads() throw(Exception)
    {
        // We first forget all wisdom and re-load, as threaded wisdom doesn't play well with un-threaded
        void fftw_forget_wisdom(void);

        /*
         * This test generates an fftw wisdom file telling fftw how to efficiently compute fourier transforms of a
         * given size, using two threads.  We generate wisdom for two DFTs on data contiguous in memory:
         *    * 2d forward R2C and backward C2R transforms (16x16 --> 4096x4096)
         *    * 3d forward R2C and backward C2R transforms (16x16x16 --> 256x256x256)
         *
         * This test takes a LONG time to run if there is currently no wisdom (around 4 hours).
         */

        int thread_flag = fftw_init_threads();

        // 1 means it's read correctly, 0 indicates a failure
        TS_ASSERT_EQUALS(thread_flag, 1);

        fftw_plan_with_nthreads(2);

        std::string filename = "./projects/ImmersedBoundary/src/fftw_threads.wisdom";
        int wisdom_flag = fftw_import_wisdom_from_filename(filename.c_str());

        // 1 means it's read correctly, 0 indicates a failure
        TS_ASSERT_EQUALS(wisdom_flag, 1);

        // Create 3D arrays that will represent two 2D arrays
        typedef boost::multi_array<std::complex<double>, 3> complex_array_2d;

        typedef boost::multi_array<double, 3> real_array_2d;

        // Create 2D wisdom
        for (unsigned i = 16 ; i < 5000 ; i*=2)
        {
            real_array_2d input(boost::extents[2][i][i]);
            complex_array_2d output(boost::extents[2][i][(i/2) + 1]);
            real_array_2d check(boost::extents[2][i][i]);

            double* fftw_input = &input[0][0][0];
            fftw_complex* fftw_output = reinterpret_cast<fftw_complex*>(&output[0][0][0]);
            double* fftw_check = &check[0][0][0];

            // Plan variables
            int rank = 2;                   // Number of dimensions for each array
            int real_dims[] = {i, i};       // Dimensions of each real array
            int comp_dims[] = {i, 1 + i/2}; // Dimensions of each complex array
            int how_many = 2;               // Number of transforms
            int real_sep = i * i;           // How many doubles between start of first array and start of second
            int comp_sep = i * (1 + i/2);   // How many fftw_complex between start of first array and start of second
            int real_stride = 1;            // Each real array is contiguous in memory
            int comp_stride = 1;            // Each complex array is contiguous in memory
            int* real_nembed = real_dims;
            int* comp_nembed = comp_dims;

            fftw_plan plan_f;
            plan_f = fftw_plan_many_dft_r2c(rank, real_dims, how_many,
                                            fftw_input,  real_nembed, real_stride, real_sep,
                                            fftw_output, comp_nembed, comp_stride, comp_sep,
                                            FFTW_EXHAUSTIVE);

            fftw_plan plan_b;
            plan_b = fftw_plan_many_dft_c2r(rank, real_dims, how_many,
                                            fftw_output, comp_nembed, comp_stride, comp_sep,
                                            fftw_check,  real_nembed, real_stride, real_sep,
                                            FFTW_EXHAUSTIVE);

            // We now verify that the forward followed by inverse transform produces the correct result
            for (unsigned dim = 0 ; dim < 2 ; dim++)
            {
                for (unsigned x = 0 ; x < i ; x++)
                {
                    for (unsigned y = 0 ; y < i ; y++)
                    {
                        input[dim][x][y] = RandomNumberGenerator::Instance()->ranf();
                    }
                }
            }

            fftw_execute(plan_f);
            fftw_execute(plan_b);

            for (unsigned dim = 0 ; dim < 2 ; dim++)
            {
                for (unsigned x = 0 ; x < i ; x++)
                {
                    for (unsigned y = 0 ; y < i ; y++)
                    {
                        TS_ASSERT_DELTA(input[dim][x][y], check[dim][x][y]/(i*i), 1e-10);
                    }
                }
            }

            // Export each step of the loop so if the test is stopped progress is kept
            fftw_export_wisdom_to_filename(filename.c_str());
            std::cout << "Exported 2-thread wisdom for two contiguous arrays of size " << i << " by " << i << std::endl;
        }
    }

    void TestGenerateManyR2CWisdomFourThreads() throw(Exception)
    {
        // We first forget all wisdom and re-load, as threaded wisdom doesn't play well with un-threaded
        void fftw_forget_wisdom(void);

        /*
         * This test generates an fftw wisdom file telling fftw how to efficiently compute fourier transforms of a
         * given size, using two threads.  We generate wisdom for two DFTs on data contiguous in memory:
         *    * 2d forward R2C and backward C2R transforms (16x16 --> 4096x4096)
         *    * 3d forward R2C and backward C2R transforms (16x16x16 --> 256x256x256)
         *
         * This test takes a LONG time to run if there is currently no wisdom (around 4 hours).
         */

        int thread_flag = fftw_init_threads();

        // 1 means it's read correctly, 0 indicates a failure
        TS_ASSERT_EQUALS(thread_flag, 1);

        fftw_plan_with_nthreads(4);

        std::string filename = "./projects/ImmersedBoundary/src/fftw_threads.wisdom";
        int wisdom_flag = fftw_import_wisdom_from_filename(filename.c_str());

        // 1 means it's read correctly, 0 indicates a failure
        TS_ASSERT_EQUALS(wisdom_flag, 1);

        // Create 3D arrays that will represent two 2D arrays
        typedef boost::multi_array<std::complex<double>, 3> complex_array_2d;

        typedef boost::multi_array<double, 3> real_array_2d;

        // Create 2D wisdom
        for (unsigned i = 16 ; i < 5000 ; i*=2)
        {
            real_array_2d input(boost::extents[2][i][i]);
            complex_array_2d output(boost::extents[2][i][(i/2) + 1]);
            real_array_2d check(boost::extents[2][i][i]);

            double* fftw_input = &input[0][0][0];
            fftw_complex* fftw_output = reinterpret_cast<fftw_complex*>(&output[0][0][0]);
            double* fftw_check = &check[0][0][0];

            // Plan variables
            int rank = 2;                   // Number of dimensions for each array
            int real_dims[] = {i, i};       // Dimensions of each real array
            int comp_dims[] = {i, 1 + i/2}; // Dimensions of each complex array
            int how_many = 2;               // Number of transforms
            int real_sep = i * i;           // How many doubles between start of first array and start of second
            int comp_sep = i * (1 + i/2);   // How many fftw_complex between start of first array and start of second
            int real_stride = 1;            // Each real array is contiguous in memory
            int comp_stride = 1;            // Each complex array is contiguous in memory
            int* real_nembed = real_dims;
            int* comp_nembed = comp_dims;

            fftw_plan plan_f;
            plan_f = fftw_plan_many_dft_r2c(rank, real_dims, how_many,
                                            fftw_input,  real_nembed, real_stride, real_sep,
                                            fftw_output, comp_nembed, comp_stride, comp_sep,
                                            FFTW_EXHAUSTIVE);

            fftw_plan plan_b;
            plan_b = fftw_plan_many_dft_c2r(rank, real_dims, how_many,
                                            fftw_output, comp_nembed, comp_stride, comp_sep,
                                            fftw_check,  real_nembed, real_stride, real_sep,
                                            FFTW_EXHAUSTIVE);

            // We now verify that the forward followed by inverse transform produces the correct result
            for (unsigned dim = 0 ; dim < 2 ; dim++)
            {
                for (unsigned x = 0 ; x < i ; x++)
                {
                    for (unsigned y = 0 ; y < i ; y++)
                    {
                        input[dim][x][y] = RandomNumberGenerator::Instance()->ranf();
                    }
                }
            }

            fftw_execute(plan_f);
            fftw_execute(plan_b);

            for (unsigned dim = 0 ; dim < 2 ; dim++)
            {
                for (unsigned x = 0 ; x < i ; x++)
                {
                    for (unsigned y = 0 ; y < i ; y++)
                    {
                        TS_ASSERT_DELTA(input[dim][x][y], check[dim][x][y]/(i*i), 1e-10);
                    }
                }
            }

            // Export each step of the loop so if the test is stopped progress is kept
            fftw_export_wisdom_to_filename(filename.c_str());
            std::cout << "Exported 4-thread wisdom for two contiguous arrays of size " << i << " by " << i << std::endl;
        }
    }
};