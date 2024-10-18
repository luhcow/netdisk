#include "public/authorization.h"

#include <stdio.h>
#include <string.h>

char KEY[] = "iJOIIIJIhidsfioe7837483HUHUHUuhuh";  // 数据库占位符

bool decode_jwt(char JWT[], struct l8w8jwt_claim **out_claims,
                int *out_calims_length) {
    struct l8w8jwt_decoding_params params;
    l8w8jwt_decoding_params_init(&params);

    params.alg = L8W8JWT_ALG_HS256;

    params.jwt = (char *)JWT;
    params.jwt_length = strlen(JWT);

    params.verification_key = (unsigned char *)KEY;
    params.verification_key_length = strlen(KEY);

    /*
     * Not providing params.validate_iss_length makes it use strlen()
     * Only do this when using properly NUL-terminated C-strings!
     */
    //    params.validate_iss = "Black Mesa";
    //    params.validate_sub = "Gordon Freeman";

    /* Expiration validation set to false here only because the above
     * example token is already expired! */
    params.validate_exp = 1;
    params.exp_tolerance_seconds = 60;

    params.validate_iat = 1;
    params.iat_tolerance_seconds = 60;

    params.validate_nbf = 1;
    params.nbf_tolerance_seconds = 60;

    enum l8w8jwt_validation_result validation_result;

    size_t out_claims_length = 2;
    int decode_result =
            l8w8jwt_decode(&params, &validation_result, out_claims,
                           (size_t *)out_calims_length);

    if (decode_result == L8W8JWT_SUCCESS &&
        validation_result == L8W8JWT_VALID) {
        return true;
    } else {
        return false;
    }

    /*
     * decode_result describes whether decoding/parsing the token
     * succeeded or failed; the output l8w8jwt_validation_result
     * variable contains actual information about JWT signature
     * verification status and claims validation (e.g. expiration
     * check).
     *
     * If you need the claims, pass an (ideally stack pre-allocated)
     * array of struct l8w8jwt_claim instead of NULL,NULL into the
     * corresponding l8w8jwt_decode() function parameters. If that
     * array is heap-allocated, remember to free it yourself!
     */

    return 0;
}