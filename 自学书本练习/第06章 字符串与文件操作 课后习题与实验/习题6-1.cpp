#include<iostream>
#include<string>
using namespace std;
int main(){
    string s;
    getline(cin,s);
    int n=0;
    for(int i=0;i<s.length();i++){
        if(s[i]=='a'||s[i]=='d'||s[i]=='g'||s[i]=='j'||s[i]=='m'||s[i]=='p'||s[i]=='t'||s[i]=='w'||s[i]==' '){
            n+=1;
        }else if(s[i]=='b'||s[i]=='e'||s[i]=='h'||s[i]=='k'||s[i]=='n'||s[i]=='q'||s[i]=='u'||s[i]=='x'){
            n+=2;
        }else if(s[i]=='s'||s[i]=='z'){
            n+=4;
        }else{
            n+=3;
        }
    }
    cout<<n<<endl;
    return 0;
}