#ifndef ZKEY_UTILS_H
#define ZKEY_UTILS_H

#include <gmp.h>
#include <memory>
#include <cstdint>

#include "binfile_utils.hpp"


// Header(1)
//      prover_type (1337 for UltraGroth)
// HeaderGroth(2)
//      n8q (the number of bytes needed to hold field order)
//      q (field order)
//      n8r (the number of bytes needed to hold group order)
//      r (group order)
//      n_vars (number of all the witness vars)
//      n_pubs (number of public inputs + outputs)
//      domain_size (2 ** (log2(n_constraints + n_pubs) + 1))
//      n_indexes_c1
//      n_indexes_c2
//      rand_indx
//      alpha_1
//      beta_1
//      beta_2
//      gamma_2
//      delta_c1_1
//      delta_c1_2
//      delta_c2_1
//      delta_c2_2
// IC(3)
// Coeffs(4)
// PointsA(5)
// PointsB1(6)
// PointsB2(7)
// PointsC1(8)
// PointsC2(9)
// IndexesC1(10)
// IndexesC2(11)
// PointsH(12)
// Contributions(13)

namespace ZKeyUtils {

    class Header {

    public:
        uint32_t n8q;
        mpz_t qPrime;
        uint32_t n8r;
        mpz_t rPrime;

        uint32_t nVars;
        uint32_t nPublic;
        uint32_t domainSize;
        uint64_t nCoefs;

        void *vk_alpha1;
        void *vk_beta1;
        void *vk_beta2;
        void *vk_gamma2;
        void *vk_delta1;
        void *vk_delta2;

        Header();
        ~Header();
    };

    std::unique_ptr<Header> loadHeader(BinFileUtils::BinFile *f);


    class UltraGrothHeader {

    public:
        uint32_t n8q;
        mpz_t qPrime;
        uint32_t n8r;
        mpz_t rPrime;

        uint32_t nVars;
        uint32_t nPublic;
        uint32_t domainSize;
        uint64_t nCoefs;

        uint32_t num_indexes_c1;
        uint32_t num_indexes_c2;
        uint32_t rand_indx;

        void *alpha1;
        void *beta1;
        void *beta2;
        void *gamma2;
        void *round_delta1;
        void *round_delta2; 
        void *final_delta1;
        void *final_delta2;

        UltraGrothHeader();
        ~UltraGrothHeader();
    };

    std::unique_ptr<UltraGrothHeader> ultra_groth_loadHeader(BinFileUtils::BinFile *f);
    std::tuple<std::vector<uint32_t>, std::vector<uint32_t>> load_indexes(std::string path);
}

#endif // ZKEY_UTILS_H
