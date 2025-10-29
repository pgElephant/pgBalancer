/*-------------------------------------------------------------------------
 *
 * scram.h
 *      PostgreSQL connection pooler and load balancer
 *
 * Copyright (c) 2003-2021 PgPool Global Development Group
 * Copyright (c) 2024-2025, pgElephant, Inc.
 *
 *-------------------------------------------------------------------------
 */
#ifndef PG_SCRAM_H
#define PG_SCRAM_H

/* Name of SCRAM-SHA-256 per IANA */
#define SCRAM_SHA_256_NAME "SCRAM-SHA-256"

/* Status codes for message exchange */
#define SASL_EXCHANGE_CONTINUE		0
#define SASL_EXCHANGE_SUCCESS		1
#define SASL_EXCHANGE_FAILURE		2

/* Routines dedicated to authentication */
extern void *pg_be_scram_init(const char *username, const char *shadow_pass);
extern int	pg_be_scram_exchange(void *opaq, char *input, int inputlen,
								 char **output, int *outputlen, char **logdetail);

/* Routines to handle and check SCRAM-SHA-256 verifier */
extern char *pg_be_scram_build_verifier(const char *password);
extern bool scram_verify_plain_password(const char *username,
										const char *password, const char *verifier);
extern void *pg_fe_scram_init(const char *username, const char *password);
extern void pg_fe_scram_exchange(void *opaq, char *input, int inputlen,
								 char **output, int *outputlen,
								 bool *done, bool *success);
extern void pg_fe_scram_free(void *opaq);
extern char *pg_fe_scram_build_verifier(const char *password);

#endif							/* PG_SCRAM_H */
