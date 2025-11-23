#ifndef NEURAL_OBFUSCATOR_HPP
#define NEURAL_OBFUSCATOR_HPP

#include "weights.h"

#if defined(_MSC_VER)
#define C_FORCEINLINE __forceinline
#elif defined(__GNUC__) || defined(__clang__)
#define C_FORCEINLINE inline __attribute__((__always_inline__))
#else
#define C_FORCEINLINE inline
#endif

constexpr uint64_t fnv1a_hash(const char* str, uint64_t hash = 14695981039346656037ULL)
{
    return (*str == '\0') ? hash : fnv1a_hash(str + 1, (hash ^ static_cast<uint64_t>(*str)) * 1099511628211ULL);
}

#define ROUNDS 16       // rounds 8 (for speed) or 16 (for more complexity)
#define INPUT_DIM 96    // 32 data + 64 key
#define HIDDEN_DIM 64
#define OUTPUT_DIM 32
#define GEN_CT_KEY (fnv1a_hash(__TIME__) ^ fnv1a_hash(__FILE__) ^ ((uint64_t)__LINE__ * (uint64_t)__COUNTER__))

namespace neural_crypto 
{
    // Forward Pass
    C_FORCEINLINE auto neural_f(uint32_t right_part, uint64_t round_key) -> uint32_t
    {
        float input[INPUT_DIM];

        // Load data (0..31)
        for (int i = 0; i < 32; ++i) 
            input[i] = (right_part & (1U << i)) ? 1.0f : 0.0f;

        // Load key (32..95)
        for (int i = 0; i < 64; ++i) 
            input[32 + i] = (round_key & (1ULL << i)) ? 1.0f : 0.0f;

        float hidden[HIDDEN_DIM];

        // Layer 1
        for (int i = 0; i < HIDDEN_DIM; ++i) 
        {
            float sum = weights::B1[i];
            const float* w_ptr = &weights::W1[i * INPUT_DIM];
            for (int j = 0; j < INPUT_DIM; ++j) 
            {
                sum += input[j] * w_ptr[j];
            }
            hidden[i] = (sum > 0.0f ? (sum) : 0.0f); // ReLU
        }

        // Layer 2 + Binarization
        uint32_t result = 0;
        for (int i = 0; i < OUTPUT_DIM; ++i) 
        {
            float sum = weights::B2[i];
            const float* w_ptr = &weights::W2[i * HIDDEN_DIM];
            for (int j = 0; j < HIDDEN_DIM; ++j) 
            {
                sum += hidden[j] * w_ptr[j];
            }
            if (sum > 0.0f) 
                result |= (1U << i);
        }
        return result;
    }

    // Feistel Web (Crypt/Decrypt)
    C_FORCEINLINE auto process(uint64_t val, uint64_t key, bool encrypt) -> uint64_t
    {
        uint32_t L = (val >> 32) & 0xFFFFFFFF;
        uint32_t R = val & 0xFFFFFFFF;
        const int rounds = 16;

        for (int i = 0; i < ROUNDS; ++i)
        {
            // Key Schedule: circular shift
            int round_idx = encrypt ? i : (ROUNDS - 1 - i);
            uint64_t step_key = key ^ round_idx;
            uint64_t round_key = (step_key << round_idx) | (step_key >> (64 - round_idx));

            uint32_t f_out = neural_f(R, round_key);
            uint32_t temp = R;
            R = L ^ f_out;
            L = temp;
        }

        return ((uint64_t)R << 32) | L;
    }

    // Shit fix (protect from delete by optimization) i fuck MSVC
    template <typename T>
    C_FORCEINLINE T* force_memory(T* ptr) 
    {
#if defined(__GNUC__) || defined(__clang__)
        asm volatile("" : : "r,m"(ptr) : "memory");
#elif defined(_MSC_VER)
        _ReadWriteBarrier();
        volatile T* v_ptr = ptr;
        (void)v_ptr;
#endif
        return ptr;
    }
}

// Class-Wrapper
template <typename T, uint64_t Key>
class neural_ptr_crypt 
{
private:
    uint64_t encrypted_val;

public:
    // Constructor: accepts a raw pointer and encrypts it
    C_FORCEINLINE neural_ptr_crypt(T* ptr)
    {
        encrypted_val = neural_crypto::process(reinterpret_cast<uint64_t>(ptr), Key, true);
    }

    // The value retrieval operator
    C_FORCEINLINE T* decrypt() const
    {
        return reinterpret_cast<T*>(neural_crypto::process(encrypted_val, Key, false));
    }

	// Get raw encrypted value (for storage or debugging)
    C_FORCEINLINE uint64_t raw() const
    {
        return encrypted_val;
    }

    // Operators for easy work
    T* operator->() const { return decrypt(); }
    T& operator*() const { return *decrypt(); }
};

#define CRYPT_PTR(ptr) neural_ptr_crypt<typename std::remove_pointer<decltype(ptr)>::type, GEN_CT_KEY>(neural_crypto::force_memory(ptr))

#endif // NEURAL_OBFUSCATOR_HPP