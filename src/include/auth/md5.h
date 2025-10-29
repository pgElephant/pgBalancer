/*-------------------------------------------------------------------------
 *
 * md5.h
 *      PostgreSQL connection pooler and load balancer
 *
 * Copyright (c) 2003-2021 PgPool Global Development Group
 * Copyright (c) 2024-2025, pgElephant, Inc.
 *
 *-------------------------------------------------------------------------
 */
#ifndef MD5_H
#define MD5_H

#define MD5_PASSWD_LEN 32

#define WD_AUTH_HASH_LEN 64

extern int	pool_md5_hash(const void *buff, size_t len, char *hexsum);
extern int	pool_md5_encrypt(const char *passwd, const char *salt, size_t salt_len, char *buf);
extern void bytesToHex(char *b, int len, char *s);
extern bool pg_md5_encrypt(const char *passwd, const char *salt, size_t salt_len, char *buf);

#endif
