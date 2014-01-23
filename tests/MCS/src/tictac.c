#include <stdio.h>
#include <errno.h>
#include <string.h>

#include <mcs.h>

#include "utils.h"

#define MUTEX 1
#define CV 1

#define MAX_DELAY 30

int main(int argc, char* argv[]){

	int res,turns,i;

	char * text;

	if (argc!=2){
		fail("one argument needed");
	}

	turns=atoi(argv[1]);
	if (turns<0 || turns > MAX_DELAY){
		fail("wrong number of turns");
	}
	

        report("p lock:",mcs_lock(MUTEX));

	if (fork()){
		/* parent */
		for (i=0;i< turns; i++){
			mcs_wait(CV,MUTEX);
			write(1,"TI",2);
			sleep(1);
			write(1,"C\n",2);
			mcs_broadcast(CV);
		}
		mcs_unlock(MUTEX);
		wait(NULL);
		exit(0);
	} else {
		mcs_lock(MUTEX);
		mcs_broadcast(CV);
		for (i=0;i< turns; i++){
			mcs_wait(CV,MUTEX);
			write(1,"TA",2);
			sleep(1);
			write(1,"C\n",2);
			mcs_broadcast(CV);
		}
		mcs_unlock(MUTEX);
		exit(0);
	}
}
