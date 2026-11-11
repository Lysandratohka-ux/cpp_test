#include<iostream>
#include<vector>
#include<algorithm>
#include<functional>
using namespace std;
int main(){
    int n,k;    //n表示题目个数，k表示每个题目参数数量
    cin>>n>>k;
    int a[10]={};
    int m[100]={};
    for(int i=0;i<n;i++){
        for(int j=0;j<k;j++){
            cin>>a[j];
            m[i]+=a[j];
        }
    }
    int posLargest=0,posSecondLargest=0;
    for(int i=0;i<n-1;i++){
        if(m[posLargest]<m[i+1]){
            posLargest=i+1;
        }
    }
    posSecondLargest = (posLargest == 0 ? 1 : 0);
    for (int i = 0; i < n; i++) {
    if (i != posLargest && m[i] > m[posSecondLargest]) {
        posSecondLargest = i;
    }
}
    cout<<posLargest+1<<'\n'<<posSecondLargest+1;
    return 0;
}