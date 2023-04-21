#include <stdio.h>
#include <string>

int main(int argc, char *argv[])
{
    static bool avx = __builtin_cpu_supports("avx");
    static bool avx2 = __builtin_cpu_supports("avx2");
    static bool avx512f = __builtin_cpu_supports("avx512f");
    static bool avx512vbmi = __builtin_cpu_supports("avx512vbmi");
    static bool avx512vnni = __builtin_cpu_supports("avx512vnni");
    static bool fma = __builtin_cpu_supports("fma");
    static bool sse3 = __builtin_cpu_supports("sse3");
    static std::string s;
    s  = "My hardware supports:\n";
    s += "AVX = "         + std::to_string(avx)         + "\n";
    s += "AVX2 = "        + std::to_string(avx2)        + "\n";
    s += "AVX512 = "      + std::to_string(avx512f)     + "\n";
    s += "AVX512_VBMI = " + std::to_string(avx512vbmi)  + "\n";
    s += "AVX512_VNNI = " + std::to_string(avx512vnni)  + "\n";
    s += "FMA = "         + std::to_string(fma)         + "\n";
    s += "SSE3 = "        + std::to_string(sse3)        + "\n";
    printf("%s", s.c_str());
    fflush(stdout);
    return 0;
}
