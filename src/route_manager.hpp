/*
 * route_collector.hpp
 *
 *  Created on:  2017-08-23
 *      Author: Michael Reichert <michael.reichert@geofabrik.de>
 */

#ifndef SRC_ROUTE_COLLECTOR_HPP_
#define SRC_ROUTE_COLLECTOR_HPP_

#include <osmium/relations/relations_manager.hpp>
#include "abstract_view_handler.hpp"

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

enum class MemberStatus : char {
    BEFORE_FIRST = 0,
    FIRST = 1,
    SECOND = 2,
    NORMAL = 3,
    /// The first way after way which is not connected to its predecessor.
    AFTER_GAP = 4,
    MISSING = 5,
    AFTER_MISSING = 6,
    ROUNDABOUT = 7,
    SECOND_ROUNDABOUT = 8,
    AFTER_ROUNDABOUT = 9
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
    STOP_IS_NOT_NODE = 16384
};

inline RouteError& operator|= (RouteError& a, const RouteError& b) {
    return a = static_cast<RouteError>(static_cast<size_t>(a) | static_cast<size_t>(b));
}

inline RouteError operator&(const RouteError&a , const RouteError& b) {
    return static_cast<RouteError>(static_cast<size_t>(a) & static_cast<size_t>(b));
}

class RouteManager : public osmium::relations::RelationsManager<RouteManager, true, true, true, false>, OGROutputBase {
    gdalcpp::Dataset& m_dataset;
    gdalcpp::Layer m_ptv2_routes_valid;
    gdalcpp::Layer m_ptv2_routes_invalid;
    gdalcpp::Layer m_ptv2_error_lines;
    gdalcpp::Layer m_ptv2_error_points;

    bool is_ptv2(const osmium::Relation& relation);

    RouteError is_valid(const osmium::Relation& relation, std::vector<const osmium::OSMObject*>& member_objects,
            std::vector<const char*>& roles);

    static RouteType get_route_type(const char* route);

    RouteError check_roles_order_and_type(const osmium::Relation& relation);

    static bool is_stop(const char* role);

    static bool is_platform(const char* role);

    RouteError check_stop_tags(const osmium::Relation& relation, const osmium::Node* node, RouteType type);

    static bool vehicle_tags_matches_route_type(const osmium::TagList& tags, RouteType type);

    RouteError check_platform_tags(const osmium::Relation& relation, const RouteType type, const osmium::OSMObject* object);

    RouteError is_way_usable(const osmium::Relation& relation, RouteType type, const osmium::Way* way);

    static bool check_valid_railway_track(RouteType type, const osmium::TagList& member_tags);

    static bool check_valid_road_way(const osmium::TagList& member_tags);

    static bool check_valid_trolleybus_way(const osmium::TagList& member_tags);

    static bool is_ferry(const osmium::TagList& member_tags);

    RouteError find_gaps(const osmium::Relation& relation, std::vector<const osmium::OSMObject*>& member_objects,
            std::vector<const char*>& roles);

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

public:
    RouteManager() = delete;

    RouteManager(gdalcpp::Dataset& dataset, std::string& output_format,
        osmium::util::VerboseOutput& verbose_output, int epsg = 3857);

    bool new_relation(const osmium::Relation& relation) const noexcept;

    void complete_relation(const osmium::Relation& relation);

    void process_route(const osmium::Relation& relation);

    static bool roundabout_connected_to_previous_way(const osmium::NodeRef* common_node, const osmium::Way* way);

    static bool roundabout_as_second_after_gap(const osmium::Way* previous_way, const osmium::Way* way);

    static const osmium::NodeRef* roundabout_connected_to_next_way(const osmium::Way* previous_way, const osmium::Way* way);
};



#endif /* SRC_ROUTE_COLLECTOR_HPP_ */
