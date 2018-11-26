#include <stdio.h>
#include "bigdecimal.h"

#define F(H,V) printf(H);VpPrintF(stdout,V);printf("\n");
#define E(H,V) printf(H);VpPrintE(stdout,V);printf("\n");

void test1()
{
    VP_HANDLE sqrt  = VpMemAlloc(100);
    VP_HANDLE pi    = VpMemAlloc(100);
    VP_HANDLE pi2   = VpMemAlloc(100);
    VP_HANDLE exp   = VpMemAlloc(100);
    VP_HANDLE one   = VpMemAlloc(100);
    VP_HANDLE two   = VpAlloc("2",1);
    VP_HANDLE zero  = VpMemAlloc(100);
	VP_HANDLE r;
	VP_UINT   m;

	F("Sqrt(2)    =",VpSqrt(sqrt,two));                  /* sqrt = sqrt(2) */
	VpFree(&two);two = VpMemAlloc(VpCurLength(sqrt)*2+1);
	F("Sqrt(2)**2=",VpPower(two,sqrt,2));                /* two = sqrt(2)**2  ==> 2.0 */
	F("pi        =",VpPI(pi));                           /* pi  = 3.141592...         */
	m = VpCurLength(pi)+VpCurLength(two)+1;
    if(m<VpMaxLength(pi2)) m = VpMaxLength(pi2)+1;
    r = VpMemAlloc(m);
	F("pi2       =",VpDiv(pi2,r,pi,two));                /* pi2 = pi/2 */
	E("sin(pi/2) =",VpSin(one,pi2));                     /* one = sin(pi/2)   ==> 1.0 */
	F("cos(pi/2) =",VpCos(zero,pi2));                    /* zero = sin(pi/2)   ==> 1.0 */
	F("exp       =",VpExp(exp,one));                     /* exp = e */
	F("sqrt(exp) =",VpSqrt(sqrt,exp));                   /* sqrt= sqrt(e) */
	F("log(sqrt) =",VpLog(exp,sqrt));                    /* exp = log(e)/2 ==> 0.5 */

	VpFree(&sqrt);
	VpFree(&exp);
	VpFree(&pi);
	VpFree(&pi2);
	VpFree(&zero);
	VpFree(&one);
	VpFree(&two);
	VpFree(&r);
}
void test2()
{
	VP_HANDLE c = VpAlloc("5555555555.5555555555",1);
	VP_HANDLE a = VpMemAlloc(VpMaxLength(c));
	F("c=",c);
	F("a=",VpAsgn(a,c,1));
    VpAsgn(a,c,1);	F("ScaleRound ( 0)= ",VpScaleRound (c, 0));
    VpAsgn(c,a,1); 	F("ScaleRound ( 2)= ",VpScaleRound (c, 2));
    VpAsgn(c,a,1);	F("ScaleRound (-2)= ",VpScaleRound (c,-2));
    VpAsgn(c,a,1);	F("LengthRound(12)= ",VpLengthRound(c,12));
    VpAsgn(c,a,1);	F("LengthRound( 8)= ",VpLengthRound(c, 8));
	VpFree(&a);
	VpFree(&c);
}

void MyException(VP_HANDLE h,const char *msg)
{
	printf("MyException(%s)\n",msg);
}

int main(int argc, char* argv[])
{
	test1();
	test2();

	E("Inf =",VpAlloc("Infinity",1));
	F("+Inf=",VpAlloc("+Infinity",1));
	E("-Inf=",VpAlloc("-Infinity",1));
	F("NaN =",VpAlloc("NaN",1));

	VpSetExceptionHandler(MyException);
	E("Err =",VpAlloc("-0..123",1));

	F("+0  =",VpAlloc("+0",1));
	E("-0  =",VpAlloc("-0",1));

	return 0;
}

