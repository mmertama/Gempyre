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

#if !defined(CI_ACTIONS) // Actions fails with these time limits (sometimes), we skip

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

    int b;
    int* to_stop = &b;
    const auto id4 = mgr.append(140ms, false, [&](int id) {
        foo(id);
        if(counts[id] == 12 && id == *to_stop) {
            mgr.remove(id);
        }
    }, bar);
    counts[id4] = 0;
    b = id4;

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

#endif

TEST(Unittests, test_pushpath) {
    auto p = GempyreUtils::pushPath("cat", "dog");
#ifdef WIN_OS
     EXPECT_EQ(p, std::string("cat\\dog"));
#else
     EXPECT_EQ(p, std::string("cat/dog"));
#endif
      p = GempyreUtils::pushPath(p, "mouse");
#ifdef WIN_OS
     EXPECT_EQ(p, std::string("cat\\dog\\mouse"));
#else
     EXPECT_EQ(p, std::string("cat/dog/mouse"));
#endif
      p = GempyreUtils::pushPath(p, "");
#ifdef WIN_OS
     EXPECT_EQ(p, std::string("cat\\dog\\mouse\\"));
#else
     EXPECT_EQ(p, std::string("cat/dog/mouse/"));
#endif

    p = GempyreUtils::pushPath("cat", "dog", "mouse");
 #ifdef WIN_OS
      EXPECT_EQ(p, std::string("cat\\dog\\mouse"));
 #else
      EXPECT_EQ(p, std::string("cat/dog/mouse"));
#endif
}

TEST(Unittests, test_parseArgs) {
    const char* test1[] = {"bing", 0};
    const auto& [p1, o1]  = GempyreUtils::parseArgs(1, (char**) test1, {});
    EXPECT_TRUE(p1.empty());
    EXPECT_TRUE(o1.empty());

    const char* test2[] = {"bing", "bang", 0};
    const auto& [p2, o2]  = GempyreUtils::parseArgs(2, (char**) test2, {});
    ASSERT_EQ(p2.size(), 1);
    EXPECT_EQ(p2[0] , std::string("bang"));
    EXPECT_TRUE(o2.empty());

    const char* test3[] = {"bing", "bang", "bong", 0};
    const auto& [p3, o3]  = GempyreUtils::parseArgs(3, (char**) test3, {});
    ASSERT_EQ(p3.size(), 2);
    EXPECT_EQ(p3.at(0) , std::string("bang"));
    EXPECT_EQ(p3.at(1) , std::string("bong"));
    EXPECT_TRUE(o3.empty());

    const char* test4[] = {"bing", "-a", 0};
    const auto& [p4, o4]  = GempyreUtils::parseArgs(2, (char**) test4, {});
    EXPECT_TRUE(p4.empty());
    EXPECT_TRUE(o4.empty());

    const char* test5[] = {"bing", "-a", 0};
    const auto& [p5, o5]  = GempyreUtils::parseArgs(2, (char**) test5, {{"aaa", 'a', GempyreUtils::ArgType::NO_ARG}});
    EXPECT_TRUE(p5.empty());
    ASSERT_FALSE(o5.empty());
    EXPECT_TRUE(o5.find("aaa") != o5.end());

    const char* test6[] = {"bing","-a", 0};
    const auto& [p6, o6]  = GempyreUtils::parseArgs(2, (char**) test6, {{"aaa", 'a', GempyreUtils::ArgType::OPT_ARG}});
    EXPECT_TRUE(p6.empty());
    ASSERT_FALSE(o6.empty());
    EXPECT_TRUE(o6.find("aaa") != o6.end());

    const char* test7[] = {"bing", "-a", 0};
    const auto& [p7, o7]  = GempyreUtils::parseArgs(2, (char**) test7, {{"aaa", 'a', GempyreUtils::ArgType::REQ_ARG}});
    EXPECT_TRUE(p7.empty());
    EXPECT_TRUE(o7.empty());
    EXPECT_TRUE(o7.find("aaa") == o7.end());

    const char* test8[] = {"bing", 0};
    const auto& [p8, o8]  = GempyreUtils::parseArgs(1, (char**) test8, {{"aaa", 'a', GempyreUtils::ArgType::NO_ARG}});
    EXPECT_TRUE(p8.empty());
    EXPECT_TRUE(o8.empty());
    EXPECT_TRUE(o8.find("aaa") == o8.end());

    const char* test9[] = {"bing", 0};
    const auto& [p9, o9]  = GempyreUtils::parseArgs(1, (char**) test9, {{"aaa", 'a', GempyreUtils::ArgType::OPT_ARG}});
    EXPECT_TRUE(p9.empty());
    EXPECT_TRUE(o9.empty());
    EXPECT_TRUE(o9.find("aaa") == o9.end());

    const char* test10[] = {"bing", 0};
    const auto& [p10, o10]  = GempyreUtils::parseArgs(1, (char**) test10, {{"aaa", 'a', GempyreUtils::ArgType::NO_ARG}});
    EXPECT_TRUE(p10.empty());
    EXPECT_TRUE(o10.empty());
    EXPECT_TRUE(o10.find("aaa") == o10.end());


    const char* test11[] = {"bing", "--aaa", 0};
    const auto& [p11, o11]  = GempyreUtils::parseArgs(2, (char**) test11, {{"aaa", 'a', GempyreUtils::ArgType::OPT_ARG}});
    EXPECT_TRUE(p11.empty());
    EXPECT_FALSE(o11.empty());
    EXPECT_TRUE(o11.find("aaa") != o11.end());

    const char* test12[] = {"bing", "--aaa", 0};
    const auto& [p12, o12]  = GempyreUtils::parseArgs(2, (char**) test12, {{"aaa", 'a', GempyreUtils::ArgType::REQ_ARG}});
    EXPECT_TRUE(p12.empty());
    EXPECT_TRUE(o12.empty());
    EXPECT_TRUE(o12.find("aaa") == o12.end());

    const char* test13[] = {"bing", "--aaa", 0};
    const auto& [p13, o13]  = GempyreUtils::parseArgs(2, (char**) test13, {{"aaa", 'a', GempyreUtils::ArgType::NO_ARG}});
    EXPECT_TRUE(p13.empty());
    EXPECT_FALSE(o13.empty());
    EXPECT_TRUE(o13.find("aaa") != o13.end());


    const char* test14[] = {"bing", "-a", "fat", 0};
    const auto& [p14, o14]  = GempyreUtils::parseArgs(3, (char**) test14, {{"aaa", 'a', GempyreUtils::ArgType::OPT_ARG}});
    EXPECT_FALSE(p14.empty());
    EXPECT_FALSE(o14.empty());
    EXPECT_EQ(std::get<1>(*o14.find("aaa")), std::string(""));

    const char* test15[] = {"bing", "-a", "fat", 0};
    const auto& [p15, o15]  = GempyreUtils::parseArgs(3, (char**) test15, {{"aaa", 'a', GempyreUtils::ArgType::REQ_ARG}});
    EXPECT_TRUE(p15.empty());
    EXPECT_FALSE(o15.empty());
    EXPECT_EQ(std::get<1>(*o15.find("aaa")), std::string("fat"));

    const char* test16[] = {"bing", "--aaa", "fat", 0};
    const auto& [p16, o16]  = GempyreUtils::parseArgs(3, (char**) test16, {{"aaa", 'a', GempyreUtils::ArgType::NO_ARG}});
    EXPECT_FALSE(p16.empty());
    EXPECT_FALSE(o16.empty());
    EXPECT_EQ(std::get<1>(*o16.find("aaa")), std::string(""));

    const char* test17[] = {"bing", "--aaa=fat", 0};
    const auto& [p17, o17]  = GempyreUtils::parseArgs(2, (char**) test17, {{"aaa", 'a', GempyreUtils::ArgType::OPT_ARG}});
    EXPECT_TRUE(p17.empty());
    EXPECT_FALSE(o17.empty());
    EXPECT_EQ(std::get<1>(*o17.find("aaa")), std::string("fat"));

    const char* test18[] = {"bing", "--aaa", "fat", 0};
    const auto& [p18, o18]  = GempyreUtils::parseArgs(3, (char**) test18, {{"aaa", 'a', GempyreUtils::ArgType::REQ_ARG}});
    EXPECT_TRUE(p18.empty());
    EXPECT_FALSE(o18.empty());
    EXPECT_EQ(std::get<1>(*o18.find("aaa")), std::string("fat"));

    const char* test19[] = {"bing", "--aaa=fat", 0};
    const auto& [p19, o19]  = GempyreUtils::parseArgs(2, (char**) test19, {{"aaa", 'a', GempyreUtils::ArgType::REQ_ARG}});
    EXPECT_TRUE(p19.empty());
    EXPECT_FALSE(o19.empty());
    EXPECT_EQ(std::get<1>(*o19.find("aaa")), std::string("fat"));

    const char* test20[] = {"bing", "--aaa", "fat", 0};
    const auto& [p20, o20]  = GempyreUtils::parseArgs(3, (char**) test20, {{"aaa", 'a', GempyreUtils::ArgType::NO_ARG}});
    EXPECT_FALSE(p20.empty());
    EXPECT_FALSE(o20.empty());
    EXPECT_EQ(std::get<1>(*o16.find("aaa")), std::string(""));
    EXPECT_EQ(p20.at(0), std::string("fat"));

    const char* test21[] = {"bing", "bang", "--aaa=fat", "bong", 0};
    const auto& [p21, o21]  = GempyreUtils::parseArgs(4, (char**) test21, {{"aaa", 'a', GempyreUtils::ArgType::REQ_ARG}});
    EXPECT_FALSE(p21.empty());
    EXPECT_FALSE(o21.empty());
    EXPECT_EQ(std::get<1>(*o21.find("aaa")), std::string("fat"));
    EXPECT_EQ(p21.at(0), std::string("bang"));
    EXPECT_EQ(p21.at(1), std::string("bong"));

    const char* test22[] = {"bing", R"(c:\last quack)", "--aaa=fat\\bat", R"(--bbb=dyne\gene)", R"(--ccc=geel/feel)", 0};
    const auto& [p22, o22]  = GempyreUtils::parseArgs(5, (char**) test22, {
        {"aaa", 'a', GempyreUtils::ArgType::REQ_ARG},
        {"bbb", 'b', GempyreUtils::ArgType::REQ_ARG},
        {"ccc", 'c', GempyreUtils::ArgType::REQ_ARG}});
    ASSERT_EQ(p22.size(), 1);
    EXPECT_EQ(p22.at(0) , std::string(R"(c:\last quack)"));
    EXPECT_EQ(std::get<1>(*o22.find("aaa")), std::string(R"(fat\bat)"));
    EXPECT_EQ(std::get<1>(*o22.find("bbb")), std::string(R"(dyne\gene)"));
    EXPECT_EQ(std::get<1>(*o22.find("ccc")), std::string(R"(geel/feel)"));

}

TEST(Unittests, test_levenshtein) {
    const auto words {"aisle dignity wild deck nun discovery sniff comment glasses notice break separation performer battlefield ice menu meal snow vision circle"};
    const auto list = GempyreUtils::split<std::vector<std::string>>(words, ' ');
    auto pos = list.end();
    auto min = std::numeric_limits<int>::max();
    for(auto it = list.begin(); it != list.end(); ++it) {
        const auto d = GempyreUtils::levenshteinDistance(*it, "suparation"); // incorrecly spelled separation
        if(d < min) {
            pos = it;
            min = d;
        }
    }
    EXPECT_EQ(std::find(list.begin(), list.end(), "separation"), pos);
}

TEST(Unittests, test_log) {
    const auto temp = GempyreUtils::tempName();
    do {
    GempyreUtils::FileLogWriter flw(temp);
    GempyreUtils::setLogWriter(&flw);
    GempyreUtils::setLogLevel(GempyreUtils::LogLevel::Debug);
    GempyreUtils::logDebug("foo");
    } while(false);
    const auto content = GempyreUtils::chop(GempyreUtils::slurp(temp));
    EXPECT_TRUE(!content.empty());
    const auto list = GempyreUtils::split<std::vector<std::string>>(content);
    EXPECT_TRUE(std::find(list.begin(), list.end(), "foo") != list.end());
    GempyreUtils::removeFile(temp);
}

int main(int argc, char **argv) {
   ::testing::InitGoogleTest(&argc, argv);
   for(int i = 1 ; i < argc; ++i)
       if(argv[i] == std::string_view("--verbose"))
            Gempyre::setDebug();
  return RUN_ALL_TESTS();
}
