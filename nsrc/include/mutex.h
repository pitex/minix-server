#ifndef _MUTEX_H
#define _MUTEX_H

#include <sys/types.h>
#include <lib.h>
#include <unistd.h>

/* Function Prototypes. */
_PROTOTYPE( int mcs_lock, (int mutex_id)				);
_PROTOTYPE( int mcs_unlock, (int mutex_id)				);
_PROTOTYPE( int mcs_wait, (int con_var_id, int mutex_id));
_PROTOTYPE( int mcs_broadcast, (int con_var_id)			);

#endif
