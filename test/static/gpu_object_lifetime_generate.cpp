#include <Halide.h>
#include <stdio.h>

using std::vector;

using namespace Halide;

int main(int argc, char **argv) {

    Var x;

    Image<int32_t> input(16);
    for (int i = 0; i < input.width(); ++i) {
        input(i) = i;
    }
    input.set_host_dirty();

    Func f;
    f(x) = x % 16; //input(x % 16);

    Target target = get_target_from_environment();
    if (target.has_gpu_feature()) {
        f.gpu_tile(x, 16);
    }
    // The test requires gpu_debug to examine the output.
    target.features |= Target::GPUDebug;

    f.compile_to_file("func_gpu_object_lifetime", target);

    return 0;
}
