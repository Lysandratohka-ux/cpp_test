#include <iostream>
#include<vector>
using namespace std;
vector<int>eratosthenes(int n){
    
    vector<bool>prime(n+1,true);
    prime[0]=false;
    if(n>=1){
        prime[1]=false;
    }
    for(int i=2;1LL*i*i<=n;i++){
        if(prime[i]){
            for(long long j=1LL*i*i;j<=n;j+=i){
                prime[j]=false;
            }
        }
    }

    vector<int>primes;
    for(int i=2;i<=n;i++){
        if(prime[i]){
            primes.push_back(i);
        }
    }

    return primes;
}
int main(){ 
    int n;
    cin>>n;
    vector<int>primes=eratosthenes(n);
    for(int i=0;i<primes.size();){
        if(n%primes[i]==0){
            cout<<primes[i]<<' ';
            n=n/primes[i];
        }else{
            i++;
        }
    }
    return 0;
}
