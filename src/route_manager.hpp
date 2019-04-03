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

/**
 * The RouteManager class assembles relations and their members we are interested in.
 */
class RouteManager : public osmium::relations::RelationsManager<RouteManager, true, true, true, false> {
    RouteWriter m_writer;
    PTv2Checker m_checker;

    bool is_ptv2(const osmium::Relation& relation) const noexcept;

    RouteError is_valid(const osmium::Relation& relation, std::vector<const osmium::OSMObject*>& member_objects);

public:
    RouteManager() = delete;

    RouteManager(OGRWriter& ogr_writer, Options& options, osmium::util::VerboseOutput& verbose_output);

    bool new_relation(const osmium::Relation& relation) const noexcept;

    void complete_relation(const osmium::Relation& relation);

    void process_route(const osmium::Relation& relation);
};



#endif /* SRC_ROUTE_COLLECTOR_HPP_ */
