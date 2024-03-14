/*
 * Variable precision calculator.
 * (Test program for Bigdecimal(Variable decimal precision) C/C++ library.)
 *
 *  Copyright(C) 2018 by Shigeo Kobayashi(shigeo@tinyforest.jp)
 *
 *    Version 1.1   ... VpLoad() added.
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
	printf("\nVPC(Variable Precision Calculator V2) of Bigdecimal(V%d)\n",VpVersion());
	printf("  Copyright (c) 2024 by Shigeo Kobayashi. Allrights reserved.\n\n");
	printf("Enter command");

/*
	signal(SIGINT, CtrlC);
*/
	VpSetExceptionHandler(MyException);

	InitVpc(1024,512);
	ReadLines(stdin);
	FinishVpc(0);
	return 0;
}
