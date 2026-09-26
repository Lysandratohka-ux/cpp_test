#include<iostream>
using namespace std;
int main() {
    int n=0,k;
    cin>>k;
    for(double Sn=0;Sn<=k;n++,Sn+=1.0/n);
    cout<<n<<endl;

}