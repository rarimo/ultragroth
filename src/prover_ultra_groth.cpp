#include <gmp.h>
#include <string>
#include <cstring>
#include <stdexcept>
#include <alt_bn128.hpp>
#include <nlohmann/json.hpp>
#include "prover_ultra_groth.hpp"
#include "ultra_groth.hpp"
#include "zkey_utils.hpp"
#include "wtns_utils.hpp"
#include "binfile_utils.hpp"
#include "fileloader.hpp"

using json = nlohmann::json;


class ShortBufferException : public std::invalid_argument
{
public:
    explicit ShortBufferException(const std::string &msg)
        : std::invalid_argument(msg) {}
};

class InvalidWitnessLengthException : public std::invalid_argument
{
public:
    explicit InvalidWitnessLengthException(const std::string &msg)
        : std::invalid_argument(msg) {}
};

static void
CopyError(
    char                 *error_msg,
    unsigned long long    error_msg_maxsize,
    const std::exception &e)
{
    if (error_msg) {
        strncpy(error_msg, e.what(), error_msg_maxsize);
    }
}

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

static unsigned long long
ProofBufferMinSize()
{
    return 1300;
}

static unsigned long long
PublicBufferMinSize(unsigned long long count)
{
    return count * 82 + 4;
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

static std::string
BuildPublicString(AltBn128::FrElement *wtnsData, uint32_t nPublic, uint32_t rand_indx)
{
    json jsonPublic;
    AltBn128::FrElement aux;
    for (uint32_t i=1; i<= nPublic; i++) {
        // signal corresponding to rand index is skipped because it derived from rounds commitments hash during verification
        if (i == rand_indx) {
            continue;
        }
        AltBn128::Fr.toMontgomery(aux, wtnsData[i]);
        jsonPublic.push_back(AltBn128::Fr.toString(aux));
    }

    return jsonPublic.dump();
}

static void
CheckAndUpdateBufferSizes(
    unsigned long long   proofCalcSize,
    unsigned long long  *proofSize,
    unsigned long long   publicCalcSize,
    unsigned long long  *publicSize,
    const std::string   &type)
{
    if (*proofSize < proofCalcSize || *publicSize < publicCalcSize) {

        *proofSize  = proofCalcSize;
        *publicSize = publicCalcSize;

        if (*proofSize < proofCalcSize) {
            throw ShortBufferException("Proof buffer is too short. " + type + " size: "
                                       + std::to_string(proofCalcSize) +
                                       ", actual size: "
                                       + std::to_string(*proofSize));
        } else {
            throw ShortBufferException("Public buffer is too short. " + type + " size: "
                                       + std::to_string(proofCalcSize) +
                                       ", actual size: "
                                       + std::to_string(*proofSize));
       }
    }
}

class UltraGrothProver
{
    BinFileUtils::BinFile zkey;
    std::unique_ptr<ZKeyUtils::Header> zkeyHeader;
    std::unique_ptr<UltraGroth::Prover<AltBn128::Engine>> prover;

public:
    UltraGrothProver(const void         *zkey_buffer,
                  unsigned long long  zkey_size)

        : zkey(zkey_buffer, zkey_size, "zkey", 1),
          zkeyHeader(ZKeyUtils::loadHeader(&zkey))
    {
        if (!PrimeIsValid(zkeyHeader->rPrime)) {
            throw std::invalid_argument("zkey curve not supported");
        }

         prover = UltraGroth::makeProver<AltBn128::Engine>(
            zkeyHeader->nVars,
            zkeyHeader->nPublic,
            zkeyHeader->domainSize,
            zkeyHeader->nCoefs,
            zkey.getSectionData(10),   // round indexes
            zkeyHeader->num_indexes_c1,
            zkey.getSectionData(11),   // final round indexes
            zkeyHeader->num_indexes_c2,
            zkeyHeader->rand_indx,
            zkeyHeader->alpha1,
            zkeyHeader->beta1,
            zkeyHeader->beta2,
            zkeyHeader->final_delta1,  // final delta 1
            zkeyHeader->final_delta2,  // final delta 2
            zkeyHeader->round_delta1,  // round delta 1
            zkey.getSectionData(4),    // Coefs
            zkey.getSectionData(5),    // pointsA
            zkey.getSectionData(6),    // pointsB1
            zkey.getSectionData(7),    // pointsB2
            zkey.getSectionData(9),    // final points C
            zkey.getSectionData(8),    // round points C
            zkey.getSectionData(12)    // pointsH1
        );
    }

    void prove(const void         *wtns_buffer,
               unsigned long long  wtns_size,
               std::string        &stringProof,
               std::string        &stringPublic)
    {
        BinFileUtils::BinFile wtns(wtns_buffer, wtns_size, "wtns", 2);
        auto wtnsHeader = WtnsUtils::loadHeader(&wtns);

        if (zkeyHeader->nVars != wtnsHeader->nVars) {
            throw InvalidWitnessLengthException("Invalid witness length. Circuit: "
                                        + std::to_string(zkeyHeader->nVars)
                                        + ", witness: "
                                        + std::to_string(wtnsHeader->nVars));
        }

        if (!PrimeIsValid(wtnsHeader->prime)) {
            throw std::invalid_argument("different wtns curve");
        }

        AltBn128::FrElement* signals = new AltBn128::FrElement[zkeyHeader->nVars];
        // Can't modify values on pointer from wtns.getSectionData(2) so I copy it to another
        memcpy(signals, wtns.getSectionData(2), wtns.getSectionSize(2));

        UltraGroth::LookupInfo lookupInfo = UltraGroth::LookupInfo(
            (uint32_t *)wtns.getSectionData(3), wtns.getSectionSize(3) >> 2,
            (uint32_t *)wtns.getSectionData(4), wtns.getSectionSize(4) >> 2,
            (uint32_t *)wtns.getSectionData(5), wtns.getSectionSize(5) >> 2,
            (uint32_t *)wtns.getSectionData(6), wtns.getSectionSize(6) >> 2
        );
        
        auto proof = prover->prove(signals, lookupInfo);

        stringProof = proof->toJson().dump();
        stringPublic = BuildPublicString(signals, zkeyHeader->nPublic, zkeyHeader->rand_indx);
        
        delete[] signals;
    }

    unsigned long long proofBufferMinSize() const
    {
        return ProofBufferMinSize();
    }

    unsigned long long publicBufferMinSize() const
    {
        return PublicBufferMinSize(zkeyHeader->nPublic);
    }
};

int
ultra_groth_public_size_for_zkey_buf(
    const void          *zkey_buffer,
    unsigned long long   zkey_size,
    unsigned long long  *public_size,
    char                *error_msg,
    unsigned long long   error_msg_maxsize)
{
    try {
        BinFileUtils::BinFile zkey(zkey_buffer, zkey_size, "zkey", 1);
        auto zkeyHeader = ZKeyUtils::loadHeader(&zkey);

        *public_size = PublicBufferMinSize(zkeyHeader->nPublic);

    } catch (std::exception& e) {
        CopyError(error_msg, error_msg_maxsize, e);
        return PROVER_ERROR;

    } catch (...) {
        CopyError(error_msg, error_msg_maxsize, "unknown error");
        return PROVER_ERROR;
    }

    return PROVER_OK;
}

int
ultra_groth_public_size_for_zkey_file(
    const char          *zkey_fname,
    unsigned long long  *public_size,
    char                *error_msg,
    unsigned long long   error_msg_maxsize)
{
    try {
        auto zkey = BinFileUtils::openExisting(zkey_fname, "zkey", 1);
        auto zkeyHeader = ZKeyUtils::loadHeader(zkey.get());

        *public_size = PublicBufferMinSize(zkeyHeader->nPublic);

    } catch (std::exception& e) {
        CopyError(error_msg, error_msg_maxsize, e);
        return PROVER_ERROR;

    } catch (...) {
        CopyError(error_msg, error_msg_maxsize, "unknown error");
        return PROVER_ERROR;
    }

    return PROVER_OK;
}

void
ultra_groth_proof_size(
    unsigned long long *proof_size)
{
    *proof_size = ProofBufferMinSize();
}

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
        CopyError(error_msg, error_msg_maxsize, e);
        return PROVER_ERROR;

    } catch (std::exception *e) {
        CopyError(error_msg, error_msg_maxsize, *e);
        delete e;
        return PROVER_ERROR;

    } catch (...) {
        CopyError(error_msg, error_msg_maxsize, "unknown error");
        return PROVER_ERROR;
    }

    return PROVER_OK;
}

int
ultra_groth_prover_create_zkey_file(
    void                **prover_object,
    const char          *zkey_file_path,
    char                *error_msg,
    unsigned long long   error_msg_maxsize)
{
    BinFileUtils::FileLoader fileLoader;

    try {
        fileLoader.load(zkey_file_path);

    } catch (std::exception& e) {
        CopyError(error_msg, error_msg_maxsize, e);
        return PROVER_ERROR;
    }

    return ultra_groth_prover_create(
                prover_object,
                fileLoader.dataBuffer(),
                fileLoader.dataSize(),
                error_msg,
                error_msg_maxsize);
}

int
ultra_groth_prover_prove(
    void                *prover_object,
    const void          *wtns_buffer,
    unsigned long long   wtns_size,
    std::string         &proof_buffer,
    unsigned long long  *proof_size,
    std::string         &public_buffer,
    unsigned long long  *public_size,
    char                *error_msg,
    unsigned long long   error_msg_maxsize)
{
    try {
        if (prover_object == NULL) {
            throw std::invalid_argument("Null prover object");
        }

        if (wtns_buffer == NULL) {
            throw std::invalid_argument("Null witness buffer");
        }

        UltraGrothProver *prover = static_cast<UltraGrothProver*>(prover_object);

        prover->prove(wtns_buffer, wtns_size, proof_buffer, public_buffer);

    } catch(InvalidWitnessLengthException& e) {
        CopyError(error_msg, error_msg_maxsize, e);
        return PROVER_INVALID_WITNESS_LENGTH;

    } catch(ShortBufferException& e) {
        CopyError(error_msg, error_msg_maxsize, e);
        return PROVER_ERROR_SHORT_BUFFER;

    } catch (std::exception& e) {
        CopyError(error_msg, error_msg_maxsize, e);
        return PROVER_ERROR;

    } catch (std::exception *e) {
        CopyError(error_msg, error_msg_maxsize, *e);
        delete e;
        return PROVER_ERROR;

    } catch (...) {
        CopyError(error_msg, error_msg_maxsize, "unknown error");
        return PROVER_ERROR;
    }

    return PROVER_OK;
}

void
ultra_groth_prover_destroy(void *prover_object)
{
    if (prover_object != NULL) {
        UltraGrothProver *prover = static_cast<UltraGrothProver*>(prover_object);

        delete prover;
    }
}

int
ultra_groth_prover(
    const void          *zkey_buffer,
    unsigned long long   zkey_size,
    const void          *wtns_buffer,
    unsigned long long   wtns_size,
    std::string         &proof_buffer,
    unsigned long long  *proof_size,
    std::string         &public_buffer,
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
                    wtns_buffer,
                    wtns_size,
                    proof_buffer,
                    proof_size,
                    public_buffer,
                    public_size,
                    error_msg,
                    error_msg_maxsize);

    ultra_groth_prover_destroy(prover);

    return error;
}

int
ultra_groth_prover_zkey_file(
    const char          *zkey_file_path,
    const void          *wtns_buffer,
    unsigned long long   wtns_size,
    std::string         &proof_buffer,
    unsigned long long  *proof_size,
    std::string         &public_buffer,
    unsigned long long  *public_size,
    char                *error_msg,
    unsigned long long   error_msg_maxsize)
{
    BinFileUtils::FileLoader fileLoader;

    try {
        fileLoader.load(zkey_file_path);

    } catch (std::exception& e) {
        CopyError(error_msg, error_msg_maxsize, e);
        return PROVER_ERROR;
    }

    return ultra_groth_prover(
            fileLoader.dataBuffer(),
            fileLoader.dataSize(),
            wtns_buffer,
            wtns_size,
            proof_buffer,
            proof_size,
            public_buffer,
            public_size,
            error_msg,
            error_msg_maxsize);
}
