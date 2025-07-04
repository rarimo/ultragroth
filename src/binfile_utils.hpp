#ifndef BINFILE_UTILS_H
#define BINFILE_UTILS_H
#include <string>
#include <map>
#include <vector>
#include <memory>
#include <cstdint>
#include "fileloader.hpp"

namespace BinFileUtils {
    
    class BinFile {

        FileLoader fileLoader;

        const void *addr;
        uint64_t size;
        uint64_t pos;

        class Section {
            void *start;
            uint64_t size;

        public:

            friend BinFile;
            Section(void *_start, uint64_t _size)  : start(_start), size(_size) {};
        };

        std::map<int,std::vector<Section>> sections;
        std::string type;
        uint32_t version;

        Section *readingSection;

        void readFileData(std::string _type, uint32_t maxVersion);

    public:

        BinFile(const void *fileData, size_t fileSize, std::string _type, uint32_t maxVersion);
        BinFile(const std::string& fileName, const std::string& _type, uint32_t maxVersion);
        BinFile(const BinFile&) = delete;
        BinFile& operator=(const BinFile&) = delete;

        void startReadSection(uint32_t sectionId, uint32_t setionPos = 0);
        void endReadSection(bool check = true);

        void *getSectionData(uint32_t sectionId, uint32_t sectionPos = 0);
        uint64_t getSectionSize(uint32_t sectionId, uint32_t sectionPos = 0);

        uint32_t readU32LE();
        uint64_t readU64LE();

        void *read(uint64_t l);
    };

    std::unique_ptr<BinFile> openExisting(const std::string& filename, const std::string& type, uint32_t maxVersion);
} // Namespace

#endif // BINFILE_UTILS_H
