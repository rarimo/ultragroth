#include <stdexcept>
#include <tuple>
#include <nlohmann/json.hpp>
#include <vector>
#include <fstream>
#include <string>

#include "zkey_utils.hpp"


using json = nlohmann::json;

namespace ZKeyUtils {

Header::Header() {
    mpz_init(qPrime);
    mpz_init(rPrime);
}

Header::~Header() {
    mpz_clear(qPrime);
    mpz_clear(rPrime);
}

std::tuple<std::vector<uint32_t>, std::vector<uint32_t>> load_indexes(std::string path) {

    std::ifstream file("data.json");
    if (!file.is_open()) {
        throw std::invalid_argument( "Failed to open JSON" );
    }

    json data;
    file >> data;

    std::vector<uint32_t> c1 = data["c1"].get<std::vector<uint32_t>>();
    std::vector<uint32_t> c2 = data["c2"].get<std::vector<uint32_t>>();

    return {c1, c2};
}


std::unique_ptr<Header> loadHeader(BinFileUtils::BinFile *f) {

    std::unique_ptr<Header> h(new Header());

    f->startReadSection(1);
    uint32_t protocol = f->readU32LE();
    if (protocol != 1) {
        throw std::invalid_argument( "zkey file is not groth16" );
    }
    f->endReadSection();

    f->startReadSection(2);

    h->n8q = f->readU32LE();
    mpz_import(h->qPrime, h->n8q, -1, 1, -1, 0, f->read(h->n8q));

    h->n8r = f->readU32LE();
    mpz_import(h->rPrime, h->n8r , -1, 1, -1, 0, f->read(h->n8r));

    h->nVars = f->readU32LE();
    h->nPublic = f->readU32LE();
    h->domainSize = f->readU32LE();

    h->vk_alpha1 = f->read(h->n8q*2);
    h->vk_beta1 = f->read(h->n8q*2);
    h->vk_beta2 = f->read(h->n8q*4);
    h->vk_gamma2 = f->read(h->n8q*4);
    h->vk_delta1 = f->read(h->n8q*2);
    h->vk_delta2 = f->read(h->n8q*4);
    f->endReadSection();

    h->nCoefs = f->getSectionSize(4) / (12 + h->n8r);

    return h;
}

UltraGrothHeader::UltraGrothHeader() {
    mpz_init(qPrime);
    mpz_init(rPrime);
}

UltraGrothHeader::~UltraGrothHeader() {
    mpz_clear(qPrime);
    mpz_clear(rPrime);
}

/*
    Header(1)
        prover_type (1337 for UltraGroth)
    HeaderGroth(2)
        n8q (the number of bytes needed to hold field order)
        q (field order)
        n8r (the number of bytes needed to hold group order)
        r (group order)
        n_vars (number of all the witness vars)
        n_pubs (number of public inputs + outputs)
        domain_size (2 ** (log2(n_constraints + n_pubs) + 1))
        n_indexes_c1
        n_indexes_c2
        rand_indx
        alpha_1
        beta_1
        beta_2
        gamma_2
        delta_c1_1
        delta_c1_2
        delta_c2_1
        delta_c2_2
    IC(3)
    Coeffs(4)
    PointsA(5)
    PointsB1(6)
    PointsB2(7)
    PointsC1(8)
    PointsC2(9)
    IndexesC1(10)
    IndexesC2(11)
    PointsH(12)
    Contributions(13)
*/

std::unique_ptr<UltraGrothHeader> ultra_groth_loadHeader(BinFileUtils::BinFile *f) {

    std::unique_ptr<UltraGrothHeader> h(new UltraGrothHeader());

    f->startReadSection(1);
    uint32_t protocol = f->readU32LE();
    if (protocol != 1337) {
        throw std::invalid_argument( "zkey file is not ultragroth" );
    }
    f->endReadSection();

    f->startReadSection(2);

    h->n8q = f->readU32LE();
    mpz_import(h->qPrime, h->n8q, -1, 1, -1, 0, f->read(h->n8q));

    h->n8r = f->readU32LE();
    mpz_import(h->rPrime, h->n8r , -1, 1, -1, 0, f->read(h->n8r));

    h->nVars = f->readU32LE();
    h->nPublic = f->readU32LE();
    h->domainSize = f->readU32LE();

    h->num_indexes_c1 = f->readU32LE();
    h->num_indexes_c2 = f->readU32LE();
    h->rand_indx = f->readU32LE();

    h->alpha1 = f->read(h->n8q*2);
    h->beta1 = f->read(h->n8q*2);
    h->beta2 = f->read(h->n8q*4);
    h->gamma2 = f->read(h->n8q*4);
    h->round_delta1 = f->read(h->n8q*2);
    h->round_delta2 = f->read(h->n8q*4);
    h->final_delta1 = f->read(h->n8q*2);
    h->final_delta2 = f->read(h->n8q*4);
    f->endReadSection();

    h->nCoefs = f->getSectionSize(4) / (12 + h->n8r);

    return h;
}

} // namespace

