#include <thread>
#include <sstream>
#include <gtest/gtest.h>
#include "gempyre_utils.h"
#include "gempyre.h"
#include "gempyre_graphics.h"
#include "timequeue.h"

TEST(Unittests, has_true) {
    std::unordered_map<std::string, std::string> v1 {{"foo", "true"}};
    EXPECT_TRUE(Gempyre::Event::has_true(v1, "foo"));
    EXPECT_FALSE(Gempyre::Event::has_true(v1, "bar"));
    EXPECT_FALSE(Gempyre::Event::has_true(std::nullopt, "bar"));
    EXPECT_TRUE(Gempyre::Event::has_true(std::make_optional(v1), "foo"));
}

TEST(Unittests, iequals) {
    EXPECT_TRUE(GempyreUtils::iequals("cat", "cat"));
    EXPECT_TRUE(GempyreUtils::iequals("Cat", "cat"));
    EXPECT_TRUE(GempyreUtils::iequals("cAt", "cat"));
    EXPECT_TRUE(GempyreUtils::iequals("caT", "cat"));
    EXPECT_TRUE(GempyreUtils::iequals("CAT and Dog", "cat and dog"));

    EXPECT_FALSE(GempyreUtils::iequals("cat", "cat "));
    EXPECT_FALSE(GempyreUtils::iequals("Cat", " cat"));
    EXPECT_FALSE(GempyreUtils::iequals("caAt", "cat"));
    EXPECT_FALSE(GempyreUtils::iequals("caT", "_cat"));
    EXPECT_FALSE(GempyreUtils::iequals("CAT and  Dog", "cat and dog"));
}


TEST(Unittests, parse) {
    EXPECT_EQ(GempyreUtils::parse<int>("1"), 1);
    EXPECT_EQ(GempyreUtils::parse<int>("0x1"), 1);
    EXPECT_EQ(GempyreUtils::parse<unsigned>("1"), 1);
    EXPECT_EQ(GempyreUtils::parse<unsigned>("0x1"), 1);

    EXPECT_EQ(GempyreUtils::parse<int>("12.242"), 12);
    EXPECT_EQ(GempyreUtils::parse<unsigned>("12.242"), 12);

    EXPECT_EQ(GempyreUtils::parse<int>("-12.242"), -12);
    EXPECT_EQ(GempyreUtils::parse<unsigned>("12.242"), 12);
    EXPECT_EQ(GempyreUtils::parse<unsigned>("-12.242"), static_cast<unsigned>(-12));

    EXPECT_EQ(GempyreUtils::parse<int>("1234567"), 1234567);
    EXPECT_EQ(GempyreUtils::parse<int>("0x1234567"), 0x1234567);
    EXPECT_EQ(GempyreUtils::parse<unsigned>("1234567"), 1234567);
    EXPECT_EQ(GempyreUtils::parse<unsigned>("0x1234567"), 0x1234567);
    EXPECT_EQ(GempyreUtils::parse<unsigned>("01234567"), 01234567);

    EXPECT_EQ(GempyreUtils::parse<unsigned>("Munkki"), std::nullopt);
    EXPECT_EQ(GempyreUtils::parse<unsigned>("12F"), 12);
    EXPECT_EQ(GempyreUtils::parse<unsigned>("12.1"), 12);
    EXPECT_EQ(GempyreUtils::parse<double>("12.1"), 12.1);
    EXPECT_EQ(GempyreUtils::parse<double>("012.1"), 12.1);

    
    EXPECT_EQ(GempyreUtils::parse<unsigned>("12F", 16), 0x12F);
    EXPECT_EQ(GempyreUtils::parse<unsigned>("12.1", 10), std::nullopt);
    EXPECT_EQ(GempyreUtils::parse<double>("12.1", 10), 12.1);
    EXPECT_EQ(GempyreUtils::parse<double>("012.1", 8), 12.1);
    EXPECT_EQ(GempyreUtils::parse<unsigned>("012", 8), 012);
    EXPECT_EQ(GempyreUtils::parse<double>("012", 8), 12);
    EXPECT_EQ(GempyreUtils::parse<unsigned>("Munkki", 10), std::nullopt);
}


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

TEST(Unittests, Test_colors) {
    EXPECT_EQ(*Gempyre::Color::get_color("Magenta"), Gempyre::Color::Magenta);
    EXPECT_EQ(Gempyre::Color::get_color("Pagenta"), std::nullopt);
    EXPECT_EQ(Gempyre::Color::to_string(Gempyre::Color::Magenta), std::string("#FF00FF"));
    EXPECT_EQ(*Gempyre::Color::get_color(Gempyre::Color::to_string(Gempyre::Color::Magenta)), Gempyre::Color::Magenta);
    const std::vector<std::string_view> cyans {
        "0x00FFFF",
        "0x00FFFFFF",
        "0x00ffff",
        "0x00ffffff", 
        "0x00ffff00",
        "0x00FFFF00",
        "#00FFFF",
        "#00FFFFFF",
        "#00ffff",
        "#00ffffff",
        "#00ffff00",
        "#00FFFF00"};

    for (const auto& cy : cyans) {
        EXPECT_TRUE(Gempyre::Color::is_equal(*Gempyre::Color::get_color(cy), Gempyre::Color::Cyan)) << "cy:" << cy;
    }   
    
    constexpr auto c1 = "#112233";
    EXPECT_EQ(Gempyre::Color::get_color(c1), Gempyre::Color::rgba(0x11, 0x22, 0x33, 0xFF));
    constexpr auto c2 = "0x112233";
    EXPECT_EQ(Gempyre::Color::get_color(c2), Gempyre::Color::rgba(0x11, 0x22, 0x33, 0xFF));

    constexpr auto c3 = "0x11XX33";
    EXPECT_EQ(Gempyre::Color::get_color(c3), std::nullopt);

    EXPECT_EQ(Gempyre::Color::get_color("yellow").value(), Gempyre::Color::Yellow);
    EXPECT_EQ(Gempyre::Color::get_color("Red").value(), Gempyre::Color::Red);
}

TEST(Unittests, test_timequeue) {
    Gempyre::TimeQueue tq;
    std::vector<int> ids;
    ids.push_back(tq.append(3s, true, [](int){})); //0
    ids.push_back(tq.append(4s, true, [](int){})); //1
    ids.push_back(tq.append(1s, true, [](int){})); //2
    ids.push_back(tq.append(2s, true, [](int){})); //3
    EXPECT_EQ(ids.size(), tq.size());
    EXPECT_FALSE(tq.empty());
    for(const auto i : ids)
        EXPECT_TRUE(tq.contains(i));
    const auto top = tq.copyTop();
    EXPECT_EQ(top->id(), ids[2]);
    tq.reduce(top->currentTime());

    const auto top1 = tq.copyTop();
    EXPECT_EQ(top1->id(), ids[2]);
    EXPECT_EQ(top1->currentTime(), 0s);
    EXPECT_TRUE(tq.setPending(top1->id()));

    const auto top2 = tq.copyTop();
    EXPECT_EQ(top2->id(), ids[3]);
    EXPECT_EQ(top2->currentTime(), 1s);
    tq.reduce(top2->currentTime());
    EXPECT_TRUE(tq.setPending(top2->id()));

    const auto top3 = tq.copyTop();
    EXPECT_EQ(top3->id(), ids[0]);
    EXPECT_EQ(top3->currentTime(), 1s);
    EXPECT_TRUE(tq.setPending(top3->id()));

    const auto top4 = tq.copyTop();
    EXPECT_EQ(top4->id(), ids[1]);

    tq.remove(ids[2]);
    const auto top5 = tq.copyTop();
    EXPECT_EQ(top5->id(), ids[1]);

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
    EXPECT_EQ(top6->id(), ids[1]);

    tq.setNow(true);
    EXPECT_FALSE(tq.empty());
    for(auto i = 0U; i < tq.size(); i++) {
       const auto t = tq.copyTop();
       EXPECT_EQ(t->currentTime(), 0s);
       tq.setPending(t->id());
    }

    for(auto i = 0U; i < ids.size(); i++)
        if(i != 2)
            EXPECT_TRUE(tq.restoreIf(ids[i]));
        else
            EXPECT_FALSE(tq.restoreIf(ids[i]));

    EXPECT_TRUE(tq.copyTop());
    EXPECT_NE(tq.copyTop()->currentTime(), 0s);

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
    const auto bar = [foo](int id) {
        foo(id);
    };
    const auto id0 = mgr.append(10ms, false, bar);
    counts[id0] = 0;
    const auto id1 = mgr.append(20ms, false, bar);
    counts[id1] = 0;
    const auto id2 = mgr.append(100ms, false, bar);
    counts[id2] = 0;

    const auto id3 = mgr.append(1s, true, bar);
    counts[id3] = 0;

    int b;
    int* to_stop = &b;
    const auto id4 = mgr.append(140ms, false, [&](int id) {
        foo(id);
        if(counts[id] == 12 && id == *to_stop) {
            mgr.remove(id);
        }
    });
    counts[id4] = 0;
    b = id4;

    std::this_thread::sleep_for(5s);

#ifdef WINDOWS_OS              
    EXPECT_GT(counts[id0], 300);  // windows is more relaxed, maybe all should have the same, let's thinkabout it
#else
    EXPECT_GT(counts[id0], 400);
#endif    
    EXPECT_LT(counts[id0], 500);

#ifdef WINDOWS_OS
    EXPECT_GT(counts[id1], 100); // windows is more relaxed
 #else
    EXPECT_GT(counts[id1], 200);
 #endif   
    EXPECT_LT(counts[id1], 250);

    EXPECT_GT(counts[id2], 40);
    EXPECT_LT(counts[id2], 50);

    EXPECT_EQ(counts[id3], 1);
    EXPECT_EQ(counts[id4], 12);
}

#endif

TEST(Unittests, test_pushpath) {
    auto p = GempyreUtils::push_path("cat", "dog");
    EXPECT_EQ(p, std::string("cat/dog"));

    p = GempyreUtils::push_path(p, "mouse");
    EXPECT_EQ(p, std::string("cat/dog/mouse"));

    p = GempyreUtils::push_path(p, "");
    EXPECT_EQ(p, std::string("cat/dog/mouse/"));

    p = GempyreUtils::push_path("cat", "dog", "mouse");
    EXPECT_EQ(p, std::string("cat/dog/mouse"));
}

TEST(Unittests, test_filenames) {
    constexpr auto random_name = "/foo/bar/foobar.not";
    const auto basename = GempyreUtils::base_name(random_name, GempyreUtils::PathStyle::Unix);
    EXPECT_EQ(basename, "foobar.not");
    const auto& [n, e] = GempyreUtils::split_name(basename);
    EXPECT_EQ(n, "foobar");
    EXPECT_EQ(e, "not");
}

TEST(Unittests, test_parseArgs) {
    const char* test1[] = {"bing", 0};
    const auto& [p1, o1]  = GempyreUtils::parse_args(1, (char**) test1, {});
    EXPECT_TRUE(p1.empty());
    EXPECT_TRUE(o1.empty());

    const char* test2[] = {"bing", "bang", 0};
    const auto& [p2, o2]  = GempyreUtils::parse_args(2, (char**) test2, {});
    ASSERT_EQ(p2.size(), 1);
    EXPECT_EQ(p2[0] , std::string("bang"));
    EXPECT_TRUE(o2.empty());

    const char* test3[] = {"bing", "bang", "bong", 0};
    const auto& [p3, o3]  = GempyreUtils::parse_args(3, (char**) test3, {});
    ASSERT_EQ(p3.size(), 2);
    EXPECT_EQ(p3.at(0) , std::string("bang"));
    EXPECT_EQ(p3.at(1) , std::string("bong"));
    EXPECT_TRUE(o3.empty());

    const char* test4[] = {"bing", "-a", 0};
    const auto& [p4, o4]  = GempyreUtils::parse_args(2, (char**) test4, {});
    EXPECT_TRUE(p4.empty());
    EXPECT_TRUE(o4.empty());

    const char* test5[] = {"bing", "-a", 0};
    const auto& [p5, o5]  = GempyreUtils::parse_args(2, (char**) test5, {{"aaa", 'a', GempyreUtils::ArgType::NO_ARG}});
    EXPECT_TRUE(p5.empty());
    ASSERT_FALSE(o5.empty());
    EXPECT_TRUE(o5.find("aaa") != o5.end());

    const char* test6[] = {"bing","-a", 0};
    const auto& [p6, o6]  = GempyreUtils::parse_args(2, (char**) test6, {{"aaa", 'a', GempyreUtils::ArgType::OPT_ARG}});
    EXPECT_TRUE(p6.empty());
    ASSERT_FALSE(o6.empty());
    EXPECT_TRUE(o6.find("aaa") != o6.end());

    const char* test7[] = {"bing", "-a", 0};
    const auto& [p7, o7]  = GempyreUtils::parse_args(2, (char**) test7, {{"aaa", 'a', GempyreUtils::ArgType::REQ_ARG}});
    EXPECT_TRUE(p7.empty());
    EXPECT_TRUE(o7.empty());
    EXPECT_TRUE(o7.find("aaa") == o7.end());

    const char* test8[] = {"bing", 0};
    const auto& [p8, o8]  = GempyreUtils::parse_args(1, (char**) test8, {{"aaa", 'a', GempyreUtils::ArgType::NO_ARG}});
    EXPECT_TRUE(p8.empty());
    EXPECT_TRUE(o8.empty());
    EXPECT_TRUE(o8.find("aaa") == o8.end());

    const char* test9[] = {"bing", 0};
    const auto& [p9, o9]  = GempyreUtils::parse_args(1, (char**) test9, {{"aaa", 'a', GempyreUtils::ArgType::OPT_ARG}});
    EXPECT_TRUE(p9.empty());
    EXPECT_TRUE(o9.empty());
    EXPECT_TRUE(o9.find("aaa") == o9.end());

    const char* test10[] = {"bing", 0};
    const auto& [p10, o10]  = GempyreUtils::parse_args(1, (char**) test10, {{"aaa", 'a', GempyreUtils::ArgType::NO_ARG}});
    EXPECT_TRUE(p10.empty());
    EXPECT_TRUE(o10.empty());
    EXPECT_TRUE(o10.find("aaa") == o10.end());


    const char* test11[] = {"bing", "--aaa", 0};
    const auto& [p11, o11]  = GempyreUtils::parse_args(2, (char**) test11, {{"aaa", 'a', GempyreUtils::ArgType::OPT_ARG}});
    EXPECT_TRUE(p11.empty());
    EXPECT_FALSE(o11.empty());
    EXPECT_TRUE(o11.find("aaa") != o11.end());

    const char* test12[] = {"bing", "--aaa", 0};
    const auto& [p12, o12]  = GempyreUtils::parse_args(2, (char**) test12, {{"aaa", 'a', GempyreUtils::ArgType::REQ_ARG}});
    EXPECT_TRUE(p12.empty());
    EXPECT_TRUE(o12.empty());
    EXPECT_TRUE(o12.find("aaa") == o12.end());

    const char* test13[] = {"bing", "--aaa", 0};
    const auto& [p13, o13]  = GempyreUtils::parse_args(2, (char**) test13, {{"aaa", 'a', GempyreUtils::ArgType::NO_ARG}});
    EXPECT_TRUE(p13.empty());
    EXPECT_FALSE(o13.empty());
    EXPECT_TRUE(o13.find("aaa") != o13.end());


    const char* test14[] = {"bing", "-a", "fat", 0};
    const auto& [p14, o14]  = GempyreUtils::parse_args(3, (char**) test14, {{"aaa", 'a', GempyreUtils::ArgType::OPT_ARG}});
    EXPECT_FALSE(p14.empty());
    EXPECT_FALSE(o14.empty());
    EXPECT_EQ(std::get<1>(*o14.find("aaa")), std::string(""));

    const char* test15[] = {"bing", "-a", "fat", 0};
    const auto& [p15, o15]  = GempyreUtils::parse_args(3, (char**) test15, {{"aaa", 'a', GempyreUtils::ArgType::REQ_ARG}});
    EXPECT_TRUE(p15.empty());
    EXPECT_FALSE(o15.empty());
    EXPECT_EQ(std::get<1>(*o15.find("aaa")), std::string("fat"));

    const char* test16[] = {"bing", "--aaa", "fat", 0};
    const auto& [p16, o16]  = GempyreUtils::parse_args(3, (char**) test16, {{"aaa", 'a', GempyreUtils::ArgType::NO_ARG}});
    EXPECT_FALSE(p16.empty());
    EXPECT_FALSE(o16.empty());
    EXPECT_EQ(std::get<1>(*o16.find("aaa")), std::string(""));

    const char* test17[] = {"bing", "--aaa=fat", 0};
    const auto& [p17, o17]  = GempyreUtils::parse_args(2, (char**) test17, {{"aaa", 'a', GempyreUtils::ArgType::OPT_ARG}});
    EXPECT_TRUE(p17.empty());
    EXPECT_FALSE(o17.empty());
    EXPECT_EQ(std::get<1>(*o17.find("aaa")), std::string("fat"));

    const char* test18[] = {"bing", "--aaa", "fat", 0};
    const auto& [p18, o18]  = GempyreUtils::parse_args(3, (char**) test18, {{"aaa", 'a', GempyreUtils::ArgType::REQ_ARG}});
    EXPECT_TRUE(p18.empty());
    EXPECT_FALSE(o18.empty());
    EXPECT_EQ(std::get<1>(*o18.find("aaa")), std::string("fat"));

    const char* test19[] = {"bing", "--aaa=fat", 0};
    const auto& [p19, o19]  = GempyreUtils::parse_args(2, (char**) test19, {{"aaa", 'a', GempyreUtils::ArgType::REQ_ARG}});
    EXPECT_TRUE(p19.empty());
    EXPECT_FALSE(o19.empty());
    EXPECT_EQ(std::get<1>(*o19.find("aaa")), std::string("fat"));

    const char* test20[] = {"bing", "--aaa", "fat", 0};
    const auto& [p20, o20]  = GempyreUtils::parse_args(3, (char**) test20, {{"aaa", 'a', GempyreUtils::ArgType::NO_ARG}});
    EXPECT_FALSE(p20.empty());
    EXPECT_FALSE(o20.empty());
    EXPECT_EQ(std::get<1>(*o20.find("aaa")), std::string(""));
    EXPECT_EQ(p20.at(0), std::string("fat"));

    const char* test21[] = {"bing", "bang", "--aaa=fat", "bong", 0};
    const auto& [p21, o21]  = GempyreUtils::parse_args(4, (char**) test21, {{"aaa", 'a', GempyreUtils::ArgType::REQ_ARG}});
    EXPECT_FALSE(p21.empty());
    EXPECT_FALSE(o21.empty());
    EXPECT_EQ(std::get<1>(*o21.find("aaa")), std::string("fat"));
    EXPECT_EQ(p21.at(0), std::string("bang"));
    EXPECT_EQ(p21.at(1), std::string("bong"));

    const char* test22[] = {"bing", R"(c:\last quack)", "--aaa=fat\\bat", R"(--bbb=dyne\gene)", R"(--ccc=geel/feel)", 0};
    const auto& [p22, o22]  = GempyreUtils::parse_args(5, (char**) test22, {
        {"aaa", 'a', GempyreUtils::ArgType::REQ_ARG},
        {"bbb", 'b', GempyreUtils::ArgType::REQ_ARG},
        {"ccc", 'c', GempyreUtils::ArgType::REQ_ARG}});
    ASSERT_EQ(p22.size(), 1);
    EXPECT_EQ(p22.at(0) , std::string(R"(c:\last quack)"));
    EXPECT_EQ(std::get<1>(*o22.find("aaa")), std::string(R"(fat\bat)"));
    EXPECT_EQ(std::get<1>(*o22.find("bbb")), std::string(R"(dyne\gene)"));
    EXPECT_EQ(std::get<1>(*o22.find("ccc")), std::string(R"(geel/feel)"));

    const char* test23[] = {"bing", "--aaa", "99", 0};
    const auto& [p23, o23]  = GempyreUtils::parse_args(3, (char**) test23, {{"aaa", 'a', GempyreUtils::ArgType::REQ_ARG}});
    EXPECT_TRUE(p23.empty());
    EXPECT_FALSE(o23.empty());
    EXPECT_EQ(std::get<1>(*o23.find("aaa")), std::string("99"));
    EXPECT_EQ(GempyreUtils::option_or(o23, "aaa", 100), 99);

    const char* test24[] = {"bing", "--aaa", "ccc", 0};
    const auto& [p24, o24]  = GempyreUtils::parse_args(3, (char**) test24, {{"aaa", 'a', GempyreUtils::ArgType::REQ_ARG}});
    EXPECT_TRUE(p24.empty());
    EXPECT_FALSE(o24.empty());
    EXPECT_EQ(std::get<1>(*o24.find("aaa")), std::string("ccc"));
    EXPECT_EQ(GempyreUtils::option_or(o24, "aaa", 100), 100);


}

TEST(Unittests, test_levenshtein) {
    const auto words {"aisle dignity wild deck nun discovery sniff comment glasses notice break separation performer battlefield ice menu meal snow vision circle"};
    const auto list = GempyreUtils::split<std::vector<std::string>>(words, ' ');
    auto pos = list.end();
    auto min = std::numeric_limits<int>::max();
    for(auto it = list.begin(); it != list.end(); ++it) {
        const auto d = GempyreUtils::levenshtein_distance(*it, "suparation"); // incorrectly spelled separation
        if(d < min) {
            pos = it;
            min = d;
        }
    }
    EXPECT_EQ(std::find(list.begin(), list.end(), "separation"), pos);
}

TEST(Unittests, test_log) {
    const auto temp = GempyreUtils::temp_name();
    do {
    GempyreUtils::FileLogWriter flw(temp);
    GempyreUtils::set_log_level(GempyreUtils::LogLevel::Debug);
    GempyreUtils::log_debug("foo");
    } while(false);
    const auto content = GempyreUtils::chop(GempyreUtils::slurp(temp));
    EXPECT_TRUE(!content.empty());
    const auto list = GempyreUtils::split<std::vector<std::string>>(content);
    EXPECT_TRUE(std::find(list.begin(), list.end(), "foo") != list.end());
    GempyreUtils::remove_file(temp);
}

TEST(Unitests, logs) {
    GempyreUtils::set_log_level(GempyreUtils::LogLevel::Debug_Trace);
    GempyreUtils::log(GempyreUtils::LogLevel::Debug_Trace, "White background");
    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Yellow");
    GempyreUtils::log(GempyreUtils::LogLevel::Info, "Cyan");
    GempyreUtils::log(GempyreUtils::LogLevel::Warning, "Orange Black text");
    GempyreUtils::log(GempyreUtils::LogLevel::Error, "Red Black text");
}

TEST(Unitests, fatal_logs) {
    EXPECT_EXIT(GempyreUtils::log(GempyreUtils::LogLevel::Fatal, "Red underscore"),
     testing::ExitedWithCode(99), "");
}

TEST(Unittests, trim_test) {
    std::stringstream ss;
    ss <<  GempyreUtils::ltrim(" d  Hello World < ") << "_" << GempyreUtils::rtrim(" q xxx t  \t ") << "_" 
    << GempyreUtils::trim(" a v ") << "_" << GempyreUtils::trim("") << "_" << GempyreUtils::trim("   ") <<"_"<< GempyreUtils::trim("ganesh");
   EXPECT_EQ(ss.str(), std::string("d  Hello World < _ q xxx t_a v___ganesh")); 
}


TEST(Unittests, json_test_compact) {
    const std::string_view json = R"({"name":1})";
    const auto j = GempyreUtils::json_to_any(json);
    ASSERT_TRUE(j) << "Bad Json" << json;
    const auto js = GempyreUtils::to_json_string(*j);
    ASSERT_TRUE(js);
    EXPECT_EQ(json, *js);
}

TEST(Unittests, json_test_pretty) {
    const std::string_view json =
        R"({
 "name": 1
})";
    const auto j = GempyreUtils::json_to_any(json);
    ASSERT_TRUE(j);
    const auto js = GempyreUtils::to_json_string(*j, GempyreUtils::JsonMode::Pretty);
    ASSERT_TRUE(js);
    EXPECT_EQ(json, *js);
}

TEST(Unittests, json_test_get) {
    const std::string_view json = R"({"name":1, "sub": {"a": "aaa", "f": [1, 2, 3, 4, 5]}})";
    const auto j = GempyreUtils::json_to_any(json);
    ASSERT_TRUE(j);
    const auto name = GempyreUtils::get_json_value(*j, "name");
    EXPECT_TRUE(name);
    EXPECT_TRUE(std::holds_alternative<int>(*name));
    EXPECT_EQ(std::get<int>(*name), 1);
    const auto a = GempyreUtils::get_json_value(*j, "sub/a");
    EXPECT_TRUE(a);
    EXPECT_EQ(std::get<std::string>(*a), std::string{"aaa"});
    const auto bad = GempyreUtils::get_json_value(*j, "sup");
    EXPECT_FALSE(bad);
    const auto f = GempyreUtils::get_json_value(*j, "sub/f/2");
    EXPECT_TRUE(f);
    EXPECT_EQ(std::get<int>(*f), 3);
}

TEST(Unittests, json_test_set1) {
    const std::string_view json = R"({"name":false, "sub": {"bb": 83.4, "f": [1, 2, 3, 4, 5]}})";
    auto j = GempyreUtils::json_to_any(json);
    ASSERT_TRUE(j);
    auto ok = GempyreUtils::set_json_value(*j, "name", true);
    EXPECT_TRUE(ok);
    
    const auto name = GempyreUtils::get_json_value(*j, "name");
    EXPECT_TRUE(name);
    EXPECT_TRUE(std::holds_alternative<bool>(*name));
    EXPECT_EQ(std::get<bool>(*name), true);

    ASSERT_TRUE(j);
    ok = GempyreUtils::set_json_value(*j, "sub/bb", 91.3);
    EXPECT_TRUE(ok);

    const auto v = GempyreUtils::get_json_value(*j, "sub/bb");
    EXPECT_TRUE(v);
    EXPECT_EQ(std::get<double>(*v), 91.3);

    const auto js = GempyreUtils::to_json_string(*j, GempyreUtils::JsonMode::Compact);
    ASSERT_TRUE(js);
    const std::string_view json1 = R"({"name":true,"sub":{"bb":91.3,"f":[1,2,3,4,5]}})";
    EXPECT_EQ(json1, *js);
}

TEST(Unittests, json_test_set2) {
    const std::string_view json = R"({"name":false, "sub": {"bb": 83.4, "f": [1, 2, 3, 4, 5]}})";
    auto j = GempyreUtils::json_to_any(json);
    ASSERT_TRUE(j);

    auto nome = GempyreUtils::get_json_value(*j, "nome");
    EXPECT_FALSE(nome);

#ifdef GTEST_OS_WINDOWS
    auto ok = GempyreUtils::set_json_value(*j, "nome", std::string{"quack"});
#else
    auto ok = GempyreUtils::set_json_value(*j, "nome", "quack");
#endif
    EXPECT_TRUE(ok);

    nome = GempyreUtils::get_json_value(*j, "nome");
    EXPECT_TRUE(nome);

    EXPECT_EQ("quack", std::get<std::string>(*nome));

    ok = GempyreUtils::set_json_value(*j, "sub/f/7", 13);
    EXPECT_TRUE(ok);

    auto v4 = GempyreUtils::get_json_value(*j, "sub/f/4");
    EXPECT_TRUE(v4);
    EXPECT_EQ(std::get<int>(*v4), 5);

    auto v5 = GempyreUtils::get_json_value(*j, "sub/f/5");
    EXPECT_FALSE(v5);

    auto v6 = GempyreUtils::get_json_value(*j, "sub/f/6");
    EXPECT_FALSE(v6);

    auto v7 = GempyreUtils::get_json_value(*j, "sub/f/7");
    EXPECT_TRUE(v7);
    EXPECT_EQ(std::get<int>(*v7), 13);
}

TEST(Unittests, json_test_set3) {
    const std::string_view json = R"({})";
    auto j = GempyreUtils::json_to_any(json);
    ASSERT_TRUE(j);    
    auto a = GempyreUtils::set_json_value(*j, "a", std::map<std::string, std::any>{});
    EXPECT_TRUE(a);
#ifdef GTEST_OS_WINDOWS
    const auto b = GempyreUtils::set_json_value(*j, "a/b", std::string{"banana"});
#else
    const auto b = GempyreUtils::set_json_value(*j, "a/b", "banana");
#endif
    EXPECT_TRUE(b);
    auto c = GempyreUtils::set_json_value(*j, "c", std::vector<std::any>{});
    EXPECT_TRUE(c);
    const auto c0 = GempyreUtils::set_json_value(*j, "c/0", false);
    EXPECT_TRUE(c0);
    const auto c1 = GempyreUtils::set_json_value(*j, "c/1", true);
    EXPECT_TRUE(c1);

    auto js = GempyreUtils::to_json_string(*j, GempyreUtils::JsonMode::Compact);
    ASSERT_TRUE(js) << js.error();
    const std::string_view json1 = R"({"a":{"b":"banana"},"c":[false,true]})";
    EXPECT_EQ(json1, *js);

    c = GempyreUtils::set_json_value(*j, "c", std::vector< std::any>{});
    EXPECT_TRUE(c);

    js = GempyreUtils::to_json_string(*j, GempyreUtils::JsonMode::Compact);
    ASSERT_TRUE(js);
    const std::string_view json2 = R"({"a":{"b":"banana"},"c":[]})";
    EXPECT_EQ(json2, *js);

    a = GempyreUtils::remove_json_value(*j, "a");
    EXPECT_TRUE(a);

    js = GempyreUtils::to_json_string(*j, GempyreUtils::JsonMode::Compact);
    ASSERT_TRUE(js);
    const std::string_view json3 = R"({"c":[]})";
    EXPECT_EQ(json3, *js);
}

TEST(Unittests, json_test_set4) {
    auto j = GempyreUtils::json_to_any("{}");
    ASSERT_TRUE(j);
    const auto ok = GempyreUtils::make_json_path(*j, "animals/0/cat");
    ASSERT_TRUE(ok) << ok.error();
    auto js = GempyreUtils::to_json_string(*j, GempyreUtils::JsonMode::Compact);
    ASSERT_TRUE(js);
    ASSERT_EQ(*js, R"({"animals":[{}]})");
    GempyreUtils::set_json_value(*j, "animals/0/cat", std::string{"meow"});
    GempyreUtils::make_json_path(*j, "animals/1/dog");
    GempyreUtils::set_json_value(*j, "animals/1/dog", std::string{"bark"});
    GempyreUtils::set_json_value(*j, "animals/0/food", std::string{"fish"});
    GempyreUtils::set_json_value(*j, "animals/1/food", std::string{"bone"});
    js = GempyreUtils::to_json_string(*j, GempyreUtils::JsonMode::Compact);
    ASSERT_EQ(*js, R"({"animals":[{"cat":"meow","food":"fish"},{"dog":"bark","food":"bone"}]})");
}

TEST(Unittests, join1) {
    const std::vector<int> v{1,2,3,4,5};
    const auto joined = GempyreUtils::join(v, "-");
    EXPECT_EQ(joined, "1-2-3-4-5");
}

TEST(Unittests, join2) {
    const std::vector<int> v{1,2,3,4,5};
    const auto joined = GempyreUtils::join(v, "", [](auto p){return std::to_string(p + 1);});
    EXPECT_EQ(joined, "23456");
}

TEST(Unittests, join3) {
    const std::vector<int> v{1,2,3,4,5};
    const auto joined = GempyreUtils::join(std::next(v.begin()), std::prev(v.end()), " ");
    EXPECT_EQ(joined, "2 3 4");
}

TEST(Unittests, join4) {
    const char* strs[] = {"aa", "bee", "cee", "dee"};
    const auto joined = GempyreUtils::join(std::begin(strs), std::end(strs), "_");
    EXPECT_EQ(joined, "aa_bee_cee_dee");
}

TEST(Unittests, join5) {
    const char* strs[] = {"aa", "bee", "cee", "dee"};
    const auto joined = GempyreUtils::join(std::begin(strs), std::end(strs), "_", [](const auto& s) {return GempyreUtils::to_upper(std::string_view{s});});
    EXPECT_EQ(joined, "AA_BEE_CEE_DEE");
}

TEST(Unittests, join6) {
    const std::array<std::string_view, 4> strs = {"aa", "bee", "cee", "dee"};
    const auto joined = GempyreUtils::join(strs, "_", [](const auto& s) {return GempyreUtils::to_upper(std::string_view{s});});
    EXPECT_EQ(joined, "AA_BEE_CEE_DEE");
}

TEST(Unittests, join7) {
    const std::array<int, 4> strs = {1, 2, 3, 4};
    const auto joined = GempyreUtils::join(strs, ",");
    EXPECT_EQ(joined, "1,2,3,4");
}

TEST(Unittests, join8) {
    const std::array<std::pair<int, int>, 4> strs {{{1, 2}, {3,4}, {5,6}, {7,8}}};
    const auto joined = GempyreUtils::join(strs, ",", [](const auto& a){return a.first + a.second;});
    EXPECT_EQ(joined, "3,7,11,15");
}

TEST(Unittests, join9) {
    const std::array<std::array<int, 2>, 4> strs {{{1, 2}, {3,4}, {5,6}, {7,8}}};
    const auto joined = GempyreUtils::join(std::begin(strs), std::end(strs), ",", [](const auto& a) {
        return std::to_string(a[0]) + 'x' + std::to_string(a[1]);});
    EXPECT_EQ(joined, "1x2,3x4,5x6,7x8");
}

TEST(Unittests, join10) {
    const std::array<std::array<int, 2>, 4> strs {{{1, 2}, {3,4}, {5,6}, {7,8}}};
    const auto joined = GempyreUtils::join(strs, ",", [](const auto& a) {
        return std::to_string(a[0]) + 'x' + std::to_string(a[1]);});
    EXPECT_EQ(joined, "1x2,3x4,5x6,7x8");
}


int main(int argc, char **argv) {
   ::testing::InitGoogleTest(&argc, argv);
   for(int i = 1 ; i < argc; ++i)
       if(argv[i] == std::string_view("--verbose"))
            Gempyre::set_debug();
  return RUN_ALL_TESTS();
}

#if 0
TEST(Unittests, app_time) {
    const auto p = GempyreUtils::appPath();
    const auto path = GempyreUtils::split<std::vector<std::string>>(p, '/');
    ASSERT_TRUE(p.size() > 0);

    const auto t = GempyreUtils::timeStamp(p);
    EXPECT_GT(t, 1644441122);
    EXPECT_LT(t, 3159205922);
}
#endif


