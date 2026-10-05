#include <iostream>
#include <string>

using namespace std;

int main() {
    string text;
    getline(cin, text);

    int letters = 0;
    int spaces = 0;
    int digits = 0;
    int others = 0;

    for (char character : text) {
        if ((character >= 'A' && character <= 'Z') ||
            (character >= 'a' && character <= 'z')) {
            ++letters;
        } else if (character == ' ') {
            ++spaces;
        } else if (character >= '0' && character <= '9') {
            ++digits;
        } else {
            ++others;
        }
    }

    cout << letters << ' ' << spaces << ' '
         << digits << ' ' << others << '\n';

    return 0;
}
