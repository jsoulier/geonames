#include <sqlite3.h>

#include <cctype>
#include <string>
#include <vector>

#include "geonames.db.h"
#include "geonames.hpp"

static sqlite3* GetDatabase()
{
    static sqlite3* handle = []() -> sqlite3*
    {
        sqlite3* handle = nullptr;
        if (sqlite3_open(":memory:", &handle) != SQLITE_OK)
        {
            sqlite3_close(handle);
            return nullptr;
        }
        if (sqlite3_deserialize(
            handle, "main",
            kGeoNames, sizeof(kGeoNames), sizeof(kGeoNames),
            SQLITE_DESERIALIZE_READONLY) != SQLITE_OK)
        {
            sqlite3_close(handle);
            return nullptr;
        }
        return handle;
    }();
    return handle;
}

static std::string GetQuery(const std::string_view pattern)
{
    std::string query;
    std::string token;
    auto flush = [&]()
    {
        if (!token.empty())
        {
            if (!query.empty())
            {
                query += ' ';
            }
            query += '"';
            query += token;
            query += "\"*";
            token.clear();
        }
    };
    for (const char c : pattern)
    {
        if (std::isalnum(static_cast<unsigned char>(c)))
        {
            token += c;
        }
        else
        {
            flush();
        }
    }
    flush();
    return query;
}

void GetGeoNames(std::vector<GeoNames>& results, int maxResults, const std::string_view pattern)
{
    static constexpr const char* kSelect =
        "SELECT location, latitude, longitude "
        "FROM cities WHERE location MATCH ?1 "
        "ORDER BY CAST(population AS INTEGER) DESC LIMIT ?2";
    results.clear();
    const std::string query = GetQuery(pattern);
    if (query.empty())
    {
        return;
    }
    sqlite3* handle = GetDatabase();
    if (!handle)
    {
        return;
    }
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(handle, kSelect, -1, &stmt, nullptr) != SQLITE_OK)
    {
        return;
    }
    sqlite3_bind_text(stmt, 1, query.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, maxResults);
    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        GeoNames& point = results.emplace_back();
        point.Location = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        point.Latitude = sqlite3_column_double(stmt, 1);
        point.Longitude = sqlite3_column_double(stmt, 2);
    }
    sqlite3_finalize(stmt);
}
