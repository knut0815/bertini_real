
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <gmp.h>
#include <time.h>
#include <float.h>
#include <limits.h>
#include <mpfr.h>
#include <mpf2mpfr.h>


#ifndef NULLSPACE_H_
#define NULLSPACE_H_

extern "C" {
#include "cascade.h"
}
extern "C" {
#include "polysolve.h"
}
#include "fileops.hpp"
#include "data_type.hpp"
#include "witnessSet.hpp"
#include "solver_multilintolin.hpp"
#include "solver_nullspace_left.hpp"
#include "output.hpp"
#include "missing_bertini_headers.hpp"


#include "derivative_systems.hpp"



class double_odometer
{
	
private:
	
	
	int increment_registers(){
		
		int carry = 1; // seed carry so that it causes addition of at least the last entry of the odometer
		for (int ii=num_active_registers-1; ii>=0; ii--) { // count down from the end of the indexes
			
			if (carry==1)
				registers[ii]++;
			
			if ( registers[ii]>=(bases[active_registers[ii]]) ) {
				registers[ii] = 0;
				carry = 1;
			}
			else{
				carry = 0;
				break;
			}
		}
		return carry;  // if return 1, then need to increment the functions.
		
	};
	
	
	int increment_active_registers(){
		int carry = 1; // seed 1
		
		for (int ii=num_active_registers-1; ii>=0; ii--) {
			
			if (carry==1){
				
				active_registers[ii]++;
				for (int jj=ii+1; jj<num_active_registers; jj++) {
					active_registers[jj] = active_registers[jj-1]+1;
				}
			}
			
			if (active_registers[num_active_registers-1]>=num_total_registers) {
				carry = 1;
			}
			else{
				carry = 0;
				break; // all done!
			}
			
		}
		
		int local_counter = 0;
		for (int ii=0; ii<num_total_registers; ii++) {
			
			// a crappy find function
			int ok = 1;
			for (int jj=0; jj<num_active_registers; jj++) {
				if (active_registers[jj]==ii) 
					ok=0;
			}
			
			if ( (ok==1) && (num_total_registers != num_active_registers)){ // if didn't find current index in the list, put it in the unused list.
				inactive_registers[local_counter] = ii;
				local_counter++;
			}
		}
		
		return carry;
	};
	
public:
	
	int num_total_registers;
	int num_active_registers;
	int num_inactive_registers;
	// create and seed the function indices -- keep track of which functions we are working on
	
	std::vector< int > inactive_registers; // of length total - active
	std::vector< int > active_registers; // of length num_active_registers
	
	std::vector< int > bases; // of length num_total_registers
	std::vector< int > registers;
	
	double_odometer()
	{
		num_total_registers = num_active_registers = num_inactive_registers = 0;
	}
	
	double_odometer(int num_total_, int num_active_, int uniform_base)
	{
		num_total_registers = num_total_;
		num_active_registers = num_active_;
		num_inactive_registers = num_total_registers - num_active_registers;
		
		for (int ii=0; ii<num_active_registers; ii++) 
			active_registers.push_back(ii);
		
		for (int ii=num_active_registers; ii<num_total_registers; ii++)
			inactive_registers.push_back(ii);
		
		for (int ii=0; ii<num_active_registers; ii++) 
			registers.push_back(0);
		
		for (int ii=0; ii<num_total_registers; ii++)
			bases.push_back(uniform_base);
		
	}
	
	int reg_val(int reggie){
		return registers[reggie];
	}
		
	int act_reg(int reggie){
		return active_registers[reggie];
	}
	
	int inact_reg(int reggie){
		return inactive_registers[reggie];
	}
	
	int increment(){

		if (double_odometer::increment_registers()!=0) {
			if (double_odometer::increment_active_registers()!=0)
				return -1;
			else
				return 1;
		}
		else
			return 0;
	};
	
	void print(){
		std::cout << "active: ";
		for (int ii=0; ii<num_active_registers; ii++) 
			std::cout << active_registers[ii] << " ";
		std::cout << "\t|\t";
		
		
		std::cout << "inactive: ";
		for (int ii=0; ii<num_inactive_registers; ii++)
			std::cout << inactive_registers[ii] << " ";
		std::cout << "\t|\t";
		
		std::cout << "registers values: ";
		for (int ii=0; ii<num_active_registers; ii++)
			std::cout << registers[ii] << " ";
		std::cout << "\n";
	}
};


/**
 the main function for computing critical sets.
 
 \param W_crit_real			the main returned structure.  
 \param W								input witness_set.
 \param randomizer_matrix	randomizes the system down to the correct number of equations.
 \param pi							the set of projections to use.
 \param randomized_degrees		the degrees of the randomized functions, before differentiation.
 \param target_dim						the dimension of the object to find.
 \param program_options				holds the configuration for the main program.  is a pointer so that it is mutable.
 \param solve_options					holds the configuration for any solvers called.  is a pointer so that it is mutable.
 */
int compute_crit_nullspace(witness_set *W_crit_real, // the returned value
													 witness_set & W,
													 mat_mp randomizer_matrix,
													 vec_mp *pi,
													 int *randomized_degrees,
													 int ambient_dim,
													 int target_dim, // this should also be the number of vectors in the *pi entry
													 int target_crit_dim,
													 BR_configuration *program_options,
													 solver_configuration *solve_options,
													 nullspace_config *ns_config);



/**
 increments the odometer keeping track of which indices in the set of linears.
 
 \param function_indices					the current functions we are working on building up linears for.
 \param subindices								the current linears we will use.  this is set in this function.
 \param randomized_degrees				the degrees of the pre-differentiated functions we are building up to.
 \param	num_inner_functions				cardinal number of the subset we are considering
 \param num_functions							cardinal number of the set of functions we are indexing into.
 */
int increment_subindices(int **function_indices,
												 int **subindices,
												 int * randomized_degrees,
												 int num_inner_indices, //these *should* be implicitly available.  switch to c++ plx.
												 int num_functions);



/**
 increments the monotonically increasing array of indices which keep track of which functions we are taking linears from
 
 \param function_indices					the array we are incrementing
 \param unused_function_indices		the array of unused functions.  this is the complement of function_indices.
 \param	num_inner_functions				cardinal number of the subset we are considering
 \param num_functions							cardinal number of the set of functions we are indexing into.
 */
int increment_function_indices(int **function_indices,
												int **unused_function_indices,
												int num_inner_indices,
												int num_functions);

/**
 performs the setup for the nullspace_config which is used in the compute_crit_nullspace method, and is passed into the solverNullspace. 
 
 \param ns_config						the data structure we are setting up.
 \param target_dim					the dimension of the object we are detecting.
 \param randomized_degrees	array of integers holding the degree of each equation, *before* differentiation.
 \param randomizer_matrix		input matrix which randomizes the system down to the appropriate number of equations.
 \param W										the input witness_set
 */
void nullspace_config_setup(nullspace_config *ns_config,
														vec_mp *pi, // an array of projections, the number of which is the target dimensions
														int ambient_dim,
														int target_dim,
														int target_crit_codim,
														int *max_degree, // a pointer to the value
														int *randomized_degrees, // an array of randomized degrees
														mat_mp randomizer_matrix,
														witness_set & W,
														solver_configuration *solve_options);



void create_nullspace_system(boost::filesystem::path output_name,
														 boost::filesystem::path input_name,
														 BR_configuration * program_options,
														 nullspace_config *ns_config);


void createMatlabDerivative(boost::filesystem::path output_name,
														boost::filesystem::path input_name,
														nullspace_config *ns_config,
														int numVars, char **vars, int *lineVars, int numConstants, char **consts, int *lineConstants, int numFuncs, char **funcs, int *lineFuncs);





#endif

