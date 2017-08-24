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
    RAIL
};

class RouteManager : public osmium::relations::RelationsManager<RouteManager, true, true, true, false>, OGROutputBase {
    gdalcpp::Dataset& m_dataset;
    gdalcpp::Layer m_ptv2_routes_valid;
    gdalcpp::Layer m_ptv2_routes_invalid;
    gdalcpp::Layer m_ptv2_error_lines;
    gdalcpp::Layer m_ptv2_error_points;

    void process_route(const osmium::Relation& relation);

    bool is_ptv2(const osmium::Relation& relation);

    size_t is_valid(const osmium::Relation& relation, std::vector<const osmium::OSMObject*>& member_objects,
            std::vector<const char*>& roles);

    size_t check_roles_order_and_type(const RouteType type, std::vector<const osmium::OSMObject*>& member_objects,
            std::vector<const char*>& roles);

    size_t find_gaps(const osmium::Relation& relation, std::vector<const osmium::OSMObject*>& member_objects,
            std::vector<const char*>& roles);

    void write_valid_route(const osmium::Relation& relation, std::vector<const osmium::OSMObject*>& member_objects,
            std::vector<const char*>& roles);

    void write_invalid_route(const osmium::Relation& relation, std::vector<const osmium::OSMObject*>& member_objects,
            std::vector<const char*>& roles, size_t validation_result);

    void write_error_way(const osmium::Relation& relation, const osmium::object_id_type node_id,
            const char* error_text, const osmium::Way* way);

    void write_error_point(const osmium::Relation& relation, const osmium::NodeRef* node_ref,
            const char* error_text, const osmium::object_id_type way_id);

public:
    RouteManager() = delete;

    RouteManager(gdalcpp::Dataset& dataset, std::string& output_format,
        osmium::util::VerboseOutput& verbose_output, int epsg = 3857);

    bool new_relation(const osmium::Relation& relation) const noexcept;

    void complete_relation(const osmium::Relation& relation);

    static bool roundabout_connected_to_previous_way(const osmium::NodeRef* common_node, const osmium::Way* way);

    static const osmium::NodeRef* roundabout_connected_to_next_way(const osmium::Way* previous_way, const osmium::Way* way);
};



#endif /* SRC_ROUTE_COLLECTOR_HPP_ */
