/*-------------------------------------------------------------------------
 *
 * base64.h
 *      PostgreSQL connection pooler and load balancer
 *
 * Copyright (c) 2003-2021 PgPool Global Development Group
 * Copyright (c) 2024-2025, pgElephant, Inc.
 *
 *-------------------------------------------------------------------------
 */
#ifndef BASE64_H
#define BASE64_H

/* base 64 */
extern int	pg_b64_encode(const char *src, int len, char *dst);
extern int	pg_b64_decode(const char *src, int len, char *dst);
extern int	pg_b64_enc_len(int srclen);
extern int	pg_b64_dec_len(int srclen);

#endif							/* BASE64_H */
