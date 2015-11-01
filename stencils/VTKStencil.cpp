#include "VTKStencil.h"

/** TODO WS1: Stencil for writting VTK files
 *
 * When iterated with, creates a VTK file.
 */
 
VTKStencil::VTKStencil(const Parameters & parameters) : FieldStencil<FlowField> (parameters) {}
	
void VTKStencil::apply ( FlowField & flowField, int i, int j){}
void VTKStencil::apply ( FlowField & flowField, int i, int j, int k){}
void VTKStencil::write ( FlowField & flowField, int timeStep ){}
