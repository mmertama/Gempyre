#include <gtest/gtest.h>
#include "gempyre.h"
#include "gempyre_graphics.h"


TEST(Unittests, Test_rgb) {
    auto col1 = Gempyre::Color::rgba(0x33, 0x44, 0x55);
    EXPECT_EQ(Gempyre::Color::rgb(col1), "#334455");
    EXPECT_EQ(Gempyre::Color::rgba(col1), "#334455FF");

    auto col2 = Gempyre::Color::rgba(0xAA, 0xBB, 0xCC);
    EXPECT_EQ(Gempyre::Color::rgb(col2), "#AABBCC");
    EXPECT_EQ(Gempyre::Color::rgba(col2), "#AABBCCFF");

    auto col3 = Gempyre::Color::rgba(0, 0, 0xCC);
    EXPECT_EQ(Gempyre::Color::rgb(col3), "#0000CC");
    EXPECT_EQ(Gempyre::Color::rgba(col3), "#0000CCFF");

    auto col4 = Gempyre::Color::rgba(0, 0x3, 0xCC);
    EXPECT_EQ(Gempyre::Color::rgb(col4), "#0003CC");
    EXPECT_EQ(Gempyre::Color::rgba(col4), "#0003CCFF");

    auto col5 = Gempyre::Color::rgba(0x11, 0x22, 0x33, 0xCC);
    EXPECT_EQ(Gempyre::Color::rgb(col5), "#112233");
    EXPECT_EQ(Gempyre::Color::rgba(col5), "#112233CC");
}


int main(int argc, char **argv) {
   ::testing::InitGoogleTest(&argc, argv);
   for(int i = 1 ; i < argc; ++i)
       if(argv[i] == std::string_view("--verbose"))
            Gempyre::setDebug();
  return RUN_ALL_TESTS();
}
