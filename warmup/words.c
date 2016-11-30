#include "common.h"

int
main(int argc, char **argv)
{
	int i;
	//TBD();
	for (i = 1; i < argc; i++){
		printf("%s\n", argv[i]);
	}
	return 0;
}
