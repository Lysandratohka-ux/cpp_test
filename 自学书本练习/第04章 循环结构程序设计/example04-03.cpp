#include<iostream>
#include<cstdio>
using namespace std;    
int main() {
    int n,k;
    int Asum=0,Bsum=0;
    cin >> n >> k;
    for(int i=0;i<=n;i+=k){
        Asum += i;
    }
    Bsum=n*(n+1)/2 - Asum;
    printf("%.1f",double(Asum)/(n/k));
    printf(" %.1f\n",double(Bsum)/(n-n/k));
    return 0;
}