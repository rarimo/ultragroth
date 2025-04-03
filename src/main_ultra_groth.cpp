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

#include <openssl/sha.h>


int main(int argc, char **argv) {

    if (argc != 2) {
        std::cerr << "Invalid number of parameters" << std::endl;
        std::cerr << "Usage: prover <circuit.zkey>" << std::endl;
        return EXIT_FAILURE;
    }

    try {
        const std::string zkeyFilename = argv[1];
        

    } catch (std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    
    exit(EXIT_SUCCESS);
}