#include <iostream>
using namespace std;
int main() {
    long long dividend, divisor;
    char divisionSign;
    cin >> dividend >> divisionSign >> divisor;
    cout << dividend / divisor << '.';
    long long remainder = dividend % divisor;
    for (int i = 0; i < 50; ++i) {
        remainder *= 10;
        cout << remainder / divisor;
        remainder %= divisor;
    }
    cout << '\n';
    return 0;
}