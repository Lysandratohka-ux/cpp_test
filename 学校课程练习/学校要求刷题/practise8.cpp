#include<iostream>
using namespace std;
int main() {
    int n;
    cin>>n;
    switch(n) {
        case 1:
            cout << "red\n";
            break;
        case 2:
            cout << "green\n";
            break;
        case 3:
            cout << "yellow\n";
            break;
        default:
            cout << "error\n";
    }
    return 0;
}