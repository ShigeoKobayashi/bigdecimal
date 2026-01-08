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

extern 	int	gfLoop;
extern  int gfInFile;

SETTING gSetting[] = {
	{"$title",VPC_SETTING,PrintTitle,DoTitle},
	{"$format",VPC_SETTING,PrintFormat,DoFormat},
	{"$precision",VPC_SETTING,PrintPrecision,DoPrecision},
	{"$max_iterations",VPC_SETTING,PrintIterations,DoIterations},
	{"$round",VPC_SETTING,PrintRound,DoRound},
	{"$a",VPC_SETTING,PrintVariableTitle,SetVTitle},
	{"$b",VPC_SETTING,PrintVariableTitle,SetVTitle},
	{"$c",VPC_SETTING,PrintVariableTitle,SetVTitle},
	{"$d",VPC_SETTING,PrintVariableTitle,SetVTitle},
	{"$e",VPC_SETTING,PrintVariableTitle,SetVTitle},
	{"$f",VPC_SETTING,PrintVariableTitle,SetVTitle},
	{"$g",VPC_SETTING,PrintVariableTitle,SetVTitle},
	{"$h",VPC_SETTING,PrintVariableTitle,SetVTitle},
	{"$i",VPC_SETTING,PrintVariableTitle,SetVTitle},
	{"$j",VPC_SETTING,PrintVariableTitle,SetVTitle},
	{"$k",VPC_SETTING,PrintVariableTitle,SetVTitle},
	{"$l",VPC_SETTING,PrintVariableTitle,SetVTitle},
	{"$m",VPC_SETTING,PrintVariableTitle,SetVTitle},
	{"$n",VPC_SETTING,PrintVariableTitle,SetVTitle},
	{"$o",VPC_SETTING,PrintVariableTitle,SetVTitle},
	{"$p",VPC_SETTING,PrintVariableTitle,SetVTitle},
	{"$q",VPC_SETTING,PrintVariableTitle,SetVTitle},
	{"$r",VPC_SETTING,PrintVariableTitle,SetVTitle},
	{"$s",VPC_SETTING,PrintVariableTitle,SetVTitle},
	{"$t",VPC_SETTING,PrintVariableTitle,SetVTitle},
	{"$u",VPC_SETTING,PrintVariableTitle,SetVTitle},
	{"$v",VPC_SETTING,PrintVariableTitle,SetVTitle},
	{"$w",VPC_SETTING,PrintVariableTitle,SetVTitle},
	{"$x",VPC_SETTING,PrintVariableTitle,SetVTitle},
	{"$y",VPC_SETTING,PrintVariableTitle,SetVTitle},
	{"$z",VPC_SETTING,PrintVariableTitle,SetVTitle}
};
int gmSetting = sizeof(gSetting) / sizeof(gSetting[0]);

void CloseParser(PARSER* p)
{
	gfInFile = 0;
	if (p == NULL) return;
	if (p->r) CloseReader(p->r);
	if (p->TokenStack != NULL) free(p->TokenStack);
	if (p->Polish != NULL) free(p->Polish);
	if (p->TotalPolish != NULL) free(p->TotalPolish);
	if (p->PolishDivider != NULL) free(p->PolishDivider);
	free(p);
}

PARSER *OpenParser(FILE *f,int mToken)
{
	PARSER* p = (PARSER*)calloc(1, sizeof(PARSER));
	if(f==stdin) gfInFile = 0;
	else         gfInFile = 1;
	if (p == NULL) {
		ERROR(fprintf(stderr, "SYSTEM_ERROR: failed to allocate PARSER structur.\n"));
		return NULL;
	}
	p->r = OpenReader(f, mToken * 2);
	if (p->r == NULL) goto Error;

	p->TokenStack = (int *)calloc(mToken, sizeof(int));
	if (p->TokenStack == NULL) {
		ERROR(fprintf(stderr, "SYSTEM_ERROR: failed to allocate token stack memory.\n"));
		goto Error;
	}

	p->Polish = (int*)calloc(mToken, sizeof(int));
	if (p->Polish == NULL) {
		ERROR(fprintf(stderr, "SYSTEM_ERROR: failed to allocate temporary reverse polish memory.\n"));
		goto Error;
	}

	p->TotalPolish = (int*)calloc(mToken, sizeof(int));
	if (p->TotalPolish == NULL) {
		ERROR(fprintf(stderr, "SYSTEM_ERROR: failed to allocate total reverse polish memory.\n"));
		goto Error;
	}

	p->PolishDivider = (P_STATEMENT*)calloc(mToken, sizeof(P_STATEMENT));
	if (p->PolishDivider == NULL) {
		ERROR(fprintf(stderr, "SYSTEM_ERROR: failed to allocate memory the rerse polish for each statement.\n"));
		goto Error;
	}
	p->mSize = mToken;
	return p;

Error:
	CloseParser(p);
	return NULL;
}


static void PrintToken(FILE* f,READER *r, int it)
{
	fprintf(f, "%s",TokenPTR(r,it));
}


static int IsVariable(READER *r,int it)
{
	int  ich;
	char ch;

	SetTokenWhat (r,it,0);
	SetTokenWhat2(r,it, 0);

	if (TokenSize(r,it) != 1) return 0;
	ch = TokenChar(r,it, 0);
	if (ch == 'R'           ) { SetTokenWhat(r,it,VPC_VARIABLE);  SetTokenWhat2(r,it,26);  goto More; }
	ich = ch - 'a';
	if (ich >= 0 && ich < 26) { SetTokenWhat(r,it,VPC_VARIABLE);  SetTokenWhat2(r,it,ich); goto More; }
	return 0;

More:
	if (it+1>=TokenCount(r)) return 1;
	if (IsToken("(", r, it+1)) {
		ERROR(fprintf(stderr, "Error: syntax invalid(variable is not a function).\n"));
		return 0;
	}
	return 1;
}

static int IsOperator(READER *r,int it)
{
	int ich;
	char ch;

	SetTokenWhat(r,it,0);
	SetTokenPriority(r,it,0);

	if (TokenSize(r,it) != 1) return 0;
	ch = TokenChar(r, it, 0);

	if (ch == '(') { SetTokenWhat(r, it,VPC_BRA);       SetTokenPriority(r, it, 0); return 1; }
	if (ch == ')') { SetTokenWhat(r, it,VPC_KET);       SetTokenPriority(r, it, 1); return 1; }
	if (ch == '?') { SetTokenWhat(r, it,VPC_PRINT);     SetTokenPriority(r, it, 0); return 1; }
	if (ch == '=') { SetTokenWhat(r, it,VPC_BOPERATOR); SetTokenPriority(r, it,10); return 1; }
	if (ch == '*') { SetTokenWhat(r, it,VPC_BOPERATOR); SetTokenPriority(r, it,40); return 1; }
	if (ch == '/') { SetTokenWhat(r, it,VPC_BOPERATOR); SetTokenPriority(r, it,40); return 1; }
	if (ch == ',') { SetTokenWhat(r, it,VPC_COMMA);     SetTokenPriority(r, it, 0); return 1; }
	if (ch == '!') { SetTokenWhat(r, it,VPC_EXEC);      SetTokenPriority(r, it, 0); return 1; }
	
	if (ch == '+' || ch == '-') {
		SetTokenWhat(r, it, VPC_BOPERATOR); SetTokenPriority(r, it,20);
		if (it <= 0) {
			SetTokenWhat(r, it,VPC_UOPERATOR); SetTokenPriority(r, it,200);
			return 1;
		}
		ich = TokenWhat(r, it - 1);
		if (ich == VPC_BRA || ich == VPC_COMMA || ich == VPC_BOPERATOR || ich == VPC_UOPERATOR) {
			SetTokenWhat(r, it,VPC_UOPERATOR); SetTokenPriority(r, it,200);
			return 1;
		}
		return 1;
	}
	return 0;
}

static int IsSetting(READER *r, int it)
{
	int i;

	SetTokenWhat(r, it, 0);
	SetTokenWhat2(r, it, 0);

	for (i = 0; i < gmSetting; ++i) {
		if (strcmp(TokenPTR(r, it),gSetting[i].name)==0) { SetTokenWhat(r, it, VPC_SETTING); SetTokenWhat2(r, it, 0); return 1; }
	}
	return 0;
}

static int IsFunction(READER *r,int it)
{
	int i;
	int cb = 0;
	int cc = 0;
	int na = 0;
	int nt = TokenCount(r);
	SetTokenWhat(r, it, 0);
	SetTokenWhat2(r, it, 0);

	for (i = 0; i < gmFunctions; ++i) {
		if (IsToken(FunctionName(i), r, it)) {
			SetTokenWhat(r,it, VPC_FUNC); SetTokenWhat2(r,it, i);   na = FunctionArguments(i); goto ArgNoCheck;
		}
	}
	return 0;

ArgNoCheck: /* check on argument number */

	if (it >= r->Statements[r->iStatement].end || !IsToken("(", r, it + 1)) goto Error;
	cb = 0;
	for (i = it+1; i < nt; ++i) {
		if (IsToken("(", r,i)) {
			++cb;
			if ( i+1 >= nt )               goto Error;
			if (TokenSize(r,i + 1) > 1)      continue;
			if (IsToken("+", r, i - 1))       continue;
			if (IsToken("-", r, i - 1))       continue;
			if (IsToken(",", r, i + 1))       goto Error;
			if (IsToken("*", r, i + 1))       goto Error;
			if (IsToken("/", r, i + 1))       goto Error;
			continue;
		}
		if (IsToken(")", r,i)) {
			if (--cb <= 0)                 break;
			if (TokenSize(r,i - 1) > 1)      continue;
			if (IsToken(",", r,i - 1))       goto Error;
			if (IsToken("+", r,i - 1))       goto Error;
			if (IsToken("-", r,i - 1))       goto Error;
			if (IsToken(",", r,i - 1))       goto Error;
			if (IsToken("*", r,i - 1))       goto Error;
			if (IsToken("/", r,i - 1))       goto Error;
			continue;
		}

		if (cb == 1) {
			if (IsToken(",", r,i)) { ++cc; continue; }
		}
	}

	if (na < 0) na = -na;
	if (na <= 0 && cc == 0) return 1;
	if ( (na -1) != cc    ) goto Error;
	return 1;

Error:
	ERROR(fprintf(stderr, "Error:'"));
	PrintToken(stderr, r, it);
	ERROR(fprintf(stderr, "' syntax invalid(illegal arguments or illegal number of arguments).\n"));
	return 0;
}

static int ParseToken(PARSER *p,int it)
{
	READER* r = p->r;
	if (IsVariable (r, it)) return 1;
	if (IsOperator (r, it)) return 1;
	if (IsNumeric  (r, it)) return 1;
	if (IsSetting  (r, it)) return 1;
	if (IsFunction (r, it)) return 1;
	SetTokenWhat(r, it,VPC_UNDEFINED);
	SetTokenWhat2(r, it, 0);
	return 0;
}

static void ClearTokenStack(PARSER *p)
{
	p->cTokenStack  = 0;
	p->cPolish      = 0;
}

static int TokenStackSize(PARSER *p)
{
	return p->mSize;
}

static int PushToken(PARSER *p,int i)
{
	if (p->cTokenStack >= TokenStackSize(p)) {
		ERROR(fprintf(stderr, "Error: stack overflowed(%d).\n", TokenStackSize(p)));
		return -1;
	}
	p->TokenStack[p->cTokenStack++] = i;
	return p->cTokenStack;
}

static int IsTokenStackEmpty(PARSER *p) { return p->cTokenStack == 0; }

static int PopToken(PARSER* p)
{
	if (p->cTokenStack <= 0) {
		ERROR(fprintf(stderr, "Error: stack underflowed.\n"));
		return -1;
	}
	return p->TokenStack[--p->cTokenStack];
}

static int TopToken(PARSER *p)
{
	if (p->cTokenStack <= 0) {
		ERROR(fprintf(stderr, "Error: stack underflowed.\n"));
		return -1;
	}
	return p->TokenStack[p->cTokenStack - 1];
}

/* Put token(==it) to reverse polish array */
static void PutPolish(PARSER *p,int it)
{
	int iStatement = (p->r)->iStatement;
	p->PolishDivider[iStatement].end = p->cTotalPolish;
	p->TotalPolish[p->cTotalPolish++] = it;
}

static int ParsePolish(PARSER *p)
{
	int i;
	int ci = 0;
	int token;
	int iStatement = (p->r)->iStatement;
#ifdef _DEBUG
	printf("R_Polish:");
#endif

	for (i = p->PolishDivider[iStatement].start; i <= p->PolishDivider[iStatement].end; ++i) {
		token = p->TotalPolish[i];
#ifdef _DEBUG
		printf(" %s ",TokenPTR(p->r,token));
#endif
		switch(TokenWhat(p->r,token))
		{
			case VPC_VARIABLE:
			case VPC_NUMERIC:
				++ci;
				break;
			case VPC_UOPERATOR:
				break;
			case VPC_BOPERATOR:
				--ci;
				if (ci < 1) {
					ERROR(fprintf(stderr, "Error: syntax error.\n"));
					return 0;
				}
				break;
			case VPC_FUNC:
				ci -= (abs(FunctionArguments(TokenWhat2(p->r,token)))-1);
				if (ci < 1) {
					ERROR(fprintf(stderr, "Error: syntax error.\n"));
					return 0;
				}
				break;
			default:
				++ci;
				break;
		}
	}
#ifdef _DEBUG
	printf(" \n");
#endif
	if (ci == 1) return 1;
	ERROR(fprintf(stderr, "Error: syntax error.\n"));
	return 0;
}

static int MkReversePolish(PARSER *p)
{
	int i;
	int nc;
	int it;
	int order;
	int token;
	int iStatement = (p->r)->iStatement;
	int nt = TokenCount(p->r);

	ClearTokenStack(p);

	if (iStatement > 0) {
		int j;
		for (j = 0; j < iStatement; ++j) {
			if (p->PolishDivider[j].start > p->PolishDivider[j].end) continue;
			p->PolishDivider[iStatement].start = p->PolishDivider[j].end + 1;
		}
	} 
	else {
		p->PolishDivider[iStatement].start = 0;
	}
	p->PolishDivider[iStatement].end = p->PolishDivider[iStatement].start - 1;

	for (it = 0; it < nt; ++it) {
		switch (TokenWhat(p->r,it))
		{
		case VPC_VARIABLE:
		case VPC_NUMERIC:
			PutPolish(p, it);
			break;
		case VPC_BRA:
			PushToken(p,it); /* '(' */
			break;
		case VPC_KET: /* ')' */
			while (TokenWhat(p->r,token = PopToken(p)) != VPC_BRA) {
				PutPolish(p, token);
			}
			if (TokenWhat(p->r, TopToken(p)) == VPC_FUNC) PutPolish(p, PopToken(p));
			break;
		case VPC_UOPERATOR:
			if(TokenChar(p->r,it,0)=='-') PushToken(p,it); /* Unary operator: '+' is ignored. */
			break;
		case VPC_BOPERATOR:
			if (IsTokenStackEmpty(p)) PushToken(p,it);
			else {
				char ch = TokenChar(p->r, it, 0);
				order = TokenPriority(p->r,it); /* info == operator priority */
				if (ch == '=') {
					/* '=' must be processed from right to left */
					if(TokenWhat(p->r,it-1)==VPC_NUMERIC) {
						ERROR(fprintf(stderr, "Error: syntax error('=')\n"));
						return -1;
					}
					while (!IsTokenStackEmpty(p) && TokenPriority(p->r, TopToken(p)) > order) {
						PutPolish(p, PopToken(p));
					}
				} else {
					while (!IsTokenStackEmpty(p) && TokenPriority(p->r, TopToken(p)) >= order) {
						PutPolish(p, PopToken(p));
					}
				}
				if (PushToken(p,it) < 0) return -1;
			}
			break;
		case VPC_COMMA:
			while (!IsTokenStackEmpty(p) && TokenWhat(p->r,token = TopToken(p)) != VPC_BRA) {
				PutPolish(p, PopToken(p));
			}
			break;
		case VPC_FUNC:
			PushToken(p,it); /* sin cos ... */
			break;
		default:
			ERROR(fprintf(stderr, "Error: syntax error("));
			nc = TokenSize(p->r,it);
			for (i = 0; i < nc; ++i) {
				fprintf(stderr, "%c", TokenChar(p->r,it, i));
			}
			fprintf(stderr, ").\n");
			return -1;
		}
	}
	while (!IsTokenStackEmpty(p)) PutPolish(p, PopToken(p));
	return 0;
}

static int ParseCalculation(PARSER *p)
{
	if (TokenWhat(p->r,0) != VPC_VARIABLE) goto SyntaxError;
	if (TokenChar(p->r,1, 0) != '=')       goto SyntaxError;
	if (MkReversePolish(p) < 0) return 0;
	if (!ParsePolish(p))        return 0;
	return 1;

SyntaxError:
	ERROR(fprintf(stderr, "Syntax error.\n"));
	return 0;
}

/* Parse and call calculation routines in calculator.c if the statement is valid. */
void ExecuteStatement(PARSER *p,int iStatement)
{
	int nt;

	if (iStatement >= (p->r)->cStatements) return;
	SetStatement(p->r, iStatement);

	if (gfBreak) return;

	nt = TokenCount(p->r);

	if (nt <= 0)                             goto Next;
	if (nt == 1) {
		if (IsToken("quit", p->r, 0)||IsToken("exit", p->r, 0)) {
			gfQuit = 1;
			return;
		}
		if (IsToken("break", p->r, 0)) {
			gfBreak = 1;
			return;
		}
		if (IsToken("save", p->r, 0)) {
			DoWrite (p,"./vpc.ini");
			goto Next;
		}
		if (IsToken("restore", p->r, 0)) {
			DoRead("./vpc.ini");
			goto Next;
		}
	}

	ClearTokenStack(p);

	if (TokenWhat(p->r, 0) == VPC_VARIABLE) {
		int i;
		/* Copy reverse polish to execution polish buffer */
		p->cPolish = 0;
		for (i = p->PolishDivider[(p->r)->iStatement].start; i <= p->PolishDivider[(p->r)->iStatement].end; ++i) {
			p->Polish[p->cPolish++] = p->TotalPolish[i];
		}
		ComputePolish(p);
		goto Next;
	}

	switch (TokenChar(p->r, 0, 0))
	{
	case '$':
		DoSetting(p); /* In setting.c */
		goto Next;
	case '?':
		DoPrint(p);         /* In setting.c */
		goto Next;
	case '!':
		if (TokenCount(p->r) == 2) {
			system(TokenPTR(p->r,1));
		} else {
			ERROR(fprintf(stderr, "Error: Syntax error(!).\n"));
		}
		goto Next;
	}

	if (IsToken("load", p->r, 0) && nt > 2) {
		ParseAndExecuteLoad(p,nt);
		return;
	}

	if (IsToken("repeat", p->r, 0) && nt == 2) {
		gfLoop = 1;
		ParseAndExecuteRepeat(p);
		return; /* "repeat" executes to the end of line */
	}
	if (IsToken("while", p->r, 0) && (nt == 4 || nt == 5)) 	{
		gfLoop = 1;
		ParseAndExecuteWhile(p, nt);
		return; 
	}
	if (IsToken("if", p->r, 0) && (nt == 4 || nt == 5)) {
		ParseAndExecuteIf(p, nt);
		return;
	}

	if (IsToken("read", p->r, 0) && nt == 2)           {
		DoRead  (TokenPTR(p->r, 1));
		goto Next;
	} 
	if (IsToken("write", p->r, 0) && nt == 2)          {
		DoWrite (p,TokenPTR(p->r, 1));
		goto Next;
	}


	ERROR(fprintf(stderr, "Error: Syntax error(%s).\n",TokenPTR(p->r,0)));
	return;

Next:
	if (IsError()) return;
	ExecuteStatement(p, iStatement + 1);
}

int ParseStatement(PARSER *p)
{
	int i;
	int nt;
	int cBracket = 0;

	nt = TokenCount(p->r);

	if (nt <= 0)                         return 1;
	if (nt == 1) {
		if (IsToken("quit",    p->r, 0)) return 1;
		if (IsToken("exit",    p->r, 0)) return 1;
		if (IsToken("save",    p->r, 0)) return 1;
		if (IsToken("restore", p->r, 0)) return 1;
		if (IsToken("break",   p->r, 0)) return 1;
	}

	for (i = 0; i < nt; ++i) {
		if (IsToken("(", p->r, i)) ++cBracket;
		if (IsToken(")", p->r, i)) {
			if (cBracket <= 0) {
				ERROR(fprintf(stderr, "Error: brackets unbalanced.\n"));
				return 0;
			}
			--cBracket;
		}
		ParseToken(p, i);
	}

	if (cBracket != 0) {
		ERROR(fprintf(stderr, "Error: brackets unbalanced.\n"));
		return 0;
	}

	switch (TokenChar(p->r, 0, 0))
	{
	case '$':
		if (nt != 3) {
			ERROR(fprintf(stderr, " Syntax error.\n"));
			return 0;
		}
		if (IsToken("=", p->r, 1)) return 1;
		ERROR(fprintf(stderr, " Syntax error (=).\n"));
		return 0;
	case '?':
		if (TokenCount(p->r) == 2) return 1;
		ERROR(fprintf(stderr, " Syntax error(?).\n"));
		return 0;
	case '!':
		if (TokenCount(p->r) == 2) return 1;
		ERROR(fprintf(stderr, " Syntax error(!).\n"));
		return 0;
	}
	if (IsToken("load", p->r, 0) && nt > 2)    return 1;
	if (IsToken("read", p->r, 0) && nt == 2)   return 1;
	if (IsToken("write", p->r, 0) && nt == 2)  return 1;
	if (IsToken("repeat", p->r, 0) && nt == 2) return 1;
	if (IsToken("while", p->r, 0) && (nt == 4 || nt == 5)) {
		int  nm = 0;
		char chv1 = TokenChar(p->r, 1, 0);
		char chv2;
		if (nt == 4) {
			if (!IsToken(">", p->r, 2) && !IsToken("<", p->r, 2)) goto Error;
			chv2 = TokenChar(p->r, 3, 0);
		}
		else {
			if (!IsToken("=", p->r, 3) ) goto Error;
			if (TokenSize(p->r, 2) != 1) goto Error;
			chv2 = TokenChar(p->r, 2, 0);
			if ((chv2 != '=') && (chv2 != '!') && (chv2 != '>') && (chv2 != '<')) goto Error;
			chv2 = TokenChar(p->r, 4, 0);
		}
		chv1 = chv1 - '0';
		chv2 = chv2 - '0';
		if ((chv1 >= 0 && chv1<=9) && (chv2 >= 0 && chv2<=9)) {
			ERROR(fprintf(stderr, "Error: At least one variable must be specified(%s).\n", TokenPTR(p->r, 0)));
			goto Error;
		}
		return 1;
	}
	if (IsToken("if", p->r, 0) && (nt == 4 || nt == 5)) {
		int  nm = 0;
		char chv1 = TokenChar(p->r, 1, 0);
		char chv2;
		if (nt == 4) {
			if (!IsToken(">", p->r, 2) && !IsToken("<", p->r, 2)) goto Error;
			chv2 = TokenChar(p->r, 3, 0);
		}
		else {
			if (!IsToken("=", p->r, 3)) goto Error;
			if (TokenSize(p->r, 2) != 1) goto Error;
			chv2 = TokenChar(p->r, 2, 0);
			if ((chv2 != '=') && (chv2 != '!') && (chv2 != '>') && (chv2 != '<')) goto Error;
			chv2 = TokenChar(p->r, 4, 0);
		}
		chv1 = chv1 - '0';
		chv2 = chv2 - '0';
		if ((chv1 >= 0 && chv1 <= 9) && (chv2 >= 0 && chv2 <= 9)) {
			ERROR(fprintf(stderr, "Error: At least one variable must be specified(%s).\n", TokenPTR(p->r, 0)));
			goto Error;
		}
		return 1;
	}

	if (TokenWhat(p->r, 0) == VPC_VARIABLE) {
		return ParseCalculation(p);
	}

Error:
	ERROR(fprintf(stderr, "Error: Syntax error(%s).\n",TokenPTR(p->r,0)));
	return 0;
}

#ifdef _DEBUG
static void PrintPolish(PARSER* p)
{
	int i, j;
	READER* r = p->r;
	for (i = 0; i < r->cStatements; ++i) {
		printf("\n Polish(%d-%d) %d:[", p->PolishDivider[i].start, p->PolishDivider[i].end, p->PolishDivider[i].end - p->PolishDivider[i].start + 1);
		for (j = p->PolishDivider[i].start; j <= p->PolishDivider[i].end; ++j) {
			if (j != p->PolishDivider[i].start) printf(",");
			printf("%s", TokenPTR(r, p->TotalPolish[j]));
		}
		printf("]\n");
	}
}
#endif

static char ExecuteLine(PARSER *p,char ch)
{
	int i;

	if ( IsError() ) return ch;

	/* No syntax error => execute statements in a line */
	for (i = 0; i < (p->r)->cStatements; ++i) {
		SetStatement(p->r,i);
		ParseStatement(p);
		if ( IsError() ) return ch;
	}

	ExecuteStatement(p,0);
	return ch;
}

void ClearParser(PARSER* p)
{
	int i;
	p->cTotalPolish = 0;
	for (i = 0; i < (p->r)->mStatements; ++i) {
		p->PolishDivider[i].start =  0;
		p->PolishDivider[i].end   = -1;
	}
}

void ReadAndExecuteLines(FILE* f)
{
	char ch = '\n';
	PARSER* p = OpenParser(f,1024);
	do {
		if (IsEOL(ch) && !gfInFile) printf("\n>");
		ClearGlobal();
		ClearParser(p);
		ch = ReadLine(p->r);
		ch = ExecuteLine(p,ch);
	} while (!IsEOF(ch) && !IsQuit());
	CloseParser(p);
}
