#include <iostream>
using namespace std;

int main() {
    int s = 0, n;
    cin >> n;
    for (int i = 1; i <= n; i++)
        s += i;
    cout << s;
    return 0;
}
