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

using namespace yocto;
using std::cout;
using std::endl;
using std::tie;

// comparator for set3f
bool cmp3f(vec3f a, vec3f b)
{
    return tie(a.x, a.y, a.z) < tie(b.x, b.y, b.z);
}
// represents a branch segment of the tree's skeleton
struct branch
{
    vec3f start, end;         // extremities of the segment
    int parent_ind;           // index of the parent branch in branches
    vector<int> children;     // indexes of the children in branches
    vector<vec3f> attractors; // points that decide the growth of the segment in this step

    vec3f direction() { return normalize(end - start); }
};
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

rng_state rng;
float random_factor = 0.0f;

vec3f random_growth_vector()
{
    return sample_sphere(rand2f(rng)) * random_factor;
}

/* Grows the leaves forward (in their current direction),
   by adding a branch segment on each leaf
 */
void grow_forward(vector<branch> &branches, vector<int> &leaves, float branch_length)
{
    for (int &leaf : leaves)
    {
        branch new_leaf;
        new_leaf.start = branches[leaf].end;
        vec3f direction = normalize(branches[leaf].direction() + random_growth_vector());
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

void grow_towards_attractors(vector<branch> &branches, float branch_length)
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
        dir += random_growth_vector();
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

// generates the tree model given the parameters and the sampled points
shape_data generate_tree(string input, float branch_length, float kill_range, float attraction_range)
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
            grow_towards_attractors(branches, branch_length);
            leaves = recalc_leaves(branches);
        }
        else
            grow_forward(branches, leaves, branch_length);
    }
    // for (int i : range(branches.size()))
    // {
    //     cout << endl
    //          << "branch " << i << " at pos " << branches[i].end << ": ";
    //     for (vec3f p : branches[i].attractors)
    //         cout << p << ' ';
    // }
    // cout << endl;
    // for (int i : range(branches.size()))
    // {
    //     cout << i << ":";
    //     for (int ch : branches[i].children)
    //         cout << ch << ' ';
    //     cout << endl;
    // }
    cout << "exited at " << 1000000 - iterations << " iterations\n";
    shape_data sh = shape_from_branches(branches);

    // for (vec3f p : sampling.positions)
    // {
    //     sh.points.push_back(sh.positions.size());
    //     sh.positions.push_back(p);
    // }
    return sh;
}

shape_data make_tcone(float rad1, float rad2, float height)
{
    shape_data sh;
    int steps = 16;
    float stepangle = 2 * pi / steps;
    for (int i = 0; i < steps; i++)
    {
        auto x = cosf(i * stepangle), y = sinf(i * stepangle);
        sh.positions.push_back(vec3f{x, y, 0} * rad1 + vec3f{0, 0, height});
        sh.positions.push_back(vec3f{x, y, 0} * rad2 - vec3f{0, 0, height});
    }
    for (int ci = 0; ci < steps; ci++)
    {
        int i = ci * 2;
        int mod = sh.positions.size();
        for (int h : range(1))
        {
            sh.triangles.push_back({i, i + 1, (i + 2) % mod});
            sh.triangles.push_back({(i + 2) % mod, i + 1, (i + 3) % mod});
            i++;
        }
    }

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
}

shape_data miomerge(shape_data a, shape_data b)
{
    int n = a.positions.size();
    shape_data res = {a.points, a.lines, a.triangles, a.quads, a.positions};
    for (vec3f p : b.positions)
        res.positions.push_back(p);
    for (auto p : b.points)
        res.points.push_back(p + n);
    for (auto [a, b] : b.lines)
        res.lines.push_back({a + n, b + n});
    for (auto [a, b, c] : b.triangles)
        res.triangles.push_back({a + n, b + n, c + n});
    for (auto [a, b, c, d] : b.quads)
        res.quads.push_back({a + n, b + n, c + n, d + n});
    return res;
}
shape_data lines_to_trunc_cones(const vector<vec2i> &lines,
                                const vector<vec3f> &positions, float branch_length)
{
    auto shape = shape_data{};
    for (auto &line : lines)
    {
        auto cylinder = make_tcone(branch_length / 7, branch_length / 9, 1);
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

    auto cli = make_cli("tree", "generate treee given a model of attraction points");
    add_option(cli, "input", input, "a model containing the attraction point");
    add_option(cli, "output", output, "the filename of the resulting tree model");
    add_option(cli, "br_length", branch_length, "the length of a single branch segment");
    add_option(cli, "kill", kill_range, "the branch-point distance at which attraction points are deleted");
    add_option(cli, "attraction", attraction_range, "the distance at which attraction points attract the branch grows");
    add_option(cli, "random_factor", random_factor, "decides the influence of randomness in the directions of the branches");
    add_option(cli, "seed", seed, "rng seed (defaults time)");

    parse_cli(cli, args);

    assert(attraction_range > kill_range && kill_range > branch_length);

    rng = make_rng(seed);
    shape_data sh = generate_tree(input, branch_length, kill_range, attraction_range);

    shape_data acc{};
    for (vec3f p : sh.positions)
    {
        shape_data sph = quads_to_triangles(make_sphere(5, branch_length / 7));
        sph = {sph.points, sph.lines, sph.triangles, sph.quads, sph.positions};
        for (vec3f &p2 : sph.positions)
            p2 += p;
        acc = miomerge(acc, sph);
    }

    auto finale = miomerge(acc, lines_to_trunc_cones(sh.lines, sh.positions, branch_length));
    compute_normals(finale);
    save_shape(output, finale);
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
