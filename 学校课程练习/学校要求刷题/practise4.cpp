#include<iostream> 
#include<cmath>
#include<iomanip>
using namespace std;
int main() {
    double p,r=0.08;
    int n=9;
    p = pow(1 + r, n);
    cout << fixed << setprecision(6) << p << '\n';
    return 0;
}