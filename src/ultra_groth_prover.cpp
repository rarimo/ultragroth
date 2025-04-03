#include <gmp.h>
#include <string>
#include <cstring>
#include <stdexcept>
#include <alt_bn128.hpp>
#include <nlohmann/json.hpp>

#include "ultra_groth.hpp"
#include "zkey_utils.hpp"
#include "wtns_utils.hpp"
#include "binfile_utils.hpp"
#include "fileloader.hpp"

#include "ultra_groth_prover.h"
#include "prover.h"


static void
CopyError(
    char               *error_msg,
    unsigned long long  error_msg_maxsize,
    const char         *str)
{
    if (error_msg) {
        strncpy(error_msg, str, error_msg_maxsize);
    }
}

static bool
PrimeIsValid(mpz_srcptr prime)
{
    mpz_t altBbn128r;

    mpz_init(altBbn128r);
    mpz_set_str(altBbn128r, "21888242871839275222246405745257275088548364400416034343698204186575808495617", 10);

    const bool is_valid = (mpz_cmp(prime, altBbn128r) == 0);

    mpz_clear(altBbn128r);

    return is_valid;
}

struct UltraGrothProver {
    BinFileUtils::BinFile zkey;
    std::unique_ptr<ZKeyUtils::Header> zkeyHeader;
    std::unique_ptr<UltraGroth::Prover<AltBn128::Engine>> prover;

    UltraGrothProver(const void         *zkey_buffer,
                     unsigned long long  zkey_size)

        : zkey(zkey_buffer, zkey_size, "zkey", 1),
          zkeyHeader(ZKeyUtils::loadHeader(&zkey))
    {
        if (!PrimeIsValid(zkeyHeader->rPrime)) {
            throw std::invalid_argument("zkey curve not supported");
        }

        // TODO Think about extra params
        prover = UltraGroth::makeProver<AltBn128::Engine>(
            zkeyHeader->nVars,
            zkeyHeader->nPublic,
            zkeyHeader->domainSize,
            zkeyHeader->nCoefs,
            zkeyHeader->vk_alpha1,
            zkeyHeader->vk_beta1,
            zkeyHeader->vk_beta2,
            zkeyHeader->vk_delta1,
            zkeyHeader->vk_delta2,
            nullptr,                   // final delta 1
            nullptr,                   // final delta 2
            zkey.getSectionData(4),    // Coefs
            zkey.getSectionData(5),    // pointsA
            zkey.getSectionData(6),    // pointsB1
            zkey.getSectionData(7),    // pointsB2
            zkey.getSectionData(8),    // final points C
            nullptr,                   // round points C
            zkey.getSectionData(9)     // pointsH1
        );
    }
};

int
ultra_groth_prover_create(
    void                **prover_object,
    const void          *zkey_buffer,
    unsigned long long   zkey_size,
    char                *error_msg,
    unsigned long long   error_msg_maxsize)
{
    try {
        if (prover_object == NULL) {
            throw std::invalid_argument("Null prover object");
        }

        if (zkey_buffer == NULL) {
            throw std::invalid_argument("Null zkey buffer");
        }

        UltraGrothProver *prover = new UltraGrothProver(zkey_buffer, zkey_size);

        *prover_object = prover;

    } catch (std::exception& e) {
        CopyError(error_msg, error_msg_maxsize, e.what());
        return PROVER_ERROR;

    } catch (std::exception *e) {
        CopyError(error_msg, error_msg_maxsize, e->what());
        delete e;
        return PROVER_ERROR;

    } catch (...) {
        CopyError(error_msg, error_msg_maxsize, "unknown error");
        return PROVER_ERROR;
    }

    return PROVER_OK;
}

int
ultra_groth_prover(
    const void          *zkey_buffer,
    unsigned long long   zkey_size,
    char                *public_buffer,
    unsigned long long  *public_size,
    char                *error_msg,
    unsigned long long   error_msg_maxsize)
{
    void *prover = NULL;

    int error = ultra_groth_prover_create(
                    &prover,
                    zkey_buffer,
                    zkey_size,
                    error_msg,
                    error_msg_maxsize);

    if (error != PROVER_OK) {
        return error;
    }

    error = ultra_groth_prover_prove(
                    prover,
                    public_buffer,
                    public_size,
                    error_msg,
                    error_msg_maxsize);

    ultra_groth_prover_destroy(prover);

    return error;
}

int
ultra_groth_prover_prove(
    void                *prover_object,
    char                *public_buffer,
    unsigned long long  *public_size,
    char                *error_msg,
    unsigned long long   error_msg_maxsize)
{
    try {
        if (prover_object == NULL) {
            throw std::invalid_argument("Null prover object");
        }

        if (public_buffer == NULL) {
            throw std::invalid_argument("Null public buffer");
        }

        if (public_size == NULL) {
            throw std::invalid_argument("Null public size");
        }

        UltraGrothProver *prover = static_cast<UltraGrothProver*>(prover_object);
        std::string stringProof, stringPublic;

        auto proof = prover->prover->prove();
        stringProof = proof->toJson().dump();

        // TODO Get wtns somehow
        //stringPublic = BuildPublicString(wtnsData, prover->zkeyHeader->nPublic);
        //std::strncpy(public_buffer, stringPublic.c_str(), *public_size);

    } catch (std::exception& e) {
        CopyError(error_msg, error_msg_maxsize, e.what());
        return PROVER_ERROR;

    } catch (std::exception *e) {
        CopyError(error_msg, error_msg_maxsize, e->what());
        delete e;
        return PROVER_ERROR;

    } catch (...) {
        CopyError(error_msg, error_msg_maxsize, "unknown error");
        return PROVER_ERROR;
    }

    return PROVER_OK;
}

void
ultra_groth_prover_destroy(void *prover_object) {
    if (prover_object == NULL) return;

    UltraGrothProver *prover = static_cast<UltraGrothProver*>(prover_object);
    delete prover;
}
