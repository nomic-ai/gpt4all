#include <stdio.h>
#include <string>

int main(int argc, char *argv[])
{
    static bool avx = __builtin_cpu_supports("avx");
    static bool avx2 = __builtin_cpu_supports("avx2");
    static bool fma = __builtin_cpu_supports("fma");
    static bool sse3 = __builtin_cpu_supports("sse3");
    static std::string s;
    s  = "gpt4all hardware test results:\n";
    s += "    AVX  = "        + std::to_string(avx)         + "\n";
    s += "    AVX2 = "        + std::to_string(avx2)        + "\n";
    s += "    FMA  = "        + std::to_string(fma)         + "\n";
    s += "    SSE3 = "        + std::to_string(sse3)        + "\n";
    fprintf(stderr, "%s", s.c_str());
    fprintf(stderr, "your hardware supports the \"");
    fflush(stderr);
    if (avx2)
        printf("full");
    else if (avx && fma)
        printf("avx_only");
    else
        printf("bare_minimum");
    fflush(stdout);
    fprintf(stderr, "\" version of gpt4all.\n");
    fflush(stderr);
    return 0;
}
