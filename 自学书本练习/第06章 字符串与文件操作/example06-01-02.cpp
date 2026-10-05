#include<iostream>
#include<cstdio>
using namespace std;
int main(){
    char s;
    while(1){
        s=getchar();
        if(s==EOF)break;
        if('a'<=s&&s<='z')
            s+='A'-'a';
        putchar(s);
        }
    return 0;
}