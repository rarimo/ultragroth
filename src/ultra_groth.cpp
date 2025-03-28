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
        LOG_TRACE("a After fft:");
        LOG_DEBUG(E.fr.toString(a[0]).c_str());
        LOG_DEBUG(E.fr.toString(a[1]).c_str());
        LOG_TRACE("Start iFFT B");
        fft->ifft(b, domainSize);
        LOG_TRACE("b After ifft:");
        LOG_DEBUG(E.fr.toString(b[0]).c_str());
        LOG_DEBUG(E.fr.toString(b[1]).c_str());
        LOG_TRACE("Start Shift B");
        threadPool.parallelFor(0, domainSize, [&] (int64_t begin, int64_t end, uint64_t idThread) {
            for (u_int64_t i=begin; i<end; i++) {
                E.fr.mul(b[i], b[i], fft->root(domainPower+1, i));
            }
        });
        LOG_TRACE("b After shift:");
        LOG_DEBUG(E.fr.toString(b[0]).c_str());
        LOG_DEBUG(E.fr.toString(b[1]).c_str());
        LOG_TRACE("Start FFT B");
        fft->fft(b, domainSize);
        LOG_TRACE("b After fft:");
        LOG_DEBUG(E.fr.toString(b[0]).c_str());
        LOG_DEBUG(E.fr.toString(b[1]).c_str());
    
        LOG_TRACE("Start iFFT C");
        fft->ifft(c, domainSize);
        LOG_TRACE("c After ifft:");
        LOG_DEBUG(E.fr.toString(c[0]).c_str());
        LOG_DEBUG(E.fr.toString(c[1]).c_str());
        LOG_TRACE("Start Shift C");
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


        // Plug for now
        return nullptr;
    }

    Output *round2(const Input *input) {

        // Plug for now too
        return nullptr;
    }
}