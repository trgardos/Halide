#include <Halide.h>
#include <stdio.h>

#ifdef _MSC_VER
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT
#endif

using namespace Halide;

int call_count = 0;
extern "C" DLLEXPORT int call_counter(int x) {
    call_count++;
    return x;
}
HalideExtern_1(int, call_counter, int);

int main(int argc, char **argv) {
    Var x("x"), y("y");

    Param<int> width("width");
    width.set(16);

    RDom range(0, width*2);
    RVar xi, xo;

    {
        // Test bounds inference of an RVar split.
        Func f("f"), g("g");

        g(x) = call_counter(x);

        f(x) = 0;
        f(range.x) = g(range.x);

        f.update(0).split(range.x, xo, xi, 2);
        g.compute_at(f, xi);

        f.compile_to_lowered_stmt("rvar_bounds_split.html", HTML);

        call_count = 0;
        f.realize(32);
        if (call_count != 32) {
            printf("Error in split! call_counter called %d instead of %d times.", call_count, 32);
            return -1;
        }
    }
/*
    {
        // Test bounds inference of an RVar split + reorder.
        Func f("f"), g("g");

        g(x) = call_counter(x);

        f(x) = 0;
        f(range.x) = g(range.x);

        f.update(0).split(range.x, xo, xi, 2).reorder(xo, xi);
        g.compute_at(f, xi);

        f.compile_to_lowered_stmt("rvar_bounds_split_reorder.html", HTML);

        call_count = 0;
        f.realize(32);
        if (call_count != 32) {
            printf("Error in split+reorder! call_counter called %d instead of %d times.", call_count, 32);
            return -1;
        }
    }
*/
    printf("Success!");
    return 0;
}
