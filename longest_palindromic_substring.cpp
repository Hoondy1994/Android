#include <iostream>
#include <string>
using namespace std;

class Solution {
public:
    string longestPalindrome(string s) {
        int n = static_cast<int>(s.size());
        if (n <= 1) return s;

        int start = 0, maxLen = 1;

        auto expand = [&](int left, int right) {
            while (left >= 0 && right < n && s[left] == s[right]) {
                left--;
                right++;
            }
            int len = right - left - 1;
            if (len > maxLen) {
                maxLen = len;
                start = left + 1;
            }
        };

        for (int i = 0; i < n; i++) {
            expand(i, i);       // 奇数长度
            expand(i, i + 1);   // 偶数长度
        }

        return s.substr(start, maxLen);
    }
};

int main() {
    Solution sol;

    // 示例 1
    string s1 = "babad";
    cout << "输入: \"" << s1 << "\"\n";
    cout << "输出: \"" << sol.longestPalindrome(s1) << "\"\n\n";

    // 示例 2
    string s2 = "cbbd";
    cout << "输入: \"" << s2 << "\"\n";
    cout << "输出: \"" << sol.longestPalindrome(s2) << "\"\n\n";

    // 交互输入
    cout << "请输入字符串 (直接回车结束): ";
    string s;
    getline(cin, s);
    if (!s.empty()) {
        cout << "输出: \"" << sol.longestPalindrome(s) << "\"\n";
    }

    return 0;
}
