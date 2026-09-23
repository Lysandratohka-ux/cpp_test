#include <iostream>
using namespace std;   
int main() {
    int x;
    bool p1,p2,p3;
    cin >> x;
    p1=x%4==0;
    p2=x%100!=0;
    p3=x%400==0;
    cout << (p1||p2&&p3) << endl;
}
