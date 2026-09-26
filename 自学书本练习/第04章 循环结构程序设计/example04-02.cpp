#include<iostream> 
using namespace std;
int main() {
    int n,i,min=10000000;
    cin>>n;
    for(i=0;i<n;i++) {
        int x;
        cin>>x;
        if(x<min) min=x;
    }
    cout<<min<<endl;
    return 0;
}