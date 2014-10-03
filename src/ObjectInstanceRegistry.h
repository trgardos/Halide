#ifndef HALIDE_OBJECT_INSTANCE_REGISTRY_H
#define HALIDE_OBJECT_INSTANCE_REGISTRY_H

#if __cplusplus > 199711L

/** \file
 *
 * Provides a single global registry of Generators, GeneratorParams,
 * and Params indexed by this pointer. This is used for finding the
 * parameters inside of a Generator.
 */

#include <map>
#include <mutex>
#include <vector>

namespace Halide {
namespace Internal {

class ObjectInstanceRegistry {
public:
    enum Kind {
        Generator,
        GeneratorParam,
        FilterParam,
    };

    static void register_instance(void *this_ptr, size_t size, Kind kind, void *subject_ptr);
    static void update_instance_size(void *this_ptr, size_t size);
    static void unregister_instance(void *this_ptr);

    /** Returns the list of subject pointers for objects that have
     *   been directly registered within the given range. If there is
     *   another containing object inside the range, instances within
     *   that object are skipped.
     */
    static std::vector<void *> instances_in_range(void *start, size_t size, Kind kind);

private:
    static std::mutex mutex;
    static ObjectInstanceRegistry registry;

    struct InstanceInfo {
        void *subject_ptr; // May be different from the this_ptr in the key
        size_t size; // May be 0 for params
        Kind kind;

        InstanceInfo(size_t size, Kind kind, void *subject_ptr)
            : subject_ptr(subject_ptr), size(size), kind(kind) {
        }
    };

    std::map<uintptr_t, InstanceInfo> instances;

    ObjectInstanceRegistry() { }
    ObjectInstanceRegistry(ObjectInstanceRegistry &rhs) = delete;
};

}  // namespace Internal
}  // namespace Halide

#endif  // __cplusplus > 199711L

#endif // HALIDE_OBJECT_INSTANCE_REGISTRY_H
