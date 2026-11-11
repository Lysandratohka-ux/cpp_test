#include<iostream>
using namespace std;
int x,y,ans[500],cnt=0;
void init(){
    cin>>x>>y;
}
void doit(){
    for(int i=x;i<=y;i++){
        if(i%400==0||(i%4==0&&i%100!=0)){
            ans[cnt++]=i;
        }
    }
}
void output(){
    cout<<cnt<<endl;
    for(int i=0;i<cnt;i++){
        cout<<ans[i]<<" ";
    }
    cout<<endl;
}
int main(){
    init();
    doit();
    output();
    return 0;
}