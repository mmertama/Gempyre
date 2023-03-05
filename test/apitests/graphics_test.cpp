#include "gempyre_test.h"
#include "gempyre_graphics.h"
#include <array>

using namespace std::chrono_literals;
using namespace GempyreTest;

#define MAKE_CANVAS Gempyre::CanvasElement canvas(ui(), "canvas"); 

constexpr auto max_image_wait = 30s;
constexpr auto min_image_wait = 5s;

TEST_F(TestUi, make_canvas) {
    MAKE_CANVAS
    canvas.draw_completed([this]() {
        test_exit();
    });
    Gempyre::Bitmap g(640, 640);
    canvas.draw(0, 0, g);
    timeout(max_image_wait);
}

TEST_F(TestUi, make_canvas1) {
    MAKE_CANVAS
    canvas.draw_completed([this]() {
        test_exit();
    });
    Gempyre::Bitmap g(200, 200);
    canvas.draw(0, 0, g);
    timeout(max_image_wait);
}

TEST_F(TestUi, make_canvas2) {
    MAKE_CANVAS
    canvas.draw_completed([this]() {
        test_exit();
    });
    Gempyre::Bitmap g(840, 840);
    canvas.draw(0, 0, g);
    timeout(max_image_wait);
}


TEST_F(TestUi, make_canvas3) {
    MAKE_CANVAS
    canvas.draw_completed([this]() {
        test_exit();
    });
    Gempyre::Bitmap g(200, 1200);
    canvas.draw(0, 0, g);
    timeout(max_image_wait);
}

TEST_F(TestUi, make_canvas4) {
    MAKE_CANVAS
    canvas.draw_completed([this]() {
        test_exit();
    });
    Gempyre::Bitmap g(840, 4);
    canvas.draw(0, 0, g);
}

TEST_F(TestUi, add_image) {
    MAKE_CANVAS
    const auto id = canvas.add_image("/spiderman1.png");
    EXPECT_FALSE(id.empty());
    test_wait(max_image_wait); // add image sends an event and otherwise its rogue and tests get it ... later (errors)
}


TEST_F(TestUi, add_image_nay) {
    MAKE_CANVAS
    const auto id = canvas.add_image("/fii_foo_not_found.png", [](const auto& iid) {
        FAIL() << "Should not be called" << iid;
    });
    EXPECT_FALSE(id.empty());
    test_wait(min_image_wait);
}


TEST_F(TestUi, add_image_cb) {
    MAKE_CANVAS
    auto id_ptr = std::make_shared<std::string>();
    bool visited = false;
    const auto id = canvas.add_image("/spiderman1.png", [id_ptr, this, &visited](const auto& iid) {
       *id_ptr = iid;
       ASSERT_NE(*id_ptr, "");
       test_exit();
    });
    EXPECT_FALSE(id.empty());
    post_test([id_ptr, id](){
         EXPECT_EQ(*id_ptr, id);
    });
    timeout(max_image_wait);
}


TEST_F(TestUi, paint_test) {
    MAKE_CANVAS
    const auto id = canvas.add_image("/spiderman1.png", [this, canvas](const auto& iid) {
         canvas.paint_image(iid, 0, 0);
         test_exit();
    });
    test_wait();
}

TEST_F(TestUi, paint_test_rect) {
    MAKE_CANVAS
    const auto id = canvas.add_image("/spiderman1.png", [this, canvas](const auto& iid) {
         canvas.paint_image(iid, 0, 0, {10, 10, 10, 10});
         test_exit();
    });
    test_wait();
}

TEST_F(TestUi, paint_test_rect_clip) {
    MAKE_CANVAS
    const auto id = canvas.add_image("/spiderman1.png", [this, canvas](const auto& iid) {
         canvas.paint_image(iid, {20, 20, 20, 20}, {10, 10, 10, 10});
         test_exit();
    });
    test_wait();
}


TEST_F(TestUi, draw_command_list) {
    MAKE_CANVAS
     bool scope = false;
    canvas.draw_completed([this, &scope]() {
        test_exit();
        ASSERT_FALSE(scope);
        scope = true;
    });
    canvas.draw({
        "fillStyle", "lime",
        "save",
        "strokeStyle", "maroon",
        "fillStyle", "grey",
        "textAlign", "middle",
        "font", "50px serif",
        "strokeText", "Solenoidal serenity", 50, 510,
        "fillText", "Solenoidal serenity", 52, 512,
        "restore",
        "font", "20px monospace",
        "fillText", "Gempyre", 400, 51
        });
    timeout(max_image_wait);
    ASSERT_TRUE(scope);    
}

TEST_F(TestUi, draw_composer) {
    MAKE_CANVAS
    const auto foo = &canvas;
    bool scope = false;
    foo->draw_completed([this, &scope]() {
        test_exit();
        ASSERT_FALSE(scope);
        scope = true;
    });
    auto f = Gempyre::FrameComposer();
            f.begin_path();
            f.fill_style("red");
            f.ellipse(500, 400, 50, 75, 3.14159 / 4., 0, 2. * 3.14159);
            f.fill();
    canvas.draw(f);
    timeout(max_image_wait);
    ASSERT_TRUE(scope);    
}

TEST_F(TestUi, draw_bitmap0) {
    MAKE_CANVAS
    bool scope = false;
    canvas.draw_completed([this, &scope]() {
        FAIL() << "Empty bitmap is not drawn";
        test_exit();
        ASSERT_FALSE(scope);
        scope = true;
    });
    Gempyre::Bitmap bmp;
    canvas.draw(0, 0, bmp);
    test_wait(min_image_wait);
    ASSERT_FALSE(scope);
}

TEST_F(TestUi, draw_bitmap1) {
    MAKE_CANVAS
    canvas.draw_completed([this]() {
        test_exit();
    });
    Gempyre::Bitmap bmp(200, 200);
    canvas.draw(0, 0, bmp);
    timeout(max_image_wait);
}

TEST_F(TestUi, draw_bitmap_png) {
    MAKE_CANVAS
    canvas.draw_completed([this]() {
        test_exit();
    });
    const auto data = ui().resource("/spiderman1.png");
    ASSERT_TRUE(data);
    Gempyre::Bitmap bmp(*data);
    canvas.draw(0, 0, bmp);
    timeout(max_image_wait);
}

namespace Gempyre {
static
bool operator==(const Gempyre::Bitmap& b1, const Gempyre::Bitmap& b2) {
    if(b1.height() != b2.height()) return false;
    if(b1.width() != b2.width()) return false;
    for(auto j = 0; j < b2.height(); j++)
        for(auto i = 0; i < b2.width(); i++)
            if(b1.pixel(i, j) != b2.pixel(i, j))
                return false;
     return true;             
}

static
bool operator!=(const Gempyre::Bitmap& b1, const Gempyre::Bitmap& b2) {
    return ! (b1 == b2);
}
}

TEST(Graphics, draw_rect) {
    Gempyre::Bitmap b(20, 30);
    b.draw_rect({0, 0, 20, 30}, Gempyre::Color::Blue);
    b.draw_rect({5, 8, 5, 10}, Gempyre::Color::Green);
    ASSERT_EQ(b.width(), 20) << "bad witdh";
    ASSERT_EQ(b.height(), 30) << "bad height";
    for(auto j = 0; j < b.height(); j++) {
        for(auto i = 0; i < b.width(); i++) {
            if (j < 8 || j >= 18 || i < 5 || i >= 10) 
                ASSERT_EQ(b.pixel(i, j), Gempyre::Color::Blue) << "expect blue " << i << " " << j;
            else 
                ASSERT_EQ(b.pixel(i, j), Gempyre::Color::Green) << "expect green " << i << " " << j;
        }
    }
}

TEST(Graphics, bitmap_clone) {
    Gempyre::Bitmap b(20, 20);
    b.draw_rect({5, 5, 5, 5}, Gempyre::Color::Yellow);

    Gempyre::Bitmap b2(20, 20);
    b2.draw_rect({5, 5, 5, 5}, Gempyre::Color::Yellow);

    ASSERT_EQ(b, b2);

    const auto cloned = b.clone();
    ASSERT_EQ(b, b2);
    ASSERT_EQ(b, cloned);
    b.draw_rect({5, 5, 5, 5}, Gempyre::Color::Red);
    ASSERT_NE(b, cloned);
    ASSERT_EQ(b2, cloned);
}

TEST(Graphics, bitmap_copy) {
    Gempyre::Bitmap b(20, 20);
    b.draw_rect({5, 5, 5, 5}, Gempyre::Color::Yellow);
    const auto copied = b;
    EXPECT_EQ(b, copied);
    b.draw_rect({5, 5, 5, 5}, Gempyre::Color::Red);
    EXPECT_EQ(b, copied);
}

TEST(Graphics, bitmap_swap) {
    Gempyre::Bitmap b(20, 20);
    b.draw_rect({5, 5, 5, 5}, Gempyre::Color::Blue);
    ASSERT_EQ(b.pixel(6, 6), Gempyre::Color::Blue) << "expect Blue";
    Gempyre::Bitmap g(20, 20);
    g.draw_rect({5, 5, 5, 5}, Gempyre::Color::Green);
    ASSERT_EQ(g.pixel(6, 6), Gempyre::Color::Green) << "expect Green";
    b.swap(g);
    EXPECT_EQ(g.pixel(6, 6), Gempyre::Color::Blue) << "expected Blue";
    EXPECT_EQ(b.pixel(6, 6), Gempyre::Color::Green) << "expected Green";
}

template <int B, int E>
struct Range : public std::array<int, E - B> {
    constexpr Range() {for(auto i = 0; i < E - B; i++) (*this)[i] = i + B;}
}; 

Gempyre::Bitmap rect(int width, int height, Gempyre::Color::type color) {
    Gempyre::Bitmap b(width, height);
    b.draw_rect({0, 0, width, height}, color);
    return b;
}

TEST(Graphics, colors) {
    EXPECT_EQ(Gempyre::Color::rgb(Gempyre::Color::Red), std::string("#FF0000"));
    EXPECT_EQ(Gempyre::Color::rgb(Gempyre::Color::Blue), std::string("#0000FF"));
    EXPECT_EQ(Gempyre::Color::rgb(Gempyre::Color::Green), std::string("#00FF00"));
}

TEST(Graphics, construct) {
    using namespace Gempyre;
    const auto red_color = Color::rgb(0xFF, 0, 0);
    ASSERT_EQ(red_color, Color::Red);
    Gempyre::Bitmap b(100, 100);
    b.draw_rect({0, 0, 100, 100}, red_color);
    const auto r = b.clone();
    const Range<0, 100> range;
    for(const auto row : range) {
        for(const auto col : range) {
            ASSERT_EQ(b.pixel(col, row), red_color) << col << 'x' << row;
            ASSERT_EQ(r.pixel(col, row), red_color) << col << 'x' << row;
        }
    }
}

TEST(Graphics, bitmap_merge) {
    using namespace Gempyre;
    const auto red_color = Color::rgb(0xFF, 0, 0);
    const auto red = rect(100, 100, red_color);
    const auto blue_color = Color::rgb(0, 0x0, 0xFF);
    auto blue = rect(100, 100, blue_color);

    const Range<0, 100> range;

    const auto b = Color::rgba(blue_color);
    auto bmp = red.clone();
    bmp.merge(0, 0, blue);
    for(const auto row : range) {
        for(const auto col : range) {
            ASSERT_EQ(bmp.pixel(row, col), blue_color)  
                << Color::rgba(bmp.pixel(row, col)) << " vs " << b << " " << row << " " << col;
        }
    }
    blue = rect(10, 10, blue_color);
    bmp = red.clone();
    bmp.merge(10, 10, blue);
    for(const auto row : range) {
        for(const auto col : range) {
            if (row >= 10 && row < 20 && col >= 10 && col < 20) {
                    ASSERT_EQ(bmp.pixel(row, col), blue_color) << bmp.pixel(row, col) << " vs " << blue_color;
                } else {
                    ASSERT_EQ(bmp.pixel(row, col), red_color) <<  bmp.pixel(row, col) << " vs " << red_color << " " << row << " " << col;
                }
            }
        }

    bmp = red.clone();
    bmp.merge(-5, -5, blue);
    for(const auto row :  range) {
        for(const auto col : range) {
            if (row < 5 && col < 5) {
                ASSERT_EQ(bmp.pixel(row, col), blue_color) <<  bmp.pixel(row, col) << " vs " << blue_color << " " <<  row << " " <<  col;
            } else {
                ASSERT_EQ(bmp.pixel(row, col), red_color) << bmp.pixel(row, col) << " vs" << red_color << " " << row << " " << col;
            }
        }
    }

    bmp = red.clone();
    bmp.merge(95, 95, blue);
    for(const auto row : range) {
        for(const auto col :  range) {
            if (row >= 95 && col >= 95) {
                ASSERT_EQ(bmp.pixel(row, col), blue_color) << bmp.pixel(row, col) << " vs " <<  blue_color;
            } else {
                ASSERT_EQ(bmp.pixel(row, col), red_color) << bmp.pixel(row, col) << " vs " << red_color << " " << row << " " << col;
            }
        }
    }

    bmp = red.clone();
    bmp.merge(101, 101, blue);
    for(const auto row :  range) {
        for(const auto col : range) {
            if (row >= 95 && col >= 95) {
                ASSERT_EQ(bmp.pixel(row, col), red_color) <<  bmp.pixel(row, col) << " vs " << red_color << " " << row << " " << col;
            }
        }
    }

    bmp = red.clone();
    bmp.merge(100, 100, blue);
    for(const auto row :  range) {
        for(const auto col : range) {
            if (row >= 95 && col >= 95) {
                ASSERT_EQ(bmp.pixel(row, col), red_color) <<  bmp.pixel(row, col) << " vs " << red_color << " " << row << " " << col;
            }
        }
    }

    bmp = red.clone();
    bmp.merge(-11, -11, blue);
    for(const auto row :  range) {
        for(const auto col : range) {
            if (row >= 95 && col >= 95) {
                ASSERT_EQ(bmp.pixel(row, col), red_color) <<  bmp.pixel(row, col) << " vs " << red_color << " " << row << " " << col;
            }
        }
    }

    bmp = red.clone();
    bmp.merge(-10, -10, blue);
    for(const auto row :  range) {
        for(const auto col : range) {
            if (row >= 95 && col >= 95) {
                ASSERT_EQ(bmp.pixel(row, col), red_color) <<  bmp.pixel(row, col) << " vs " << red_color << " " << row << " " << col;
            }
        }
    }
}



