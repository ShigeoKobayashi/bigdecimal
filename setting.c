
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

int  gmPrecision   = 100;
int  gmIterations  = 10000;

UCHAR gszTitle[512];
int   gmTitle = sizeof(gszTitle) / sizeof(gszTitle[0]);

UCHAR* gszVTitle[26];
int    gmVTitle = sizeof(gszVTitle) / sizeof(gszVTitle[0]);

/* format */
int   gnCount       = 10;  /* digits separation count */
UCHAR gchFormatChar = 'E';
UCHAR gchSeparator  = ' ';
UCHAR gchQuote      = 'q';
UCHAR gchLeader     = '*';
int   gRoundMode    = VP_ROUND_HALF_UP;

typedef struct _ROUNDMODE {
	UCHAR* name;
	int   value;
} ROUNDMODE;
static ROUNDMODE gRMode[] = {
	{"up",VP_ROUND_UP},
	{"down",VP_ROUND_DOWN},
	{"half_up",VP_ROUND_HALF_UP},     /*  default */
	{"half_down",VP_ROUND_HALF_DOWN},
	{"ceil",VP_ROUND_CEIL},
	{"floor",VP_ROUND_FLOOR},
	{"half_even",VP_ROUND_HALF_EVEN}
};
static int gmRMode = sizeof(gRMode) / sizeof(gRMode[0]);

void PrintFormat(PARSER *p,FILE *f,int newline)
{
	if(newline) fprintf(f, "$format = '%d%c%c%c%c'\n", gnCount, gchLeader, gchFormatChar, gchSeparator, gchQuote);
	else        fprintf(f, "$format = '%d%c%c%c%c'; ", gnCount, gchLeader, gchFormatChar, gchSeparator, gchQuote);
}

void PrintPrecision(PARSER *p,FILE *f, int newline)
{
	if(newline) fprintf(f, "$precision = '%d'\n", gmPrecision);
	else        fprintf(f, "$precision = '%d'; ", gmPrecision);
}

void PrintRound(PARSER *p,FILE *f, int newline)
{
	int i;
	for (i = 0; i < gmRMode; ++i) {
		if (gRMode[i].value == gRoundMode) {
			if(newline) fprintf(f, "$round = '%s'\n", gRMode[i].name);
			else        fprintf(f, "$round = '%s'; ", gRMode[i].name);
			return;
		}
	}
	ERROR(fprintf(stderr,"SYSTE_ERROR:undefine current round mode(%d),reset!\n",gRoundMode));
	gRoundMode = VpGetRoundMode();
}

void PrintIterations(PARSER *p,FILE *f,int newline)
{
	if(newline) fprintf(f, "$max_iterations = '%d'\n", gmIterations);
	else        fprintf(f, "$max_iterations = '%d'; ", gmIterations);
}

static UCHAR GetQuote(UCHAR* psz)
{
	UCHAR ch;
	while (ch = *psz++) {
		if (ch == '\'') return '\"';
		if (ch == '\"') return '\'';
	}
	return'\"';
}
void PrintTitle(PARSER *p,FILE* f,int newline)
{
	UCHAR ch;
	
	if (gszTitle[0] == '\0') {
		if(newline) fprintf(f, "$title = ' '\n");
		else        fprintf(f, "$title = ' '; ");
	}
	else {
		ch = GetQuote(gszTitle); 
		if (newline) fprintf(f, "$title = %c%s%c\n", ch, gszTitle, ch);
		else         fprintf(f, "$title = %c%s%c; ", ch, gszTitle, ch);
	}
}

void OutputVariableTitle(FILE* f, UCHAR chv,int newline)
{
	int ixv = chv - 'a';
	UCHAR ch;

	if (ixv < 0 || ixv > 25) {
		ERROR(fprintf(stderr, "Error: undefined variable(%c)\n", chv));
		return;
	}
	if (gszVTitle[ixv] != NULL) {
		fprintf(f, " $%c = ", chv);
		ch = GetQuote(gszVTitle[ixv]);
		if(newline) fprintf(f, "%c%s%c\n", ch, gszVTitle[ixv],ch);
		else        fprintf(f, "%c%s%c; ", ch, gszVTitle[ixv], ch);
	}
	else {
		if (newline) fprintf(f, " $%c = ' '\n", chv);
		else         fprintf(f, " $%c = ' '; ", chv);
	}
}

void PrintVariableTitle(PARSER* p,FILE* f,int newline)
{
	UCHAR chv = TokenChar(p->r,1, 1);
	OutputVariableTitle(f, chv,newline);
}

void PrintVariable(FILE* f, UCHAR chv,int newline) 
{
	VP_HANDLE v;
	int ixv = chv - 'a';
	if (chv == 'R') ixv = 26;
	else {
		if (ixv < 0 || ixv > 25) {
			ERROR(fprintf(stderr,"Error: undefined identifier(%c)\n",chv));
			return;
		}
	}
	v = gVariables[ixv];
	fprintf(f," %c = ", chv);
	if (gchQuote == 'Q') fprintf(f,"%c", '\'');
	if (VpIsValid(v)) {
		if (gchFormatChar == 'E') VpPrintE(f, v);
		else                      VpPrintF(f, v);
	}
	else {
		fprintf(f,"0.0");
	}
	if (gchQuote == 'Q') fprintf(f,"%c", '\'');
	if(newline) fprintf(f, "\n");
	else        fprintf(f, "; ");
}

void DoPrint(PARSER *p)
{
	int i;
	FILE* f = stdout;
	int fNewLine = 1;
	if (TokenCount(p->r) == 3 && IsToken("+", p->r, 2)) fNewLine = 0;
	for (i = 0; i < gmSetting; ++i) {
		if (strcmp(TokenPTR(p->r,1),gSetting[i].name)==0) {
			((void(*)(PARSER *,FILE *,int))gSetting[i].print)(p,f, fNewLine); return;
		};
	}
	if (TokenSize(p->r,1) == 1)  { PrintVariable(f, TokenChar(p->r, 1, 0), fNewLine); return;}
	ERROR(fprintf(stderr, "Error: undefined variable(%s).\n", TokenPTR(p->r,1)));
	return;
}

void DoRound(PARSER *p)
{
	int i;

	for (i = 0; i < gmRMode; ++i) {
		if (IsToken(gRMode[i].name, p->r, 2)) { gRoundMode = gRMode[i].value; VpSetRoundMode(gRoundMode); return; }
	}
	ERROR(fprintf(stderr, "Error: undefined round mode(%s)", TokenPTR(p->r,2)));
}

void DoFormat(PARSER *p)
{
	int i,id,nd;
	UCHAR ch;
	int nch;
	nch = TokenSize(p->r,2);
	for (i = 0; i < nch; ++i) {
		ch = TokenChar(p->r, 2,  i);
		if (ch == '\'' || ch == '\"') continue;
		if (ch == 'E') { gchFormatChar = 'E'; continue; }
		if (ch == 'F') { gchFormatChar = 'F'; continue; }
		if (ch == ' ') { gchSeparator  = ' '; VpSetDigitSeparator(' '); continue; }
		if (ch == ',') { gchSeparator  = ','; VpSetDigitSeparator(','); continue; }
		if (ch == 'Q') { gchQuote      = 'Q'; continue; }
		if (ch == 'q') { gchQuote      = 'q'; continue; }
		if (ch == '+') { gchLeader     = '+'; VpSetDigitLeader('+'); continue; }   /* '+' for positive number */
		if (ch == '-') { gchLeader     = '-'; VpSetDigitLeader('\0'); continue; }  /* no ' ' before positive number */
		if (ch == '*') { gchLeader     = '*'; VpSetDigitLeader(' '); continue; }   /* ' ' before positive number */
		id = ch - '0';
		if (id >= 0 && id <= 9) {
			nd = 0;
			while (i < nch) {
				nd = nd * 10 + id;
				ch = TokenChar(p->r, 2, i+1);
				id = ch - '0';
				if (id < 0 || id > 9) break;
				++i;
			}
			gnCount = nd;
			VpSetDigitSeparationCount(nd);
		}
		else {
			ERROR(fprintf(stderr, "Warning: illegal format character(%c ignored).\n", ch));
		}
	}
}

void DoPrecision(PARSER *p)
{
	int  nd;
	VP_HANDLE v1, v2;
	int i;
	if (!ToIntFromSz(&nd, TokenPTR(p->r,2))) return;
	if (nd < 20) {
		ERROR(fprintf(stderr, "Error: negative or too small value for $precision(%d).\n", nd)); return;
	}
	gmPrecision = nd;
	for (i = 0; i < gmVariables-1; ++i) {
		if (VpIsInvalid(gVariables[i])) continue;
		v1 = VpMemAlloc(gmPrecision);
		if (VpIsInvalid(v1)) continue;
		v2 = gVariables[i];
		VpAsgn(v1, v2, 1);
		VpFree(&v2);
		gVariables[i] = v1;
	}
}

void DoIterations(PARSER *p)
{
	int  nd;
	if (TokenCount(p->r) != 3 || !IsToken("=", p->r, 1)) { ERROR(fprintf(stderr, "Error: syntax error.\n")); return; }
	if (!ToIntFromSz(&nd, TokenPTR(p->r,2))) return;
	if (nd < 100) { ERROR(fprintf(stderr, "Error: negative or too small value for $max_iterations(%d).\n", nd)); return; }
	VpSetMaxIterationCount(nd);
	gmIterations = nd;
}

void DoTitle(PARSER *p)
{
	READER* r = p->r;
	int l;
	UCHAR* psz = TokenPTR(r,2);
	if (TokenCount(r) == 2 && IsToken("=", r,1)) {	strcpy(gszTitle, " ");	return; }
	if (TokenCount(r) != 3 || !IsToken("=", r, 1)) { ERROR(fprintf(stderr, "Error: syntax error.\n")); return; }
	if (((size_t)(l=strlen(psz))) >= (size_t)gmTitle) {
		ERROR(fprintf(stderr, "Error: String too long for $title.\n"));
		return;
	}
	if(l>0) strcpy(gszTitle, psz);
	else    strcpy(gszTitle, " ");
}

void DoSetting(PARSER *p)
{
	int i;
	READER* r = p->r;

	for (i = 0; i < gmSetting; ++i) {
		if (IsToken(gSetting[i].name, r, 0)) {
			((void(*)(PARSER *))gSetting[i].calc)(p); return;
		}
	}
	ERROR(fprintf(stderr, "Error: invalid identifier(%s).\n", TokenPTR(r,0)));
}

void SetVTitle(PARSER *p)
{
	READER* r = p->r;
	UCHAR  chv;
	UCHAR* pv;
	int   ixv,nc;
	int nt = TokenCount(r);

	if (TokenSize(r,0) != 2)                                goto Error;
	if (TokenCount(r)  != 2 && TokenCount(r) != 3) goto Error;

	chv = TokenChar(r,0,1);
	ixv = chv - 'a';
	if (ixv < 0 || ixv>25)  goto Error;
	pv = gszVTitle[ixv];
	nc = strlen(TokenPTR(r,2)) + 2;
	if (nc > 512) {
		ERROR(fprintf(stderr, "Error: too long variable title."));
		return;
	}
	if (pv != NULL) free(pv);
	gszVTitle[ixv] = calloc(sizeof(UCHAR), nc);

	if (TokenCount(r) == 2) {
		strcpy(gszVTitle[ixv], " ");
	}
	else if (TokenCount(r) == 3) {
		strcpy(gszVTitle[ixv],TokenPTR(r,2));
	}
	return;

Error:
	ERROR(printf("Error: syntax error.\n"));
}

void DoRead(UCHAR* inFile)
{
	FILE* f = fopen(inFile, "r");
	if (f == NULL) {
		ERROR(fprintf(stderr, "Error: unable to open the file(%s).\n", inFile));
		return;
	}
	ReadAndExecuteLines(f);
	fclose(f);
}

void DoWrite(PARSER *p,UCHAR* otFile)
{
	int i;
	char chq = gchQuote;
	FILE* f  = fopen(otFile, "w");
	if (f == NULL) {
		ERROR(fprintf(stderr, "Error: unable to open the file(%s).\n", otFile));
		return;
	}
	PrintTitle(p,f,1);
	PrintFormat(p,f,1);
	PrintPrecision(p,f,1);
	PrintRound(p,f,1);
	PrintIterations(p,f,1);
	gchQuote = 'Q';
	for (i = 0; i < 26; ++i) {
		OutputVariableTitle(f, 'a' + i,0);
		PrintVariable(f, 'a' + i,1);
	}
	gchQuote = chq;
	fclose(f);
}

void ParseAndExecuteRepeat(PARSER *p)
{

	int n;
	int iSaved = (p->r)->iStatement;

	if(!ToIntFromSz(&n, TokenPTR(p->r,1))) return;
	if (n <= 0) {
		ERROR(fprintf(stderr, "Error: \'repeat\' must be follwes by a positive integer(%d).\n",n));
		return;
	}
	while (--n >= 0) {
		ExecuteStatement(p, iSaved + 1);
		if (IsError()) return;
	}
}

void ParseAndExecuteWhile(PARSER *p, int nt)
{
	int fOk = 0;
	int iSaved = (p->r)->iStatement;
	VP_HANDLE v1;
	VP_HANDLE v2;
	int f = 0;
	char chv1 = TokenChar(p->r, 1, 0);
	char chv2;
	char op1;
	char op2;
	op1 = TokenChar(p->r, 2, 0);

	if (nt == 4) {
		op2 = 0;
		chv2 = TokenChar(p->r, 3, 0);
	}
	else {
		op2 = TokenChar(p->r, 3, 0);
		chv2 = TokenChar(p->r, 4, 0);
	}

	chv1 = chv1 - 'a';
	chv2 = chv2 - 'a';

	if ((chv1 >= 0 && chv1 <= 25)) {
		if (!EnsureVariable(chv1, gmPrecision)) return;
		v1 = gVariables[chv1];
	}
	if ((chv2 >= 0 && chv2 <= 25)) {
		if (!EnsureVariable(chv2, gmPrecision)) return;
		v2 = gVariables[chv2];
	}

do_while:
	SetStatement(p->r, iSaved);
	if((chv1 < 0 || chv1 > 25)) {
		v1 = gWorkVariables[-CreateNumericWorkVariable(TokenPTR(p->r, 1)) - 1];
	}
	if ((chv2 < 0 || chv2 > 25)) {
		v2 = gWorkVariables[-CreateNumericWorkVariable(TokenPTR(p->r, nt - 1)) - 1];
	}
	f = VpCmp(v1, v2);
	fOk = 1;
	if (f == 0) {
			if (nt  ==  4 )  fOk = 0; /* Condition not satisfied */
			if (op1 == '!')  fOk = 0; /* Condition not satisfied */
	} else
	if (f > 0) {
		if (nt == 4) {
			if (op1 == '>') fOk = 1; /* Condition satisfied */
			else            fOk = 0;
		} else {
			if (op1 == '>' || op1 == '!') fOk = 1;
			else                          fOk = 0;
		}
	} else {
		if (nt == 4) {
			if (op1 == '<') fOk = 1;
			else            fOk = 0;
		} else {
			if (op1 == '<' || op1 == '!') fOk = 1;
			else                          fOk = 0;
		}
	}
	if (fOk) {
		ExecuteStatement(p, iSaved + 1);
		if (IsError()) return;
		goto do_while;
	}
	return;
}

void ParseAndExecuteIf(PARSER* p, int nt)
{
	int fOk = 0;
	int iSaved = (p->r)->iStatement;
	VP_HANDLE v1;
	VP_HANDLE v2;
	int f = 0;
	char chv1 = TokenChar(p->r, 1, 0);
	char chv2;
	char op1;
	char op2;
	op1 = TokenChar(p->r, 2, 0);

	if (nt == 4) {
		op2 = 0;
		chv2 = TokenChar(p->r, 3, 0);
	}
	else {
		op2 = TokenChar(p->r, 3, 0);
		chv2 = TokenChar(p->r, 4, 0);
	}

	chv1 = chv1 - 'a';
	chv2 = chv2 - 'a';

	if ((chv1 >= 0 && chv1 <= 25)) {
		if (!EnsureVariable(chv1, gmPrecision)) return;
		v1 = gVariables[chv1];
	}
	if ((chv2 >= 0 && chv2 <= 25)) {
		if (!EnsureVariable(chv2, gmPrecision)) return;
		v2 = gVariables[chv2];
	}

	SetStatement(p->r, iSaved);
	if ((chv1 < 0 || chv1 > 25)) {
		v1 = gWorkVariables[-CreateNumericWorkVariable(TokenPTR(p->r, 1)) - 1];
	}
	if ((chv2 < 0 || chv2 > 25)) {
		v2 = gWorkVariables[-CreateNumericWorkVariable(TokenPTR(p->r, nt - 1)) - 1];
	}
	f = VpCmp(v1, v2);
	fOk = 1;
	if (f == 0) {
		if (nt == 4)  fOk = 0; /* Condition not satisfied */
		if (op1 == '!')  fOk = 0; /* Condition not satisfied */
	}
	else
		if (f > 0) {
			if (nt == 4) {
				if (op1 == '>') fOk = 1; /* Condition satisfied */
				else            fOk = 0;
			}
			else {
				if (op1 == '>' || op1 == '!') fOk = 1;
				else                          fOk = 0;
			}
		}
		else {
			if (nt == 4) {
				if (op1 == '<') fOk = 1;
				else            fOk = 0;
			}
			else {
				if (op1 == '<' || op1 == '!') fOk = 1;
				else                          fOk = 0;
			}
		}
	if (fOk) {
		ExecuteStatement(p, iSaved + 1);
		if (IsError()) return;
	}
	return;
}

void ParseAndExecuteLoad(PARSER* p, int nt)
{
	int lines = 0;
	int     i,j;
	char    chv;
	UCHAR   ch;
	FILE*   fin;
	READER* r;

	int iSaved = (p->r)->iStatement;

	fin = fopen(TokenPTR(p->r,1), "r");
	if (fin == NULL) {
		ERROR(fprintf(stderr, "Error: faile to open file(%s) for load.\n", TokenPTR(p->r, 1)) );
		return;
	}

	r = OpenReader(fin, p->mSize * 2);
	if (r == NULL) {
		fclose(fin);
		return;
	}

	do {
		int ixs = 0;
		int ixt = 0;
		SetStatement(p->r, iSaved);
		ch = ReadLine(r);
		if (IsEOF(ch) && r->cStatements <= 0) goto Close;
		lines++;
		ixs = 0;
		ixt = 0;
		for (i = 2; i < nt; ++i) {
			if (TokenSize(p->r, i) != 1) {
				ERROR(fprintf(stderr, "Error: invalid argument for load(%s is not a variable).\n", TokenPTR(p->r, i)));
				goto Close;
			}
			chv = TokenChar(p->r, i, 0);
			j = chv - 'a';
			if (j < 0 || j>25) {
				ERROR(fprintf(stderr, "Error: invalid argument for load(%s is not a variable).\n", TokenPTR(p->r, i)));
				goto Close;
			}
			if (!EnsureVariable(j, gmPrecision)) goto Close;
			do {
				SetStatement(r, ixs);
				do {
					if (IsNumeric(r, ixt)) {
						VP_HANDLE v = gVariables[j];
						VpLoad(v, TokenPTR(r, ixt));
						if (ixt > 0 && IsToken("-",r,ixt-1)) {
							VpNegate(v);
						}
						if (i + 1 >= nt) goto ExecLoad;
						if (++ixt >= TokenCount(r)) {
							ixt = 0;
							if (++ixs >= r->cStatements) {
								ERROR(fprintf(stderr, "Error:insufficient numeric data in a line(line:%d)\n", lines));
								goto Close;
							}
						}
						goto NextToken;
					}
				} while (++ixt < TokenCount(r));
				ixt = 0;
			} while (++ixs < r->cStatements);
			ERROR(fprintf(stderr, "Error:insufficient numeric data in a line(line:%d)\n", lines));
			goto Close;
		ExecLoad:
			ExecuteStatement(p, iSaved + 1);
			if (IsError()) goto Close;
			if (IsQuit() ) goto Close;
			if (gfBreak)   goto Close;
			break;
		NextToken:
			continue;
		}
	} while (!IsEOF(ch));

Close:
	CloseReader(r);
}
