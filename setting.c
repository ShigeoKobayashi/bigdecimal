
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

void PrintFormat(FILE *f)
{
	fprintf(f,"$format = '%d%c%c%c%c'\n", gnCount, gchLeader, gchFormatChar, gchSeparator, gchQuote);
}

void PrintPrecision(FILE *f)
{
	fprintf(f,"$precision = '%d'\n", gmPrecision);
}

void PrintRound(FILE *f)
{
	int i;
	for (i = 0; i < gmRMode; ++i) {
		if (gRMode[i].value == gRoundMode) {
			fprintf(f, "$round = '%s'\n", gRMode[i].name);
			return;
		}
	}
	ERROR(fprintf(stderr,"SYSTE_ERROR:undefine current round mode(%d),reset!\n",gRoundMode));
	gRoundMode = VpGetRoundMode();
}

void PrintIterations(FILE *f)
{
	fprintf(f,"$max_iterations = '%d'\n", gmIterations);
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
void PrintTitle(FILE* f)
{
	UCHAR ch;
	if ( gszTitle[0] =='\0' ) fprintf(f, "$title = ' '\n");
	else {
		ch = GetQuote(gszTitle); fprintf(f, "$title = %c%s%c\n", ch, gszTitle, ch);
	}
}

void OutputVariableTitle(FILE* f, UCHAR chv)
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
		fprintf(f, "%c%s%c\n",ch, gszVTitle[ixv],ch);
	}
	else {
		fprintf(f, " $%c = ' '\n", chv);
	}

}

void PrintVariableTitle(FILE* f,int iStatement)
{
	UCHAR chv = TokenChar(iStatement,1, 1);
	OutputVariableTitle(f, chv);
}

void PrintVariable(FILE* f, UCHAR chv) 
{
	VP_HANDLE v;
	int ixv = chv - 'a';

	if (ixv < 0 || ixv >= 25) ixv = 26;
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
	fprintf(f,"\n");
}

void DoPrint(int iStatement)
{
	int i;
	FILE* f = stdout;

	for (i = 0; i < gmSetting; ++i) {
		if (strcmp(TokenPTR(iStatement,1),gSetting[i].name)==0) {
			((void(*)(FILE*,int))gSetting[i].print)(f,iStatement); return;
		};
	}
	if (TokenSize(iStatement,1) == 1)  { PrintVariable(f, TokenChar(iStatement, 1, 0)); return;}
	ERROR(fprintf(stderr, "Error: undefined variable(%s).\n", TokenPTR(iStatement,1)));
	return;
}

void DoRound(int iStatement)
{
	int i;

	for (i = 0; i < gmRMode; ++i) {
		if (IsToken(gRMode[i].name, iStatement, 2)) { gRoundMode = gRMode[i].value; VpSetRoundMode(gRoundMode); return; }
	}
	ERROR(fprintf(stderr, "Error: undefined round mode(%s)", TokenPTR(iStatement,2)));
}

void DoFormat(int iStatement)
{
	int i,id,nd;
	UCHAR ch;
	int nch;
	nch = TokenSize(iStatement,2);
	for (i = 0; i < nch; ++i) {
		ch = TokenChar(iStatement, 2,  i);
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
				ch = TokenChar(iStatement, 2, i+1);
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

void DoPrecision(int iStatement)
{
	int  nd;
	if (!ToIntFromSz(&nd, TokenPTR(iStatement,2))) return;
	if (nd < 20) {
		ERROR(fprintf(stderr, "Error: negative or too small value for $precision(%d).\n", nd)); return;
	}
	gmPrecision = nd;
}

void DoIterations(int iStatement)
{
	int  nd;
	if (TokenCount(iStatement) != 3 || !IsToken("=", iStatement, 1)) { ERROR(fprintf(stderr, "Error: syntax error.\n")); return; }
	if (!ToIntFromSz(&nd, TokenPTR(iStatement,2))) return;
	if (nd < 100) { ERROR(fprintf(stderr, "Error: negative or too small value for $max_iterations(%d).\n", nd)); return; }
	VpSetMaxIterationCount(nd);
	gmIterations = nd;
}

void DoTitle(int iStatement)
{
	int l;
	UCHAR* psz = TokenPTR(iStatement,2);
	if (TokenCount(iStatement) == 2 && IsToken("=", iStatement,1)) {	strcpy(gszTitle, " ");	return; }
	if (TokenCount(iStatement) != 3 || !IsToken("=", iStatement, 1)) { ERROR(fprintf(stderr, "Error: syntax error.\n")); return; }
	if (((size_t)l=strlen(psz)) >= (size_t)gmTitle) {
		ERROR(fprintf(stderr, "Error: String too long for $title.\n"));
		return;
	}
	if(l>0) strcpy(gszTitle, psz);
	else    strcpy(gszTitle, " ");
}

void DoSetting(int iStatement)
{
	int i;

	for (i = 0; i < gmSetting; ++i) {
		if (IsToken(gSetting[i].name, iStatement, 0)) {
			((void(*)(int))gSetting[i].calc)(iStatement); return;
		}
	}
	ERROR(fprintf(stderr, "Error: invalid identifier(%s).\n", TokenPTR(iStatement,0)));
}

void SetVTitle(int iStatement)
{
	UCHAR  chv;
	UCHAR* pv;
	int   ixv,nc;
	int nt = TokenCount(iStatement);

	if (TokenSize(iStatement,0) != 2)                                goto Error;
	if (TokenCount(iStatement)  != 2 && TokenCount(iStatement) != 3) goto Error;

	chv = TokenChar(iStatement,0,1);
	ixv = chv - 'a';
	if (ixv < 0 || ixv>25)  goto Error;
	pv = gszVTitle[ixv];
	nc = strlen(TokenPTR(iStatement,2)) + 2;
	if (nc > 512) {
		ERROR(fprintf(stderr, "Error: too long variable title."));
		return;
	}
	if (pv != NULL) free(pv);
	gszVTitle[ixv] = calloc(sizeof(UCHAR), nc);

	if (TokenCount(iStatement) == 2) {
		strcpy(gszVTitle[ixv], " ");
	}
	else if (TokenCount(iStatement) == 3) {
		strcpy(gszVTitle[ixv],TokenPTR(iStatement,2));
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
	ReadLines(f);
	fclose(f);
}

void DoWrite(UCHAR* otFile)
{
	int i;
	char chq = gchQuote;
	FILE* f  = fopen(otFile, "w");
	if (f == NULL) {
		ERROR(fprintf(stderr, "Error: unable to open the file(%s).\n", otFile));
		return;
	}
	PrintTitle(f);
	PrintFormat(f);
	PrintPrecision(f);
	PrintRound(f);
	PrintIterations(f);
	gchQuote = 'Q';
	for (i = 0; i < 26; ++i) {
		OutputVariableTitle(f, 'a' + i);
		PrintVariable(f, 'a' + i);
	}
	gchQuote = chq;
	fclose(f);
}

void DoRepeat(UCHAR* repeat)
{
	UCHAR ch;
	int   v;

	if (gnRepeat == 0 && !gfWhile) {
		if (isspace(*repeat)) repeat++;
		if (*repeat == '+')   repeat++;
		while (ch = *repeat++) {
			v = ch - '0';
			if (v < 0 || v>9) goto Error;
			gnRepeat = gnRepeat * 10 + v;
		}
	}
	else {
		ERROR(fprintf(stderr, "Error: \'repeat\' or \'while\' can not be nested(execution interrupted.).\n"));
		gnRepeat = 0; gfWhile = 0;
	}
	return;

Error:
	ERROR(fprintf(stderr, "Error: invalid character(%c), integer number expected in \'repeat\'.\n", ch));
	gnRepeat = 0; gfWhile = 0;
	return;
}

void DoWhile(int iStatement, int nt)
{
	VP_HANDLE v1;
	VP_HANDLE v2;
	int f = 0;
	char chv1 = TokenChar(iStatement, 1, 0);
	char chv2;
	char op1;
	char op2;
	op1 = TokenChar(iStatement, 2, 0);

	if (gnRepeat > 0 || gfWhile) {
		ERROR(fprintf(stderr, "Error: \'repeat\' or \'while\' can not be nested(execution interrupted.).\n"));
		gnRepeat = 0; gfWhile = -1;
		return;
	}

	if (nt == 4) {
		op2 = 0;
		chv2 = TokenChar(iStatement, 3, 0);
	}
	else {
		op2 = TokenChar(iStatement, 3, 0);
		chv2 = TokenChar(iStatement, 4, 0);
	}
	chv1 = chv1 - 'a';
	chv2 = chv2 - 'a';

	if ((chv1 >= 0 && chv1 <= 25)) {
		if (!EnsureVariable(chv1, gmPrecision)) return;
		v1 = gVariables[chv1];
	}
	else {
		v1 = gWorkVariables[-CreateNumericWorkVariable(TokenPTR(iStatement, 1)) - 1];
	}

	if ((chv2 >= 0 && chv2 <= 25)) {
		if (!EnsureVariable(chv2, gmPrecision)) return;
		v2 = gVariables[chv2];
	}
	else {
		v2 = gWorkVariables[-CreateNumericWorkVariable(TokenPTR(iStatement, nt - 1)) - 1];
	}

	f = VpCmp(v1, v2);

	if (f == 0) {
		if (nt == 4) { gfWhile = -1; return; }	/* only > or < */
		if (op1 == '!') { gfWhile = -1; return; }
		gfWhile = 1;
		return;
	}
	if (f > 0) {
		if (nt == 4) {
			if (op1 == '>') gfWhile = 1;
			else            gfWhile = -1;
			return;
		}
		if (op1 == '>' || op1 == '!') gfWhile = 1;
		else                         gfWhile = -1;
		return;
	}
	if (f < 0) {
		if (nt == 4) {
			if (op1 == '<') gfWhile = 1;
			else            gfWhile = -1;
			return;
		}
		if (op1 == '<' || op1 == '!') gfWhile = 1;
		else                          gfWhile = -1;
		return;
	}
	return;
}
