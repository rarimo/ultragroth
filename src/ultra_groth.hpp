#ifndef ULTRA_GROTH_HPP
#define ULTRA_GROTH_HPP

#include <string>
#include <array>
#include <vector>
#include <tuple>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "fft.hpp"

//Error codes returned by the functions.
#define PROVER_OK                     0x0
#define PROVER_ERROR                  0x1
#define PROVER_ERROR_SHORT_BUFFER     0x2
#define PROVER_INVALID_WITNESS_LENGTH 0x3

namespace UltraGroth {

    template <typename Engine>
    class Proof {
        Engine &E;
    public:
        typename Engine::G1PointAffine A;
        typename Engine::G2PointAffine B;
        typename Engine::G1PointAffine final_commitment;
        typename Engine::G1PointAffine round_commitment;

        Proof(Engine &_E) : E(_E) { }
        std::string toJsonStr();
        json toJson();
        void fromJson(const json &proof);
    };

    template <typename Engine>
    class VerificationKey {
        Engine &E;
    public:
        typename Engine::G1PointAffine alpha1;
        typename Engine::G2PointAffine beta2;
        typename Engine::G2PointAffine gamma2;
        typename Engine::G2PointAffine final_delta2;
        typename Engine::G2PointAffine round_delta2;
        std::vector<typename Engine::G1PointAffine> IC;

        VerificationKey(Engine &_E) : E(_E) { }
        void fromJson(const json &proof);
    };

// TODO Understand what is it?
#pragma pack(push, 1)
    template <typename Engine>
    struct Coef {
        uint32_t m;
        uint32_t c;
        uint32_t s;
        typename Engine::FrElement coef;
    };
#pragma pack(pop)

    template <typename Engine>
    class Prover{

        Engine &E;
        uint32_t nVars;
        uint32_t nPublic;
        uint32_t domainSize;
        uint64_t nCoefs;
        // Indexes to extract witness elements for first round
        uint32_t *round_indexes;
        uint32_t round_indexes_count;
        // Indexes to extract witness elements for final round
        uint32_t *final_round_indexes;
        uint32_t final_round_indexes_count;
        // Toxic waste wrapped into corresponding groups
        typename Engine::G1PointAffine &alpha1;
        typename Engine::G1PointAffine &beta1;
        typename Engine::G2PointAffine &beta2;
        typename Engine::G1PointAffine &final_delta1;
        typename Engine::G2PointAffine &final_delta2;
        typename Engine::G1PointAffine &round_delta1;
        // Probably matrix coefficients
        Coef<Engine> *coefs;
        // [interpolated polynomials from L matrix evaluated in tau]_g1
        typename Engine::G1PointAffine *pointsA;
        // [interpolated polynomials from R matrix evaluated in tau]_g1
        typename Engine::G1PointAffine *pointsB1;
        // [interpolated polynomials from R matrix evaluated in tau]_g2
        typename Engine::G2PointAffine *pointsB2;
        // [(Oj + beta * Lj + alpha * Rj) / delta]_g1 for final round
        typename Engine::G1PointAffine *final_pointsC;
        // [(Oj + beta * Lj + alpha * Rj) / delta]_g1 for common round
        typename Engine::G1PointAffine *round_pointsC;
        // Probably points of target polynomail - [(tau * Z(tau)) / delta]_g1
        typename Engine::G1PointAffine *pointsH;

        FFT<typename Engine::Fr> *fft;
    public:
        Prover(
            Engine &_E,
            uint32_t _nVars,
            uint32_t _nPublic,
            uint32_t _domainSize,
            uint64_t _nCoefs,
            uint32_t *_round_indexes,
            uint32_t _round_indexes_count,
            uint32_t *_final_round_indexes,
            uint32_t _final_round_indexes_count,
            typename Engine::G1PointAffine &_alpha1,
            typename Engine::G1PointAffine &_beta1,
            typename Engine::G2PointAffine &_beta2,
            typename Engine::G1PointAffine &_final_delta1,
            typename Engine::G2PointAffine &_final_delta2,
            typename Engine::G1PointAffine &_round_delta1,
            Coef<Engine> *_coefs,
            typename Engine::G1PointAffine *_pointsA,
            typename Engine::G1PointAffine *_pointsB1,
            typename Engine::G2PointAffine *_pointsB2,
            typename Engine::G1PointAffine *_final_pointsC,
            typename Engine::G1PointAffine *_round_pointsC,
            typename Engine::G1PointAffine *_pointsH
        ):
            E(_E),
            nVars(_nVars),
            nPublic(_nPublic),
            domainSize(_domainSize),
            nCoefs(_nCoefs),
            round_indexes(_round_indexes),
            round_indexes_count(_round_indexes_count),
            final_round_indexes(_final_round_indexes),
            final_round_indexes_count(_final_round_indexes_count),
            alpha1(_alpha1),
            beta1(_beta1),
            beta2(_beta2),
            final_delta1(_final_delta1),
            final_delta2(_final_delta2),
            round_delta1(_round_delta1),
            coefs(_coefs),
            pointsA(_pointsA),
            pointsB1(_pointsB1),
            pointsB2(_pointsB2),
            final_pointsC(_final_pointsC),
            round_pointsC(_round_pointsC),
            pointsH(_pointsH)
        {
            fft = new FFT<typename Engine::Fr>(domainSize*2);
        }

        ~Prover() {
            delete fft;
        }

        // Function to execute entire proving process
        std::unique_ptr<Proof<Engine>> prove(uint8_t *accumulator, const uint8_t *input_path, const uint8_t *sym_path);

        // Function to execute common round of proving process
        // Pointer to accumulator is passed to function; accumulator size is 32
        typename std::tuple<typename Engine::G1PointAffine, typename Engine::FrElement>
        execute_round(const typename Engine::FrElement *round_wtns, const uint64_t wtns_count, uint8_t *accumulator);

        // Function to execute final round of proving process
        std::tuple<typename Engine::G1PointAffine, typename Engine::G2PointAffine, typename Engine::G1PointAffine>
        execute_final_round(typename Engine::FrElement *wtns, typename Engine::FrElement *final_wtns, typename Engine::FrElement round_random_factor);

        void debug_prover_inputs();
    };

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
        void *vk_alpha1,
        void *vk_beta1,
        void *vk_beta2,
        void *vk_delta1,
        void *vk_delta2,
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
    );

    template <typename Engine>
    class Verifier {

        typedef std::vector<typename Engine::Fr::Element> InputsVector;
        typedef std::array<typename Engine::G1Point, 5> G1PointArray;
        typedef std::array<typename Engine::G2Point, 5> G2PointArray;

        Engine &E;

    public:
        Verifier();

        bool verify(
            Proof<Engine> &proof,
            InputsVector &inputs,
            VerificationKey<Engine> &key);

    private:

        typename Engine::F12Element pairing(typename Engine::G1Point a, typename Engine::G2Point b);

        bool challenge_check(InputsVector &inputs, uint8_t *accumulator, typename Engine::G1PointAffine round_commitment, uint32_t challenge_index);

        bool pairingCheck(G1PointArray& g1, G2PointArray& g2);

        typename Engine::F12Element miller(typename Engine::G2Point& b, typename Engine::G1Point& a);

        typename Engine::F12Element finalExponentiation(typename Engine::F12Element& in);

        void lineFunctionDouble(
            typename Engine::G2Point& r,
            typename Engine::G1PointAffine& q,
            typename Engine::F2Element& a,
            typename Engine::F2Element& b,
            typename Engine::F2Element& c,
            typename Engine::G2Point& rOut);

        void lineFunctionAdd(
            typename Engine::G2Point& r,
            typename Engine::G2PointAffine& p,
            typename Engine::G1PointAffine& q,
            typename Engine::F2Element& r2,
            typename Engine::F2Element& a,
            typename Engine::F2Element& b,
            typename Engine::F2Element& c,
            typename Engine::G2Point& rOut);

        void mulLine(
            typename Engine::F12Element& ret,
            typename Engine::F2Element& a,
            typename Engine::F2Element& b,
            typename Engine::F2Element& c);

    private:
        typename Engine::F2Element xiToPMinus1Over3;
        typename Engine::F2Element xiToPMinus1Over2;
        typename Engine::F1Element xiToPSquaredMinus1Over3;
    };
}


#include "ultra_groth.cpp"

#endif
