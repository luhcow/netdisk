#include <l8w8jwt/claim.h>
#include <l8w8jwt/decode.h>
#include <stdbool.h>
#include <time.h>

bool decode_jwt(char JWT[], struct l8w8jwt_claim **out_claims,
                int *out_calims_length);