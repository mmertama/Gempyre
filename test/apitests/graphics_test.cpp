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

TEST_F(TestUi, add_image_nay) {
    MAKE_CANVAS
    const auto id = canvas.add_image("/fii_foo_not_found.png");
    EXPECT_FALSE(id.empty());
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
    test_wait();
}




