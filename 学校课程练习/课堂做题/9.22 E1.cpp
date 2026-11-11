#include <iostream>
#include <cstdlib>
using namespace std;

int main() {
    int number;
    cin >> number;

    int a = number / 100;
    int b = number / 10 % 10;
    int c = number % 10;

    int sum = a * a * a + b * b * b + c * c * c;

    if (sum == number) {
        cout << 0 << endl;
    } else {
        cout << abs(sum - number) << endl;
    }

    return 0;
}