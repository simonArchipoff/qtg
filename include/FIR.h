#include <cstddef>
#include <type_traits>

template<auto& Coeff>
class FIR {
    static constexpr size_t N = std::extent_v<std::remove_reference_t<decltype(Coeff)>>;

public:
    FIR() : state{}, head(0) {}

    void process(const float* in, float* out) {
        state[head] = *in;

        float sum = 0.0f;
        size_t idx = head;
        for (size_t i = 0; i < N; ++i) {
            sum += Coeff[i] * state[idx];
            idx = (idx - 1 + N) % N;
        }

        *out = sum;
        head = (head + 1) % N;
    }

private:
    float state[N];
    size_t head = 0;
};