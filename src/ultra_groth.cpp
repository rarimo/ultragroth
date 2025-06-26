#include <sstream>
#include <fstream>
#include <vector>
#include <mutex>
#include <tuple>
#include <memory>
#include <gmp.h>
#include <cstring>
#include "../build/fr.hpp"
#include <chrono>

#include <keccak256.h>
#include <nlohmann/json.hpp>
#include <fstream>

#include "random_generator.hpp"
#include "misc.hpp"

using json = nlohmann::json;


namespace UltraGroth {

static void copy_digits(RawFr::Element a, uint64_t *dest) {
    RawFr::Element tmp;
    Fr_rawFromMontgomery(tmp.v, a.v);
    
    for (int i = 0; i < 4; i++){
        dest[i] = tmp.v[i];
    }
}
template <typename Engine>
typename Engine::FrElement derive_challenge(Engine& E, typename Engine::G1PointAffine round_commitment)
{
    // Load x and y coordinates of commitment to buffer
    uint8_t buffer[2 * 32];
    uint8_t challenge[32];

    mpz_t coordinate_buffer;
    mpz_init(coordinate_buffer);

    E.f1.toMpz(coordinate_buffer, round_commitment.x);
    mpz_export(buffer + 0, NULL, 1, 8, 1, 0, coordinate_buffer);

    E.f1.toMpz(coordinate_buffer, round_commitment.y);
    mpz_export(buffer + 32, NULL, 1, 8, 1, 0, coordinate_buffer);

    FIPS202_KECCAK_256(buffer, 32*2, challenge);

    // Load challenge bytes into FrElement to perform mod reduce
    typename Engine::FrElement rand;
    mpz_t x;
    mpz_init(x);
    mpz_import(x, 32, 0, 1, -1, 0, challenge);
    E.fr.fromMpz(rand, x);

    return rand;
}


/// Computes lookup signals and writes them to witness
static void compute_lookup(AltBn128::Engine::FrElement* signals, LookupInfo &info, RawFr::Element rand) {
    uint32_t *chunks = info.chunks;
    uint32_t *frequencies = info.frequencies;
    uint32_t chunks_total = info.chunks_len;
    uint32_t lookup_size = info.frequencies_len;

    uint64_t *inv1_digits = new uint64_t[chunks_total << 2];
    uint64_t *inv2_digits = new uint64_t[lookup_size << 2];
    uint64_t *prod_digits = new uint64_t[lookup_size << 2];

    for (int i = 0; i < lookup_size; i++) {
        RawFr::Element sum = RawFr::field.add(i, rand);
        RawFr::Element inv;
        RawFr::field.inv(inv, sum);
        copy_digits(inv, &inv2_digits[i << 2]);
        RawFr::Element prod = RawFr::field.mul(frequencies[i], inv);
        copy_digits(prod, &prod_digits[i << 2]);
    }

    for (int i = 0; i < chunks_total; i++) {
        memcpy(&inv1_digits[i << 2], &inv2_digits[chunks[i] << 2], sizeof(uint64_t) * 4);
    }
    uint64_t rand_digits[4];
    copy_digits(rand, rand_digits);

    uint64_t offset = 0;
    uint64_t *push_vector = new uint64_t[(2 * lookup_size + chunks_total + 1) * 4];

    memcpy(&push_vector[offset], rand_digits, 32);
    offset += 4;
    memcpy(&push_vector[offset], inv1_digits, chunks_total * 4 * sizeof(uint64_t));
    offset += chunks_total * 4;
    memcpy(&push_vector[offset], inv2_digits, lookup_size * 4 * sizeof(uint64_t));
    offset += lookup_size * 4;
    memcpy(&push_vector[offset],prod_digits, lookup_size * 4 * sizeof(uint64_t));
    offset += lookup_size * 4;

    for (uint32_t i = 0; i < info.wtns_indxs_len; i++) {

        uint32_t wtns_ind = info.wtns_indxs[i];
        uint32_t push_ind = info.push_indxs[i];

        memcpy(&signals[wtns_ind], &push_vector[push_ind << 2], 4 * sizeof(uint64_t));
    }
}

template <typename Engine>
std::unique_ptr<Prover<Engine>> makeProver(
    uint32_t nVars,
    uint32_t nPublic,
    uint32_t domainSize,
    uint64_t nCoefs,
    void *round_indexes,
    uint32_t round_indexes_count,
    void *final_round_indexes,
    uint32_t final_round_indexes_count,
    uint32_t challenge_index,
    void *vk_alpha1,
    void *vk_beta1,
    void *vk_beta2,
    void *final_delta1,
    void *final_delta2,
    void *round_delta1,
    void *coefs,
    void *pointsA,
    void *pointsB1,
    void *pointsB2,
    void *final_pointsC,
    void *round_pointsC,
    void *pointsH
) {
    Prover<Engine> *p = new Prover<Engine>(
        Engine::engine,
        nVars,
        nPublic,
        domainSize,
        nCoefs,
        (uint32_t *)round_indexes,
        round_indexes_count,
        (uint32_t *)final_round_indexes,
        final_round_indexes_count,
        challenge_index,
        *(typename Engine::G1PointAffine *)vk_alpha1,
        *(typename Engine::G1PointAffine *)vk_beta1,
        *(typename Engine::G2PointAffine *)vk_beta2,
        *(typename Engine::G1PointAffine *)final_delta1,
        *(typename Engine::G2PointAffine *)final_delta2,
        *(typename Engine::G1PointAffine *)round_delta1,
        (Coef<Engine> *)((uint64_t)coefs + 4),
        (typename Engine::G1PointAffine *)pointsA,
        (typename Engine::G1PointAffine *)pointsB1,
        (typename Engine::G2PointAffine *)pointsB2,
        (typename Engine::G1PointAffine *)final_pointsC,
        (typename Engine::G1PointAffine *)round_pointsC,
        (typename Engine::G1PointAffine *)pointsH
    );
    return std::unique_ptr< Prover<Engine> >(p);
}

template <typename Engine>
std::tuple<typename Engine::G1PointAffine, typename Engine::FrElement>
Prover<Engine>::execute_round(
    const typename Engine::FrElement *round_wtns, const uint64_t wtns_count
) {
    uint32_t sW = sizeof(round_wtns[0]);
    typename Engine::G1Point commitment_projective;
    E.g1.multiMulByScalarMSM(commitment_projective, round_pointsC, (uint8_t *)round_wtns, sW, wtns_count);
    
    typename Engine::FrElement r;
    typename Engine::G1Point tmp;
    E.fr.copy(r, E.fr.zero());
    randombytes_buf((void *)&(r.v[0]), sizeof(r)-1);
    
    // Add blinding factor r_k
    E.g1.mulByScalar(tmp, final_delta1, (uint8_t *)&r, sizeof(r));
    E.g1.add(commitment_projective, commitment_projective, tmp);

    // Convert commitment back to affine
    typename Engine::G1PointAffine commitment;
    E.g1.copy(commitment, commitment_projective);

    return {commitment, r};
}


template <typename Engine>
std::tuple<typename Engine::G1PointAffine, typename Engine::G2PointAffine, typename Engine::G1PointAffine>
Prover<Engine>::execute_final_round(
    typename Engine::FrElement *wtns, 
    typename Engine::FrElement *final_wtns,
    typename Engine::FrElement round_random_factor
) {
    ThreadPool &threadPool = ThreadPool::defaultPool();

    uint32_t sW = sizeof(wtns[0]);
    typename Engine::G1Point pi_a;

    std::cout << "nVars: " << nVars << std::endl;

    auto start_msm1 = std::chrono::high_resolution_clock::now();

    E.g1.multiMulByScalarMSM(pi_a, pointsA, (uint8_t *)wtns, sW, nVars);

    auto end_msm1 = std::chrono::high_resolution_clock::now();

    auto duration_msm1 = std::chrono::duration_cast<std::chrono::milliseconds>(end_msm1 - start_msm1);
    std::cout << "MSM1 taken: " << duration_msm1.count() << " milliseconds" << std::endl;

    typename Engine::G1Point pib1;
    
    auto start_msm2 = std::chrono::high_resolution_clock::now();

    E.g1.multiMulByScalarMSM(pib1, pointsB1, (uint8_t *)wtns, sW, nVars);

    auto end_msm2 = std::chrono::high_resolution_clock::now();

    auto duration_msm2 = std::chrono::duration_cast<std::chrono::milliseconds>(end_msm2 - start_msm2);
    std::cout << "MSM2 taken: " << duration_msm2.count() << " milliseconds" << std::endl;

    auto start_msm3 = std::chrono::high_resolution_clock::now();

    typename Engine::G2Point pi_b;
    E.g2.multiMulByScalarMSM(pi_b, pointsB2, (uint8_t *)wtns, sW, nVars);

    auto end_msm3 = std::chrono::high_resolution_clock::now();

    auto duration_msm3 = std::chrono::duration_cast<std::chrono::milliseconds>(end_msm3 - start_msm3);
    std::cout << "MSM3 taken: " << duration_msm3.count() << " milliseconds" << std::endl;

    auto start_msm4 = std::chrono::high_resolution_clock::now();

    typename Engine::G1Point pi_c;
    E.g1.multiMulByScalarMSM(pi_c, final_pointsC, (uint8_t *)final_wtns, sW, final_round_indexes_count);

    auto end_msm4 = std::chrono::high_resolution_clock::now();

    auto duration_msm4 = std::chrono::duration_cast<std::chrono::milliseconds>(end_msm4 - start_msm4);
    std::cout << "MSM4 taken: " << duration_msm4.count() << " milliseconds" << std::endl;

    auto start_fft = std::chrono::high_resolution_clock::now();

    auto a = new typename Engine::FrElement[domainSize];
    auto b = new typename Engine::FrElement[domainSize];
    auto c = new typename Engine::FrElement[domainSize];

    threadPool.parallelFor(0, domainSize, [&] (int64_t begin, int64_t end, uint64_t idThread) {
        for (uint32_t i=begin; i<end; i++) {
            E.fr.copy(a[i], E.fr.zero());
            E.fr.copy(b[i], E.fr.zero());
        }
    });

    // Following code computes sum_k {h_k * Z_k}
    #define NLOCKS 1024
    std::vector<std::mutex> locks(NLOCKS);

    threadPool.parallelFor(0, nCoefs, [&] (int64_t begin, int64_t end, uint64_t idThread) {
        for (uint64_t i=begin; i<end; i++) {
            typename Engine::FrElement *ab = (coefs[i].m == 0) ? a : b;
            typename Engine::FrElement aux;

            E.fr.mul(
                aux,
                wtns[coefs[i].s],
                coefs[i].coef
            );

            std::lock_guard<std::mutex> guard(locks[coefs[i].c % NLOCKS]);

            E.fr.add(
                ab[coefs[i].c],
                ab[coefs[i].c],
                aux
            );
        }
    });
    threadPool.parallelFor(0, domainSize, [&] (int64_t begin, int64_t end, uint64_t idThread) {
        for (uint64_t i=begin; i<end; i++) {
            E.fr.mul(
                c[i],
                a[i],
                b[i]
            );
        }
    });

    uint32_t domainPower = fft->log2(domainSize);
    fft->ifft(a, domainSize);
    threadPool.parallelFor(0, domainSize, [&] (int64_t begin, int64_t end, uint64_t idThread) {
        for (uint64_t i=begin; i<end; i++) {
            E.fr.mul(a[i], a[i], fft->root(domainPower+1, i));
        }
    });
    fft->fft(a, domainSize);
    fft->ifft(b, domainSize);
    threadPool.parallelFor(0, domainSize, [&] (int64_t begin, int64_t end, uint64_t idThread) {
        for (uint64_t i=begin; i<end; i++) {
            E.fr.mul(b[i], b[i], fft->root(domainPower+1, i));
        }
    });
    fft->fft(b, domainSize);
    fft->ifft(c, domainSize);
    threadPool.parallelFor(0, domainSize, [&] (int64_t begin, int64_t end, uint64_t idThread) {
        for (uint64_t i=begin; i<end; i++) {
            E.fr.mul(c[i], c[i], fft->root(domainPower+1, i));
        }
    });
    fft->fft(c, domainSize);
    threadPool.parallelFor(0, domainSize, [&] (int64_t begin, int64_t end, uint64_t idThread) {
        for (uint64_t i=begin; i<end; i++) {
            E.fr.mul(a[i], a[i], b[i]);
            E.fr.sub(a[i], a[i], c[i]);
            E.fr.fromMontgomery(a[i], a[i]);
        }
    });

    delete [] b;
    delete [] c;

    auto end_fft = std::chrono::high_resolution_clock::now();

    auto duration_fft = std::chrono::duration_cast<std::chrono::milliseconds>(end_fft - start_fft);
    std::cout << "FFT taken: " << duration_fft.count() << " milliseconds" << std::endl;

    auto start_msm5 = std::chrono::high_resolution_clock::now();

    typename Engine::G1Point pih;
    E.g1.multiMulByScalarMSM(pih, pointsH, (uint8_t *)a, sizeof(a[0]), domainSize);
    delete [] a;

    auto end_msm5 = std::chrono::high_resolution_clock::now();

    auto duration_msm5 = std::chrono::duration_cast<std::chrono::milliseconds>(end_msm5 - start_msm5);
    std::cout << "MSM5 taken: " << duration_msm5.count() << " milliseconds" << std::endl;

    // initializing variables for blinding factors
    typename Engine::FrElement r;
    typename Engine::FrElement s;
    typename Engine::FrElement rs;

    E.fr.copy(r, E.fr.zero());
    E.fr.copy(s, E.fr.zero());

    randombytes_buf((void *)&(r.v[0]), sizeof(r)-1);
    randombytes_buf((void *)&(s.v[0]), sizeof(s)-1);

    // tmp variables for storing products
    typename Engine::G1Point p1;
    typename Engine::G2Point p2;

    // Add to r * [delta_1]_g1 to first proof point
    E.g1.add(pi_a, pi_a, alpha1);
    E.g1.mulByScalar(p1, final_delta1, (uint8_t *)&r, sizeof(r));
    E.g1.add(pi_a, pi_a, p1);

    // Add to s * [delta_2]_g2 to second proof point
    E.g2.add(pi_b, pi_b, beta2);
    E.g2.mulByScalar(p2, final_delta2, (uint8_t *)&s, sizeof(s));
    E.g2.add(pi_b, pi_b, p2);

    // Add to s * [delta_1]_g1 to auxiliar proof point
    E.g1.add(pib1, pib1, beta1);
    E.g1.mulByScalar(p1, final_delta1, (uint8_t *)&s, sizeof(s));
    E.g1.add(pib1, pib1, p1);

    // Add target polynomial sum to third proof point
    E.g1.add(pi_c, pi_c, pih);

    // Add s * first_point to third proof point
    E.g1.mulByScalar(p1, pi_a, (uint8_t *)&s, sizeof(s));
    E.g1.add(pi_c, pi_c, p1);

    // Add auxiliary point to third proof point
    E.g1.mulByScalar(p1, pib1, (uint8_t *)&r, sizeof(r));
    E.g1.add(pi_c, pi_c, p1);

    // Mutliply r and s and convert to montgomery form
    E.fr.mul(rs, r, s);
    E.fr.toMontgomery(rs, rs);

    // Subtract from third proof point r * [delta_final_1]_g1
    E.g1.mulByScalar(p1, final_delta1, (uint8_t *)&rs, sizeof(rs));
    E.g1.sub(pi_c, pi_c, p1);

    // Subtract from third proof point round randomness * round commitment
    E.g1.mulByScalar(p1, round_delta1, (uint8_t *)&round_random_factor, sizeof(round_random_factor));
    E.g1.sub(pi_c, pi_c, p1);

    // Convert to affine
    typename Engine::G1PointAffine A;    
    typename Engine::G2PointAffine B;    
    typename Engine::G1PointAffine C;    
    E.g1.copy(A, pi_a);
    E.g2.copy(B, pi_b);
    E.g1.copy(C, pi_c);

    return {A, B, C};
};

template <typename Engine>
std::unique_ptr<Proof<Engine>> Prover<Engine>::prove(
    typename Engine::FrElement* wtns, LookupInfo &lookupInfo
) {

    Proof<Engine> *p = new Proof<Engine>(Engine::engine);
    p->error = nullptr;
    p->error_size = 0;
    
    // initialization
    typename Engine::FrElement round_random_factor;
    typename Engine::G1PointAffine round_commitment;
    
    // here cloning of appropriate part of witness for first round should happen
    typename Engine::FrElement *round_wtns = new typename Engine::FrElement[round_indexes_count];

    for (uint32_t i = 0; i < round_indexes_count; i++) {
        round_wtns[i] = wtns[round_indexes[i]];
    }

    std::tuple<typename Engine::G1PointAffine, typename Engine::FrElement> round_result = execute_round(round_wtns, round_indexes_count);
    
    round_commitment = std::get<0>(round_result);
    round_random_factor = std::get<1>(round_result);
    delete[] round_wtns;
    
    // Hash point to derive challenge
    typename Engine::FrElement rand = derive_challenge<Engine>(E, round_commitment);

    // Unload challenge bytes from FrElement
    uint8_t challenge[32];
    mpz_t x;
    mpz_init(x);
    E.fr.toMpz(x, rand);
    mpz_export(challenge, 0, -1, 8, -1, 0, x);

    compute_lookup(wtns, lookupInfo, rand);

    typename Engine::FrElement *final_round_wtns = new typename Engine::FrElement[final_round_indexes_count];

    // Convert witness from uint64 to FrElement
    for (uint32_t i = 0; i < final_round_indexes_count; i++) {
        uint32_t index = final_round_indexes[i];
        final_round_wtns[i] = wtns[index]; 
    }

    auto final_round_result = execute_final_round(
        wtns,
        final_round_wtns,
        round_random_factor
    );
    
    //witness_from_digits((uint64_t *)wtnsData->signals, nVars);
    delete[] final_round_wtns;

    E.g1.copy(p->A, std::get<0>(final_round_result));
    E.g2.copy(p->B, std::get<1>(final_round_result));
    E.g1.copy(p->final_commitment, std::get<2>(final_round_result));
    E.g1.copy(p->round_commitment, round_commitment);

    return std::unique_ptr<Proof<Engine>>(p);
}


template <typename Engine>
std::string Proof<Engine>::toJsonStr()
{
    std::ostringstream ss;
    ss << "{ \"pi_a\":[\"" << E.f1.toString(A.x) << "\",\"" << E.f1.toString(A.y) << "\",\"1\"], ";
    ss << " \"pi_b\": [[\"" << E.f1.toString(B.x.a) << "\",\"" << E.f1.toString(B.x.b) << "\"],[\"" << E.f1.toString(B.y.a) << "\",\"" << E.f1.toString(B.y.b) << "\"], [\"1\",\"0\"]], ";
    ss << " \"pi_c\": [\"" << E.f1.toString(final_commitment.x) << "\",\"" << E.f1.toString(final_commitment.y) << "\",\"1\"], ";
    ss << " \"protocol\":\"ultragroth\" }";
    return ss.str();
}

template <typename Engine>
json Proof<Engine>::toJson()
{
    json p;

    p["pi_a"] = {};
    p["pi_a"].push_back(E.f1.toString(A.x));
    p["pi_a"].push_back(E.f1.toString(A.y));
    p["pi_a"].push_back("1" );

    json x2;
    x2.push_back(E.f1.toString(B.x.a));
    x2.push_back(E.f1.toString(B.x.b));
    json y2;
    y2.push_back(E.f1.toString(B.y.a));
    y2.push_back(E.f1.toString(B.y.b));
    json z2;
    z2.push_back("1");
    z2.push_back("0");
    p["pi_b"] = {};
    p["pi_b"].push_back(x2);
    p["pi_b"].push_back(y2);
    p["pi_b"].push_back(z2);

    p["pi_f"] = {};
    p["pi_f"].push_back(E.f1.toString(final_commitment.x));
    p["pi_f"].push_back(E.f1.toString(final_commitment.y));
    p["pi_f"].push_back("1");

    p["pi_r"] = {};
    p["pi_r"].push_back(E.f1.toString(round_commitment.x));
    p["pi_r"].push_back(E.f1.toString(round_commitment.y));
    p["pi_r"].push_back("1");

    p["protocol"] = "ultragroth";
            
    return p;
}

template <typename Engine>
static void
G1PointAffineFromJson(Engine &E, typename Engine::G1PointAffine &point, const json &value)
{
    E.f1.fromString(point.x, value[0]);
    E.f1.fromString(point.y, value[1]);
}

template <typename Engine>
static void
G2PointAffineFromJson(Engine &E, typename Engine::G2PointAffine &point, const json &value)
{
    E.f1.fromString(point.x.a, value[0][0]);
    E.f1.fromString(point.x.b, value[0][1]);
    E.f1.fromString(point.y.a, value[1][0]);
    E.f1.fromString(point.y.b, value[1][1]);
}

template <typename Engine>
void Proof<Engine>::fromJson(const json& proof)
{
    G1PointAffineFromJson(E, A, proof["pi_a"]);
    G2PointAffineFromJson(E, B, proof["pi_b"]);
    G1PointAffineFromJson(E, final_commitment, proof["pi_f"]);
    G1PointAffineFromJson(E, round_commitment, proof["pi_r"]);
}

template <typename Engine>
void VerificationKey<Engine>::fromJson(const json& key)
{
    G1PointAffineFromJson(E, alpha1, key["vk_alpha_1"]);
    G2PointAffineFromJson(E, beta2,  key["vk_beta_2"]);
    G2PointAffineFromJson(E, gamma2, key["vk_gamma_2"]);
    G2PointAffineFromJson(E, final_delta2, key["vk_delta_c2_2"]);
    G2PointAffineFromJson(E, round_delta2, key["vk_delta_c1_2"]);

    auto j_ic = key["IC"];

    IC.reserve(j_ic.size());

    for (const auto& el : j_ic.items()) {
        typename Engine::G1PointAffine point;

        G1PointAffineFromJson(E, point, el.value());
        IC.push_back(point);
    }
    G1PointAffineFromJson(E, ic_rand, key["IC_rand"]);
    //challenge_index = key["randIdx"];
}

template <typename Engine>
Verifier<Engine>::Verifier():
    E(Engine::engine)
{
    E.f2.fromString(xiToPMinus1Over3,
                    "21575463638280843010398324269430826099269044274347216827212613867836435027261,"
                    "10307601595873709700152284273816112264069230130616436755625194854815875713954");

    E.f2.fromString(xiToPMinus1Over2,
                    "2821565182194536844548159561693502659359617185244120367078079554186484126554,"
                    "3505843767911556378687030309984248845540243509899259641013678093033130930403");

    E.f1.fromString(xiToPSquaredMinus1Over3,
                    "21888242871839275220042445260109153167277707414472061641714758635765020556616");
}

template <typename Engine>
bool Verifier<Engine>::verify(Proof<Engine> &proof, InputsVector &inputs,
                             VerificationKey<Engine> &key)
{
    if (inputs.size() + 1!= key.IC.size()) {
        throw std::invalid_argument("len(inputs) != len(vk.IC)");
    }

    typename Engine::G1Point vkX = E.g1.zero();

    std::cout << "inputs.size(): " << inputs.size() << std::endl;
    for (int i = 0; i < inputs.size(); i++) {

        typename Engine::FrElement input;

        E.fr.fromMontgomery(input, inputs[i]);

        typename Engine::G1Point p1;
        E.g1.mulByScalar(p1, key.IC[i+1], (uint8_t *)&input, sizeof(input));
        E.g1.add(vkX, vkX, p1);
    }

    typename Engine::FrElement derived_rand = derive_challenge(E, proof.round_commitment);

    typename Engine::FrElement rand_casted;
    E.fr.fromMontgomery(rand_casted, derived_rand);

    typename Engine::G1Point p;
    E.g1.mulByScalar(p, key.ic_rand, (uint8_t *)&rand_casted, sizeof(derived_rand));
    
    E.g1.add(vkX, vkX, key.IC[0]);
    E.g1.add(vkX, vkX, p);

    typename Engine::G1Point pA;
    E.g1.copy(pA, proof.A);

    typename Engine::G1Point negAlpha;
    E.g1.neg(negAlpha, key.alpha1);

    typename Engine::G1Point negvkX;
    E.g1.neg(negvkX, vkX);

    typename Engine::G1Point negFinalCommit;
    E.g1.neg(negFinalCommit, proof.final_commitment);

    typename Engine::G1Point negRoundCommit;
    E.g1.neg(negRoundCommit, proof.round_commitment);

    typename Engine::G2Point pB;
    E.g2.copy(pB, proof.B);

    typename Engine::G2Point pBeta;
    E.g2.copy(pBeta, key.beta2);

    typename Engine::G2Point pGamma;
    E.g2.copy(pGamma, key.gamma2);

    typename Engine::G2Point pFinalDelta;
    E.g2.copy(pFinalDelta, key.final_delta2);

    typename Engine::G2Point pRoundDelta;
    E.g2.copy(pRoundDelta, key.round_delta2);

    G1PointArray g1 = {pA, negAlpha, negvkX, negFinalCommit, negRoundCommit};
    G2PointArray g2 = {pB, pBeta, pGamma, pFinalDelta, pRoundDelta};

    return pairingCheck(g1, g2);
}

template <typename Engine>
void Verifier<Engine>::lineFunctionAdd(
    typename Engine::G2Point& r,
    typename Engine::G2PointAffine& p,
    typename Engine::G1PointAffine& q,
    typename Engine::F2Element& r2,
    typename Engine::F2Element& a,
    typename Engine::F2Element& b,
    typename Engine::F2Element& c,
    typename Engine::G2Point& rOut)
{
    typename Engine::F2Element B, D, H, I, E1, J, L1, V, t, t2;

    E.f2.mul(B, p.x, r.zzz);
    E.f2.add(D, p.y, r.zz);
    E.f2.square(D, D);
    E.f2.sub(D, D, r2);
    E.f2.sub(D, D, r.zzz);
    E.f2.mul(D, D, r.zzz);

    E.f2.sub(H, B, r.x);
    E.f2.square(I, H);
    E.f2.add(E1, I, I);
    E.f2.add(E1, E1, E1);
    E.f2.mul(J, H, E1);
    E.f2.sub(L1, D, r.y);
    E.f2.sub(L1, L1, r.y);
    E.f2.mul(V, r.x, E1);

    E.f2.square(rOut.x, L1);
    E.f2.sub(rOut.x, rOut.x, J);
    E.f2.sub(rOut.x, rOut.x, V);
    E.f2.sub(rOut.x, rOut.x, V);

    E.f2.add(rOut.zz, r.zz, H);
    E.f2.square(rOut.zz, rOut.zz);
    E.f2.sub(rOut.zz, rOut.zz, r.zzz);
    E.f2.sub(rOut.zz, rOut.zz, I);

    E.f2.sub(t, V, rOut.x);
    E.f2.mul(t, t, L1);
    E.f2.mul(t2, r.y, J);
    E.f2.add(t2, t2, t2);
    E.f2.sub(rOut.y, t, t2);
    E.f2.square(rOut.zzz, rOut.zz);

    E.f2.add(t, p.y, rOut.zz);
    E.f2.square(t, t);
    E.f2.sub(t, t, r2);
    E.f2.sub(t, t, rOut.zzz);

    E.f2.mul(t2, L1, p.x);
    E.f2.add(t2, t2, t2);

    E.f2.sub(a, t2, t);

    E.f2.mulScalar(c, rOut.zz, q.y);
    E.f2.add(c, c, c);

    E.f2.copy(b, E.f2.zero());
    E.f2.sub(b, b, L1);
    E.f2.mulScalar(b, b, q.x);
    E.f2.add(b, b, b);
}

template <typename Engine>
void Verifier<Engine>::lineFunctionDouble(
    typename Engine::G2Point& r,
    typename Engine::G1PointAffine& q,
    typename Engine::F2Element& a,
    typename Engine::F2Element& b,
    typename Engine::F2Element& c,
    typename Engine::G2Point& rOut
) {
    typename Engine::F2Element A, B, C_, D, E1, G, t;

    E.f2.square(A, r.x);
    E.f2.square(B, r.y);
    E.f2.square(C_, B);
    E.f2.add(D, r.x, B);
    E.f2.square(D, D);
    E.f2.sub(D, D, A);
    E.f2.sub(D, D, C_);
    E.f2.add(D, D, D);

    E.f2.add(E1, A, A);
    E.f2.add(E1, E1, A);
    E.f2.square(G, E1);
    E.f2.sub(rOut.x, G, D);
    E.f2.sub(rOut.x, rOut.x, D);

    E.f2.add(rOut.zz, r.y, r.zz);
    E.f2.square(rOut.zz, rOut.zz);
    E.f2.sub(rOut.zz, rOut.zz, B);
    E.f2.sub(rOut.zz, rOut.zz, r.zzz);

    E.f2.sub(rOut.y, D, rOut.x);
    E.f2.mul(rOut.y, rOut.y, E1);

    E.f2.add(t, C_, C_);
    E.f2.add(t, t, t);
    E.f2.add(t, t, t);
    E.f2.sub(rOut.y, rOut.y, t);
    E.f2.square(rOut.zzz, rOut.zz);

    E.f2.mul(t, E1, r.zzz);
    E.f2.add(t, t, t);
    E.f2.copy(b, E.f2.zero());
    E.f2.sub(b, b, t);
    E.f2.mulScalar(b, b, q.x);

    E.f2.add(a, r.x, E1);
    E.f2.square(a, a);
    E.f2.sub(a, a, A);
    E.f2.sub(a, a, G);
    E.f2.add(t, B, B);
    E.f2.add(t, t, t);
    E.f2.sub(a, a, t);

    E.f2.mul(c, rOut.zz, r.zzz);
    E.f2.add(c, c, c);
    E.f2.mulScalar(c, c, q.y);
}

template <typename Engine>
void Verifier<Engine>::mulLine(
    typename Engine::F12Element& ret,
    typename Engine::F2Element& a,
    typename Engine::F2Element& b,
    typename Engine::F2Element& c
) {
    typename Engine::F6Element a2, t3, t2;
    typename Engine::F2Element t;

    E.f2.copy(a2.x, E.f2.zero());
    E.f2.copy(a2.y, a);
    E.f2.copy(a2.z, b);

    E.f6.mul(a2, a2, ret.x);
    E.f6.mulScalar(t3, ret.y, c);

    E.f2.add(t, b, c);
    E.f2.copy(t2.x, E.f2.zero());
    E.f2.copy(t2.y, a);
    E.f2.copy(t2.z, t);
    E.f6.add(ret.x, ret.x, ret.y);

    E.f6.copy(ret.y, t3);

    E.f6.mul(ret.x, ret.x, t2);
    E.f6.sub(ret.x, ret.x, a2);
    E.f6.sub(ret.x, ret.x, ret.y);
    E.f6.mulTau(a2, a2);
    E.f6.add(ret.y, ret.y, a2);
}

template <typename Engine>
typename Engine::F12Element
Verifier<Engine>::miller(typename Engine::G2Point &q, typename Engine::G1Point &p)
{

    const int  sixuPlus2NAF[] =  {0, 0, 0, 1, 0, 1, 0, -1, 0, 0, 1, -1, 0, 0, 1, 0,
                                  0, 1, 1, 0, -1, 0, 0, 1, 0, -1, 0, 0, 0, 0, 1, 1,
                                  1, 0, 0, -1, 0, 0, 1, 0, 0, 0, 0, 0, -1, 0, 0, 1,
                                  1, 0, 0, -1, 0, 0, 0, 1, 1, 0, -1, 0, 0, 1, 0, 1, 1};

    typename Engine::F12Element ret = E.f12.one();

    typename Engine::G2PointAffine aAffine, minusA;
    typename Engine::G1PointAffine bAffine;
    typename Engine::G2Point r, newR;
    typename Engine::F2Element r2, a, b, c;

    E.g2.copy(aAffine, q);
    E.g1.copy(bAffine, p);
    E.g2.neg(minusA, aAffine);
    E.g2.copy(r, aAffine);

    E.f2.square(r2, aAffine.y);

    const size_t count = sizeof(sixuPlus2NAF)/sizeof(sixuPlus2NAF[0]) - 1;

    for (int i = count; i > 0; i--) {

        lineFunctionDouble(r, bAffine, a, b, c, newR);

        if (i != count) {
            E.f12.square(ret, ret);
        }

        mulLine(ret, a, b, c);
        E.g2.copy(r, newR);

        switch (sixuPlus2NAF[i-1]) {
        case 1:
            lineFunctionAdd(r, aAffine, bAffine, r2, a, b, c, newR);
            break;
        case -1:
            lineFunctionAdd(r, minusA, bAffine, r2, a, b, c, newR);
            break;

        default:
            continue;
        }

        mulLine(ret, a, b, c);
        E.g2.copy(r, newR);
    }

    typename Engine::G2Point q1;

    E.f2.conjugate(q1.x, aAffine.x);
    E.f2.mul(q1.x, q1.x, xiToPMinus1Over3);
    E.f2.conjugate(q1.y, aAffine.y);
    E.f2.mul(q1.y, q1.y, xiToPMinus1Over2);
    E.f2.copy(q1.zz, E.f2.one());
    E.f2.copy(q1.zzz, E.f2.one());

    typename Engine::G2Point minusQ2;

    E.f2.mulScalar(minusQ2.x, aAffine.x, xiToPSquaredMinus1Over3);
    E.f2.copy(minusQ2.y, aAffine.y);
    E.f2.copy(minusQ2.zz, E.f2.one());
    E.f2.copy(minusQ2.zzz, E.f2.one());

    E.f2.square(r2, q1.y);

    typename Engine::G2PointAffine q1Affine;
    E.g2.copy(q1Affine, q1);

    lineFunctionAdd(r, q1Affine, bAffine, r2, a, b, c, newR);
    mulLine(ret, a, b, c);
    E.g2.copy(r, newR);

    typename Engine::G2PointAffine minusQ2Affine;
    E.g2.copy(minusQ2Affine, minusQ2);

    E.f2.square(r2, minusQ2.y);
    lineFunctionAdd(r, minusQ2Affine, bAffine, r2, a, b, c, newR);
    mulLine(ret, a, b, c);
    E.g2.copy(r, newR);

    return ret;
}

template <typename Engine>
typename Engine::F12Element
Verifier<Engine>::finalExponentiation(typename Engine::F12Element &in)
{
    typename Engine::F12Element t1, inv, t2, fp, fp2, fp3, fu, fu2, fu3, y3, fu2p, fu3p;
    typename Engine::F12Element y2, y0, y1, y4, y5, y6, t0;

    uint64_t u = 4965661367192848881;

    E.f6.neg(t1.x, in.x);
    E.f6.copy(t1.y, in.y);

    E.f12.inv(inv, in);
    E.f12.mul(t1, t1, inv);

    E.f12.FrobeniusP2(t2, t1);
    E.f12.mul(t1, t1, t2);

    E.f12.Frobenius(fp, t1);
    E.f12.FrobeniusP2(fp2, t1);
    E.f12.Frobenius(fp3, fp2);

    E.f12.exp(fu,  t1, (uint8_t*)&u, sizeof(u));
    E.f12.exp(fu2, fu, (uint8_t*)&u, sizeof(u));
    E.f12.exp(fu3, fu2, (uint8_t*)&u, sizeof(u));

    E.f12.Frobenius(y3, fu);
    E.f12.Frobenius(fu2p, fu2);
    E.f12.Frobenius(fu3p, fu3);
    E.f12.FrobeniusP2(y2, fu2);

    E.f12.mul(y0, fp, fp2);
    E.f12.mul(y0, y0, fp3);

    E.f12.conjugate(y1, t1);
    E.f12.conjugate(y5, fu2);
    E.f12.conjugate(y3, y3);
    E.f12.mul(y4, fu, fu2p);
    E.f12.conjugate(y4, y4);

    E.f12.mul(y6, fu3, fu3p);
    E.f12.conjugate(y6, y6);

    E.f12.square(t0, y6);
    E.f12.mul(t0, t0, y4);
    E.f12.mul(t0, t0, y5);
    E.f12.mul(t1, y3, y5);
    E.f12.mul(t1, t1, t0);
    E.f12.mul(t0, t0, y2);

    E.f12.square(t1, t1);
    E.f12.mul(t1, t1, t0);
    E.f12.square(t1, t1);
    E.f12.mul(t0, t1, y1);
    E.f12.mul(t1, t1, y0);
    E.f12.square(t0, t0);
    E.f12.mul(t0, t0, t1);

    return t0;
}

template <typename Engine>
bool Verifier<Engine>::pairingCheck(G1PointArray &a, G2PointArray &b)
{
    typename Engine::F12Element acc = E.f12.one();

    for (int i = 0; i < a.size(); i++) {

        if (E.g1.isZero(a[i]) || E.g2.isZero(b[i])) {
            continue;
        }

        auto millerRes = miller(b[i], a[i]);
        E.f12.mul(acc, acc, millerRes);
    }

    auto ret = finalExponentiation(acc);
    
    return E.f12.isOne(ret);
}

} // namespace