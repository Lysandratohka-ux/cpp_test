#include<iostream>
#include<cmath>
using namespace std;
int main(){
    constexpr int MAXN=1024;
    int N,a[3][MAXN],Sum=0;
    cin>>N;
    for(int i=0;i<N;i++){
        cin>>a[0][i]>>a[1][i]>>a[2][i];
    }
    for(int i=0;i<N;i++){
        for(int j=i+1;j<N;j++){
            if (abs(a[0][i]-a[0][j])<=5
                &&abs(a[1][i]-a[1][j])<=5
                &&abs(a[2][i]-a[2][j])<=5
                &&abs(a[0][i]-a[0][j]+a[1][i]-a[1][j]+a[2][i]-a[2][j])<=10){
                Sum++;
               }
        }
    }
    cout<<Sum<<'\n';
    return 0;
}