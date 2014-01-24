#include "mutex.h"

int mcs_lock(mutex_id)
int mutex_id;
{
	message m;

	m.m_u.m_m1.m1i1 = mutex_id;
	
	return _syscall(MCS_PROC_NR, MCS_LOCK, &m);
	
	if(result == -1) {
		errno = EINTR;
	}
	
	return result;
}

int mcs_unlock(mutex_id)
int mutex_id;
{
	message m;
	
	m.m_u.m_m1.m1i1 = mutex_id;
	
	return _syscall(MCS_PROC_NR, MCS_UNLOCK, &m);
}

int mcs_wait(con_var_id, mutex_id)
int con_var_id;
int mutex_id;
{
	message m;
	
	m.m_u.m_m1.m1i1 = mutex_id;
	m.m_u.m_m1.m1i2 = con_var_id;
	
	return _syscall(MCS_PROC_NR, MCS_WAIT, &m);
	
	if(result == -1) {
		errno = EINVAL;
	}

	return result;
}

int mcs_broadcast(con_var_id)
int con_var_id;
{
	message m;
	
	m.m_u.m_m1.m1i1 = con_var_id;
	
	return _syscall(MCS_PROC_NR, MCS_BROADCAST, &m);

	return result;
}
