#ifndef _SIMULATION_H_
#define _SIMULATION_H_

#include <petscksp.h>
#include <float.h>
#include "FlowField.h"
#include "stencils/FGHStencil.h"
#include "stencils/MovingWallStencils.h"
#include "stencils/RHSStencil.h"
#include "stencils/VelocityStencil.h"
#include "stencils/ObstacleStencil.h"
#include "stencils/VTKStencil.h"
#include "stencils/MaxUStencil.h"
#include "stencils/PeriodicBoundaryStencils.h"
#include "stencils/BFStepInitStencil.h"
#include "stencils/NeumannBoundaryStencils.h"
#include "stencils/BFInputStencils.h"
#include "stencils/InitTaylorGreenFlowFieldStencil.h"
#include "GlobalBoundaryFactory.h"
#include "Iterators.h"
#include "Definitions.h"

#include "LinearSolver.h"
#include "solvers/SORSolver.h"
#include "solvers/PetscSolver.h"



class Simulation {
  protected:
    Parameters &_parameters;

    FlowField &_flowField;

    MaxUStencil _maxUStencil;
    FieldIterator<FlowField> _maxUFieldIterator;
    GlobalBoundaryIterator<FlowField> _maxUBoundaryIterator;

    // Set up the boundary conditions
    GlobalBoundaryFactory _globalBoundaryFactory;
    GlobalBoundaryIterator<FlowField> _wallVelocityIterator;
    GlobalBoundaryIterator<FlowField> _wallFGHIterator;

    FGHStencil _fghStencil;
    FieldIterator<FlowField> _fghIterator;

    RHSStencil _rhsStencil;
    FieldIterator<FlowField> _rhsIterator;

    VelocityStencil _velocityStencil;
    ObstacleStencil _obstacleStencil;
    FieldIterator<FlowField> _velocityIterator;
    FieldIterator<FlowField> _obstacleIterator;

    PetscSolver _solver;


  public:
    Simulation(Parameters &parameters, FlowField &flowField):
       _parameters(parameters),
       _flowField(flowField),
       _maxUStencil(parameters),
       _maxUFieldIterator(_flowField,parameters,_maxUStencil),
       _maxUBoundaryIterator(_flowField,parameters,_maxUStencil),
       _globalBoundaryFactory(parameters),
       _wallVelocityIterator(_globalBoundaryFactory.getGlobalBoundaryVelocityIterator(_flowField)),
       _wallFGHIterator(_globalBoundaryFactory.getGlobalBoundaryFGHIterator(_flowField)),
       _fghStencil(parameters),
       _fghIterator(_flowField,parameters,_fghStencil),
       _rhsStencil(parameters),
       _rhsIterator(_flowField,parameters,_rhsStencil),
       _velocityStencil(parameters),
       _obstacleStencil(parameters),
       _velocityIterator(_flowField,parameters,_velocityStencil),
       _obstacleIterator(_flowField,parameters,_obstacleStencil),
       _solver(_flowField,parameters)
       {
       }

    virtual ~Simulation(){}

    /** initialises the flow field according to the scenario */
    virtual void initializeFlowField(){
      if (_parameters.simulation.scenario=="taylor-green"){
        // currently, a particular initialization is only requrid for the taylor-green vortex
        InitTaylorGreenFlowFieldStencil stencil(_parameters);
        FieldIterator<FlowField> iterator(_flowField,_parameters,stencil);
        iterator.iterate();
      } else if (_parameters.simulation.scenario=="channel"){
        BFStepInitStencil stencil(_parameters);
        FieldIterator<FlowField> iterator(_flowField,_parameters,stencil,0,1);
        iterator.iterate();
	_wallVelocityIterator.iterate();
      } else if (_parameters.simulation.scenario=="pressure-channel"){
	    //set pressure boundaries here for left wall
        const FLOAT value = _parameters.walls.scalarLeft;
        ScalarField& rhs = _flowField.getRHS();

        if (_parameters.geometry.dim==2){
          const int sizey = _flowField.getNy();
          for (int i =0 ;i < sizey+3;i++) {
            rhs.getScalar(0,i) = value;
          }
        } else {
          const int sizey = _flowField.getNy();
          const int sizez = _flowField.getNz();
          for (int i=0;i<sizey+3;i++)
            for(int j=0;j<sizez + 3;j++)
              rhs.getScalar(0,i,j) =value;
	    }
	    
	    // do same procedure for domain flagging as for regular channel
	    BFStepInitStencil stencil(_parameters);
        FieldIterator<FlowField> iterator(_flowField,_parameters,stencil,0,1);
        iterator.iterate();
	  }
      	_solver.reInitMatrix();
    }

    virtual void solveTimestep(){
        // determine and set max. timestep which is allowed in this simulation
        setTimeStep();
        // compute fgh
        _fghIterator.iterate();
        // set global boundary values
        _wallFGHIterator.iterate();
        // compute the right hand side
        _rhsIterator.iterate();
        // solve for pressure 
        _solver.solve();
        // TODO WS2: communicate pressure values
        // compute velocity
        _velocityIterator.iterate();
	// set obstacle boundaries
	_obstacleIterator.iterate();
        // TODO WS2: communicate velocity values
        // Iterate for velocities on the boundary
        _wallVelocityIterator.iterate();
    }

    /** TODO WS1: plots the flow field. */
    virtual void plotVTK(int timeStep){
      // TODO WS1: create VTKStencil and respective iterator; iterate stencil
      //           over _flowField and write flow field information to vtk file
    	VTKStencil vtkStencil(_parameters);
    	FieldIterator<FlowField> vtkIt(_flowField, _parameters, vtkStencil);

    	removeTempFiles();
    	if(!initializeTempFiles()){
    		std::cout << "Failed to initialize files!" << std::endl;
    		return;
    	}

    	vtkIt.iterate();

    	std::ostringstream temp_stream;
    	temp_stream << "vtkFiles/" << _parameters.vtk.prefix << timeStep << ".vtk";
    	std::string vtkFileName = temp_stream.str();

    	if(!combineTempFiles(vtkFileName))
    		std::cout << "Failed to create vtk file!" << std::endl;
    	else
    		std::cout << "Succesfully created a vtk file!" << std::endl;


    	removeTempFiles();
    }

  protected:
    /** sets the time step*/
    virtual void setTimeStep(){

      FLOAT localMin, globalMin;
      assertion(_parameters.geometry.dim == 2 || _parameters.geometry.dim == 3);
      FLOAT factor = 1.0/(_parameters.meshsize->getDxMin() * _parameters.meshsize->getDxMin()) +
                     1.0/(_parameters.meshsize->getDyMin() * _parameters.meshsize->getDyMin());

      // determine maximum velocity
      _maxUStencil.reset();
      _maxUFieldIterator.iterate();
      _maxUBoundaryIterator.iterate();
      if (_parameters.geometry.dim == 3) {
        factor += 1.0/(_parameters.meshsize->getDzMin() * _parameters.meshsize->getDzMin());
        _parameters.timestep.dt = 1.0 / _maxUStencil.getMaxValues()[2];
      } else {
        _parameters.timestep.dt = 1.0 / _maxUStencil.getMaxValues()[0];
      }

      localMin = std::min(_parameters.timestep.dt,
                                        std::min(std::min(_parameters.flow.Re/(2*factor),
                                        1.0 / _maxUStencil.getMaxValues()[0]),
                                        1.0 / _maxUStencil.getMaxValues()[1]));

      // Here, we select the type of operation before compiling. This allows to use the correct
      // data type for MPI. Not a concern for small simulations, but useful if using heterogeneous
      // machines.

      globalMin = MY_FLOAT_MAX;
      MPI_Allreduce(&localMin, &globalMin, 1, MY_MPI_FLOAT, MPI_MIN, PETSC_COMM_WORLD);

      _parameters.timestep.dt = globalMin;
      _parameters.timestep.dt *= _parameters.timestep.tau;
    }

    void removeTempFiles(){
    	std::remove("vtkFiles/_pressure.temp");
    	std::remove("vtkFiles/_velocity.temp");
    	std::remove("vtkFiles/_points.temp");
    }

    bool initializeTempFiles(){
    	std::fstream pointsFile(POINTS_FILE, std::fstream::out);
    	std::fstream pressureFile(PRESSURE_FILE, std::fstream::out);
    	std::fstream velocityFile(VELOCITY_FILE, std::fstream::out);
    	std::ostringstream temp_stream;
    	std::string temp_string;
    	int dim = _parameters.geometry.dim;
    	int dim_x = _flowField.getNx() + 1;
    	int dim_y = _flowField.getNy() + 1;
    	int dim_z = dim == 3 ? _flowField.getNz() + 1 : 1;
    	int points = dim_x*dim_y*dim_z;
    	int cells = dim == 3 ? (dim_x-1)*(dim_y-1)*(dim_z-1) : (dim_x-1)*(dim_y-1);
    	std::string precission_type = sizeof(FLOAT) == 4 ? "float" : "double";

    	if (pointsFile.fail())
    		return false;
    	else{
    		temp_string = "DATASET STRUCTURED_GRID\n";
    		pointsFile.write(temp_string.c_str(),temp_string.length());

    		temp_stream << "DIMENSIONS " << dim_x << " " << dim_y << " " << dim_z << "\n";
    		temp_string = temp_stream.str();
    		pointsFile.write(temp_string.c_str(),temp_string.length());

    		temp_stream.str("");
    		temp_stream.clear();
    		temp_stream << "POINTS " << points << " " << precission_type << "\n";
    		temp_string = temp_stream.str();
    		pointsFile.write(temp_string.c_str(),temp_string.length());

    		pointsFile.close();
    	}

    	if (pressureFile.fail())
    		return false;
    	else{
    		temp_stream.str("");
    		temp_stream.clear();
    		temp_stream << "CELL_DATA " << cells << "\n";
    		temp_string = temp_stream.str();
    		pressureFile.write(temp_string.c_str(),temp_string.length());

    		temp_stream.str("");
    		temp_stream.clear();
    		temp_stream << "SCALARS pressure " << precission_type << " 1\n";
    		temp_string = temp_stream.str();
    		pressureFile.write(temp_string.c_str(),temp_string.length());

    		temp_stream.str("");
    		temp_stream.clear();
    		temp_stream << "LOOKUP_TABLE default\n";
    		temp_string = temp_stream.str();
    		pressureFile.write(temp_string.c_str(),temp_string.length());

    		pressureFile.close();
    	}

    	if (velocityFile.fail())
    		return false;
    	else{
    		temp_stream.str("");
    		temp_stream.clear();
    		temp_stream << "VECTORS velocity " << precission_type << "\n";
    		temp_string = temp_stream.str();
    		velocityFile.write(temp_string.c_str(),temp_string.length());
    		velocityFile.close();
    	}

		//pointsFile.open("vtkFiles/_points.temp", std::fstream::out | std::fstream::in | std::fstream::app);
    	return true;
    }

    bool combineTempFiles(std::string fileName){
    	std::fstream pointsFile(POINTS_FILE, std::fstream::in);
    	std::fstream pressureFile(PRESSURE_FILE, std::fstream::in);
    	std::fstream velocityFile(VELOCITY_FILE, std::fstream::in);
    	std::fstream combinedFile(fileName.c_str(), std::fstream::out);
    	std::string temp_string;

    	if(pointsFile.fail() || pressureFile.fail() || velocityFile.fail())
    		return false;

    	combinedFile << "# vtk DataFile Version 2.0\nGenerated VTK file\nASCII\n\n";
    	combinedFile << pointsFile.rdbuf() << "\n";
    	combinedFile << pressureFile.rdbuf() << "\n";
    	combinedFile << velocityFile.rdbuf();

    	return true;
    }

};

#endif // _SIMULATION_H_

