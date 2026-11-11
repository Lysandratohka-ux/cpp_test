#include<iostream>
using namespace std;
int n,a[100];
bool is_prime(int x){
    if(x==0||x==1)return 0;
    for (int i=2;i*i<=x;i++)
       if(x%i==0)
           return 0;
    return 1;
}
int main(){
    cin>>n;
    for(int i=0;i<n;i++)
        cin>>a[i];
    for(int i=0;i<n;i++)
        if(is_prime(a[i]))
            cout<<a[i]<<" ";
    cout<<endl;
    return 0;
}