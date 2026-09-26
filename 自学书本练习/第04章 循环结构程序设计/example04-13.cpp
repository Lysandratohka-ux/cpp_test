#include<iostream>
using namespace std;
int main(){
    int L;    //L代表质数总和最大值，即所有质数相加不能超过L
    int sum=0;    //sum代表当前质数总和
    cin>>L;
    
    for (int i=2;sum<=L;i++){
        for (int j=2;j<i;j++){
            if(i%j==0) break;
            if(j*j>i) {
            sum+=i;
            }
        }
    }
    cout<<sum<<endl;
    return 0;
}
