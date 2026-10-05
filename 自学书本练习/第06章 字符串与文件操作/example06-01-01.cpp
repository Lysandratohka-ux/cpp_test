#include<iostream>
#include<cstdio>
using namespace std;
int main(){
    char s[110];
    scanf("%s",s);
    for(int i=0;s[i]!='\0';i++){
        if('a'<=s[i]&&s[i]<='z')
            s[i]-='a'-'A';
    }        
    printf("%s\n",s);    
    return 0;
}