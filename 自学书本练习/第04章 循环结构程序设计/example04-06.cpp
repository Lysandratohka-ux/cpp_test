#include<iostream>
#include<cstdio>
int main() {
    int n;
    scanf("%d",&n);
    int num = 1;
    for(int i=1;i<=n;i++) {
        for(int j=1;j<=n-i+1;j++) {
            printf("%02d", num);
            num++;
        }
        printf("\n");
    }
    return 0;
}
