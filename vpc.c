/*
 * Variable precision calculator.
 * (Test program for Bigdecimal(Variable decimal precision) C/C++ library.)
 *
 *  Copyright(C) 2024 by Shigeo Kobayashi(shigeo@tinyforest.jp)
 *
 */

#include "vpc.h"
#include <signal.h>

int        gcError   = 0; /* flag for error(error counter in a line) */
int        gfQuit    = 0; /* flag for 'quit' */
int        gfBreak   = 0; /* flag for 'break' */
int        gfLoop    = 0; /* 1 if in a loop like while ... */
int        gfInFile  = 0; /* 1 if the input file != stdin  */

void InitVpc(int cInput,int cToken)
{
}

void FinishVpc(int e)
{
	exit(e);
}

void ClearGlobal()
{
	gcError = 0;
	gfQuit  = 0;
	gfBreak = 0;
}

void MyException(VP_HANDLE h, const char* msg)
{
	ERROR(fprintf(stderr,"Error: %s\n", msg));
}


void CtrlC(int signal);
void SetCTRLC()
{
	signal(SIGINT, CtrlC);
}
void CtrlC(int signal)
{
    ERROR(;);
	SetCTRLC();
}

int main(int argc, char* argv[])
{
	FILE* fIni;

	printf("\nVPC(Variable Precision Calculator V2.1) of Bigdecimal(V%d)\n",VpVersion());
	printf("  Copyright (c) 2024 by Shigeo Kobayashi. Allrights reserved.\n");
	printf("  Enter ?? for help.\n");

	SetCTRLC();
	VpSetExceptionHandler(MyException);

	InitVpc(1024,512);

	fIni = fopen("./vpc.ini", "r");
	if (fIni != NULL) {
		char ch=0;
		printf("\n vpc.ini found, read it ");
		while (ch != 'y' && ch != 'n') {
			if(ch!='\n') printf("(y/n) ?");
			ch = getchar();
		}
		if (ch == 'y') {
			printf("\nreading vpc.ini ...");
			ReadAndExecuteLines(fIni);
		}
		fclose(fIni);
	}

	printf("\nEnter command");
	ReadAndExecuteLines(stdin);
	FinishVpc(0);
	return 0;
}
