/*
 * turn_restriction_handler.hpp
 *
 *  Created on:  2017-08-21
 *      Author: Michael Reichert <michael.reichert@geofabrik.de>
 */

#ifndef SRC_TURN_RESTRICTION_HANDLER_HPP_
#define SRC_TURN_RESTRICTION_HANDLER_HPP_

#include <osmium/handler.hpp>
#include <osmium/index/id_set.hpp>
#include <osmium/osm/relation.hpp>

/**
 * This handler populates a IdSet with IDs of all nodes which are a via member of a turn restriction.
 */
class TurnRestrictionHandler : public osmium::handler::Handler {
private:
    osmium::index::IdSetDense<osmium::unsigned_object_id_type>& m_point_node_members;

public:
    TurnRestrictionHandler() =  delete;

    TurnRestrictionHandler(osmium::index::IdSetDense<osmium::unsigned_object_id_type>& point_node_members);

    void relation(const osmium::Relation& relation);
};



#endif /* SRC_TURN_RESTRICTION_HANDLER_HPP_ */
