#include <gtest/gtest.h>

#include <rfl.hpp>
#include <rfl/error_format.hpp>
#include <rfl/json.hpp>
#include <string>
#include <vector>

namespace test_json_error_format {

using Path = std::vector<rfl::ErrorDetail::PathSegment>;

TEST(json, test_error_combine_and_chaining) {
  EXPECT_EQ(rfl::Error::combine({}).what(), "");

  const auto single_src =
      rfl::error("bad port", rfl::ErrorKind::invalid_value, {"port"}).error();
  const auto single = rfl::Error::combine({single_src});
  EXPECT_EQ(single.what(), "bad port");
  EXPECT_EQ(single.details(), single_src.details());

  // Lvalue chaining preserves the original Error and propagates path/kind.
  const rfl::Error base_lval =
      rfl::error("must be positive", rfl::ErrorKind::constraint_violation)
          .error();
  const auto wrapped_lval =
      base_lval.in_field("zip").in_field("address").at(0).in_field("users");
  EXPECT_EQ(base_lval.what(), "must be positive");
  EXPECT_TRUE(base_lval.details()[0].path.empty());
  EXPECT_EQ(wrapped_lval.what(),
            "Failed to parse field 'users': Failed to parse element 0: "
            "Failed to parse field 'address': Failed to parse field 'zip': "
            "must be positive");
  ASSERT_EQ(wrapped_lval.details().size(), 1u);
  EXPECT_EQ(wrapped_lval.details()[0].path,
            (Path{"users", size_t{0}, "address", "zip"}));
  EXPECT_EQ(wrapped_lval.details()[0].kind,
            rfl::ErrorKind::constraint_violation);

  // Rvalue chaining and PathSegment overloads (field name + index).
  const auto wrapped_rval =
      rfl::error("expected int", rfl::ErrorKind::invalid_type)
          .error()
          .at(rfl::ErrorDetail::PathSegment{"count"})
          .at(rfl::ErrorDetail::PathSegment{size_t{2}})
          .at("items");
  EXPECT_EQ(wrapped_rval.what(),
            "Failed to parse field 'items': Failed to parse element 2: "
            "Failed to parse field 'count': expected int");
  ASSERT_EQ(wrapped_rval.details().size(), 1u);
  EXPECT_EQ(wrapped_rval.details()[0].path, (Path{"items", size_t{2}, "count"}));
  EXPECT_EQ(wrapped_rval.details()[0].kind, rfl::ErrorKind::invalid_type);

  const auto combined = rfl::Error::combine({wrapped_lval, wrapped_rval});
  EXPECT_EQ(combined.what(),
            "Found 2 errors:\n"
            "1) Failed to parse field 'users': Failed to parse element 0: "
            "Failed to parse field 'address': Failed to parse field 'zip': "
            "must be positive\n"
            "2) Failed to parse field 'items': Failed to parse element 2: "
            "Failed to parse field 'count': expected int");
  ASSERT_EQ(combined.details().size(), 2u);
  EXPECT_EQ(combined.details()[0].path,
            (Path{"users", size_t{0}, "address", "zip"}));
  EXPECT_EQ(combined.details()[0].kind, rfl::ErrorKind::constraint_violation);
  EXPECT_EQ(combined.details()[1].path, (Path{"items", size_t{2}, "count"}));
  EXPECT_EQ(combined.details()[1].kind, rfl::ErrorKind::invalid_type);
}

TEST(json, test_format_path) {
  EXPECT_EQ(rfl::format_path({}), "");
  EXPECT_EQ(rfl::format_path(Path{"users", size_t{0}, "address", "zip"}),
            "users[0].address.zip");
  EXPECT_EQ(rfl::format_path(Path{size_t{0}, "name"}), "[0].name");
  EXPECT_EQ(rfl::format_path(Path{"a.b"}), "[\"a.b\"]");
  EXPECT_EQ(rfl::format_path(Path{"cfg", "a.b", "has space", "k[0]", "x]y"}),
            "cfg[\"a.b\"][\"has space\"][\"k[0]\"][\"x]y\"]");
  EXPECT_EQ(rfl::format_path(Path{""}), "[\"\"]");
  EXPECT_EQ(rfl::format_path(Path{"a\"b\\c"}), "[\"a\\\"b\\\\c\"]");
  EXPECT_EQ(rfl::format_path(Path{"line1\nline2\t\r\x01"}),
            "[\"line1\\nline2\\t\\r\\x01\"]");
}

TEST(json, test_format_json_pointer) {
  EXPECT_EQ(rfl::format_json_pointer({}), "");
  EXPECT_EQ(rfl::format_json_pointer(Path{"users", size_t{0}, "a/b", "c~d"}),
            "/users/0/a~1b/c~0d");
  EXPECT_EQ(rfl::format_json_pointer(Path{"~1", "/~"}), "/~01/~1~0");
  EXPECT_EQ(rfl::format_json_pointer(Path{""}), "/");
}

TEST(json, test_format_error_tree) {
  EXPECT_EQ(rfl::format_error_tree(rfl::Error("Invalid syntax")),
            "Invalid syntax");

  const auto grouped = rfl::Error::combine({
      rfl::error("Could not cast to int.", rfl::ErrorKind::invalid_type,
                 {"port"})
          .error(),
      rfl::error("Must be >= 1.", rfl::ErrorKind::constraint_violation,
                 {"items", size_t{0}, "count"})
          .error(),
      rfl::error("Could not cast to string.", rfl::ErrorKind::invalid_type,
                 {"items", size_t{0}, "name"})
          .error(),
      rfl::error("Field named 'host' not found.", rfl::ErrorKind::missing_field,
                 {"host"})
          .error(),
  });

  EXPECT_EQ(rfl::format_error_tree(grouped),
            "├─ port\n"
            "│  └─ Could not cast to int.\n"
            "├─ items\n"
            "│  └─ [0]\n"
            "│     ├─ count\n"
            "│     │  └─ Must be >= 1.\n"
            "│     └─ name\n"
            "│        └─ Could not cast to string.\n"
            "└─ host\n"
            "   └─ Field named 'host' not found.");

  const auto custom_tree =
      rfl::format_error_tree(grouped, [](const rfl::ErrorDetail& _d) {
        switch (_d.kind) {
          case rfl::ErrorKind::invalid_type:
            return std::string("[TYPE] ") + _d.message;
          case rfl::ErrorKind::constraint_violation:
            return std::string("[RULE] ") + _d.message;
          case rfl::ErrorKind::missing_field:
            return std::string("[MISSING] ") + _d.message;
          default:
            return _d.message;
        }
      });
  EXPECT_EQ(custom_tree,
            "├─ port\n"
            "│  └─ [TYPE] Could not cast to int.\n"
            "├─ items\n"
            "│  └─ [0]\n"
            "│     ├─ count\n"
            "│     │  └─ [RULE] Must be >= 1.\n"
            "│     └─ name\n"
            "│        └─ [TYPE] Could not cast to string.\n"
            "└─ host\n"
            "   └─ [MISSING] Field named 'host' not found.");
}

TEST(json, test_error_details_json_serialization) {
  const auto err = rfl::Error::combine({
      rfl::error("Must be >= 1.", rfl::ErrorKind::constraint_violation,
                 {"items", size_t{0}, "count"})
          .error(),
      rfl::error("Field named 'host' not found.", rfl::ErrorKind::missing_field,
                 {"host"})
          .error(),
  });

  const std::string serialized = rfl::json::write(err.details());
  const auto roundtrip =
      rfl::json::read<std::vector<rfl::ErrorDetail>>(serialized);
  ASSERT_TRUE(roundtrip) << roundtrip.error().what();
  EXPECT_EQ(*roundtrip, err.details());
  EXPECT_NE(serialized.find("\"path\":[\"items\",0,\"count\"]"),
            std::string::npos);
  EXPECT_NE(serialized.find("\"kind\":\"constraint_violation\""),
            std::string::npos);
  EXPECT_NE(serialized.find("\"kind\":\"missing_field\""), std::string::npos);
}

}  // namespace test_json_error_format
