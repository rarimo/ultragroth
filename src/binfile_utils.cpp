#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <system_error>
#include <string>
#include <memory.h>
#include <stdexcept>

#include "binfile_utils.hpp"
#include "fileloader.hpp"

namespace BinFileUtils {

BinFile::BinFile(const std::string& fileName, const std::string& _type, uint32_t maxVersion)
    : fileLoader(fileName)
{
    addr = fileLoader.dataBuffer();
    size = fileLoader.dataSize();

    readFileData(_type, maxVersion);
}

BinFile::BinFile(const void *fileData, size_t fileSize, std::string _type, uint32_t maxVersion) {

    addr = fileData;
    size = fileSize;

    readFileData(_type, maxVersion);
}

void BinFile::readFileData(std::string _type, uint32_t maxVersion) {

    const uint64_t headerSize = 12;
    const uint64_t minSectionSize = 12;

    if (size < headerSize) {
        throw std::range_error("File is too short.");
    }

    type.assign((const char *)addr, 4);
    pos = 4;

    if (type != _type) {
        throw std::invalid_argument("Invalid file type. It should be " + _type + " and it is " + type);
    }

    version = readU32LE();
    if (version > maxVersion) {
        throw std::invalid_argument("Invalid version. It should be <=" + std::to_string(maxVersion) + " and it is " + std::to_string(version));
    }

    uint32_t nSections = readU32LE();

    if (size < headerSize + nSections * minSectionSize) {
        throw std::range_error("File is too short to contain " + std::to_string(nSections) + " sections.");
    }

    for (uint32_t i=0; i<nSections; i++) {
        uint32_t sType=readU32LE();
        uint64_t sSize=readU64LE();

        if (sections.find(sType) == sections.end()) {
            sections.insert(std::make_pair(sType, std::vector<Section>()));
        }

        sections[sType].push_back(Section( (void *)((uint64_t)addr + pos), sSize));

        pos += sSize;

        if (pos > size) {
            throw std::range_error("Section #" + std::to_string(i) + " is invalid."
                ". It ends at pos " + std::to_string(pos) +
                " but should end before " + std::to_string(size) + ".");
        }
    }

    pos = 0;
    readingSection = nullptr;
}


void BinFile::startReadSection(uint32_t sectionId, uint32_t sectionPos) {

    if (sections.find(sectionId) == sections.end()) {
        throw std::range_error("Section does not exist: " + std::to_string(sectionId));
    }

    if (sectionPos >= sections[sectionId].size()) {
        throw std::range_error("Section pos too big. There are " + std::to_string(sections[sectionId].size()) + " and it's trying to access section: " + std::to_string(sectionPos));
    }

    if (readingSection != nullptr) {
        throw std::range_error("Already reading a section");
    }

    pos = (uint64_t)(sections[sectionId][sectionPos].start) - (uint64_t)addr;

    readingSection = &sections[sectionId][sectionPos];
}

void BinFile::endReadSection(bool check) {
    if (check) {
        if ((uint64_t)addr + pos - (uint64_t)(readingSection->start) != readingSection->size) {
            throw std::range_error("Invalid section size");
        }
    }
    readingSection = nullptr;
}

void *BinFile::getSectionData(uint32_t sectionId, uint32_t sectionPos) {

    if (sections.find(sectionId) == sections.end()) {
        throw std::range_error("Section does not exist: " + std::to_string(sectionId));
    }

    if (sectionPos >= sections[sectionId].size()) {
        throw std::range_error("Section pos too big. There are " + std::to_string(sections[sectionId].size()) + " and it's trying to access section: " + std::to_string(sectionPos));
    }

    return sections[sectionId][sectionPos].start;
}

uint64_t BinFile::getSectionSize(uint32_t sectionId, uint32_t sectionPos) {

    if (sections.find(sectionId) == sections.end()) {
        throw std::range_error("Section does not exist: " + std::to_string(sectionId));
    }

    if (sectionPos >= sections[sectionId].size()) {
        throw std::range_error("Section pos too big. There are " + std::to_string(sections[sectionId].size()) + " and it's trying to access section: " + std::to_string(sectionPos));
    }

    return sections[sectionId][sectionPos].size;
}

uint32_t BinFile::readU32LE() {
    const uint64_t new_pos = pos + 4;

    if (new_pos > size) {
        throw std::range_error("File pos is too big. There are " + std::to_string(size) + " bytes and it's trying to access byte " + std::to_string(new_pos));
    }

    uint32_t res = *((uint32_t *)((uint64_t)addr + pos));
    pos = new_pos;
    return res;
}

uint64_t BinFile::readU64LE() {
    const uint64_t new_pos = pos + 8;

    if (new_pos > size) {
        throw std::range_error("File pos is too big. There are " + std::to_string(size) + " bytes and it's trying to access byte " + std::to_string(new_pos));
    }

    uint64_t res = *((uint64_t *)((uint64_t)addr + pos));
    pos = new_pos;
    return res;
}

void *BinFile::read(uint64_t len) {
    const uint64_t new_pos = pos + len;

    if (new_pos > size) {
        throw std::range_error("File pos is too big. There are " + std::to_string(size) + " bytes and it's trying to access byte " + std::to_string(new_pos));
    }

    void *res = (void *)((uint64_t)addr + pos);
    pos = new_pos;
    return res;
}

std::unique_ptr<BinFile> openExisting(const std::string& filename, const std::string& type, uint32_t maxVersion) {
    return std::unique_ptr<BinFile>(new BinFile(filename, type, maxVersion));
}

} // Namespace

