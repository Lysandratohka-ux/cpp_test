#include <iostream>
#include <iomanip>
using namespace std;

int main() {
    int n;
    cin >> n;

    double numerator = 2;
    double denominator = 1;
    double ans = 0;

    for (int i = 0; i < n; i++) {
        ans += numerator / denominator;

        double next = numerator + denominator;
        denominator = numerator;
        numerator = next;
    }

    cout << fixed << setprecision(6) << ans << '\n';
    return 0;
}