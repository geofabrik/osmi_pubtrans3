/*
 * ptv2_checker.hpp
 *
 *  Created on:  2017-09-18
 *      Author: Michael Reichert <michael.reichert@geofabrik.de>
 */

#ifndef SRC_PTV2_CHECKER_HPP_
#define SRC_PTV2_CHECKER_HPP_

#include "route_writer.hpp"

class PTv2Checker {
    RouteWriter& m_writer;

public:
    PTv2Checker() = delete;

    PTv2Checker(RouteWriter& writer);

    RouteType get_route_type(const char* route);

    bool is_stop(const char* role);

    bool is_platform(const char* role);

    bool vehicle_tags_matches_route_type(const osmium::TagList& tags, RouteType type);

    bool check_valid_railway_track(RouteType type, const osmium::TagList& member_tags);

    bool check_valid_road_way(const osmium::TagList& member_tags);

    bool check_valid_trolleybus_way(const osmium::TagList& member_tags);

    bool is_ferry(const osmium::TagList& member_tags);

    bool roundabout_connected_to_previous_way(const osmium::NodeRef* common_node, const osmium::Way* way);

    bool roundabout_as_second_after_gap(const osmium::Way* previous_way, const osmium::Way* way);

    const osmium::NodeRef* roundabout_connected_to_next_way(const osmium::Way* previous_way, const osmium::Way* way);
};



#endif /* SRC_PTV2_CHECKER_HPP_ */
