#include <vector>
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