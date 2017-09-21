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

/**
 * All errors this tool can detect on route relations.
 *
 * A route has one variable which stores all this errors. Each error is represented by a bit.
 */
enum class RouteError : uint32_t {
    /// route has no errors
    CLEAN = 0,
    /// Route for railways goes over a non-railway way.
    OVER_NON_RAIL = 1,
    /// Route for buses goes over a non-highway or non-ferry way.
    OVER_NON_ROAD = 2,
    /// Route for trolleybuses has no trolley wire.
    NO_TROLLEY_WIRE = 4,
    /// Route has a gap or is not ordered correctly.
    UNORDERED_GAP = 8,
    /// Route has a wrong structure in the member list.
    WRONG_STRUCTURE = 16,
    /// Stops and platforms are missing at the beginning of the route.
    NO_STOPPLTF_AT_FRONT = 32,
    /// A member which is not a way has an empty role.
    EMPTY_ROLE_NON_WAY = 64,
    /// A member which is a stop or platform is found after the first highway/ferry/railway member.
    STOPPLTF_AFTER_ROUTE = 128,
    /// A stop position is not on a way. Not implemented yet.
    STOP_NOT_ON_WAY = 256,
    /// The relation does not contain any highway/ferry/railway members.
    NO_ROUTE = 512,
    /// One or many members have an unknown role.
    UNKNOWN_ROLE = 1024,
    /// The route has an invalid or unknown type (`route=*` tag).
    UNKNOWN_TYPE = 2048,
    /// A stop member lacks the necessary tags.
    STOP_TAG_MISSING = 4096,
    /// A platform member lacks the necessary tags.
    PLTF_TAG_MISSING = 8192,
    /// A stop member is not a node.
    STOP_IS_NOT_NODE = 16384,
    /// A ferry route uses a way which is not a ferry way.
    NO_FERRY = 32768
};

inline RouteError& operator|= (RouteError& a, const RouteError& b) {
    return a = static_cast<RouteError>(static_cast<size_t>(a) | static_cast<size_t>(b));
}

inline RouteError operator&(const RouteError&a , const RouteError& b) {
    return static_cast<RouteError>(static_cast<size_t>(a) & static_cast<size_t>(b));
}

/**
 * The RouteWriter class writes routes as multilinestrings and their errors (points and linestrings) to
 * the output dataset.
 */
class RouteWriter : public OGROutputBase {
    gdalcpp::Dataset& m_dataset;
    gdalcpp::Layer m_ptv2_routes_valid;
    gdalcpp::Layer m_ptv2_routes_invalid;
    gdalcpp::Layer m_ptv2_error_lines;
    gdalcpp::Layer m_ptv2_error_points;

public:
    RouteWriter() = delete;

    RouteWriter(gdalcpp::Dataset& dataset, Options& options, osmium::util::VerboseOutput& verbose_output);

    void write_valid_route(const osmium::Relation& relation, std::vector<const osmium::OSMObject*>& member_objects,
            std::vector<const char*>& roles);

    void write_invalid_route(const osmium::Relation& relation, std::vector<const osmium::OSMObject*>& member_objects,
            RouteError validation_result);

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
