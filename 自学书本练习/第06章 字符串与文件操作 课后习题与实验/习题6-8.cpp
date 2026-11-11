#include<iostream>
#include<string>
using namespace std;
int main(){
    string text,line;
    while(getline(cin,line)){
        text+=line;
    }
    int num[30]={};
    for(size_t i=0;i<text.length();i++){
            if (text[i] >= 'A' && text[i] <= 'Z') {
            num[text[i] - 'A']++;
        }
    }
    int max=0;
    for(size_t i=0;i<26;i++){
        if(max<num[i]){
        max=num[i];
        }
    }
    while (max > 0) {
    int right = 25;
    while (num[right] < max) {
        right--;
    }

    for (int j = 0; j <= right; j++) {
        cout << (num[j] >= max ? '*' : ' ');
        if (j < right) cout << ' ';
    }
    cout << '\n';
    max--;
    }
    for (int i=0; i < 26; i++) {
    if (i > 0){
    cout << ' ';
    }
    cout << char('A' + i);
    }
    return 0;
}