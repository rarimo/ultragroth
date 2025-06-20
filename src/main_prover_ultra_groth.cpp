#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <stdexcept>
#include "prover_ultra_groth.hpp"
#include "fileloader.hpp"

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
        std::string        publicBuffer;
        std::string        proofBuffer;
        unsigned long long       publicSize = 0;
        unsigned long long       proofSize = 0;
        char                     errorMsg[1024];

        int error = ultra_groth_public_size_for_zkey_buf(
                     zkeyFile.dataBuffer(),
                     zkeyFile.dataSize(),
                     &publicSize,
                     errorMsg,
                     sizeof(errorMsg));

        if (error != PROVER_OK) {
            throw std::runtime_error(errorMsg);
        }

        ultra_groth_proof_size(&proofSize);

        error = ultra_groth_prover(
                   zkeyFile.dataBuffer(),
                   zkeyFile.dataSize(),
                   wtnsFile.dataBuffer(),
                   wtnsFile.dataSize(),
                   proofBuffer,
                   &proofSize,
                   publicBuffer,
                   &publicSize,
                   errorMsg,
                   sizeof(errorMsg));

        if (error != PROVER_OK) {
            throw std::runtime_error(errorMsg);
        }

        std::ofstream proofFile(proofFilename);
        proofFile << proofBuffer;
        proofFile.close();

        std::ofstream publicFile(publicFilename);
        publicFile << publicBuffer;
        publicFile.close();

    } catch (std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;

    }

    exit(EXIT_SUCCESS);
}
