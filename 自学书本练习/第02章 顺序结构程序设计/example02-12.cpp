#include<cstdio>
#include<cmath>
using namespace std;
int main() {
    int s, v;
    scanf("%d%d", &s, &v);
    int t_walk = ceil(1.0 * s / v) + 10; // 两次类型转换注意到了吗
    int from_zero = 60 * (24 + 8) - t_walk; // 计算到前一天零点的时间
    int hh = (from_zero / 60) % 24; // 计算小时
    int mm = from_zero % 60; // 计算分钟
    printf("%02d:%02d\n", hh, mm); // 输出两位，用0补齐
    return 0;
}
