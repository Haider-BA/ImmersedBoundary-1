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

#include <cxxtest/TestSuite.h>
#include "CheckpointArchiveTypes.hpp"

#include "SimulationTime.hpp"
#include "RandomNumberGenerator.hpp"
#include "ExecutableSupport.hpp"

#include "Debug.hpp"

/*
 * These headers handle passing parameters to the executable.
 */
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/parsers.hpp>

#include "OffLatticeSimulation.hpp"
#include "StochasticDurationCellCycleModel.hpp"
#include "CellsGenerator.hpp"
#include "ImmersedBoundaryMesh.hpp"
#include "ImmersedBoundaryCellPopulation.hpp"
#include "ImmersedBoundarySimulationModifier.hpp"
#include "ImmersedBoundaryPalisadeMeshGenerator.hpp"
#include "ImmersedBoundaryMembraneElasticityForce.hpp"
#include "ImmersedBoundaryCellCellInteractionForce.hpp"

/*
 * Prototype functions
 */
void SetupSingletons(unsigned simulationId);
void DestroySingletons();
void SetupAndRunSimulation(unsigned simulation_id, unsigned spring_const);
void OutputOnCompletion(unsigned simulation_id, unsigned spring_const);

int main(int argc, char *argv[])
{
    // This sets up PETSc and prints out copyright information, etc.
    ExecutableSupport::StandardStartup(&argc, &argv);

    // Define command line options
    boost::program_options::options_description general_options("This is a  sample chaste executable.\n");
    general_options.add_options()
                    ("help", "produce help message")
                    ("ID", boost::program_options::value<unsigned>()->default_value(0),"ID of the simulation (for output)")
                    ("K", boost::program_options::value<unsigned>()->default_value(0),"Cell-cell spring const for the simulation");

    // define parse command line into variables_map
    boost::program_options::variables_map variables_map;
    boost::program_options::store(parse_command_line(argc, argv, general_options), variables_map);

    // print help message if wanted
    if (variables_map.count("help"))
    {
        std::cout << setprecision(3) << general_options << "\n";
        std::cout << general_options << "\n";
        return 1;
    }

    // get id and name from command line
    unsigned simulation_id=variables_map["ID"].as<unsigned>();
    unsigned spring_const=variables_map["K"].as<unsigned>();

    SetupSingletons(simulation_id);
    SetupAndRunSimulation(simulation_id, spring_const);
    DestroySingletons();
    OutputOnCompletion(simulation_id, spring_const);
}

void SetupSingletons(unsigned simulationId)
{
    // Set up what the test suite would do
    SimulationTime::Instance()->SetStartTime(0.0);

    // Reseed with 0 for same random numbers each time, or time(NULL) or simulation_id to change each realisation
    RandomNumberGenerator::Instance()->Reseed(simulationId);
    CellPropertyRegistry::Instance()->Clear();
    CellId::ResetMaxCellId();
}

void DestroySingletons()
{
    // this is from the tearDown method of the test suite
    SimulationTime::Destroy();
    RandomNumberGenerator::Destroy();
    CellPropertyRegistry::Instance()->Clear();
}

void OutputOnCompletion(unsigned simulation_id, unsigned springConst)
{
    // Compose the message
    std::stringstream message;
    message << "Completed simulation with spring const " << springConst << " and ID " << simulation_id << std::endl;

    // Send it to the console
    std::cout << message.str() << std::flush;
}
void SetupAndRunSimulation(unsigned simulation_id, unsigned spring_const)
{
    double fp_spring_const = 0.01 * (double)spring_const * 1e4;
    /*
     * 1: Num cells
     * 2: Num nodes per cell
     * 3: Superellipse exponent
     * 4: Superellipse aspect ratio
     * 5: Random y-variation
     * 6: Include membrane
     */
    ImmersedBoundaryPalisadeMeshGenerator gen(9, 256, 0.1, 3.0, 0.0, true);
    ImmersedBoundaryMesh<2, 2>* p_mesh = gen.GetMesh();

    p_mesh->SetNumGridPtsXAndY(512);

    std::vector<CellPtr> cells;
    MAKE_PTR(DifferentiatedCellProliferativeType, p_diff_type);
    CellsGenerator<StochasticDurationCellCycleModel, 2> cells_generator;
    cells_generator.GenerateBasicRandom(cells, p_mesh->GetNumElements(), p_diff_type);

    ImmersedBoundaryCellPopulation<2> cell_population(*p_mesh, cells);
    cell_population.SetIfPopulationHasActiveSources(true);

    OffLatticeSimulation<2> simulator(cell_population);
//    simulator.SetConsoleProgressOutput(false);

    // Add main immersed boundary simulation modifier
    MAKE_PTR(ImmersedBoundarySimulationModifier<2>, p_main_modifier);
    simulator.AddSimulationModifier(p_main_modifier);

    // Add force laws
    MAKE_PTR_ARGS(ImmersedBoundaryMembraneElasticityForce<2>, p_boundary_force, (cell_population));
    p_main_modifier->AddImmersedBoundaryForce(p_boundary_force);

    MAKE_PTR_ARGS(ImmersedBoundaryCellCellInteractionForce<2>, p_cell_cell_force, (cell_population));
    p_main_modifier->AddImmersedBoundaryForce(p_cell_cell_force);
    p_cell_cell_force->SetSpringConstant(fp_spring_const);

    // Create and set an output directory that is different for each
    std::stringstream output_directory;
    output_directory << "Exe_VaryCellCellAdhesion/" << spring_const << "_" << simulation_id;
    simulator.SetOutputDirectory(output_directory.str());

    // Set simulation properties
    double dt = 0.005;
    simulator.SetDt(dt);
    simulator.SetSamplingTimestepMultiple(5);
    simulator.SetEndTime(50.0 * dt);
    simulator.Solve();


    // Get height of basement lamina
    double lamina_height = 0.0;
    for (unsigned node_idx = 0 ; node_idx < p_mesh->GetElement(0)->GetNumNodes() ; node_idx++)
    {
        lamina_height += p_mesh->GetElement(0)->GetNode(node_idx)->rGetModifiableLocation()[1];
    }
    lamina_height /= p_mesh->GetElement(0)->GetNumNodes();

    // Kick
    for (unsigned elem_idx = 1 ; elem_idx < p_mesh->GetNumElements() ; elem_idx++)
    {
        double kick = 1.1 - 0.2 * RandomNumberGenerator::Instance()->ranf();

        for (unsigned node_idx = 0 ; node_idx < p_mesh->GetElement(elem_idx)->GetNumNodes() ; node_idx++)
        {
            double new_height = lamina_height + kick * (p_mesh->GetElement(elem_idx)->GetNode(node_idx)->rGetLocation()[1] - lamina_height);

            p_mesh->GetElement(elem_idx)->GetNode(node_idx)->rGetModifiableLocation()[1] = new_height;
        }
    }

    simulator.SetEndTime(500.0 * dt);
    simulator.Solve();

    // Compute tortuosity (defined as ratio of total length to straight-line length) of piecewise linear curve through centroids of successive elements
    double total_length = 0.0;

    c_vector<double, 2> previous_centroid = p_mesh->GetCentroidOfElement(1);

    for (unsigned elem_idx = 2 ; elem_idx < p_mesh->GetNumElements() ; elem_idx++)
    {
        c_vector<double, 2> this_centroid = p_mesh->GetCentroidOfElement(elem_idx);
        total_length += norm_2(p_mesh->GetVectorFromAtoB(previous_centroid, this_centroid));
        previous_centroid = this_centroid;
    }

    c_vector<double, 2> first_centroid = p_mesh->GetCentroidOfElement(1);
    c_vector<double, 2> last_centroid = p_mesh->GetCentroidOfElement(p_mesh->GetNumElements()-1);
    double straight_line_length = norm_2(p_mesh->GetVectorFromAtoB(first_centroid, last_centroid));
    straight_line_length = std::max(straight_line_length, 1.0-straight_line_length);
    double tortuosity = total_length / straight_line_length;

    std::vector<double> output_stuff;
    output_stuff.push_back(fp_spring_const);
    output_stuff.push_back(tortuosity);

    OutputFileHandler results_handler(output_directory.str(), false);
    out_stream results_file = results_handler.OpenOutputFile("results.dat");


    // Output summary statistics to results file
    (*results_file)<< fp_spring_const << "," << tortuosity;

    // Tidy up
    results_file->close();
}