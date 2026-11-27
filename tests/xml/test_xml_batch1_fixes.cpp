#include <gtest/gtest.h>

#include <memory>
#include <optional>
#include <pugixml.hpp>
#include <rfl.hpp>
#include <rfl/json.hpp>
#include <rfl/xml.hpp>
#include <string>
#include <vector>

namespace test_xml_batch1_fixes {

// --- #107: is_attribute_v unwrapping + Attribute<std::optional<T>> JSON Schema ---

static_assert(rfl::internal::is_attribute_v<rfl::Attribute<int>>);
static_assert(rfl::internal::is_attribute_v<std::optional<rfl::Attribute<int>>>);
static_assert(rfl::internal::is_attribute_v<
              rfl::Rename<"ver", std::optional<rfl::Attribute<std::string>>>>);
static_assert(
    rfl::internal::is_attribute_v<rfl::DefaultVal<rfl::Attribute<int>>>);
static_assert(rfl::internal::is_attribute_v<
              rfl::Description<"desc", std::optional<rfl::Attribute<int>>>>);
static_assert(
    rfl::internal::is_attribute_v<std::unique_ptr<rfl::Attribute<int>>>);
static_assert(!rfl::internal::is_attribute_v<std::optional<int>>);

struct AttrOrders {
  rfl::Attribute<std::string> req_attr;
  std::optional<rfl::Attribute<std::string>> opt_outer;
  rfl::Attribute<std::optional<int>> opt_inner;
  rfl::Rename<"renamed-opt", std::optional<rfl::Attribute<std::string>>>
      renamed_opt;
  rfl::DefaultVal<rfl::Attribute<int>> def_attr = 7;
  std::string body_elem;
};

TEST(xml, issue_107_optional_attribute_xml_roundtrip_and_schema) {
  // Engaged values serialize as XML attributes (not child elements).
  const AttrOrders full{
      .req_attr = "v1",
      .opt_outer = std::string("outer"),
      .opt_inner = 42,
      .renamed_opt = std::string("ren"),
      .def_attr = 9,
      .body_elem = "hello",
  };
  const auto xml_str = rfl::xml::write<"node">(full);
  EXPECT_NE(xml_str.find(R"(req_attr="v1")"), std::string::npos) << xml_str;
  EXPECT_NE(xml_str.find(R"(opt_outer="outer")"), std::string::npos) << xml_str;
  EXPECT_NE(xml_str.find(R"(opt_inner="42")"), std::string::npos) << xml_str;
  EXPECT_NE(xml_str.find(R"(renamed-opt="ren")"), std::string::npos) << xml_str;
  EXPECT_NE(xml_str.find(R"(def_attr="9")"), std::string::npos) << xml_str;
  EXPECT_EQ(xml_str.find("<opt_outer>"), std::string::npos) << xml_str;
  EXPECT_EQ(xml_str.find("<renamed-opt>"), std::string::npos) << xml_str;

  const auto parsed = rfl::xml::read<AttrOrders>(xml_str);
  ASSERT_TRUE(parsed.has_value()) << parsed.error().what();
  EXPECT_EQ(parsed->req_attr.value(), "v1");
  ASSERT_TRUE(parsed->opt_outer.has_value());
  EXPECT_EQ(parsed->opt_outer->value(), "outer");
  EXPECT_EQ(parsed->opt_inner.value(), 42);
  ASSERT_TRUE(parsed->renamed_opt.value().has_value());
  EXPECT_EQ(parsed->renamed_opt.value()->value(), "ren");
  EXPECT_EQ(parsed->def_attr.value().value(), 9);
  EXPECT_EQ(parsed->body_elem, "hello");

  // Empty optionals omitted under both default and NoOptionals.
  const AttrOrders sparse{.req_attr = "v2", .body_elem = "hi"};
  const auto sparse_xml = rfl::xml::write<"node">(sparse);
  EXPECT_EQ(sparse_xml.find("opt_outer"), std::string::npos) << sparse_xml;
  EXPECT_EQ(sparse_xml.find("opt_inner"), std::string::npos) << sparse_xml;
  EXPECT_EQ(sparse_xml.find("renamed-opt"), std::string::npos) << sparse_xml;

  const auto sparse_no_opt = rfl::xml::write<"node", rfl::NoOptionals>(sparse);
  EXPECT_EQ(sparse_no_opt.find("<opt_outer"), std::string::npos)
      << sparse_no_opt;
  EXPECT_EQ(sparse_no_opt.find("<opt_inner"), std::string::npos)
      << sparse_no_opt;

  const auto sparse_back = rfl::xml::read<AttrOrders>(sparse_xml);
  ASSERT_TRUE(sparse_back.has_value()) << sparse_back.error().what();
  EXPECT_FALSE(sparse_back->opt_outer.has_value());
  EXPECT_FALSE(sparse_back->opt_inner.value().has_value());
  EXPECT_FALSE(sparse_back->renamed_opt.value().has_value());

  // JSON Schema: neither Attribute<std::optional<T>> nor std::optional<Attribute<T>>
  // is marked required, and no Attribute<...> $defs entry is emitted.
  const auto schema = rfl::json::to_schema<AttrOrders>();
  EXPECT_EQ(schema.find("Attribute"), std::string::npos) << schema;
  EXPECT_NE(schema.find(R"("required":["req_attr","body_elem"])"),
            std::string::npos)
      << schema;
  EXPECT_NE(
      schema.find(R"("opt_outer":{"anyOf":[{"type":"string"},{"type":"null"}]})"),
      std::string::npos)
      << schema;
  EXPECT_NE(
      schema.find(R"("opt_inner":{"anyOf":[{"type":"integer"},{"type":"null"}]})"),
      std::string::npos)
      << schema;
}

// --- #108: rfl::NoExtraFields across all 3 XML reader paths ---

struct SimpleXml {
  rfl::Attribute<std::string> id;
  std::string name;
};

struct WithItems {
  rfl::Attribute<std::string> version;
  std::vector<std::string> item;
  std::optional<int> count;
};

struct WithContentAndDefaults {
  rfl::Attribute<std::string> id = "default_id";
  std::string xml_content = "default_text";
};

struct WithExtra {
  std::string known;
  std::vector<int> nums;
  rfl::ExtraFields<std::string> extra;
};

struct ManyFields {
  int f01 = 0; int f02 = 0; int f03 = 0; int f04 = 0;
  int f05 = 0; int f06 = 0; int f07 = 0; int f08 = 0;
  int f09 = 0; int f10 = 0; int f11 = 0; int f12 = 0;
  int f13 = 0; int f14 = 0; int f15 = 0; int f16 = 0;
  int f17 = 0; int f18 = 0;
  std::vector<int> list;
};

TEST(xml, issue_108_no_extra_fields_all_paths) {
  // Path 1: struct without xml_content succeeds under NoExtraFields.
  {
    const auto res = rfl::xml::read<SimpleXml, rfl::NoExtraFields>(
        R"(<root id="1"><name>Alice</name></root>)");
    ASSERT_TRUE(res.has_value()) << res.error().what();
    EXPECT_EQ(res->id.value(), "1");
    EXPECT_EQ(res->name, "Alice");
  }

  // Unknown child element rejected with ErrorKind::unknown_field.
  {
    const auto res = rfl::xml::read<SimpleXml, rfl::NoExtraFields>(
        R"(<root id="1"><name>Alice</name><bogus>x</bogus></root>)");
    ASSERT_FALSE(res.has_value());
    EXPECT_EQ(res.error().kind(), rfl::ErrorKind::unknown_field);
  }

  // Unknown attribute rejected with ErrorKind::unknown_field.
  {
    const auto res = rfl::xml::read<SimpleXml, rfl::NoExtraFields>(
        R"(<root id="1" extra_attr="nope"><name>Alice</name></root>)");
    ASSERT_FALSE(res.has_value());
    EXPECT_EQ(res.error().kind(), rfl::ErrorKind::unknown_field);
  }

  // Path 2: comments & pcdata under pugi::parse_full do not trigger unknown "".
  {
    pugi::xml_document doc;
    ASSERT_TRUE(doc.load_string(
        R"(<root id="2"><!-- a comment -->   <name>Bob</name><!-- tail --></root>)",
        pugi::parse_full));
    const auto res = rfl::xml::read<SimpleXml, rfl::NoExtraFields>(doc);
    ASSERT_TRUE(res.has_value()) << res.error().what();
    EXPECT_EQ(res->id.value(), "2");
    EXPECT_EQ(res->name, "Bob");
  }

  // Path 3: repeated sibling elements for vectors (contiguous & interleaved).
  {
    const auto res = rfl::xml::read<WithItems, rfl::NoExtraFields>(
        R"(<root version="1"><item>a</item><count>3</count><item>b</item><item>c</item></root>)");
    ASSERT_TRUE(res.has_value()) << res.error().what();
    EXPECT_EQ(res->version.value(), "1");
    EXPECT_EQ(res->item, (std::vector<std::string>{"a", "b", "c"}));
    EXPECT_EQ(res->count, 3);
  }

  // Mixed known array + unknown element still rejected.
  {
    const auto res = rfl::xml::read<WithItems, rfl::NoExtraFields>(
        R"(<root version="1"><item>a</item><item>b</item><unknown>z</unknown></root>)");
    ASSERT_FALSE(res.has_value());
    EXPECT_EQ(res.error().kind(), rfl::ErrorKind::unknown_field);
  }

  // Overflow deduplication (>16 distinct sibling elements + repeated vector).
  {
    const auto res =
        rfl::xml::read<ManyFields, rfl::DefaultIfMissing, rfl::NoExtraFields>(
            R"(<root><f01>1</f01><f02>2</f02><f03>3</f03><f04>4</f04>)"
            R"(<f05>5</f05><f06>6</f06><f07>7</f07><f08>8</f08>)"
            R"(<f09>9</f09><f10>10</f10><f11>11</f11><f12>12</f12>)"
            R"(<f13>13</f13><f14>14</f14><f15>15</f15><f16>16</f16>)"
            R"(<list>100</list><f17>17</f17><f18>18</f18><list>200</list></root>)");
    ASSERT_TRUE(res.has_value()) << res.error().what();
    EXPECT_EQ(res->f18, 18);
    EXPECT_EQ(res->list, (std::vector<int>{100, 200}));
  }

  // ViewReaderWithDefault (DefaultIfMissing) populates xml_content and respects NoExtraFields.
  {
    const auto res =
        rfl::xml::read<WithContentAndDefaults, rfl::DefaultIfMissing,
                       rfl::NoExtraFields>(R"(<entry>some text</entry>)");
    ASSERT_TRUE(res.has_value()) << res.error().what();
    EXPECT_EQ(res->id.value(), "default_id");
    EXPECT_EQ(res->xml_content, "some text");

    const auto bad =
        rfl::xml::read<WithContentAndDefaults, rfl::DefaultIfMissing,
                       rfl::NoExtraFields>(
            R"(<entry bogus="x">some text</entry>)");
    ASSERT_FALSE(bad.has_value());
    EXPECT_EQ(bad.error().kind(), rfl::ErrorKind::unknown_field);
  }

  // ExtraFields does not leak xml_content, empty comment keys, or 2nd+ array siblings.
  {
    pugi::xml_document doc;
    ASSERT_TRUE(doc.load_string(
        R"(<root><!-- c --><known>k</known><nums>1</nums><nums>2</nums><dyn>v</dyn></root>)",
        pugi::parse_full));
    const auto res = rfl::xml::read<WithExtra>(doc);
    ASSERT_TRUE(res.has_value()) << res.error().what();
    EXPECT_EQ(res->known, "k");
    EXPECT_EQ(res->nums, (std::vector<int>{1, 2}));
    EXPECT_EQ(res->extra.size(), 1u);
    EXPECT_EQ(res->extra.get("dyn").value(), "v");
  }

  // FieldVariantReader with repeated sibling array elements.
  {
    using FV = rfl::Variant<rfl::Field<"nums", std::vector<int>>,
                            rfl::Field<"name", std::string>>;
    const auto res = rfl::xml::read<FV>(
        R"(<root><nums>10</nums><nums>20</nums><nums>30</nums></root>)");
    ASSERT_TRUE(res.has_value()) << res.error().what();
    const auto* field = rfl::get_if<rfl::Field<"nums", std::vector<int>>>(&*res);
    ASSERT_NE(field, nullptr);
    EXPECT_EQ(field->value(), (std::vector<int>{10, 20, 30}));
  }
}

// --- #112: empty vs declaration-only XML + ErrorKind tagging ---

struct ScalarTypes {
  bool flag;
  int count;
  double ratio;
};

struct NestedHolder {
  SimpleXml inner;
};

TEST(xml, issue_112_empty_vs_declaration_only_and_error_kinds) {
  // Empty and whitespace-only strings -> "XML document is empty." + syntax_error.
  for (const std::string_view empty_doc :
       {"", "   \n\t\r  ", "\xEF\xBB\xBF   \n"}) {
    const auto res = rfl::xml::read<SimpleXml>(empty_doc);
    ASSERT_FALSE(res.has_value());
    EXPECT_EQ(res.error().kind(), rfl::ErrorKind::syntax_error);
    EXPECT_EQ(res.error().what(), "XML document is empty.");
  }

  // Declaration-only and comment-only -> "XML document has no root element." + syntax_error.
  for (const std::string_view no_root_doc :
       {R"(<?xml version="1.0" encoding="UTF-8"?>)",
        R"(<!-- just a comment -->)"}) {
    const auto res = rfl::xml::read<SimpleXml>(no_root_doc);
    ASSERT_FALSE(res.has_value());
    EXPECT_EQ(res.error().kind(), rfl::ErrorKind::syntax_error);
    EXPECT_EQ(res.error().what(), "XML document has no root element.");
  }

  // Malformed XML -> syntax_error with parse description.
  {
    const auto res = rfl::xml::read<SimpleXml>("<root><unclosed></root>");
    ASSERT_FALSE(res.has_value());
    EXPECT_EQ(res.error().kind(), rfl::ErrorKind::syntax_error);
    EXPECT_NE(res.error().what().find("XML string could not be parsed"),
              std::string::npos);
  }

  // Empty pugi::xml_node & declaration-only pugi::xml_document -> syntax_error.
  {
    const pugi::xml_node null_node;
    const auto res = rfl::xml::read<SimpleXml>(null_node);
    ASSERT_FALSE(res.has_value());
    EXPECT_EQ(res.error().kind(), rfl::ErrorKind::syntax_error);
  }
  {
    pugi::xml_document doc;
    doc.append_child(pugi::node_declaration);
    const auto res = rfl::xml::read<SimpleXml>(doc);
    ASSERT_FALSE(res.has_value());
    EXPECT_EQ(res.error().kind(), rfl::ErrorKind::syntax_error);
    EXPECT_EQ(res.error().what(), "XML document has no root element.");
  }

  // Missing field -> missing_field.
  {
    const auto res = rfl::xml::read<SimpleXml>(R"(<root id="1"></root>)");
    ASSERT_FALSE(res.has_value());
    EXPECT_EQ(res.error().kind(), rfl::ErrorKind::missing_field);
  }

  // Direct Reader::get_field_from_object / get_field_from_array -> missing_field.
  {
    pugi::xml_document doc;
    ASSERT_TRUE(doc.load_string(R"(<root><item>1</item></root>)"));
    const rfl::xml::Reader reader;
    const auto obj_err = reader.get_field_from_object(
        "missing", rfl::xml::Reader::XMLInputObject(doc.first_child()));
    ASSERT_FALSE(obj_err.has_value());
    EXPECT_EQ(obj_err.error().kind(), rfl::ErrorKind::missing_field);

    const auto arr_err = reader.get_field_from_array(
        5, rfl::xml::Reader::XMLInputArray(doc.first_child().child("item")));
    ASSERT_FALSE(arr_err.has_value());
    EXPECT_EQ(arr_err.error().kind(), rfl::ErrorKind::missing_field);
  }

  // Invalid scalar casts -> invalid_value.
  {
    const auto res = rfl::xml::read<ScalarTypes>(
        R"(<root><flag>maybe</flag><count>1</count><ratio>1.5</ratio></root>)");
    ASSERT_FALSE(res.has_value());
    EXPECT_EQ(res.error().kind(), rfl::ErrorKind::invalid_value);
  }
  {
    const auto res = rfl::xml::read<ScalarTypes>(
        R"(<root><flag>true</flag><count>abc</count><ratio>1.5</ratio></root>)");
    ASSERT_FALSE(res.has_value());
    EXPECT_EQ(res.error().kind(), rfl::ErrorKind::invalid_value);
  }
  {
    const auto res = rfl::xml::read<ScalarTypes>(
        R"(<root><flag>true</flag><count>1</count><ratio>xyz</ratio></root>)");
    ASSERT_FALSE(res.has_value());
    EXPECT_EQ(res.error().kind(), rfl::ErrorKind::invalid_value);
  }

  // Attribute supplied where an object node is required -> invalid_type.
  {
    const auto res =
        rfl::xml::read<NestedHolder>(R"(<root inner="not-a-node"/>)");
    ASSERT_FALSE(res.has_value());
    EXPECT_EQ(res.error().kind(), rfl::ErrorKind::invalid_type);
  }
}

}  // namespace test_xml_batch1_fixes
