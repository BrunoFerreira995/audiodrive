#pragma once
#include <algorithm>
#include <cmath>
#include <optional>
#include <span>
namespace audio32::diagnostics {
struct RoundTripMatch { std::size_t lag; double correlation; };
inline std::optional<RoundTripMatch> findRoundTrip(std::span<const float> capture,
                                                 std::span<const float> probe,
                                                 std::size_t maxLag,double threshold=0.7) {
    if (probe.empty() || capture.size()<probe.size() || maxLag==0) return std::nullopt;
    double probeEnergy=0;
    for (float x:probe) probeEnergy+=static_cast<double>(x)*x;
    if (probeEnergy<1e-12) return std::nullopt;
    RoundTripMatch best{0,0};
    const auto count=std::min(maxLag,capture.size()-probe.size()+1);
    for (std::size_t lag=0;lag<count;++lag) {
        double product=0,energy=0;
        for (std::size_t i=0;i<probe.size();++i) {
            const double sample=capture[lag+i]; product+=probe[i]*sample; energy+=sample*sample;
        }
        const double correlation=energy>1e-12?std::fabs(product)/std::sqrt(energy*probeEnergy):0;
        if (std::isfinite(correlation) && correlation>best.correlation) best={lag,correlation};
    }
    return best.correlation>=threshold ? std::optional<RoundTripMatch>(best) : std::nullopt;
}
}
