/*
 * Variable precision calculator.
 * (Test program for Bigdecimal(Variable decimal precision) C/C++ library.)
 *
 *  Copyright(C) 2018 by Shigeo Kobayashi(shigeo@tinyforest.jp)
 *
 *    Version 1.1   ... VpLoad() added.
 *
 */

#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#define strcasecmp(a,b) _strcmpi(a,b)
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "bigdecimal.h"

static int DEF_SIZE = 100; /* default size for Bigdecimal variable 'a'-'z' */

/* input line buffer */
#define MAX_BUFF 2048
#define MAX_TOKEN   6

static VP_HANDLE gVpVars[26]; /* 'a' - 'z' */
static int       gnVpVars = sizeof(gVpVars)/sizeof(gVpVars[0]);
static int       gixVars[MAX_TOKEN];

static char  gszBuf[MAX_BUFF]; /* read buffer */
static char *gpszToken[MAX_TOKEN];
static int   gnToken;

#define VP_RETURN unsigned long long
#define VP_ARG    unsigned long long
#define VP_ARGS   unsigned long long *

typedef VP_RETURN (*VPF)(VP_ARGS);
#define FLAG64_NOMSG 0x8000000000000001LL /* Flag for no need to output message(already output) */

static VP_RETURN GetDefSize(VP_ARGS args) {
	return (VP_RETURN)DEF_SIZE;
}
static VP_RETURN SetDefSize(VP_ARGS args) {
	int n = (int)args[0];
	int i;
	VP_HANDLE v;
	if(n>0) {
		DEF_SIZE = n;
		/* reallocate variables */
		for(i=0;i<gnVpVars;++i) {
			if(VpMaxLength(gVpVars[i])>=(unsigned int)n) continue;
			v = VpMemAlloc(n);
			if(VpIsInvalid(v)) break;
			VpAsgn(v,gVpVars[i],1);
			VpFree(&gVpVars[i]);
			gVpVars[i] = v;
		}
	}
	return (VP_RETURN)DEF_SIZE;
}
static VP_RETURN GetDigitSeparationCount(VP_ARGS args){
	return (VP_RETURN)VpGetDigitSeparationCount();
}
static VP_RETURN SetDigitSeparationCount(VP_ARGS args){
	return (VP_RETURN)VpSetDigitSeparationCount((VP_UINT)args[0]);
}
static VP_RETURN GetDigitSeparator(VP_ARGS args) {
	return (VP_RETURN)VpGetDigitSeparator();
}
static VP_RETURN SetDigitSeparator(VP_ARGS args) {
	return (VP_RETURN)VpSetDigitSeparator((char)args[0]);
}
static VP_RETURN GetDigitLeader(VP_ARGS args) {
	return (VP_RETURN)VpGetDigitLeader();
}
static VP_RETURN SetDigitLeader(VP_ARGS args) 
{
	return (VP_RETURN)VpSetDigitLeader((char)args[0]);
}
static void PrintMode(FILE *f)
{
	fprintf(f,"   1:VP_ROUND_UP\n");
	fprintf(f,"   2:VP_ROUND_DOWN\n");
	fprintf(f,"   3:VP_ROUND_HALF_UP\n");
	fprintf(f,"   4:VP_ROUND_HALF_DOWN\n");
	fprintf(f,"   5:VP_ROUND_CEIL\n");
	fprintf(f,"   6:VP_ROUND_FLOOR\n");
	fprintf(f,"   7:VP_ROUND_HALF_EVEN\n");
}

static VP_RETURN GetRoundMode(VP_ARGS args) {
	switch (VpGetRoundMode())
	{
	case 1:printf("  VP_ROUND_UP(1)\n");break;
	case 2:printf("  VP_ROUND_DOWN(2)\n");break;
	case 3:printf("  VP_ROUND_HALF_UP(3)\n");break;  /* Default mode */
	case 4:printf("  VP_ROUND_HALF_DOWN(4)\n");break;
	case 5:printf("  VP_ROUND_CEIL(5)\n");break;
	case 6:printf("  VP_ROUND_FLOOR(6)\n");break;
	case 7:printf("  VP_ROUND_HALF_EVEN(7)\n");break;
	}
	printf("\n  List of round modes(1-7)\n");
	PrintMode(stdout);
	return (VP_RETURN)FLAG64_NOMSG;
}

static VP_RETURN SetRoundMode(VP_ARGS args) {
	int i = (int)args[0];
	if(i<1 || i>7) {
		fprintf(stderr,"  Invalid round mode,enter 1-7\n");
		PrintMode(stderr);
		return (VP_RETURN)FLAG64_NOMSG;
	}
	switch (VpSetRoundMode(i))
	{
	case 1:printf("  VP_ROUND_UP(1)\n");break;
	case 2:printf("  VP_ROUND_DOWN(2)\n");break;
	case 3:printf("  VP_ROUND_HALF_UP(3)\n");break;  /* Default mode */
	case 4:printf("  VP_ROUND_HALF_DOWN(4)\n");break;
	case 5:printf("  VP_ROUND_CEIL(5)\n");break;
	case 6:printf("  VP_ROUND_FLOOR(6)\n");break;
	case 7:printf("  VP_ROUND_HALF_EVEN(7)\n");break;
	}
	return (VP_RETURN)FLAG64_NOMSG;
}
static VP_RETURN GetMaxIterationCount(VP_ARGS args) {
	return (VP_RETURN)VpGetMaxIterationCount();
}
static VP_RETURN SetMaxIterationCount(VP_ARGS args) {
	return (VP_RETURN)VpSetMaxIterationCount((VP_UINT)args[0]);
}
static VP_RETURN GetIterationCount(VP_ARGS args) {
	return (VP_RETURN)VpGetIterationCount();
}
static VP_RETURN IsNumeric(VP_ARGS args) {
	return (VP_RETURN)VpIsNumeric((VP_HANDLE)args[0]);
}
static VP_RETURN GetTag(VP_ARGS args) {
	unsigned long long v = (unsigned long long)VpGetTag((VP_HANDLE)args[0]);
	printf("  %llu\n",v);
	return (VP_RETURN)FLAG64_NOMSG;
}
static VP_RETURN SetTag(VP_ARGS args) {
	unsigned long long v;
	VpSetTag((VP_HANDLE)args[0],(VP_UINT)args[1]);
	v = (unsigned long long)VpGetTag((VP_HANDLE)args[0]);
	printf("  %llu\n",v);
	return (VP_RETURN)FLAG64_NOMSG;
}

static VP_RETURN SetSign(VP_ARGS args) {
	return (VP_RETURN)VpSetSign((VP_HANDLE)args[0],(int)args[1]);
}
static VP_RETURN GetSign(VP_ARGS args) {
	return (VP_RETURN)VpGetSign((VP_HANDLE)args[0]);
}

static VP_RETURN IsOne(VP_ARGS args) {
	return (VP_RETURN)VpIsOne((VP_HANDLE)args[0]);
}
static VP_RETURN SetOne(VP_ARGS args)
{
	return (VP_RETURN)VpSetOne((VP_HANDLE)args[0]);
}
static VP_RETURN IsPosZero(VP_ARGS args) {
	return (VP_RETURN)VpIsPosZero((VP_HANDLE)args[0]);
}
static VP_RETURN SetPosZero(VP_ARGS args) {
	return (VP_RETURN)VpSetPosZero((VP_HANDLE)args[0]);
}
static VP_RETURN IsNegZero(VP_ARGS args) {
	return (VP_RETURN)VpIsNegZero((VP_HANDLE)args[0]);
}
static VP_RETURN SetNegZero(VP_ARGS args) {
	return (VP_RETURN)VpSetNegZero((VP_HANDLE)args[0]);
}
static VP_RETURN IsZero(VP_ARGS args) {
	return (VP_RETURN)VpIsZero((VP_HANDLE)args[0]);
}
static VP_RETURN SetZero(VP_ARGS args) {
	return (VP_RETURN)VpSetZero((VP_HANDLE)args[0],(int)args[1]);
}
static VP_RETURN IsNaN(VP_ARGS args) {
	return (VP_RETURN)VpIsNaN((VP_HANDLE)args[0]);
}
static VP_RETURN SetNaN(VP_ARGS args) {
	return (VP_RETURN)VpSetNaN((VP_HANDLE)args[0]);
}
static VP_RETURN IsPosInf(VP_ARGS args) {
	return (VP_RETURN)VpIsPosInf((VP_HANDLE)args[0]);
}
static VP_RETURN SetPosInf(VP_ARGS args) {
	return (VP_RETURN)VpSetPosInf((VP_HANDLE)args[0]);
}
static VP_RETURN IsInf(VP_ARGS args) {
	return (VP_RETURN)VpIsInf((VP_HANDLE)args[0]);
}
static VP_RETURN SetInf(VP_ARGS args) {
	return (VP_RETURN)VpSetInf((VP_HANDLE)args[0],(int)args[1]);
}
static VP_RETURN IsNegInf(VP_ARGS args) {
	return (VP_RETURN)VpIsNegInf((VP_HANDLE)args[0]);
}
static VP_RETURN SetNegInf(VP_ARGS args) {
	return (VP_RETURN)VpSetNegInf((VP_HANDLE)args[0]);
}
static VP_RETURN Alloc(VP_ARGS args) {
	VP_HANDLE v;
	int i = ((char)args[0]) - 'a'; 
	if(i<0 || i>=26) {
		fprintf(stderr,"Undefined variable(%c)!\n",(char)args[0]);
		return (VP_RETURN)FLAG64_NOMSG;
	}
	v = VpAlloc((const char*)args[1],(VP_UINT)args[2]);
	if(VpIsInvalid(v)) {
		fprintf(stderr,"Failed to allocate.\n");
		return (VP_RETURN)FLAG64_NOMSG;
	}
	printf("  %c=",(char)args[0]);VpPrintE(stdout,v);printf("\n");
	VpFree(&gVpVars[i]);
	gVpVars[i] = v;
	return (VP_RETURN)FLAG64_NOMSG;
}
static VP_RETURN Alloc0(VP_ARGS args) {
	args[2] = (VP_UINT)DEF_SIZE;
	return Alloc(args);
}

static VP_RETURN Load(VP_ARGS args) {
	VP_HANDLE v;
	int i = ((char)args[0]) - 'a'; 
	if(i<0 || i>=26) {
		fprintf(stderr,"Undefined variable(%c)!\n",(char)args[0]);
		return (VP_RETURN)FLAG64_NOMSG;
	}
	v = gVpVars[i];
	v = VpLoad(v,(const char*)args[1]);
	if(VpIsInvalid(v)) {
		fprintf(stderr,"Failed to Load(%s).\n",(const char*)args[1]);
		return (VP_RETURN)FLAG64_NOMSG;
	}
	printf("  %c=",(char)args[0]);VpPrintE(stdout,v);printf("\n");
	return (VP_RETURN)FLAG64_NOMSG;
}


static VP_RETURN MemAlloc(VP_ARGS args) {
	int i = ((char)args[0]) - 'a';
	VP_HANDLE v;
	if(i<0 || i>=26) {
		fprintf(stderr,"Undefined variable(%c)!\n",(char)args[0]);
		return (VP_RETURN)FLAG64_NOMSG;
	}
	v = VpMemAlloc((VP_UINT)args[1]);
	if(VpIsInvalid(v)) {
		fprintf(stderr,"Failed to allocate.\n");
		return (VP_RETURN)FLAG64_NOMSG;
	}
	printf("  %c=",(char)args[0]);VpPrintE(stdout,v);printf("\n");
	VpFree(&gVpVars[i]);
	gVpVars[i] = v;
	return (VP_RETURN)FLAG64_NOMSG;
}
static VP_RETURN Clone(VP_ARGS args) {
	VP_HANDLE v;
	int i = ((char)args[0]) - 'a';
	if(i<0 || i>=26) {
		fprintf(stderr,"Undefined variable(%c)!\n",(char)args[0]);
		return (VP_RETURN)FLAG64_NOMSG;
	}
	v = VpClone((VP_HANDLE)args[1]);
	if(VpIsInvalid(v)) {
		fprintf(stderr,"Failed to clone!\n");
		return (VP_RETURN)FLAG64_NOMSG;
	}
	printf("  %c=",(char)args[0]);VpPrintE(stdout,v);printf("\n");
	VpFree(&gVpVars[i]);
	gVpVars[i] = v;
	return (VP_RETURN)FLAG64_NOMSG;
}
static VP_RETURN MaxLength(VP_ARGS args) {
	return (VP_RETURN)VpMaxLength((VP_HANDLE)args[0]);
}
static VP_RETURN CurLength(VP_ARGS args) {
	return (VP_RETURN)VpCurLength((VP_HANDLE)args[0]);
}
static VP_RETURN EffectiveDigits(VP_ARGS args) {
	return (VP_RETURN)VpEffectiveDigits((VP_HANDLE)args[0]);
}
static VP_RETURN Exponent(VP_ARGS args) {
	return (VP_RETURN)VpExponent((VP_HANDLE)args[0]);
}
static VP_RETURN PrintE(VP_ARGS args) {
	printf("  ");VpPrintE(stdout,(VP_HANDLE)args[0]);printf("\n");
	return (VP_RETURN)FLAG64_NOMSG;
}
static VP_RETURN PrintF(VP_ARGS args) {
	printf("  ");VpPrintF(stdout,(VP_HANDLE)args[0]);printf("\n");
	return (VP_RETURN)FLAG64_NOMSG;
}
static VP_RETURN StringLengthE(VP_ARGS args) {
	return (VP_RETURN)VpStringLengthE((VP_HANDLE)args[0]);
}
static VP_RETURN StringLengthF(VP_ARGS args) {
	return (VP_RETURN)VpStringLengthF((VP_HANDLE)args[0]);
}
static VP_RETURN ToStringE(VP_ARGS args) {
	VP_HANDLE v = (VP_HANDLE)args[0];
	char *s = (char*)malloc(VpStringLengthE(v));
	VpToStringE(v,s);
	printf("  %s\n",s);
	free(s);
	return (VP_RETURN)FLAG64_NOMSG;
}
static VP_RETURN ToStringF(VP_ARGS args) {
	VP_HANDLE v = (VP_HANDLE)args[0];
	char *s = (char*)malloc(VpStringLengthF(v));
	VpToStringF(v,s);
	printf("  %s\n",s);
	free(s);
	return (VP_RETURN)FLAG64_NOMSG;
}
static VP_RETURN RevertSign(VP_ARGS args) {
	return (VP_RETURN)VpRevertSign((VP_HANDLE)args[0]);
}
static VP_RETURN Abs(VP_ARGS args) {
	return (VP_RETURN)VpAbs((VP_HANDLE)args[0]);
}
static VP_RETURN Add(VP_ARGS args) {
	return (VP_RETURN)VpAdd((VP_HANDLE)args[0],(VP_HANDLE)args[1],(VP_HANDLE)args[2]);
}
static VP_RETURN Sub(VP_ARGS args) {
	return (VP_RETURN)VpSub((VP_HANDLE)args[0],(VP_HANDLE)args[1],(VP_HANDLE)args[2]);
}
static VP_RETURN Mul(VP_ARGS args) {
	VP_HANDLE c = (VP_HANDLE)args[0];
	VP_HANDLE a = (VP_HANDLE)args[1];
	VP_HANDLE b = (VP_HANDLE)args[2];
	if(c==a||c==b) {
		printf(" Invalid operation(output==input)!\n");
		VpSetNaN(c);
		return (VP_RETURN)FLAG64_NOMSG;
	}
	if(VpMaxLength(c) <= VpCurLength(a)+VpCurLength(b)) {
		VpFree( &(gVpVars[gixVars[0]]) );
		c = VpMemAlloc(VpCurLength(a)+VpCurLength(b)+1);
		gVpVars[gixVars[0]] = c;
	}
	return (VP_RETURN)VpMul(c,a,b);
}
static VP_RETURN Div(VP_ARGS args) {
	VP_HANDLE c = (VP_HANDLE)args[0];
	VP_HANDLE r = (VP_HANDLE)args[1];
	VP_HANDLE a = (VP_HANDLE)args[2];
	VP_HANDLE b = (VP_HANDLE)args[3];
	VP_UINT n;
	if(c==a||c==b||c==r||r==a||r==b) {
		printf(" Invalid operation(output==input)!\n");
		VpSetNaN(c);
		return (VP_RETURN)FLAG64_NOMSG;
	}

	 /*      VpMaxLength(r) > max(VpCurLength(a),VpMacLength(c)+VpCurLength(b)) */ 

	n = VpCurLength(b)+VpMaxLength(c)+1;
	if(n<VpCurLength(a)) n = VpCurLength(a)+1;
	if(VpMaxLength(r) <= n) {
		VpFree( &(gVpVars[gixVars[1]]) );
		r = VpMemAlloc(n+1);
		gVpVars[gixVars[1]] = r;
	}
	VpDiv(c,r,a,b);
	printf("  %c=",(char)(gixVars[0]+'a'));VpPrintE(stdout,c);printf("\n");
	printf("  %c=",(char)(gixVars[1]+'a'));VpPrintE(stdout,r);printf("\n");
	return (VP_RETURN)FLAG64_NOMSG;
}
static VP_RETURN Cmp(VP_ARGS args) {
	return (VP_RETURN)VpCmp((VP_HANDLE)args[0],(VP_HANDLE)args[1]);
}

static VP_RETURN Asgn0(VP_ARGS args) {
	return (VP_RETURN)VpAsgn((VP_HANDLE)args[0],(VP_HANDLE)args[1],1);
}

static VP_RETURN Asgn(VP_ARGS args) {
	return (VP_RETURN)VpAsgn((VP_HANDLE)args[0],(VP_HANDLE)args[1],(int)args[2]);
}
static VP_RETURN Asgn2(VP_ARGS args) {
	int i = (int)args[3];
	if(i<1 || i>7) {
		fprintf(stderr,"  Invalid round mode,enter 1-7\n");
		PrintMode(stderr);
		return (VP_RETURN)FLAG64_NOMSG;
	}
	return (VP_RETURN)VpAsgn2((VP_HANDLE)args[0],(VP_HANDLE)args[1],(int)args[2],(int)args[3]);
}
static VP_RETURN ScaleRound(VP_ARGS args) {
	return (VP_RETURN)VpScaleRound((VP_HANDLE)args[0],(int)args[1]);
}
static VP_RETURN LengthRound(VP_ARGS args) {
	int i = (int)args[1];
	return (VP_RETURN)VpLengthRound((VP_HANDLE)args[0],i);
}
static VP_RETURN ScaleRound2(VP_ARGS args) {
	int i = (int)args[1];
	int f = (int)args[2];
	if(f<1 || f>7) {
		fprintf(stderr,"  Invalid round mode,enter 1-7\n");
		PrintMode(stderr);
		return (VP_RETURN)FLAG64_NOMSG;
	}
	return (VP_RETURN)VpScaleRound2((VP_HANDLE)args[0],i,f);
}
static VP_RETURN LengthRound2(VP_ARGS args) {
	int i = (int)args[1];
	int f = (int)args[2];
	if(f<1 || f>7) {
		fprintf(stderr,"  Invalid round mode,enter 1-7\n");
		PrintMode(stderr);
		return (VP_RETURN)FLAG64_NOMSG;
	}
	return (VP_RETURN)VpLengthRound2((VP_HANDLE)args[0],i,f);
}

static VP_RETURN Frac(VP_ARGS args) {
	return (VP_RETURN)VpFrac((VP_HANDLE)args[0],(VP_HANDLE)args[1]);
}
static VP_RETURN Fix(VP_ARGS args) {
	return (VP_RETURN)VpFix((VP_HANDLE)args[0],(VP_HANDLE)args[1]);
}

static VP_RETURN Sqrt(VP_ARGS args) {
	return (VP_RETURN)VpSqrt((VP_HANDLE)args[0],(VP_HANDLE)args[1]);
}
static VP_RETURN PI(VP_ARGS args) {
	return (VP_RETURN)VpPI((VP_HANDLE)args[0]);
}
static VP_RETURN Exp(VP_ARGS args) {
	return (VP_RETURN)VpExp((VP_HANDLE)args[0],(VP_HANDLE)args[1]);
}
static VP_RETURN Sin(VP_ARGS args) {
	return (VP_RETURN)VpSin((VP_HANDLE)args[0],(VP_HANDLE)args[1]);
}
static VP_RETURN Cos(VP_ARGS args) {
	return (VP_RETURN)VpCos((VP_HANDLE)args[0],(VP_HANDLE)args[1]);
}
static VP_RETURN Atan(VP_ARGS args) {
	return (VP_RETURN)VpAtan((VP_HANDLE)args[0],(VP_HANDLE)args[1]);
}
static VP_RETURN Log(VP_ARGS args) {
	return (VP_RETURN)VpLog((VP_HANDLE)args[0],(VP_HANDLE)args[1]);
}
static VP_RETURN Power(VP_ARGS args) {
	return (VP_RETURN)VpPower((VP_HANDLE)args[0],(VP_HANDLE)args[1],(int)args[2]);
}

typedef struct _VP_TOKEN {
	char *szKeyName;
	VPF   vpf;
	char  chRet; /* return type */
	int   nArg;
	char  chArgs[MAX_TOKEN]; /* 
							   'i':int,
							   'u':unsigned int,
							   'c':char,
							   's':string,
							   'f':bool,
							   'r':round mode,
							   'S':VP-sign,
							   'v':VP_HANDLE(numeric literal allowed,RHSV)
							   'V':VP_HANDLE.
				 		    */
} VP_TOKEN;
static VP_TOKEN gTokens[] =
{
	{"#?",GetDefSize,'u',0,0,0,0,0,0},
	{"#=",SetDefSize,'u',1,'u',0,0,0,0},
	{"GetDigitSeparationCount",GetDigitSeparationCount,'u',0, 0 ,0,0,0,0},
	{"dc?",GetDigitSeparationCount,'u',0, 0 ,0,0,0,0},
	{"SetDigitSeparationCount",SetDigitSeparationCount,'u',1,'u',0,0,0,0},
	{"dc=",SetDigitSeparationCount,'u',1,'u',0,0,0,0},

	{"GetDigitSeparator",GetDigitSeparator,'c',0,0,0,0,0,0},
	{"ds?",GetDigitSeparator,'c',0,0,0,0,0,0},
	{"SetDigitSeparator",SetDigitSeparator,'c',1,'c',0,0,0,0},
	{"ds=",SetDigitSeparator,'c',1,'c',0,0,0,0},

	{"GetDigitLeader",GetDigitLeader,'c',0,0,0,0,0,0},
	{"dl?",GetDigitLeader,'c',0,0,0,0,0,0},
	{"SetDigitLeader",SetDigitLeader,'c',1,'c',0,0,0,0},
	{"dl=",SetDigitLeader,'c',1,'c',0,0,0,0},

	{"GetRoundMode",GetRoundMode,'i',0,0,0,0,0,0},
	{"rm?",GetRoundMode,'i',0,0,0,0,0,0},
	{"SetRoundMode",SetRoundMode,'i',1,'i',0,0,0,0},
	{"rm=",SetRoundMode,'i',1,'i',0,0,0,0},
	
	{"GetMaxIterationCount",GetMaxIterationCount,'u',0,0,0,0,0,0},
	{"mi?",GetMaxIterationCount,'u',0,0,0,0,0,0},
	{"SetMaxIterationCount",SetMaxIterationCount,'u',1,'u',0,0,0,0},
	{"mi=",SetMaxIterationCount,'u',1,'u',0,0,0,0},
	{"GetIterationCount",GetIterationCount,'u',0,0,0,0,0,0},
	{"ic?",GetIterationCount,'u',0,0,0,0,0,0},

	{"IsNumeric",IsNumeric,'f',1,'v',0,0,0,0},
	{"GetTag",GetTag,'u',1,'v',0,0,0,0},
	{"SetTag",SetTag,0,2,'v','u',0,0,0},
	{"SetSign",SetSign,'v',2,'v','i',0,0,0},
	{"GetSign",GetSign,'i',1,'v',0,0,0,0},
	{"IsOne",IsOne,'f',1,'v',0,0,0,0},
	{"SetOne",SetOne,'v',1,'v',0,0,0,0},
	{"IsPosZero",IsPosZero,'f',1,'v',0,0,0,0},
	{"SetPosZero",SetPosZero,'v',1,'v',0,0,0,0},
	{"IsNegZero",IsNegZero,'f',1,'v',0,0,0,0},
	{"SetNegZero",SetNegZero,'v',1,'v',0,0,0,0},
	{"IsZero",IsZero,'f',1,'v',0,0,0,0},
	{"SetZero",SetZero,'v',2,'v','i',0,0,0},
	{"IsNaN",IsNaN,'f',1,'v',0,0,0,0},
	{"SetNaN",SetNaN,'v',1,'v',0,0,0,0},
	{"IsPosInf",IsPosInf,'f',1,'v',0,0,0,0},
	{"SetPosInf",SetPosInf,'v',1,'v',0,0,0,0},
	{"IsInf",IsInf,'f',1,'v',0,0,0,0},
	{"SetInf",SetInf,'v',2,'v','i',0,0,0},
	{"IsNegInf",IsNegInf,'f',1,'v',0,0,0,0},
	{"SetNegInf",SetNegInf,'v',1,'v',0,0,0,0},

	{"Alloc",Alloc,'v',3,'c','s','u',0,0},
	{"al",Alloc,'v',3,'c','s','u',0,0},
	{"=",Alloc0,'v',2,'c','s',0,0,0},

	{"Load",Load,'v',2,'c','s',0,0,0},
	{"ld",Load,'v',2,'c','s',0,0,0},


	{"MemAlloc",MemAlloc,'v',2,'c','u',0,0,0},
	{"ma",MemAlloc,'v',2,'c','u',0,0,0},
	
	{"Clone",Clone,'v',2,'c','v',0,0},

	{"MaxLength",MaxLength,'u',1,'v',0,0,0,0}, 
	{"ml",MaxLength,'u',1,'v',0,0,0,0}, 
	{"CurLength",CurLength,'u',1,'v',0,0,0,0},
	{"cl",CurLength,'u',1,'v',0,0,0,0},

	{"EffectiveDigits",EffectiveDigits,'u',1,'v',0,0,0,0},
	{"Exponent",Exponent,'i',1,'v',0,0,0,0},
	{"PrintE",PrintE,'i',1,'v',0,0,0,0},
	{"?E",PrintE,'i',1,'v',0,0,0,0},
	{"PrintF",PrintF,'i',1,'v',0,0,0,0},
	{"?F",PrintF,'i',1,'v',0,0,0,0},
	{"StringLengthE",StringLengthE,'u',1,'v',0,0,0,0},
	{"StringLengthF",StringLengthF,'u',1,'v',0,0,0,0},
	{"ToStringE",ToStringE,'s',1,'v',0,0,0,0},
	{"ToStringF",ToStringF,'s',1,'v',0,0,0,0},
	{"RevertSign",RevertSign,'v',1,'v',0,0,0,0},
	{"Negate",RevertSign,'v',1,'v',0,0,0,0},
	{"Abs",Abs,'v',1,'v',0,0,0,0},
	{"Add",Add,'v',3,'V','v','v',0,0},
	{"+",Add,'v',3,'V','v','v',0,0},
	{"Sub",Sub,'v',3,'V','v','v',0,0},
	{"-",Sub,'v',3,'V','v','v',0,0},
	{"Mul",Mul,'v',3,'V','v','v',0,0},
	{"*",Mul,'v',3,'V','v','v',0,0},
	{"Div",Div,'v',4,'V','V','v','v',0},
	{"/",Div,'v',4,'V','V','v','v',0},
	{"Cmp",Cmp,'i',2,'v','v',0,0,0},
	{"Asgn",Asgn,'v',3,'V','v','i',0,0},
	{"<-",Asgn0,'v',2,'V','v',0,0,0},
	{"Asgn2",Asgn2,'v',4,'V','v','i','i',0},
	{"ScaleRound",ScaleRound,'v',2,'v','i',0,0,0},
	{"sr",ScaleRound,'v',2,'v','i',0,0,0},
	{"LengthRound",LengthRound,'v',2,'v','i',0,0,0},
	{"lr",LengthRound,'v',2,'v','i',0,0,0},
	{"ScaleRound2",ScaleRound2,'v',3,'v','i','i',0,0},
	{"LengthRound2",LengthRound2,'v',3,'v','i','i',0,0},
	{"Frac",Frac,'v',2,'V','v',0,0,0},
	{"Fix",Fix,'v',2,'V','v',0,0,0},
	{"Int",Fix,'v',2,'V','v',0,0,0},
	{"Sqrt",Sqrt,'v',2,'V','v',0,0,0},
	{"PI",PI,'v',1,'V',0,0,0,0},
	{"Exp",Exp,'v',2,'V','v',0,0,0},
	{"Sin",Sin,'v',2,'V','v',0,0,0},
	{"Cos",Cos,'v',2,'V','v',0,0,0},
	{"Atan",Atan,'v',2,'V','v',0,0,0},
	{"Log",Log,'v',2,'V','v',0,0,0},
	{"Power",Power,'v',3,'V','v','i',0,0},
	{"**",Power,'v',3,'V','v','i',0,0}
};
static int gnMaxTokens = sizeof(gTokens)/sizeof(gTokens[0]);


/*
	Input:  char gszBuf[MAX_BUFF]       raw data read from stdin.
	output: char *gpszToken[MAX_TOKEN]  pointer array to each token.
	        int   gnToken               number of tokens
	return:
	      0 ... failure
		  1 ... valid tokens read.
*/
static int Tokenize()
{
	int   i;
	char ch;
	char *pszBuf = gszBuf;
	int   n = strlen(gszBuf);

	/* remove trailing spaces */
	for(i=n-1;i>=0;--i) {
		if(isspace(pszBuf[i]))  gszBuf[i] = 0;
		else break;
	}
	if(!gszBuf[0]) return 0;

	/* skip leading spaces */
	while(isspace(ch=*pszBuf)) ++pszBuf;
	if(ch>='a' && ch<='z') {
		if(pszBuf[1]==0) {
			printf("  %c=",ch);VpPrintE(stdout,gVpVars[ch-'a']);printf("\n");
			return 0;
		}
	}

	gnToken = -1;
Set:
	if(++gnToken>=MAX_TOKEN) {fprintf(stderr,"statement unrecognizable!\n");return 0;}
	if(*pszBuf=='\'') {
		gpszToken[gnToken] = ++pszBuf;
		while(*pszBuf!='\'') {
			if(!(*pszBuf)) {
				fprintf(stderr,"imcomplete string!\n");
				return 0;
			}
			++pszBuf;
		}
	} else {
		gpszToken[gnToken] = pszBuf;
		while(*pszBuf && !isspace(*pszBuf)) ++pszBuf;
	}
	*pszBuf++ = 0;
	while(isspace(*pszBuf)) ++pszBuf;
	if(*pszBuf) goto Set;
	gnToken++;
	return gnToken; /* Number of tokens */
}

static int GetToken(VP_TOKEN *pToken)
{
	int i;
	for(i=0;i<gnMaxTokens;++i) {
		if(strcasecmp(gTokens[i].szKeyName,gpszToken[0])==0) {
			*pToken = gTokens[i];
			/* check on number of tokens */
			if((gnToken-1)!=gTokens[i].nArg) {
				fprintf(stderr,"Bad number of arguments!\n");
				return 0;
			}
			return 1; /* Keyword found */
		}
	}
	fprintf(stderr,"%s:undefined!\n",gpszToken[0]);
	return 0;
}

static void VpCall(VP_TOKEN *pToken)
{
	int       iv;
	int       i;
	int       v;
	int       iarg = 0;
	VP_ARG    args[MAX_TOKEN];
	int       ixv = 0;
	int       nw = 0; /* Number of work variables */
	VP_HANDLE W[2];
	VP_RETURN ret;

	W[0] = 0;
	W[1] = 0;
	for(i=1;i<gnToken;++i)
	{
		switch(pToken->chArgs[i-1])
		{
		case 'i': /* int */
		case 'S': /* VP-sign */
		case 'u': /* unsigned int */
		case 'f': /* bool */
		case 'r': /* round mode */
			if(0 >= sscanf(gpszToken[i], "%d", &v) ) {
				fprintf(stderr,"Invalid argument(%s)!\n",gpszToken[i]);
				return;
			}
			if(v<0 &&(pToken->chArgs[i-1]=='u')) {
				fprintf(stderr,"Invalid argument(%s)!\n",gpszToken[i]);
				return;
			}
			args[iarg++] = (VP_ARG)(v);
			break;
		case 'c': /* char */
			if(strlen(gpszToken[i])!=1) {
				fprintf(stderr,"Invalid argument(%s)!\n",gpszToken[i]);
				return;
			}
			args[iarg++] = (VP_ARG)(gpszToken[i][0]);
			break;
		case 's': /* string */
			args[iarg++] = (VP_ARG)(gpszToken[i]);
			break;
		case 'v': /* VP_HANDLE('a'-'z') or numeric string */
			iv = (int)(gpszToken[i][0])-(int)'a';
			if(iv<0||iv>=gnVpVars) {
				VP_HANDLE v = VpAlloc(gpszToken[i],1);
				if(VpIsValid(v)) {
					args[iarg++]  = v;
					W[nw++]       = v;
					gixVars[ixv++] = iv;
					break;
				}
			}
		case 'V': /* VP_HANDLE only */
			if(strlen(gpszToken[i])!=1) {
				fprintf(stderr,"Invalid argument(%s)!\n",gpszToken[i]);
				return;
			}
			iv = (int)(gpszToken[i][0])-(int)'a';
			if(iv<0||iv>=(sizeof(gVpVars)/sizeof(gVpVars[0]))) {
				fprintf(stderr,"Invalid argument(%s)!\n",gpszToken[i]);
				return;
			}
			args[iarg++]  = gVpVars[iv];
			gixVars[ixv++] = iv;
			break;
		default:
			fprintf(stderr,"System error: Undefined argument!!\n");
		}
	}
	
	/* Call vp rourine */
	ret = (*pToken->vpf)(args); /* call */
	while(nw>0) VpFree(&W[--nw]);
	if(FLAG64_NOMSG == ret) return; /* Message already output and no need to print any more */

	switch(pToken->chRet)
	{
		case 'i': /* int */
		case 'S': /* VP-sign */
		case 'u': /* unsigned int */
		case 'f': /* bool */
		case 'r': /* round mode */
			printf("  %d\n",(int)ret);
			break;
		case 'c': /* char */
			printf("  '%c'\n",(char)ret);
			break;
		case 's': /* string */
			break;
		case 'v': /* VP_HANDLE('a'-'z') */
			printf("  %c=",(char)(gixVars[0]+'a'));VpPrintE(stdout,(VP_HANDLE)ret);printf("\n");
			break;
		default:
			fprintf(stderr,"System error: Undefined argument!!\n");
	}
}

int main(int argc, char* argv[])
{
	int      i;
	VP_TOKEN token;

	printf("VPC(Variable Precision Calculator) of Bigdecimal(V%d)\n",VpVersion());
	printf("  Copyright (c) 2018 by Shigeo Kobayashi. Allrights reserved.\n\n");
	printf("Enter command\n");

	for(i=0;i<(sizeof(gVpVars)/sizeof(gVpVars[0]));++i) {
		gVpVars[i] = VpMemAlloc(DEF_SIZE);
	}

	printf("? ");
	while(fgets(gszBuf,MAX_BUFF,stdin)!=(char)0) {
		if(Tokenize()) {
			if(gnToken==1 && strcmp("quit",gpszToken[0])==0) break;
			if(!GetToken(&token)) goto Next;
			VpCall(&token);
		}
	Next:
		printf("? ");
	}

	for(i=0;i<(sizeof(gVpVars)/sizeof(gVpVars[0]));++i) {
		VpFree(&(gVpVars[i]));
	}
	return 0;
}
