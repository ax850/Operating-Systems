#include "common.h"

int fact(int n){
	if (n >= 1){
		return n * fact(n-1);
	}
	else
		return 1;
	
}

int
main(int argc, char **argv)
{
	if (argc == 1){
		return 0;
	}
	float num1 = atof(argv[1]);
	int num2 = atoi(argv[1]);
	if(num2 > 12){
	  printf ("Overflow\n");
	  return 0;
	}
	
	else if (num1*num1 == num2*num2 && num2 > 0){
		printf("%d\n", fact(num2));
	}
	
	else {
		printf ("Huh?\n");
	}
	return 0;
}
