// Test.cpp : コンソール アプリケーションのエントリ ポイントを定義します。
//

#include "stdafx.h"
#include <limits.h>

#include "bigdecimal.h"

int main(int argc, char* argv[])
{
	VP_HANDLE h,h1;
	char sz[1000];
	char *psz;
	unsigned long long V;
	unsigned long long v = 1000000000ULL; // 64-bit BASE
	unsigned long      uv = 10000L;       // 32-bit BASE

	printf("ULONG_MAX=%lu\n",ULONG_MAX);
	printf("v        =%llu\n",v);
	printf("uv*uv+uv =%llu\n",((unsigned long long)uv)*((unsigned long long)uv)+uv);
	printf("ULONG_MAX=%llu\n",ULLONG_MAX);
	printf("v*v+v    =%llu\n",v*v+v);
	printf("sizeof=%d\n",sizeof(unsigned long long));



	h  = VpAlloc("1245678901234567890.12345678901234567890",0);
	h1 = VpAlloc("12456",0);
	printf("Asign=");VpPrintE(stdout,VpAsgn(h,h1,1));printf("\n");


	printf("%s\n",psz=VpToStringF(h,sz));
	printf("max=%d, len=%d\n",VpStringLengthF(h),strlen(psz));
	printf("%s\n",psz=VpToStringE(h,sz));
	printf("max=%d, len=%d\n",VpStringLengthE(h),strlen(psz));
	printf("\n");






	VpPrintE(stdout,VpAlloc("0 . 000012 3456 7890123456 7890123456 7890 E6",10));printf("\n");
	VpPrintE(stdout,VpAlloc("0.0000123456789012345678 901234567890E5",10));printf("\n");
	VpPrintE(stdout,VpAlloc("0.000012345678 9012345678901234567890E4",10));printf("\n");
	VpPrintE(stdout,VpAlloc("0.000012345678901 2345678901234567890E3",10));printf("\n");
	VpPrintE(stdout,VpAlloc("- 1234567.89012345678901234567890e+333",10));printf("\n");
	VpPrintE(stdout,VpAlloc("123456.7890123456789012345 67890e+333",10));printf("\n");
	VpPrintE(stdout,VpAlloc("-12345 .67890123 45678901234567890e+333",10));printf("\n");
	VpPrintE(stdout,VpAlloc("1234. 56789012345678901234567890e+333",10));printf("\n");
    printf("Allocs=%d\n",VpAllocCount());

	getchar();
	return 0;
}

