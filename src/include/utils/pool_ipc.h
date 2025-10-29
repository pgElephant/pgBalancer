/*-------------------------------------------------------------------------
 *
 * pool_ipc.h
 *      PostgreSQL connection pooler and load balancer
 *
 * Copyright (c) 2003-2021 PgPool Global Development Group
 * Copyright (c) 2024-2025, pgElephant, Inc.
 *
 *-------------------------------------------------------------------------
 */
#ifndef IPC_H
#define IPC_H

#define IPCProtection	(0600)	/* access/modify by user only */

extern void shmem_exit(int code);
extern void on_shmem_exit(void (*function) (int code, Datum arg), Datum arg);
extern void on_exit_reset(void);

/*pool_sema.c*/
extern void pool_semaphore_create(int numSems);
extern void pool_semaphore_lock(int semNum);
extern int	pool_semaphore_lock_allow_interrupt(int semNum);
extern void pool_semaphore_unlock(int semNum);

#endif							/* IPC_H */
