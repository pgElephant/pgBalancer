/* jwt_auth.h - JWT authentication header */
#ifndef JWT_AUTH_H
#define JWT_AUTH_H

int jwt_generate(const char *username, char *token, int token_len);
char *jwt_validate(const char *token);

#endif /* JWT_AUTH_H */
