#include <rfl.hpp>
#include <string>

struct InnerA {
  using Tag = rfl::Literal<"a">;
  std::string x;
};

// Same discriminator "type" as outer TaggedUnion -> must fail at compile time.
using BadInner = rfl::TaggedUnion<"type", InnerA>;

struct BadEvent {
  using Tag = rfl::Literal<"event">;
  rfl::Flatten<BadInner> payload;
};

struct Response {
  using Tag = rfl::Literal<"response">;
  std::string command;
};

using BadOuter = rfl::TaggedUnion<"type", Response, BadEvent>;

void trigger_compile_error() {
  const BadOuter msg = BadEvent{.payload = BadInner(InnerA{.x = "hi"})};
  (void)rfl::to_generic(msg);
}
