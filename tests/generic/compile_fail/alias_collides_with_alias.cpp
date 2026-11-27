#include <rfl.hpp>

struct CollidesWithAlias {
  rfl::Alias<"legacy", int> first;
  rfl::Alias<"legacy", int> second;
};

int main() {
  rfl::Generic::Object obj;
  obj["first"] = 1;
  obj["second"] = 2;
  auto res = rfl::from_generic<CollidesWithAlias>(rfl::Generic(obj));
  (void)res;
  return 0;
}
