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

int     gcError = 0; /* flag for error */

UCHAR*     gInputBuffer  = NULL;  /* read buffer */
int        gmInputBuffer = 0;     /* Max buffer character size*/
int        gcInputBuffer = 0;     /* character size read to gInputBuffer */

TOKEN*     gTokens       = NULL;  /* Token buffer */
int        gmTokens      = 0;     /* Max tokens */
int        gcTokens      = 0;     /* Total token counter */

STATEMENT* gStatements   = NULL;  /* Token count read to gTokens of 'one' statement */
int        gmStatements  = 0;
int        gcStatements  = 0;     /* Current statement processed    */

UCHAR      gDelimiters[] = { '?' , '(', ')' ,  '=' , '*' , '/' , '+' , '-' , ',' , ';' };
int        gmDelimiters  = sizeof(gDelimiters)/sizeof(gDelimiters[0]);

int        gnRepeat; /* repeat counter */

void InitReader(int buffer_size,int token_size)
{
	if (buffer_size > gmInputBuffer) {
		gmInputBuffer = buffer_size;
		if (gInputBuffer != NULL) {
			free(gInputBuffer);
			gInputBuffer = NULL;
		}
		gInputBuffer = malloc(gmInputBuffer+1);
	}
	if (token_size > gmTokens) {
		gmTokens = token_size;
		if (gTokens != NULL) {
			free(gTokens);
			gTokens = NULL;
		}
		gTokens = calloc(gmTokens+1,sizeof(TOKEN));
		if (gStatements != NULL) free(gStatements);
		gmStatements     = gmTokens / 2 + 1;
		gStatements      = calloc(sizeof(STATEMENT), gmStatements + 1);
	}

	if (gInputBuffer == NULL || gTokens == NULL || gStatements == NULL) {
		gcError++; fprintf(stderr,"SYSTEM ERROR: Buffer memory allocation failed(tried to allocate character=%d,token=%d\n)", gmInputBuffer,gmTokens);
		FinishVpc(-1);
	}
}

void FinishReader()
{
	if (gInputBuffer != NULL) free(gInputBuffer);
	if (gTokens != NULL)      free(gTokens);
	if (gStatements != NULL)  free(gStatements);
	gTokens       = NULL;
	gStatements   = NULL;
	gInputBuffer  = NULL;
	gmTokens      = 0;
	gmStatements  = 0;
	gmInputBuffer = 0;
}

#ifdef _DEBUG
static void PrintTokens()
{
	int i, j;

	for (i = 0; i < gcStatements; ++i) {
		printf("\n %d:[", gStatements[i].end - gStatements[i].start+1);
		for (j = 0; j<= gStatements[i].end - gStatements[i].start; ++j) {
			if (j != 0) printf(",");
			printf("%s", TokenPTR(i,j));
		}
		printf("]\n");
	}
}
#endif

static UCHAR IsDEL(UCHAR ch)
{
	int i;
	for (i = 0; i < gmDelimiters; ++i)
	{
		if (ch == gDelimiters[i]) return ch;
	}
	return 0;
}

static int IsEOL(UCHAR ch)
{
	return (ch == (UCHAR)EOF) || (ch == '\n');
}

static int IsEOS(UCHAR ch)
{
	return (ch == ';') || IsEOL(ch);
}

static int IsEOF(UCHAR ch)
{
	return ((ch == (UCHAR)EOF) || (ch == 0x1A));
}

static UCHAR GetChar(FILE* f)
{
	UCHAR ch = getc(f);
	if (ch == 0x1A) ch = EOF;
	return ch;
}

static UCHAR SkipToEOL(FILE *f)
{
	UCHAR ch;
	while (!IsEOL(ch = GetChar(f)));
	return ch;
}

static void PushBuffer(int ch)
{
	if (gcInputBuffer >= gmInputBuffer) {
		gcError++; fprintf(stderr, "SYSTEM_ERROR:String buffer overflowed(%d)\n", gmInputBuffer);
		FinishVpc(-1);
	}
	gInputBuffer[gcInputBuffer++] = ch;
	gInputBuffer[gcInputBuffer] = '\0';
}

static UCHAR PopBuffer()
{
	if (--gcInputBuffer < 0) {
		gcError++; fprintf(stderr, "SYSTEM_ERROR:String buffer underflowed.");
		FinishVpc(-1);
	}
	return gInputBuffer[gcInputBuffer];
}

static void PushToken(int iStatement,int ixs, int ixe)
{
	if (gcTokens >= gmTokens) {
		gcError++; fprintf(stderr, "SYSTEM_ERROR:Token buffer overflowed(%d)\n", gmTokens);
		FinishVpc(-1);
	}
	gTokens[gcTokens].start     = ixs;
	gTokens[gcTokens].end       = ixe;
	gStatements[iStatement].end = gcTokens++;
}

int TokenCount(int iStatement)
{
	return gStatements[iStatement].end- gStatements[iStatement].start + 1;
}

int TokenFrom(int iStatement,int it)
{
	return gTokens[gStatements[iStatement].start +it].start;
}

int TokenTo(int iStatement,int it)
{
	return gTokens[gStatements[iStatement].start + it].end;
}

int TokenSize(int iStatement,int it)
{
	return gTokens[gStatements[iStatement].start + it].end - gTokens[gStatements[iStatement].start + it].start + 1;
}


UCHAR TokenChar(int iStatement,int it, int ic)
{
	return gInputBuffer[gTokens[gStatements[iStatement].start + it].start + ic];
}

UCHAR* TokenPTR(int iStatement,int it)
{
	return &(gInputBuffer[gTokens[gStatements[iStatement].start + it].start]);
}

int TokenWhat(int iStatement,int it)
{
	return gTokens[gStatements[iStatement].start + it].what;
}

void SetTokenWhat(int iStatement,int it, int w)
{
	gTokens[gStatements[iStatement].start + it].what = w;
}

int TokenPriority(int iStatement,int it)
{
	return gTokens[gStatements[iStatement].start + it].etc;
}

void SetTokenPriority(int iStatement,int it, int p)
{
	gTokens[gStatements[iStatement].start + it].etc = p;
}

int TokenWhat2(int iStatement,int it)
{
	return gTokens[gStatements[iStatement].start + it].etc;
}

void SetTokenWhat2(int iStatement,int it, int w)
{
	gTokens[gStatements[iStatement].start + it].etc = w;
}

static UCHAR ReadToken(FILE* f, int* ixFrom, int* ixTo)
{
	UCHAR ch;
	UCHAR chE=0;

	*ixFrom = 0;
	*ixTo   =-1;

	do {
		ch = GetChar(f);
		if (IsEOS(ch)) return ch;
	} while (isspace(ch));
	if (ch == '#') return SkipToEOL(f);

	*ixFrom = gcInputBuffer;
	*ixTo   = gcInputBuffer;
	PushBuffer(ch);

	if (IsDEL(ch)) {
		PushBuffer('\0');
		return ch;
	}

	if (ch == '"' || ch=='\'') {
		UCHAR endS = ch;
		PopBuffer();
		while (!IsEOL(ch = GetChar(f))) {
			*ixTo = gcInputBuffer;
			PushBuffer(ch);
			if (ch == endS) { --*ixTo; PopBuffer(); PushBuffer('\0'); return ch; }
		}
		return ch;
	}

	while(!IsEOS(ch = GetChar(f)) && !isspace(ch)) {
		if (!chE || (ch != '-' && ch != '+' )) {
			if (IsDEL(ch)) { ungetc(ch, f); break; }
		}
		chE = 0;
		if (ch == 'E'||ch=='e') chE = 1;
		*ixTo = gcInputBuffer;
		PushBuffer(ch);
	}
	PushBuffer('\0'); /* Token del. in input character buffer */
	return ch;
}

static UCHAR ReadStatement(FILE* f,int iStatement,int *nc)
{
	UCHAR ch;
	int   ixs, ixe;

	*nc = 0;
	gStatements[iStatement].start = gcTokens;
	do {
		ch = ReadToken(f, &ixs, &ixe);
		if (ixs <= ixe) {
			PushToken(iStatement,ixs, ixe);
			*nc += (ixe - ixs) + 1;
		}
	} while (!IsEOS(ch));
	return ch;
}

void DoRepeat(UCHAR* repeat)
{
	UCHAR ch;
	int   v;

	if (gnRepeat == 0) {
		if (isspace(*repeat)) repeat++;
		if (*repeat == '+')   repeat++;
		while (ch = *repeat++) {
			v = ch - '0';
			if (v < 0 || v>9) goto Error;
			gnRepeat = gnRepeat * 10 + v;
		}
	}
	else {
		gcError++; fprintf(stderr, "Error: \'repeat\' can not be nested(execution interrupted.).\n");
	}
	return;

Error:
	gcError++; fprintf(stderr, "Error: invalid character(%c), integer number expected.\n", ch);
	gnRepeat = 0;
	gcError++;
	return;
}

static UCHAR ReadLine(FILE* f)
{
	UCHAR ch;
	int   nc;

	gcError                         = 0;
	gnRepeat                        = 0;
	gcInputBuffer                   = 0;
	gcTokens                        = 0;
	gcStatements                    = 0;
	gStatements[gcStatements].start = 0;
	gStatements[gcStatements].end   = 0;

	do {
		ch = ReadStatement(f, gcStatements ,&nc);
		if (nc <= 0) continue;

		PushBuffer('\0');
		if (gcStatements >= gmStatements) {
			fprintf(stderr, "SYTEM_ERROR: too meny statements(%d)", gmStatements);
			FinishVpc(-1);
		}
		ParseStatement (gcStatements);
		gcStatements++;
		gStatements[gcStatements].start = 0;
		gStatements[gcStatements].end   = 0;
	} while (!IsEOL(ch));
#ifdef _DEBUG
	PrintTokens();
#endif

	if (gcError == 0) {
		int i;
		for (i = 0; i < gcStatements; ++i) {
			gnRepeat = 0;
			if (ExecuteStatement(i) < 0) { ch = EOF; break; }
			if (gnRepeat > 0) {
				int j;
				while (--gnRepeat > 0) {
					for (j = i + 1; j < gcStatements; ++j) {
						if (ExecuteStatement(j) < 0) { ch = EOF; break; }
						if (gcError > 0) break;
					}
				}
			}
			if (IsEOF(ch) || gcError > 0) break;
		}
	}
	gnRepeat = 0;
	return ch;
}


void ReadLines(FILE* f)
{
	UCHAR ch='\n';
	do {
		if (IsEOL(ch) && f == stdin) printf("\n>");
		ch = ReadLine(f);
	} while (!IsEOF(ch));
}
