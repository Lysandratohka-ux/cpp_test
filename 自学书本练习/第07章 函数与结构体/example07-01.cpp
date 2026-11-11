#include<cstdio>
#include<cmath>
using namespace std;
double sq(double x){
    return x*x;
}
double dist(double x1,double y1,double x2,double y2){
    return sqrt(sq(x1-x2)+sq(y1-y2));
}
int main(){
    double x1,y1,x2,y2,x3,y3,ans;
    scanf("%lf%lf%lf%lf%lf%lf",&x1,&y1,&x2,&y2,&x3,&y3);
    ans=dist(x1,y1,x2,y2);
    ans+=dist(x2,y2,x3,y3);
    ans+=dist(x3,y3,x1,y1);
    printf("%.2f",ans);
}