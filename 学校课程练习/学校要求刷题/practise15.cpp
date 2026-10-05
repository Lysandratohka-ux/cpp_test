#include <iostream>
#include <string>

using namespace std;

int main() {
    string text;
    getline(cin, text);

    for (char &character : text) {
        if (character >= 'a' && character <= 'z') {
            character -= 'a' - 'A';
        } else if (character >= 'A' && character <= 'Z') {
            character += 'a' - 'A';
        }
    }

    cout << text << '\n';

    return 0;
}
