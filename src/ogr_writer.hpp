/*
 * ogr_writer.hpp
 *
 *  Created on:  2019-04-03
 *      Author: Michael Reichert <michael.reichert@geofabrik.de>
 */

#ifndef SRC_OGR_WRITER_HPP_
#define SRC_OGR_WRITER_HPP_

#include <memory>
#include <vector>
#include <gdalcpp.hpp>
#include <osmium/util/verbose_output.hpp>
#include "options.hpp"

/**
 * This class manages the output datasets and serves as factory for layers.
 */
class OGRWriter {
public:

    using datasets_type = std::vector<std::unique_ptr<gdalcpp::Dataset>>;

private:

    /// reference to output manager for STDERR
    osmium::util::VerboseOutput& m_verbose_output;

    Options& m_options;

    /// ORG datasets
    // This has to be a vector of unique_ptr because a vector of objects themselves fails to
    // compile due to "copy constructor of 'Dataset' is implicitly deleted because field
    // 'm_options' has a deleted copy constructor".
    datasets_type m_datasets;

    const std::vector<std::string> GDAL_DEFAULT_OPTIONS;

    /// maximum length of a string field
    static constexpr size_t MAX_FIELD_LENGTH = 254;

    /**
     * Get filename suffix by output format with a leading dot.
     *
     * If the format is not found, an empty string is returned.
     *
     * This method is required because GDAL does not provide such a method.
     */
    std::string filename_suffix();

    /**
     * Return capability to create multiple layers with one data source.
     *
     * By default, this method returns false.
     */
    bool one_layer_per_datasource_only();

    /**
     * \brief Add default options for the to the back of a vector of options.
     *
     * Default dataset creation options are added at the *back* of the vector. If you read the vector in a later step
     * to set them via the functions provided by the GDAL library, do it in reverse order. Otherwise
     * the defaults will overwrite your explicitly set options.
     *
     * \param output_format output format
     * \param gdal_options vector where to add the default options.
     */
    static std::vector<std::string> get_gdal_default_dataset_options(std::string& output_format);

    /**
     * \brief Add default options for the to the back of a vector of options.
     *
     * Default layer creation options are added at the *back* of the vector. If you read the vector in a later step
     * to set them via the functions provided by the GDAL library, do it in reverse order. Otherwise
     * the defaults will overwrite your explicitly set options.
     *
     * \param output_format output format
     * \param gdal_options vector where to add the default options.
     */
    static std::vector<std::string> get_gdal_default_layer_options(std::string& output_format);

public:
    OGRWriter() = delete;

    OGRWriter(Options& options, osmium::util::VerboseOutput& verbose_output);

    void rename_output_files(const std::string& view_name);

    /**
     * Add a new dataset to the vector if the last one cannot be use for multiple layers
     */
    void ensure_writeable_dataset(const char* layer_name);

    //TODO check if it is better to keep a vector of layers and return a reference only
    gdalcpp::Layer create_layer(const char* layer_name, OGRwkbGeometryType type);

    //TODO check if it is better to keep a vector of layers and return a pointer on stack only
    std::unique_ptr<gdalcpp::Layer> create_layer_ptr(const char* layer_name, OGRwkbGeometryType type);
};

#endif /* SRC_OGR_WRITER_HPP_ */
