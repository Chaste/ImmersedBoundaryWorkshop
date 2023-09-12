# Immersed Boundary Workshop - 12/09/2023
This workshop will guide you through four sections:

1. Setting up the immersed boundary code
2. Building simple immersed boundary capable simulations
3. Adding and manipulating fluid sources
4. Integrating cell functionality

## 1. Set Up

### Required Software
- git
- docker
- vscode with devcontainers extension
- paraview

### Installation 

If you have already attended the chaste with docker workshop this morning, you may have already completed some of these steps.

1. Clone the chaste git repository `git clone git@github.com:Chaste/Chaste.git`
2. Navigate into the repository with the terminal/command line
3. Checkout the immersed boundary code `git checkout mleach/immersed-boundary-pr`
4. Navigate into the `projects` subfolder
5. Clone the workshop project `git clone git@github.com:MILeach/ImmersedBoundaryWorkshop.git`

We are now ready to build the code

6. Open the repository using vscode and the devcontainers extension
7. Using the termainal in vscode, navigate to the lib directory `cd lib`
8. Because we have introduced new source files and a new project, we need to reconfigure the project. Do this using `cmake ../src`
9. Once this completes, we are ready to build the workshop project! `make -j8 project_ImmersedBoundaryWorkshop`. This may take some time the first time you run it.

Each time you make a change to the code, you will re-run `make -j8 project_ImmersedBoundaryWorkshop` to compile your changes. 

**Tip** You can use the up arrow whilst on the terminal to cycle through previously used commands

### Running the Code

The workshop code is implemented within a test file. We can run the code using the following command `ctest -V -R ImmersedBoundaryWorkshop`

**Tip** The build and run commands (`make`, `ctest`) will only work from the `lib` folder

### Editing the Code

The code for the workshop is in `projects/ImmersedBoundaryWorkshop/test/TestImmersedBoundaryWorkshop.hpp`

**Tip** The starting code for each exercise is in a separate function. Each exercise has a return statement at the beginning. This will cause the function to exit without running the following code. You can comment these in or out to enable/disable running particular exercises.

## Exercise 1 - Simple Immersed Boundary Simulations
We will begin by exploring simulations containing a single cell. This will familiarise you with how to generate immersed boundary cells, the steps involved in setting up an immersed boundary simulation, and the options available for controlling how the cells are generated and behave.

Immersed boundary simulations operate over a square domain, with `x` and `y` coordinates lying in the range `0` to `1`. The domain wraps on both axes - this means that if a cell moves off the right hand edge of the domain, the segment will appear on the left hand side. This is not purely visual; forces are also transmitted across these boundaries.

**Tip** Make sure all your coordinates are between `0` and `1`.

### 1.1 The First Cell

The starter code for exercise one is `TestImmersedBoundaryWorkshop_Exercise_1` in the `TestImmersedBoundaryWorkshop.hpp` file. The starter code sets up a minimal immersed boundary simulation. We begin the simulation set up by setting the start time to 0.
    
    // Set the start time for the simulation
    SimulationTime::Instance()->SetStartTime(0.0);

**Tip** Lines of code beginning with `//` are comments in C++

Next, we define the necessary geometry.
        
    // Generate a mesh containing a single cell
    ImmersedBoundaryPalisadeMeshGenerator gen(1, 128, 0.1, 2.0, 0.0, false);
    ImmersedBoundaryMesh<2, 2>* p_mesh = gen.GetMesh();

The first line of code defines an `ImmersedBoundaryPalisadeMeshGenerator` called `gen`. We will explore the parameters of the mesh generator in more detail later. The second line of code instructs the mesh generator to generate a mesh and we store a pointer to it. The angle brackets `<2, 2>` following denote that we are using a 2-dimensional space, and 2-dimensional elements to define the mesh.

We now set the fluid grid resolution. The following code specifies that we are using a 64x64 grid to simulate our fluid over.

    // Set the fluid grid resolution
    p_mesh->SetNumGridPtsXAndY(64);

Next, we set up the cell population

    // Set up the cell population
    std::vector<CellPtr> cells;
    MAKE_PTR(DifferentiatedCellProliferativeType, p_cell_type);
    CellsGenerator<UniformCellCycleModel, 2> cells_generator;
    cells_generator.GenerateBasicRandom(cells, p_mesh->GetNumElements(), p_cell_type);

    ImmersedBoundaryCellPopulation<2> cell_population(*p_mesh, cells);

We allocate storage space for the cells, and then specify a cell type and cell cycle model. These can be interchanged to modify the life cycle of the cells. The `CellsGenerator` then constructs the necessary information for each of the elements in the mesh. Finally, we construct an `ImmersedBoundaryCellPopulation`. We then specify whether the population has active fluid sources or not. For now, we are not using any fluid sources, so we set this to false

    // Specify whether the population has active fluid sources
    cell_population.SetIfPopulationHasActiveSources(false);


As in the previous workshops, we specify a simulator to control the simulation. Here we use an `OffLatticeSimulation`. Although the fluid is simulated on a lattice (grid), the nodes/cells are not bound to a lattice. 

    // Create a simulator to manage our simulation
    OffLatticeSimulation<2> simulator(cell_population);
    simulator.SetNumericalMethod(boost::make_shared<ForwardEulerNumericalMethod<2,2> >());
    simulator.GetNumericalMethod()->SetUseUpdateNodeLocation(true);

As we have an off-lattice simulation, we need a way to model the fluid. This is handled by the `ImmersedBoundarySimulationModifier`. Modifiers in chaste are classes that can be attached to simulations to perform some additional custom functionality each timestep. In this case, the modifier is responsible for solving the Navier-Stokes equations and propagating forces between the nodes and the fluid.

    // Add an immersed boundary simulation modifier
    // The modifier is responsible for managing the fluid simulation
    // and propagating forces between the fluid & cell
    MAKE_PTR(ImmersedBoundarySimulationModifier<2>, p_main_modifier);
    simulator.AddSimulationModifier(p_main_modifier);

We must also provide the modifier with a force model to govern interactions between the nodes forming the boundary of the cells. Note that these forces only act between nodes in the same cell, they do not control interactions between cells.

    // Add a force law to model the behaviour of the cell membrane
    MAKE_PTR(ImmersedBoundaryLinearMembraneForce<2>, p_boundary_force);
    p_main_modifier->AddImmersedBoundaryForce(p_boundary_force);
    p_boundary_force->SetElementSpringConst(1.0 * 1e7);

The spring constant defines how stiff the cell boundary is. Finally, we set up the simulation properties and run it.

    // Set simulation properties
    double dt = 0.05;
    simulator.SetOutputDirectory("ImmersedBoundaryWorkshop_Exercise_1");
    simulator.SetDt(dt);
    simulator.SetSamplingTimestepMultiple(4u);
    simulator.SetEndTime(1000 * dt);

    // Perform the simulation
    simulator.Solve();
    
    SimulationTime::Instance()->Destroy();

### 1.2 Visualising the Results

The results from the simulation will be written to the `testouput` directory. Within this, the results from the first exercise will be inside a folder called `ImmersedBoundaryWorkshop_Exercise_1`. If you are using vscode and the devcontainers extension, you can save the results to your local machine by right clicking the folder in the vscode explorer and clicking download.

We will then use paraview to display the simulation results.
1. Open Paraview.
2. Click File -> Open.
3. Navigate to the `ImmersedBoundaryWorkshop_Exercise_1` folder. A few files will be shown. We will open the `.pvd` file.
4. `results.pvd` will now be shown in the pipeline browser (left hand pane).
5. To the left of `results.pvd` there is a small icon of a closed eye. Click on this icon to show the dataset.
6. Use the control arrows at the top to play back the simulation.

You can use this process each time you modify the simulation to visualise your results.

### 1.3 Modifying the Initial Shape

We will now see how we can vary the initial shape of cells generated using the `ImmersedBoundaryPalisadeMeshGenerator`

Find the following line of code, which sets up the mesh generator:

    ImmersedBoundaryPalisadeMeshGenerator gen(1, 128, 0.1, 2.0, 0.0, false);

The 3rd and 4th parameters control the exponent of the superellipse and the aspect ratio of the cell. Experiment with modifying these to change the initial shape of the cell.

**Tip** There are comments labelling the correct bits of code for each exercise, e.g. `EXERCISE 1.3`. You can search for these to quickly find the right section.


### 1.4 Exploring Cell Membrane Forces

In addition to changing the initial shape of the cells, we can also adapt the behaviour of the cell membrane by modifying the force behaviour between nodes of the cell boundary.

Find the following line of code, and try adjusting the parameter. You can experiment with different combinations of spring constants and cell shapes.

    p_boundary_force->SetElementSpringConst(1.0 * 1e7);

The `ImmersedBoundaryLinearMembraneForce` models forces between membrane nodes using linear springs i.e, the force applied is proportional to the deviation of the distance between nodes from a rest length.

# Exercise 2 - Adding More Cells

### 2.1 Adding More Cells

### 2.2 Introducing Laminas

### 2.3 Cell Variations

### 2.4 Intercellular Interactions

## Exercise 3 - Adding Fluid Sources

### 3.1 Adding a Fluid Source

FluidSource<2> (0, 0.5, 0.7)

### 3.2 Varying the Source Location

### 3.3 Varying the Source Strength

### 3.4 Fluid-Cell Interaction

### 3.5 Adding More Sources