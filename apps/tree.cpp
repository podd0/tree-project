#include <iostream>
#include <set>
#include <tuple>
#include <yocto/yocto_sampling.h>
#include <yocto/yocto_cli.h>
#include <yocto/yocto_shape.h>
#include <yocto/yocto_sceneio.h>
#include <yocto/yocto_math.h>

using namespace yocto;
using std::cout;
using std::endl;
using std::tie;

void generate_tree(string input)
{
    shape_data sampling = load_shape(input);
    auto cmp = [](vec3f a, vec3f b)
    {
        return tie(a.x, a.y, a.z) < tie(b.x, b.y, b.z);
    };
    std::set<vec3f, decltype(cmp)>
        points(sampling.positions.begin(), sampling.positions.end(), cmp);
}

void run(const vector<string> &args)
{
    uint64_t seed = time(0);
    string input = "points.ply";
    string output = "tree.ply";

    auto cli = make_cli("tree", "generate treee given a model of attraction points");
    add_option(cli, "input", input, "a model containing the attraction point");
    add_option(cli, "output", output, "the filenmae of the resulting tree model");

    parse_cli(cli, args);

    rng_state rng = make_rng(seed);
    shape_data sh;
    generate_tree(input);
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
