#include<iostream>
#include<iomanip>
#include<cmath>
using namespace std;
int main(){
    double principal,years,customRate,benchmarkRate;
    cin>>principal>>years>>customRate;
    if(years==1){
        benchmarkRate=0.015;
    }else if(years==2){
        benchmarkRate=0.021;
    }else{
        benchmarkRate=0.0275;
    }
    double benchmarkTotal =principal * pow(1.0 + benchmarkRate, years);
    double customTotal=principal*pow(1.0+customRate,years);
    cout << fixed << setprecision(2);
    cout << benchmarkTotal << ' ' << customTotal << endl; 
    return 0;
}