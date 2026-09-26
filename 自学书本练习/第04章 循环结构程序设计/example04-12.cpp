#include <iostream>
using namespace std;

int main() {
    int n;
    double s = 0;
    cin >> n;

    // 原书说明：为避免用浮点数作循环变量，
    // 将原数列全部乘以 10，改用整数循环。
    for (int i = 1; i <= 10 * n - 1; i++)
        s += i / 10.0;

    cout << s;
    return 0;
}
