/*-------------------------------------------------------------------------
 *
 * regex_array.h
 *      PostgreSQL connection pooler and load balancer
 *
 * Copyright (c) 2003-2021 PgPool Global Development Group
 * Copyright (c) 2024-2025, pgElephant, Inc.
 *
 *-------------------------------------------------------------------------
 */
#ifndef REGEX_ARRAY_H
#define REGEX_ARRAY_H

#include <regex.h>

#define AR_ALLOC_UNIT	16

/*
 * Regular expression array
 */
typedef struct
{
	int			size;			/* regex array size */
	int			pos;			/* next regex array index position */
	regex_t   **regex;			/* regular expression array */
} RegArray;

RegArray   *create_regex_array(void);
int			add_regex_array(RegArray *ar, char *pattern);
int			regex_array_match(RegArray *ar, char *pattern);
void		destroy_regex_array(RegArray *ar);

/*
 * String left-right token type
 */
typedef struct
{
	char	   *left_token;
	char	   *right_token;
	double		weight_token;
} Left_right_token;

typedef struct
{
	int			pos;
	int			size;
	Left_right_token *token;
} Left_right_tokens;

Left_right_tokens *create_lrtoken_array(void);
void		extract_string_tokens2(char *str, char *delimi, char delimi2, Left_right_tokens *lrtokens);

#endif
