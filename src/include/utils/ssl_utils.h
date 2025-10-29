/*-------------------------------------------------------------------------
 *
 * ssl_utils.h
 *      PostgreSQL connection pooler and load balancer
 *
 * Copyright (c) 2003-2021 PgPool Global Development Group
 * Copyright (c) 2024-2025, pgElephant, Inc.
 *
 *-------------------------------------------------------------------------
 */
#ifndef SSL_UTILS_H
#define SSL_UTILS_H

extern void calculate_hmac_sha256(const char *data, int len, char *buf);
extern int	aes_decrypt_with_password(unsigned char *ciphertext, int ciphertext_len,
									  const char *password, unsigned char *plaintext);
extern int	aes_encrypt_with_password(unsigned char *plaintext, int plaintext_len,
									  const char *password, unsigned char *ciphertext);

#endif
