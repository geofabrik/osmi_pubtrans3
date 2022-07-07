/*
 * ptv2_checker.hpp
 *
 *  Created on:  2017-09-18
 *      Author: Michael Reichert <michael.reichert@geofabrik.de>
 */

#ifndef SRC_PTV2_CHECKER_HPP_
#define SRC_PTV2_CHECKER_HPP_

#include "route_writer.hpp"

/**
 * This classed enum tracks the status of the current and the previous member processed by the gap checker.
 */
enum class MemberStatus : char {
    /// We have not seen the first member which is neither a stop nor a platform.
    BEFORE_FIRST = 0,
    /// first member which is neither a stop nor a platform
    FIRST = 1,
    /// second member which is neither a stop nor a platform
    SECOND = 2,
    /// any other member which is neither a stop nor a platform and which does not have a special status
    NORMAL = 3,
    /// The first way after way which is not connected to its predecessor.
    AFTER_GAP = 4,
    /// The current way is missing (incomplete relation).
    MISSING = 5,
    /// The previous way is missing (incomplete relation).
    AFTER_MISSING = 6,
    /// This member way is a roundabout.
    ROUNDABOUT = 7,
    /// a roundabout which is either the second way in the route or the second way after a gap
    SECOND_ROUNDABOUT = 8,
    /// The previous member way is a roundabout.
    AFTER_ROUNDABOUT = 9
};

enum class BackOrFront : char {
    UNDEFINED = 0,
    FRONT = 1,
    BACK = 2
};

struct WayNodeWithOrderID {
	/// Node ID
	osmium::object_id_type id;
	/// index (= offset from begin) of the way in a list of all member ways with an empty role
	size_t way_index;
	bool found = false;

	explicit WayNodeWithOrderID(osmium::object_id_type node_ref, size_t index) :
			id(node_ref),
			way_index(index) {}

	bool operator<(const WayNodeWithOrderID& rhs) const {
		return this->way_index < rhs.way_index || (this->way_index == rhs.way_index && this->id < rhs.id);
	}
};

/**
 * This class provides methods to check the validity of a route relation.
 */
class PTv2Checker {
    RouteWriter& m_writer;

    RouteError role_check_handle_road_member(const osmium::Relation& relation, const RouteType type,
            const osmium::OSMObject* object, const bool seen_stop_platform);

    RouteError handle_errorneous_stop_platform(const osmium::Relation& relation, const osmium::OSMObject* object);

    RouteError handle_stop_not_on_way(const osmium::Relation& relation, const osmium::Node* node);

    RouteError handle_stop_wrong_order(const osmium::Relation& relation, const osmium::Node* node);

    RouteError handle_unknown_role(const osmium::Relation& relation, const osmium::OSMObject* object, const char* role);

    int gap_detector_member_handling(const osmium::Relation& relation, /*const osmium::OSMObject* object,*/
            const osmium::Way* way,
            const osmium::Way* previous_way,
            osmium::RelationMemberList::const_iterator member_it, MemberStatus& status, BackOrFront& previous_way_end);

    static const osmium::NodeRef* back_or_front_to_node_ref(BackOrFront back_or_front, const osmium::Way* way);

public:
    PTv2Checker() = delete;

    PTv2Checker(RouteWriter& writer);

    /**
     * Determine the type of the route.
     *
     * \param route value of the route tag of the relation
     *
     * \returns type of the route
     */
    RouteType get_route_type(const char* route);

    /**
     * Is the role a way (including the empty string and `hail_and_stop`)?
     *
     * \param role string
     */
    bool is_way(const char* role);

    /**
     * Is the role a stop (including `stop_exit_only` and `stop_entry_only`)?
     *
     * \param role string
     */
    bool is_stop(const char* role);

    /**
     * Is the role a platform (including `platform_exit_only` and `platform_entry_only`)?
     *
     * \param role string
     */
    bool is_platform(const char* role);

    /**
     * Check if the type of transport tags (bus=*, train=* etc.) match the type of the route.
     *
     * This method is intended for stop positions and platforms.
     *
     * \param tags list of tags of the object (stop/platform)
     *
     * \param type type of the route
     */
    bool vehicle_tags_matches_route_type(const osmium::TagList& tags, RouteType type);

    /**
     * Check if the way is a valid member for of a train, subway or tram route relation.
     *
     * \param type type of the route
     *
     * \param member_tags tag list of the member to be checked
     */
    bool check_valid_railway_track(RouteType type, const osmium::TagList& member_tags);

    /**
     * Check if the way is a valid member for of a bus route relation.
     *
     * \param member_tags tag list of the member to be checked
     */
    bool check_valid_road_way(const osmium::TagList& member_tags);

    /**
     * Check if the way is a valid member for of a trolleybus route relation.
     *
     * \todo This method does only check if `trolley_wire:forward/backward=*` exists but not if it leads into the right direction.
     *
     * \param member_tags tag list of the member to be checked
     */
    bool check_valid_trolleybus_way(const osmium::TagList& member_tags);

    /**
     * Check if the way is a valid member for of a ferry route relation.
     *
     * This method is also useable for train routes over train ferries and bus routes over ferries.
     *
     * \param member_tags tag list of the member to be checked
     *
     * \param permit_untagged_ways Set to true if you check the members of a ferry route, false otherwise.
     */
    bool is_ferry(const osmium::TagList& member_tags, bool permit_untagged_ways = false);

    /**
     * Check if a roundabout (closed way) is connected to the front or back node of the previous way.
     *
     * This method checks all nodes of the roundabout way to be equal by ID with the either the front
     * or the back node of the previous way.
     *
     * \param back_or_front Should the front or the back node be connected to the roundabout?
     *
     * \param previous_way pointer to the previous way
     *
     * \param way pointer to the way (the roundabout)
     */
    bool roundabout_connected_to_previous_way(const BackOrFront back_or_front, const osmium::Way* previous_way, const osmium::Way* way);

    /**
     * Check if a roundabout (closed way) is connected to the front or back node of the previous way.
     *
     * Use this method if the roundabout is the second highway/railway member in the route or the second after a gap.
     *
     * This method checks all nodes of the roundabout way to be equal by ID with the either the front
     * or the back node of the previous way. Both the front and the back node are checked. Therefore, use
     * this method only if you don't know the open end of the previous way.
     *
     * \param previous_way pointer to the previous way
     *
     * \param way pointer to the way (the roundabout)
     */
    bool roundabout_as_second_after_gap(const osmium::Way* previous_way, const osmium::Way* way);

    /**
     * How is a roundabout (closed way) which is the previous way connected to the currently handled way?
     *
     * This method checks all nodes of the roundabout way to be equal by ID with the either the front
     * or the back node of the previous way. Both the front and the back node are checked.
     *
     * \param previous_way pointer to the previous way
     *
     * \param way pointer to the way (the roundabout)
     *
     * \return whether the front or the back node of the way is connected to the open end.
     */
    BackOrFront roundabout_connected_to_next_way(const osmium::Way* previous_way, const osmium::Way* way);

    /**
     * Check if a way which is neither a stop nor platform is a useable highway/railway/ferry segment for the
     * given route.
     *
     * \param relation route relation
     *
     * \param type type of the route
     *
     * \param way way to be checked
     */
    RouteError is_way_usable(const osmium::Relation& relation, RouteType type, const osmium::Way* way);

    /**
     * Check if a stop is tagged properly.
     *
     * \param relation route relation
     *
     * \param type type of the route
     *
     * \param node member node to be checked
     */
    RouteError check_stop_tags(const osmium::Relation& relation, const osmium::Node* node, RouteType type);

    /**
     * Check if a platform is tagged properly.
     *
     * \param relation route relation
     *
     * \param type type of the route
     *
     * \param object member object to be checked
     */
    RouteError check_platform_tags(const osmium::Relation& relation, const RouteType type, const osmium::OSMObject* object);

    /*
     * Check the correct order of the members of the relation without looking on their geometry.
     */
    RouteError check_roles_order_and_type(const osmium::Relation& relation, std::vector<const osmium::OSMObject*>& member_objects);

    /**
     * Count the number of gaps in a route and write errors using the RouteWriter class if any.
     *
     * \param relation relation to be checked
     *
     * \param member_objects vector of pointers to the member objects
     *
     * \return number of gaps
     */
    int find_gaps(const osmium::Relation& relation, std::vector<const osmium::OSMObject*>& member_objects);
};



#endif /* SRC_PTV2_CHECKER_HPP_ */
