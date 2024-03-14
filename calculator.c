/*
 * Variable precision calculator.
 * (Test program for Bigdecimal(Variable decimal precision) C/C++ library.)
 *
 *  Copyright(C) 2024 by Shigeo Kobayashi(shigeo@tinyforest.jp)
 *
 *    Version 2.0
 *
 */
#include "vpc.h"

static VP_HANDLE VpcAbs(VP_HANDLE v) { return VpAbs(v); }
static VP_HANDLE VpcSin(VP_HANDLE v, VP_HANDLE a) { return VpSin(v, a); }
static VP_HANDLE VpcCos(VP_HANDLE v, VP_HANDLE a) { return VpCos(v, a); }
static VP_HANDLE VpcAtn(VP_HANDLE v, VP_HANDLE a) { return VpAtan(v, a); }
static VP_HANDLE VpcExp(VP_HANDLE v, VP_HANDLE a) { return VpExp(v, a); }
static VP_HANDLE VpcLog(VP_HANDLE v, VP_HANDLE a) { return VpLog(v, a); }
static VP_HANDLE VpcPI (VP_HANDLE v) { return VpPI(v); }
static VP_HANDLE VpcInt(VP_HANDLE v, VP_HANDLE a) { return VpInt(v, a); }
static VP_HANDLE VpcFrac(VP_HANDLE v, VP_HANDLE a) { return VpFrac(v, a); }
static VP_HANDLE VpcSqrt(VP_HANDLE v, VP_HANDLE a) { return VpSqrt(v, a); }
static VP_HANDLE VpcTrim(VP_HANDLE v, int nt)      { return VpLengthRound(v, nt); }
static VP_HANDLE VpcRound(VP_HANDLE v, int nt)     { return VpScaleRound (v, nt); }
static VP_HANDLE VpcPower(VP_HANDLE v, VP_HANDLE a,int nt)   { return VpPower(v, a, nt); }
static VP_HANDLE VpcDigits(VP_HANDLE v,VP_HANDLE va) {
	char sz[32];
	int n = VpEffectiveDigits(va);
	sprintf(sz, "%d", n);
	return VpLoad(v, sz);
};
static VP_HANDLE VpcExponent(VP_HANDLE v,VP_HANDLE va) {
	char sz[32];
	int n = VpExponent(va);
	sprintf(sz, "%d", n);
	return VpLoad(v, sz);
};
static VP_HANDLE VpcIterations(VP_HANDLE v) {
	char sz[32];
	int n = VpGetIterationCount();
	sprintf(sz, "%d", n);
	return VpLoad(v, sz);
};

FUNCTION  gFunctions[] = {
	{"abs",VpcAbs,1},
	{"sin",VpcSin,1},
	{"cos",VpcCos,1},
	{"atan",VpcAtn,1 },
	{"exp",VpcExp,1 },
	{"pi", VpcPI,0 },
	{"iterations", VpcIterations,0 },
	{"ln", VpcLog,1},
	{"int", VpcInt,1},
	{"frac", VpcFrac,1},
	{"sqrt", VpcSqrt,1},
	{"trim", VpcTrim,2},
	{"round", VpcRound,-2},
	{"power", VpcPower,2},
	{"digits", VpcDigits,1},
	{"exponent", VpcExponent,1}
};
int       gmFunctions = sizeof(gFunctions)/ sizeof(gFunctions[0]);

VP_HANDLE gVariables[27];
int       gmVariables = sizeof(gVariables) / sizeof(gVariables[0]);

VP_HANDLE gWorkVariables[512];
int       gmWorkVariables = sizeof(gWorkVariables) / sizeof(gWorkVariables[0]);
static int       gcWorkVariables = 0;

void InitCalculator()
{

}

void FinishCalculator()
{
}

static int GetPolish(int ip)
{
	return gPolish[ip];
}

static void SetPolish(int ip,int v)
{
	gPolish[ip] = v;
}

static int PolishCount()
{
	return gcPolish;
}

static void PolishReplace(int v, int ixs, int ixe)
{
	gPolish[ixs++] = v;
	if (ixs > ixe) return;
	while (++ixe < gcPolish) {
		gPolish[ixs++] = gPolish[ixe];
	}
	gcPolish = ixs;
}

const UCHAR* FunctionName(int i)
{
	return gFunctions[i].name;
}
int FunctionArguments(int i)
{
	return gFunctions[i].arguments;
}

static void * FunctionAddress(int i)
{
	return gFunctions[i].func;
}


static int CreateWorkVariable(VP_HANDLE *pv,int mx)
{
	VP_HANDLE wv;
	*pv = 0;
	if (gcWorkVariables >= gmWorkVariables) {
		ERROR(fprintf(stderr, "SYSTEM_ERROR: too many working variables are required(%d)\n", gmWorkVariables));
		return 0;
	}
	if (mx < gmPrecision+10) mx = gmPrecision+10;
	wv = gWorkVariables[gcWorkVariables];
	if (VpIsInvalid(wv)) gWorkVariables[gcWorkVariables] = VpMemAlloc(mx);
	else {
		if (VpMaxLength(wv) < (unsigned int)mx) {
			VpFree(&wv);
			gWorkVariables[gcWorkVariables] = VpMemAlloc(mx);
		}
	}
	if (VpIsInvalid(gWorkVariables[gcWorkVariables])) {
		ERROR(fprintf(stderr, "SYSTEM_ERROR: unable to allocate working variable.\n"));
		return 0;
	}
	*pv = gWorkVariables[gcWorkVariables++];
	return -gcWorkVariables;
}

int EnsureVariable(int iv, int mx)
{
	VP_HANDLE v;
	int mxv;

	if (iv<0 && ((-iv) - 1) >= gmWorkVariables) {
		ERROR(fprintf(stderr, "SYSTEM_ERROR: invalid variable is to be processed(%d)\n", iv));
		return 0;
	}

	v = gVariables[iv];
	if (VpIsInvalid(v)) v = VpMemAlloc(mx);
	else {
		mxv = VpMaxLength(v);
		if (mxv < mx) {
			VpFree(&v);
			v = VpMemAlloc(mx);
		}
	}
	if (VpIsInvalid(v)) {
		ERROR(fprintf(stderr, "SYSTEM_ERROR: mamory allocation failed(size=%d).\n", mx));
		return 0;
	}
	gVariables[iv] = v;
	return 1;
}

int CreateNumericWorkVariable(UCHAR *szN)
{
	VP_HANDLE vw;

	if (gcWorkVariables >= gmWorkVariables) {
		FATAL(fprintf(stderr, "SYSTEM_ERROR: too many working variables(%d),\n", gmWorkVariables));
	}

	vw = gWorkVariables[gcWorkVariables];
	if (VpIsInvalid(vw)) {
		vw = VpAlloc(szN, gmPrecision);
	} else if(szN) {
		int l = strlen(szN);
		int mx = VpMaxLength(vw);
		if (mx < l) {
			VpFree(&vw);
			vw = VpAlloc(szN, gmPrecision);
		} else {
			vw = VpLoad(vw,szN);
		}
	}
	else {
		int mx = VpMaxLength(vw);
		if (mx < gmPrecision) {
			VpFree(&vw);
			vw = VpMemAlloc(gmPrecision);
		}
	}
	if (VpIsInvalid(vw)) {
		FATAL(fprintf(stderr, "SYSTEM_ERROR: unable to create working variable.\n"));
		return 0;
	}
	gWorkVariables[gcWorkVariables++] = vw;
	return -gcWorkVariables;
}

static VP_HANDLE GetPolishVariable(int ixp)
{
	int ixv = GetPolish(ixp);
	if(ixv>=0) return gVariables[ixv];
	return gWorkVariables[-(ixv + 1)];
}

static int Negate(int ixp)
{
	VP_HANDLE pw, w; /* w1=-w */
	int ixw = CreateWorkVariable(&w,gmPrecision);

	if (ixw >= 0) return -1; /* failed to create new work variable */

	pw = GetPolishVariable(ixp-1);
	
	if (VpIsInvalid(VpAsgn(w, pw, -1))) {
		ERROR(fprintf(stderr, "SYSTEM_ERROR: negation failed.\n"));
		return -1;
	}
	PolishReplace(ixw,ixp-1,ixp);
	return ixp - 1;
}

static int DoBinaryOperation(int iStatement,int ixp, int token)
{
	VP_HANDLE v1 = GetPolishVariable(ixp - 2);
	VP_HANDLE v2 = GetPolishVariable(ixp - 1);
	VP_HANDLE w; 
	int ixw;

	int l1 = VpCurLength(v1);
	int l2 = VpCurLength(v2);
	int e1 = VpExponent(v1);
	int e2 = VpExponent(v2);
	int l = l1;
	int d = 0;

	UCHAR ch = TokenChar(iStatement,token,0);
	switch (ch)
	{
	case '+':
		if (l1 < l2) l = l2;
		d = e1 - e2;
		if (d < 0) d = -d;
		ixw = CreateWorkVariable(&w, l + d + 2);
		VpAdd(w, v1, v2);
		break;
	case '-':
		if (l1 < l2) l = l2;
		d = e1 - e2;
		if (d < 0) d = -d;
		ixw = CreateWorkVariable(&w, l + d + 2);
		VpSub(w, v1, v2);
		break;
	case '*':
		ixw = CreateWorkVariable(&w, l1 + l2 + 2);
		VpMul(w, v1, v2);
		break;
	case '/':
		ixw = CreateWorkVariable(&w, l1 + l2 + 2);
		d = VpMaxLength(w) + VpCurLength(v2);
		if (d < (int)VpCurLength(v1)) d = VpCurLength(v1);
		EnsureVariable(26, d);
		VpDiv(w, gVariables[26], v1, v2);
		break;
	case '=':
		VpAsgn(v1, v2, 1);
		ixw = GetPolish(ixp - 1);
		break;
	default:
		ERROR(fprintf(stderr, "SYSTEM_ERROR: invalid binary operator(%c).\n", ch));
		return -1;
	}
	PolishReplace(ixw, ixp - 2, ixp);
	return ixp-2;
}

int ToIntFromSz(int* pi, UCHAR* sz)
{
	int  sign = 1;
	int  s    = 0;
	int  i    = 0;
	int dot   = 0;

	UCHAR ch;

	while (isspace(*sz)) sz++;
	ch = *sz;
	if (ch == '\'' || ch == '\"') sz++;
	while(ch=*sz++) {
		if (ch == '\'' || ch == '\"') {
			if (*sz != '\0') {
				ERROR(fprintf(stderr, "Error: Integer character(not digit).\n"));
				return 0;
			}
			ch = *sz++;
		}
		if (ch == '\0') break;
		if (i == 0 && ch == '+') continue;
		if (i == 0 && ch == '-') { sign = -1; continue; }
		if (ch == gchSeparator )  continue;
		if (ch == '.') { dot = 1; continue; }
		++i;
		ch -= '0';
		if ((ch!=0 && dot!=0) || ( ch < 0 || ch>9)) {
			ERROR(fprintf(stderr, "Error: Integer value required.\n"));
			return 0;
		}
		s = s * 10 + ch;
	}
	*pi = sign * s;
	return 1;
}

static int ToInteger(int *pi,VP_HANDLE v) {
	static UCHAR szInt[50];
	static int ni = sizeof(szInt) / sizeof(szInt[0]);
	int         s = VpStringLengthF(v);
	if (s >= ni) {
		ERROR(fprintf(stderr, "Error: illegal argument(too big integer value).\n"));
		return -1;
	}
	VpToStringF(v, szInt);
	if (!ToIntFromSz(pi, szInt)) return 0;
	return 1;
}

int DoFunction(int ixp,int iSatement, int token)
{
	int iw  = TokenWhat2(iSatement,token);
	int na  = FunctionArguments(iw);
	void* f = FunctionAddress(iw);
	if (na == 0) {
		VP_HANDLE v;
		int ixw = CreateWorkVariable(&v, gmPrecision);
		((VP_HANDLE(*)(VP_HANDLE))f)(v);
		PolishReplace(ixw, ixp, ixp);
		return ixp;
	}
	else if (na == 1) {
		VP_HANDLE v;
		int       ixw = CreateWorkVariable(&v, gmPrecision);
		VP_HANDLE va  = GetPolishVariable(ixp - 1);
		if (f == VpcAbs) {
			VpAsgn(v, va, 1);
			((VP_HANDLE(*)(VP_HANDLE))f)(v);
			PolishReplace(ixw, ixp - 1, ixp);
			return ixp - 1;
		}
		((VP_HANDLE(*)(VP_HANDLE,VP_HANDLE))f)(v,va);
		PolishReplace(ixw, ixp-1, ixp);
		return ixp-1;
	}
	else if (na == 2 && f == VpcPower) {
		/* Power() */
		VP_HANDLE v;
		int       s;
		int ixw = CreateWorkVariable(&v, gmPrecision);
		VP_HANDLE a2 = GetPolishVariable(ixp - 1);
		VP_HANDLE a1 = GetPolishVariable(ixp - 2);
		if (!ToInteger(&s, a2)) return -1;
		((VP_HANDLE(*)(VP_HANDLE, VP_HANDLE, int))f)(v, a1, s);
		PolishReplace(ixw, ixp - 2, ixp);
		return ixp - 2;
	}
	else if (na == 2 || na==-2) {
		VP_HANDLE v;
		int       s;
		int ixw      = CreateWorkVariable(&v, gmPrecision);
		VP_HANDLE a2 = GetPolishVariable(ixp - 1);
		VP_HANDLE a1 = GetPolishVariable(ixp - 2);
		if (!ToInteger(&s, a2)) return -1;
		VpAsgn(v, a1, 1);
		((VP_HANDLE(*)(VP_HANDLE, int))f)(v, s);
		PolishReplace(ixw, ixp - 2, ixp);
		return ixp - 2;
	}else {
		ERROR(fprintf(stderr, "Error:invalid number of arguments(%d).\n", na));
	}
	return  - 1;
}

void ComputePolish(int iStatement)
{
	int ip = 0;
	int token;
	int what;
	int iv;
	int ixs, ixe;
	UCHAR ch;

	gcWorkVariables = 0;
	while (ip < PolishCount()) {
		token = GetPolish(ip);
		what  = TokenWhat(iStatement,token);
		switch (what) {
		case VPC_VARIABLE:  /* a,b,c,..,z or R                         */
			iv = TokenWhat2(iStatement,token); /* a,b,c,... => 0,1,2,.... */
			if (!EnsureVariable(iv, gmPrecision)) return;
			SetPolish(ip, iv);
			break;
		case VPC_NUMERIC:   /* 1,2,3,... or '123...'                   */
			ch  = TokenChar(iStatement,token, 0);
			ixs = TokenFrom(iStatement,token);
			ixe = TokenTo(iStatement,token);
			iv = CreateNumericWorkVariable(TokenPTR(iStatement,token));
			if (iv >= 0) return; /* Failed to create anumeric working variable */
			SetPolish(ip, iv);
			break;
		case VPC_UOPERATOR: /*  unary  operator:  + -                  */
			ip = Negate(ip);
			if (ip < 0) return;
			break;
		case VPC_BOPERATOR: /*  binary operator:  + - * / =            */
			ip = DoBinaryOperation(iStatement,ip, token);
			if (ip < 0) return;
			break;
		case VPC_FUNC:      /* sin cos etc                             */
			if (ip < 0) return;
			ip = DoFunction(ip, iStatement,token);
			if (ip < 0) return;
			break;
		default:
			ERROR(fprintf(stderr, "SYSTEM_ERROR: invalid identifier %d(in reverse polish)\n", token));
			return;
		}
		++ip;
	}
}
