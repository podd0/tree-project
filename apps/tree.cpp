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

double calc_branch_radius(vector<branch> &branches, branch &curr, double leaf_radius, double inverted_growth)
{
    if (curr.children.empty())
    {
        curr.high_base_radius = leaf_radius;
        return leaf_radius;
    }
    double sum = 0;
    if (curr.children.size() == 1)
        return curr.high_base_radius = calc_branch_radius(branches, branches[curr.children[0]], leaf_radius, inverted_growth);
    for (int child : curr.children)
    {
        calc_branch_radius(branches, branches[child], leaf_radius, inverted_growth);
        sum += pow(branches[child].high_base_radius, inverted_growth);
    }
    curr.high_base_radius = pow(sum, 1 / inverted_growth);
    return curr.high_base_radius;
}

// generates the tree model given the parameters and the sampled points
vector<branch> generate_tree(string input, float branch_length, float kill_range, float attraction_range, rng_state rng, float random_factor, float leaf_radius, float inverted_growth, int iterations)
{
    shape_data sampling = load_shape(input);
    vector<vec3f> points = sampling.positions; // vector of the attractors
    vector<int> leaves = {0}; // vector of indexes of the leave branches
    vector<branch> branches = {branch{{0, 0, 0}, {0, branch_length, 0}, -1}};
    int iteration = 1;
    for (; iteration <= iterations; iteration++)
    {
        kill_points(points, branches, kill_range);
        // reset all attractors before recalculating them
        if (points.empty())
            break;
        if (choose_attractors(points, branches, attraction_range))
        {
            grow_towards_attractors(branches, branch_length, rng, random_factor);
            leaves = recalc_leaves(branches);
            points.pop_back(); // stupid way to avoid "stuck" branches
        }
        else
        {
            cout << "forward \n";
            grow_forward(branches, leaves, branch_length, rng, random_factor);
        }
        cout << "iteration #" << iteration << " remaining points : " << points.size() << " branches : " << branches.size() << endl;
    }
    cout << "exited at " << iteration - 1 << " iterations" << endl;
    return branches;
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

shape_data make_sphere_mesh( vector<branch> branches, int sphere_steps) {
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
    return acc;
}

shape_data make_leaves(vector<branch> branches, string leaf_model, float leaf_scale, string leaves_output, int cone_steps, string output) {
    shape_data leaf = load_shape(leaf_model);
    for (auto &p : leaf.positions)
    {
        p.y *= leaf_scale;
        p.x *= leaf_scale;
    }
    shape_data leaves{};
    for (branch &b : branches)
    {
        if (b.children.empty())
        {
            shape_data leaf_copy = leaf;
            vec3f direction = b.direction() * leaf_scale * 2;
            auto frame = frame_fromz(b.end + direction / 2, -direction);
            cout<<direction <<' '<<b.end<<' '<<frame.o<<endl;
            for (auto &position : leaf_copy.positions)
                position = transform_point(frame, position * vec3f{1, 1, leaf_scale});
            merge_shape_inplace(leaves, leaf_copy);
        }
    }
    return leaves;
}


void run(const vector<string> &args)
{
    uint64_t seed = time(0);
    string input = "points.ply";
    string output = "tree.ply";
    string leaf_model = "leaf.ply";
    string leaves_output = "leaves.ply";
    string skeleton = "";
    float branch_length = 0.2f;
    float kill_range = 0.5f;
    float attraction_range = 1.0f;
    float random_factor = 0.0f;
    double leaf_radius = 0.005f;
    double inverted_growth = 2.0f;
    int sphere_steps = 4;
    int cone_steps = 16;
    bool enable_leaves = false;
    float leaf_scale = 0.05;
    int iterations = 1000;

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
    add_option(cli, "leaf", leaf_model, "leaf model to put on the branches");
    add_option(cli, "leaves_output", leaves_output, "resulting model of leaves");
    add_option(cli, "enable_leaves", enable_leaves, "enable to generate the leaves");
    add_option(cli, "skeleton", skeleton, "set to generate a lines-only version of the model");
    add_option(cli, "leaf_scale", leaf_scale, "scale to apply to the leaf model");
    add_option(cli, "iterations", iterations, "maximum number of iterations while growing branches");
    parse_cli(cli, args);

    assert(attraction_range > kill_range && kill_range > branch_length);

    rng_state rng = make_rng(seed);
    vector<branch> branches = generate_tree(input, branch_length, kill_range, attraction_range, rng, random_factor, leaf_radius, inverted_growth, iterations);
    shape_data leaves;
    shape_data acc;

    calc_branch_radius(branches, branches[0], leaf_radius, inverted_growth);
    acc = make_sphere_mesh(branches, sphere_steps);
    if (skeleton != "")
        save_shape(skeleton, shape_from_branches(branches));
    if (enable_leaves) {
        leaves = make_leaves(branches, leaf_model, leaf_scale, leaves_output, cone_steps, output);
        save_shape(leaves_output, leaves);
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