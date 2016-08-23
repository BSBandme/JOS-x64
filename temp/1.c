#include <stdio.h>

void test(int deep, long long b ) {
	 if(deep != 0) 
		 test(deep - 1, b);
	 else 
		 return ;
}

int main()  {
	test(2, 1) ;
	return 0;
}
