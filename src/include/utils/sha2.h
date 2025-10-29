/*-------------------------------------------------------------------------
 *
 * sha2.h
 *      PostgreSQL connection pooler and load balancer
 *
 * Copyright (c) 2003-2021 PgPool Global Development Group
 * Copyright (c) 2024-2025, pgElephant, Inc.
 *
 *-------------------------------------------------------------------------
 */
#ifndef _PG_SHA2_H_
#define _PG_SHA2_H_

#include "pool_type.h"
#ifdef USE_SSL_NO_USE
#include <openssl/sha.h>
#endif

/*** SHA224/256/384/512 Various Length Definitions ***********************/
#define PG_SHA224_BLOCK_LENGTH			64
#define PG_SHA224_DIGEST_LENGTH			28
#define PG_SHA224_DIGEST_STRING_LENGTH	(PG_SHA224_DIGEST_LENGTH * 2 + 1)
#define PG_SHA256_BLOCK_LENGTH			64
#define PG_SHA256_DIGEST_LENGTH			32
#define PG_SHA256_DIGEST_STRING_LENGTH	(PG_SHA256_DIGEST_LENGTH * 2 + 1)
#define PG_SHA384_BLOCK_LENGTH			128
#define PG_SHA384_DIGEST_LENGTH			48
#define PG_SHA384_DIGEST_STRING_LENGTH	(PG_SHA384_DIGEST_LENGTH * 2 + 1)
#define PG_SHA512_BLOCK_LENGTH			128
#define PG_SHA512_DIGEST_LENGTH			64
#define PG_SHA512_DIGEST_STRING_LENGTH	(PG_SHA512_DIGEST_LENGTH * 2 + 1)

/* Context Structures for SHA-1/224/256/384/512 */
#ifdef USE_SSL_NO_USE
typedef SHA256_CTX pg_sha256_ctx;
typedef SHA512_CTX pg_sha512_ctx;
typedef SHA256_CTX pg_sha224_ctx;
typedef SHA512_CTX pg_sha384_ctx;
#else
typedef struct pg_sha256_ctx
{
	uint32		state[8];
	uint64		bitcount;
	uint8		buffer[PG_SHA256_BLOCK_LENGTH];
} pg_sha256_ctx;
typedef struct pg_sha512_ctx
{
	uint64		state[8];
	uint64		bitcount[2];
	uint8		buffer[PG_SHA512_BLOCK_LENGTH];
} pg_sha512_ctx;
typedef struct pg_sha256_ctx pg_sha224_ctx;
typedef struct pg_sha512_ctx pg_sha384_ctx;
#endif							/* USE_SSL */

/* Interface routines for SHA224/256/384/512 */
extern void pg_sha224_init(pg_sha224_ctx *ctx);
extern void pg_sha224_update(pg_sha224_ctx *ctx, const uint8 *input0,
							 size_t len);
extern void pg_sha224_final(pg_sha224_ctx *ctx, uint8 *dest);

extern void pg_sha256_init(pg_sha256_ctx *ctx);
extern void pg_sha256_update(pg_sha256_ctx *ctx, const uint8 *input0,
							 size_t len);
extern void pg_sha256_final(pg_sha256_ctx *ctx, uint8 *dest);

extern void pg_sha384_init(pg_sha384_ctx *ctx);
extern void pg_sha384_update(pg_sha384_ctx *ctx,
							 const uint8 *, size_t len);
extern void pg_sha384_final(pg_sha384_ctx *ctx, uint8 *dest);

extern void pg_sha512_init(pg_sha512_ctx *ctx);
extern void pg_sha512_update(pg_sha512_ctx *ctx, const uint8 *input0,
							 size_t len);
extern void pg_sha512_final(pg_sha512_ctx *ctx, uint8 *dest);

#endif							/* _PG_SHA2_H_ */
