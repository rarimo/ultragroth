#ifndef GROTH16_HPP
#define GROTH16_HPP

#include <string>
#include <array>
#include <vector>
#include <tuple>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "fft.hpp"


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
        typename Engine::G1PointAffine Alpha;
        typename Engine::G2PointAffine Beta;
        typename Engine::G2PointAffine Gamma;
        typename Engine::G2PointAffine Delta;
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
    class Round {

        Engine &E;
        uint32_t nVars;
        uint32_t nPublic;
        uint32_t domainSize;
        uint64_t nCoefs;
        typename Engine::G1PointAffine &vk_alpha1;
        typename Engine::G1PointAffine &vk_beta1;
        typename Engine::G2PointAffine &vk_beta2;
        typename Engine::G1PointAffine &final_delta_g1;
        typename Engine::G2PointAffine &final_delta_g2;
        typename Engine::G2PointAffine &round_delta_g1;
        typename Engine::Fr::Element &round_random_factor;
        Coef<Engine> *coefs;
        typename Engine::G1PointAffine *pointsA;
        typename Engine::G1PointAffine *pointsB1;
        typename Engine::G2PointAffine *pointsB2;
        typename Engine::G1PointAffine *pointsC;
        typename Engine::G1PointAffine *pointsH;
        typename Engine::FrElement *wtns;
        uint32_t *wtns_indexes;

        FFT<typename Engine::Fr> *fft;
    public:
        Round(
            Engine &_E, 
            uint32_t _nVars, 
            uint32_t _nPublic, 
            uint32_t _domainSize, 
            uint64_t _nCoefs, 
            typename Engine::G1PointAffine &_vk_alpha1,
            typename Engine::G1PointAffine &_vk_beta1,
            typename Engine::G2PointAffine &_vk_beta2,
            typename Engine::G1PointAffine &_final_delta_g1,
            typename Engine::G2PointAffine &_final_delta_g2,
            typename Engine::G2PointAffine &_round_delta_g1,
            typename Engine::Fr::Element &_round_random_factor,
            Coef<Engine> *_coefs, 
            typename Engine::G1PointAffine *_pointsA,
            typename Engine::G1PointAffine *_pointsB1,
            typename Engine::G2PointAffine *_pointsB2,
            typename Engine::G1PointAffine *_pointsC,
            typename Engine::G1PointAffine *_pointsH,
            typename Engine::FrElement *_wtns,
            uint32_t *_wtns_indexes
        ) :
            E(_E),
            nVars(_nVars),
            nPublic(_nPublic),
            domainSize(_domainSize),
            nCoefs(_nCoefs),
            vk_alpha1(_vk_alpha1),
            vk_beta1(_vk_beta1),
            vk_beta2(_vk_beta2),
            final_delta_g1(_final_delta_g1),
            final_delta_g2(_final_delta_g2),
            round_delta_g1(_round_delta_g1),
            round_random_factor(_round_random_factor),
            coefs(_coefs),
            pointsA(_pointsA),
            pointsB1(_pointsB1),
            pointsB2(_pointsB2),
            pointsC(_pointsC),
            pointsH(_pointsH),
            wtns(_wtns),
            wtns_indexes(_wtns_indexes)
        {
            fft = new FFT<typename Engine::Fr>(domainSize*2);
        }

        ~Round() {
            delete fft;
        }

        std::tuple<typename Engine::G1PointAffine, typename Engine::G2PointAffine, typename Engine::G1PointAffine>
        execute_round();

        std::tuple<typename Engine::G1PointAffine, typename Engine::G2PointAffine, typename Engine::G1PointAffine>
        execute_final_round();
    };

    template <typename Engine>
    std::unique_ptr<Round<Engine>> prepare_final_round(
        uint32_t nVars, 
        uint32_t nPublic, 
        uint32_t domainSize, 
        uint64_t nCoefs, 
        void *vk_alpha1,
        void *vk_beta1,
        void *vk_beta2,
        void *final_delta_g1,
        void *final_delta_g2,
        void *round_delta_g1,
        void *round_random_factor,
        // array of deltas from and factors r_k (maybe factors k)
        void *coefs,
        void *pointsA,
        void *pointsB1,
        void *pointsB2,
        void *pointsC,
        void *pointsH
    );

    template <typename Engine>
    class Verifier {

        typedef std::vector<typename Engine::Fr::Element> InputsVector;
        typedef std::array<typename Engine::G1Point, 4> G1PointArray;
        typedef std::array<typename Engine::G2Point, 4> G2PointArray;

        Engine &E;

    public:
        Verifier();

        bool verify(
            Proof<Engine> &proof,
            InputsVector &inputs,
            VerificationKey<Engine> &key);

    private:
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
