#include<cstdio>
using namespace std;
int main() {
    int a, b, t;
    scanf("%d %d", &a, &b);
    t = a; a = b; b = t; // 一组相关的短语句也可以写在一行内
    printf("%d %d", a, b);
    return 0;
}
