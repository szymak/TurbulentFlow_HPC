#include "VTKStencil.h"

/** TODO WS1: Stencil for writting VTK files
 *
 * When iterated with, creates a VTK file.
 */
 
VTKStencil::VTKStencil(const Parameters & parameters) : FieldStencil<FlowField> (parameters) {}
	
void VTKStencil::apply ( FlowField & flowField, int i, int j){
	std::fstream pointsFile(POINTS_FILE, std::fstream::out | std::fstream::in | std::fstream::app);
	std::fstream pressureFile(PRESSURE_FILE, std::fstream::out | std::fstream::in | std::fstream::app);
	std::fstream velocityFile(VELOCITY_FILE, std::fstream::out | std::fstream::in | std::fstream::app);

	if(pointsFile.fail() || pressureFile.fail() || velocityFile.fail())
		return;

	pointsFile << (double)(i-1)*_parameters.geometry.lengthX/_parameters.geometry.sizeX << " " <<
			      (double)(j-1)*_parameters.geometry.lengthY/_parameters.geometry.sizeY << "0.0\n";

	if (i == _parameters.geometry.sizeX || j == _parameters.geometry.sizeY){
		// out of bounds
	}
	else{
		flowField.getPressureAndVelocity(pressureVTK, velocityVTK, i, j);
		pressureFile << pressureVTK << "\n";
		velocityFile << velocityVTK[0] << " " << velocityVTK[1] << " 0\n";
	}

	pointsFile.close();
	pressureFile.close();
	velocityFile.close();
}

void VTKStencil::apply ( FlowField & flowField, int i, int j, int k){
	std::fstream pointsFile(POINTS_FILE, std::fstream::out | std::fstream::in | std::fstream::app);
	std::fstream pressureFile(PRESSURE_FILE, std::fstream::out | std::fstream::in | std::fstream::app);
	std::fstream velocityFile(VELOCITY_FILE, std::fstream::out | std::fstream::in | std::fstream::app);

	if(pointsFile.fail() || pressureFile.fail() || velocityFile.fail())
		return;

	pointsFile << (double)(i-1)*_parameters.geometry.lengthX/_parameters.geometry.sizeX << " " <<
				  (double)(j-1)*_parameters.geometry.lengthY/_parameters.geometry.sizeY << " " <<
				  (double)(k-1)*_parameters.geometry.lengthZ/_parameters.geometry.sizeZ << "\n";

	if (i == _parameters.geometry.sizeX || j == _parameters.geometry.sizeY || k == _parameters.geometry.sizeZ){
		// out of bounds
	}
	else{
		flowField.getPressureAndVelocity(pressureVTK, velocityVTK, i, j, k);
		pressureFile << pressureVTK << "\n";
		velocityFile << velocityVTK[0] << " " << velocityVTK[1] << " " << velocityVTK[2] << "\n";
	}

	pointsFile.close();
	pressureFile.close();
	velocityFile.close();
}
void VTKStencil::write ( FlowField & flowField, int timeStep ){}
