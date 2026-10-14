// Explicit hidden validation driver; djultra64.
#pragma once
#include <memory>
#include <string>
#include <cstdint>
namespace clay::testing {
class InputScript {
 struct Impl;std::unique_ptr<Impl> impl_;
public:
 explicit InputScript(const std::string& path);
 ~InputScript();
 void tick();
 int32_t instance_id(unsigned player=0) const;
};
}
