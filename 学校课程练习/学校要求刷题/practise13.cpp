#include <iostream>

using namespace std;

int main() {
    long long dividend, divisor;
    char divisionSign;
    cin >> dividend >> divisionSign >> divisor;

    // 先输出整数部分。
    cout << dividend / divisor << '.';

    // 利用余数模拟竖式除法，依次求出50位小数。
    long long remainder = dividend % divisor;
    for (int i = 0; i < 50; ++i) {
        remainder *= 10;
        cout << remainder / divisor;
        remainder %= divisor;
    }

    cout << '\n';

    return 0;
}
