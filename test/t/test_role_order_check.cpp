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

static osmium::item_type NODE = osmium::item_type::node;
static osmium::item_type WAY = osmium::item_type::way;


TEST_CASE("check valid simple bus route") {
    Options options;
    options.output_directory = ".tmp-";
    options.output_format = "GeoJSON";
    srand (time(NULL));
    options.output_directory += std::to_string(rand());
    options.output_directory += "-testoutput.sqlite";
    if (test_utils::file_exists(options.output_directory)) {
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

    SECTION("Stop positions (not) on the route") {
        std::vector<osmium::item_type> types = {NODE, NODE, NODE, NODE, WAY, WAY, WAY, WAY, WAY};
        std::vector<std::string> roles = {"stop", "stop", "stop", "stop", "", "", "", "", ""};

        std::map<std::string, std::string> stop_pos;
        stop_pos.emplace("public_transport", "stop_position");

        std::map<std::string, std::string> tags1;
        tags1.emplace("highway", "secondary");

        static constexpr int buffer_size = 10 * 1000 * 1000;
        osmium::memory::Buffer buffer(buffer_size);

        std::map<std::string, std::string> tags_rel = test_utils::get_bus_route_tags();

        osmium::Node& node1 = test_utils::create_new_node(buffer, 1, osmium::Location{}, stop_pos);
        buffer.commit();
        osmium::Node& node2 = test_utils::create_new_node(buffer, 2, osmium::Location{}, stop_pos);
        buffer.commit();
        osmium::Node& node8 = test_utils::create_new_node(buffer, 8, osmium::Location{}, stop_pos);
        buffer.commit();
        osmium::Node& node9 = test_utils::create_new_node(buffer, 9, osmium::Location{}, stop_pos);
        buffer.commit();
        osmium::Node& node10 = test_utils::create_new_node(buffer, 10, osmium::Location{}, stop_pos);
        buffer.commit();
        osmium::Node& node11 = test_utils::create_new_node(buffer, 11, osmium::Location{}, stop_pos);
        buffer.commit();
        osmium::Node& node13 = test_utils::create_new_node(buffer, 13, osmium::Location{}, stop_pos);
        buffer.commit();
        osmium::Node& node99 = test_utils::create_new_node(buffer, 13, osmium::Location{}, stop_pos);
        buffer.commit();

        std::vector<const osmium::NodeRef*> node_refs1 {new osmium::NodeRef(1), new osmium::NodeRef(2), new osmium::NodeRef(3), new osmium::NodeRef(4)};
        std::vector<const osmium::NodeRef*> node_refs2 {new osmium::NodeRef(4), new osmium::NodeRef(5), new osmium::NodeRef(6), new osmium::NodeRef(99), new osmium::NodeRef(7)};
        std::vector<const osmium::NodeRef*> node_refs3 {new osmium::NodeRef(7), new osmium::NodeRef(8), new osmium::NodeRef(9)};
        std::vector<const osmium::NodeRef*> node_refs4 {new osmium::NodeRef(9), new osmium::NodeRef(10), new osmium::NodeRef(11)};
        std::vector<const osmium::NodeRef*> node_refs5 {new osmium::NodeRef(13), new osmium::NodeRef(12), new osmium::NodeRef(11)};

        osmium::Way& way1 = test_utils::create_way(buffer, 1, node_refs1, tags1);
        buffer.commit();
        osmium::Way& way2 = test_utils::create_way(buffer, 2, node_refs2, tags1);
        buffer.commit();
        osmium::Way& way3 = test_utils::create_way(buffer, 3, node_refs3, tags1);
        buffer.commit();
        osmium::Way& way4 = test_utils::create_way(buffer, 4, node_refs4, tags1);
        buffer.commit();
        osmium::Way& way5 = test_utils::create_way(buffer, 5, node_refs5, tags1);
        buffer.commit();

        SECTION("correct stop order – this is valid") {
            std::vector<osmium::object_id_type> ids = {1, 2, 8, 11, 1, 2, 3, 4, 5};
            std::vector<const osmium::OSMObject*> objects {&node1, &node2, &node8, &node11, &way1, &way2, &way3, &way4, &way5};

            osmium::Relation& relation1 = test_utils::create_relation(buffer, 1, tags_rel, ids, types, roles, objects);
            RouteError error = checker.check_roles_order_and_type(relation1, objects);
            CHECK(error== RouteError::CLEAN);
        }

        SECTION("correct stop order but the route includes a loop") {
            types = {NODE, NODE, NODE, NODE, NODE, WAY, WAY, WAY, WAY, WAY, WAY};
            roles = {"stop", "stop", "stop", "stop", "stop", "", "", "", "", "", ""};
            std::vector<osmium::object_id_type> ids = {1, 2, 8, 8, 13, 1, 2, 3, 4, 3, 5};
            std::vector<const osmium::OSMObject*> objects {&node1, &node2, &node8, &node8, &node13, &way1, &way2, &way3, &way4, &way3, &way5};

            osmium::Relation& relation1 = test_utils::create_relation(buffer, 1, tags_rel, ids, types, roles, objects);
            RouteError error = checker.check_roles_order_and_type(relation1, objects);
            CHECK(error== RouteError::CLEAN);
        }

        SECTION("inverse stop order – this is invalid") {
            std::vector<osmium::object_id_type> ids = {11, 8, 2, 1, 1, 2, 3, 4, 5};
            std::vector<const osmium::OSMObject*> objects {&node11, &node8, &node2, &node1, &way1, &way2, &way3, &way4, &way5};

            osmium::Relation& relation1 = test_utils::create_relation(buffer, 1, tags_rel, ids, types, roles, objects);
            RouteError error = checker.check_roles_order_and_type(relation1, objects);
            CHECK((error & RouteError::STOP_MISORDERED) == RouteError::STOP_MISORDERED);
        }

        SECTION("two stops swapped – this is invalid") {
            std::vector<osmium::object_id_type> ids = {1, 8, 2, 11, 1, 2, 3, 4, 5};
            std::vector<const osmium::OSMObject*> objects {&node11, &node8, &node2, &node1, &way1, &way2, &way3, &way4, &way5};

            osmium::Relation& relation1 = test_utils::create_relation(buffer, 1, tags_rel, ids, types, roles, objects);
            RouteError error = checker.check_roles_order_and_type(relation1, objects);
            CHECK((error & RouteError::STOP_MISORDERED) == RouteError::STOP_MISORDERED);
        }

        SECTION("two stops swapped – this is invalid") {
            std::vector<osmium::object_id_type> ids = {1, 8, 2, 11, 1, 2, 3, 4, 5};
            std::vector<const osmium::OSMObject*> objects {&node11, &node8, &node2, &node1, &way1, &way2, &way3, &way4, &way5};

            osmium::Relation& relation1 = test_utils::create_relation(buffer, 1, tags_rel, ids, types, roles, objects);
            RouteError error = checker.check_roles_order_and_type(relation1, objects);
            CHECK((error & RouteError::STOP_MISORDERED) == RouteError::STOP_MISORDERED);
        }

        SECTION("correct stop order – but two stops belong to the same way") {
            std::vector<osmium::object_id_type> ids = {1, 2, 9, 11, 1, 2, 3, 4, 5};
            std::vector<const osmium::OSMObject*> objects {&node1, &node2, &node9, &node11, &way1, &way2, &way3, &way4, &way5};

            osmium::Relation& relation1 = test_utils::create_relation(buffer, 1, tags_rel, ids, types, roles, objects);
            RouteError error = checker.check_roles_order_and_type(relation1, objects);
            CHECK(error== RouteError::CLEAN);
        }

        SECTION("undetectable wrong stop order – but the two stops belong to the same way") {
            std::vector<osmium::object_id_type> ids = {1, 2, 11, 10, 1, 2, 3, 4, 5};
            std::vector<const osmium::OSMObject*> objects {&node1, &node2, &node11, &node10, &way1, &way2, &way3, &way4, &way5};

            osmium::Relation& relation1 = test_utils::create_relation(buffer, 1, tags_rel, ids, types, roles, objects);
            RouteError error = checker.check_roles_order_and_type(relation1, objects);
            CHECK(error== RouteError::CLEAN);
        }

        SECTION("undetectable wrong stop order – but the two stops belong to the same/first way") {
            std::vector<osmium::object_id_type> ids = {2, 1, 9, 11, 1, 2, 3, 4, 5};
            std::vector<const osmium::OSMObject*> objects {&node2, &node1, &node9, &node11, &way1, &way2, &way3, &way4, &way5};

            osmium::Relation& relation1 = test_utils::create_relation(buffer, 1, tags_rel, ids, types, roles, objects);
            RouteError error = checker.check_roles_order_and_type(relation1, objects);
            CHECK(error== RouteError::CLEAN);
        }

        SECTION("messed up stop order – but each two of the four stops belong to the same ways") {
            std::vector<osmium::object_id_type> ids = {9, 11, 1, 2, 1, 2, 3, 4, 5};
            std::vector<const osmium::OSMObject*> objects {&node9, &node11, &node1, &node2, &way1, &way2, &way3, &way4, &way5};

            osmium::Relation& relation1 = test_utils::create_relation(buffer, 1, tags_rel, ids, types, roles, objects);
            RouteError error = checker.check_roles_order_and_type(relation1, objects);
            CHECK(error== RouteError::STOP_MISORDERED);
            CHECK((error & RouteError::STOP_MISORDERED) == RouteError::STOP_MISORDERED);
        }

        SECTION("correct stop order but one highway member has an invalid role") {
        	roles = {"stop", "stop", "stop", "stop", "forward", "", "", "", ""};
            std::vector<osmium::object_id_type> ids = {1, 2, 8, 11, 1, 2, 3, 4, 5};
            std::vector<const osmium::OSMObject*> objects {&node11, &node8, &node2, &node1, &way1, &way2, &way3, &way4, &way5};

            osmium::Relation& relation1 = test_utils::create_relation(buffer, 1, tags_rel, ids, types, roles, objects);
            RouteError error = checker.check_roles_order_and_type(relation1, objects);
            CHECK((error & RouteError::STOP_NOT_ON_WAY) == RouteError::STOP_NOT_ON_WAY);
        }

        SECTION("correct stop order but one highway member has an invalid role") {
        	roles = {"stop", "stop", "stop", "stop", "", "", "forward", "", ""};
            std::vector<osmium::object_id_type> ids = {1, 2, 8, 11, 1, 2, 3, 4, 5};
            std::vector<const osmium::OSMObject*> objects {&node11, &node8, &node2, &node1, &way1, &way2, &way3, &way4, &way5};

            osmium::Relation& relation1 = test_utils::create_relation(buffer, 1, tags_rel, ids, types, roles, objects);
            RouteError error = checker.check_roles_order_and_type(relation1, objects);
            CHECK((error & RouteError::STOP_NOT_ON_WAY) == RouteError::STOP_NOT_ON_WAY);
        }

        SECTION("correct stop order, one highway member has a non-empy, valid role") {
        	roles = {"stop", "stop", "stop", "stop", "", "", "hail_and_ride", "", ""};
            std::vector<osmium::object_id_type> ids = {1, 2, 8, 11, 1, 2, 3, 4, 5};
            std::vector<const osmium::OSMObject*> objects {&node11, &node8, &node2, &node1, &way1, &way2, &way3, &way4, &way5};

            osmium::Relation& relation1 = test_utils::create_relation(buffer, 1, tags_rel, ids, types, roles, objects);
            RouteError error = checker.check_roles_order_and_type(relation1, objects);
            CHECK(error == RouteError::CLEAN);
        }

        SECTION("correct stop order but the route includes a loop (one stop in the loop served twice, the other one once") {
            types = {NODE, NODE, NODE, NODE, NODE, NODE, WAY, WAY, WAY, WAY, WAY, WAY};
            roles = {"stop", "stop", "stop", "stop", "stop", "stop", "", "", "", "", "", ""};
            std::vector<osmium::object_id_type> ids = {1, 2, 8, 10, 9, 13, 1, 2, 3, 4, 3, 5};
            std::vector<const osmium::OSMObject*> objects {&node1, &node2, &node8, &node10, &node9, &node13, &way1, &way2, &way3, &way4, &way3, &way5};

            osmium::Relation& relation1 = test_utils::create_relation(buffer, 1, tags_rel, ids, types, roles, objects);
            RouteError error = checker.check_roles_order_and_type(relation1, objects);
            CHECK(error== RouteError::CLEAN);
        }
    }

    if (test_utils::delete_directory(options.output_directory.c_str()) != 0) {
        std::cerr << " deleting " << options.output_directory << " after running the unit test failed!\n";
        exit(1);
    }
}
