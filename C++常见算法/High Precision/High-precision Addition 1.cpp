#include <iostream>
#include <string>
#include <algorithm>
using namespace std;

string add(string a, string b) {
    string result;
    int carry = 0;                 // 进位
    int i = a.length() - 1;        // a 的最后一位
    int j = b.length() - 1;        // b 的最后一位

    while (i >= 0 || j >= 0 || carry) {
        int sum = carry;                    // 先加上进位
        if (i >= 0) sum += a[i--] - '0';    // 加上 a 的这一位
        if (j >= 0) sum += b[j--] - '0';    // 加上 b 的这一位

        carry = sum / 10;                   // 满 10 进 1
        result += (sum % 10) + '0';         // 当前位存下来
    }
    reverse(result.begin(), result.end());  // 结果反着存的，翻转回来
    return result;
}

int main() {
    cout << add("12345678901234567890", "98765432109876543210") << endl;
    return 0;
}
