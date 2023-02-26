#include "gempyre_test.h"
#include "gempyre_graphics.h"

using namespace std::chrono_literals;
using namespace GempyreTest;

#define MAKE_CANVAS Gempyre::CanvasElement canvas(*m_ui, "canvas", m_ui->root());

TEST_F(TestUi, make_canvas) {
    MAKE_CANVAS    
}


TEST_F(TestUi, add_image) {
    MAKE_CANVAS
    const auto id = canvas.add_image("/spiderman1.png");
    EXPECT_FALSE(id.empty());
}

constexpr auto max_image_wait = 3s;

TEST_F(TestUi, add_image_nay) {
    MAKE_CANVAS
    const auto id = canvas.add_image("/fii_foo_not_found.png", [](const auto& iid) {
        FAIL() << "Should not be called" << iid;
    });
    EXPECT_FALSE(id.empty());
    test_wait(max_image_wait);
}

TEST_F(TestUi, add_image_cb) {
    MAKE_CANVAS
    auto id_ptr = std::make_shared<std::string>();
    const auto id = canvas.add_image("/spiderman1.png", [id_ptr, this](const auto& iid) {
       *id_ptr = iid;
       m_ui->exit();
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
         m_ui->exit();
    });
    test_wait();
}

TEST_F(TestUi, paint_test_rect) {
    MAKE_CANVAS
    const auto id = canvas.add_image("/spiderman1.png", [this, canvas](const auto& iid) {
         canvas.paint_image(iid, 0, 0, {10, 10, 10, 10});
         m_ui->exit();
    });
    test_wait();
}

TEST_F(TestUi, paint_test_rect_clip) {
    MAKE_CANVAS
    const auto id = canvas.add_image("/spiderman1.png", [this, canvas](const auto& iid) {
         canvas.paint_image(iid, {20, 20, 20, 20}, {10, 10, 10, 10});
         m_ui->exit();
    });
    test_wait();
}


TEST_F(TestUi, draw_command_list) {
    MAKE_CANVAS
    canvas.draw_completed([this]() {
        m_ui->exit();
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
    test_wait();    
}

TEST_F(TestUi, draw_composer) {
    MAKE_CANVAS
    canvas.draw_completed([this]() {
        m_ui->exit();
    });
    auto f = Gempyre::FrameComposer();
            f.begin_path();
            f.fill_style("red");
            f.ellipse(500, 400, 50, 75, 3.14159 / 4., 0, 2. * 3.14159);
            f.fill();
    canvas.draw(f);
    test_wait();    
}

TEST_F(TestUi, draw_bitmap0) {
    MAKE_CANVAS
    canvas.draw_completed([this]() {
        FAIL() << "Empty bitmap is not drawn";
        m_ui->exit();
    });
    Gempyre::Bitmap bmp;
    canvas.draw(bmp, 0, 0);
    test_wait(3s);
}

TEST_F(TestUi, draw_bitmap1) {
    MAKE_CANVAS
    canvas.draw_completed([this]() {
        m_ui->exit();
    });
    Gempyre::Bitmap bmp(200, 200);
    canvas.draw(bmp, 0, 0);
    timeout(10s);
}

TEST_F(TestUi, draw_bitmap_png) {
    MAKE_CANVAS
    canvas.draw_completed([this]() {
        m_ui->exit();
    });
    const auto data = m_ui->resource("/spiderman1.png");
    ASSERT_TRUE(data);
    Gempyre::Bitmap bmp(*data);
    canvas.draw(bmp, 0, 0);
    timeout(10s);
}



