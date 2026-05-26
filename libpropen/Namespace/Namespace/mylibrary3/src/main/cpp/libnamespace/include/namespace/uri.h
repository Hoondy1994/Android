#pragma once
#ifndef URI_PARAM_H
#define URI_PARAM_H

#include <iostream>

class URI {
public:

    // 构造函数
    URI(std::string &str_uri);
    URI(char *ch_uri);

    // 析构函数
    ~URI() = default;;

    // 获取scheme
    std::string GetScheme();

    // userinfo
    std::string GetUserInfo();

    // 获取 host
    std::string GetHost();

    // 获取port
    int GetPort();

    // 获取path
    std::string GetPath();

    // 获取path
    std::string GetQuery();

    // 获取 fragment
    std::string GetFragment();

    // 判断uri是否合法
    bool IsValid();

private:
    // 解析uri
    int ParseUri(std::string &parse_uri);

    // 清空uri
    void ClearUri();

    std::string scheme_;
    std::string userinfo_;
    std::string host_;
    std::string path_;
    std::string query_;
    std::string fragment_;
    int port_;
    bool vaild_;
};

#endif
