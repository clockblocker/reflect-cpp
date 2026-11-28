#include <gtest/gtest.h>

#include <optional>
#include <rfl.hpp>
#include <string>

#include "write_and_read.hpp"

namespace test_nested_tagged_union {

struct WaveformsLoaded {
  using Tag = rfl::Literal<"waveforms_loaded">;
  std::string source;
};

struct GotoDeclaration {
  using Tag = rfl::Literal<"goto_declaration">;
  std::string variable;
};

using S2CEvent = rfl::TaggedUnion<"event", WaveformsLoaded, GotoDeclaration>;

struct Response {
  using Tag = rfl::Literal<"response">;
  std::string command;
};

struct Event {
  using Tag = rfl::Literal<"event">;
  rfl::Flatten<S2CEvent> payload;
};

using S2CMessage = rfl::TaggedUnion<"type", Response, Event>;

struct Envelope {
  int seq;
  rfl::Flatten<S2CEvent> body;
  std::optional<std::string> note;
};

struct DirectEvent
    : rfl::TaggedUnion<"event", WaveformsLoaded, GotoDeclaration> {
  using Tag = rfl::Literal<"event">;
  using TaggedUnion::TaggedUnion;
};

using DirectS2CMessage = rfl::TaggedUnion<"type", Response, DirectEvent>;

struct L3Leaf {
  using Tag = rfl::Literal<"leaf">;
  int val;
};

using Level3 = rfl::TaggedUnion<"subkind", L3Leaf>;

struct L2Branch {
  using Tag = rfl::Literal<"branch">;
  std::string tag_info;
  rfl::Flatten<Level3> inner;
};

using Level2 = rfl::TaggedUnion<"kind", L2Branch>;

struct L1Root {
  using Tag = rfl::Literal<"root">;
  int seq;
  rfl::Flatten<Level2> body;
};

using Level1 = rfl::TaggedUnion<"type", Response, L1Root>;

TEST(generic, test_nested_tagged_union_wcp_flat_generic) {
  const S2CMessage msg =
      Event{.payload = S2CEvent(WaveformsLoaded{.source = "foo.vcd"})};

  const auto g = rfl::to_generic(msg);
  const auto obj = g.to_object().value();
  EXPECT_EQ(obj.size(), 3u);
  EXPECT_EQ(obj.get("type").value().to_string().value(), "event");
  EXPECT_EQ(obj.get("event").value().to_string().value(), "waveforms_loaded");
  EXPECT_EQ(obj.get("source").value().to_string().value(), "foo.vcd");

  const auto roundtripped =
      rfl::from_generic<S2CMessage, rfl::NoExtraFields>(g);
  ASSERT_TRUE(roundtripped.has_value()) << roundtripped.error().what();
  const auto* ev = rfl::get_if<Event>(&roundtripped->variant());
  ASSERT_NE(ev, nullptr);
  const auto* wf = rfl::get_if<WaveformsLoaded>(&ev->payload->variant());
  ASSERT_NE(wf, nullptr);
  EXPECT_EQ(wf->source, "foo.vcd");

  write_and_read(msg);
  write_and_read<rfl::NoExtraFields>(msg);
}

TEST(generic, test_nested_tagged_union_sibling_fields) {
  const Envelope env{
      .seq = 10,
      .body = S2CEvent(GotoDeclaration{.variable = "dut.sig"}),
      .note = "memo"};
  write_and_read(env);
  write_and_read<rfl::NoExtraFields>(env);
}

TEST(generic, test_nested_tagged_union_direct_subclass) {
  const DirectS2CMessage msg =
      DirectEvent(WaveformsLoaded{.source = "sub.vcd"});
  write_and_read(msg);
  write_and_read<rfl::NoExtraFields>(msg);
}

TEST(generic, test_nested_tagged_union_three_levels) {
  const Level1 msg = L1Root{
      .seq = 5,
      .body = Level2(
          L2Branch{.tag_info = "mid", .inner = Level3(L3Leaf{.val = 99})})};
  const auto g = rfl::to_generic(msg);
  const auto obj = g.to_object().value();
  EXPECT_EQ(obj.size(), 6u);
  EXPECT_EQ(obj.get("type").value().to_string().value(), "root");
  EXPECT_EQ(obj.get("seq").value().to_int().value(), 5);
  EXPECT_EQ(obj.get("kind").value().to_string().value(), "branch");
  EXPECT_EQ(obj.get("tag_info").value().to_string().value(), "mid");
  EXPECT_EQ(obj.get("subkind").value().to_string().value(), "leaf");
  EXPECT_EQ(obj.get("val").value().to_int().value(), 99);

  write_and_read<rfl::NoExtraFields>(msg);
}

TEST(generic, test_nested_tagged_union_errors) {
  rfl::Generic::Object bad_extra;
  bad_extra["type"] = "event";
  bad_extra["event"] = "waveforms_loaded";
  bad_extra["source"] = "a.vcd";
  bad_extra["unexpected"] = 1;
  const auto res_extra =
      rfl::from_generic<S2CMessage, rfl::NoExtraFields>(rfl::Generic(bad_extra));
  ASSERT_FALSE(res_extra.has_value());
  EXPECT_NE(res_extra.error().what().find("unexpected"), std::string::npos);

  rfl::Generic::Object bad_tag;
  bad_tag["type"] = "event";
  bad_tag["event"] = "does_not_exist";
  const auto res_tag = rfl::from_generic<S2CMessage>(rfl::Generic(bad_tag));
  ASSERT_FALSE(res_tag.has_value());
  EXPECT_NE(res_tag.error().what().find("does_not_exist"), std::string::npos);
}

}  // namespace test_nested_tagged_union
