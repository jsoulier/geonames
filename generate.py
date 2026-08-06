#!python3
# https://download.geonames.org/export/dump/readme.txt

from pathlib import Path
import argparse
import io
import sqlite3
import tempfile
import time
import urllib.request
import zipfile

DATASET = "cities15000"
URL = "https://download.geonames.org/export/dump"
DATABASE = Path("geonames.db")
HEADER = Path("src") / Path("geonames.db.h")
CACHE = Path(tempfile.gettempdir()) / "geonames"
MAX_AGE = 24 * 60 * 60

def download(filename):
    cache = CACHE / filename
    if cache.exists() and time.time() - cache.stat().st_mtime < MAX_AGE:
        print(f"Using cached {cache}")
        return cache
    url = f"{URL}/{filename}"
    print(f"Downloading {url}")
    CACHE.mkdir(exist_ok=True)
    urllib.request.urlretrieve(url, cache.with_suffix(cache.suffix + ".tmp"))
    cache.with_suffix(cache.suffix + ".tmp").replace(cache)
    return cache

def get_lookup(filename, key, name, id_field):
    lookup = {}
    text = download(filename).read_text("utf-8")
    for line in text.splitlines():
        if not line or line.startswith("#"):
            continue
        fields = line.split("\t")
        lookup[fields[key]] = (fields[name], int(fields[id_field]))
    return lookup

def get_cities():
    admin1 = get_lookup("admin1CodesASCII.txt", 0, 1, 3)
    admin2 = get_lookup("admin2Codes.txt", 0, 2, 3)
    countries = get_lookup("countryInfo.txt", 0, 4, 16)
    archive = zipfile.ZipFile(download(f"{DATASET}.zip"))
    with archive.open(f"{DATASET}.txt") as raw:
        for line in io.TextIOWrapper(raw, encoding="utf-8"):
            fields = line.rstrip("\n").split("\t")
            name = fields[2]
            latitude = float(fields[4])
            longitude = float(fields[5])
            country = fields[8]
            admin1_code = fields[10]
            admin2_code = fields[11]
            population = int(fields[14] or 0)
            county = admin2.get(f"{country}.{admin1_code}.{admin2_code}")
            state = admin1.get(f"{country}.{admin1_code}")
            nation = countries.get(country, (country, None))
            yield (int(fields[0]), name, county, state, nation, latitude, longitude, population)

def get_alt_names(geonameids, language):
    languages = (language, "abbr", "short", "colloquial")
    names = {}
    display = {}
    archive = zipfile.ZipFile(download("alternateNames.zip"))
    with archive.open("alternateNames.txt") as raw:
        for line in io.TextIOWrapper(raw, encoding="utf-8"):
            fields = line.rstrip("\n").split("\t")
            if len(fields) < 4 or fields[2] not in languages:
                continue
            geonameid = int(fields[1])
            if geonameid not in geonameids:
                continue
            name = fields[3].strip()
            if name:
                names.setdefault(geonameid, []).append(name)
                if fields[2] == language:
                    preferred = fields[4] == "1"
                    current = display.get(geonameid)
                    if current is None or (preferred and not current[1]):
                        display[geonameid] = (name, preferred)
    for geonameid, values in names.items():
        dedupe = {}
        for name in values:
            dedupe.setdefault(name.casefold(), name)
        names[geonameid] = list(dedupe.values())
    display = {geonameid: name for geonameid, (name, _) in display.items()}
    return names, display

def main():
    parser = argparse.ArgumentParser(description="Generate the GeoNames database")
    parser.add_argument("--lang", default="en")
    args = parser.parse_args()
    if DATABASE.exists():
        DATABASE.unlink()
    rows = list(get_cities())
    ids = {row[0] for row in rows}
    for row in rows:
        for admin in (row[2], row[3], row[4]):
            if admin and admin[1]:
                ids.add(admin[1])
    alt_names, display = get_alt_names(ids, args.lang)
    database = sqlite3.connect(DATABASE)
    database.execute(
        "CREATE VIRTUAL TABLE cities USING fts5("
        "location, altnames, latitude UNINDEXED, longitude UNINDEXED, population UNINDEXED)"
    )
    def insert(row):
        geonameid, name, county, state, nation, latitude, longitude, population = row
        parts = [display.get(geonameid) or name]
        for admin in (county, state, nation):
            if admin:
                parts.append(display.get(admin[1]) or admin[0])
        location = ", ".join(parts)
        names = ", ".join(alt_names.get(geonameid, []))
        return (location, names, latitude, longitude, population)
    count = database.executemany(
        "INSERT INTO cities(location, altnames, latitude, longitude, population) VALUES (?, ?, ?, ?, ?)",
        (insert(row) for row in rows),
    ).rowcount
    database.commit()
    database.close()
    data = DATABASE.read_bytes()
    with open(HEADER, "w") as header:
        header.write("#pragma once\n\n")
        header.write("static unsigned char kGeoNames[] = \n{\n")
        for offset in range(0, len(data), 20):
            header.write("".join(f"0x{byte:02x}," for byte in data[offset:offset + 20]) + "\n")
        header.write("};\n")
    print(f"Wrote {count} cities to {HEADER} ({DATABASE.stat().st_size} bytes)")

if __name__ == "__main__":
    main()
