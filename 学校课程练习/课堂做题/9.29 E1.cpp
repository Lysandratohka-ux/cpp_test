#include<iostream>
#include<cmath>
#include<iomanip>
using namespace std;
int main(){
    double m, n, r;
    double base_rate;
    double base_total, custom_total;
    cin >> m >> n >> r;

    if (n < 2) {
        base_rate = 0.0150;
    } else if (n < 3) {
        base_rate = 0.0210;
    } else {
        base_rate = 0.0275;
    }

    base_total = m * pow(1 + base_rate, n);
    custom_total = m * pow(1 + r, n);

    cout << fixed << setprecision(2) << base_total << " "
         << custom_total << endl;
    return 0;
}