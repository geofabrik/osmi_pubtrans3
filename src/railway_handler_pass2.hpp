/*
 * railway_handler_pass2.hpp
 *
 *  Created on:  2017-08-22
 *      Author: Michael Reichert <michael.reichert@geofabrik.de>
 */

#ifndef SRC_RAILWAY_HANDLER_PASS2_HPP_
#define SRC_RAILWAY_HANDLER_PASS2_HPP_

#include <unordered_map>
#include <memory>

#include <osmium/handler.hpp>
#include <osmium/index/id_set.hpp>
#include <osmium/storage/item_stash.hpp>

#include "ogr_output_base.hpp"

/**
 * This handler class creates the points layer (`railway=switch`) and
 * the layer of nodes which should be referenced by a way due to their tags (signals, points, stop positions etc.).
 */
class RailwayHandlerPass2 : public osmium::handler::Handler, public OGROutputBase {

    /// The map holds the handles which point to the objects in the ItemStash and makes them accessible by their OSM ID.
    std::unordered_map<osmium::object_id_type, osmium::ItemStash::handle_type>& m_must_on_track_handles;

    /// item stash holds OSM objects to all nodes which have to be referenced by a way
    osmium::ItemStash& m_must_on_track;

    /// Set of IDs which contain all via nodes of all turn restrictions
    osmium::index::IdSetDense<osmium::unsigned_object_id_type>& m_via_nodes;

    Options& m_options;

    /// GDAL layer for nodes which should be referenced by a way but are not
    gdalcpp::Layer m_on_track;

    /// GDAL layer for points (`railway=switch`)
    std::unique_ptr<gdalcpp::Layer> m_points;

    void handle_point(const osmium::Node& node);

public:
    RailwayHandlerPass2() = delete;

    RailwayHandlerPass2(OGRWriter& writer, osmium::index::IdSetDense<osmium::unsigned_object_id_type>& via_nodes,
            std::unordered_map<osmium::object_id_type, osmium::ItemStash::handle_type>& must_on_track_handles,
            osmium::ItemStash& must_on_track, Options& options, osmium::util::VerboseOutput& verbose_output);

    void node(const osmium::Node& node);

    void way(const osmium::Way& way);

    void after_ways();

    void relation(const osmium::Relation&);
};



#endif /* SRC_RAILWAY_HANDLER_PASS2_HPP_ */
