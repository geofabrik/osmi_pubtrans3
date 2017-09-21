# OSMI Public Transport 3

This repository contains the programme which generates the data which powers
the Public Transport views of OpenStreetMap Inspector. It is called version 3
because it replaces two older (unpublished) implementations. The main difference
to its predecessors is that it supports the current tagging schemes.

This software uses the [Osmium library](https://github.com/osmcode/libosmium) by Jochen Topf for everything related with
reading OSM data and the [GDAL](http://gdal.org/) library (via Jochen Topf's C++ wrapper
[gdalcpp](https://github.com/joto/gdalcpp)) to write the output data.


## License and Authors

This software was developed by Geofabrik GmbH. See the Git history for a full
list of contributors.

This software is licensed under the terms of GNU General Public License version
3 or newer. See [LICENSE.md](LICENSE.md) for the full legal text of the license.

The Catch unit test framework is available under the terms of [Boost Software
License](test/include/LICENSE_1_0.txt).


## How it works

This software reads an OpenStreetMap planet dump (or one of its smaller extracts) and
produces an Spatialite database which contains all errorenous objects which were
found in the OpenStreetMap data. Other output formats than Spatialite are
possible but not as well tested. You can open the output files using QGIS.

The SpatiaLite database is used as the data source of the [WMS
service](https://wiki.openstreetmap.org/wiki/OSM_Inspector/WxS) by the OSMI
backend. This service provides the map and an GetFeatureInfo API call used by
the frontend.

## Output

The documentation of the contents of the SQlite file can be found at [output-documentation.md](output-documentation.md).


## Dependencies

* C++11 compiler
* libosmium (`libosmium-dev`) and all its [important dependencies](http://osmcode.org/libosmium/manual.html#dependencies)
* GDAL library (`libgdal-dev`)
* proj.4 (`libproj4-dev`)
* CMake (`cmake`)

You can install libosmium either using your package manager or just cloned from
its Github repository to any location on your disk. Add a symlink libosmium in
the top level directory of this repository to the location of
`libosmium/include` if you use the latter variant. Take care to use libosmium
v2.x not the old Osmium v1.x!


## Building

```sh
mkdir build
cd build
cmake ..
make
```

To run the unit tests:

```sh
make test
```

The unit tests write temporary files to `build/test/`. If the tests run properly,
they clean up these files. Otherwise run `rm build/tests/.tmp.*` manually. If a
test fails because a check fails, the temporary files are clean up.

## Usage

Run `./osmi_pubtrans3 -h` to see the available options.

There are two binaries. `osmi_pubtrans3_merc` can only produce output files
in Web Mercator projection (EPSG:3857) but is faster than `osmi_pubtrans3`
because it uses a faster coordinate transformation engine provided by libosmium
while `osmi_simple_views` calls Proj4.

