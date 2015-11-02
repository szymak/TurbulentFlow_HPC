#include "VTKStencil.h"

/** TODO WS1: Stencil for writting VTK files
 *
 * When iterated with, creates a VTK file.
 */
 
VTKStencil::VTKStencil(const Parameters & parameters) : FieldStencil<FlowField> (parameters) {}
	
void VTKStencil::apply ( FlowField & flowField, int i, int j){
	std::fstream pointsFile("vtkFiles/_points.temp", std::fstream::out | std::fstream::in | std::fstream::app);
	std::fstream pressureFile("vtkFiles/_pressure.temp", std::fstream::out | std::fstream::in | std::fstream::app);
	std::fstream velocityFile("vtkFiles/_velocity.temp", std::fstream::out | std::fstream::in | std::fstream::app);
	FLOAT temp_pressure;
	FLOAT temp_velocity[3];

	if(pointsFile.fail() || pressureFile.fail() || velocityFile.fail())
		return;

	pointsFile << (double)(i-1)*_parameters.geometry.lengthX/_parameters.geometry.sizeX << " " <<
			      (double)(j-1)*_parameters.geometry.lengthY/_parameters.geometry.sizeY << "0.0\n";

	if (i == _parameters.geometry.sizeX || j == _parameters.geometry.sizeY){

	}
	else{
		flowField.getPressureAndVelocity(temp_pressure, temp_velocity, i, j);
		pressureFile << temp_pressure << "\n";
		velocityFile << temp_velocity[0] << " " << temp_velocity[1] << " 0\n";
	}

	pointsFile.close();
	pressureFile.close();
	velocityFile.close();
}
void VTKStencil::apply ( FlowField & flowField, int i, int j, int k){
	std::fstream pointsFile("vtkFiles/_points.temp", std::fstream::out | std::fstream::in | std::fstream::app);
	std::fstream pressureFile("vtkFiles/_pressure.temp", std::fstream::out | std::fstream::in | std::fstream::app);
	std::fstream velocityFile("vtkFiles/_velocity.temp", std::fstream::out | std::fstream::in | std::fstream::app);
	FLOAT temp_pressure;
	FLOAT temp_velocity[3];

	if(pointsFile.fail() || pressureFile.fail() || velocityFile.fail())
		return;

	pointsFile << (double)(i-1)*_parameters.geometry.lengthX/_parameters.geometry.sizeX << " " <<
				  (double)(j-1)*_parameters.geometry.lengthY/_parameters.geometry.sizeY << " " <<
				  (double)(k-1)*_parameters.geometry.lengthZ/_parameters.geometry.sizeZ << "\n";

	if (i == _parameters.geometry.sizeX || j == _parameters.geometry.sizeY || k == _parameters.geometry.sizeZ){

	}
	else{
		flowField.getPressureAndVelocity(temp_pressure, temp_velocity, i, j, k);
		pressureFile << temp_pressure << "\n";
		velocityFile << temp_velocity[0] << " " << temp_velocity[1] << " " << temp_velocity[2] << "\n";
	}

	pointsFile.close();
	pressureFile.close();
	velocityFile.close();
}
void VTKStencil::write ( FlowField & flowField, int timeStep ){}
