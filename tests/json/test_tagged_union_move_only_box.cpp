#include <gtest/gtest.h>

#include <memory>
#include <rfl.hpp>
#include <rfl/json.hpp>
#include <string>

namespace test_tagged_union_move_only_box {

struct Expr;

struct LiteralExpr {
  rfl::Literal<"literal"> kind;
  int value;
};

struct UnaryExpr {
  rfl::Literal<"unary"> kind;
  std::string op;
  rfl::Box<Expr> operand;
};

struct BinaryExpr {
  rfl::Literal<"binary"> kind;
  std::string op;
  rfl::Box<Expr> left;
  std::unique_ptr<Expr> right;
};

struct Expr {
  using Variant = rfl::TaggedUnion<"kind", LiteralExpr, UnaryExpr, BinaryExpr>;
  using ReflectionType = Variant;

  Variant value;

  const Variant& reflection() const { return value; }
};

struct ExternalBoxAlt {
  rfl::Box<int> boxed;
};

struct ExternalUniqueAlt {
  std::unique_ptr<std::string> text;
};

TEST(json, tagged_union_inline_discriminator_with_move_only_box) {
  Expr ast{
      .value = BinaryExpr{
          .kind = rfl::Literal<"binary">::make<"binary">(),
          .op = "+",
          .left = rfl::make_box<Expr>(Expr{
              .value = UnaryExpr{
                  .kind = rfl::Literal<"unary">::make<"unary">(),
                  .op = "-",
                  .operand = rfl::make_box<Expr>(Expr{
                      .value = LiteralExpr{
                          .kind = rfl::Literal<"literal">::make<"literal">(),
                          .value = 3,
                      },
                  }),
              },
          }),
          .right = std::make_unique<Expr>(Expr{
              .value = LiteralExpr{
                  .kind = rfl::Literal<"literal">::make<"literal">(),
                  .value = 5,
              },
          }),
      },
  };

  const auto json_str = rfl::json::write(ast);
  const auto parsed = rfl::json::read<Expr>(json_str);
  ASSERT_TRUE(parsed.has_value()) << parsed.error().what();
  EXPECT_EQ(rfl::json::write(*parsed), json_str);

  const auto* bin = rfl::get_if<BinaryExpr>(&parsed->value.variant());
  ASSERT_NE(bin, nullptr);
  EXPECT_EQ(bin->op, "+");
  const auto* unary = rfl::get_if<UnaryExpr>(&bin->left->value.variant());
  ASSERT_NE(unary, nullptr);
  EXPECT_EQ(unary->op, "-");
  const auto* lit_l = rfl::get_if<LiteralExpr>(&unary->operand->value.variant());
  ASSERT_NE(lit_l, nullptr);
  EXPECT_EQ(lit_l->value, 3);
  ASSERT_NE(bin->right, nullptr);
  const auto* lit_r = rfl::get_if<LiteralExpr>(&bin->right->value.variant());
  ASSERT_NE(lit_r, nullptr);
  EXPECT_EQ(lit_r->value, 5);

  const auto schema = rfl::json::to_schema<Expr>();
  EXPECT_NE(schema.find("\"oneOf\""), std::string::npos);
  EXPECT_NE(schema.find("\"unary\""), std::string::npos);
  EXPECT_NE(schema.find("\"binary\""), std::string::npos);
}

TEST(json, tagged_union_external_tag_with_move_only_box) {
  using MoveOnlyUnion =
      rfl::TaggedUnion<"type", ExternalBoxAlt, ExternalUniqueAlt>;
  const MoveOnlyUnion orig = ExternalBoxAlt{.boxed = rfl::make_box<int>(99)};
  const auto json_str = rfl::json::write(orig);
  EXPECT_EQ(json_str, R"({"type":"ExternalBoxAlt","boxed":99})");

  const auto parsed = rfl::json::read<MoveOnlyUnion>(json_str);
  ASSERT_TRUE(parsed.has_value()) << parsed.error().what();
  const auto* alt = rfl::get_if<ExternalBoxAlt>(&parsed->variant());
  ASSERT_NE(alt, nullptr);
  EXPECT_EQ(*alt->boxed, 99);

  const auto schema = rfl::json::to_schema<MoveOnlyUnion>();
  EXPECT_NE(schema.find("\"oneOf\""), std::string::npos);
}

}  // namespace test_tagged_union_move_only_box
