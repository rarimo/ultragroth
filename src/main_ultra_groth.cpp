#include <iostream>
#include <string>
#include <cstring>
#include <stdexcept>
#include <vector>
#include <fstream>

#include <gmp.h>
#include <openssl/sha.h>

#include <alt_bn128.hpp>

#include "ultra_groth.hpp"
#include "zkey_utils.hpp"
#include "wtns_utils.hpp"
#include "binfile_utils.hpp"
#include "fileloader.hpp"

#include "ultra_groth_prover.h"
#include "ultra_groth.hpp"


int main(int argc, char **argv) {

    std::cout << "Entry" << std::endl;

    auto x = round1();
    if (argc != 2) {
        std::cerr << "Invalid number of parameters" << std::endl;
        std::cerr << "Usage: prover <circuit.zkey>" << std::endl;
        return EXIT_FAILURE;
    }

    try {
        const std::string zkeyFilename = argv[1];

        BinFileUtils::FileLoader zkeyFile(zkeyFilename);
        std::vector<char>        publicBuffer;
        unsigned long long       publicSize = 0;
        char                     errorMsg[1024];


        int error = ultragroth_public_size_for_zkey_buf(
            zkeyFile.dataBuffer(),
            zkeyFile.dataSize(),
            &publicSize,
            errorMsg,
            sizeof(errorMsg));
        
        if (error != PROVER_OK) {
            throw std::runtime_error(errorMsg);
        }

        publicBuffer.resize(publicSize);
        
        error = ultra_groth_prover(
            zkeyFile.dataBuffer(),
            zkeyFile.dataSize(),
            publicBuffer.data(),
            &publicSize,
            errorMsg,
            sizeof(errorMsg));

        if (error != PROVER_OK) {
            throw std::runtime_error(errorMsg);
        }

    } catch (std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    std::ifstream file("data.json");
    if (!file.is_open()) {
        throw std::invalid_argument( "Failed to open JSON" );
    }

    json data;
    file >> data;


    UltraGroth::Proof<AltBn128::Engine> proof(AltBn128::Engine::engine);
    proof.fromJson(data);
    UltraGroth::Verifier<AltBn128::Engine> verifier;

    //bool valid = verifier.verify();
    
    exit(EXIT_SUCCESS);

}