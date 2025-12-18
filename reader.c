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

int       gfLoop = 0;
char      gDelimiters[] = { '?' , '(', ')' ,  '=' , '*' , '/' , '+' , '-' , ',' , ';', '<' , '>' , '!' };
int       gmDelimiters = sizeof(gDelimiters) / sizeof(gDelimiters[0]);

void CloseReader(READER* r)
{
	if (r == NULL) return;
	if (r->file != NULL) {
		if(r->file!=stdin) fclose(r->file);
	}
	if (r->Buffer != NULL)     free(r->Buffer);
	if (r->Statements != NULL) free(r->Statements);
	if (r->Tokens != NULL)     free(r->Tokens);
	free(r);
}

void SetStatement(READER* r, int i)
{
	r->iStatement = i;
}

int GetStatement(READER* r)
{
	return r->iStatement;
}


READER* OpenReader(FILE* f, int max_line_char)
{
	READER* r = NULL;

	r = (READER*)calloc(1, sizeof(READER));
	if (r == NULL) {
		ERROR(fprintf(stderr, "Error: failed to allocate memory for LINE structure(%d).\n", sizeof(READER)));
		goto Error;
	}
	r->file = f;
	r->Buffer = (char*)calloc(max_line_char, sizeof(char));
	if (r->Buffer==NULL) {
		ERROR(fprintf(stderr, "Error: failed to allocate memory for line buffer(%d).\n", max_line_char));
		goto Error;
	}
	r->mBuffer = max_line_char;
	r->Statements = (STATEMENT*)calloc(max_line_char / 2, sizeof(STATEMENT));
	if (r->Statements==NULL) {
		ERROR(fprintf(stderr, "Error: failed to allocate memory for STATEMENT structure(%d).\n", (max_line_char / 2)*sizeof(STATEMENT)));
		goto Error;
	}
	r->mStatements = max_line_char / 2;
	r->Tokens = (TOKEN*)calloc(max_line_char / 2, sizeof(TOKEN));
	if (r->Tokens==NULL) {
		ERROR(fprintf(stderr, "Error: failed to allocate memory for TOKEN structure(%d).\n", (max_line_char / 2) * sizeof(TOKEN)));
		goto Error;
	}
	r->mTokens = max_line_char / 2;
	return r;

Error:
	CloseReader(r);
	return NULL;
}

void ClearReader(void* reader)
{
	int i;
	READER* r = (READER*)reader;
	if (r == NULL) return;
	r->cBuffer = 0;
	r->cTokens = 0;
	r->cStatements = 0;
	memset(r->Buffer, 0, r->mBuffer);
	for (i = 0; i < r->mStatements; ++i) {
		r->Statements[i].start =  0;
		r->Statements[i].end   = -1;
	}
	for (i = 0; i < r->mTokens; ++i) {
		r->Tokens[i].start =  0;
		r->Tokens[i].end   = -1;
		r->Tokens[i].what  = 0;
		r->Tokens[i].etc   = 0;
	}
}

#ifdef _DEBUG
static void PrintTokens(READER* r)
{
	int i, j;

	for (i = 0; i < r->cStatements; ++i) {
		printf("\n Tokens %d:[", r->Statements[i].end - r->Statements[i].start+1);
		for (j = 0; j<= r->Statements[i].end - r->Statements[i].start; ++j) {
			if (j != 0) printf(",");
			printf("%s", TokenPTR(r,j));
		}
		printf("]\n");
	}
}
#endif

char  IsDEL(char ch) { int i; for (i = 0; i < gmDelimiters; ++i) {if (ch == gDelimiters[i]) return ch;}	return 0;}
int   IsEOL(char ch) { return (ch == (char)EOF) || (ch == '\n');}
int   IsEOS(char ch) { return (ch == ';') || IsEOL(ch);}
int   IsEOF(char ch) { return ((ch == (char)EOF) || (ch == 0x1A));}

int IsDigit(char ch)
{
	if (ch == '0' || ch == '1' || ch == '2' || ch == '3' || ch == '4' || ch == '5' || ch == '6' || ch == '7' || ch == '8' || ch == '9') return 1;
	return 0;
}


static char GetChar(READER* r)
{
	FILE* f  = r->file;
	char  ch = getc(f);
	if (ch == 0x1A) ch = (char)EOF;
	return ch;
}

static char SkipToEOL(READER *r)
{
	char ch;
	while (!IsEOL(ch = GetChar(r)));
	return ch;
}

static void PushBuffer(READER* r,int ch)
{
	if (r->cBuffer >= r->mBuffer) {
		FATAL(fprintf(stderr, "SYSTEM_ERROR:String buffer overflowed(%d)\n", r->mBuffer));
	}
	r->Buffer[r->cBuffer++] = ch;
	r->Buffer[r->cBuffer  ] = '\0';
}

static char PopBuffer(READER* r)
{
	if (--(r->cBuffer) < 0) {
		FATAL(fprintf(stderr, "SYSTEM_ERROR:String buffer underflowed."));
	}
	return r->Buffer[r->cBuffer];
}

static void PushToken(READER* r,int ixs, int ixe)
{
	if (r->cTokens >= r->mTokens) {
		FATAL(fprintf(stderr, "SYSTEM_ERROR:Token buffer overflowed(%d)\n", r->mTokens));
	}
	r->Tokens[r->cTokens].start     = ixs;
	r->Tokens[r->cTokens].end       = ixe;
	r->Statements[r->iStatement].end = r->cTokens++;
}

int TokenCount(READER* r)
{
	return r->Statements[r->iStatement].end- r->Statements[r->iStatement].start + 1;
}
int TokenSize(READER* r,int it)
{
	return r->Tokens[r->Statements[r->iStatement].start + it].end - r->Tokens[r->Statements[r->iStatement].start + it].start + 1;
}
char  TokenChar(READER* r,int it, int ic)
{
	return r->Buffer[r->Tokens[r->Statements[r->iStatement].start + it].start + ic];
}
char* TokenPTR(READER* r,int it)
{
	return &(r->Buffer[r->Tokens[r->Statements[r->iStatement].start + it].start]);
}
int    TokenWhat(READER* r,int it)
{
	return r->Tokens[r->Statements[r->iStatement].start + it].what;
}
void   SetTokenWhat(READER* r,int it, int w) {
	r->Tokens[r->Statements[r->iStatement].start + it].what = w;
}
int    TokenPriority(READER* r,int it)
{
	return r->Tokens[r->Statements[r->iStatement].start + it].etc;
}
void   SetTokenPriority(READER* r,int it, int p) {
	r->Tokens[r->Statements[r->iStatement].start + it].etc = p;
}
int    TokenWhat2(READER* r,int it)
{
	return r->Tokens[r->Statements[r->iStatement].start + it].etc;
}
void   SetTokenWhat2(READER* r,int it, int w)
{
	r->Tokens[r->Statements[r->iStatement].start + it].etc = w;
}

int IsToken(const char* token, READER* r, int it)
{
	int nc = TokenSize(r, it);
	int i;
	if (strlen(token) != nc) return 0;
	for (i = 0; i < nc; ++i) { if (token[i] != TokenChar(r, it, i)) return 0; }
	return 1;
}

int IsNumeric(READER* r, int it)
{
	int  ixs = 0;
	int  ixe = TokenSize(r, it) - 1;
	char ch = TokenChar(r, it, 0);
	int  cDot = 0;

	int   cd = 0;
	int   cSign = 0;

	int   cE = 0;
	int   cEDigits = 0;
	int   cESign = 0;

	int i;

	if (IsToken("NaN", r, it)) {
		SetTokenWhat2(r, it, 3); /* NaN */
		SetTokenWhat(r, it, VPC_NUMERIC);
		return 1;
	}
	if (IsToken("Infinity", r, it)) {
		SetTokenWhat2(r, it, 4); /* Infinity */
		SetTokenWhat(r, it, VPC_NUMERIC);
		return 1;
	}
	if (IsToken("+Infinity", r, it)) {
		SetTokenWhat2(r, it, 4); /* +Infinity */
		SetTokenWhat(r, it, VPC_NUMERIC);
		return 1;
	}
	if (IsToken("-Infinity", r, it)) {
		SetTokenWhat2(r, it, 5); /* -Infinity */
		SetTokenWhat(r, it, VPC_NUMERIC);
		return 1;
	}

	for (i = ixs; i <= ixe; ++i) {
		ch = TokenChar(r, it, i);
		if (isspace(ch) || ch == gchSeparator) continue;
		if (IsDigit(ch)) {
			if (cE == 0) cd++;
			else         cEDigits++;
			continue;
		}
		if (ch == '+' || ch == '-') {
			if (cE == 0) {
				if (cd > 0) return 0;
				if (cSign++ > 0) return 0;
			}
			else {
				if (cEDigits > 0) return 0;
				if (cESign++ > 0) return 0;
			}
		}
		else
			if (ch == 'E' || ch == 'e' || ch == 'D' || ch == 'd') {
				if (cd <= 0)  return 0;
				if (cE++ > 0) return 0;
			}
			else
				if (ch == '.') {
					if (cDot++ > 0) return 0;
					if (cE > 0) return 0;
					if (cd <= 0) return 0;
				}
				else {
					return 0;
				}
	}
	if (cE > 0 && cEDigits == 0) return 0;
	if (cd <= 0)                 return 0;

	if (cE > 0 || cDot > 0) SetTokenWhat2(r, it, 2); /* Floating point number. */
	else                    SetTokenWhat2(r, it, 1); /* Integer number. */
	SetTokenWhat(r, it, VPC_NUMERIC);
	return 1;                         /* Yes numeric(Integer or floating) */
}

static char ReadToken(READER* r, int* ixFrom, int* ixTo)
{
	char ch;
	char chE=0;

	*ixFrom = 0;
	*ixTo   =-1;

	do {
		ch = GetChar(r);
		if (IsEOS(ch)) return ch;
	} while (isspace(ch));
	if (ch == '#') return SkipToEOL(r);

	*ixFrom = r->cBuffer;
	*ixTo   = r->cBuffer;
	PushBuffer(r,ch);

	if (IsDEL(ch)) {
		PushBuffer(r,'\0');
		return ch;
	}

	if (ch == '"' || ch=='\'') {
		char endS = ch;
		PopBuffer(r);
		while (!IsEOL(ch = GetChar(r))) {
			*ixTo = r->cBuffer;
			PushBuffer(r,ch);
			if (ch == endS) { --*ixTo; PopBuffer(r); PushBuffer(r,'\0'); return ch; }
		}
		return ch;
	}

	while(!IsEOS(ch = GetChar(r)) && !isspace(ch)) {
		if (!chE || (ch != '-' && ch != '+' )) {
			if (IsDEL(ch)) { ungetc(ch,r->file); break; }
		}
		chE = 0;
		if (ch == 'E'||ch=='e') chE = 1;
		*ixTo = r->cBuffer;
		PushBuffer(r,ch);
	}
	PushBuffer(r,'\0');
	return ch;
}

static char ReadStatement(READER *r,int *nc)
{
	char ch;
	int   ixs, ixe;

	*nc = 0;
	r->Statements[r->iStatement].start = r->cTokens;
	do {
		ch = ReadToken(r, &ixs, &ixe);
		if (ixs <= ixe) {
			PushToken(r,ixs, ixe);
			*nc += (ixe - ixs) + 1;
		}
	} while (!IsEOS(ch));
	return ch;
}

char ReadLine(READER* r)
{
	char ch;
	int  nc;

	ClearReader(r);
	gfLoop = 0;
	do {
		r->iStatement = r->cStatements;
		ch = ReadStatement(r ,&nc);
		if (nc <= 0) continue;
		PushBuffer(r,'\0');
		if (r->cStatements >= r->mStatements) {
			FATAL(fprintf(stderr, "SYTEM_ERROR: too meny statements(%d)", r->mStatements));
		}
		r->cStatements++;
	} while (!IsEOL(ch));
	return ch;
}
