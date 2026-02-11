
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

extern int gfLoop;
extern int gfInFile;

int  gmPrecision   = 100;
int  gmIterations  = 10000;

char gszTitle[512];
int  gmTitle = sizeof(gszTitle) / sizeof(gszTitle[0]);

char* gszVTitle[26];
int   gmVTitle = sizeof(gszVTitle) / sizeof(gszVTitle[0]);

/* format */
int  gnCount       = 10;  /* digits separation count */
char gchFormatChar = 'E';
char gchSeparator  = ' ';
char gchQuote      = 'q';
char gchLeader     = '*';
int  gRoundMode    = VP_ROUND_HALF_UP;

typedef struct _ROUNDMODE {
	const char* name;
	int   value;
} ROUNDMODE;
static ROUNDMODE gRMode[] = {
	{(const char*)"up",VP_ROUND_UP},
	{(const char*)"down",VP_ROUND_DOWN},
	{(const char*)"half_up",VP_ROUND_HALF_UP},     /*  default */
	{(const char*)"half_down",VP_ROUND_HALF_DOWN},
	{(const char*)"ceil",VP_ROUND_CEIL},
	{(const char*)"floor",VP_ROUND_FLOOR},
	{(const char*)"half_even",VP_ROUND_HALF_EVEN}
};
static int gmRMode = sizeof(gRMode) / sizeof(gRMode[0]);

void PrintFormat(PARSER *p,FILE *f,int newline)
{
	if(newline) fprintf(f, "$format         = '%d%c%c%c%c'\n", gnCount, gchLeader, gchFormatChar, gchSeparator, gchQuote);
	else        fprintf(f, "$format         = '%d%c%c%c%c'; ", gnCount, gchLeader, gchFormatChar, gchSeparator, gchQuote);
}

void PrintPrecision(PARSER *p,FILE *f, int newline)
{
	if(newline) fprintf(f, "$precision      = '%d'\n", gmPrecision);
	else        fprintf(f, "$precision      = '%d'; ", gmPrecision);
}

void PrintRound(PARSER *p,FILE *f, int newline)
{
	int i;
	for (i = 0; i < gmRMode; ++i) {
		if (gRMode[i].value == gRoundMode) {
			if(newline) fprintf(f, "$round          = '%s'\n", gRMode[i].name);
			else        fprintf(f, "$round          = '%s'; ", gRMode[i].name);
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

static char GetQuote(char* psz)
{
	char ch;
	while (ch = *psz++) {
		if (ch == '\'') return '\"';
		if (ch == '\"') return '\'';
	}
	return'\"';
}
void PrintTitle(PARSER *p,FILE* f,int newline)
{
	char ch;
	
	if (gszTitle[0] == '\0') {
		if(newline) fprintf(f, "$title          = ' '\n");
		else        fprintf(f, "$title          = ' '; ");
	}
	else {
		ch = GetQuote(gszTitle); 
		if (newline) fprintf(f, "$title      = %c%s%c\n", ch, gszTitle, ch);
		else         fprintf(f, "$title      = %c%s%c; ", ch, gszTitle, ch);
	}
}

void OutputVariableTitle(FILE* f, char chv,int newline)
{
	int ixv = chv - 'a';
	char ch;

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
	char chv = TokenChar(p->r,0, 0);
	if(chv=='?') chv = TokenChar(p->r,1, 1);
	else         chv = TokenChar(p->r,0, 1);
	OutputVariableTitle(f, chv,newline);
}

void PrintVariable(FILE* f, char chv,int newline) 
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

void PrintHelp(FILE *f)
{
	fprintf(f,"#\n# Definitions:\n");
	fprintf(f,"#   V: A variable             => a|b|...|z (One of a or b or ... or z)\n");
	fprintf(f,"#   S: An environment setting => $format|$max_iterations|$precision|$round|$title\n");
	fprintf(f,"#                               |$a|$b|...|$y|$z\n");
	fprintf(f,"#   N: An integer number      => 1|2|10|20...\n");
	fprintf(f,"#   F: A function             => atan|sin|cos|exp|ln|pi|sqrt|iterations|abs|power\n");
	fprintf(f,"#                               |int|frac|digits|exponent|trim|round\n");
	fprintf(f,"#   O: An operator            => +|-|*|/|=  (Try and confirm the results: >a = sin(b=pi()/2); ?ab)\n");
	fprintf(f,"#   L: A logical operator     => <|>|<=|>=|==|!=\n");
	fprintf(f,"#   E: An expression          => V|N|F(E) [O V|N|F(E)]\n");
	fprintf(f,"#   EOL: An end of input line\n");
	fprintf(f,"#\n# Commands:\n");
	fprintf(f,"#   quit:    quits vpc.exe and exit.\n");
	fprintf(f,"#   exit:    exits vpc.exe(same as quit).\n");
	fprintf(f,"#   save:    writes everything to ./vpc.ini (same as => write './vpc.ini').\n");
	fprintf(f,"#   restore: reads everything from ./vpc.ini (same as => read './vpc.ini').\n");
	fprintf(f,"#   !c:      execute any command(=c) by c-standard API system(\"c\") function.\n");
	fprintf(f,"#   =:       Asignment=> V|S = E;...;\n");
	fprintf(f,"#   ?:       Printing => ??     (print this help)\n");
	fprintf(f,"#                       |?V...  (print all value of V specified)\n");
	fprintf(f,"#                       |?S     (print value of S)\n");
	fprintf(f,"#                       |?*     (print all V and S)\n");
	fprintf(f,"#                       |?$     (print all S)\n");
	fprintf(f,"#   if:               => if V L V;ex1;ex2;...exn;EOL (execute ex1 to exn if V L V is TRUE)\n");
	fprintf(f,"#   repeat:           => repeat N;ex1;ex2;...exn;EOL (repeat ex1 to exn N times)\n");
	fprintf(f,"#   while:            => while V L V;ex1;ex2;...exn;EOL (execute ex1 to exn while V L V is TRUE)\n");
	fprintf(f,"#   load:             => load  'text file' V1 V2 ...; (read values from 'text file' and assign them to V)\n");
	fprintf(f,"#   write:            => write 'text file'; (write everything to 'text file' in command form)\n");
	fprintf(f,"#   read:             => read  'text file'; (read and execute every commands in 'text file')\n");
	fprintf(f,"#\n# Environment settings:\n");
	fprintf(f,"#   $title|$a|$b... = any comment string\n");
	fprintf(f,"#   $round = up|down|half_up|half_down ceilfloor|half_even:\n");
	fprintf(f,"#   $format= '10*E q'; (default)\n");
	fprintf(f,"#              '10' ... output number is separated in each 10 digits\n");
	fprintf(f,"#              '*'  ... '*'|'+'|'-' can be specified to control the first character(see document in detail)\n");
	fprintf(f,"#              'E'  ... output number is represented by E-form(F for F-form)\n");
	fprintf(f,"#              ' '  ... output number is separated by ' ' (only ' ' or ',' is allowed)\n");
	fprintf(f,"#              'q'  ... output number is not quoted by ','Q' otherwise\n");
	fprintf(f,"#   $max_iterations = N: maximum iteration count\n");
	fprintf(f,"#   $precision = N: maximum digits count each variable can hold\n");
	fprintf(f,"#\n");
}


void DoPrint(PARSER *p)
{
	int i;
	FILE *f = stdout;
	char *psz;
	char  ch;

	if (TokenCount(p->r) == 2 && IsToken("?", p->r, 1)) {
		PrintHelp(stdout);
		return;
	}


	if (TokenCount(p->r) == 2 && IsToken("*", p->r, 1)) {
		WriteContents(p,stdout);
		return;
	}

	if (TokenCount(p->r) == 2 && IsToken("$", p->r, 1)) {
		WriteSetting(p, stdout);
		return;
	}

	for (i = 0; i < gmSetting; ++i) {
		if (strcmp((const char*)TokenPTR(p->r,1),(const char*)gSetting[i].name)==0) {
			((void(*)(PARSER *,FILE *,int))gSetting[i].print)(p,f, 1);
			return;
		};
	}

	psz = TokenPTR(p->r,1);
	while(ch=*psz++) {
		if(ch>'z' || ch<'a') {
			ERROR(fprintf(stderr, "Error: undefined variable(%c).\n", ch));
			return;
		}
		PrintVariable(f, ch, 1); 
	}
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
	char ch;
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
	char* psz = TokenPTR(r,2);
	if (TokenCount(r) == 2 && IsToken("=", r,1)) {	strcpy(gszTitle, " ");	return; }
	if (TokenCount(r) != 3 || !IsToken("=", r, 1)) { ERROR(fprintf(stderr, "Error: syntax error.\n")); return; }
	if (((size_t)(l=strlen((const char*)psz))) >= (size_t)gmTitle) {
		ERROR(fprintf(stderr, "Error: String too long for $title.\n"));
		return;
	}
	if(l>0) strcpy(gszTitle, (const char*)psz);
	else    strcpy(gszTitle, (const char*)" ");
}

void DoSetting(PARSER *p)
{
	int i;
	READER* r = p->r;

	for (i = 0; i < gmSetting; ++i) {
		if (IsToken(gSetting[i].name, r, 0)) {
			((void(*)(PARSER *))gSetting[i].calc)(p); 
			if((!gfLoop) && (!gfInFile)) {
				((void(*)(PARSER *,FILE *,int))gSetting[i].print)(p,stdout, 1);
			}
			return;
		};
	}
	ERROR(fprintf(stderr, "Error: invalid identifier(%s).\n", TokenPTR(r,0)));
}

void SetVTitle(PARSER *p)
{
	READER* r = p->r;
	char  chv;
	char* pv;
	int   ixv,nc;
	int nt = TokenCount(r);

	if (TokenSize(r,0) != 2)                                goto Error;
	if (TokenCount(r)  != 2 && TokenCount(r) != 3) goto Error;

	chv = TokenChar(r,0,1);
	ixv = chv - 'a';
	if (ixv < 0 || ixv>25)  goto Error;
	pv = gszVTitle[ixv];
	nc = strlen((const char*)TokenPTR(r,2)) + 2;
	if (nc > 512) {
		ERROR(fprintf(stderr, "Error: too long variable title."));
		return;
	}
	if (pv != NULL) free(pv);
	gszVTitle[ixv] = (char*)calloc(sizeof(char), nc);

	if (TokenCount(r) == 2) {
		strcpy(gszVTitle[ixv], " ");
	}
	else if (TokenCount(r) == 3) {
		strcpy(gszVTitle[ixv],(const char*)TokenPTR(r,2));
	}
	return;

Error:
	ERROR(printf("Error: syntax error.\n"));
}

void DoRead(char* inFile)
{
	FILE* f = fopen((const char*)inFile, "r");
	if (f == NULL) {
		ERROR(fprintf(stderr, "Error: unable to open the file(%s).\n", inFile));
		return;
	}
	ReadAndExecuteLines(f);
	fclose(f);
}

void WriteSetting(PARSER* p, FILE* f)
{
	PrintTitle(p, f, 1);
	PrintFormat(p, f, 1);
	PrintPrecision(p, f, 1);
	PrintRound(p, f, 1);
	PrintIterations(p, f, 1);
}

void WriteContents(PARSER *p,FILE* f)
{
	int i;
	char chq = gchQuote;
	WriteSetting(p, f);
	gchQuote = 'Q';
	for (i = 0; i < 26; ++i) {
		OutputVariableTitle(f, 'a' + i, 0);
		PrintVariable(f, 'a' + i, 1);
	}
	gchQuote = chq;
}

void DoWrite(PARSER *p,char* otFile)
{
	FILE* f  = fopen((const char*)otFile, "w");
	if (f == NULL) {
		ERROR(fprintf(stderr, "Error: unable to open the file(%s).\n", otFile));
		return;
	}
	WriteContents(p,f);
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
		if (gfBreak||gfQuit) return;
	}
}

void ParseAndExecuteWhile(PARSER *p, int nt)
{
	int fOk = 0;
	int iSaved = (p->r)->iStatement;
	VP_HANDLE v1;
	VP_HANDLE v2;
	int f = 0;
	int iv1=1,iv2=3;
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
		iv2=4;
	}

	chv1 = chv1 - 'a';
	chv2 = chv2 - 'a';

	if ((chv1 >= 0 && chv1 <= 25)) {
		if (!EnsureVariable(chv1, gmPrecision)) return;
		v1 = gVariables[chv1];
	} else if(TokenWhat(p->r,iv1)!=VPC_NUMERIC) {
		ERROR((fprintf(stderr,"ERROR:Not a variable nor a numeric(%s).\n",TokenPTR(p->r,iv1))));
		return;
	}
	if ((chv2 >= 0 && chv2 <= 25)) {
		if (!EnsureVariable(chv2, gmPrecision)) return;
		v2 = gVariables[chv2];
	} else if(TokenWhat(p->r,iv2)!=VPC_NUMERIC) {
		ERROR((fprintf(stderr,"ERROR:Not a variable nor a numeric(%s).\n",TokenPTR(p->r,iv2))));
		return;
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
		if (gfBreak||gfQuit) return;
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
	int iv1=1,iv2=3;
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
		iv2=4;
	}

	chv1 = chv1 - 'a';
	chv2 = chv2 - 'a';

	if ((chv1 >= 0 && chv1 <= 25)) {
		if (!EnsureVariable(chv1, gmPrecision)) return;
		v1 = gVariables[chv1];
	} else if(TokenWhat(p->r,iv1)!=VPC_NUMERIC) {
		ERROR((fprintf(stderr,"ERROR:Not a variable nor a numeric(%s).\n",TokenPTR(p->r,iv1))));
		return;
	}
	if ((chv2 >= 0 && chv2 <= 25)) {
		if (!EnsureVariable(chv2, gmPrecision)) return;
		v2 = gVariables[chv2];
	} else if(TokenWhat(p->r,iv2)!=VPC_NUMERIC) {
		ERROR((fprintf(stderr,"ERROR:Not a variable nor a numeric(%s).\n",TokenPTR(p->r,iv2))));
		return;
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
		if (gfBreak||gfQuit) return;
	}
	return;
}

void ParseAndExecuteLoad(PARSER* p, int nt)
{
	int lines = 0;
	int     i,j;
	char    chv;
	char    ch;
	FILE*   fin;
	READER* r;

	int iSaved = (p->r)->iStatement;

	fin = fopen((const char*)TokenPTR(p->r,1), "r");
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
						VpLoad(v, (const char*)TokenPTR(r, ixt));
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
