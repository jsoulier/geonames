#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#include <cstdio>

#include "geonames.hpp"

static void PrintResults(const std::string_view pattern)
{
    std::printf("Trying \"%s\"\n", pattern.data());
    std::vector<GeoNames> results;
    GeoNamesQuery(results, 8, pattern);
    for (const GeoNames& result : results)
    {
        std::printf("Found \"%s\" at %f,%f\n", result.Location.c_str(), result.Latitude, result.Longitude);
    }
}

int main(int argc, char** argv)
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif
    PrintResults("ot");
    PrintResults("mont");
    PrintResults("wpg");
    PrintResults("nyc");
    PrintResults("munchen");
    PrintResults("koln");
    PrintResults("cologne");
    PrintResults("moscou");
    PrintResults("moskau");
    PrintResults("москва");
    PrintResults("québec");
    PrintResults("firenze");
    PrintResults("florence");
    PrintResults("madrid");
    return 0;
}