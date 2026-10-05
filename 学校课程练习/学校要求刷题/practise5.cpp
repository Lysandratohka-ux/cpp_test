#include<iostream>
#include<cstdio>
using namespace std;
int main(){
    int n=4;
    char s[10];
    scanf("%s",s);
    for (int i = 0; s[i]; i++) {
    putchar(s[i] + 4);
}
    return 0;
}