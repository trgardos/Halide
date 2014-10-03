// Generator requires C++11
#if __cplusplus > 199711L

#include "Generator.h"

namespace Halide {

using Internal::GeneratorParamBase;
using Internal::ObjectInstanceRegistry;

namespace Internal {

GeneratorParamBase::GeneratorParamBase(const std::string& name) : name(name) {
    ObjectInstanceRegistry::register_instance(this, 0, ObjectInstanceRegistry::GeneratorParam, this);
}

GeneratorParamBase::~GeneratorParamBase() {
    ObjectInstanceRegistry::unregister_instance(this);
}

GeneratorRegistry GeneratorRegistry::registry;

void GeneratorRegistry::register_factory(const std::string& name,
                                 std::unique_ptr<GeneratorFactory> factory) {
  internal_assert(registry.factories.find(name) == registry.factories.end()) << "Duplicate Generator name: " << name;
  registry.factories[name] = std::move(factory);
}

void GeneratorRegistry::unregister_factory(const std::string& name) {
  internal_assert(registry.factories.find(name) != registry.factories.end()) << "Generator not found: " << name;
  registry.factories.erase(name);
}

std::unique_ptr<Generator> GeneratorRegistry::create(const std::string& name, const GeneratorArguments& args) {
  auto it = registry.factories.find(name);
  if (it != registry.factories.end()) {
    return it->second->create(args);
  }
  return nullptr;
}

std::vector<std::string> GeneratorRegistry::enumerate() {
  std::vector<std::string> result;
  for (auto it = registry.factories.begin();
       it != registry.factories.end(); ++it) {
    result.push_back(it->first);
  }
  return result;
}

void GeneratorFactory::find_params(Generator* generator, size_t size) {
    Internal::ObjectInstanceRegistry::update_instance_size(generator, size);
    generator->find_params(size);
}

}  // namespace Internal

Generator::Generator() {
    // We don't know the size of the subclass being created, so pass 0;
    // the GeneratorFactory will call update_instance_size() for us.
    ObjectInstanceRegistry::register_instance(this, 0, ObjectInstanceRegistry::Generator, this);
}

Generator::~Generator() {
    ObjectInstanceRegistry::unregister_instance(this);
}

void Generator::find_params(size_t size) {
    std::vector<void *> v = ObjectInstanceRegistry::instances_in_range(this, size, ObjectInstanceRegistry::GeneratorParam);
    generator_params.clear();
    for (int i = 0; i < v.size(); ++i) {
        GeneratorParamBase* param = static_cast<GeneratorParamBase*>(v[i]);
        internal_assert(param != nullptr);
        user_assert(generator_params.find(param->name) == generator_params.end()) << "Duplicate GeneratorParam name: " << param->name;
        generator_params[param->name] = param;
    }
}

void Generator::set_generator_arguments(const GeneratorArguments& args) {
    internal_assert(!generator_params.empty()) << "find_params() must be called before set_generator_arguments";
    for (auto key_value : args) {
        const std::string& key = key_value.first;
        const std::string& value = key_value.second;
        auto param = generator_params.find(key);
        user_assert(param != generator_params.end()) << "Generator has no GeneratorParam named: " << key;
        param->second->set_from_string(value);
    }
}

}  // namespace Halide

#endif  // __cplusplus > 199711L
