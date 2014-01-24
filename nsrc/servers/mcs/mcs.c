/* System Information Service. 
 * This service handles the various debugging dumps, such as the process
 * table, so that these no longer directly touch kernel memory. Instead, the 
 * system task is asked to copy some table in local memory. 
 * 
 * Created:
 *	 Apr 29, 2004	by Jorrit N. Herder
 */

#include "mcs.h"

/* Set debugging level to 0, 1, or 2 to see no, some, all debug output. */
#define DEBUG_LEVEL	1
#define DPRINTF		if (DEBUG_LEVEL > 0) printf

struct mutex {
    int pid;
    int nr;
};

struct cond_var {
    int pid;
    int cond_id;
    int mtx_id;
};

/* Allocate space for the global variables. */
message m_in;		/* the input message itself */
message m_out;		/* the output message used for reply */
int who;		/* caller's proc number */
int callnr;		/* system call number */

struct mutex mutexes[MUTEX_LIMIT];
struct mtx_queue[NR_PROCS];
int mtx_que_size;
int mtx_size;

struct cond_var proc_waiting[NR_PROCS];
int pr_wait_size;

extern int errno;	/* error number set by system library */

/* Declare some local functions. */
FORWARD _PROTOTYPE(void init_server, (int argc, char **argv)                );
FORWARD _PROTOTYPE(void exit_server, (void)                                 );
FORWARD _PROTOTYPE(void get_work, (void)                                    );
FORWARD _PROTOTYPE(void reply, (int whom, int result)                       );
FORWARD _PROTOTYPE(void mutex_lock, (int mutex_id, int who)                 );
FORWARD _PROTOTYPE(void mutex_unlock, (int mutex_id, int who)               );
FORWARD _PROTOTYPE(void mutex_wait, (int mutex_id, int cond_var_id, int who));
FORWARD _PROTOTYPE(void mutex_broadcast, (int cond_var_id, int who)         );
FORWARD _PROTOTYPE(void mutex_unpause, (int who)                            );
FORWARD _PROTOTYPE(void mutex_terminate, (int who)                          );
FORWARD _PROTOTYPE(void move_waiting, (int start)                           );
FORWARD _PROTOTYPE(void move_mutexes, (int start)                           );
FORWARD _PROTOTYPE(void move_queue, (int start)                             );
FORWARD _PROTOTYPE(int find_mutex, (int mutex_id)                           );
FORWARD _PROTOTYPE(int find_mutex, (int mutex_id, int who)                  );
FORWARD _PROTOTYPE(int waiting_for, (int mutex_id)                          );

/*===========================================================================*
 *				main														 *
 *===========================================================================*/
PUBLIC int main(int argc, char **argv)
{
/* This is the main routine of this service. The main loop consists of 
 * three major activities: getting new work, processing the work, and
 * sending the reply. The loop never terminates, unless a panic occurs.
 */
	int result;								 
	sigset_t sigset;

	/* Initialize the server, then go to work. */
	init_server(argc, argv);

	/* Main loop - get work and do it, forever. */				 
	while (TRUE) {							
		/* Wait for incoming message, sets 'callnr' and 'who'. */
		get_work();
		switch (callnr) {
		case SYS_SIG:
			sigset = (sigset_t) m_in.NOTIFY_ARG;
			if (sigismember(&sigset,SIGTERM) || sigismember(&sigset,SIGKSTOP)) {
				exit_server();
			}
			continue;
		case MCS_LOCK:
            mutex_lock(m_in.m_u.m_m1.m1i1, who);
			break;
        case MCS_UNLOCK:
            mutex_unlock(m_in.m_u.m_m1.m1i1, who);
            break;
        case MCS_WAIT:
            mutex_wait(m_u.m_m1.m1i1, m_u.m_m1.m1i2, who);
            break;
        case MCS_BROADCAST:
            mutex_broadcast(m_u.m_m1.m1i1, who);
            break;
        case MCS_UNPAUSE:
            reply(who, 0);
            mutex_unpause(m_u.m_m1.m1i1);
            break;
        case MCS_TERM:
            reply(who, 0);
            mutex_terminate(m_u.m_m1.m1i1);
            break;
		default: 
			report("MCS","warning, got illegal request from %d\n", m_in.m_source);
			result = EINVAL;
		}
		/* Finally send reply message, unless disabled. */
		if (result != EDONTREPLY) {
            reply(who, result);
		}
	}
	return(OK);				/* shouldn't come here */
}

/*===========================================================================*
 *				 init_server											     *
 *===========================================================================*/
PRIVATE void init_server(int argc, char **argv)
{
/* Initialize the information service. */
	int fkeys, sfkeys;
	int i, s;
#if DEAD_CODE
	struct sigaction sigact;

	/* Install signal handler. Ask PM to transform signal into message. */
	sigact.sa_handler = SIG_MESS;
	sigact.sa_mask = ~0;			/* block all other signals */
	sigact.sa_flags = 0;			/* default behaviour */
	if (sigaction(SIGTERM, &sigact, NULL) < 0) 
			report("MCS","warning, sigaction() failed", errno);
#endif
}

/*===========================================================================*
 *				exit_server													 *
 *===========================================================================*/
PRIVATE void exit_server()
{
	/* Done. Now exit. */
	exit(0);
}

/*===========================================================================*
 *				get_work													 *
 *===========================================================================*/
PRIVATE void get_work()
{
	int status = 0;
	status = receive(ANY, &m_in);	 /* this blocks until message arrives */
	if (OK != status) {
		panic("MCS","failed to receive message!", status);
    }
	who = m_in.m_source;				/* message arrived! set sender */
	callnr = m_in.m_type;			 /* set function call number */
}

/*===========================================================================*
 *				reply							                             *
 *===========================================================================*/
PRIVATE void reply(who, result)
int who;													 	/* destination */
int result;													 	/* report result to replyee */
{
	int send_status;
	m_out.m_type = result;			/* build reply message */
	send_status = send(who, &m_out);		/* send the message */
	if (OK != send_status) {
		panic("MCS", "unable to send reply!", send_status);
    }
}

/*===========================================================================*
 *              mutex_lock                                                   *
 *===========================================================================*/
PRIVATE void mutex_lock(mutex_id, who)
int mutex_id;
int who;
{
    int i;
    int locked;

    /*  Check if mutex is already locked    */
    locked = find_mutex(mutex_id);

    if (locked != -1) {
        mtx_queue[mtx_que_size].nr = mutex_id;
        mtx_queue[mtx_que_size].pid = who;
        mtx_que_size++;
    } else {
        mutexes[mtx_size].nr = mutex_id;
        mutexes[mtx_size].pid = who;
        mtx_size++;
        reply(who, 0);
    }
}

/*===========================================================================*
 *              mutex_unlock                                                 *
 *===========================================================================*/
PRIVATE void mutex_unlock(mutex_id, who)
int mutex_id;
int who;
{
    int i;
    int found;
    int next;

    /*  Check if mutex is locked, found equals place in mutexes where mutex is located  */
    found = find_mutex(mutex_id, who);

    /*  If mutex is not locked  */
    if (found == -1) {
        reply(who, EPERM);
        return;
    }

    /*  Check if someone is waiting for unlocked mutex  */
    next = waiting_for(mutex_id);

    /*  If someone is waiting   */
    if (next != -1) {
        mutexes[found].pid = mtx_queue[next].pid;

        /*  Rewrite waiting queue   */
        move_queue(next);

        reply(mutexes[found].pid, 0);
    } else {
        /*  Rewrite mutexes */
        move_mutexes(found);
    }

    reply(who, 0);
}

/*===========================================================================*
 *              mutex_wait                                                   *
 *===========================================================================*/
PRIVATE void mutex_wait(mutex_id, cond_var_id, who)
int mutex_id;
int cond_var_id;
int who;
{
    int i;
    int found;

    found = find_mutex(mutex_id, who);

    if (found == -1) {
        reply(who, EINVAL);
        return;
    }

    mutex_unlock(mutex_id, who);

    proc_waiting[pr_wait_size].mtx_id = mutex_id;
    proc_waiting[pr_wait_size].cond_id = cond_var_id;
    proc_waiting[pr_wait_size].pid = who;
}

/*===========================================================================*
 *              mutex_broadcast                                              *
 *===========================================================================*/
PRIVATE void mutex_broadcast(cond_var_id, who)
int cond_var_id;
int who;
{
    int i;

    i=0;
    while (i<pr_wait_size) {
        if (proc_waiting[i].cond_id == cond_var_id) {
            mutex_lock(proc_waiting[i].mtx_id, proc_waiting[i].pid);
            move_waiting(i);
        } else {
            i++;
        }
    }

    reply(who, 0);
}

/*===========================================================================*
 *              mutex_unpause                                                *
 *===========================================================================*/
PRIVATE void mutex_unpause(who)
int who;
{
    int i;

    for (i=0; i<mtx_que_size; i++) {
        if (mtx_queue[i].pid == who) {
            move_queue(i);

            reply(who, EINTR);

            return;
        }
    }

    for (i=0; i<pr_wait_size; i++) {
        if (proc_waiting[i].pid == who) {
            move_waiting(i);

            reply(who, EINTR);

            return;
        }
    }
}

/*===========================================================================*
 *              mutex_terminate                                              *
 *===========================================================================*/
PRIVATE void mutex_terminate(who)
int who;
{
    int i;
    int next;

    /*  Unlock mutexes  */
    while (i < mtx_size) {
        if (mutexes[i].pid == who) {
            next = waiting_for(mutexes[i].nr);

            if (next != -1) {
                mutexes[i].pid = mtx_queue[next].pid;
                move_queue(next);
                reply(mutexes[i].pid, 0);
            } else {
                move_mutexes(i);
            }
        } else {
            i++;
        }
    }

    /*  Remove from queue   */
    while (i<mtx_que_size) {
        if (mtx_queue[i].pid == who) {
            move_queue(i);
        } else {
            i++;
        }
    }

    /*  Remove from waiting */
    while (i<pr_wait_size) {
        if (proc_waiting[i].pid == who) {
            move_waiting(i);
        } else {
            i++;
        }
    }
}
 
/*===========================================================================*
 *              misc functions                                               *
 *===========================================================================*/
PRIVATE void move_mutexes(start) 
int start;
{
    int i;

    mtx_size--;

    for (i=start; i<mtx_size; i++) {
        mutexes[i].nr = mutexes[i+1].nr;
        mutexes[i].pid = mutexes[i+1].pid;
    }
}

PRIVATE void move_queue(start)
int start;
{
    int i;

    mtx_que_size--;

    for (i=start; i<mtx_que_size; i++) {
        mtx_queue[i].nr = mtx_queue[i+1].nr;
        mtx_queue[i].pid = mtx_queue[i+1].pid;
    }
}

PRIVATE void move_waiting(start)
int start;
{
    int i;

    pr_wait_size--;

    for (i=start; i<pr_wait_size; i++) {
        proc_waiting[i].pid = proc_waiting[i+1].pid;
        proc_waiting[i].mtx_id = proc_waiting[i+1].mtx_id;
        proc_waiting[i].cond_id = proc_waiting[i+1].cond_id;
    }
}

PRIVATE int find_mutex(mutex_id) 
int mutex_id;
{
    for (int i=0; i<mtx_size; i++) {
        if (mutexes[i].nr == mutex_id) {
            return i;
        }
    }

    return -1;
}

PRIVATE int find_mutex(mutex_id, who) 
int mutex_id;
int who;
{
    for (int i=0; i<mtx_size; i++) {
        if (mutexes[i].nr == mutex_id && mutexes[i].pid == who) {
            return i;
        }
    }

    return -1;
}

PRIVATE int waiting_for(mutex_id)
int mutex_id;
{
    for (int i=0; i<mtx_que_size; i++) {
        if (mtx_queue[i].nr == mutex_id) {
            return i;
        }
    }

    return -1;
}