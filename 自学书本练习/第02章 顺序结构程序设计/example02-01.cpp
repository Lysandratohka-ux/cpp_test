#include<iostream>
using namespace std;
int main() {
    int v_a = 5, v_yao = 8, distance = 100; // 小A和八尾勇的速度，以及距离
    double delta, ans; // 速度的差值和答案
    delta = v_yao - v_a; // 两人的相对速度，也就是每秒距离缩短多远
    ans = distance / delta;
    // ans = 1.0 * distance / (v_yao - v_a)
    cout << ans << endl;
    return 0;
}
