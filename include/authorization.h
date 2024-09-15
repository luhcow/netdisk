#include <stdbool.h>
#include <time.h>

#include "l8w8jwt/claim.h"
#include "l8w8jwt/decode.h"

bool encode_jwt(char JWT[], char user_name[], time_t pwd_ts,
                struct l8w8jwt_claim **out_claims, int out_calims_length);