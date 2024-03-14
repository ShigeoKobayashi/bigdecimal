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

static int* gStack  = 0; /* stack for creating reverse polish */
static int  gcStack = 0;

int* gPolish = 0;
int  gcPolish = 0;

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

void InitParser()
{
	if (gStack  != NULL) free(gStack);
	if (gPolish != NULL) free(gPolish);
	gStack  = calloc(gmTokens, sizeof(int));
	gPolish = calloc(gmTokens, sizeof(int));
	if (gStack == NULL|| gPolish == NULL) {
		FATAL(fprintf(stderr, "SYSTEM_ERROR: memory allocation failed: stack(%d)\n", gmTokens));
	}
}

void FinishParser()
{
	if (gStack  != NULL) free(gStack);
	if (gPolish != NULL) free(gPolish);
	gStack  = NULL;
	gPolish = NULL;
}

static int IsDigit(UCHAR ch)
{
	if (ch == '0' || ch == '1' || ch == '2' || ch == '3' || ch == '4' || ch == '5' || ch == '6' || ch == '7' || ch == '8' || ch == '9') return 1;
	return 0;
}

static int IsNumeric(int iStatement,int it)
{
	int ixs      = 0;
	int ixe      = TokenSize(iStatement,it)-1;
	UCHAR ch      = TokenChar(iStatement,it, 0);
	int  cd      = 0;
	int cSign    = 0;
	int cE       = 0;
	int cEDigits = 0;
	int cDot     = 0;

	int i;
	for (i = ixs; i <= ixe; ++i) {
		ch = TokenChar(iStatement,it, i);
		if(isspace(ch)||ch == gchSeparator) continue;
		if (IsDigit(ch))       {
			if (cE == 0) cd++;
			else         cEDigits++;
			continue;
		}
		if (ch == '+' || ch == '-') {
			if (cd > 0) return 0;
			if (cSign++ > 0) return 0;
		} else
		if (ch == 'E' || ch == 'e' || ch == 'D' || ch == 'd') {
			if (cd <= 0)  return 0;
			if (cE++ > 0) return 0;
			cSign = 0;
		} else
		if (ch == '.') {
			if (cDot++ > 0) return 0;
			if (cE     > 0) return 0;
			if (cd    <= 0) return 0;
		}
		else {
			return 0;
		}
	}
	if (cE > 0 && cEDigits == 0) return 0;
	if (cd <= 0)                 return 0;

	if (cE > 0 || cDot > 0) SetTokenWhat2(iStatement,it,2); /* Floating point number. */
	else                    SetTokenWhat2(iStatement,it,1); /* Integer number. */
	SetTokenWhat(iStatement,it,VPC_NUMERIC);
	return 1;                         /* Yes numeric(Integer or floating) */
}

static void PrintToken(FILE* f,int iStatement, int it)
{
	fprintf(f, "%s",TokenPTR(iStatement,it));
}

int IsToken(const UCHAR* token,int iStatement, int it)
{
	int nc = TokenSize(iStatement,it);
	int i;
	if (strlen(token) != nc) return 0;
	for (i = 0; i < nc; ++i) if (token[i] != TokenChar(iStatement,it, i)) return 0;
	return 1;
}

static int IsVariable(int iStatement,int it)
{
	int ich;
	UCHAR ch;

	SetTokenWhat (iStatement,it,0);
	SetTokenWhat2(iStatement,it, 0);

	if (TokenSize(iStatement,it) != 1) return 0;
	ch = TokenChar(iStatement,it, 0);
	if (ch == 'R'           ) { SetTokenWhat(iStatement,it,VPC_VARIABLE);  SetTokenWhat2(iStatement,it,26);  goto More; }
	ich = ch - 'a';
	if (ich >= 0 && ich < 26) { SetTokenWhat(iStatement,it,VPC_VARIABLE);  SetTokenWhat2(iStatement,it,ich); goto More; }
	return 0;

More:
	if (it + 1 >= gStatements[iStatement].end) return 1;
	if (IsToken("(", iStatement, it+1)) {
		ERROR(fprintf(stderr, "Error: syntax invalid(variable is not a function).\n"));
		return 0;
	}
	return 1;
}

static int IsOperator(int iStatement,int it)
{
	int ich;
	UCHAR ch;

	SetTokenWhat(iStatement,it,0);
	SetTokenPriority(iStatement,it,0);

	if (TokenSize(iStatement,it) != 1) return 0;
	ch = TokenChar(iStatement, it, 0);

	if (ch == '(') { SetTokenWhat(iStatement, it,VPC_BRA);       SetTokenPriority(iStatement, it, 0); return 1; }
	if (ch == ')') { SetTokenWhat(iStatement, it,VPC_KET);       SetTokenPriority(iStatement, it, 1); return 1; }
	if (ch == '?') { SetTokenWhat(iStatement, it,VPC_PRINT);     SetTokenPriority(iStatement, it, 0); return 1; }
	if (ch == '=') { SetTokenWhat(iStatement, it,VPC_BOPERATOR); SetTokenPriority(iStatement, it,20); return 1; }
	if (ch == '*') { SetTokenWhat(iStatement, it,VPC_BOPERATOR); SetTokenPriority(iStatement, it,40); return 1; }
	if (ch == '/') { SetTokenWhat(iStatement, it,VPC_BOPERATOR); SetTokenPriority(iStatement, it,40); return 1; }
	if (ch == ',') { SetTokenWhat(iStatement, it,VPC_COMMA);     SetTokenPriority(iStatement, it, 0); return 1; }

	if (ch == '+' || ch == '-') {
		SetTokenWhat(iStatement, it, VPC_BOPERATOR); SetTokenPriority(iStatement, it,20);
		if (it <= 0) {
			SetTokenWhat(iStatement, it,VPC_UOPERATOR); SetTokenPriority(iStatement, it,200);
			return 1;
		}
		ich = TokenWhat(iStatement, it - 1);
		if (ich == VPC_BRA || ich == VPC_COMMA || ich == VPC_BOPERATOR || ich == VPC_UOPERATOR) {
			SetTokenWhat(iStatement, it,VPC_UOPERATOR); SetTokenPriority(iStatement, it,200);
			return 1;
		}
		return 1;
	}
	return 0;
}

static int IsSetting(int iStatement, int it)
{
	int i;

	SetTokenWhat(iStatement, it, 0);
	SetTokenWhat2(iStatement, it, 0);

	for (i = 0; i < gmSetting; ++i) {
		if (strcmp(TokenPTR(iStatement, it),gSetting[i].name)==0) { SetTokenWhat(iStatement, it, VPC_SETTING); SetTokenWhat2(iStatement, it, 0); return 1; }
	}
	return 0;
}

static int IsFunction(int iStatement,int it)
{
	int i;
	int cb = 0;
	int cc = 0;
	int na = 0;
	int nt = TokenCount(iStatement);
	SetTokenWhat(iStatement, it, 0);
	SetTokenWhat2(iStatement, it, 0);

	for (i = 0; i < gmFunctions; ++i) {
		if (IsToken(FunctionName(i), iStatement, it)) {
			SetTokenWhat(iStatement,it, VPC_FUNC); SetTokenWhat2(iStatement,it, i);   na = FunctionArguments(i); goto ArgNoCheck;
		}
	}
	return 0;

ArgNoCheck: /* check on argument number */

	if (it >= gStatements[iStatement].end || !IsToken("(", iStatement, it + 1)) goto Error;
	cb = 0;
	for (i = it+1; i < nt; ++i) {
		if (IsToken("(", iStatement,i)) {
			++cb;
			if ( i+1 >= nt )               goto Error;
			if (TokenSize(iStatement,i + 1) > 1)      continue;
			if (IsToken("+", iStatement, i - 1))       continue;
			if (IsToken("-", iStatement, i - 1))       continue;
			if (IsToken(",", iStatement, i + 1))       goto Error;
			if (IsToken("*", iStatement, i + 1))       goto Error;
			if (IsToken("/", iStatement, i + 1))       goto Error;
			continue;
		}
		if (IsToken(")", iStatement,i)) {
			if (--cb <= 0)                 break;
			if (TokenSize(iStatement,i - 1) > 1)      continue;
			if (IsToken(",", iStatement,i - 1))       goto Error;
			if (IsToken("+", iStatement,i - 1))       goto Error;
			if (IsToken("-", iStatement,i - 1))       goto Error;
			if (IsToken(",", iStatement,i - 1))       goto Error;
			if (IsToken("*", iStatement,i - 1))       goto Error;
			if (IsToken("/", iStatement,i - 1))       goto Error;
			continue;
		}

		if (cb == 1) {
			if (IsToken(",", iStatement,i)) { ++cc; continue; }
		}
	}

	if (na < 0) na = -na;
	if (na <= 0 && cc == 0) return 1;
	if ( (na -1) != cc    ) goto Error;
	return 1;

Error:
	ERROR(fprintf(stderr, "Error:'"));
	PrintToken(stderr, iStatement, it);
	ERROR(fprintf(stderr, "' syntax invalid(illegal arguments or illegal number of arguments).\n"));
	return 0;
}

static int ParseToken(int iStatement,int it)
{
	if (IsVariable (iStatement, it)) return 1;
	if (IsOperator (iStatement, it)) return 1;
	if (IsNumeric  (iStatement, it)) return 1;
	if (IsSetting  (iStatement, it)) return 1;
	if (IsFunction (iStatement, it)) return 1;
	SetTokenWhat(iStatement, it,VPC_UNDEFINED);
	SetTokenWhat2(iStatement, it, 0);
	return 0;
}

static void ClearStack()
{
	gcStack  = 0;
	gcPolish = 0;
}

static int StackSize()
{
	return gmTokens;
}

static int Push(int i)
{
	if (gcStack >= StackSize()) {
		ERROR(fprintf(stderr, "Error: stack overflowed(%d).\n", StackSize()));
		return -1;
	}
	gStack[gcStack++] = i;
	return gcStack;
}

static int Empty()
{
	return gcStack == 0;
}

static int Pop()
{
	if (gcStack <= 0) {
		ERROR(fprintf(stderr, "Error: stack underflowed.\n"));
		return -1;
	}
	return gStack[--gcStack];
}

static int Top()
{
	if (gcStack <= 0) {
		ERROR(fprintf(stderr, "Error: stack underflowed.\n"));
		return -1;
	}
	return gStack[gcStack - 1];
}

/* Put token(==it) to reverse polish array */
static void PutPolish(int it)
{
	gPolish[gcPolish++] = it;
}

static int ParsePolish(int iStatement)
{
	int i;
	int ci = 0;
	int token;

	for (i = 0; i < gcPolish; ++i) {
		token = gPolish[i];
		switch(TokenWhat(iStatement,token))
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
				ci -= (abs(FunctionArguments(TokenWhat2(iStatement,token)))-1);
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
	if (ci == 1) return 1;
	ERROR(fprintf(stderr, "Error: syntax error.\n"));
	return 0;
}

static int MkReversePolish(int iStatement)
{
	int i;
	int nc;
	int it;
	int order;
	int token;

	ClearStack();

	int nt = TokenCount(iStatement);
	for (it = 0; it < nt; ++it) {
		switch (TokenWhat(iStatement,it))
		{
		case VPC_VARIABLE:
		case VPC_NUMERIC:
			PutPolish(it);
			break;
		case VPC_BRA:
			Push(it); /* '(' */
			break;
		case VPC_KET: /* ')' */
			while (TokenWhat(iStatement,token = Pop()) != VPC_BRA) {
				PutPolish(token);
			}
			if (TokenWhat(iStatement,Top()) == VPC_FUNC) PutPolish(Pop());
			break;
		case VPC_UOPERATOR:
			if(TokenChar(iStatement,it,0)=='-') Push(it); /* Unary operator: '+' is ignored. */
			break;
		case VPC_BOPERATOR:
			if (Empty()) Push(it);
			else {
				order = TokenPriority(iStatement,it); /* info == operator priority */
				while (!Empty() && TokenPriority(iStatement,Top()) > order) {
					PutPolish(Pop());
				}
				if (Push(it) < 0) return -1;
			}
			break;
		case VPC_COMMA:
			while (!Empty() && TokenWhat(iStatement,token = Top()) != VPC_BRA) {
				PutPolish(Pop());
			}
			break;
		case VPC_FUNC:
			Push(it); /* sin cos ... */
			break;
		default:
			ERROR(fprintf(stderr, "Error: syntax error("));
			nc = TokenSize(iStatement,it);
			for (i = 0; i < nc; ++i) {
				fprintf(stderr, "%c", TokenChar(iStatement,it, i));
			}
			fprintf(stderr, ").\n");
			return -1;
		}
	}
	while (!Empty()) PutPolish(Pop());
	return 0;
}

static int ParseCalculation(int iStatement)
{
	if (TokenWhat(iStatement,0) != VPC_VARIABLE) goto SyntaxError;
	if (TokenChar(iStatement,1, 0) != '=')       goto SyntaxError;
	if (MkReversePolish(iStatement) < 0) return 0;
	if (!ParsePolish(iStatement))        return 0;
	return 1;

SyntaxError:
	ERROR(fprintf(stderr, "Syntax error.\n"));
	return 0;
}

/* Parse and call calculation routines in calculator.c if the statement is valid. */
int ExecuteStatement(int iStatement)
{
	int nt;

	nt = TokenCount(iStatement);

	if (nt <= 0)                                   return  1;
	if (nt == 1 && IsToken("quit", iStatement, 0)) return -1;

	ClearStack();

	switch (TokenChar(iStatement, 0, 0))
	{
	case '$':
		DoSetting(iStatement); /* In setting.c */
		return 1;
	case '?':
		DoPrint(iStatement);         /* In calculaot.c */
		return 1;
	}

	if (IsToken("read", iStatement, 0) && nt == 2) { DoRead  (TokenPTR(iStatement, 1)); return 1; }
	if (IsToken("write", iStatement, 0) && nt == 2) { DoWrite (TokenPTR(iStatement, 1)); return 1; }
	if (IsToken("repeat", iStatement, 0) && nt == 2) { DoRepeat(TokenPTR(iStatement, 1)); return 1; }
	if (IsToken("while", iStatement, 0) && (nt == 4||nt==5)) { DoWhile(iStatement, nt); return 1; }

	if (TokenWhat(iStatement, 0) == VPC_VARIABLE) {
		if(ParseCalculation(iStatement)) ComputePolish(iStatement);
	} else {
		ERROR(fprintf(stderr, "Error: Syntax error.\n"));
	}
	return 1;
}

int ParseStatement(int iStatement)
{
	int i;
	int nt;
	int cBracket = 0;

	nt = TokenCount(iStatement);

	if (nt <= 0)                                   return 1;
	if (nt == 1 && IsToken("quit", iStatement, 0)) return 1;

	for (i = 0; i < nt; ++i) {
		if (IsToken("(", iStatement, i)) ++cBracket;
		if (IsToken(")", iStatement, i)) {
			if (cBracket <= 0) {
				ERROR(fprintf(stderr, "Error: brackets unbalanced.\n"));
				return 0;
			}
			--cBracket;
		}
		ParseToken(iStatement, i);
	}

	if (cBracket != 0) {
		ERROR(fprintf(stderr, "Error: brackets unbalanced.\n"));
		return 0;
	}

	switch (TokenChar(iStatement, 0, 0))
	{
	case '$':
		if (nt != 3) {
			ERROR(fprintf(stderr, " Syntax error.\n"));
			return 0;
		}
		if (IsToken("=", iStatement, 1)) return 1;
		ERROR(fprintf(stderr, " Syntax error (=).\n"));
		return 0;
	case '?':
		if (TokenCount(iStatement) != 2) { ERROR(fprintf(stderr, " Syntax error.\n")); return 0; }
		return 1;
	}

	if (IsToken("read", iStatement, 0) && nt == 2) return 1;
	if (IsToken("write", iStatement, 0) && nt == 2) return 1;
	if (IsToken("repeat", iStatement, 0) && nt == 2) return 1;
	if (IsToken("while", iStatement, 0) && (nt == 4 || nt == 5)) {
		int  nm = 0;
		char chv1 = TokenChar(iStatement, 1, 0);
		char chv2;
		if (nt == 4) {
			if (!IsToken(">", iStatement, 2) && !IsToken("<", iStatement, 2)) goto Error;
			chv2 = TokenChar(iStatement, 3, 0);
		}
		else {
			if (!IsToken("=", iStatement, 3) ) goto Error;
			if (TokenSize(iStatement, 2) != 1) goto Error;
			chv2 = TokenChar(iStatement, 2, 0);
			if ((chv2 != '=') && (chv2 != '!') && (chv2 != '>') && (chv2 != '<')) goto Error;
			chv2 = TokenChar(iStatement, 4, 0);
		}
		chv1 = chv1 - 'a';
		chv2 = chv2 - 'a';
		if ((chv1 < 0 || chv1>9) && (chv2 < 0 || chv2>9)) {
			ERROR(fprintf(stderr, "Error: At least one variable must be specified(%s).\n", TokenPTR(iStatement, 0)));
			goto Error;
		}
		return 1;
	}

	if (TokenWhat(iStatement, 0) == VPC_VARIABLE) return ParseCalculation(iStatement);

Error:
	ERROR(fprintf(stderr, "Error: Syntax error(%s).\n",TokenPTR(iStatement,0)));
	return 0;
}
