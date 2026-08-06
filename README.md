# GeoNames

GeoNames is a lightweight, offline, city search library for C++ applications.
It loads the [geonames.org](https://geonames.org) dataset into a ~4 MB in-memory database and supports name prefixing and alternative names (e.g. "ott" finds Ottawa, "wpg" finds Winnipeg).
The average query takes ~0.1 ms on my machine.

### Example

```c++
#include <geonames.hpp>

#include <cstdio>
#include <vector>

int main()
{
    std::vector<GeoNames> results;
    GeoNamesQuery(results, 8, "nyc");
    for (const GeoNames& result : results)
    {
        std::printf("Found \"%s\" at %f,%f\n", result.Location.c_str(), result.Latitude, result.Longitude);
    }
    return 0;
}
```

```
>>> Found "New York City, New York, United States" at 40.714272,-74.005966
>>> Found "Manhattan, New York County, New York, United States" at 40.783428,-73.966248
```

### CMake

Add the following to your `CMakeLists.txt`:

```cmake
add_subdirectory(<path>)
target_link_libraries(<name> PRIVATE geonames::geonames)
```

### Generation

Run `generate.py` to download the [geonames.org](https://geonames.org) dataset and rebuild the database.
Use `--lang` to generate for a different language (e.g. `fr` for French)
