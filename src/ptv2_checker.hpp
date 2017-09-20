/*
 * ptv2_checker.hpp
 *
 *  Created on:  2017-09-18
 *      Author: Michael Reichert <michael.reichert@geofabrik.de>
 */

#ifndef SRC_PTV2_CHECKER_HPP_
#define SRC_PTV2_CHECKER_HPP_

#include "route_writer.hpp"

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

enum class BackOrFront : char {
    UNDEFINED = 0,
    FRONT = 1,
    BACK = 2
};

class PTv2Checker {
    RouteWriter& m_writer;

    RouteError role_check_handle_road_member(const osmium::Relation& relation, const RouteType type,
            const osmium::OSMObject* object, const bool seen_stop_platform);

    RouteError handle_errorneous_stop_platform(const osmium::Relation& relation, const osmium::OSMObject* object);

    RouteError handle_unknown_role(const osmium::Relation& relation, const osmium::OSMObject* object, const char* role);

    int gap_detector_member_handling(const osmium::Relation& relation, /*const osmium::OSMObject* object,*/
            const osmium::Way* way,
            const osmium::Way* previous_way,
            osmium::RelationMemberList::const_iterator member_it, MemberStatus& status, BackOrFront& previous_way_end);

    static const osmium::NodeRef* back_or_front_to_node_ref(BackOrFront back_or_front, const osmium::Way* way);

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

    bool roundabout_connected_to_previous_way(const BackOrFront, const osmium::Way* previous_way, const osmium::Way* way);

    bool roundabout_as_second_after_gap(const osmium::Way* previous_way, const osmium::Way* way);

    BackOrFront roundabout_connected_to_next_way(const osmium::Way* previous_way, const osmium::Way* way);

    RouteError is_way_usable(const osmium::Relation& relation, RouteType type, const osmium::Way* way);

    RouteError check_stop_tags(const osmium::Relation& relation, const osmium::Node* node, RouteType type);

    RouteError check_platform_tags(const osmium::Relation& relation, const RouteType type, const osmium::OSMObject* object);

    /*
     * Check the correct order of the members of the relation without looking on their geometry.
     */
    RouteError check_roles_order_and_type(const osmium::Relation& relation, std::vector<const osmium::OSMObject*>& member_objects);

    int find_gaps(const osmium::Relation& relation, std::vector<const osmium::OSMObject*>& member_objects);
};



#endif /* SRC_PTV2_CHECKER_HPP_ */
