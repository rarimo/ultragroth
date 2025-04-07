#ifndef ZKEY_UTILS_H
#define ZKEY_UTILS_H

#include <gmp.h>
#include <memory>

#include "binfile_utils.hpp"

namespace ZKeyUtils {

    class Header {


    public:
        u_int32_t n8q;
        mpz_t qPrime;
        u_int32_t n8r;
        mpz_t rPrime;

        u_int32_t nVars;
        u_int32_t nPublic;
        u_int32_t domainSize;
        u_int64_t nCoefs;

        void *alpha1;
        void *beta1;
        void *beta2;
        void *gamma2;
        void *round_delta1;
        void *round_delta2;
        void *final_delta1;
        void *final_delta2;

        Header();
        ~Header();
    };

    std::unique_ptr<Header> loadHeader(BinFileUtils::BinFile *f);
    std::tuple<std::vector<uint32_t>, std::vector<uint32_t>> load_indexes(std::string path);
}

#endif // ZKEY_UTILS_H
