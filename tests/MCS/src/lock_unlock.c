#include <stdio.h>
#include <errno.h>
#include <string.h>

#include <mcs.h>

#include "utils.h"

int main(int argc, char* argv[]){

	int res;

        res=mcs_lock(1);
	printf("res:%d\n", res);
	if (res)
		printf("errno:%d \n", errno);

 	res= mcs_unlock(2);
	printf("res:%d\n", res);
	if (res)
		printf("errno:%d \n", errno);
	
 	res= mcs_unlock(1);
	printf("res:%d\n", res);
	if (res)
		printf("errno:%d \n", errno);
	
 	res= mcs_unlock(1);
	printf("res:%d\n", res);
	if (res)
		printf("errno:%d \n", errno);
	
	return 0;
}
