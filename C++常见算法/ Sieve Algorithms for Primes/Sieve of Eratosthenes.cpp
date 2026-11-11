#include <vector>
using namespace std;


vector<bool> eratosthenes(int n) {
    vector<bool> prime(n + 1, true);
    prime[0] = prime[1] = false;
    for (int i=4;i<=n;i+=2) {
        prime[i] = false;
    }                             //删除除2以外所有偶数
    for (int p = 3; 1LL * p * p <= n; p+=2) {
        if (prime[p]) {
            for (long long j = 1LL * p * p; j <= n; j += p) {
                prime[j] = false;
            }
        }
    }

    return prime;
}