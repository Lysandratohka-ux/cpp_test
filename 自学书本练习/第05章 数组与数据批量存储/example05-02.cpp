#include <iostream>
using namespace std;

int main() {
    int n = 0, a[110];

    do {
        cin >> a[n++];    //以数组形式输入数据，随后将n加一
    } while (a[n - 1] != 0);    //查验刚才输入的数字是否为0

    n--;    //把n先下降一位，因为在上一个dowhile循环中输入数据后n会上升一位

    while (n--) {    //查验n是否为0（n为0会判定为false，n不为零一律判定为真
        cout << a[n] << ' ';
    }

    return 0;
}