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
#include <yocto/branch.h>
namespace yocto
{
    using std::cout;
    using std::endl;
    using std::tie;

    // comparator for set3f
    bool cmp3f(vec3f a, vec3f b)
    {
        return tie(a.x, a.y, a.z) < tie(b.x, b.y, b.z);
    }
    // represents a branch segment of the tree's skeleton
    vec3f branch::direction() { return normalize(end - start); }
    

    /*
     removes the points which are in kill_range from a branch
     (points that are too close to a branch.end)
    */
    void kill_points(vector<vec3f> &points, vector<branch> &branches, float kill_range)
    {
        vector<vec3f> old = points;
        points.clear();

        for (vec3f p : old)
        {
            bool kill = false;
            for (branch &br : branches)
                if (distance(br.end, p) < kill_range)
                {
                    kill = true;
                    break;
                }
            if (!kill)
                points.push_back(p);
        }
    }

    vec3f random_growth_vector(rng_state rng, float random_factor)
    {
        return sample_sphere(rand2f(rng)) * random_factor;
    }

    /* Grows the leaves forward (in their current direction),
       by adding a branch segment on each leaf
     */
    void grow_forward(vector<branch> &branches, vector<int> &leaves, float branch_length, rng_state rng, float random_factor)
    {
        for (int &leaf : leaves)
        {
            branch new_leaf;
            new_leaf.start = branches[leaf].end;
            vec3f direction = normalize(branches[leaf].direction() + random_growth_vector(rng, random_factor));
            new_leaf.end = new_leaf.start + direction * branch_length;
            new_leaf.parent_ind = leaf;
            int child_ind = branches.size();
            branches.push_back(new_leaf);
            branches[leaf].children.push_back(child_ind);
            leaf = child_ind; // since leaf has a child, we can swap it with the new leaf in leaves
        }
    }

    /* chooses the points that are attractors for each branch.
       Returns true if it finds at least one match
    */
    bool choose_attractors(vector<vec3f> &points, vector<branch> &branches, float attraction_range)
    {
        for (branch &b : branches)
            b.attractors.clear();
        bool match_found = false;
        for (vec3f p : points)
        {
            float min = 1.0 / 0.0;
            branch *closest = NULL;
            for (branch &b : branches)
            {
                float dist = distance(p, b.end);
                if (dist < attraction_range && min > dist)
                {
                    closest = &b;
                    min = dist;
                }
            }
            if (closest != NULL)
            {
                closest->attractors.push_back(p);
                match_found = true;
            }
        }
        return match_found;
    }

    shape_data shape_from_branches(vector<branch> &branches)
    {
        shape_data sh;
        int ind = 0;
        for (auto &br : branches)
        {
            sh.positions.push_back(br.end);
            for (int child : br.children)
                sh.lines.push_back({ind, child});
            ind++;
        }
        sh.positions.push_back(branches[0].start);
        sh.lines.push_back({(int)sh.positions.size() - 1, 0});
        return sh;
    }

    void grow_towards_attractors(vector<branch> &branches, float branch_length, rng_state rng, float random_factor)
    {
        for (int i : range(branches.size()))
        {
            branch &b = branches[i];
            if (b.attractors.empty())
                continue;
            vec3f dir = {0, 0, 0};
            for (vec3f attr : b.attractors)
            {
                dir += normalize(attr - b.end);
            }
            dir /= b.attractors.size();
            dir += random_growth_vector(rng, random_factor);
            dir = normalize(dir);

            branch new_leaf;
            new_leaf.start = b.end;
            new_leaf.end = new_leaf.start + dir * branch_length;
            new_leaf.parent_ind = i;
            int child_ind = branches.size();
            branches.push_back(new_leaf);
            branches[i].children.push_back(child_ind);
        }
    }

    vector<int> recalc_leaves(vector<branch> &branches)
    {
        vector<int> leaves;
        for (int i : range(branches.size()))
        {
            if (branches[i].children.empty())
                leaves.push_back(i);
        }
        return leaves;
    }
}
