#include <iostream>
#include <string>
#include <cstring>
#include <stdexcept>

#include <gmp.h>
#include <alt_bn128.hpp>
//#include <nlohmann/json.hpp>
//#include "prover.h"

#include "ultra_groth.hpp"
#include "zkey_utils.hpp"
#include "wtns_utils.hpp"
#include "binfile_utils.hpp"
#include "fileloader.hpp"

#include "ultra_groth.hpp"

int main() {

    //long long res = UltraGroth::dummy(5);
    AltBn128::G1PointAffine commitment = {1243232532, 12432433, 234324, 234324};

    //commitment.x.v[0] = 1;
    //commitment.x.v[1] = 2;
    //commitment.x.v[2] = 0;
    //commitment.x.v[3] = 0;
//
    //commitment.y.v[0] = 1;
    //commitment.y.v[1] = 2;
    //commitment.y.v[2] = 0;
    //commitment.y.v[3] = 0;

    unsigned char accumulator[32];
    for(int i = 0; i < 32; i++) {
        accumulator[i] = i;
    }
    unsigned char buffer[32 + 2 * 32];
    memcpy(buffer, accumulator, 32 * sizeof(unsigned char));
    uint64_t points_bytes[8] = {commitment.x.v[0], commitment.x.v[1], commitment.x.v[2], commitment.x.v[3],
                                commitment.y.v[0], commitment.y.v[1], commitment.y.v[2], commitment.y.v[3]};
    memcpy(buffer + 32, points_bytes, 8 * sizeof(uint64_t));
    for(int i = 0; i < 96; i++) {
        std::cout << static_cast<int>(buffer[i]) << " ";
    }
    
    std::cout << std::endl;
    return 0;
}