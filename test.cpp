#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#define FN(__VA_ARGS__) [&](auto _) { return body; }
TEST_CASE("test linear probing", "[probe]"){
  REQUIRE(linear_probe(0,3,FN(false),1) == 8);
}
