/*
 * ogr_writer.cpp
 *
 *  Created on:  2019-04-03
 *      Author: Michael Reichert <michael.reichert@geofabrik.de>
 */

#include "ogr_writer.hpp"

OGRWriter::OGRWriter(Options& options, osmium::util::VerboseOutput& verbose_output) :
    m_verbose_output(verbose_output),
    m_options(options),
    m_datasets() {
}


/**
 * Do a case-insensitive string comparison assuming that the right side is lower case.
 */
bool case_insensitive_comp_left(const std::string& a, const std::string& b) {
    return (
        a.size() == b.size()
        && std::equal(
            a.begin(), a.end(), b.begin(),
            [](const char c, const char d) {
                return c == d || std::tolower(c) == d;
            })
    );
}

bool OGRWriter::one_layer_per_datasource_only() {
    return case_insensitive_comp_left(m_options.output_format, "geojson")
        || case_insensitive_comp_left(m_options.output_format, "esri shapefile");
}

std::string OGRWriter::filename_suffix() {
    if (case_insensitive_comp_left(m_options.output_format, "geojson")) {
        return ".json";
    } else if (case_insensitive_comp_left(m_options.output_format, "sqlite")) {
        return ".db";
    }
    return "";
}

void OGRWriter::rename_output_files(const std::string& view_name) {
    if (m_datasets.size() == 1 && filename_suffix().length()) {
        // rename output file if there is one output dataset only
        std::string destination_name {m_options.output_directory};
        destination_name += '/';
        destination_name += view_name;
        destination_name += filename_suffix();
        if (access(destination_name.c_str(), F_OK) == 0) {
            std::cerr << "ERROR: Cannot rename output file from to " << destination_name << " because file exists already.\n";
        } else {
            if (rename(m_datasets.front()->dataset_name().c_str(), destination_name.c_str())) {
                std::cerr << "ERROR: Rename from " << m_datasets.front()->dataset_name() << " to " << destination_name << "failed.\n";
            }
        }
    } else if (m_datasets.size() > 1 && filename_suffix().length()) {
        for (auto& d: m_datasets) {
            std::string destination_name = d->dataset_name();
            destination_name += filename_suffix();
            if (access(destination_name.c_str(), F_OK) == 0) {
                std::cerr << "ERROR: Cannot rename output file from to " << destination_name << " because file exists already.\n";
            } else {
                if (rename(d->dataset_name().c_str(), destination_name.c_str())) {
                    std::cerr << "ERROR: Rename from " << m_datasets.front()->dataset_name() << " to " << destination_name << "failed.\n";
                }
            }
        }
    }
}

void OGRWriter::ensure_writeable_dataset(const char* layer_name) {
    if (m_datasets.empty() || one_layer_per_datasource_only()) {
        std::string output_filename = m_options.output_directory;
        output_filename += '/';
        output_filename += layer_name;
        std::unique_ptr<gdalcpp::Dataset> ds {new gdalcpp::Dataset(m_options.output_format,
                output_filename, gdalcpp::SRS(m_options.srs), get_gdal_default_options(m_options.output_format))};
        m_datasets.push_back(std::move(ds));
        m_datasets.back()->enable_auto_transactions(10000);
    }
}

gdalcpp::Layer OGRWriter::create_layer(const char* layer_name, OGRwkbGeometryType type,
        const std::vector<std::string>& options /*= {}*/) {
    ensure_writeable_dataset(layer_name);
    return gdalcpp::Layer(*(m_datasets.back()), layer_name, type, options);
}

std::unique_ptr<gdalcpp::Layer> OGRWriter::create_layer_ptr(const char* layer_name, OGRwkbGeometryType type,
        const std::vector<std::string>& options /*= {}*/) {
    ensure_writeable_dataset(layer_name);
    return std::unique_ptr<gdalcpp::Layer>{new gdalcpp::Layer(*(m_datasets.back()), layer_name, type, options)};
}

std::vector<std::string> OGRWriter::get_gdal_default_options(std::string& output_format) {
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
