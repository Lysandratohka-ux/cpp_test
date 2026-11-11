#include <iostream>
#include<iomanip>
using namespace std;
long long fibonacci(int a){
    long long f[52]={0,1};
    for(int i=2;i<=a;i++){
        f[i]=f[i-1]+f[i-2];
    }
    return f[a];
}
int main() {
    int n;
    cin>>n;
    double ans=0,b;
    for(int i=1;i<=n;i++){
        b=fibonacci(i+2)*1.0/fibonacci(i+1);
        ans+=b;
    }
    cout<<fixed<<setprecision(6)<<ans<<'\n';
    return 0;
}
