#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <stdexcept>
#include <cstdint>
#include "prover.h"
#include "fileloader.hpp"


void first_zero_symbol(std::vector<char> &arr) {
    for (int i = 0; i < arr.size(); ++i)
        if (arr[i] == 0)
            arr.resize(i);
}

int main(int argc, char **argv)
{
    if (argc != 5) {
        std::cerr << "Invalid number of parameters" << std::endl;
        std::cerr << "Usage: prover <circuit.zkey> <witness.wtns> <proof.json> <public.json>" << std::endl;
        return EXIT_FAILURE;
    }

    try {
        const std::string zkeyFilename = argv[1];
        const std::string wtnsFilename = argv[2];
        const std::string proofFilename = argv[3];
        const std::string publicFilename = argv[4];

        BinFileUtils::FileLoader zkeyFile(zkeyFilename);
        BinFileUtils::FileLoader wtnsFile(wtnsFilename);
        std::vector<char>        publicBuffer;
        std::vector<char>        proofBuffer;
        unsigned long long       publicSize = 0;
        unsigned long long       proofSize = 0;
        char                     errorMsg[1024];

        int error = groth16_public_size_for_zkey_buf(
                     zkeyFile.dataBuffer(),
                     zkeyFile.dataSize(),
                     &publicSize,
                     errorMsg,
                     sizeof(errorMsg));

        if (error != PROVER_OK) {
            throw std::runtime_error(errorMsg);
        }

        groth16_proof_size(&proofSize);

        publicBuffer.resize(publicSize);
        proofBuffer.resize(proofSize);

        error = groth16_prover(
                   zkeyFile.dataBuffer(),
                   zkeyFile.dataSize(),
                   wtnsFile.dataBuffer(),
                   wtnsFile.dataSize(),
                   proofBuffer.data(),
                   &proofSize,
                   publicBuffer.data(),
                   &publicSize,
                   errorMsg,
                   sizeof(errorMsg));

        if (error != PROVER_OK) {
            throw std::runtime_error(errorMsg);
        }

        std::ofstream proofFile(proofFilename);
        first_zero_symbol(proofBuffer);
        proofFile.write(proofBuffer.data(), proofBuffer.size());

        std::ofstream publicFile(publicFilename);
        first_zero_symbol(publicBuffer);
        publicFile.write(publicBuffer.data(), publicBuffer.size());

    } catch (std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;

    }

    exit(EXIT_SUCCESS);
}
