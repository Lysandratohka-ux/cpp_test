#include<iostream>
#include<vector>
using namespace std;
vector<bool>eratosthenes(int n){
    vector<bool> prime(n+1,true);
    prime[1]=prime[0]=false;
    for(int i=2;1LL*i*i<=n;i++){
        if(prime[i]){
            for(long long j=1LL*i*i;j<=n;j+=i){
                prime[j]=false;
            }
        }
    }
    return prime;
}
int main(){
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    int n,q;
    cin>>n>>q;
    vector<bool>result=eratosthenes(n);
    vector<int> primes;
    for(int i=2;i<=n;i++){
        if(result[i]){
            primes.push_back(i);
        }
    }
    while(q--){
        int k;
        cin>>k;
        cout<<primes[k-1]<<'\n';
    }
    return 0;
}
