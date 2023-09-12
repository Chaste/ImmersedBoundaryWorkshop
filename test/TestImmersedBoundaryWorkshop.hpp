/*

Copyright (c) 2005-2022, University of Oxford.
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

#ifndef TESTIMMERSEDBOUNDARYWORKSHOP_HPP_
#define TESTIMMERSEDBOUNDARYWORKSHOP_HPP_

#include <cxxtest/TestSuite.h>
/* Most Chaste code uses PETSc to solve linear algebra problems.  This involves starting PETSc at the beginning of a test-suite
 * and closing it at the end.  (If you never run code in parallel then it is safe to replace PetscSetupAndFinalize.hpp with FakePetscSetup.hpp)
 */
#include "PetscSetupAndFinalize.hpp"
#include "ImmersedBoundaryWorkshop.hpp"

#include "SuperellipseGenerator.hpp"
#include "ImmersedBoundarySimulationModifier.hpp"
#include "ImmersedBoundaryLinearInteractionForce.hpp"
#include "ImmersedBoundaryLinearMembraneForce.hpp"
#include "DifferentiatedCellProliferativeType.hpp"
#include "UniformCellCycleModel.hpp"
#include "CellsGenerator.hpp"
#include "SmartPointers.hpp"
#include "OffLatticeSimulation.hpp"
#include "ForwardEulerNumericalMethod.hpp"
#include "boost/make_shared.hpp"
#include "FluidSource.hpp"
#include "ImmersedBoundaryPalisadeMeshGenerator.hpp"
#include "AlwaysDivideCellCycleModel.hpp"

class TestImmersedBoundaryWorkshop : public CxxTest::TestSuite
{
public:
    void TestImmersedBoundaryWorkshop_Exercise_1()
    {
        SimulationTime::Instance()->SetStartTime(0.0);

        std::unique_ptr<SuperellipseGenerator> p_gen(new SuperellipseGenerator(128, 1.0, 0.4, 0.8, 0.3, 0.2));
        const std::vector<c_vector<double, 2> > locations = p_gen->GetPointsAsVectors();

        std::vector<Node<2>* > nodes;
        std::vector<ImmersedBoundaryElement<2,2>* > elements;

        for (unsigned location = 0; location < locations.size(); location++)
        {
            nodes.push_back(new Node<2>(location, locations[location], true));
        }

        elements.push_back(new ImmersedBoundaryElement<2,2>(0, nodes));

        std::unique_ptr<ImmersedBoundaryMesh<2,2>> p_mesh(new ImmersedBoundaryMesh<2,2>(nodes, elements));
        p_mesh->SetNumGridPtsXAndY(64);

        std::vector<CellPtr> cells;
        MAKE_PTR(DifferentiatedCellProliferativeType, p_cell_type);
        CellsGenerator<UniformCellCycleModel, 2> cells_generator;
        cells_generator.GenerateBasicRandom(cells, p_mesh->GetNumElements(), p_cell_type);

        ImmersedBoundaryCellPopulation<2> cell_population(*p_mesh, cells);
        cell_population.SetIfPopulationHasActiveSources(false);

        OffLatticeSimulation<2> simulator(cell_population);
        simulator.SetNumericalMethod(boost::make_shared<ForwardEulerNumericalMethod<2,2> >());
        simulator.GetNumericalMethod()->SetUseUpdateNodeLocation(true);

        // Add main immersed boundary simulation modifier
        MAKE_PTR(ImmersedBoundarySimulationModifier<2>, p_main_modifier);
        simulator.AddSimulationModifier(p_main_modifier);

        // Add force law
        MAKE_PTR(ImmersedBoundaryLinearMembraneForce<2>, p_boundary_force);
        p_main_modifier->AddImmersedBoundaryForce(p_boundary_force);
        p_boundary_force->SetElementSpringConst(1.0 * 1e7);
        
        // Set simulation properties
        double dt = 0.05;
        simulator.SetOutputDirectory("ImmersedBoundaryWorkshop_Exercise_1");
        simulator.SetDt(dt);
        simulator.SetSamplingTimestepMultiple(4u);
        simulator.SetEndTime(1000 * dt);

        simulator.Solve();
        
        SimulationTime::Instance()->Destroy();
    }

    void TestImmersedBoundaryWorkshop_Exercise_2()
    {
        SimulationTime::Instance()->SetStartTime(0.0);

        ImmersedBoundaryPalisadeMeshGenerator gen(2, 128, 0.1, 2.0, 0.0, false);
        ImmersedBoundaryMesh<2, 2>* p_mesh = gen.GetMesh();
        p_mesh->SetNumGridPtsXAndY(64);

        std::vector<CellPtr> cells;
        MAKE_PTR(DifferentiatedCellProliferativeType, p_cell_type);
        CellsGenerator<UniformCellCycleModel, 2> cells_generator;
        cells_generator.GenerateBasicRandom(cells, p_mesh->GetNumElements(), p_cell_type);

        ImmersedBoundaryCellPopulation<2> cell_population(*p_mesh, cells);
        cell_population.SetIfPopulationHasActiveSources(false);
        

        OffLatticeSimulation<2> simulator(cell_population);
        simulator.SetNumericalMethod(boost::make_shared<ForwardEulerNumericalMethod<2,2> >());
        simulator.GetNumericalMethod()->SetUseUpdateNodeLocation(true);

        // Add main immersed boundary simulation modifier
        MAKE_PTR(ImmersedBoundarySimulationModifier<2>, p_main_modifier);
        simulator.AddSimulationModifier(p_main_modifier);

        // Add force law
        MAKE_PTR(ImmersedBoundaryLinearMembraneForce<2>, p_boundary_force);
        p_main_modifier->AddImmersedBoundaryForce(p_boundary_force);
        p_boundary_force->SetElementSpringConst(1.0 * 1e7);
        
        MAKE_PTR(ImmersedBoundaryLinearInteractionForce<2>, p_cell_cell_force);
        p_main_modifier->AddImmersedBoundaryForce(p_cell_cell_force);
        p_cell_cell_force->SetSpringConst(1.0 * 1e6);
        p_cell_cell_force->SetRestLength(5.1);

        // Set simulation properties
        double dt = 0.05;
        simulator.SetOutputDirectory("ImmersedBoundaryWorkshop_Exercise_2");
        simulator.SetDt(dt);
        simulator.SetSamplingTimestepMultiple(4u);
        simulator.SetEndTime(1000 * dt);

        simulator.Solve();
        
        SimulationTime::Instance()->Destroy();
    }

    // 3.1 Introduce fluid source
    // 3.2 Vary source location
    // 3.3 Vary source strength
    // 3.4 Vary boundary stiffness 
    // 3.5 Add a second fluid source
    void TestImmersedBoundaryWorkshop_Exercise_3()
    {
        
        SimulationTime::Instance()->SetStartTime(0.0);

        ImmersedBoundaryPalisadeMeshGenerator gen(5, 128, 0.1, 2.0, 0.0, false);
        ImmersedBoundaryMesh<2, 2>* p_mesh = gen.GetMesh();
        p_mesh->SetNumGridPtsXAndY(64);

        std::vector<CellPtr> cells;
        MAKE_PTR(DifferentiatedCellProliferativeType, p_cell_type);
        CellsGenerator<UniformCellCycleModel, 2> cells_generator;
        cells_generator.GenerateBasicRandom(cells, p_mesh->GetNumElements(), p_cell_type);

        // 0.5, 0.7
        FluidSource<2>* source = new FluidSource<2>(0, 0.5, 0.7);
        // 0.005
        source->SetStrength(0.012);
        p_mesh->GetElement(2)->SetFluidSource(source);

        // 0.5, 0.7
        FluidSource<2>* source2 = new FluidSource<2>(1, 0.5, 0.3);
        // 0.005
        source2->SetStrength(0.012);
        p_mesh->GetElement(3)->SetFluidSource(source2);

        ImmersedBoundaryCellPopulation<2> cell_population(*p_mesh, cells);
        cell_population.SetIfPopulationHasActiveSources(true);
        

        OffLatticeSimulation<2> simulator(cell_population);
        simulator.SetNumericalMethod(boost::make_shared<ForwardEulerNumericalMethod<2,2> >());
        simulator.GetNumericalMethod()->SetUseUpdateNodeLocation(true);

        // Add main immersed boundary simulation modifier
        MAKE_PTR(ImmersedBoundarySimulationModifier<2>, p_main_modifier);
        simulator.AddSimulationModifier(p_main_modifier);

        // Add force law
        MAKE_PTR(ImmersedBoundaryLinearMembraneForce<2>, p_boundary_force);
        p_main_modifier->AddImmersedBoundaryForce(p_boundary_force);
        // 1e7
        p_boundary_force->SetElementSpringConst(1.0 * 1e7);
        
        MAKE_PTR(ImmersedBoundaryLinearInteractionForce<2>, p_cell_cell_force);
        p_main_modifier->AddImmersedBoundaryForce(p_cell_cell_force);
        p_cell_cell_force->SetSpringConst(1.0 * 1e6);

        // Set simulation properties
        double dt = 0.05;
        simulator.SetOutputDirectory("ImmersedBoundaryWorkshop_Exercise_3");
        simulator.SetDt(dt);
        simulator.SetSamplingTimestepMultiple(4u);
        simulator.SetEndTime(300 * dt);

        simulator.Solve();
        
        SimulationTime::Instance()->Destroy();
    }

    void TestImmersedBoundaryWorkshop_Exercise_4()
    {
        
        SimulationTime::Instance()->SetStartTime(0.0);

        ImmersedBoundaryPalisadeMeshGenerator gen(5, 128, 0.1, 2.0, 0.0, false);
        ImmersedBoundaryMesh<2, 2>* p_mesh = gen.GetMesh();
        p_mesh->SetNumGridPtsXAndY(64);

        std::vector<CellPtr> cells;
        MAKE_PTR(DifferentiatedCellProliferativeType, p_cell_type);
        CellsGenerator<AlwaysDivideCellCycleModel, 2> cells_generator;
        cells_generator.GenerateBasicRandom(cells, p_mesh->GetNumElements(), p_cell_type);

        // 0.5, 0.7
        FluidSource<2>* source = new FluidSource<2>(0, 0.5, 0.7);
        // 0.005
        source->SetStrength(0.012);
        p_mesh->GetElement(2)->SetFluidSource(source);

        // 0.5, 0.7
        FluidSource<2>* source2 = new FluidSource<2>(1, 0.5, 0.3);
        // 0.005
        source2->SetStrength(0.012);
        p_mesh->GetElement(3)->SetFluidSource(source2);

        ImmersedBoundaryCellPopulation<2> cell_population(*p_mesh, cells);
        cell_population.SetIfPopulationHasActiveSources(true);
        

        OffLatticeSimulation<2> simulator(cell_population);
        simulator.SetNumericalMethod(boost::make_shared<ForwardEulerNumericalMethod<2,2> >());
        simulator.GetNumericalMethod()->SetUseUpdateNodeLocation(true);

        // Add main immersed boundary simulation modifier
        MAKE_PTR(ImmersedBoundarySimulationModifier<2>, p_main_modifier);
        simulator.AddSimulationModifier(p_main_modifier);

        // Add force law
        MAKE_PTR(ImmersedBoundaryLinearMembraneForce<2>, p_boundary_force);
        p_main_modifier->AddImmersedBoundaryForce(p_boundary_force);
        // 1e7
        p_boundary_force->SetElementSpringConst(1.0 * 1e7);
        
        MAKE_PTR(ImmersedBoundaryLinearInteractionForce<2>, p_cell_cell_force);
        p_main_modifier->AddImmersedBoundaryForce(p_cell_cell_force);
        p_cell_cell_force->SetSpringConst(1.0 * 1e6);

        // Set simulation properties
        double dt = 0.05;
        simulator.SetOutputDirectory("ImmersedBoundaryWorkshop_Exercise_3");
        simulator.SetDt(dt);
        simulator.SetSamplingTimestepMultiple(4u);
        simulator.SetEndTime(300 * dt);

        simulator.Solve();
        
        SimulationTime::Instance()->Destroy();
    }
};

#endif /*TESTIMMERSEDBOUNDARYWORKSHOP_HPP_*/
