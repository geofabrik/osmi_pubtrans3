/*
 * route_collector.hpp
 *
 *  Created on:  2017-08-23
 *      Author: Michael Reichert <michael.reichert@geofabrik.de>
 */

#ifndef SRC_ROUTE_COLLECTOR_HPP_
#define SRC_ROUTE_COLLECTOR_HPP_

#include <osmium/relations/relations_manager.hpp>
#include "ptv2_checker.hpp"

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

class RouteManager : public osmium::relations::RelationsManager<RouteManager, true, true, true, false> {
    RouteWriter m_writer;
    PTv2Checker m_checker;

    bool is_ptv2(const osmium::Relation& relation) const noexcept;

    RouteError is_valid(const osmium::Relation& relation, std::vector<const osmium::OSMObject*>& member_objects,
            std::vector<const char*>& roles);

    RouteError find_gaps(const osmium::Relation& relation, std::vector<const osmium::OSMObject*>& member_objects,
            std::vector<const char*>& roles);

public:
    RouteManager() = delete;

    RouteManager(gdalcpp::Dataset& dataset, std::string& output_format,
        osmium::util::VerboseOutput& verbose_output, int epsg = 3857);

    bool new_relation(const osmium::Relation& relation) const noexcept;

    void complete_relation(const osmium::Relation& relation);

    void process_route(const osmium::Relation& relation);
};



#endif /* SRC_ROUTE_COLLECTOR_HPP_ */
