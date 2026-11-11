#include<iostream>
using namespace std;
bool isPrime(int x){
    if(x<2)return false;
    for (int i=2;i*i<=x;i++){
        if(x%i==0)return false;
    }
    return true;
}
int main(){
    int n;
    cin>>n;
    for(int i=4;i<=n;i+=2){
        for(int j=2;j<=i/2;j++){
            int delta=i-j;
            if(isPrime(j)==1&&isPrime(delta)==1){
                cout<<i<<'='<<j<<'+'<<delta<<endl;
                break;
            }
        }
    }
    return 0;
}