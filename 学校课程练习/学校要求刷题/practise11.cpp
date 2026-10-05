#include <iostream>

using namespace std;

long long greatestCommonDivisor(long long a, long long b) {
    while (b != 0) {
        long long remainder = a % b;
        a = b;
        b = remainder;
    }
    return a;
}

int main() {
    long long m, n;
    cin >> m >> n;

    long long gcd = greatestCommonDivisor(m, n);
    long long lcm = m / gcd * n;

    cout << gcd << ' ' << lcm << '\n';

    return 0;
}
