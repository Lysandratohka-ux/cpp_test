#include <iostream>
#include <iomanip>
using namespace std;

int main() {
    int n;
    cin >> n;

    const int MAXN = 60;
    long long f[MAXN];
    f[1] = 1; f[2] = 1;
    for (int i = 3; i < MAXN; i++)
        f[i] = f[i-1] + f[i-2];

    double ans = 0;
    for (int i = 1; i <= n; i++) {
        ans += (double)f[i+2] / f[i+1];
    }
    cout << fixed << setprecision(6) << ans << endl;
    return 0;
}