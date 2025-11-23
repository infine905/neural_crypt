#include <iostream>
#include <chrono>

#include "singleton.hpp"
#include "neural_crypt/neural_crypt.hpp"

class test : public singleton<test>
{
	friend class singleton<test>;

public:
    void secret_function()
    {
        std::cout << "Hello from secret function with inst: " << this << std::endl;
    }
};

int test_function(int a, int b)
{
	return a + b;
}

int main()
{
    // 1) Singleton with instance ptr crypt
    test::get().secret_function();

    // 1.1) Check that instance is the same
    test::get().secret_function();

    // 2) Pointer crypt test
    std::cout << std::endl;
    uint64_t* test_ptr = reinterpret_cast<uint64_t*>(0xDEADBEEF);
    auto crypted_value = CRYPT_PTR(test_ptr);

    std::cout << "Crypted value address:   0x" << crypted_value.raw() << std::endl;
    std::cout << "Decrypted value address: 0x" << crypted_value.decrypt() << std::endl;

    // 3) Speed test
    std::cout << std::endl;
    auto start_time1 = std::chrono::steady_clock::now();
    {
        auto funk_ptr = CRYPT_PTR(&test_function);

        int sum1 = 0;
        for (int i = 0; i < 1000; ++i)
        {
            sum1 = (*funk_ptr)(sum1, i);
        }
        std::cout << "Sum1: " << sum1 << std::endl;
    }
    auto end_time1 = std::chrono::steady_clock::now();

    auto start_time2 = std::chrono::steady_clock::now();
    {
        int sum2 = 0;
        for (int i = 0; i < 1000; ++i)
        {
            sum2 = test_function(sum2, i);
        }
        std::cout << "Sum2: " << sum2 << std::endl;
    }
    auto end_time2 = std::chrono::steady_clock::now();

    auto time1 = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time1 - start_time1).count();
    auto time2 = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time2 - start_time2).count();

    std::cout << "Crypted:     "    << time1 << " ns\n";
    std::cout << "Non-Crypted: "    << time2 << " ns\n";
    std::cout << "Preformance: "    << (float(time2 - time1) / float(time1)) * 100.f << "%" << std::endl;

    return 0;
}