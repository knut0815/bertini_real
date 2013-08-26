#include "curve.hpp"





int curve_decomposition::setup_edges(boost::filesystem::path INfile)
//setup the vertex structure
{
	FILE *IN = safe_fopen_read(INfile);
	
	fscanf(IN, "%d\n", &this->num_edges);
	int left, midpt, right;
	for(int ii=0;ii<this->num_edges;ii++) {
		fscanf(IN,"%d %d %d",&left, &midpt, &right); scanRestOfLine(IN);
		this->edges.push_back(edge(left, midpt, right));
	}
	
	fclose(IN);
	return this->num_edges;
}





void curve_decomposition::add_edge(edge new_edge)
{
	this->num_edges++;
	this->edges.push_back(new_edge);
	return;
}


void curve_decomposition::print(boost::filesystem::path base)
{
	
	std::cout << "printing curve decomposition to folder " << base << std::endl;
	
	decomposition::print(base);
	
	boost::filesystem::path edgefile = base / "E.edge";
	
	curve_decomposition::print_edges(edgefile);
}




void curve_decomposition::print_edges(boost::filesystem::path outputfile)
/**Output edge structure as follows:
 # variables
 # edges
 name of input file
 edge 1
 
 edge 2
 
 
 for each edge, output the following information:
 index to left vertex in vertices
 index to right vertex in vertices
 index to midpoint vertex in vertices
 
 **/
{
	int ii;
	FILE *OUT = safe_fopen_write(outputfile);
	
	// output the number of vertices
	fprintf(OUT,"%d\n\n",num_edges);
	
	for(ii=0;ii<num_edges;ii++)
		fprintf(OUT,"%d %d %d \n",
						edges[ii].left,
						edges[ii].midpt,
						edges[ii].right);
	fclose(OUT);
}















void curve_decomposition::main(vertex_set & V,
								witness_set & W,
								vec_mp *pi,
								BR_configuration & program_options,
								solver_configuration & solve_options)
{
	
	int num_vars = W.num_variables;

	
	


	// perform an isosingular deflation
	// note: you do not need witness_data to perform isosingular deflation
	if (program_options.verbose_level>=2)
		printf("performing isosingular deflation\n");
	
	W.write_dehomogenized_coordinates("witness_points_dehomogenized"); // write the points to file
	int num_deflations, *deflation_sequence = NULL;
	isosingular_deflation(&num_deflations, &deflation_sequence,
												program_options, program_options.current_working_filename,
												"witness_points_dehomogenized",
												program_options.max_deflations,
												W.dim, W.comp_num);
	free(deflation_sequence);
	
	
	
	program_options.input_deflated_filename = W.input_filename;
	
	std::stringstream converter;
	converter << "_dim_" << W.dim << "_comp_" << W.comp_num << "_deflated";
	program_options.input_deflated_filename += converter.str();
	converter.clear(); converter.str("");
	
	W.input_filename = program_options.input_deflated_filename;

	// this wraps around a bertini routine
	parse_input_file(W.input_filename);

	preproc_data_clear(&solve_options.PPD);
	parse_preproc_data("preproc_data", &solve_options.PPD);
	
	
	
	int self_conjugate = 1;
	if (W.num_synth_vars==0) {

		if (program_options.verbose_level>=2) {
			printf("checking if component is self-conjugate\n");
		}
		self_conjugate = checkSelfConjugate(W, num_vars, program_options, W.input_filename);  //later:  could be passed in from user, if we want
		
		//regenerate the various files, since we ran bertini since then.
		parse_input_file(W.input_filename);

		if (verify_projection_ok(W,
														 *pi,
														 solve_options)==1){
			if (program_options.verbose_level>=1) {
				printf("verified projection is ok\n");
			}
		}
		else{
			printf("the projection is invalid, in that the jacobian of the randomized system\nbecomes singular at a random point, when the projection is concatenated\n");
			
			print_point_to_screen_matlab(pi[0], "pi[0]");
			
			exit(196);
		}
		
	}
	
	

	
	input_filename = W.input_filename;
	component_num = W.comp_num;
	dimension = W.dim;
	num_variables = num_vars;
	add_projection(pi[0]);
	
	if (self_conjugate==0)  //C is not self-conjugate
	{
		//Call non-self-conjugate case code

		computeCurveNotSelfConj(W, pi[0], V, num_vars,
														program_options, solve_options); 

	}
	else
	{
		//Call self-conjugate case code

		computeCurveSelfConj(W,
												 pi,
												 V,
												 num_vars,
												 program_options, solve_options); 
	}
	
}











void curve_decomposition::computeCurveSelfConj(witness_set & W_curve,
																							 vec_mp *pi,
																							 vertex_set &V,
																							 int num_vars,
																							 BR_configuration & program_options,
																							 solver_configuration & solve_options)
{
	//IN DEVELOPMENT
  
	
	
	int ambient_dim = 1;
	
	witness_set Wtemp;
	
	
	
	
	
	// 2) randomize down to N-1 equations
	// to get a square system for the homotopies in the following steps.
	
	
	//create the matrix
	init_mat_mp2(randomizer_matrix,
							 W_curve.num_variables-W_curve.num_patches-ambient_dim,solve_options.PPD.num_funcs,
							 solve_options.T.AMP_max_prec);
	
	//create the array of integers
	int *randomized_degrees = (int *)br_malloc((W_curve.num_variables-W_curve.num_patches-ambient_dim)*sizeof(int));
	
	//get the matrix and the degrees of the resulting randomized functions.
	make_randomization_matrix_based_on_degrees(randomizer_matrix, &randomized_degrees, W_curve.num_variables-W_curve.num_patches-ambient_dim, solve_options.PPD.num_funcs);
	
	if (program_options.verbose_level>=4)
		print_matrix_to_screen_matlab(randomizer_matrix,"randomization");
	
	FILE *OUT = safe_fopen_write("Rand_matrix");
	fprintf(OUT,"%d %d\n",randomizer_matrix->rows,randomizer_matrix->cols);
	fprintf(OUT,"%d\n",solve_options.T.AMP_max_prec);
	fprintf(OUT,"\n");
	print_matrix_to_file_mp(OUT, 0, randomizer_matrix);
	fclose(OUT);
	
	
	
  // 4) solve for critical conditions for random complex projection
	witness_set W_crit_real;
	
	
	curve_compute_critical_points(W_curve,
																randomizer_matrix,
																randomized_degrees,
																pi,
																program_options,
																solve_options,
																W_crit_real);
	
	
	interslice(W_curve,
						 W_crit_real,
						 randomizer_matrix,
						 pi,
						 program_options,
						 solve_options,
						 V);
	
	return;
} // re: computeCurveSelfConj


//subfunctions
int curve_compute_critical_points(witness_set & W_curve,
																	mat_mp randomizer_matrix,
																	int *randomized_degrees,
																	vec_mp *pi,
																	BR_configuration & program_options,
																	solver_configuration solve_options,
																	witness_set & W_crit_real)
{
	int ambient_dim = 1;
	
	int *declarations = NULL;
	
	partition_parse(&declarations, program_options.current_working_filename, "func_input", "config", 0); // the 0 means not self conjugate.
																																																			 // i would like to move this.
	
	
	W_crit_real.cp_names(W_curve);
	W_crit_real.cp_patches(W_curve);
	
	if (program_options.crit_solver == LINPRODTODETJAC)
	{
		
		
		//compute the number of linears we will need to move to, to perform the regeneration for linprodtodetjac method.
		int num_new_linears = 0;
		for (int ii=0; ii<W_curve.num_variables-W_curve.num_patches-ambient_dim; ii++) {
			num_new_linears+= randomized_degrees[ii]-1; // subtract 1 for differentiation
		}
		num_new_linears -= 1; // subtract 1 because started with a linear (in the witness_set)
		
		if (program_options.verbose_level>=1) {
			printf("the number of *new* linears will be %d\n",num_new_linears);
		}
		
		
		compute_crit_linprodtodetjac(&W_crit_real, // the returned value
																 W_curve,            // input the original witness set
																 randomizer_matrix,
																 *pi,
																 num_new_linears,
																 program_options,
																 solve_options);
	}
	else	{
		
		nullspace_config ns_config;
		compute_crit_nullspace(&W_crit_real, // the returned value
													 W_curve,            // input the original witness set
													 randomizer_matrix,
													 pi,
													 randomized_degrees,
													 ambient_dim,  // dimension of ambient complex object
													 ambient_dim,   //  target dimension to find
													 ambient_dim,   // COdimension of the critical set to find.
													 program_options,
													 solve_options,
													 &ns_config);
		ns_config.clear();
		
		
		W_crit_real.cp_patches(W_curve);
		W_crit_real.cp_linears(W_curve);
		W_crit_real.only_first_vars(W_curve.num_variables); // trim the fat, since we are at the lowest level.
		
//		std::cout << "W_crit_real:" << std::endl;
//		W_crit_real.print_to_screen();
		W_crit_real.sort_for_real(solve_options.T);
		// now get the bounding box critical points and ends of the interval
		curve_get_additional_critpts(&W_crit_real,
																 W_curve,
																 randomizer_matrix,
																 pi[0],
																 randomized_degrees,
																 program_options,
																 solve_options);
		
	}
	
	
	return SUCCESSFUL;
}

int curve_decomposition::interslice(witness_set & W_curve,
							 witness_set & W_crit_real,
							 mat_mp randomizer_matrix,
							 vec_mp *pi,
							 BR_configuration & program_options,
							 solver_configuration solve_options,
							 vertex_set & V)
{
	
	for (int ii=0; ii<W_curve.num_patches; ii++) 
		add_patch(W_curve.patch_mp[ii]);
	
	this->num_variables = W_crit_real.num_variables;
	input_filename = W_curve.input_filename;
	
	int blabla; int *declarations = NULL;
	
	parse_input_file(W_curve.input_filename, &blabla);
	partition_parse(&declarations, W_curve.input_filename, "func_input", "config", 0); // the 0 means not self conjugate.
																																										 // i would like to move this.
	
	mat_cp_mp(this->randomizer_matrix, randomizer_matrix);
	

	///////
	//
	//   actually form crit.
	//
	/////////
	
	
	
	vec_mp crit_downstairs; init_vec_mp(crit_downstairs,0);
	vec_mp midpoints_downstairs; init_vec_mp(midpoints_downstairs,0);
	std::vector< int > index_tracker;
	
	W_crit_real.compute_downstairs_crit_midpts(crit_downstairs, midpoints_downstairs, index_tracker, pi[0]);
	
	int num_midpoints = midpoints_downstairs->size;
	
	if (program_options.verbose_level>=-1) {
		print_point_to_screen_matlab(crit_downstairs,"crit_downstairs");
		print_point_to_screen_matlab(midpoints_downstairs,"midpoints_downstairs");
	}
	
	
	
	vertex temp_vertex;
	
	if (crit_downstairs->size==1) {
		
		set_mp(temp_vertex.projVal_mp,  &crit_downstairs->coord[0]); // set projection value
		vec_cp_mp(temp_vertex.pt_mp, W_crit_real.pts_mp[0]);// set point
		
		temp_vertex.type = CRITICAL; // set type
		
		int I = index_in_vertices_with_add(V, temp_vertex,  solve_options.T);
		
		
		edge E(I,I,I);
		
		add_edge(E);
	}
	//copy the points from witness sets into curve decomposition V0;
	
	

	
	for (int ii=0; ii<crit_downstairs->size; ii++){
		
		if (program_options.verbose_level>=2)
			printf("adding point %d of %d from crit_real to vertices\n",ii,crit_downstairs->size);
		
		set_mp(temp_vertex.projVal_mp,  &crit_downstairs->coord[ii]); // set projection value
		vec_cp_mp(temp_vertex.pt_mp, W_crit_real.pts_mp[index_tracker[ii]]);// set point
		temp_vertex.type = CRITICAL; // set type
		
		index_in_vertices_with_add(V, temp_vertex,  solve_options.T);
	}
	
	
	
	
	
	solve_options.allow_multiplicity=1;
	solve_options.allow_singular=1;
	solve_options.use_midpoint_checker = 0;
	
	
	int edge_counter = 0; // set the counter
	
	
	std::vector< witness_set> midpoint_witness_sets;
	midpoint_witness_sets.resize(num_midpoints);
	
	
  vec_mp particular_projection;  init_vec_mp(particular_projection,W_curve.num_variables); particular_projection->size = W_curve.num_variables;
	vec_cp_mp(particular_projection,pi[0]);
	
	solve_options.backup_tracker_config();
	
	
	
	solve_options.allow_multiplicity = 0;
	solve_options.allow_singular = 0;
	solve_options.complete_witness_set = 1;
	
	for (int ii=0; ii<num_midpoints; ii++) {
		
		if (program_options.verbose_level>=2) {
			printf("solving midpoints upstairs %d, projection value %lf\n",ii,mpf_get_d(midpoints_downstairs->coord[ii].r));
		}
		
		neg_mp(&particular_projection->coord[0], &midpoints_downstairs->coord[ii]);
		
		//make the projection we will solve at.
		
		
		
		
		
		lintolin_solver_main(solve_options.T.MPType,
												 W_curve,
												 randomizer_matrix,
												 &particular_projection, 1,
												 &midpoint_witness_sets[ii],
												 solve_options); //sets this value
		
		
		if (program_options.verbose_level>=2) {
			printf("sorting midpoint witness set %d for realness\n",ii);
		}
		midpoint_witness_sets[ii].sort_for_real(solve_options.T);
		midpoint_witness_sets[ii].sort_for_unique(solve_options.T);
		
		edge_counter += midpoint_witness_sets[ii].num_pts;
	}
	
	solve_options.reset_tracker_config();
	
	
  // 7) find edge endpoints
  
	solve_options.allow_multiplicity = 1;
	solve_options.allow_singular = 1;
	
	
	witness_set Wleft, Wright;
	
	edge temp_edge;
	comp_mp left_proj_val; init_mp(left_proj_val);
	comp_mp right_proj_val; init_mp(right_proj_val);
	
	for (int ii=0; ii<num_midpoints; ++ii) {
		if (program_options.verbose_level>=1) {
			printf("moving from downstairs midpoint %d, to left and right\n",ii);
		}
		
		
		neg_mp(&particular_projection->coord[0], &crit_downstairs->coord[ii]);
		print_comp_matlab(&crit_downstairs->coord[ii],"left");
		lintolin_solver_main(solve_options.T.MPType,
												 midpoint_witness_sets[ii], //the input
												 randomizer_matrix,
												 &particular_projection, 1,
												 &Wleft,
												 solve_options); // the output
		
		
		neg_mp(&particular_projection->coord[0], &crit_downstairs->coord[ii+1]);
		
		
		print_comp_matlab(&crit_downstairs->coord[ii+1],"right");
		lintolin_solver_main(solve_options.T.MPType,
												 midpoint_witness_sets[ii], //the input
												 randomizer_matrix,
												 &particular_projection, 1,
												 &Wright,
												 solve_options); // the output
																				 //each member of Wtemp should real.  if a member of V1 already, mark index.  else, add to V1, and mark.
		
		
		int keep_going = 1;
		if (Wright.num_pts!=midpoint_witness_sets[ii].num_pts) {
			printf("had a critical failure\n moving right was deficient a point\n");
			keep_going = 0;
		}
		if (Wleft.num_pts!=midpoint_witness_sets[ii].num_pts) {
			printf("had a critical failure\n moving left was deficient a point\n");
			keep_going = 0;
		}
		if (!keep_going) {
			exit(3999);
		}
		
		
		for (int kk=0; kk<midpoint_witness_sets[ii].num_pts; kk++) {
			
			set_mp(temp_vertex.projVal_mp, &midpoints_downstairs->coord[ii]  ); // set projection value
			vec_cp_mp(temp_vertex.pt_mp,midpoint_witness_sets[ii].pts_mp[kk]);// set point
			
			
			
			temp_vertex.type = MIDPOINT; // set type
			
			temp_edge.midpt = index_in_vertices_with_add(V,temp_vertex,solve_options.T); // gets the index of the new midpoint as it is added
			temp_edge.left  = index_in_vertices_with_add(V, Wleft.pts_mp[kk],  &crit_downstairs->coord[ii],  solve_options.T);
			temp_edge.right = index_in_vertices_with_add(V, Wright.pts_mp[kk], &crit_downstairs->coord[ii+1], solve_options.T);
			
			add_edge(temp_edge);
			
			
			
			
			
			if (program_options.verbose_level>=2) {
				printf("upstairs midpoint %d\n",kk);
				print_comp_matlab(V.vertices[temp_edge.left].projVal_mp,"left");
				print_comp_matlab(V.vertices[temp_edge.midpt].projVal_mp,"mid");
				print_comp_matlab(V.vertices[temp_edge.right].projVal_mp,"right");
				printf("indices of left, mid, right: %d %d %d\n",temp_edge.left,temp_edge.midpt,temp_edge.right);
				printf("\n\n");
			}
		}
		Wleft.reset();
		Wright.reset();
		
	}//re: for ii
	clear_mp(left_proj_val); clear_mp(right_proj_val);
	
	if (program_options.verbose_level>=0) {
		printf("num_edges = %d\n",num_edges);
	}
	
	

	
	
	return SUCCESSFUL;
}


int curve_get_additional_critpts(witness_set *W_crit_real,
																 witness_set & W,
																 mat_mp randomizer_matrix,
																 vec_mp pi,
																 int *randomized_degrees,
																 BR_configuration & program_options,
																 solver_configuration & solve_options)
{
	
	
	solve_options.complete_witness_set=1;
	
	int ii, jj;
	
	witness_set Wtemp;
	
	/////now compute the bounds, whether bounding box or bounding the interval
	
	comp_mp one; init_mp(one);
	set_one_mp(one);
	
	vec_mp variable_projection; init_vec_mp(variable_projection, W.num_variables); variable_projection->size = W.num_variables;
	
	if (!program_options.use_bounding_box) {// do not want bounding box
		
		if (program_options.verbose_level>=0)
			printf("in no-box mode.  computing bounds of interval\n");
		
		
		vec_cp_mp(variable_projection,pi);
		
		// compute projections of W_crit_real
		vec_mp projection_values; init_vec_mp(projection_values,W_crit_real->num_pts); projection_values->size=W_crit_real->num_pts;
		for (jj=0; jj<W_crit_real->num_pts; jj++) {
			projection_value_homogeneous_input(&projection_values->coord[jj], W_crit_real->pts_mp[jj],pi);
		}
		
		vec_mp projections_sorted;
		init_vec_mp(projections_sorted,W_crit_real->num_pts); projections_sorted->size = W_crit_real->num_pts;
		std::vector< int > index_tracker;
		sort_increasing_by_real(projections_sorted, index_tracker, projection_values);
		
		//		printf("%d points in W_crit_real\n", W_crit_real->num_pts);
		//		print_point_to_screen_matlab(projections_sorted,"proj_sorted");
		
		
		comp_mp left_proj_val, right_proj_val;
		init_mp(left_proj_val); init_mp(right_proj_val);
		
		if (W_crit_real->num_pts==0) {
			set_one_mp(right_proj_val);
			set_one_mp(left_proj_val);
			neg_mp(left_proj_val,left_proj_val);
		}
		else{
			set_mp(left_proj_val, &projections_sorted->coord[0]);
			set_mp(right_proj_val, &projections_sorted->coord[W_crit_real->num_pts-1]);
		}
		
		
		comp_mp relevant_proj_val; init_mp(relevant_proj_val);
		//this loop presupposes that w_crit_real has at least one member
		for (jj=0; jj<2; jj++) {
			
			
			//set the value for the projection to move to
			if (jj==0){ // if on the first of the two iterations
				set_mp(relevant_proj_val,left_proj_val);}
			else{
				set_mp(relevant_proj_val,right_proj_val);}
			
			set_mp(&variable_projection->coord[0],relevant_proj_val);
			
			if (jj==0){
				sub_mp(&variable_projection->coord[0],&variable_projection->coord[0],one);}
			else{
				add_mp(&variable_projection->coord[0],&variable_projection->coord[0],one);}
			
			neg_mp(&variable_projection->coord[0],&variable_projection->coord[0]);
			//done setting the value of the projection.
			
			//run the solver
			lintolin_solver_main(solve_options.T.MPType,
													 W,
													 randomizer_matrix,
													 &variable_projection, 1,
													 &Wtemp,
													 solve_options);
			
			Wtemp.sort_for_real(solve_options.T);
			Wtemp.sort_for_unique(solve_options.T);
			W_crit_real->merge(Wtemp);
			
			Wtemp.reset();
		}
		
		clear_mp(relevant_proj_val);
	}
	else //want a bounding box
	{
		
		if (program_options.verbose_level>=0) {
			printf("computing bounding box\n");
		}
		// for each variable direction, compute projections of crit points onto that axis.
		
		
		for (ii=0; ii<W.num_variables - W.num_synth_vars - 1; ++ii)
		{
			if (program_options.verbose_level>=1) {
				printf("projecting onto variable %d\n",ii);
			}
			//make the projection
			for (jj=0; jj<W.num_variables; jj++) {
				set_zero_mp(&variable_projection->coord[jj]);
			}
			set_one_mp(&variable_projection->coord[ii+1]);
			
			
			while (verify_projection_ok(W,
																	randomizer_matrix,
																	variable_projection,
																	solve_options)!=1){
				// the projection onto variable ii is bad.
				if (ii==0) {
					get_comp_rand_real_mp(&variable_projection->coord[2]);
				}
				else{
					get_comp_rand_real_mp(&variable_projection->coord[ii]);
				}
			}
			
			
			nullspace_config ns_config;
			compute_crit_nullspace(&Wtemp, // the returned value
														 W,            // input the original witness set
														 randomizer_matrix,
														 &variable_projection,
														 randomized_degrees,
														 1,  // dimension of ambient complex object
														 1,   //  target dimension to find
														 1,   // COdimension of the critical set to find.
														 program_options,
														 solve_options,
														 &ns_config);
			ns_config.clear();
			
			Wtemp.sort_for_real(solve_options.T);
			
			if (Wtemp.num_pts>0)
			{
				//compute the projections, go outside a bit, find points.
				vec_mp projection_values; init_vec_mp(projection_values,Wtemp.num_pts); projection_values->size=Wtemp.num_pts;
				for (jj=0; jj<Wtemp.num_pts; jj++) {
					projection_value_homogeneous_input(&projection_values->coord[jj], Wtemp.pts_mp[jj],variable_projection);
				}
				
				
				vec_mp projections_sorted;
				init_vec_mp(projections_sorted,Wtemp.num_pts); projections_sorted->size = Wtemp.num_pts;
				std::vector< int > index_tracker;
				sort_increasing_by_real(projections_sorted, index_tracker, projection_values);
				
				Wtemp.reset();
				
				
				set_mp(&variable_projection->coord[0],&projections_sorted->coord[0]); // the first one
				sub_mp(&variable_projection->coord[0],&variable_projection->coord[0],one);
				neg_mp(&variable_projection->coord[0],&variable_projection->coord[0]);
				
				lintolin_solver_main(solve_options.T.MPType,
														 W,
														 randomizer_matrix,
														 &variable_projection, 1,
														 &Wtemp,
														 solve_options);
				
				Wtemp.sort_for_real(solve_options.T);
				Wtemp.sort_for_unique(solve_options.T);
				W_crit_real->merge(Wtemp);
				
				Wtemp.reset();
				
				
				set_mp(&variable_projection->coord[0],&projections_sorted->coord[projections_sorted->size-1]); // the last one
				add_mp(&variable_projection->coord[0],&variable_projection->coord[0],one);
				neg_mp(&variable_projection->coord[0],&variable_projection->coord[0]);
				
				lintolin_solver_main(solve_options.T.MPType,
														 W,
														 randomizer_matrix,
														 &variable_projection, 1,
														 &Wtemp,
														 solve_options);
				
				Wtemp.sort_for_real(solve_options.T);
				Wtemp.sort_for_unique(solve_options.T);
				W_crit_real->merge(Wtemp);
				
				Wtemp.reset();
				
				clear_vec_mp(projection_values);
				clear_vec_mp(projections_sorted);
				index_tracker.clear();
			}
			
			Wtemp.reset();
		}
		
		
		if (program_options.verbose_level>=0)
			printf("completed detjac2detjac solve, have %d real solutions to work with\n\n", W_crit_real->num_pts);
		
		
		W_crit_real->sort_for_unique(solve_options.T);
		
		// if have box, intersect box with C
	} // re: bounding box calc
	
	clear_vec_mp(variable_projection);
	clear_mp(one);
	
	
	
	
	
	return 0;
}















//the linprodtodetjac method for getting the critical points
int compute_crit_linprodtodetjac(witness_set *W_crit_real, // the returned value
																 witness_set & W,
																 mat_mp randomizer_matrix,
																 vec_mp pi,
																 int num_new_linears,
																 BR_configuration & program_options,
																 solver_configuration & solve_options)
{
	
	int ii,jj,kk;
	
	int success = 1;
	
	
	//get the random complex projection, which we will use to find the crit pts
	vec_mp random_complex_projection; init_vec_mp2(random_complex_projection,W.num_variables,solve_options.T.AMP_max_prec); random_complex_projection->size = W.num_variables;
	
	set_zero_mp(&random_complex_projection->coord[0]); // first coordinate is 0
	for (ii=1; ii<W.num_variables; ii++) {
		get_comp_rand_mp(&random_complex_projection->coord[ii]); // all other coordinates random complex
	}
	
	
	//////////////
	//
	//  first of three steps for obtaining the critical points.
	//
	/////////////
	
	
	vec_mp *new_linears_full_prec = (vec_mp *) malloc(num_new_linears*sizeof(vec_mp));
	
	for (ii=0; ii<num_new_linears; ii++) {
		init_vec_mp2(new_linears_full_prec[ii],W.num_variables,solve_options.T.AMP_max_prec); new_linears_full_prec[ii]->size = W.num_variables;
		for (kk=0; kk<W.num_variables; kk++) {
			get_comp_rand_mp(&new_linears_full_prec[ii]->coord[kk]);
		}
	}
	
	
	witness_set Wtemp;
	witness_set W_lintolin = W;
	
	//set some solver options.  these will have to be reset later;
	solve_options.allow_multiplicity = 1;
	solve_options.allow_singular = 1;
	solve_options.use_midpoint_checker = 0;
	
	
	
	lintolin_solver_main(solve_options.T.MPType,
											 W,
											 randomizer_matrix,
											 new_linears_full_prec, num_new_linears,
											 &Wtemp,
											 solve_options);
	
	
	
	
	//add the W to Wnew.
	W_lintolin.merge(Wtemp);
	Wtemp.reset();
	
	// these three write calls optional.
	W_lintolin.write_homogeneous_coordinates("lintolin_points_homogeneous");
	W_lintolin.write_dehomogenized_coordinates("lintolin_points");
	W_lintolin.write_linears("lintolin_linears");
	
	if (program_options.verbose_level>=1)
		printf("done with initial use of lintolin solver\n");
	
	
	
	
	
	//////////////
	//
	//  second of three steps for obtaining the critical points.
	//
	/////////////
	
	
	
	solve_options.allow_multiplicity = 1;
	solve_options.allow_singular = 1;
	
	witness_set W_linprod;
	
	double initial_bound_on_degree = solve_options.T.AMP_bound_on_degree;
	solve_options.T.AMP_bound_on_degree = (double) num_new_linears+1;
	linprod_to_detjac_solver_main(solve_options.T.MPType,
																W_lintolin,
																randomizer_matrix,
																random_complex_projection,
																&W_linprod,
																solve_options);
	solve_options.T.AMP_bound_on_degree = initial_bound_on_degree;
	
	
	//
	//	print_witness_set_to_screen(W_linprod);
	W_linprod.write_dehomogenized_coordinates("member_points");
	W_linprod.write_dehomogenized_coordinates("linprod_solns");
	
	
	
	//	W_linprod should have only the points from W_in which lie on the correct component.
	W_linprod.incidence_number = W.incidence_number; // copy over from the input.  note that the incidence number may be different from the component number
	
	W_linprod.sort_for_unique(solve_options.T);
	W_linprod.write_dehomogenized_coordinates("linprod_solns_postmembership");
	
	
	
	if (program_options.verbose_level>=1)
		printf("done with linprod_to_detjac\n");
	
	
	
	//////////////
	//
	//  third of three steps for obtaining the critical points.
	//
	/////////////
	witness_set W_detjacdetjac;
	W_detjacdetjac.cp_names(W);
	
	solve_options.allow_multiplicity = 1;
	solve_options.allow_singular = 1;
	
	initial_bound_on_degree = solve_options.T.AMP_bound_on_degree;
	solve_options.T.AMP_bound_on_degree = (double) num_new_linears+1;
	
	detjac_to_detjac_solver_main(solve_options.T.MPType,
															 W_linprod,
															 randomizer_matrix,
															 random_complex_projection, // move from
															 pi,  // move to
															 W_crit_real,
															 solve_options);
	
	solve_options.T.AMP_bound_on_degree = initial_bound_on_degree;
	
	W_crit_real->write_dehomogenized_coordinates("detjac_solns");
	
	W_crit_real->sort_for_real(solve_options.T); // get only the real solutions.
	
	
	
	
	
	// NOW WE HAVE THE CRITICAL POINTS
	
	
	
	/////now compute the bounds, whether bounding box or bounding the interval
	
	comp_mp one; init_mp(one);
	set_one_mp(one);
	
	vec_mp variable_projection; init_vec_mp(variable_projection, W.num_variables); variable_projection->size = W.num_variables;
	
	if (!program_options.use_bounding_box) {// do not want bounding box
		
		if (program_options.verbose_level>=0)
			printf("in no-box mode.  computing bounds of interval\n");
		
		
		vec_cp_mp(variable_projection,pi);
		
		// compute projections of W_crit_real
		vec_mp projection_values; init_vec_mp(projection_values,W_crit_real->num_pts); projection_values->size=W_crit_real->num_pts;
		for (jj=0; jj<W_crit_real->num_pts; jj++) {
			projection_value_homogeneous_input(&projection_values->coord[jj], W_crit_real->pts_mp[jj],pi);
		}
		
		vec_mp projections_sorted;
		init_vec_mp(projections_sorted,W_crit_real->num_pts); projections_sorted->size = W_crit_real->num_pts;
		std::vector< int > index_tracker;
		sort_increasing_by_real(projections_sorted, index_tracker, projection_values);
		
		printf("%d points in W_crit_real\n", W_crit_real->num_pts);
		print_point_to_screen_matlab(projections_sorted,"proj_sorted");
		
		
		comp_mp left_proj_val, right_proj_val;
		init_mp(left_proj_val); init_mp(right_proj_val);
		
		if (W_crit_real->num_pts==0) {
			set_one_mp(right_proj_val);
			set_one_mp(left_proj_val);
			neg_mp(left_proj_val,left_proj_val);
		}
		else{
			set_mp(left_proj_val, &projections_sorted->coord[0]);
			set_mp(right_proj_val, &projections_sorted->coord[W_crit_real->num_pts-1]);
		}
		
		
		comp_mp relevant_proj_val; init_mp(relevant_proj_val);
		//this loop presupposes that w_crit_real has at least one member
		for (jj=0; jj<2; jj++) {
			
			
			//set the value for the projection to move to
			if (jj==0){ // if on the first of the two iterations
				set_mp(relevant_proj_val,left_proj_val);}
			else{
				set_mp(relevant_proj_val,right_proj_val);}
			
			set_mp(&variable_projection->coord[0],relevant_proj_val);
			
			if (jj==0){
				sub_mp(&variable_projection->coord[0],&variable_projection->coord[0],one);}
			else{
				add_mp(&variable_projection->coord[0],&variable_projection->coord[0],one);}
			
			neg_mp(&variable_projection->coord[0],&variable_projection->coord[0]);
			//done setting the value of the projection.
			
			//run the solver
			lintolin_solver_main(solve_options.T.MPType,
													 W,
													 randomizer_matrix,
													 &variable_projection, 1,
													 &Wtemp,
													 solve_options);
			
			Wtemp.sort_for_real(solve_options.T);
			Wtemp.sort_for_unique(solve_options.T);
			W_crit_real->merge(Wtemp);
			
			Wtemp.reset();
		}
		
		clear_mp(relevant_proj_val);
	}
	else //want a bounding box
	{
		
		if (program_options.verbose_level>=0) {
			printf("computing bounding box\n");
		}
		// for each variable direction, compute projections of crit points onto that axis.
		
		
		for (ii=0; ii<W.num_variables - W.num_synth_vars -1; ++ii)
		{
			if (program_options.verbose_level>=1) {
				printf("projecting onto variable %d\n",ii);
			}
			//make the projection
			for (jj=0; jj<W.num_variables; jj++) {
				set_zero_mp(&variable_projection->coord[jj]);
			}
			set_one_mp(&variable_projection->coord[ii+1]);
			
			
			while (verify_projection_ok(W,
																	randomizer_matrix,
																	variable_projection,
																	solve_options)!=1){
				// the projection onto variable ii is bad.
				if (ii==0) {
					get_comp_rand_real_mp(&variable_projection->coord[2]);
				}
				else{
					get_comp_rand_real_mp(&variable_projection->coord[ii]);
				}
			}
			
			
			initial_bound_on_degree = solve_options.T.AMP_bound_on_degree;
			solve_options.T.AMP_bound_on_degree = (double) num_new_linears+1;
			detjac_to_detjac_solver_main(solve_options.T.MPType,
																	 W_linprod,
																	 randomizer_matrix,
																	 random_complex_projection, // move from
																	 variable_projection,  // move to
																	 &Wtemp,
																	 solve_options);
			solve_options.T.AMP_bound_on_degree = initial_bound_on_degree;
			
			Wtemp.sort_for_real(solve_options.T);
			
			
			if (Wtemp.num_pts>0)
			{
				//compute the projections, go outside a bit, find points.
				vec_mp projection_values; init_vec_mp(projection_values,Wtemp.num_pts); projection_values->size=Wtemp.num_pts;
				for (jj=0; jj<Wtemp.num_pts; jj++) {
					projection_value_homogeneous_input(&projection_values->coord[jj], Wtemp.pts_mp[jj],variable_projection);
				}
				
				
				vec_mp projections_sorted;
				init_vec_mp(projections_sorted,Wtemp.num_pts); projections_sorted->size = Wtemp.num_pts;
				std::vector< int > index_tracker;
				sort_increasing_by_real(projections_sorted, index_tracker, projection_values);
				
				Wtemp.reset();
				
				
				set_mp(&variable_projection->coord[0],&projections_sorted->coord[0]); // the first one
				sub_mp(&variable_projection->coord[0],&variable_projection->coord[0],one);
				neg_mp(&variable_projection->coord[0],&variable_projection->coord[0]);
				
				lintolin_solver_main(solve_options.T.MPType,
														 W,
														 randomizer_matrix,
														 &variable_projection, 1,
														 &Wtemp,
														 solve_options);
				
				Wtemp.sort_for_real(solve_options.T);
				Wtemp.sort_for_unique(solve_options.T);
				W_crit_real->merge(Wtemp);
				
				Wtemp.reset();
				
				
				set_mp(&variable_projection->coord[0],&projections_sorted->coord[projections_sorted->size-1]); // the last one
				add_mp(&variable_projection->coord[0],&variable_projection->coord[0],one);
				neg_mp(&variable_projection->coord[0],&variable_projection->coord[0]);
				
				lintolin_solver_main(solve_options.T.MPType,
														 W,
														 randomizer_matrix,
														 &variable_projection, 1,
														 &Wtemp,
														 solve_options);
				
				Wtemp.sort_for_real(solve_options.T);
				Wtemp.sort_for_unique(solve_options.T);
				W_crit_real->merge(Wtemp);
				
				Wtemp.reset();
				
				clear_vec_mp(projection_values);
				clear_vec_mp(projections_sorted);
				index_tracker.clear();
			}
			
			Wtemp.reset();
		}
		
		
		if (program_options.verbose_level>=0)
			printf("completed detjac2detjac solve, have %d real solutions to work with\n\n", W_crit_real->num_pts);
		
		W_crit_real->sort_for_unique(solve_options.T);
		
		// if have box, intersect box with C
	} // re: bounding box calc
	
	clear_vec_mp(variable_projection);
	clear_mp(one);
	
	W_linprod.reset();
	W_lintolin.reset();
	W_detjacdetjac.reset();
	
	return success;
}




int get_sum_degrees(char filename[], int num_funcs){
	int degsum = 0, tmpdeg, ii;
	
	FILE *IN;
	
	IN =  safe_fopen_read(filename);
	
	for (ii = 0; ii<num_funcs; ii++) {
		fscanf(IN,"%d",&tmpdeg);
		degsum += tmpdeg;
	}
	
	fclose(IN);
	
	
	return degsum;
}


//
//
//void sort_for_membership(char * input_file,
//												 witness_set *W_out,
//												 witness_set W_in,
//												 char *stifle_text){
////	printf("sorting points for membership\n");
//
//
//	if (W_in.incidence_number==-1) {
//		printf("input witness_set has unset incidence_number for comparison.\n");
//		exit(-1112);
//	}
//
//	W_out->MPType = W_in.MPType;
//	W_out->incidence_number = W_in.incidence_number;
//	W_out->num_variables = W_in.num_variables;
//	W_out->num_var_gps = W_in.num_var_gps;
//
//	//copy the linears, patches from old to new
//	cp_patches(W_out, W_in);
//	cp_linears(W_out, W_in);
//	cp_names(W_out, W_in);
//
//	//more important files out of the way.  this will probably crash the program if it is called without these files being present.
//	rename_bertini_files_dotbak();
//
//
//	int strLength = 0, digits = 15, *declarations = NULL;
//  char *SysStr = NULL,*fmt = NULL, *bertini_command="bertini";
//  FILE *IN = NULL;
//
//
//
//	//make the command string to run
//	strLength = 1 + snprintf(NULL, 0, "%s input_membership_test %s ", bertini_command, stifle_text);
//  SysStr = (char *)br_malloc(strLength * sizeof(char));
//  sprintf(SysStr, "%s input_membership_test %s ", bertini_command, stifle_text);
//
//
//
//	// make the format to write the member_points file
//  strLength = 1 + snprintf(NULL, 0, "%%.%dle %%.%dle\n", digits, digits);
//  // allocate size
//  fmt = (char *)br_malloc(strLength * sizeof(char));
//  // setup fmt
//  sprintf(fmt, "%%.%dle %%.%dle\n", digits, digits);
//
//
//  // setup input file
//	IN = safe_fopen_read(input_file);
//  partitionParse(&declarations, IN, "func_input_real", "config_real",0); // the 0 means not self conjugate
//	fclose(IN);
//
//
//	//check existence of the required witness_data file.
//	IN = safe_fopen_read("witness_data");
//	fclose(IN);
//
//
//
//
//
//
//	//only need to do this once.  we put the point and its conjugate into the same member points file and run the membership test simultaneously with one bertini call.
//  membership_test_input_file("input_membership_test", "func_input_real", "config_real",3);
//
//
//
//
//	// Do membership test
//	printf("*\n%s\n*\n",SysStr);
//  system(SysStr);
//
//
//	int on_component_indicator[W_in.num_pts];
//	read_incidence_matrix_wrt_number(on_component_indicator,W_in.incidence_number);
//
////	printf("done reading incidence_matrix\n");
//
//	int *is_on_component;
//	is_on_component = (int *)br_malloc(W_in.num_pts*sizeof(int));
//
//
//
//	int num_pts_on_component =0;
//	int ii,jj;
//	for (ii=0; ii<W_in.num_pts; ii++) {
////		printf("%d\n",on_component_indicator[ii]);
//
//		if (on_component_indicator[ii]==1) {
//			num_pts_on_component++;
//			is_on_component[ii]=1;
//		}
//		else{
//			is_on_component[ii]=0;
//		}
//
//	}
//
//	if (num_pts_on_component==0) {
//		printf("found 0 points out of %d candidates, on component.\n",W_in.num_pts);
//		restore_bertini_files_dotbak();
//		return;
//	}
//
//
//	vec_d *points_on_component;
//	points_on_component =(point_d *)br_malloc(num_pts_on_component*sizeof(point_d));
//
//	int *indices_on_component;
//	indices_on_component = (int *)br_malloc(num_pts_on_component*sizeof(int));
//
//	int counter =0;
//	for (ii=0; ii<W_in.num_pts; ii++) {
//		if (on_component_indicator[ii]==1) {
//			init_vec_d(points_on_component[counter],W_in.pts_d[ii]->size);
//			points_on_component[counter]->size = W_in.pts_d[ii]->size;
//			vec_cp_d(points_on_component[counter],W_in.pts_d[ii]);
//
//			indices_on_component[counter] = ii;
//
//			counter++;
//		}
//	}
//
//
//
//	//now remove duplicates
//
//
//
//	int *is_unique; int curr_uniqueness; int num_good_pts = 0;
//	is_unique  = (int *)br_malloc(num_pts_on_component*sizeof(int));
//	for (ii = 0; ii<num_pts_on_component; ++ii) {
//		curr_uniqueness = 1;
//		if (ii!= (num_pts_on_component-1) ) { // the last point is unique...  always
//			for (jj=ii+1; jj<num_pts_on_component; ++jj) {
//				if (isSamePoint_homogeneous_input_d(points_on_component[ii],points_on_component[jj])){
//					//formerly (isSamePoint(points_on_component[ii],NULL,52,points_on_component[jj],NULL,52,1e-8)){
//					curr_uniqueness = 0;
////					printf("the following two points are not distinct:\n");
////					print_point_to_screen_matlab(points_on_component[ii],"left");
////					print_point_to_screen_matlab(points_on_component[jj],"right");
//				}
//			}
//		}
//
//
//		if (curr_uniqueness==1) {
//			is_unique[ii] = 1;
//			num_good_pts++;
//		}
//		else
//		{
//			is_unique[ii] = 0;
//		}
//
//	}
//
//
//
//
//
//
//
//
//	//allocate the memory
//	W_out->pts_d=(point_d *)br_malloc(num_good_pts*sizeof(point_d));
//	W_out->pts_mp=(point_mp *)br_malloc(num_good_pts*sizeof(point_mp));
//	W_out->num_pts = num_good_pts;
//	W_out->num_pts = num_good_pts;
//
//
//
//	//copy in the distinct points lying on the correct component
//	counter = 0;
//	for (ii=0; ii<num_pts_on_component; ++ii) {
//		if (is_unique[ii]==1) {
//			init_vec_d(W_out->pts_d[counter],W_in.num_variables); W_out->pts_d[counter]->size = W_in.num_variables;
//			init_vec_mp2(W_out->pts_mp[counter],W_in.num_variables,1024);  W_out->pts_mp[counter]->size = W_in.num_variables;
//
//			vec_cp_d(W_out->pts_d[counter], W_in.pts_d[indices_on_component[ii]]);
//			vec_cp_mp(W_out->pts_mp[counter], W_in.pts_mp[indices_on_component[ii]]);
//			counter++;
//		}
//	}
//
//	if (!counter==num_good_pts){
//		printf("counter mismatch; counter!=num_good_pts in sort_for_membership\n");
//		exit(169);
//	}
//
//		//clear the memory
//
//	for (ii=0; ii<num_pts_on_component; ++ii) {
//		clear_vec_d(points_on_component[ii]);
//	}
//	free(points_on_component);
//
//
//	free(is_unique);
//  free(declarations);
//
//  // delete temporary files
//  remove("func_input_real");
//  remove("config_real");
//	remove("incidence_matrix");
////	remove("member_points");
//
//	//move files back into place
//	restore_bertini_files_dotbak();
//	return;
//}
//
//









int verify_projection_ok(witness_set & W,
												 vec_mp projection,
												 solver_configuration & solve_options)
{
	
	
	//create a matrix
	mat_mp randomizer_matrix;
	init_mat_mp(randomizer_matrix,W.num_variables-W.num_patches-W.dim,solve_options.PPD.num_funcs);
	
	//create the array of integers
	int *randomized_degrees = (int *)br_malloc((W.num_variables-W.num_patches-W.dim)*sizeof(int));
	
	//get the matrix and the degrees of the resulting randomized functions.
	make_randomization_matrix_based_on_degrees(randomizer_matrix, &randomized_degrees, W.num_variables-W.num_patches-W.dim, solve_options.PPD.num_funcs);
	
	int invalid_flag = verify_projection_ok(W, randomizer_matrix, projection, solve_options);
	
	clear_mat_mp(randomizer_matrix);
	free(randomized_degrees);
	
	return invalid_flag;
}



int verify_projection_ok(witness_set & W,
												 mat_mp randomizer_matrix,
												 vec_mp projection,
												 solver_configuration & solve_options)
{
	int ii,jj;
	
	
	int invalid_flag;
	
	
	vec_mp temp_rand_point;  init_vec_mp(temp_rand_point,W.num_variables); temp_rand_point->size = W.num_variables;
	set_one_mp(&temp_rand_point->coord[0]); // first coordinate must be 1
	for (ii=1; ii<W.num_variables; ++ii) {
		get_comp_rand_mp(&temp_rand_point->coord[ii]);
	}
	
	prog_t SLP;
	setupProg(&SLP, solve_options.T.Precision, 2);
	
	
	comp_mp zerotime; init_mp(zerotime);
	set_zero_mp(zerotime);
	
	
	
	eval_struct_mp ED; init_eval_struct_mp(ED, 0, 0, 0);
	evalProg_mp(ED.funcVals, ED.parVals, ED.parDer, ED.Jv, ED.Jp, temp_rand_point, zerotime, &SLP);
	
	mat_mp AtimesJ; init_mat_mp(AtimesJ, 1, 1); AtimesJ->rows = AtimesJ->cols = 1;
	
	mat_mul_mp(AtimesJ, randomizer_matrix, ED.Jv);
	
	//inflate the matrix
	
	mat_mp detme;  init_mat_mp(detme, W.num_variables-1, W.num_variables-1);
	detme->cols = detme->rows= W.num_variables-1;
	for (ii=0; ii<W.num_variables-2; ++ii) {
		for (jj=0; jj<W.num_variables-1; ++jj) {
			set_mp(&detme->entry[ii][jj],&AtimesJ->entry[ii][jj+1]); // omit the homogeneous coordinate's columns
		}
	}
	
	for (ii=0; ii<W.num_variables-1; ii++) {
		set_mp(&detme->entry[W.num_variables-2][ii],&projection->coord[ii+1]);
	}
	
	comp_mp determinant; init_mp(determinant);
	take_determinant_mp(determinant,detme);
	
	if ( d_abs_mp(determinant) < 1e-12){
		invalid_flag = 0;
		std::cout << d_abs_mp(determinant) << "\n";
		print_matrix_to_screen_matlab(ED.Jv,"Jv");
		print_matrix_to_screen_matlab(detme,"detme");
	}
	else
		invalid_flag = 1;
	
	return invalid_flag;
	
}









void curve_decomposition::computeCurveNotSelfConj(witness_set & W_in,
                             vec_mp         pi_mp,
														 vertex_set			&V,
                             int           num_vars,
														 BR_configuration & program_options,
														 solver_configuration & solve_options)
/***************************************************************\
 * USAGE: compute the isolated points for Non-Self-Conjugate case            *
 * ARGUMENTS: witness set, projection, # of variables, name of input file*
 * RETURN VALUES: Curve decomposition*
 * NOTES:                                                        *
 \***************************************************************/
{
	
	// num_vars includes the homogeneous variable
	
	
  int ii,jj,num_sols,*declarations = NULL;
  
	std::string bertini_system_command = program_options.bertini_command;
	bertini_system_command.append(" input_NSC");
	
  FILE *IN = NULL;
  vec_mp cur_sol,cur_sol_bar;
  init_vec_mp(cur_sol,num_vars); cur_sol->size = num_vars;
	set_one_mp(&cur_sol->coord[0]);
	
  init_vec_mp(cur_sol_bar,num_vars); cur_sol_bar->size = num_vars;
	set_one_mp(&cur_sol_bar->coord[0]);
	
  partition_parse(&declarations, W_in.input_filename, "func_input_nsc", "config_nsc",1);
	
	
  //generate input file
  diag_homotopy_input_file("input_NSC", "func_input_nsc","func_inputbar","config_nsc",W_in.L_mp[0],num_vars-1);
  //generate start file
	diag_homotopy_start_file("start",  W_in);
	
	
  //run bertini
	
	copyfile("witness_data","witness_data_0");
	
  system(bertini_system_command.c_str());
	
	rename("witness_data_0","witness_data");
	
	
  //read the real solutions
  IN = safe_fopen_read("real_solutions");
  
  fscanf(IN, "%d\n\n", &num_sols);
	
	
	vertex temp_vertex;
	//	init_vertex(&temp_vertex);
	change_size_vec_mp(temp_vertex.pt_mp,num_vars); temp_vertex.pt_mp->size = num_vars;
	
	temp_vertex.type = ISOLATED;
	
	comp_mp projection_value;  init_mp(projection_value);
	
	for(ii=0;ii<num_sols;ii++) {
		for(jj=0;jj<num_vars-1;jj++){
			mpf_inp_str(cur_sol->coord[jj+1].r, IN, 10);
			mpf_inp_str(cur_sol->coord[jj+1].i, IN, 10);
			
			mpf_inp_str(cur_sol_bar->coord[jj+1].r, IN, 10);
			mpf_inp_str(cur_sol_bar->coord[jj+1].i, IN, 10);
		}
    
    //check if x=x_bar
		
    if (isSamePoint_homogeneous_input(cur_sol,cur_sol_bar)) { // x=x_bar
			
			vec_cp_mp(temp_vertex.pt_mp,cur_sol);
			
			dot_product_mp(temp_vertex.projVal_mp, temp_vertex.pt_mp, pi_mp);
			
			if (index_in_vertices(V, temp_vertex.pt_mp, temp_vertex.projVal_mp,
															solve_options.T) ==-1
					)
				add_vertex(V,temp_vertex);
		}
		
	}
	
	clear_mp(projection_value);
  //set the number of vertices
  fclose(IN);
  //clear
  clear_vec_d(cur_sol);
  clear_vec_d(cur_sol_bar);
  free(declarations);
  
	// delete temporary files
  remove("func_input_nsc");
  remove("config_nsc");
  remove("func_inputbar");
  remove("var_names");
	
	
}




// MISC. FUNCTIONS


void diag_homotopy_input_file(boost::filesystem::path outputFile,
                              boost::filesystem::path funcInputx,
                              boost::filesystem::path funcInputy,
                              boost::filesystem::path configInput,
                              vec_mp L,
                              int   num_vars)
/***************************************************************\
 * USAGE: setup input file to do diagonal homotopy             *
 * ARGUMENTS: name of output file, function & configuration input*
 * RETURN VALUES: none                                           *
 * NOTES:                                                        *
 \***************************************************************/
{
  char ch,**str,*fmt = NULL;
  int ii,jj,size;
  mat_d A;
	
	FILE *IN = NULL;
	
	
  str=(char **)br_malloc(num_vars*sizeof(char *));
  for(ii=0;ii<num_vars;ii++)
    str[ii]=(char*)br_malloc(sizeof(char)*256);
  
	FILE *OUT = safe_fopen_write(outputFile.c_str());
	
  // setup configurations in OUT
  fprintf(OUT, "CONFIG\n");
  IN = safe_fopen_read(configInput.c_str());
  fclose(IN);
	
  fprintf(OUT, "USERHOMOTOPY: 1;\nDeleteTempFiles: 1;\nEND;\nINPUT\n");
	
  // setup variables in OUT
  IN = safe_fopen_read(funcInputx.c_str());
  while ((ch = fgetc(IN)) != EOF )
    fprintf(OUT, "%c", ch);
  fclose(IN);
	
	//setup the function name in OUT
	IN = safe_fopen_read(funcInputy.c_str());
  while ((ch = fgetc(IN)) != EOF )
    fprintf(OUT, "%c", ch);
  fclose(IN);
	
	
  IN = safe_fopen_read("var_names");
  ii=0;jj=0;
  while ((ch = fgetc(IN)) != EOF)
  {
    if(ch!='\n')
      str[ii][jj++]=ch;
    else
    {
      str[ii++][jj]='\0';
      jj=0;
    }
  }
	
  //setup the linear equations
  // find the size needed
  size = 1 + snprintf(NULL, 0, "%%.%dlf+%%.%dlf*I", 15, 15);
  // allocate size
  fmt = (char *)br_malloc(size * sizeof(char));
  // setup fmt
  sprintf(fmt, "%%.%dlf+%%.%dlf*I", 15, 15);
  // output the linear function L and L_bar
	
	comp_mp temp;  init_mp(temp);
  for (ii = 0; ii < L->size; ii++)
  {
    fprintf(OUT, "L%d = ",ii);
    // print output
		print_mp(OUT, 0, &L->coord[ii]); fprintf(OUT, "\n");
		//    fprintf(OUT, fmt, L->coord[ii].r, L->coord[ii].i);
    fprintf(OUT, ";\n");
		
    fprintf(OUT, "Lbar%d = ",ii);
		conjugate_mp(temp, &L->coord[ii]);
    // print output
		print_mp(OUT, 0, temp); fprintf(OUT, "\n");
		//    fprintf(OUT, fmt, L->coord[ii].r, -L->coord[ii].i);
    fprintf(OUT, ";\n");
  }
  fprintf(OUT, "\n");
	clear_mp(temp);
	
  //Generate a random matrix A and output to input file.
  init_mat_d(A, 2, num_vars);
	make_matrix_random_d(A, 2, num_vars);
  for (ii = 0; ii < 2; ii++)
    for(jj=0;jj<num_vars;jj++)
    {
      fprintf(OUT, "A%d%d = ",ii,jj);
      // print output
      fprintf(OUT, fmt, A->entry[ii][jj].r, A->entry[ii][jj].i);
      fprintf(OUT, ";\n");
    }
  //setup the diagonal homotopy functions
  fprintf(OUT, "\nL=t*(");
  //(Lx-1)*t+(1-t)*A[0]*(x-x_bar)
  for(ii=0;ii<num_vars;ii++)
  {
    fprintf(OUT, "L%d*", ii);
    fprintf(OUT, "%s", str[ii]);
    fprintf(OUT, "+");
  }
  fprintf(OUT, "-1)+(1-t)*(");
  for(ii=0;ii<num_vars;ii++)
  {
    fprintf(OUT, "A0%d*", ii);
    fprintf(OUT, "(%s-%s", str[ii],str[ii]);
    fprintf(OUT, "bar)+");
  }
  //(L_bar x_bar-1)*t+(1-t)*A[1]*(x-x_bar)
  fprintf(OUT, "0);\nLbar=t*(");
  for(ii=0;ii<num_vars;ii++)
  {
    fprintf(OUT, "Lbar%d*", ii);
    fprintf(OUT, "%sbar", str[ii]);
    fprintf(OUT, "+");
  }
  fprintf(OUT, "-1)+(1-t)*(");
  for(ii=0;ii<num_vars;ii++)
  {
    fprintf(OUT, "A1%d*", ii);
    fprintf(OUT, "(%s-%s", str[ii],str[ii]);
    fprintf(OUT, "bar)+");
  }
  fprintf(OUT, "0);\nEND;");
  fclose(OUT);
  //free
  for(ii=0;ii<num_vars;ii++)
    free(str[ii]);
  free(str);
  free(fmt);
  clear_mat_d(A);
  return;
}



void diag_homotopy_start_file(boost::filesystem::path startFile,
                              witness_set & W)
/***************************************************************\
 * USAGE: setup start file to do diagonal homotopy             *
 * ARGUMENTS: name of output file, start points & number of variables*
 * RETURN VALUES: none                                           *
 * NOTES:                                                        *
 \***************************************************************/
{
  
  char *fmt = NULL;
  int ii,jj,kk;
	int size,digits=15;
	
	
	FILE *OUT = safe_fopen_write(startFile.c_str());
  
  size = 1 + snprintf(NULL, 0, "%%.%de %%.%de\n", digits, digits);
  // allocate size
  fmt = (char *)br_malloc(size * sizeof(char));
  // setup fmt & fmtb
  sprintf(fmt, "%%.%de %%.%de\n", digits, digits);
	// output the number of start points
  fprintf(OUT,"%d\n\n",W.num_pts*W.num_pts);
	
	
	comp_mp temp; init_mp(temp);
	
	
	for (kk=0; kk<W.num_pts; kk++){
		for (ii=0; ii<W.num_pts; ii++) { // output {w \bar{w}}'
			
			vec_mp result; init_vec_mp(result,0);
			vec_mp result2; init_vec_mp(result2,0);
			
			dehomogenize(&result,W.pts_mp[ii]);
			dehomogenize(&result2,W.pts_mp[kk]);
			
			
			for(jj=0; jj<W.num_variables-1;jj++) {
				print_mp(OUT, 15, &result->coord[jj]); fprintf(OUT, "\n");
				
				conjugate_mp(temp, &result2->coord[jj] )
				print_mp(OUT, 15, temp); fprintf(OUT, "\n");
				//      fprintf(OUT, fmt, result->coord[jj].r,  result->coord[jj].i);
			}
			
			
			
			fprintf(OUT,"\n");
		}// re: ii
	}// re: kk
	
  free(fmt);
  fclose(OUT);
}





