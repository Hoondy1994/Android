#include <string>
#include <iostream>
#include <namespace/uri.h>

URI::URI(std::string &str_uri) {
    ClearUri();
    ParseUri(str_uri);
}

URI::URI(char *ch_uri) {
    ClearUri();
    std::string str_uri;
    if (ch_uri != nullptr) {
        str_uri = ch_uri;
    }
    ParseUri(str_uri);
}

std::string URI::GetScheme() {
    return scheme_;
}

std::string URI::GetUserInfo() {
    return userinfo_;
}


std::string URI::GetHost() {
    return host_;
}

int URI::GetPort() {
    return port_;
}

std::string URI::GetPath() {
    return path_;
}

std::string URI::GetQuery() {
    return query_;
}
std::string URI::GetFragment() {
    return fragment_;
}

// 检查uri是否合法
bool URI::IsValid() {
    return vaild_;
}

// 清空uri
void URI::ClearUri() {
    scheme_.clear();
    userinfo_.clear();
    host_.clear();
    path_.clear();
    query_.clear();
    fragment_.clear();
    port_ = 0;
    vaild_ = true;
}

int URI::ParseUri(std::string &parse_uri) {


    // 获取scheme
    int scheme_end;
    scheme_end = parse_uri.find(":");

    if (scheme_end != std::string::npos) {
        scheme_ = parse_uri.substr(0, scheme_end);
    }
    else {
        ClearUri();
        vaild_ = false;
        return -1;
    }
    // 2.获取authority 查找 // 如果找到，则存在authority
    int author_start , userinfo_end, host_end, port_end, path_start, path_end, query_end, fragment_end;
    author_start = parse_uri.find("//", scheme_end);

    if (author_start != std::string::npos) {
        // 如果找到，则说明有authority
        std::string str_author = parse_uri.substr(scheme_end + 1, 2);

        if (str_author.compare("//") != 0) {
            ClearUri();
            vaild_ = false;
            return -1;
        }
        userinfo_end = parse_uri.find("@", author_start);
        // 查找@,如果找到，则说明存在userinfo

        if (userinfo_end != std::string::npos) {
            userinfo_ = parse_uri.substr(author_start + 2, userinfo_end - author_start - 2);
            // 获取host
            host_end = parse_uri.find(":", userinfo_end);
            if (host_end != std::string::npos) {
                host_ = parse_uri.substr(userinfo_end + 1, host_end - userinfo_end - 1);
                // 如果存在port,则获取port
                port_end = parse_uri.find("/", host_end);
                if (port_end != std::string::npos) {
                    std::string str_port = parse_uri.substr(host_end + 1,port_end - host_end - 1);
                    if (std::all_of(str_port.begin(), str_port.end(), ::isdigit)) {
                        port_ = atoi(str_port.c_str());
                    }
                    path_start = port_end;
                }
                else {
                    ClearUri();
                    vaild_ = false;
                    return -1;
                }
            }
            else {
                // 不存在port
                host_end = parse_uri.find("/", userinfo_end);

                if (host_end != std::string::npos) {
                    host_ = parse_uri.substr(userinfo_end + 1, host_end - userinfo_end - 1);
                    path_start = host_end;
                }
                else {
                    ClearUri();
                    vaild_ = false;
                    return -1;
                }
            }
        }
            //如果'@'不存在，则说明不存在userinfo
        else {
            host_end = parse_uri.find(":", author_start);

            if (host_end != std::string::npos) {
                host_ = parse_uri.substr(author_start + 2, host_end - author_start - 2);
                // 如果存在port,则获取port
                port_end = parse_uri.find("/", host_end);

                if (port_end != std::string::npos) {
                    std::string str_port = parse_uri.substr(host_end + 1, port_end - host_end - 1);
                    if (std::all_of(str_port.begin(), str_port.end(), ::isdigit)) {
                        port_ = atoi(str_port.c_str());
                        path_start = port_end;
                    }
                }
                else {
                    ClearUri();
                    vaild_ = false;
                    return -1;
                }
            }
            else {
                host_end = parse_uri.find("/", author_start + 2);

                if (host_end != std::string::npos) {
                    host_ = parse_uri.substr(author_start + 2, host_end - author_start - 2);
                    // 不存在port ，获取path
                    path_start = host_end;
                }
                else {
                    ClearUri();
                    vaild_ = false;
                    return -1;
                }
            }
        }
        // 获取path query authority
        path_end = parse_uri.find("?", path_start);

        if (path_end != std::string::npos) {
            path_ = parse_uri.substr(path_start + 1, path_end - path_start - 1);
            // 获取query
            query_end = parse_uri.find("#", path_end);

            if (path_end != std::string::npos) {
                query_ = parse_uri.substr(path_end + 1, query_end - path_end - 1);
                fragment_ = parse_uri.substr(query_end + 1);
            }
            else {
                query_ = parse_uri.substr(path_end + 1);
            }
        }
    }
        // 查找 "//" 如果不存在，则不存在authority,取path
    else {
        std::string str_end = parse_uri.substr(scheme_end + 1);

        if (str_end.empty()) {
            ClearUri();
            vaild_ = false;
            return -1;
        }
        path_end = parse_uri.find("?", scheme_end);

        if (path_end != std::string::npos) {
            path_ = parse_uri.substr(scheme_end + 1, path_end - scheme_end - 1);
            // 获取query
            query_end = parse_uri.find("#", path_end);

            if (query_end != std::string::npos) {
                query_ = parse_uri.substr(path_end + 1, query_end - path_end - 1);
                fragment_ = parse_uri.substr(query_end + 1);
            }
            else {
                query_ = parse_uri.substr(path_end + 1);
            }
        }
        else {
            query_end = parse_uri.find("#", scheme_end);

            if (query_end != std::string::npos) {
                path_ = parse_uri.substr(scheme_end + 1, query_end - scheme_end - 1);
                fragment_ = parse_uri.substr(query_end + 1);
            }
            else {
                path_ = parse_uri.substr(scheme_end + 1);

                if (path_.empty()) {
                    ClearUri();
                    vaild_ = false;
                    return -1;
                }
            }
        }
    }
    
    return 0;
}
