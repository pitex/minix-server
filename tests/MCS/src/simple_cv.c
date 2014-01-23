#include <stdio.h>
#include <errno.h>
#include <string.h>

#include <mcs.h>

#include "utils.h"

#define MUTEX 1
#define CV 1

#define MAX_DELAY 30

int main(int argc, char* argv[]){

	int res,pdelay;

	pdelay=0;
	if (argc==2){
		pdelay=atoi(argv[1]);
		if (pdelay<0 || pdelay > MAX_DELAY){
			fail("wrong delay value");
		}
	}

        report("p lock:",mcs_lock(MUTEX));

	if (fork()){
		/* parent */
		if (pdelay) sleep(pdelay);
		report("p wait:", mcs_wait(CV,MUTEX));
		report("p unlock:",mcs_unlock(MUTEX));
		wait(NULL);
		exit(0);
	} else {
		report("ch lock:",mcs_lock(MUTEX));
		report("ch broadcast:",mcs_broadcast(CV));
		report("ch unlock:",mcs_unlock(MUTEX));
		exit(0);
	}
}
