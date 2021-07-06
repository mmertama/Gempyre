#include <gtest/gtest.h>
#include "gempyre.h"
#include "gempyre_graphics.h"

#include "timequeue.h"

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


TEST(Unittests, test_timequeue) {
    Gempyre::TimeQueue tq;
    std::vector<int> ids;
    ids.push_back(tq.append(3s, [](int){})); //0
    ids.push_back(tq.append(4s, [](int){})); //1
    ids.push_back(tq.append(1s, [](int){})); //2
    ids.push_back(tq.append(2s, [](int){})); //3
    EXPECT_EQ(ids.size(), tq.size());
    EXPECT_FALSE(tq.empty());
    for(const auto i : ids)
        EXPECT_TRUE(tq.contains(i));
    const auto top = tq.copyTop();
    EXPECT_EQ(top->id, ids[2]);
    tq.reduce(top->currentTime);

    const auto top1 = tq.copyTop();
    EXPECT_EQ(top1->id, ids[2]);
    EXPECT_EQ(top1->currentTime, 0s);
    EXPECT_TRUE(tq.setPending(top1->id));

    const auto top2 = tq.copyTop();
    EXPECT_EQ(top2->id, ids[3]);
    EXPECT_EQ(top2->currentTime, 1s);
    tq.reduce(top2->currentTime);
    EXPECT_TRUE(tq.setPending(top2->id));

    const auto top3 = tq.copyTop();
    EXPECT_EQ(top3->id, ids[0]);
    EXPECT_EQ(top3->currentTime, 1s);
    EXPECT_TRUE(tq.setPending(top3->id));

    const auto top4 = tq.copyTop();
    EXPECT_EQ(top4->id, ids[1]);

    tq.remove(ids[2]);
    const auto top5 = tq.copyTop();
    EXPECT_EQ(top5->id, ids[1]);

    EXPECT_EQ(ids.size() - 1, tq.size());
    EXPECT_FALSE(tq.empty());

    for(auto i = 0U; i < ids.size(); i++)
        if(i != 2)
            EXPECT_TRUE(tq.contains(ids[i]));
        else
            EXPECT_FALSE(tq.contains(ids[i]));

    EXPECT_FALSE(tq.restoreIf(ids[2]));
    EXPECT_TRUE(tq.restoreIf(ids[3]));
    EXPECT_TRUE(tq.restoreIf(ids[0]));

    //now when 1 is removed, 2 and 3 restored, 4 shall be on top

    const auto top6 = tq.copyTop();
    EXPECT_EQ(top6->id, ids[1]);

    tq.setNow(true);
    EXPECT_FALSE(tq.empty());
    for(auto i = 0U; i < tq.size(); i++) {
       const auto t = tq.copyTop();
       EXPECT_EQ(t->currentTime, 0s);
       tq.setPending(t->id);
    }

    for(auto i = 0U; i < ids.size(); i++)
        if(i != 2)
            EXPECT_TRUE(tq.restoreIf(ids[i]));
        else
            EXPECT_FALSE(tq.restoreIf(ids[i]));

    EXPECT_TRUE(tq.copyTop());
    EXPECT_NE(tq.copyTop()->currentTime, 0s);

    tq.clear();

    EXPECT_TRUE(tq.empty());
}

TEST(Unittests, test_timermgr) {
    Gempyre::TimerMgr mgr;
    std::map<int, int> counts;
    const auto foo = [&counts] (int id) {
        EXPECT_TRUE(counts.find(id) != counts.end());
        ++counts[id];
        };
    const auto bar = [](const std::function<void()>& f) {
        f();
    };
    const auto id0 = mgr.append(10ms, false, foo, bar);
    counts[id0] = 0;
    const auto id1 = mgr.append(20ms, false, foo, bar);
    counts[id1] = 0;
    const auto id2 = mgr.append(100ms, false, foo, bar);
    counts[id2] = 0;

    const auto id3 = mgr.append(1s, true, foo, bar);
    counts[id3] = 0;

    int* to_stop;
    const auto id4 = mgr.append(140ms, false, [&](int id) {
        foo(id);
        if(counts[id] == 12 && id == *to_stop) {
            mgr.remove(id);
        }
    }, bar);
    counts[id4] = 0;
    *to_stop = id4;

    std::this_thread::sleep_for(5s);

    EXPECT_GT(counts[id0], 400);
    EXPECT_LT(counts[id0], 500);

    EXPECT_GT(counts[id1], 200);
    EXPECT_LT(counts[id1], 250);

    EXPECT_GT(counts[id2], 40);
    EXPECT_LT(counts[id2], 50);

    EXPECT_EQ(counts[id3], 1);
    EXPECT_EQ(counts[id4], 12);
}

int main(int argc, char **argv) {
   ::testing::InitGoogleTest(&argc, argv);
   for(int i = 1 ; i < argc; ++i)
       if(argv[i] == std::string_view("--verbose"))
            Gempyre::setDebug();
  return RUN_ALL_TESTS();
}
