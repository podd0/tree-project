#include <iostream>
#include <set>
#include <tuple>
#include <cassert>
#include <yocto/yocto_sampling.h>
#include <yocto/yocto_cli.h>
#include <yocto/yocto_shape.h>
#include <yocto/yocto_sceneio.h>
#include <yocto/yocto_math.h>
#include <yocto/yocto_geometry.h>

namespace yocto
{

	bool cmp3f(vec3f a, vec3f b);

	// represents a branch segment of the tree's skeleton
	struct branch
	{
		vec3f start, end;		  // extremities of the segment
		int parent_ind;			  // index of the parent branch in branches
		float base_radius;
		vector<int> children;	  // indexes of the children in branches
		vector<vec3f> attractors; // points that decide the growth of the segment in this step
		
		vec3f direction();
	};

	/*
	 removes the points which are in kill_range from a branch
	 (points that are too close to a branch.end)
	*/
	void kill_points(vector<vec3f> &points, vector<branch> &branches, float kill_range);

	vec3f random_growth_vector(rng_state rng, float random_factor);

	/* Grows the leaves forward (in their current direction),
	   by adding a branch segment on each leaf
	 */
	void grow_forward(vector<branch> &branches, vector<int> &leaves, float branch_length, rng_state rng, float random_factor);
	/* chooses the points that are attractors for each branch.
	   Returns true if it finds at least one match
	*/
	bool choose_attractors(vector<vec3f> &points, vector<branch> &branches, float attraction_range);

	shape_data shape_from_branches(vector<branch> &branches);

	void grow_towards_attractors(vector<branch> &branches, float branch_length, rng_state rng, float random_factor);

	vector<int> recalc_leaves(vector<branch> &branches);
}