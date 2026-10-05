#include<iostream>
#include<string>
using namespace std;
int main(){
    string word,s;
    getline(cin,word);
    getline(cin,s);
    for(int i=0;i<word.length();i++){
        if('A'<word[i]&&word[i]<'Z'){
            word[i]+='a'-'A';
        }
    }
    for(int i=0;i<s.length();i++){
        if('A'<s[i]&&s[i]<'Z'){
            s[i]+='a'-'A';
        }
    }
    word=' '+word+' ';
    s=' '+s+' ';
    if (s.find(word)==string::npos){
        cout<<-1<<endl;
    }else{
        int firstpos=s.find(word);
        int nextpos=s.find(word),cnt=0;
        while(nextpos!=string::npos){
            cnt++;
            nextpos=s.find(word,nextpos+1);
        }
        cout<<cnt<<" "<<firstpos<<endl;
    }
    return 0;
}