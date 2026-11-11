#include<iostream>
using namespace std;
int main(){
    int n;
    bool a=1;
    cin>>n;
    if(n==1){
        a=0;
    }else if(n==2){
        a=1;
    }else{
        for(int i=2;i<=n/i;i++){
            if(n%i==0){
                a=0;
            }
        }
    }
    cout<<a<<'\n';
    return 0;
}