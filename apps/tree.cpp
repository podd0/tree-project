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

// generates the tree model given the parameters and the sampled points
shape_data generate_tree(string input, float branch_length, float kill_range, float attraction_range, rng_state rng, float random_factor)
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
        else
            grow_forward(branches, leaves, branch_length, rng, random_factor);
    }
    cout << "exited at " << 1000000 - iterations << " iterations\n";
    shape_data sh = shape_from_branches(branches);
    return sh;
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

shape_data lines_to_trunc_cones(const vector<vec2i> &lines,
                                const vector<vec3f> &positions, float branch_length)
{
    auto shape = shape_data{};
    for (auto &line : lines)
    {
        auto cylinder = make_truncated_cone(branch_length / 7, branch_length / 9, 1);
        auto frame = frame_fromz((positions[line.x] + positions[line.y]) / 2,
                                 positions[line.x] - positions[line.y]);
        auto length = distance(positions[line.x], positions[line.y]);
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

    parse_cli(cli, args);

    assert(attraction_range > kill_range && kill_range > branch_length);

    rng_state rng = make_rng(seed);
    shape_data sh = generate_tree(input, branch_length, kill_range, attraction_range, rng, random_factor);

    shape_data acc{};
    shape_data csph = quads_to_triangles(make_sphere(3, branch_length / 7));
    for (vec3f p : sh.positions)
    {
        shape_data sph = {csph.points, csph.lines, csph.triangles, csph.quads, csph.positions};
        for (vec3f &p2 : sph.positions)
            p2 += p;
        merge_shape_inplace(acc, sph);
    }

    merge_shape_inplace(acc, lines_to_trunc_cones(sh.lines, sh.positions, branch_length));
    compute_normals(acc);
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
