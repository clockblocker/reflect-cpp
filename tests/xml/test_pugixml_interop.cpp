#include <gtest/gtest.h>
#include <pugixml.hpp>

#include <array>
#include <memory>
#include <rfl.hpp>
#include <rfl/xml.hpp>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

namespace test_pugixml_interop {

struct Item {
  rfl::Attribute<std::string> id;
  std::string item_name;
  int quantity;
};

TEST(xml_pugixml_interop, read_subtree_via_xpath) {
  const char* xml = R"(<?xml version="1.0" encoding="UTF-8"?>
<!-- outer comment -->
<envelope xmlns:ext="http://example.com/ext">
  <ext:header>ignore me</ext:header>
  <section id="other">
    <item id="wrong"><item_name>Wrong</item_name><quantity>0</quantity></item>
  </section>
  <section id="main">
    <!-- section comment -->
    <item id="sku-42">
      <item_name>Widget</item_name>
      <quantity>7</quantity>
    </item>
    <trailing>untouched</trailing>
  </section>
</envelope>
)";

  pugi::xml_document doc;
  ASSERT_TRUE(doc.load_string(xml, pugi::parse_full));

  const auto node = doc.select_node("//section[@id='main']/item").node();
  ASSERT_TRUE(node);

  const auto res = rfl::xml::read<Item>(node);
  ASSERT_TRUE(res) << res.error().what();
  EXPECT_EQ(res.value().id(), "sku-42");
  EXPECT_EQ(res.value().item_name, "Widget");
  EXPECT_EQ(res.value().quantity, 7);
}

TEST(xml_pugixml_interop, read_document_with_declaration_and_comments) {
  const char* xml = R"(<?xml version="1.0" encoding="UTF-8"?>
<!-- leading comment -->
<Item id="doc-1">
  <item_name>Gadget</item_name>
  <quantity>3</quantity>
</Item>
)";

  pugi::xml_document doc;
  ASSERT_TRUE(doc.load_string(xml, pugi::parse_full));

  const auto from_doc = rfl::xml::read<Item>(doc);
  ASSERT_TRUE(from_doc) << from_doc.error().what();
  EXPECT_EQ(from_doc.value().id(), "doc-1");
  EXPECT_EQ(from_doc.value().item_name, "Gadget");
  EXPECT_EQ(from_doc.value().quantity, 3);

  const pugi::xml_node doc_as_node = doc;
  const auto from_doc_node = rfl::xml::read<Item>(doc_as_node);
  ASSERT_TRUE(from_doc_node) << from_doc_node.error().what();
  EXPECT_EQ(from_doc_node.value().id(), "doc-1");
  EXPECT_EQ(from_doc_node.value().item_name, "Gadget");
  EXPECT_EQ(from_doc_node.value().quantity, 3);
}

TEST(xml_pugixml_interop, read_empty_node_returns_error) {
  const auto res_null = rfl::xml::read<Item>(pugi::xml_node{});
  EXPECT_FALSE(res_null);

  pugi::xml_document doc;
  ASSERT_TRUE(doc.load_string("<root/>"));
  const auto res_missing = rfl::xml::read<Item>(doc.child("missing"));
  EXPECT_FALSE(res_missing);

  pugi::xml_document empty_doc;
  const auto res_empty_doc = rfl::xml::read<Item>(empty_doc);
  EXPECT_FALSE(res_empty_doc);

  const pugi::xml_node empty_doc_node = empty_doc;
  const auto res_empty_doc_node = rfl::xml::read<Item>(empty_doc_node);
  EXPECT_FALSE(res_empty_doc_node);
}

TEST(xml_pugixml_interop, write_splice_into_existing_node) {
  pugi::xml_document doc;
  ASSERT_TRUE(doc.load_string(
      "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
      "<config><!-- keep me --><existing>42</existing></config>",
      pugi::parse_full));

  const auto item =
      Item{.id = "sku-9", .item_name = "Sprocket", .quantity = 12};

  // Supports rvalue pugi::xml_node (e.g. doc.child("config")), compile-time
  // and runtime root names, and processors.
  const auto n_default = rfl::xml::write(item, doc.child("config"));
  ASSERT_TRUE(n_default);
  EXPECT_STREQ(n_default.name(), "Item");

  const auto config = doc.child("config");
  const auto n_compile_time = rfl::xml::write<"custom_item">(item, config);
  ASSERT_TRUE(n_compile_time);
  EXPECT_STREQ(n_compile_time.name(), "custom_item");

  const std::string runtime_tag = "runtime_item";
  const auto n_runtime = rfl::xml::write(item, config, runtime_tag);
  ASSERT_TRUE(n_runtime);
  EXPECT_STREQ(n_runtime.name(), "runtime_item");

  const auto n_camel =
      rfl::xml::write<"camel_item", rfl::SnakeCaseToCamelCase>(item, config);
  ASSERT_TRUE(n_camel);
  EXPECT_STREQ(n_camel.name(), "camel_item");
  EXPECT_STREQ(n_camel.child_value("itemName"), "Sprocket");
  const auto parsed_camel =
      rfl::xml::read<Item, rfl::SnakeCaseToCamelCase>(n_camel);
  ASSERT_TRUE(parsed_camel) << parsed_camel.error().what();
  EXPECT_EQ(parsed_camel.value().item_name, "Sprocket");

  // Existing comment and sibling must remain intact and first.
  const auto first = config.first_child();
  ASSERT_EQ(first.type(), pugi::node_comment);
  EXPECT_STREQ(first.value(), " keep me ");
  EXPECT_STREQ(first.next_sibling().name(), "existing");
  EXPECT_STREQ(first.next_sibling().child_value(), "42");

  // Round-trip directly from the returned pugixml handles.
  for (const auto& node : {n_default, n_compile_time, n_runtime}) {
    const auto parsed = rfl::xml::read<Item>(node);
    ASSERT_TRUE(parsed) << parsed.error().what();
    EXPECT_EQ(parsed.value().id(), "sku-9");
    EXPECT_EQ(parsed.value().item_name, "Sprocket");
    EXPECT_EQ(parsed.value().quantity, 12);
  }

  // Writing directly into a stack-allocated pugi::xml_document does not take
  // ownership of the document.
  pugi::xml_document standalone;
  const auto root = rfl::xml::write(item, standalone);
  ASSERT_TRUE(root);
  EXPECT_EQ(root, standalone.document_element());
  EXPECT_EQ(rfl::xml::read<Item>(standalone).value().item_name, "Sprocket");

  // Writing into an empty handle or with an empty runtime root name fails safely.
  EXPECT_FALSE(rfl::xml::write(item, pugi::xml_node{}));
  EXPECT_FALSE(rfl::xml::write(item, config, ""));
}

struct RichText {
  std::string lang;
  std::string plain;
  std::vector<std::string> bold_spans;

  static rfl::Result<RichText> from_xml_obj(const pugi::xml_node& _node) {
    const auto lang_attr = _node.attribute("lang");
    if (!lang_attr) {
      return rfl::error("RichText node missing 'lang' attribute.");
    }
    RichText out;
    out.lang = lang_attr.value();
    for (auto child = _node.first_child(); child;
         child = child.next_sibling()) {
      if (child.type() == pugi::node_pcdata) {
        out.plain += child.value();
      } else if (child.type() == pugi::node_element &&
                 std::string_view(child.name()) == "b") {
        out.plain += child.child_value();
        out.bold_spans.emplace_back(child.child_value());
      }
    }
    return out;
  }
};

struct Article {
  std::string title;
  RichText body;
};

struct DirectValueNode {
  std::string text;

  static DirectValueNode from_xml_obj(const pugi::xml_node& _node) {
    return DirectValueNode{.text = std::string(_node.child_value())};
  }
};

struct ThrowingNode {
  int v = 0;

  static ThrowingNode from_xml_obj(const pugi::xml_node&) {
    throw std::runtime_error("custom parse failure");
  }
};

struct MoveOnlyNode {
  std::unique_ptr<int> value;

  static rfl::Result<MoveOnlyNode> from_xml_obj(const pugi::xml_node& _node) {
    return MoveOnlyNode{.value = std::make_unique<int>(_node.text().as_int())};
  }
};

TEST(xml_pugixml_interop, custom_from_xml_obj_in_reflected_struct) {
  const char* xml = R"(<?xml version="1.0" encoding="UTF-8"?>
<Article>
  <title>Hello</title>
  <body lang="en">Click <b>here</b> or <b>there</b>!</body>
</Article>
)";

  const auto res = rfl::xml::read<Article>(xml);
  ASSERT_TRUE(res) << res.error().what();
  EXPECT_EQ(res.value().title, "Hello");
  EXPECT_EQ(res.value().body.lang, "en");
  EXPECT_EQ(res.value().body.plain, "Click here or there!");
  ASSERT_EQ(res.value().body.bold_spans.size(), 2u);
  EXPECT_EQ(res.value().body.bold_spans[0], "here");
  EXPECT_EQ(res.value().body.bold_spans[1], "there");

  // Error returned by from_xml_obj propagates cleanly.
  const auto missing_attr =
      rfl::xml::read<Article>("<Article><title>T</title><body>hi</body></Article>");
  EXPECT_FALSE(missing_attr);

  // Attribute matched against a from_xml_obj field returns an error cleanly.
  const auto attr_instead_of_elem =
      rfl::xml::read<Article>("<Article body=\"oops\"><title>T</title></Article>");
  EXPECT_FALSE(attr_instead_of_elem);

  // Direct value return (non-Result), exception catching, and move-only types.
  const auto direct =
      rfl::xml::read<DirectValueNode>("<DirectValueNode>abc</DirectValueNode>");
  ASSERT_TRUE(direct) << direct.error().what();
  EXPECT_EQ(direct.value().text, "abc");

  const auto thrown =
      rfl::xml::read<ThrowingNode>("<ThrowingNode>1</ThrowingNode>");
  ASSERT_FALSE(thrown);
  EXPECT_NE(thrown.error().what().find("custom parse failure"),
            std::string::npos);

  const auto move_only =
      rfl::xml::read<MoveOnlyNode>("<MoveOnlyNode>99</MoveOnlyNode>");
  ASSERT_TRUE(move_only) << move_only.error().what();
  ASSERT_NE(move_only.value().value, nullptr);
  EXPECT_EQ(*move_only.value().value, 99);
}

TEST(xml_pugixml_interop, regression_get_field_from_array) {
  pugi::xml_document doc;
  ASSERT_TRUE(doc.load_string(
      "<root><v>10</v><v>20</v><v>30</v></root>"));

  const rfl::xml::Reader reader;
  const rfl::xml::Reader::InputArrayType arr(doc.child("root").child("v"));

  const auto elem0 = reader.get_field_from_array(0, arr);
  ASSERT_TRUE(elem0) << elem0.error().what();
  EXPECT_EQ(reader.to_basic_type<int>(elem0.value()).value(), 10);

  const auto elem1 = reader.get_field_from_array(1, arr);
  ASSERT_TRUE(elem1) << elem1.error().what();
  EXPECT_EQ(reader.to_basic_type<int>(elem1.value()).value(), 20);

  const auto elem2 = reader.get_field_from_array(2, arr);
  ASSERT_TRUE(elem2) << elem2.error().what();
  EXPECT_EQ(reader.to_basic_type<int>(elem2.value()).value(), 30);

  const auto oob = reader.get_field_from_array(3, arr);
  EXPECT_FALSE(oob);

  // Also verify std::tuple and std::array round-trip cleanly in XML.
  struct WithSeq {
    std::tuple<int, std::string, bool> tup;
    std::array<int, 3> arr;
  };
  const WithSeq orig{.tup = {1, "two", true}, .arr = {4, 5, 6}};
  const auto xml = rfl::xml::write(orig);
  const auto parsed = rfl::xml::read<WithSeq>(xml);
  ASSERT_TRUE(parsed) << parsed.error().what();
  EXPECT_EQ(parsed.value().tup, orig.tup);
  EXPECT_EQ(parsed.value().arr, orig.arr);
}

TEST(xml_pugixml_interop, regression_write_custom_indent) {
  struct Simple {
    int a = 1;
    std::string b = "two";
  };

  const Simple s;
  const auto indent2 = rfl::xml::write(s, "  ");
  EXPECT_EQ(indent2,
            "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
            "<Simple>\n"
            "  <a>1</a>\n"
            "  <b>two</b>\n"
            "</Simple>\n");

  const auto indent_tab = rfl::xml::write(s, "\t");
  EXPECT_EQ(indent_tab,
            "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
            "<Simple>\n"
            "\t<a>1</a>\n"
            "\t<b>two</b>\n"
            "</Simple>\n");
}

}  // namespace test_pugixml_interop
