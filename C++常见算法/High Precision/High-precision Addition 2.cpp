#include <vector>
#include <algorithm>
using namespace std;

vector<int> add(vector<int>& a, vector<int>& b) {
    vector<int> c;
    int carry = 0;
    for (int i = 0; i < max(a.size(), b.size()) || carry; i++) {
        if (i < a.size()) carry += a[i];
        if (i < b.size()) carry += b[i];
        c.push_back(carry % 10);
        carry /= 10;
    }
    return c;
}
