#include <rfl.hpp>
#include <string>

struct InnerA {
  using Tag = rfl::Literal<"a">;
  std::string seq;  // Collides with outer sibling field "seq"
};

using Inner = rfl::TaggedUnion<"event", InnerA>;

struct BadEnvelope {
  int seq;
  rfl::Flatten<Inner> body;
};

void trigger_compile_error() {
  const BadEnvelope env{.seq = 1, .body = Inner(InnerA{.seq = "clash"})};
  (void)rfl::to_generic(env);
}
