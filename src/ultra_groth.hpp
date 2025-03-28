#ifndef ULTRA_GROTH_HPP
#define ULTRA_GROTH_HPP

#include <string>
#include <array>
#include <memory>
#include <cstdint>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "fft.hpp"


namespace UltraGroth {
    struct Input {
        uint32_t m_acc_k;  // TODO Some file from contract side (think about store in gmp.h library)
        //std::vector<uint8_t> m_witness;
        std::vector<uint32_t> m_indexes;
    };

    struct Output {
        // Randomness r and C (commitment)
        uint32_t dummy;
    };

    template <typename Engine>
    Output *round1(
        const Input *input,
        typename Engine::FrElement *wtns,
        uint32_t domainSize
    );
    Output *round2(const Input *input);
}


#include "ultra_groth.cpp"

#endif // ULTRA_GROTH_HPP
