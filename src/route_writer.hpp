/*
 * route_error_writer.hpp
 *
 *  Created on:  2017-08-28
 *      Author: Michael Reichert <michael.reichert@geofabrik.de>
 */

#ifndef SRC_ROUTE_WRITER_HPP_
#define SRC_ROUTE_WRITER_HPP_

#include <osmium/osm/relation.hpp>

#include "ogr_output_base.hpp"

enum class RouteType : char {
    NONE,
    BUS,
    TROLLEYBUS,
    AERIALWAY,
    FERRY,
    TRAIN,
    TRAM,
    SUBWAY
};

enum class RouteError : size_t {
    CLEAN = 0,
    OVER_NON_RAIL = 1,
    OVER_NON_ROAD = 2,
    NO_TROLLEY_WIRE = 4,
    UNORDERED_GAP = 8,
    WRONG_STRUCTURE = 16,
    NO_STOPPLTF_AT_FRONT = 32,
    EMPTY_ROLE_NON_WAY = 64,
    STOPPLTF_AFTER_ROUTE = 128,
    STOP_NOT_ON_WAY = 256,
    NO_ROUTE = 512,
    UNKNOWN_ROLE = 1024,
    UNKNOWN_TYPE = 2048,
    STOP_TAG_MISSING = 4096,
    PLTF_TAG_MISSING = 8192,
    STOP_IS_NOT_NODE = 16384,
    NO_FERRY = 32768
};

inline RouteError& operator|= (RouteError& a, const RouteError& b) {
    return a = static_cast<RouteError>(static_cast<size_t>(a) | static_cast<size_t>(b));
}

inline RouteError operator&(const RouteError&a , const RouteError& b) {
    return static_cast<RouteError>(static_cast<size_t>(a) & static_cast<size_t>(b));
}

class RouteWriter : public OGROutputBase {
    gdalcpp::Dataset& m_dataset;
    gdalcpp::Layer m_ptv2_routes_valid;
    gdalcpp::Layer m_ptv2_routes_invalid;
    gdalcpp::Layer m_ptv2_error_lines;
    gdalcpp::Layer m_ptv2_error_points;

public:
    RouteWriter() = delete;

    RouteWriter(gdalcpp::Dataset& dataset, std::string& output_format,
        osmium::util::VerboseOutput& verbose_output, int epsg = 3857);

    void write_valid_route(const osmium::Relation& relation, std::vector<const osmium::OSMObject*>& member_objects,
            std::vector<const char*>& roles);

    void write_invalid_route(const osmium::Relation& relation, std::vector<const osmium::OSMObject*>& member_objects,
            std::vector<const char*>& roles, RouteError validation_result);

    void write_error_way(const osmium::Relation& relation, const osmium::object_id_type node_id,
            const char* error_text, const osmium::Way* way);

    void write_error_point(const osmium::Relation& relation, const osmium::NodeRef* node_ref,
            const char* error_text, const osmium::object_id_type way_id);

    void write_error_point(const osmium::Relation& relation, const osmium::object_id_type node_ref,
            const osmium::Location& location, const char* error_text, const osmium::object_id_type way_id);

    void write_error_object(const osmium::Relation& relation, const osmium::OSMObject* object, const osmium::object_id_type node_id,
            const char* error_text);
};



#endif /* SRC_ROUTE_WRITER_HPP_ */
