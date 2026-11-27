#include <rfl.hpp>

struct CollidesWithPrimary {
  rfl::Alias<"other", int> first;
  int other;
};

int main() {
  rfl::Generic::Object obj;
  obj["first"] = 1;
  obj["other"] = 2;
  auto res = rfl::from_generic<CollidesWithPrimary>(rfl::Generic(obj));
  (void)res;
  return 0;
}
