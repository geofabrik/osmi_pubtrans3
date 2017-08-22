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

#include <osmium/index/id_set.hpp>
#include <osmium/storage/item_stash.hpp>

#include "abstract_view_handler.hpp"

class RailwayHandlerPass2 : public AbstractViewHandler {
    std::unordered_map<osmium::object_id_type, osmium::ItemStash::handle_type>& m_must_on_track_handles;
    osmium::ItemStash& m_must_on_track;
    osmium::index::IdSetDense<osmium::unsigned_object_id_type>& m_via_nodes;

    gdalcpp::Layer m_points;
    gdalcpp::Layer m_on_track;


    void handle_point(const osmium::Node& node);

public:
    RailwayHandlerPass2() = delete;

    RailwayHandlerPass2(osmium::index::IdSetDense<osmium::unsigned_object_id_type>& via_nodes,
            std::unordered_map<osmium::object_id_type, osmium::ItemStash::handle_type>& must_on_track_handles,
            osmium::ItemStash& must_on_track, gdalcpp::Dataset& dataset, std::string& output_format,
            osmium::util::VerboseOutput& verbose_output, int epsg = 3857);

    void node(const osmium::Node& node);

    void way(const osmium::Way& way);

    void after_ways();

    void relation(const osmium::Relation&);
};



#endif /* SRC_RAILWAY_HANDLER_PASS2_HPP_ */
