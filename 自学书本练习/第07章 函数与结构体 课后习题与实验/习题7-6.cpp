#include<iostream>
#include<cmath>
using namespace std;
int main(){
    int a[31]={0};
    int i=0;
    while(cin>>a[i++]);
    int sum=0;
    for(int j=0;j<i;j++){
        sum+=a[j];
    }
    long long ans=pow(2,i-2)*sum;
    cout<<ans;
    return 0;
}