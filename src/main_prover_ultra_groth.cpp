#include <iostream>
#include <string>
#include <cstring>
#include <stdexcept>
#include <vector>
#include <fstream>

#include <gmp.h>

#include <alt_bn128.hpp>

#include "ultra_groth.hpp"
#include "prover_ultra_groth.hpp"
#include "zkey_utils.hpp"
#include "wtns_utils.hpp"
#include "binfile_utils.hpp"
#include "fileloader.hpp"

const uint8_t* get_json_bytes(const char* filename, size_t* out_size) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file) return nullptr;

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    static std::vector<uint8_t> buffer;
    buffer.resize(size);

    if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
        return nullptr;
    }

    *out_size = buffer.size();
    return buffer.data();  // pointer to byte array
}

int main(int argc, char **argv)
{
    if (argc != 3) {
        std::cerr << "Invalid number of parameters" << std::endl;
        std::cerr << "Usage: prover <circuit.zkey> <input.json>" << std::endl;
        return EXIT_FAILURE;
    }

    try {
        const std::string zkeyFilename = argv[1];

        BinFileUtils::FileLoader zkeyFile(zkeyFilename);
        std::vector<char>        publicBuffer;
        unsigned long long       publicSize = 0;
        char                     errorMsg[1024];

        size_t json_size = 0;
        const uint8_t* bytes = get_json_bytes(argv[2], &json_size);

        if (!bytes) {
            std::cerr << "Failed to read JSON file\n";
            return 1;
        }

        int error = ultragroth_public_size_for_zkey_buf(
            zkeyFile.dataBuffer(),
            zkeyFile.dataSize(),
            &publicSize,
            errorMsg,
            sizeof(errorMsg)
        );
        
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
            sizeof(errorMsg),
            bytes,
            json_size
        );

        if (error != PROVER_OK) {
            throw std::runtime_error(errorMsg);
        }

    } catch (std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    
    exit(EXIT_SUCCESS);

}