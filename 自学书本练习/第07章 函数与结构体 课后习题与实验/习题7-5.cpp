#include<iostream>
#include<vector>
using namespace std;
vector<bool> eulerSieve(int n) {
    vector<bool> prime(n + 1, true);
    prime[0] = prime[1] = false;

    vector<int> primes;

    for (int i = 2; i <= n; i++) {
        if (prime[i]) {
            primes.push_back(i);
        }

        for (int p : primes) {
            if (1LL * i * p > n) {
                break;
            }

            prime[i * p] = false;

            if (i % p == 0) {
                break;
            }
        }
    }

    return prime;
}
bool check2(int n){
    int origin=n;
    int a[10];
    int digits=to_string(n).length();
    for(int i=0;i<digits;i++){
        a[i]=n%10;
        n=n/10;
    }
    int m=0;
    for(int j=0;j<digits;j++){
        m=10*m+a[j];
    }
    if(origin==m){
        return true;
    }else{
        return false;
    }
}
int main(){
    long long a;
    long long b;
    cin>>a>>b;
    vector<bool>result=eulerSieve(b);
    for(int i=a;i<=b;i++){
        if(result[i]==1&&check2(i)==1){
            cout<<i<<endl;
        }
    }
    return 0;
}