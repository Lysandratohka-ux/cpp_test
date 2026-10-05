#include <iostream>

using namespace std;

void compareAndSwap(int &left, int &right) {
    if (left > right) {
        int temporary = left;
        left = right;
        right = temporary;
    }
}

int main() {
    int a, b, c, d, e;
    cin >> a >> b >> c >> d >> e;

    // 第一轮把最大的数移动到e。
    compareAndSwap(a, b);
    compareAndSwap(b, c);
    compareAndSwap(c, d);
    compareAndSwap(d, e);

    // 第二轮把剩余数字中最大的数移动到d。
    compareAndSwap(a, b);
    compareAndSwap(b, c);
    compareAndSwap(c, d);

    // 继续对尚未排好序的数字进行比较。
    compareAndSwap(a, b);
    compareAndSwap(b, c);
    compareAndSwap(a, b);

    cout << a << ' ' << b << ' ' << c << ' ' << d << ' ' << e << '\n';

    return 0;
}
