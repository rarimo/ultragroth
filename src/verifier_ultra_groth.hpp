#ifndef VERIFIER_ULTRA_GROTH_HPP
#define VERIFIER_ULTRA_GROTH_HPP

//Error codes returned by the functions.
#define VERIFIER_VALID_PROOF        0x0
#define VERIFIER_INVALID_PROOF      0x1
#define VERIFIER_ERROR              0x2

/**
 * 'proof', 'inputs' and 'verification_key' are null-terminated json strings.
 *
 * @return error code:
 *         VERIFIER_VALID_PROOF   - in case of valid 'proof'.
 *         VERIFIER_INVALID_PROOF - in case of invalid 'proof'.
           VERIFIER_ERROR         - in case of an error
 */

int
ultra_groth_verify(
    const char    *proof,
    const char    *inputs,
    const char    *verification_key,
    char          *error_msg,
    unsigned long  error_msg_maxsize
);

#endif // VERIFIER_HPP
