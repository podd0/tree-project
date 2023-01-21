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
#include <yocto/truncated_cone.h>

using namespace yocto;
using std::cout;
using std::endl;
using std::tie;

float dfs(vector<branch> &branches, branch &curr, float leaf_radius, float inverted_growth)
{
    if (curr.children.empty())
    {
        curr.high_base_radius = leaf_radius;
        return leaf_radius;
    }
    float sum = 0;
    for (int child : curr.children)
    {
        sum += pow(dfs(branches, branches[child], leaf_radius, inverted_growth), inverted_growth);
    }
    curr.high_base_radius = pow(sum, 1 / inverted_growth);
    return curr.high_base_radius;
}

// generates the tree model given the parameters and the sampled points
vector<branch> generate_tree(string input, float branch_length, float kill_range, float attraction_range, rng_state rng, float random_factor, float leaf_radius, float inverted_growth)
{
    shape_data sampling = load_shape(input);
    vector<vec3f> points = sampling.positions; // vector of the attractors
    //{{0, 0, 0.6}, {10, 10, 10}, {0, 0, -1}, {0, 0.2, -1.1}, {0, 0.2, -0.9}, {0, 0.2, 0.9}, {0, 0.2, 0.1}};
    vector<int> leaves = {0}; // vector of indexes of the leave branches
    vector<branch> branches = {branch{{0, 0, 0}, {0, branch_length, 0}, -1}};

    int iterations = 1000000;
    while (!points.empty() && iterations)
    {
        --iterations;
        kill_points(points, branches, kill_range);
        // reset all attractors before recalculating them

        if (choose_attractors(points, branches, attraction_range))
        {
            grow_towards_attractors(branches, branch_length, rng, random_factor);
            leaves = recalc_leaves(branches);
        }
        else {
            cout<<"forward \n";
            grow_forward(branches, leaves, branch_length, rng, random_factor);
        }
        cout<< "iteration #"<<1000000 - iterations<< "remaining points : "<<points.size()<<" branches : "<<branches.size()<<endl;
    }
    cout << "exited at " << 1000000 - iterations << " iterations" << endl;
    // shape_data sh = shape_from_branches(branches);
    return branches;
}

void merge_shape_inplace(shape_data &shape, const shape_data &merge)
{
    auto offset = (int)shape.positions.size();
    for (auto &p : merge.points)
        shape.points.push_back(p + offset);
    for (auto &l : merge.lines)
        shape.lines.push_back({l.x + offset, l.y + offset});
    for (auto &t : merge.triangles)
        shape.triangles.push_back({t.x + offset, t.y + offset, t.z + offset});
    for (auto &q : merge.quads)
        shape.quads.push_back(
            {q.x + offset, q.y + offset, q.z + offset, q.w + offset});
    shape.positions.insert(
        shape.positions.end(), merge.positions.begin(), merge.positions.end());
    shape.tangents.insert(
        shape.tangents.end(), merge.tangents.begin(), merge.tangents.end());
    shape.texcoords.insert(
        shape.texcoords.end(), merge.texcoords.begin(), merge.texcoords.end());
    shape.colors.insert(
        shape.colors.end(), merge.colors.begin(), merge.colors.end());
    shape.radius.insert(
        shape.radius.end(), merge.radius.begin(), merge.radius.end());
}

shape_data lines_to_trunc_cones(const vector<branch> &branches, int steps)
{
    auto shape = shape_data{};
    for (int i : range(branches.size()))
    {
        const branch &b = branches[i];
        float low_base_radius;

        if (i == 0)
            low_base_radius = b.high_base_radius;
        else
            low_base_radius = branches[b.parent_ind].high_base_radius;

        auto cylinder = make_truncated_cone(low_base_radius, b.high_base_radius, 1, steps);
        auto frame = frame_fromz((b.start + b.end) / 2,
                                 b.start - b.end);
        auto length = distance(b.start, b.end);
        for (auto &position : cylinder.positions)
            position = transform_point(frame, position * vec3f{1, 1, length / 2});
        for (auto &normal : cylinder.normals)
            normal = transform_direction(frame, normal);
        merge_shape_inplace(shape, cylinder);
    }

    return shape;
}

void run(const vector<string> &args)
{
    uint64_t seed = time(0);
    string input = "points.ply";
    string output = "tree.ply";
    float branch_length = 0.2f;
    float kill_range = 0.5f;
    float attraction_range = 1.0f;
    float random_factor = 0.0f;
    float leaf_radius = 0.005f;
    float inverted_growth = 2.0f;
    int sphere_steps = 4;
    int cone_steps = 16;

    auto cli = make_cli("tree", "generate treee given a model of attraction points");
    add_option(cli, "input", input, "a model containing the attraction point");
    add_option(cli, "output", output, "the filename of the resulting tree model");
    add_option(cli, "br_length", branch_length, "the length of a single branch segment");
    add_option(cli, "kill", kill_range, "the branch-point distance at which attraction points are deleted");
    add_option(cli, "attraction", attraction_range, "the distance at which attraction points attract the branch grows");
    add_option(cli, "random_factor", random_factor, "decides the influence of randomness in the directions of the branches");
    add_option(cli, "seed", seed, "rng seed (defaults time)");
    add_option(cli, "leaf_radius", leaf_radius, "radius of leaves");
    add_option(cli, "inverted_growth", inverted_growth, "determines how the branch get thinner");
    add_option(cli, "sphere_steps", sphere_steps, "sphere subdivisions");
    add_option(cli, "cone_steps", cone_steps, "cone subdivisions");
    parse_cli(cli, args);

    assert(attraction_range > kill_range && kill_range > branch_length);

    rng_state rng = make_rng(seed);
    vector<branch> branches = generate_tree(input, branch_length, kill_range, attraction_range, rng, random_factor, leaf_radius, inverted_growth);
    dfs(branches, branches[0], leaf_radius, inverted_growth);
    shape_data acc{};
    shape_data csph = quads_to_triangles(make_sphere(sphere_steps, 1));
    for (branch &b : branches)
    {
        shape_data sph = {csph.points, csph.lines, csph.triangles, csph.quads, csph.positions};
        for (vec3f &p2 : sph.positions)
        {
            p2 *= b.high_base_radius;
            p2 += b.end;
        }
        merge_shape_inplace(acc, sph);
    }

    merge_shape_inplace(acc, lines_to_trunc_cones(branches, cone_steps));
    acc.normals = compute_normals(acc);
    save_shape(output, acc);
}
int main(int argc, const char *argv[])
{
    try
    {
        run({argv, argv + argc});
        return 0;
    }
    catch (const std::exception &error)
    {
        print_error(error.what());
        return 1;
    }
}
