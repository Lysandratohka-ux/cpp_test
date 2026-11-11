#include<iostream>
#include<string>
using namespace std;
int hexToDec(string hexNumber){
    int sumNumber=0;
    for(size_t i=0;i<hexNumber.length();i++){
        int digit=0;
        if(hexNumber[i]<='f'&&hexNumber[i]>='a'){
            digit=hexNumber[i]-'a'+10;
        }else if(hexNumber[i]<='F'&&hexNumber[i]>='A'){
            digit=hexNumber[i]-'A'+10;
        }else if(hexNumber[i]<='9'&&hexNumber[i]>='0'){
            digit=hexNumber[i]-'0';
        }
        sumNumber=sumNumber*16+digit;
    }
    return sumNumber;
}
int main(){
    string s;
    getline(cin,s);
    cout<<hexToDec(s);
    return 0;
}