#include<iostream>
using namespace std;
int main() {
    int a,days=1;
    cin>>a;
    while(a>1){
        a/=2;
        days++;
    }
    cout<<days<<endl;
    return 0;
}