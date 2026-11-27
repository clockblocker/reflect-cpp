#include <gtest/gtest.h>

#include <memory>
#include <optional>
#include <rfl.hpp>
#include <rfl/json.hpp>
#include <string>
#include <vector>

#include "write_and_read.hpp"

namespace test_nested_tagged_union {

// ============================================================================
// Pattern A: WCP S2CMessage / S2CEvent (from hudson-trading/slang-server)
// ============================================================================

struct WaveformsLoaded {
  using Tag = rfl::Literal<"waveforms_loaded">;
  std::string source;
};

struct GotoDeclaration {
  using Tag = rfl::Literal<"goto_declaration">;
  std::string variable;
};

struct AddDrivers {
  using Tag = rfl::Literal<"add_drivers">;
  std::string variable;
};

struct AddLoads {
  using Tag = rfl::Literal<"add_loads">;
  std::string variable;
};

using S2CEvent = rfl::TaggedUnion<"event", WaveformsLoaded, GotoDeclaration,
                                  AddDrivers, AddLoads>;

struct Response {
  using Tag = rfl::Literal<"response">;
  std::string command;
};

struct Event {
  using Tag = rfl::Literal<"event">;
  rfl::Flatten<S2CEvent> payload;
};

using S2CMessage = rfl::TaggedUnion<"type", Response, Event>;

TEST(json, test_nested_tagged_union_wcp_roundtrip) {
  const S2CMessage resp = Response{.command = "load"};
  write_and_read(resp, R"({"type":"response","command":"load"})");
  write_and_read<rfl::NoExtraFields>(resp,
                                     R"({"type":"response","command":"load"})");

  const S2CMessage wf =
      Event{.payload = S2CEvent(WaveformsLoaded{.source = "foo.vcd"})};
  write_and_read(
      wf, R"({"type":"event","event":"waveforms_loaded","source":"foo.vcd"})");
  write_and_read<rfl::NoExtraFields>(
      wf, R"({"type":"event","event":"waveforms_loaded","source":"foo.vcd"})");

  const S2CMessage gd =
      Event{.payload = S2CEvent(GotoDeclaration{.variable = "top.clk"})};
  write_and_read<rfl::NoExtraFields>(
      gd, R"({"type":"event","event":"goto_declaration","variable":"top.clk"})");

  const S2CMessage ad =
      Event{.payload = S2CEvent(AddDrivers{.variable = "top.sig"})};
  write_and_read<rfl::NoExtraFields>(
      ad, R"({"type":"event","event":"add_drivers","variable":"top.sig"})");

  const S2CMessage al =
      Event{.payload = S2CEvent(AddLoads{.variable = "top.sig"})};
  write_and_read<rfl::NoExtraFields>(
      al, R"({"type":"event","event":"add_loads","variable":"top.sig"})");
}

TEST(json, test_nested_tagged_union_wcp_out_of_order_keys) {
  const auto res = rfl::json::read<S2CMessage, rfl::NoExtraFields>(
      R"({"source":"bar.vcd","event":"waveforms_loaded","type":"event"})");
  ASSERT_TRUE(res.has_value()) << res.error().what();
  const auto* ev = rfl::get_if<Event>(&res->variant());
  ASSERT_NE(ev, nullptr);
  const auto* wf = rfl::get_if<WaveformsLoaded>(&ev->payload->variant());
  ASSERT_NE(wf, nullptr);
  EXPECT_EQ(wf->source, "bar.vcd");
}

// ============================================================================
// Plain struct with sibling fields + rfl::Flatten<TaggedUnion>
// ============================================================================

struct Envelope {
  int seq;
  rfl::Flatten<S2CEvent> body;
  std::optional<std::string> note;
};

TEST(json, test_nested_tagged_union_sibling_fields) {
  const Envelope env1{
      .seq = 7,
      .body = S2CEvent(WaveformsLoaded{.source = "sim.vcd"}),
      .note = "ok"};
  write_and_read(
      env1,
      R"({"seq":7,"event":"waveforms_loaded","source":"sim.vcd","note":"ok"})");
  write_and_read<rfl::NoExtraFields>(
      env1,
      R"({"seq":7,"event":"waveforms_loaded","source":"sim.vcd","note":"ok"})");

  const Envelope env2{
      .seq = 8,
      .body = S2CEvent(GotoDeclaration{.variable = "dut.rst"}),
      .note = std::nullopt};
  write_and_read<rfl::NoExtraFields>(
      env2, R"({"seq":8,"event":"goto_declaration","variable":"dut.rst"})");
}

// rfl::Field syntax + rfl::Flatten<TaggedUnion>
struct FieldEnvelope {
  using Tag = rfl::Literal<"field_event">;
  rfl::Field<"seq", int> seq;
  rfl::Flatten<S2CEvent> body;
};

using FieldOuterMessage = rfl::TaggedUnion<"type", Response, FieldEnvelope>;

TEST(json, test_nested_tagged_union_rfl_field_syntax) {
  const FieldEnvelope fe{
      .seq = 42, .body = S2CEvent(AddDrivers{.variable = "top.bus"})};
  write_and_read<rfl::NoExtraFields>(
      fe, R"({"seq":42,"event":"add_drivers","variable":"top.bus"})");

  const FieldOuterMessage msg = fe;
  write_and_read<rfl::NoExtraFields>(
      msg,
      R"({"type":"field_event","seq":42,"event":"add_drivers","variable":"top.bus"})");
}

// Transitive rfl::Flatten<StructContainingFlattenTU>
struct InnerHolder {
  std::string origin;
  rfl::Flatten<S2CEvent> body;
};

struct OuterHolder {
  int id;
  rfl::Flatten<InnerHolder> inner;
};

TEST(json, test_nested_tagged_union_transitive_flatten) {
  const OuterHolder h{
      .id = 99,
      .inner = InnerHolder{
          .origin = "viewer",
          .body = S2CEvent(WaveformsLoaded{.source = "trace.fst"})}};
  write_and_read<rfl::NoExtraFields>(
      h,
      R"({"id":99,"origin":"viewer","event":"waveforms_loaded","source":"trace.fst"})");
}

// ============================================================================
// Pattern B: Direct nested TaggedUnion (subclass with Tag + explicit tag field)
// ============================================================================

struct DirectEvent : rfl::TaggedUnion<"event", WaveformsLoaded, GotoDeclaration> {
  using Tag = rfl::Literal<"event">;
  using TaggedUnion::TaggedUnion;
};

using DirectS2CMessage = rfl::TaggedUnion<"type", Response, DirectEvent>;

TEST(json, test_nested_tagged_union_direct_subclass) {
  const DirectEvent standalone = WaveformsLoaded{.source = "direct.vcd"};
  write_and_read<rfl::NoExtraFields>(
      standalone, R"({"event":"waveforms_loaded","source":"direct.vcd"})");

  const DirectS2CMessage msg =
      DirectEvent(GotoDeclaration{.variable = "u0.data"});
  write_and_read<rfl::NoExtraFields>(
      msg, R"({"type":"event","event":"goto_declaration","variable":"u0.data"})");

  const DirectS2CMessage resp = Response{.command = "ping"};
  write_and_read<rfl::NoExtraFields>(resp,
                                     R"({"type":"response","command":"ping"})");
}

// Direct TaggedUnion where inner alternatives carry explicit outer tag field
struct ExplicitWaveformsLoaded {
  using Tag = rfl::Literal<"waveforms_loaded">;
  rfl::Literal<"event"> type;
  std::string source;
};

struct ExplicitGotoDeclaration {
  using Tag = rfl::Literal<"goto_declaration">;
  rfl::Literal<"event"> type;
  std::string variable;
};

using ExplicitInnerEvent =
    rfl::TaggedUnion<"event", ExplicitWaveformsLoaded, ExplicitGotoDeclaration>;
using ExplicitOuterMessage =
    rfl::TaggedUnion<"type", Response, ExplicitInnerEvent>;

TEST(json, test_nested_tagged_union_direct_with_explicit_outer_field) {
  const ExplicitOuterMessage msg =
      ExplicitInnerEvent(ExplicitWaveformsLoaded{.source = "exp.vcd"});
  write_and_read<rfl::NoExtraFields>(
      msg, R"({"event":"waveforms_loaded","type":"event","source":"exp.vcd"})");
}

// Non-flattened TaggedUnion as a struct field inside an outer TaggedUnion
struct NestedFieldEvent {
  using Tag = rfl::Literal<"nested_event">;
  S2CEvent payload;
};

using FieldNestedMessage = rfl::TaggedUnion<"type", Response, NestedFieldEvent>;

TEST(json, test_nested_tagged_union_as_regular_field) {
  const FieldNestedMessage msg = NestedFieldEvent{
      .payload = S2CEvent(WaveformsLoaded{.source = "nested.vcd"})};
  write_and_read<rfl::NoExtraFields>(
      msg,
      R"({"type":"nested_event","payload":{"event":"waveforms_loaded","source":"nested.vcd"}})");
}

// ============================================================================
// 3-Level Nested TaggedUnion with sibling fields at multiple levels
// ============================================================================

struct L3Alpha {
  using Tag = rfl::Literal<"alpha">;
  int value;
};

struct L3Beta {
  using Tag = rfl::Literal<"beta">;
  std::string label;
};

using Level3 = rfl::TaggedUnion<"subkind", L3Alpha, L3Beta>;

struct L2Simple {
  using Tag = rfl::Literal<"simple">;
  bool flag;
};

struct L2Complex {
  using Tag = rfl::Literal<"complex">;
  std::string scope;
  rfl::Flatten<Level3> detail;
};

using Level2 = rfl::TaggedUnion<"kind", L2Simple, L2Complex>;

struct L1Other {
  using Tag = rfl::Literal<"other">;
  int code;
};

struct L1Envelope {
  using Tag = rfl::Literal<"envelope">;
  int seq;
  rfl::Flatten<Level2> inner;
};

using Level1 = rfl::TaggedUnion<"type", L1Other, L1Envelope>;

TEST(json, test_nested_tagged_union_three_levels) {
  const Level1 msg = L1Envelope{
      .seq = 12,
      .inner = Level2(L2Complex{.scope = "cpu.alu",
                                .detail = Level3(L3Alpha{.value = 1337})})};
  write_and_read<rfl::NoExtraFields>(
      msg,
      R"({"type":"envelope","seq":12,"kind":"complex","scope":"cpu.alu","subkind":"alpha","value":1337})");

  const Level1 msg2 = L1Envelope{
      .seq = 13,
      .inner = Level2(L2Complex{.scope = "cpu.fpu",
                                .detail = Level3(L3Beta{.label = "nan"})})};
  write_and_read<rfl::NoExtraFields>(
      msg2,
      R"({"type":"envelope","seq":13,"kind":"complex","scope":"cpu.fpu","subkind":"beta","label":"nan"})");

  const Level1 msg3 =
      L1Envelope{.seq = 14, .inner = Level2(L2Simple{.flag = true})};
  write_and_read<rfl::NoExtraFields>(
      msg3, R"({"type":"envelope","seq":14,"kind":"simple","flag":true})");
}

// ============================================================================
// Multiple sibling rfl::Flatten<TaggedUnion> in one struct
// ============================================================================

struct CircleShape {
  using Tag = rfl::Literal<"circle">;
  double radius;
};

struct RectShape {
  using Tag = rfl::Literal<"rect">;
  double width;
  double height;
};

using ShapeTU = rfl::TaggedUnion<"shape", CircleShape, RectShape>;

struct SolidColor {
  using Tag = rfl::Literal<"solid">;
  std::string hex;
};

struct GradientColor {
  using Tag = rfl::Literal<"gradient">;
  std::string from;
  std::string to;
};

using ColorTU = rfl::TaggedUnion<"color", SolidColor, GradientColor>;

struct ColoredShape {
  int id;
  rfl::Flatten<ShapeTU> shape;
  rfl::Flatten<ColorTU> color;
};

TEST(json, test_nested_tagged_union_multiple_sibling_flattened_tus) {
  const ColoredShape cs{
      .id = 5,
      .shape = ShapeTU(CircleShape{.radius = 2.5}),
      .color = ColorTU(GradientColor{.from = "#000", .to = "#fff"})};
  write_and_read<rfl::NoExtraFields>(
      cs,
      R"({"id":5,"shape":"circle","radius":2.5,"color":"gradient","from":"#000","to":"#fff"})");
}

// ============================================================================
// Empty alternative struct, DefaultVal, ExtraFields, NoFieldNames,
// SnakeCaseToCamelCase, move-only types
// ============================================================================

struct Ping {
  using Tag = rfl::Literal<"ping">;
};

struct Pong {
  using Tag = rfl::Literal<"pong">;
  int latency_ms;
};

using ControlEvent = rfl::TaggedUnion<"event", Ping, Pong>;

struct ControlMsg {
  using Tag = rfl::Literal<"control">;
  rfl::Flatten<ControlEvent> payload;
};

using ControlEnvelope = rfl::TaggedUnion<"type", Response, ControlMsg>;

TEST(json, test_nested_tagged_union_empty_alternative) {
  const ControlEnvelope ping = ControlMsg{.payload = ControlEvent(Ping{})};
  write_and_read<rfl::NoExtraFields>(ping,
                                     R"({"type":"control","event":"ping"})");

  const ControlEnvelope pong =
      ControlMsg{.payload = ControlEvent(Pong{.latency_ms = 4})};
  write_and_read<rfl::NoExtraFields>(
      pong, R"({"type":"control","event":"pong","latency_ms":4})");
}

TEST(json, test_nested_tagged_union_no_field_names) {
  const S2CMessage wf =
      Event{.payload = S2CEvent(WaveformsLoaded{.source = "arr.vcd"})};
  write_and_read<rfl::NoFieldNames>(wf,
                                    R"(["event","waveforms_loaded","arr.vcd"])");

  const Envelope env{
      .seq = 3,
      .body = S2CEvent(GotoDeclaration{.variable = "x"}),
      .note = "hi"};
  write_and_read<rfl::NoFieldNames>(env,
                                    R"([3,"goto_declaration","x","hi"])");
}

TEST(json, test_nested_tagged_union_snake_case_to_camel_case) {
  const ControlEnvelope pong =
      ControlMsg{.payload = ControlEvent(Pong{.latency_ms = 9})};
  write_and_read<rfl::SnakeCaseToCamelCase, rfl::NoExtraFields>(
      pong, R"({"type":"control","event":"pong","latencyMs":9})");
}

struct AltWithDefault {
  using Tag = rfl::Literal<"with_default">;
  std::string name;
  rfl::DefaultVal<int> retries = 3;
};

struct AltRequired {
  using Tag = rfl::Literal<"required_only">;
  std::string path;
};

using DefaultableInner = rfl::TaggedUnion<"event", AltRequired, AltWithDefault>;

struct DefaultableEnvelope {
  rfl::DefaultVal<int> seq = 100;
  rfl::Flatten<DefaultableInner> body;
};

TEST(json, test_nested_tagged_union_default_val) {
  const auto res1 = rfl::json::read<DefaultableEnvelope, rfl::NoExtraFields>(
      R"({"event":"with_default","name":"job1"})");
  ASSERT_TRUE(res1.has_value()) << res1.error().what();
  EXPECT_EQ(res1->seq.value(), 100);
  const auto* alt = rfl::get_if<AltWithDefault>(&res1->body->variant());
  ASSERT_NE(alt, nullptr);
  EXPECT_EQ(alt->name, "job1");
  EXPECT_EQ(alt->retries.value(), 3);

  // Missing required field on AltRequired must still fail when DefaultIfMissing
  // is NOT active, even though AltWithDefault has DefaultVal!
  const auto res_missing = rfl::json::read<DefaultableEnvelope>(
      R"({"event":"required_only"})");
  EXPECT_FALSE(res_missing.has_value());
}

struct ExtraEnvelope {
  int seq;
  rfl::Flatten<S2CEvent> body;
  rfl::ExtraFields<rfl::Generic> extras;
};

TEST(json, test_nested_tagged_union_extra_fields) {
  const auto res = rfl::json::read<ExtraEnvelope>(
      R"({"seq":1,"event":"waveforms_loaded","source":"a.vcd","custom_a":10,"custom_b":"hi"})");
  ASSERT_TRUE(res.has_value()) << res.error().what();
  EXPECT_EQ(res->seq, 1);
  EXPECT_EQ(res->extras.size(), 2u);
  EXPECT_EQ(res->extras.get("custom_a").value().to_int().value(), 10);
  EXPECT_EQ(res->extras.get("custom_b").value().to_string().value(), "hi");
  const auto written = rfl::json::write(*res);
  const auto reread = rfl::json::read<ExtraEnvelope>(written);
  ASSERT_TRUE(reread.has_value()) << reread.error().what();
  EXPECT_EQ(reread->extras.size(), 2u);
}

struct MoveOnlyAlt {
  using Tag = rfl::Literal<"move_only">;
  std::unique_ptr<int> ptr;
};

using MoveOnlyInner = rfl::TaggedUnion<"kind", MoveOnlyAlt>;

struct MoveOnlyOuter {
  using Tag = rfl::Literal<"outer">;
  rfl::Flatten<MoveOnlyInner> body;
};

using MoveOnlyMsg = rfl::TaggedUnion<"type", MoveOnlyOuter>;

TEST(json, test_nested_tagged_union_move_only) {
  auto res = rfl::json::read<MoveOnlyMsg, rfl::NoExtraFields>(
      R"({"type":"outer","kind":"move_only","ptr":42})");
  ASSERT_TRUE(res.has_value()) << res.error().what();
  const auto* outer = rfl::get_if<MoveOnlyOuter>(&res->variant());
  ASSERT_NE(outer, nullptr);
  const auto* alt = rfl::get_if<MoveOnlyAlt>(&outer->body->variant());
  ASSERT_NE(alt, nullptr);
  ASSERT_NE(alt->ptr, nullptr);
  EXPECT_EQ(*alt->ptr, 42);
  EXPECT_EQ(rfl::json::write(*res),
            R"({"type":"outer","kind":"move_only","ptr":42})");
}

// ============================================================================
// JSON Schema (rfl::json::to_schema)
// ============================================================================

TEST(json, test_nested_tagged_union_to_schema_wcp) {
  const auto schema_str =
      rfl::json::to_schema<S2CMessage, rfl::NoExtraFields>();
  const auto parsed = rfl::json::read<rfl::Generic>(schema_str);
  ASSERT_TRUE(parsed.has_value()) << parsed.error().what();

  const auto root = parsed->to_object().value();
  const auto defs = root.get("$defs").value().to_object().value();

  // Event__tagged should be an anyOf with 4 merged flat object schemas
  const auto event_def =
      defs.get("test_nested_tagged_union__Event__tagged")
          .value()
          .to_object()
          .value();
  const auto any_of = event_def.get("anyOf").value().to_array().value();
  ASSERT_EQ(any_of.size(), 4u);

  const auto wf_branch = any_of[0].to_object().value();
  EXPECT_EQ(wf_branch.get("type").value().to_string().value(), "object");
  EXPECT_EQ(wf_branch.get("additionalProperties").value().to_bool().value(),
            false);

  const auto props = wf_branch.get("properties").value().to_object().value();
  EXPECT_EQ(props.get("type")
                .value()
                .to_object()
                .value()
                .get("enum")
                .value()
                .to_array()
                .value()[0]
                .to_string()
                .value(),
            "event");
  EXPECT_EQ(props.get("event")
                .value()
                .to_object()
                .value()
                .get("enum")
                .value()
                .to_array()
                .value()[0]
                .to_string()
                .value(),
            "waveforms_loaded");
  EXPECT_EQ(props.get("source")
                .value()
                .to_object()
                .value()
                .get("type")
                .value()
                .to_string()
                .value(),
            "string");

  const auto req = wf_branch.get("required").value().to_array().value();
  EXPECT_EQ(req.size(), 3u);
}

TEST(json, test_nested_tagged_union_to_schema_envelope_and_three_levels) {
  const auto env_schema = rfl::json::to_schema<Envelope, rfl::NoExtraFields>();
  const auto env_parsed = rfl::json::read<rfl::Generic>(env_schema);
  ASSERT_TRUE(env_parsed.has_value()) << env_parsed.error().what();
  const auto env_defs =
      env_parsed->to_object().value().get("$defs").value().to_object().value();
  const auto env_any_of = env_defs.get("test_nested_tagged_union__Envelope")
                              .value()
                              .to_object()
                              .value()
                              .get("anyOf")
                              .value()
                              .to_array()
                              .value();
  ASSERT_EQ(env_any_of.size(), 4u);
  const auto first_props = env_any_of[0]
                               .to_object()
                               .value()
                               .get("properties")
                               .value()
                               .to_object()
                               .value();
  EXPECT_TRUE(first_props.get("seq").has_value());
  EXPECT_TRUE(first_props.get("event").has_value());
  EXPECT_TRUE(first_props.get("source").has_value());
  EXPECT_TRUE(first_props.get("note").has_value());

  // 3-level schema: L1Envelope__tagged has 3 leaf branches (L2Simple + L3Alpha + L3Beta)
  const auto l1_schema = rfl::json::to_schema<Level1, rfl::NoExtraFields>();
  const auto l1_parsed = rfl::json::read<rfl::Generic>(l1_schema);
  ASSERT_TRUE(l1_parsed.has_value()) << l1_parsed.error().what();
  const auto l1_defs =
      l1_parsed->to_object().value().get("$defs").value().to_object().value();
  const auto l1_env_any_of =
      l1_defs.get("test_nested_tagged_union__L1Envelope__tagged")
          .value()
          .to_object()
          .value()
          .get("anyOf")
          .value()
          .to_array()
          .value();
  ASSERT_EQ(l1_env_any_of.size(), 3u);
  const auto l3_alpha_props = l1_env_any_of[1]
                                  .to_object()
                                  .value()
                                  .get("properties")
                                  .value()
                                  .to_object()
                                  .value();
  EXPECT_TRUE(l3_alpha_props.get("type").has_value());
  EXPECT_TRUE(l3_alpha_props.get("seq").has_value());
  EXPECT_TRUE(l3_alpha_props.get("kind").has_value());
  EXPECT_TRUE(l3_alpha_props.get("scope").has_value());
  EXPECT_TRUE(l3_alpha_props.get("subkind").has_value());
  EXPECT_TRUE(l3_alpha_props.get("value").has_value());

  // Direct subclass schema
  const auto direct_schema =
      rfl::json::to_schema<DirectS2CMessage, rfl::NoExtraFields>();
  const auto direct_parsed = rfl::json::read<rfl::Generic>(direct_schema);
  ASSERT_TRUE(direct_parsed.has_value()) << direct_parsed.error().what();
  const auto direct_defs = direct_parsed->to_object()
                               .value()
                               .get("$defs")
                               .value()
                               .to_object()
                               .value();
  const auto direct_any_of =
      direct_defs.get("test_nested_tagged_union__DirectEvent__tagged")
          .value()
          .to_object()
          .value()
          .get("anyOf")
          .value()
          .to_array()
          .value();
  ASSERT_EQ(direct_any_of.size(), 2u);
}

// ============================================================================
// Error cases
// ============================================================================

TEST(json, test_nested_tagged_union_error_cases) {
  // Missing outer discriminator
  {
    const auto res = rfl::json::read<S2CMessage>(
        R"({"event":"waveforms_loaded","source":"a.vcd"})");
    ASSERT_FALSE(res.has_value());
    EXPECT_NE(res.error().what().find("type"), std::string::npos);
  }

  // Unknown outer discriminator
  {
    const auto res = rfl::json::read<S2CMessage>(R"({"type":"unknown"})");
    ASSERT_FALSE(res.has_value());
    EXPECT_NE(res.error().what().find("unknown"), std::string::npos);
  }

  // Missing inner discriminator
  {
    const auto res = rfl::json::read<S2CMessage>(
        R"({"type":"event","source":"a.vcd"})");
    ASSERT_FALSE(res.has_value());
    EXPECT_NE(res.error().what().find("event"), std::string::npos);
  }

  // Unknown inner discriminator
  {
    const auto res = rfl::json::read<S2CMessage>(
        R"({"type":"event","event":"nonexistent","source":"a.vcd"})");
    ASSERT_FALSE(res.has_value());
    EXPECT_NE(res.error().what().find("nonexistent"), std::string::npos);
    EXPECT_NE(res.error().what().find("waveforms_loaded"), std::string::npos);
  }

  // Non-string inner discriminator
  {
    const auto res = rfl::json::read<S2CMessage>(
        R"({"type":"event","event":123,"source":"a.vcd"})");
    ASSERT_FALSE(res.has_value());
    EXPECT_NE(res.error().what().find("event"), std::string::npos);
  }

  // Missing required inner field
  {
    const auto res = rfl::json::read<S2CMessage>(
        R"({"type":"event","event":"waveforms_loaded"})");
    ASSERT_FALSE(res.has_value());
    EXPECT_NE(res.error().what().find("source"), std::string::npos);
  }

  // Bad inner field type
  {
    const auto res = rfl::json::read<S2CMessage>(
        R"({"type":"event","event":"waveforms_loaded","source":42})");
    ASSERT_FALSE(res.has_value());
    EXPECT_NE(res.error().what().find("source"), std::string::npos);
  }

  // Extra field rejected with NoExtraFields
  {
    const auto res = rfl::json::read<S2CMessage, rfl::NoExtraFields>(
        R"({"type":"event","event":"waveforms_loaded","source":"a.vcd","bogus":1})");
    ASSERT_FALSE(res.has_value());
    EXPECT_NE(res.error().what().find("bogus"), std::string::npos);
  }

  // Extra field rejected on Envelope with sibling fields + NoExtraFields
  {
    const auto res = rfl::json::read<Envelope, rfl::NoExtraFields>(
        R"({"seq":1,"event":"waveforms_loaded","source":"a.vcd","extra":true})");
    ASSERT_FALSE(res.has_value());
    EXPECT_NE(res.error().what().find("extra"), std::string::npos);
  }

  // Missing sibling field on Envelope
  {
    const auto res = rfl::json::read<Envelope>(
        R"({"event":"waveforms_loaded","source":"a.vcd"})");
    ASSERT_FALSE(res.has_value());
    EXPECT_NE(res.error().what().find("seq"), std::string::npos);
  }

  // Unknown 3rd-level discriminator
  {
    const auto res = rfl::json::read<Level1, rfl::NoExtraFields>(
        R"({"type":"envelope","seq":1,"kind":"complex","scope":"s","subkind":"gamma"})");
    ASSERT_FALSE(res.has_value());
    EXPECT_NE(res.error().what().find("gamma"), std::string::npos);
  }
}

}  // namespace test_nested_tagged_union
