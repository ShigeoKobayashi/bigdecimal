#include <stdio.h>
#include "bigdecimal.h"

#define F(H,V) printf(H);VpPrintF(stdout,V);printf("\n");
#define E(H,V) printf(H);VpPrintE(stdout,V);printf("\n");

void test1()
{
    VP_HANDLE sqrt2 = VpAlloc("0",100);
    VP_HANDLE pi    = VpAlloc("0",100);
    VP_HANDLE pi2   = VpAlloc("0",100);
    VP_HANDLE one   = VpAlloc("0",100);
    VP_HANDLE two   = VpAlloc("2",1);
	VP_HANDLE r;
	VP_UINT   m;

	F("Sqrt(2)    =",VpSqrt(sqrt2,two));                 /* sqrt2 = sqrt(2) */
	VpFree(&two);two = VpAlloc("0",VpCurLength(sqrt2)*2+1);
	F("Sqrt(2)**2=",VpMul(two,sqrt2,sqrt2));             /* two = sqrt(2)**2  ==> 2.0 */
	F("pi        =",VpPI(pi));                           /* pi  = 3.141592...         */
	m = VpCurLength(pi)+VpCurLength(two)+1;
    if(m<VpMaxLength(pi2)) m = VpMaxLength(pi2)+1;
    r = VpAlloc("0",m);
	F("pi2       =",VpDiv(pi2,r,pi,two));                /* pi2 = pi/2 */
	E("sin(pi2)  =",VpSin(one,pi2));                     /* one = sin(pi/2)   ==> 1.0 */
	VpFree(&sqrt2);
	VpFree(&pi);
	VpFree(&pi2);
	VpFree(&one);
	VpFree(&two);
	VpFree(&r);
}
void test2()
{
	VP_HANDLE c = VpAlloc("5555555555.5555555555",1);
	VP_HANDLE a = VpAlloc("0",VpMaxLength(c));
    VpAsgn(a,c,1);	F("ScaleRound ( 0)= ",VpScaleRound (c, 0));
    VpAsgn(c,a,1); 	F("ScaleRound ( 2)= ",VpScaleRound (c, 2));
    VpAsgn(c,a,1);	F("ScaleRound (-2)= ",VpScaleRound (c,-2));
    VpAsgn(c,a,1);	F("LengthRound(12)= ",VpLengthRound(c,12));
    VpAsgn(c,a,1);	F("LengthRound( 8)= ",VpLengthRound(c, 8));
	VpFree(&a);
	VpFree(&c);
}
int main(int argc, char* argv[])
{
	test1();
	test2();
	return 0;
}

