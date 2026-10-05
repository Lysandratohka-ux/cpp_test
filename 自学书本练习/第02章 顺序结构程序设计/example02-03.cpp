#include<iostream>
#include<cmath>
using namespace std;
#define PI 3.141593
int main() {
    int r1 = 4, r2 = 10;
    double V, l;
    V = 4.0 / 3 * PI * (r1 * r1 * r1 + r2 * r2 * r2); // 计算体积
    l = pow(V, 1.0 / 3); // 使用立方根计算边长，注意这里不能写成 1/3
    cout << (int)l << endl;
    return 0;
}
