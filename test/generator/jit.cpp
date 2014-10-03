#include <stdio.h>
#include <Halide.h>

using std::unique_ptr;
using namespace Halide;

enum SomeEnum {
    kFoo,
    kBar
};

class SimpleGenerator : public Generator {
public:
    Func build() const override {
        Var x, y, c;
        Func f, g, h;

        f(x, y) = max(x, y);
        g(x, y, c) = cast<int32_t>(f(x, y) * c * factor);

        g.bound(c, 0, bound);

        return g;
    }
private:
    // GeneratorParams can be float or ints: default, min, max
    // (note that min and max must always be specified)
    GeneratorParam<float> factor{"factor", 1, 0, 100};
    GeneratorParam<int> bound{"bound", 3, 1, 4};
    // or enums: default, name->value map
    GeneratorParam<SomeEnum> enummy{"enummy", kFoo, {{"foo", kFoo}, {"bar", kBar}}};
    // or bools: default
    GeneratorParam<bool> flag{"flag", true};
};

auto my_gen = register_generator<SimpleGenerator>("simple");

void verify(const Image<int>& img, float factor, int bound) {
    for (int i = 0; i < 32; i++) {
        for (int j = 0; j < 32; j++) {
            for (int c = 0; c < bound; c++) {
                if (img(i, j, c) != (int32_t)(factor*c*(i > j ? i : j))) {
                    printf("img[%d, %d, %d] = %d\n", i, j, c, img(i, j, c));
                    exit(-1);
                }
            }
        }
    }
}

int main(int argc, char **argv) {
    {
        // Create a Generator by name, and set its GeneratorParams by a map
        // of key-value pairs. Values are looked up by name, order doesn't matter.
        // GeneratorParams not set here retain their existing values. Note that
        // all Generators have a "target" GeneratorParam, which is just
        // a Halide::Target set using the normal syntax.
        unique_ptr<Generator> gen = make_generator("simple",
            {{"factor", "2.3"},
             {"bound", "3"},
             {"target", "x86-64"},
             {"enummy", "foo"},
             {"flag", "false"}});
        Image<int> img = gen->build().realize(32, 32, 3, gen->get_target());
        verify(img, 2.3, 3);
    }
    {
        // You can also set the GeneratorParams after creation by calling
        // set_generator_arguments explicitly.
        unique_ptr<Generator> gen = make_generator("simple");

        gen->set_generator_arguments({{"factor", "2.9"}});
        Image<int> img = gen->build().realize(32, 32, 3);
        verify(img, 2.9, 3);

        gen->set_generator_arguments({{"factor", "0.1"}, {"bound", "4"}});
        Image<int> img2 = gen->build().realize(32, 32, 4);
        verify(img2, 0.1, 4);

        // Setting non-existent GeneratorParams will fail with a user_assert.
        // gen->set_generator_arguments({{"unknown_name", "0.1"}});

        // Setting GeneratorParams to values that can't be properly parsed
        // into the correct type will fail with a user_assert.
        // gen->set_generator_arguments({{"factor", "this is not a number"}});
        // gen->set_generator_arguments({{"bound", "neither is this"}});
        // gen->set_generator_arguments({{"enummy", "not_in_the_enum_map"}});
        // gen->set_generator_arguments({{"flag", "maybe"}});
        // gen->set_generator_arguments({{"target", "6502-8"}});
    }
    {
        // make_generator() returns a unique_ptr so we can just do this inline if we like,
        // and not worry about the Generator getting leaked.
        Image<int> img = make_generator("simple", {{"factor", "3.14"}})->build().realize(32, 32, 3);
        verify(img, 3.14, 3);
    }
    {
        // It's legal to skip GeneratorRegistry and just create a Generator directly
        // (but you can't set the GeneratorParams if you do -- you're stuck with the defaults)
        unique_ptr<Generator> gen(new SimpleGenerator());
        // This will fail with a runtime assertion
        // gen->set_generator_arguments({{"factor", "2.9"}});
        Image<int> img = gen->build().realize(32, 32, 3);
        verify(img, 1, 3);
    }
    {
        // Want to be ultra-terse? Allocate on the stack. (But again,
        // you can't set the GeneratorParams this way. )
        Image<int> img = SimpleGenerator().build().realize(32, 32, 3);
        verify(img, 1, 3);
    }

    printf("Success!\n");
    return 0;
}
