/*
 *  © 2017 Geofabrik GmbH
 *
 *  This file is part of osmi_simple_views.
 *
 *  osmi_simple_views is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  osmi_simple_views is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with osmi_simple_views. If not, see <http://www.gnu.org/licenses/>.
 */


#include "ogr_output_base.hpp"

OGROutputBase::OGROutputBase(osmium::util::VerboseOutput& verbose_output, Options& options) :
#ifndef ONLYMERCATOROUTPUT
        m_factory(osmium::geom::Projection(options.srs)),
#endif
        m_verbose_output(verbose_output),
        GDAL_DEFAULT_OPTIONS(get_gdal_default_options(options.output_format)),
        m_options(options) { }

std::vector<std::string> OGROutputBase::get_gdal_default_options(std::string& output_format) {
    std::vector<std::string> default_options;
    // default layer creation options
    if (output_format == "SQlite") {
        default_options.emplace_back("OGR_SQLITE_SYNCHRONOUS=OFF");
        default_options.emplace_back("SPATIAL_INDEX=NO");
        default_options.emplace_back("COMPRESS_GEOM=NO");
        default_options.emplace_back("SPATIALITE=YES");
////        CPLSetConfigOption("OGR_SQLITE_PRAGMA", "journal_mode=OFF,TEMP_STORE=MEMORY,temp_store=memory,LOCKING_MODE=EXCLUSIVE");
////        CPLSetConfigOption("OGR_SQLITE_CACHE", "600");
////        CPLSetConfigOption("OGR_SQLITE_JOURNAL", "OFF");
    } else if (output_format == "ESRI Shapefile") {
        default_options.emplace_back("SHAPE_ENCODING=UTF8");
    }

    return default_options;
}
