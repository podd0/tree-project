#include <iostream>
#include <set>
#include <tuple>
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

struct branch
{
    vec3f start, end;
    int parent_ind;
    vector<int> children;
    vector<vec3f> attractors;

    vec3f direction() { return normalize(end - start); }
};
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

void grow_forward(vector<branch> &branches, vector<int> &leaves, float branch_length)
{
    for (int lf_ind : range(leaves.size()))
    {
        int br_ind = leaves[lf_ind];
        branch nw;
        nw.start = branches[br_ind].end;
        nw.end = nw.start + branches[br_ind].direction() * branch_length;
        nw.parent_ind = br_ind;
        int child_ind = branches.size();
        leaves[lf_ind] = child_ind;
        branches.push_back(nw);
        branches[br_ind].children.push_back(child_ind);
    }
}

shape_data generate_tree(string input, float branch_length, float kill_range, float attraction_range)
{
    shape_data sampling = load_shape(input);
    vector<vec3f> points = {{0, 0, 0.1}, {10, 10, 10}, {0, 0, -1}, {0, 0.2, -1.1}, {0, 0.2, -0.9}, {0, 0.2, 0.9}, {0, 0.2, 0.1}};
    vector<int> leaves = {0}; // vector of indexes of the leave branches
    vector<branch> branches = {branch{{0, 0, 0}, {0, branch_length, 0}, -1}};

    int iterations = 30;
    while (!points.empty() && iterations > 0)
    {
        --iterations;
        kill_points(points, branches, kill_range);
        // reset all attractors before recalculating them
        for (branch &b : branches)
            b.attractors.clear();

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
            closest->attractors.push_back(p);
        }
        for (vec3f p : branches[0].attractors)
            cout << p << ' ';
        cout << endl;
    }

    return {};
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

    parse_cli(cli, args);

    rng_state rng = make_rng(seed);
    shape_data sh;
    generate_tree(input, branch_length, kill_range, attraction_range);
    save_shape(output, sh);
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
