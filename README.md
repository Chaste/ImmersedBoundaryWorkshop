# Immersed Boundary Workshop
This workshop will guide you through four sections:

1. Setting up the immersed boundary code
2. Building single cell immersed boundary capable simulations
3. Multi-cellular simulations
4. Adding and manipulating fluid sources

## 1. Set Up

### Required Software
- git
- [Docker](https://www.docker.com)
- [VSCode](https://code.visualstudio.com) with [Dev Containers](https://code.visualstudio.com/docs/devcontainers/containers) extension
- [Paraview](https://www.paraview.org)

### Installation 

If you have already attended the Chaste installation session, you may have already completed some of these steps.

1. Clone the Chaste git repository: `git clone https://github.com/Chaste/Chaste`
2. Navigate into the repository with the terminal/command line: `cd Chaste`
3. Navigate into the `projects` subfolder: `cd projects`
4. Clone the workshop project: `git clone https://github.com/Chaste/ImmersedBoundaryWorkshop`

We are now ready to build the code

5. Open the repository using VSCode and the Dev Containers extension
6. Using the terminal in VSCode, navigate to the build directory: `cd build`
7. Because we have introduced new source files and a new project, we need to reconfigure the project. Do this using `cmake ../src`
8. Once this completes, we are ready to build the workshop project! `make -j8 project_ImmersedBoundaryWorkshop`. This may take some time the first time you run it.

Each time you make a change to the code, you will re-run `make -j8 project_ImmersedBoundaryWorkshop` to compile your changes. 

**Tip** You can use the up arrow whilst on the terminal to cycle through previously used commands

### Running the Code

The workshop code is implemented within a test file. We can run the code using the following command `ctest -V -R ImmersedBoundaryWorkshop`

**Tip** The build and run commands (`make`, `ctest`) will only work from the `build` folder

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

**Tip** You can also right click on a dataset and click reload files. This saves you going through all of the menus again.

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
The starting code for this exercise is found in `TestImmersedBoundaryWorkshop_Exercise_2`. The starter code is a simulation containing a single cell.

### 2.1 Adding More Cells
We can use the mesh generator to generate multiple cells. In the line of code

    ImmersedBoundaryPalisadeMeshGenerator gen(1, 128, 0.1, 2.0, 0.0, false);

the first parameter controls the number of cells. Try increasing this to generate more cells.

**Tip** A sensible range for this exercise will be 4-10 cells

### 2.2 Introducing Laminas
In addition to the cells we have seen so far, we can introduce laminas to the simulation. Laminas are surfaces with reduced dimensionality. For 3d elements, a lamina is a 2d surface. For the 2d elements we are working with, laminas are lines.

Change the last parameter of the mesh generator constructor from `false` to `true`. This will generate a basal lamina spanning the palisade cells. Laminas can also interact with the fluid field, and can be made 'leaky' to allow some flow across their boundary. This can be used to model a permeable boundary.

### 2.3 Cell Variations

As with the single cell in exercise 1.3, we can use the 3rd and 4th constructor parameters to modify the cell shapes. However, we can also introduce variation between cells by modifying the 5th parameter. Have a go at modifying the 5th parameter to the constructor.

### 2.4 Intercellular Interactions

So far, we have encountered forces that act to maintain the shape of the cell membrane. We can also introduce forces that apply between cells.

Using the code which sets up the `ImmersedBoundaryLinearMembraneForce` as a reference, add a new force with the `ImmersedBoundaryLinearInteractionForce` type and add it to the simulation. Instead of the `SetElementSpringConst` method, the interaction force has a `SetSpringConst` method which we should use. A sensible initial value is `1.0 * 1e6`. The rest length can also be modified using the `SetRestLength` method.


## Exercise 3 - Adding Fluid Sources

Now that we are familiar with how to generate the cells, we will introduce fluid sources. The starter code for this exericse is in the `TestImmersedBoundaryWorkshop_Exercise_3` method and contains a multi-cellular simulation.

### 3.1 Adding a Fluid Source

We will begin by constructing a fluid source object. Find the `EXERCISE 3.1` comment and add the code given below

    std::shared_ptr<FluidSource<2>> source = std::make_shared<FluidSource<2>>(0, 0.5, 0.7);

This constructs a `FluidSource` object in 2 dimensions and gets a pointer to it. The first parameter gives the index of the fluid source. Each source you create must have a unique index. The next parameters are the `x` and `y` coordinates of the source. Fluid sources in chaste are point-like, that is to say they do not have any area/volume.

Having created the fluid source, we set its strength:

    source->SetStrength(0.012);

Now, we must associate the source with an element in the simulation so that the simulation is aware of the source.

    p_mesh->GetElement(0)->SetFluidSource(source);

Finally, we must tell the cell population that fluid sources are present. Find the line of code 

    cell_population.SetIfPopulationHasActiveSources(false);

and change `false` to `true`.

### 3.2 Varying the Source Location

Experiment with the source location. Try moving it closer to and further away from the cells.

### 3.3 Varying the Source Strength

Try modifying the source strength to see what impact this has on the cell shapes.

### 3.4 Fluid-Cell Interaction

Have a go at modifying the spring constant of the `ImmersedBoundaryLinearMembraneForce` to see how this changes the effect of the fluid source on the cells.

### 3.5 Adding More Sources

Using the code for the first fluid source as a reference, add a second fluid source. You will need to use a unique index, and attach it to a different element as each element can only manage a single fluid source.

## Stretch Exercises

- Try integrating a different cell cycle model to introduce cell division. See how the presence of a fluid source impacts the structure that is formed.
- Use one of the cell writers to collect some statistics 
