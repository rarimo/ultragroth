#ifndef PROVER_ULTRA_GROTH_H
#define PROVER_ULTRA_GROTH_H

#ifdef __cplusplus
extern "C" {
#endif

int
ultragroth_public_size_for_zkey_buf(
    const void          *zkey_buffer,
    unsigned long long   zkey_size,
    unsigned long long  *public_size,
    char                *error_msg,
    unsigned long long   error_msg_maxsize);


/**
 * Initializes 'prover_object' with a pointer to a new prover object.
 * @return error code:
 *         PROVER_OK - in case of success
 *         PPOVER_ERROR - in case of an error
 */
int
ultra_groth_prover_create(
    void                **prover_object,
    const void          *zkey_buffer,
    unsigned long long   zkey_size,
    char                *error_msg,
    unsigned long long   error_msg_maxsize);

/**
 * Proves 'wtns_buffer' and saves results to 'proof_buffer' and 'public_buffer'.
 * @return error code:
 *         PROVER_OK - in case of success
 *         PPOVER_ERROR - in case of an error
 *         PROVER_ERROR_SHORT_BUFFER - in case of a short buffer error, also updates proof_size and public_size with actual proof and public sizes
 */
int
ultra_groth_prover_prove(
    void                *prover_object,
    char                *public_buffer,
    unsigned long long  *public_size,
    char                *error_msg,
    unsigned long long   error_msg_maxsize);

/**
 * Destroys 'prover_object'.
 */
void
ultra_groth_prover_destroy(void *prover_object);

/**
 * groth16_prover
 * @return error code:
 *         PROVER_OK - in case of success
 *         PPOVER_ERROR - in case of an error
 *         PROVER_ERROR_SHORT_BUFFER - in case of a short buffer error, also updates proof_size and public_size with actual proof and public sizes
 */
int
ultra_groth_prover(
    const void          *zkey_buffer,
    unsigned long long   zkey_size,
    char                *public_buffer,
    unsigned long long  *public_size,
    char                *error_msg,
    unsigned long long   error_msg_maxsize);

#ifdef __cplusplus
}
#endif

#endif // PROVER_ULTRA_GROTH_H
