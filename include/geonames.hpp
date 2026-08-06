#pragma once

#include <string>
#include <string_view>
#include <vector>

struct GeoNames
{
    std::string Location;
    float Latitude;
    float Longitude;
};

void GeoNamesQuery(std::vector<GeoNames>& results, int maxResults, const std::string_view pattern);