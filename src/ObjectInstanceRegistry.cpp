#include "ObjectInstanceRegistry.h"

#include "Error.h"

#if __cplusplus > 199711L
namespace Halide {
namespace Internal {

std::mutex ObjectInstanceRegistry::mutex;
ObjectInstanceRegistry ObjectInstanceRegistry::registry;

void ObjectInstanceRegistry::register_instance(void *this_ptr, size_t size, Kind kind, void *subject_ptr) {
    std::lock_guard<std::mutex> lock(mutex);

    auto result = registry.instances.emplace((uintptr_t)this_ptr, InstanceInfo(size, kind, subject_ptr));
    internal_assert(result.second); // true in the second member indicates there was no preexisting element
}

void ObjectInstanceRegistry::update_instance_size(void *this_ptr, size_t size) {
    std::lock_guard<std::mutex> lock(mutex);

    auto it = registry.instances.find((uintptr_t)this_ptr);
    internal_assert(it != registry.instances.end());
    internal_assert(it->second.size == 0 || it->second.size == size);
    it->second.size = size;
}

void ObjectInstanceRegistry::unregister_instance(void *this_ptr) {
    std::lock_guard<std::mutex> lock(mutex);

    size_t num_erased = registry.instances.erase((uintptr_t)this_ptr);
    internal_assert(num_erased == 1);
}

std::vector<void *> ObjectInstanceRegistry::instances_in_range(void *start, size_t size, Kind kind) {
    std::vector<void *> results;
    std::lock_guard<std::mutex> lock(mutex);

    auto it = registry.instances.lower_bound((uintptr_t)start);

    uintptr_t limit_ptr = ((uintptr_t)start) + size;
    while (it != registry.instances.end() && it->first < limit_ptr) {
      if (it->second.kind == kind) {
        results.push_back(it->second.subject_ptr);
      }

      if (it->first > (uintptr_t)start && it->second.size != 0) {
        // Skip over containers that we enclose
        it = registry.instances.lower_bound(it->first + it->second.size);
      } else {
        it++;
      }
    }

    return results;
}

}  // namespace Internal
}  // namespace Halide

#endif  // __cplusplus > 199711L
