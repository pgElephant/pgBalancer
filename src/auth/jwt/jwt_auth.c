/*
 * jwt_auth.c
 * JWT authentication for pgbalancer REST API
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include "jwt_auth.h"

#define JWT_SECRET "supersecretkey" // TODO: store securely
#define JWT_EXPIRY_SECONDS 3600


/*-------------------------------------------------------------------------
 * jwt_auth.c
 *
 * JWT authentication implementation for pgbalancer REST API
 *
 * Follows PostgreSQL coding standards.
 *-------------------------------------------------------------------------
 */

#include "jwt_auth.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>

#define JWT_SECRET "your-very-secret-key" /* TODO: Move to config */
#define JWT_EXPIRY_SECONDS 3600

/*
 * Base64 URL encoding (RFC 7515)
 */
static void
base64url_encode(const unsigned char *input, int len, char *output, int outlen)
{
    int i;
    EVP_EncodeBlock((unsigned char *)output, input, len);
    for (i = 0; output[i]; i++)
    {
        if (output[i] == '+')
            output[i] = '-';
        else if (output[i] == '/')
            output[i] = '_';
        else if (output[i] == '=')
            output[i] = '\0';
    }
}

/*
 * Generate JWT token
 */
char *
jwt_generate_token(const char *username)
{
    char header[128];
    char payload[256];
    char header_b64[256];
    char payload_b64[512];
    char signature[EVP_MAX_MD_SIZE];
    unsigned int sig_len;
    char token[1024];
    time_t now = time(NULL);

    snprintf(header, sizeof(header), "{\"alg\":\"HS256\",\"typ\":\"JWT\"}");
    snprintf(payload, sizeof(payload), "{\"sub\":\"%s\",\"exp\":%ld}", username, (long)(now + JWT_EXPIRY_SECONDS));

    base64url_encode((unsigned char *)header, (int)strlen(header), header_b64, sizeof(header_b64));
    base64url_encode((unsigned char *)payload, (int)strlen(payload), payload_b64, sizeof(payload_b64));

    char signing_input[1024];
    snprintf(signing_input, sizeof(signing_input), "%s.%s", header_b64, payload_b64);

    HMAC(EVP_sha256(), JWT_SECRET, (int)strlen(JWT_SECRET), (unsigned char *)signing_input, (int)strlen(signing_input), (unsigned char *)signature, &sig_len);

    char signature_b64[256];
    base64url_encode((unsigned char *)signature, (int)sig_len, signature_b64, sizeof(signature_b64));

    snprintf(token, sizeof(token), "%s.%s.%s", header_b64, payload_b64, signature_b64);

    return strdup(token);
}

/*
 * Validate JWT token
 *
 * TODO: Implement full JWT validation (split, decode, verify signature, check exp)
 * For demo, accept any token and return "demo" as username.
 */
int
jwt_validate_token(const char *token, char *username_out, size_t username_out_len)
{
    if (username_out)
        strlcpy(username_out, "demo", username_out_len);
    return 1;
}
