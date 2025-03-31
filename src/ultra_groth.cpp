#include "ultra_groth.hpp"
#include "random_generator.hpp"
#include "logging.hpp"
#include "misc.hpp"
#include <sstream>
#include <vector>
#include <mutex>

namespace UltraGroth {
    template <typename Engine>
    Output *round1(
        const Input *input,
        typename Engine::FrElement *wtns,
        uint32_t domainSize
    ) {
        ThreadPool &threadPool = ThreadPool::defaultPool();

        LOG_TRACE("Start Multiexp A");
        uint32_t sW = sizeof(wtns[0]);
        typename Engine::G1Point pi_a;
        E.g1.multiMulByScalarMSM(pi_a, pointsA, (uint8_t *)wtns, sW, nVars);
        std::ostringstream ss2;
        ss2 << "pi_a: " << E.g1.toString(pi_a);
        LOG_DEBUG(ss2);
        
        LOG_TRACE("Start Multiexp B1");
        typename Engine::G1Point pib1;
        E.g1.multiMulByScalarMSM(pib1, pointsB1, (uint8_t *)wtns, sW, nVars);
        std::ostringstream ss3;
        ss3 << "pib1: " << E.g1.toString(pib1);
        LOG_DEBUG(ss3);
        
        LOG_TRACE("Start Multiexp B2");
        typename Engine::G2Point pi_b;
        E.g2.multiMulByScalarMSM(pi_b, pointsB2, (uint8_t *)wtns, sW, nVars);
        std::ostringstream ss4;
        ss4 << "pi_b: " << E.g2.toString(pi_b);
        LOG_DEBUG(ss4);
        
        LOG_TRACE("Start Multiexp C");
        typename Engine::G1Point pi_c;
        E.g1.multiMulByScalarMSM(pi_c, pointsC, (uint8_t *)((uint64_t)wtns + (nPublic +1)*sW), sW, nVars-nPublic-1);
        std::ostringstream ss5;
        ss5 << "pi_c: " << E.g1.toString(pi_c);
        LOG_DEBUG(ss5);

        // a b c here is L, R, O matrixes as I understand it
        LOG_TRACE("Start Initializing a b c A");
        auto a = new typename Engine::FrElement[domainSize];
        auto b = new typename Engine::FrElement[domainSize];
        auto c = new typename Engine::FrElement[domainSize];

        threadPool.parallelFor(0, domainSize, [&] (int64_t begin, int64_t end, uint64_t idThread) {
            for (u_int32_t i=begin; i<end; i++) {
                E.fr.copy(a[i], E.fr.zero());
                E.fr.copy(b[i], E.fr.zero());
            }
        });
    
        LOG_TRACE("Processing coefs");
    
        #define NLOCKS 1024
        std::vector<std::mutex> locks(NLOCKS);
    
        threadPool.parallelFor(0, nCoefs, [&] (int64_t begin, int64_t end, uint64_t idThread) {
            for (u_int64_t i=begin; i<end; i++) {
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
        LOG_TRACE("Calculating c");
        threadPool.parallelFor(0, domainSize, [&] (int64_t begin, int64_t end, uint64_t idThread) {
            for (u_int64_t i=begin; i<end; i++) {
                E.fr.mul(
                    c[i],
                    a[i],
                    b[i]
                );
            }
        });
    
        LOG_TRACE("Initializing fft");
        u_int32_t domainPower = fft->log2(domainSize);
    
        LOG_TRACE("Start iFFT A");
        fft->ifft(a, domainSize);
        LOG_TRACE("a After ifft:");
        LOG_DEBUG(E.fr.toString(a[0]).c_str());
        LOG_DEBUG(E.fr.toString(a[1]).c_str());
        LOG_TRACE("Start Shift A");
    
        threadPool.parallelFor(0, domainSize, [&] (int64_t begin, int64_t end, uint64_t idThread) {
            for (u_int64_t i=begin; i<end; i++) {
                E.fr.mul(a[i], a[i], fft->root(domainPower+1, i));
            }
        });
    
        LOG_TRACE("a After shift:");
        LOG_DEBUG(E.fr.toString(a[0]).c_str());
        LOG_DEBUG(E.fr.toString(a[1]).c_str());
        LOG_TRACE("Start FFT A");
        fft->fft(a, domainSize);
        fft->ifft(b, domainSize);
        threadPool.parallelFor(0, domainSize, [&] (int64_t begin, int64_t end, uint64_t idThread) {
            for (u_int64_t i=begin; i<end; i++) {
                E.fr.mul(b[i], b[i], fft->root(domainPower+1, i));
            }
        });
        
        fft->fft(b, domainSize);
        fft->ifft(c, domainSize);
        threadPool.parallelFor(0, domainSize, [&] (int64_t begin, int64_t end, uint64_t idThread) {
            for (u_int64_t i=begin; i<end; i++) {
                E.fr.mul(c[i], c[i], fft->root(domainPower+1, i));
            }
        });
        LOG_TRACE("c After shift:");
        LOG_DEBUG(E.fr.toString(c[0]).c_str());
        LOG_DEBUG(E.fr.toString(c[1]).c_str());
        LOG_TRACE("Start FFT C");
        fft->fft(c, domainSize);
        LOG_TRACE("c After fft:");
        LOG_DEBUG(E.fr.toString(c[0]).c_str());
        LOG_DEBUG(E.fr.toString(c[1]).c_str());
    
        LOG_TRACE("Start ABC");
        threadPool.parallelFor(0, domainSize, [&] (int64_t begin, int64_t end, uint64_t idThread) {
            for (u_int64_t i=begin; i<end; i++) {
                E.fr.mul(a[i], a[i], b[i]);
                E.fr.sub(a[i], a[i], c[i]);
                E.fr.fromMontgomery(a[i], a[i]);
            }
        });
        LOG_TRACE("abc:");
        LOG_DEBUG(E.fr.toString(a[0]).c_str());
        LOG_DEBUG(E.fr.toString(a[1]).c_str());

        delete [] b;
        delete [] c;

        LOG_TRACE("Start Multiexp H");
        typename Engine::G1Point pih;
        E.g1.multiMulByScalarMSM(pih, pointsH, (uint8_t *)a, sizeof(a[0]), domainSize);
        std::ostringstream ss1;
        ss1 << "pih: " << E.g1.toString(pih);
        LOG_DEBUG(ss1);

        delete [] a;

        typename Engine::FrElement r;
        typename Engine::FrElement s;
        typename Engine::FrElement rs;

        E.fr.copy(r, E.fr.zero());
        E.fr.copy(s, E.fr.zero());

        randombytes_buf((void *)&(r.v[0]), sizeof(r)-1);
        randombytes_buf((void *)&(s.v[0]), sizeof(s)-1);

        typename Engine::G1Point p1;
        typename Engine::G2Point p2;

        E.g1.add(pi_a, pi_a, vk_alpha1);
        E.g1.mulByScalar(p1, vk_delta1, (uint8_t *)&r, sizeof(r));
        E.g1.add(pi_a, pi_a, p1);

        E.g2.add(pi_b, pi_b, vk_beta2);
        E.g2.mulByScalar(p2, vk_delta2, (uint8_t *)&s, sizeof(s));
        E.g2.add(pi_b, pi_b, p2);

        E.g1.add(pib1, pib1, vk_beta1);
        E.g1.mulByScalar(p1, vk_delta1, (uint8_t *)&s, sizeof(s));
        E.g1.add(pib1, pib1, p1);

        E.g1.add(pi_c, pi_c, pih);

        E.g1.mulByScalar(p1, pi_a, (uint8_t *)&s, sizeof(s));
        E.g1.add(pi_c, pi_c, p1);

        E.g1.mulByScalar(p1, pib1, (uint8_t *)&r, sizeof(r));
        E.g1.add(pi_c, pi_c, p1);

        E.fr.mul(rs, r, s);
        E.fr.toMontgomery(rs, rs);

        E.g1.mulByScalar(p1, vk_delta1, (uint8_t *)&rs, sizeof(rs));
        E.g1.sub(pi_c, pi_c, p1);

        // Proof<Engine> *p = new Proof<Engine>(Engine::engine);
        // E.g1.copy(p->A, pi_a);
        // E.g2.copy(p->B, pi_b);
        // E.g1.copy(p->C, pi_c);

        // Plug for now
        return nullptr;
    }

    Output *round2(const Input *input) {

        // Plug for now too
        return nullptr;
    }
}