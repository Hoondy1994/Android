//
// Created by 80244960 on 2022/12/14.
//

#include <gtest/gtest.h>
#include <iostream>
#include <libseqmap.h>
#include <libpreopen.h>
#include <namespace/Namespace.h>
#include <namespace/uri.h>

//测试 获取scheme
TEST(ParseUriSchemeTest, ParseUriScheme) {

    // 1.验证uri存在scheme的情况
    std::string str_uri = "https://john.doe@www.example.com:123/forum/questions/?tag=networking&order=newest#top";
    std::string str_scheme;

    URI uri_test(str_uri);
    EXPECT_TRUE(uri_test.IsValid());
    str_scheme = uri_test.GetScheme();
    EXPECT_STREQ(str_scheme.c_str(), "https");

    str_uri = "://john.doe@www.example.com:123/forum/questions/?tag=networking&order=newest#top";
    URI uri_test_vaild(str_uri);
    EXPECT_TRUE(uri_test_vaild.IsValid());
    str_scheme = uri_test_vaild.GetScheme();
    EXPECT_STREQ(str_scheme.c_str(), "");

}

// 测试获取authority
TEST(ParseUriUserinfoTest, ParseUriUserinfo) {

    // 1.验证uri存在userinfo的情况
    std::string str_uri = "https://john.doe@www.example.com:123/forum/questions/?tag=networking&order=newest#top";
    std::string str_userinfo;

    URI uri_test(str_uri);
    EXPECT_TRUE(uri_test.IsValid());
    str_userinfo = uri_test.GetScheme();
    EXPECT_STREQ(str_userinfo.c_str(), "https");

    // 2.验证uri存在userinfo的情况且存在port
    std::string str_host;
    int in_port;
    URI uri_test_host(str_uri);
    EXPECT_TRUE(uri_test_host.IsValid());
    str_host = uri_test_host.GetHost();
    EXPECT_STREQ(str_host.c_str(), "www.example.com");

    in_port = uri_test_host.GetPort();
    EXPECT_EQ(in_port, 123);

    // 3.验证uri存在userinfo的情况且不存在port
    str_uri = "https://john.doe@www.example.com/forum/questions/?tag=networking&order=newest#top";
    URI uri_test_port(str_uri);
    EXPECT_TRUE(uri_test_port.IsValid());
    str_host = uri_test_port.GetHost();
    EXPECT_STREQ(str_host.c_str(), "www.example.com");
    in_port = uri_test_port.GetPort();
    EXPECT_EQ(in_port, 0);

    // 4.验证uri不存在authority的情况
    str_uri = "http:";
    URI uri_test_author(str_uri);
    EXPECT_FALSE(uri_test_author.IsValid());
}

// 测试获取path,query,fragment
TEST(ParseUriPathTest, ParseUriPath) {

    // 1.验证uri存在authority的情况获取path，fragment,query
    std::string str_uri = "https://john.doe@www.example.com:123/forum/questions/?tag=networking&order=newest#top";
    std::string str_path;
    std::string str_query;
    std::string str_fragment;

    URI uri_test(str_uri);
    EXPECT_TRUE(uri_test.IsValid());
    str_path = uri_test.GetPath();
    EXPECT_STREQ(str_path.c_str(), "forum/questions/");
    str_query = uri_test.GetQuery();
    EXPECT_STREQ(str_query.c_str(), "tag=networking&order=newest");
    str_fragment = uri_test.GetFragment();
    EXPECT_STREQ(str_fragment.c_str(), "top");

    // 2.验证uri存在authority,fragment不存在，query存在的情况获取path,query
    URI uri_query(str_uri);
    EXPECT_TRUE(uri_query.IsValid());
    str_path = uri_query.GetPath();
    EXPECT_STREQ(str_path.c_str(), "forum/questions/");
    str_query = uri_query.GetQuery();
    EXPECT_STREQ(str_query.c_str(), "tag=networking&order=newest");

    // 3.验证uri存在authority,fragment存在，query不存在的情况获取path,query
    URI uri_fragment(str_uri);
    EXPECT_TRUE(uri_fragment.IsValid());
    str_path = uri_fragment.GetPath();
    EXPECT_STREQ(str_path.c_str(), "forum/questions/");
    str_fragment = uri_fragment.GetFragment();
    EXPECT_STREQ(str_fragment.c_str(), "top");

    // 4.验证uri authority不存在,query,fragment 存在的情况获取path,query,fragment
    str_uri = "tel:+1-816-555-1212/kjdsfkjh?sd#+12456454";
    URI uri_test_path(str_uri);
    EXPECT_TRUE(uri_test_path.IsValid());
    str_path = uri_test_path.GetPath();
    EXPECT_STREQ(str_path.c_str(), "+1-816-555-1212/kjdsfkjh");
    str_query = uri_test_path.GetQuery();
    EXPECT_STREQ(str_query.c_str(), "sd");
    str_fragment = uri_test_path.GetFragment();
    EXPECT_STREQ(str_fragment.c_str(), "+12456454");

    // 5.验证uri存在authority不存在,query存在,fragment不存在的情况获取path
    str_uri = "tel:+1-816-555-1212/kjdsfkjh?sd";
    URI uri_test_query(str_uri);
    EXPECT_TRUE(uri_test_query.IsValid());
    str_path = uri_test_query.GetPath();
    EXPECT_STREQ(str_path.c_str(), "+1-816-555-1212/kjdsfkjh");
    str_query = uri_test_query.GetQuery();
    EXPECT_STREQ(str_query.c_str(), "sd");

    // 6.验证uri存在authority,query不存在,fragment存在的情况获取path
    str_uri = "tel:+1-816-555-1212/kjdsfkjh?sd";
    URI uri_test_fragment(str_uri);
    EXPECT_TRUE(uri_test_fragment.IsValid());
    str_path = uri_test_fragment.GetPath();
    EXPECT_STREQ(str_path.c_str(), "+1-816-555-1212/kjdsfkjh");
    str_fragment = uri_test_fragment.GetFragment();
    EXPECT_STREQ(str_fragment.c_str(), "");

}