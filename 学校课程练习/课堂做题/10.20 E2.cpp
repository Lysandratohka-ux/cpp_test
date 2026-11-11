#include<iostream>
#include<vector>
using namespace std;
int main(){
    vector<char>text;
    char x;
    while(cin.get(x)&&x!='\n'){
        text.push_back(x);
    }
    int numEnglish=0,numSpace=0,numNumber=0,numElse=0;
    for (size_t i = 0; i < text.size(); i++) {
        if((text[i]>='a'&&text[i]<='z')||(text[i]>='A'&&text[i]<='Z')){
            numEnglish+=1;
        }else if(text[i]==32){
            numSpace+=1;
        }else if(text[i]>='0'&&text[i]<='9'){
            numNumber+=1;
        }else{
            numElse+=1;
        }
    }
    cout<<numEnglish<<' '<<numSpace<<' '<<numNumber<<' '<<numElse;
    return 0;
}