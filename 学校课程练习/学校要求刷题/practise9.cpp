#include<iostream>
using namespace std;
int main(){
    int a,b,ans;
    char operation;
    cin >> a >> operation >> b;
    switch (operation) {
        case '+':
            ans = a + b;
            break;
        case '-':
            ans = a - b;
            break;
        case '*':
            ans = a * b;
            break;
        case '/':
            if (b == 0) {
                ans = -1;
                break;
            }
            ans = a / b;
            break;
        case '%':
            if (b == 0) {
                ans = -1;
                break;
            }
            ans = a % b;
            break;
        default:
            ans = -1;
            break;
   }
    cout << ans << '\n';
    return 0;
}