#include <sqlite3.h>

#include <cctype>
#include <string>
#include <vector>

#include "geonames.db.h"
#include "geonames.hpp"

struct Database
{
    Database()
        : Handle{nullptr}
        , Stmt{nullptr}
    {
        if (sqlite3_open(":memory:", &Handle) != SQLITE_OK)
        {
            return;
        }
        if (sqlite3_deserialize(
            Handle, "main",
            kGeoNames, sizeof(kGeoNames), sizeof(kGeoNames),
            SQLITE_DESERIALIZE_READONLY) != SQLITE_OK)
        {
            return;
        }
        static constexpr const char* kSelect =
            "SELECT location, latitude, longitude "
            "FROM cities WHERE cities MATCH ?1 "
            "ORDER BY CAST(population AS INTEGER) DESC LIMIT ?2";
        if (sqlite3_prepare_v2(Handle, kSelect, -1, &Stmt, nullptr) != SQLITE_OK)
        {
            return;
        }
    }

    Database(const Database& other) = delete;
    Database& operator=(const Database& other) = delete;

    ~Database()
    {
        sqlite3_finalize(Stmt);
        sqlite3_close(Handle);
    }

    sqlite3* Handle;
    sqlite3_stmt* Stmt;
};

static Database database;

// ott -> "ott"*
// new york -> "new"* "york"*
// québec -> "québec"*
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
    for (const unsigned char c : pattern)
    {
        // split on ascii non-alphanumeric characters
        if (c >= 0x80 || std::isalnum(c))
        {
            token += static_cast<char>(c);
        }
        else
        {
            flush();
        }
    }
    flush();
    return query;
}

void GeoNamesQuery(std::vector<GeoNames>& results, int maxResults, const std::string_view pattern)
{
    results.clear();
    const std::string query = GetQuery(pattern);
    if (query.empty())
    {
        return;
    }
    if (!database.Handle || !database.Stmt)
    {
        return;
    }
    sqlite3_reset(database.Stmt);
    sqlite3_clear_bindings(database.Stmt);
    sqlite3_bind_text(database.Stmt, 1, query.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(database.Stmt, 2, maxResults);
    while (sqlite3_step(database.Stmt) == SQLITE_ROW)
    {
        GeoNames& point = results.emplace_back();
        point.Location = reinterpret_cast<const char*>(sqlite3_column_text(database.Stmt, 0));
        point.Latitude = sqlite3_column_double(database.Stmt, 1);
        point.Longitude = sqlite3_column_double(database.Stmt, 2);
    }
}
