#include <iostream>
#include <yocto/yocto_sampling.h>
#include <yocto/yocto_cli.h>
#include <yocto/yocto_shape.h>
#include <yocto/yocto_sceneio.h>

using namespace yocto;
using std::cout;
using std::endl;

void generateTree(string input)
{
    shape_data sampling = load_shape(input);
    for (vec3f p : sampling.positions)
        cout << p.x << endl;
}

void run(const vector<string> &args)
{
    uint64_t seed = time(0);
    string input = "points.ply";
    string output = "tree.ply";
    int samples = 1000;

    auto cli = make_cli("tree", "generate treee given a model of attraction points");
    add_option(cli, "input", input, "a model containing the attraction point");
    add_option(cli, "output", output, "the filenmae of the resulting tree model");

    parse_cli(cli, args);

    rng_state rng = make_rng(seed);
    shape_data sh;
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
