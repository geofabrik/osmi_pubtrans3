/*
 * test_node_handler.cpp
 *
 *  Created on: 13.07.2016
 *      Author: michael
 */

#include "catch.hpp"
#include "object_builder_utilities.hpp"

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
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



TEST_CASE("check valid simple bus route") {
    Options options;
    options.output_directory = ".tmp-";
    srand (time(NULL));
    options.output_directory += std::to_string(rand());
    options.output_directory += "-testoutput.sqlite";
    if (file_exists(options.output_directory)) {
        std::cerr << options.output_directory << " already exists!\n";
        exit(1);
    }
    if (mkdir(options.output_directory.c_str(), 0744) != 0) {
        std::cerr << "Failed to create directory " << options.output_directory << '\n';
        exit(1);
    }

    osmium::util::VerboseOutput vout {false};
    OGRWriter ogr_writer{options, vout};
    RouteWriter writer (ogr_writer, options, vout);
    PTv2Checker checker(writer);

    SECTION("simple route only containing ways and stops/platforms") {
        std::vector<osmium::item_type> types = {NODE, NODE, NODE, WAY, WAY, WAY};
        std::vector<osmium::object_id_type> ids = {1, 2, 3, 1, 2, 3};

        std::map<std::string, std::string> stop_pos;
        stop_pos.emplace("highway", "bus_stop");

        std::map<std::string, std::string> tags1;
        tags1.emplace("highway", "secondary");
        static constexpr int buffer_size = 10 * 1000 * 1000;
        osmium::memory::Buffer buffer(buffer_size);

        std::map<std::string, std::string> tags_rel = test_utils::get_bus_route_tags();

        SECTION("simple route only containing ways and stops") {
            std::vector<std::string> roles = {"stop", "stop", "stop", "", "", ""};
            osmium::Relation& relation1 = test_utils::create_relation(buffer, 1, tags_rel, ids, types, roles);
            std::vector<const osmium::OSMObject*> objects {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
            RouteError error = checker.check_roles_order_and_type(relation1, objects);
            CHECK(error == RouteError::CLEAN);
        }

        SECTION("simple route only containing ways and platforms") {
            std::vector<std::string> roles = {"platform", "platform", "platform", "", "", ""};
            osmium::Relation& relation1 = test_utils::create_relation(buffer, 1, tags_rel, ids, types, roles);
            std::vector<const osmium::OSMObject*> objects {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
            RouteError error = checker.check_roles_order_and_type(relation1, objects);
            CHECK(error == RouteError::CLEAN);
        }

        SECTION("empty role on non-way") {
            std::vector<std::string> roles = {"platform", "platform", "", "", "", ""};
            osmium::Relation& relation1 = test_utils::create_relation(buffer, 1, tags_rel, ids, types, roles);
            std::vector<const osmium::OSMObject*> objects {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
            RouteError error = checker.check_roles_order_and_type(relation1, objects);
            CHECK(error == RouteError::EMPTY_ROLE_NON_WAY);
        }

        SECTION("unknown type") {;
            std::vector<std::string> roles = {"platform", "platform", "platform", "", "", ""};
            osmium::Relation& relation1 = test_utils::create_relation(buffer, 1, test_utils::get_light_rail_route_tags(), ids, types, roles);
            std::vector<const osmium::OSMObject*> objects {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
            RouteError error = checker.check_roles_order_and_type(relation1, objects);
            CHECK(error == RouteError::UNKNOWN_TYPE);
        }
    }


    SECTION("bus route over non-road") {
        std::vector<osmium::item_type> types = {NODE, NODE, NODE, WAY, WAY, WAY};
        std::vector<osmium::object_id_type> ids = {1, 2, 3, 1, 2, 3};
        std::vector<std::string> roles = {"platform", "platform", "platform", "", "", ""};

        std::map<std::string, std::string> stop_pos;
        stop_pos.emplace("highway", "bus_stop");

        std::map<std::string, std::string> tags1;
        tags1.emplace("highway", "secondary");
        std::map<std::string, std::string> tags2;
        tags2.emplace("railway", "rail");
        std::vector<const osmium::NodeRef*> node_refs1 {new osmium::NodeRef(1), new osmium::NodeRef(2), new osmium::NodeRef(3), new osmium::NodeRef(4)};
        std::vector<const osmium::NodeRef*> node_refs2 {new osmium::NodeRef(4), new osmium::NodeRef(5), new osmium::NodeRef(6), new osmium::NodeRef(7)};
        std::vector<const osmium::NodeRef*> node_refs3 {new osmium::NodeRef(7), new osmium::NodeRef(8), new osmium::NodeRef(9)};

        static constexpr int buffer_size = 10 * 1000 * 1000;
        osmium::memory::Buffer buffer(buffer_size);

        osmium::Node& node1 = test_utils::create_new_node_from_node_ref(buffer, *node_refs1[0], stop_pos);
        osmium::Node& node2 = test_utils::create_new_node_from_node_ref(buffer, *node_refs2[1], stop_pos);
        osmium::Node& node3 = test_utils::create_new_node_from_node_ref(buffer, *node_refs3[2], stop_pos);

        osmium::Way& way1 = test_utils::create_way(buffer, 1, node_refs1, tags1);
        buffer.commit();
        osmium::Way& way2 = test_utils::create_way(buffer, 2, node_refs2, tags2);
        buffer.commit();
        osmium::Way& way3 = test_utils::create_way(buffer, 3, node_refs3, tags1);
        buffer.commit();

        std::map<std::string, std::string> tags_rel = test_utils::get_bus_route_tags();
        osmium::Relation& relation1 = test_utils::create_relation(buffer, 1, tags_rel, ids, types, roles);
        std::vector<const osmium::OSMObject*> objects {&node1, &node2, &node3, &way1, &way2, &way3};
        RouteError error = checker.check_roles_order_and_type(relation1, objects);
        CHECK(error == RouteError::OVER_NON_ROAD);
    }

    SECTION("train route stops at end; unknown roles") {
        std::vector<osmium::item_type> types = {WAY, WAY, WAY, NODE, NODE, NODE};
        std::vector<osmium::object_id_type> ids = {1, 2, 3, 1, 5, 9};

        std::map<std::string, std::string> stop_pos;
        stop_pos.emplace("railway", "halt");

        std::map<std::string, std::string> tags1;
        tags1.emplace("railway", "rail");

        static constexpr int buffer_size = 10 * 1000 * 1000;
        osmium::memory::Buffer buffer(buffer_size);

        std::map<std::string, std::string> tags_rel = test_utils::get_train_route_tags();

        SECTION("train route stops at end") {
            std::vector<std::string> roles = {"", "", "", "stop", "stop", "stop"};
            osmium::Relation& relation1 = test_utils::create_relation(buffer, 1, tags_rel, ids, types, roles);
            std::vector<const osmium::OSMObject*> objects {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
            RouteError error = checker.check_roles_order_and_type(relation1, objects);
            CHECK((error & RouteError::STOPPLTF_AFTER_ROUTE) == RouteError::STOPPLTF_AFTER_ROUTE);
            CHECK((error & RouteError::NO_STOPPLTF_AT_FRONT) == RouteError::NO_STOPPLTF_AT_FRONT);
        }

        SECTION("unknown roles") {;
            std::vector<std::string> roles = {"", "", "", "ab", "stop_here", "stop3"};
            osmium::Relation& relation1 = test_utils::create_relation(buffer, 1, tags_rel, ids, types, roles);
            std::vector<const osmium::OSMObject*> objects {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
            RouteError error = checker.check_roles_order_and_type(relation1, objects);
            CHECK((error & RouteError::UNKNOWN_ROLE) == RouteError::UNKNOWN_ROLE);
        }
    }

    if (test_utils::delete_directory(options.output_directory.c_str()) != 0) {
        std::cerr << " deleting " << options.output_directory << " after running the unit test failed!\n";
        exit(1);
    }
}
