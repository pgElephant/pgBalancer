/*-------------------------------------------------------------------------
 *
 * pool_path.h
 *      PostgreSQL connection pooler and load balancer
 *
 * Copyright (c) 2003-2021 PgPool Global Development Group
 * Copyright (c) 2024-2025, pgElephant, Inc.
 *
 *-------------------------------------------------------------------------
 */
#ifndef POOL_PATH_H
#define POOL_PATH_H
#include"pool_type.h"
/*
 * MAXPGPATH: standard size of a pathname buffer in PostgreSQL (hence,
 * maximum usable pathname length is one less).
 *
 * We'd use a standard system header symbol for this, if there weren't
 * so many to choose from: MAXPATHLEN, MAX_PATH, PATH_MAX are all
 * defined by different "standards", and often have different values
 * on the same platform!  So we just punt and use a reasonably
 * generous setting here.
 */
#define MAXPGPATH       1024

#define IS_DIR_SEP(ch)  ((ch) == '/')
#define is_absolute_path(filename) \
( \
    ((filename)[0] == '/') \
)

extern void get_parent_directory(char *path);
extern void join_path_components(char *ret_path, const char *head, const char *tail);
extern void canonicalize_path(char *path);
extern char *last_dir_separator(const char *filename);
extern bool get_home_directory(char *buf, int bufsize);
extern bool get_os_username(char *buf, int bufsize);
extern char *get_current_working_dir(void);
extern char *make_absolute_path(const char *path, const char *base_dir);

#endif							/* POOL_PATH_H */
