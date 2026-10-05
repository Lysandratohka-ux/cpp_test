#include<iostream>
using namespace std;
constexpr int MAXN=205;
int main(){
    int n,num=0,a[MAXN];
    cin>>n;
    while(n!=1){
        a[++num]=n;    //从编号1开始把数据计入数组当中
        if(n%2==1){
            n=n*3+1;
        }else{
            n=n/2;
        }
    }
    a[++num]=1;
    while(num!=0){
        cout<<a[num--]<<' ';
    }
}