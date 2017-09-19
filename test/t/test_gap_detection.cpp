/*
 * test_node_handler.cpp
 *
 *  Created on: 13.07.2016
 *      Author: michael
 */

#include "catch.hpp"
#include "object_builder_utilities.hpp"

#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <gdalcpp.hpp>
#include <ptv2_checker.hpp>

bool file_exists(std::string& path) {
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);
}

static osmium::item_type NODE = osmium::item_type::node;
static osmium::item_type WAY = osmium::item_type::way;
static osmium::item_type RELATION = osmium::item_type::relation;



std::map<std::string, std::string> get_bus_route_tags() {
    std::map<std::string, std::string> bus_route_tags;
    bus_route_tags.emplace("type", "route");
    bus_route_tags.emplace("route", "bus");
    bus_route_tags.emplace("public_transport:version", "2");
    return bus_route_tags;
}

std::map<std::string, std::string> get_train_route_tags() {
    std::map<std::string, std::string> train_route_tags;
    train_route_tags.emplace("type", "route");
    train_route_tags.emplace("route", "train");
    train_route_tags.emplace("public_transport:version", "2");
    return train_route_tags;
}

std::map<std::string, std::string> get_light_rail_route_tags() {
    std::map<std::string, std::string> train_route_tags;
    train_route_tags.emplace("type", "route");
    train_route_tags.emplace("route", "light_rail");
    train_route_tags.emplace("public_transport:version", "2");
    return train_route_tags;
}

std::vector<const char*> build_roles_cstr_vec(const std::vector<std::string>& from) {
    std::vector<const char*> roles_cstr;
    for (const std::string& str : from) {
        roles_cstr.push_back(str.c_str());
    }
    return roles_cstr;
}


TEST_CASE("check if gap detection works") {
    std::string format = "SQlite";
    std::string dataset_name = ".tmp-";
    srand (time(NULL));
    dataset_name += std::to_string(rand());
    dataset_name += "-testoutput.sqlite";
    if (file_exists(dataset_name)) {
        std::cerr << dataset_name << " already exists!\n";
        exit(1);
    }
    gdalcpp::Dataset dataset("SQlite", dataset_name, gdalcpp::SRS(4326),
            OGROutputBase::get_gdal_default_options(format));
    osmium::util::VerboseOutput vout {false};
    RouteWriter writer (dataset, format, vout);
    PTv2Checker checker(writer);

    SECTION("no gaps") {
        std::vector<osmium::item_type> types = {NODE, NODE, WAY, WAY, WAY};
        std::vector<osmium::object_id_type> ids = {1, 2, 1, 2, 3};
        std::vector<std::string> roles = {"platform", "platform", "", "", ""};
        std::vector<const char*> roles_cstr = build_roles_cstr_vec(roles);

        std::map<std::string, std::string> stop_pos;
        stop_pos.emplace("highway", "bus_stop");

        std::map<std::string, std::string> tags1;
        tags1.emplace("highway", "secondary");
        std::vector<const osmium::NodeRef*> node_refs1 {new osmium::NodeRef(1), new osmium::NodeRef(2), new osmium::NodeRef(3), new osmium::NodeRef(4)};
        std::vector<const osmium::NodeRef*> node_refs2 {new osmium::NodeRef(4), new osmium::NodeRef(5), new osmium::NodeRef(6), new osmium::NodeRef(7)};
        std::vector<const osmium::NodeRef*> node_refs3 {new osmium::NodeRef(7), new osmium::NodeRef(8), new osmium::NodeRef(9)};

        static constexpr int buffer_size = 10 * 1000 * 1000;
        osmium::memory::Buffer buffer(buffer_size);

        osmium::Node& node1 = test_utils::create_new_node_from_node_ref(buffer, *node_refs1[0], stop_pos);
        osmium::Node& node2 = test_utils::create_new_node_from_node_ref(buffer, *node_refs3[2], stop_pos);

        osmium::Way& way1 = test_utils::create_way(buffer, 1, node_refs1, tags1);
        buffer.commit();
        osmium::Way& way2 = test_utils::create_way(buffer, 2, node_refs2, tags1);
        buffer.commit();
        osmium::Way& way3 = test_utils::create_way(buffer, 3, node_refs3, tags1);
        buffer.commit();

        std::map<std::string, std::string> tags_rel = get_bus_route_tags();
        osmium::Relation& relation1 = test_utils::create_relation(buffer, 1, tags_rel, ids, types, roles);
        std::vector<const osmium::OSMObject*> objects {&node1, &node2, &way1, &way2, &way3};
        RouteError error = checker.find_gaps(relation1, objects, roles_cstr);
        CHECK(error == RouteError::CLEAN);
    }

    if (std::remove(dataset_name.c_str()) != 0) {
        std::cerr << " deleting " << dataset_name << " after running the unit test failed!\n";
        exit(1);
    }
}
