#include<iostream>
#include<algorithm>
using namespace std;
int main(){
    int n;
    int a,b;
    cin>>n;
    for(int i=2;i*i<=n;i++){
        if(n%i==0){
            a=i;
            break;
        }
    }
    b=n/a;
    cout<<max(a,b);
    return 0;
}