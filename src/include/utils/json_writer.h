/*-------------------------------------------------------------------------
 *
 * json_writer.h
 *      PostgreSQL connection pooler and load balancer
 *
 * Copyright (c) 2003-2021 PgPool Global Development Group
 * Copyright (c) 2024-2025, pgElephant, Inc.
 *
 *-------------------------------------------------------------------------
 */
#ifndef pgpool_xcode_json_writer_h
#define pgpool_xcode_json_writer_h
#include "parser/stringinfo.h"
#define MAX_STACK_DEPTH 10
typedef enum JWElementType
{
	JWOBJECT,
	JWARRAY
} JWElementType;

typedef struct JWStack
{
	JWElementType elementType;
	int			elementCount;
} JWStack;

typedef struct JsonNode
{
	StringInfo	buf;
	bool		pretty;
	int			stack_ptr;
	JWStack		stack[MAX_STACK_DEPTH];
} JsonNode;

extern JsonNode *jw_create(JWElementType rootElement, bool pretty_output);
extern JsonNode *jw_create_with_array(bool pretty_output);
extern JsonNode *jw_create_with_object(bool pretty_output);
extern bool jw_put_string(JsonNode *jNode, char *key, char *value);
extern bool jw_put_int(JsonNode *jNode, char *key, int value);
extern bool jw_put_bool(JsonNode *jNode, char *key, bool value);
extern bool jw_put_long(JsonNode *jNode, char *key, long value);
extern bool jw_put_null(JsonNode *jNode, char *key);
extern bool jw_put_string_value(JsonNode *jNode, char *value);
extern bool jw_put_int_value(JsonNode *jNode, int value);
extern bool jw_put_bool_value(JsonNode *jNode, bool value);
extern bool jw_put_long_value(JsonNode *jNode, long value);
extern bool jw_put_null_value(JsonNode *jNode);
extern bool jw_start_element(JsonNode *jNode, JWElementType element, char *key);
extern bool jw_start_array(JsonNode *jNode, char *key);
extern bool jw_start_object(JsonNode *jNode, char *key);
extern bool jw_end_element(JsonNode *jNode);
extern bool jw_finish_document(JsonNode *jNode);
extern char *jw_get_json_string(JsonNode *jNode);
extern int	jw_get_json_length(JsonNode *jNode);
extern void jw_destroy(JsonNode *jNode);

#endif
