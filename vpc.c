/*
 * Variable precision calculator.
 * (Test program for Bigdecimal(Variable decimal precision) C/C++ library.)
 *
 *  Copyright(C) 2024 by Shigeo Kobayashi(shigeo@tinyforest.jp)
 *
 */

#include "vpc.h"
#include <signal.h>

void InitVpc(int cInput,int cToken)
{
	InitReader(cInput, cToken);
	InitParser();
	InitCalculator();
}

void FinishVpc(int e)
{
	FinishCalculator();
	FinishParser();
	FinishReader();
	exit(e);
}

void MyException(VP_HANDLE h, const char* msg)
{
	ERROR(fprintf(stderr,"Error: %s\n", msg));
}

/*
void CtrlC(int signal)
{
	printf("\n\'CTRL-C\' pressed. To quit the program, enter \'y\'(any other to continue)? ");
	if(getchar()=='y') FinishVpc(signal);
}
*/

int main(int argc, UCHAR* argv[])
{
	FILE* fIni;

	printf("\nVPC(Variable Precision Calculator V2) of Bigdecimal(V%d)\n",VpVersion());
	printf("  Copyright (c) 2024 by Shigeo Kobayashi. Allrights reserved.\n");

/*
	signal(SIGINT, CtrlC);
*/
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
