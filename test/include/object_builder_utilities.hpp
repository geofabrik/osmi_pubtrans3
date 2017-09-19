/*
 * object_builder_utilities.hpp
 *
 *  Created on: 12.10.2016
 *      Author: michael
 */

/**
 * Helper methods used by various tests to build OSM objects with libosmium.
 */

#ifndef TEST_INCLUDE_OBJECT_BUILDER_UTILITIES_HPP_
#define TEST_INCLUDE_OBJECT_BUILDER_UTILITIES_HPP_

#include <map>
#include <osmium/builder/osm_object_builder.hpp>
#include <osmium/memory/buffer.hpp>
#include <osmium/osm/node.hpp>
#include <osmium/memory/buffer.hpp>
#include <osmium/builder/osm_object_builder.hpp>
#include <iostream>

using tagmap = std::map<std::string, std::string>;

namespace test_utils {

    void add_tags(osmium::memory::Buffer& buffer, osmium::builder::Builder* builder, const tagmap& tags) {
        osmium::builder::TagListBuilder tl_builder(buffer, builder);
        for (tagmap::const_iterator it = tags.begin(); it != tags.end(); it++) {
            tl_builder.add_tag(it->first, it->second);
        }
    }

    void add_relation_members(osmium::memory::Buffer& buffer, osmium::builder::Builder* builder,
            std::vector<osmium::object_id_type>& member_ids, std::vector<osmium::item_type>& member_types,
            std::vector<std::string>& member_roles) {
        assert(member_ids.size() == member_types.size() && member_ids.size() == member_roles.size());
        osmium::builder::RelationMemberListBuilder rml_builder(buffer, builder);
        for (size_t i = 0; i < member_ids.size(); i++) {
            rml_builder.add_member(member_types.at(i), member_ids.at(i), member_roles.at(i).c_str());
        }
    }

    void set_dummy_osm_object_attributes(osmium::OSMObject& object) {
        object.set_version("1");
        object.set_changeset("5");
        object.set_uid("140");
        object.set_timestamp(osmium::Timestamp("2016-01-05T01:22:45Z"));
    }

    osmium::Node& create_new_node(osmium::memory::Buffer& buffer, const osmium::object_id_type id, const osmium::Location& location,
            const tagmap& tags) {
        osmium::builder::NodeBuilder node_builder(buffer);
        osmium::Node& node = static_cast<osmium::Node&>(node_builder.object());
        node.set_id(id);
        set_dummy_osm_object_attributes(node);
        node_builder.set_user("");
        node.set_location(location);
        add_tags(buffer, &node_builder, tags);
        return node_builder.object();
    }

    osmium::Node& create_new_node_from_node_ref(osmium::memory::Buffer& buffer, const osmium::NodeRef& node_ref,
            const tagmap& tags) {
        return create_new_node(buffer, node_ref.ref(), node_ref.location(), tags);
    }

    void add_node_references(osmium::memory::Buffer& buffer, osmium::builder::WayBuilder* way_builder,
            std::vector<const osmium::NodeRef*>& node_refs) {
        osmium::builder::WayNodeListBuilder wnl_builder(buffer, way_builder);
        for (const osmium::NodeRef* node_ref : node_refs) {
            wnl_builder.add_node_ref(*node_ref);
        }
    }

    osmium::Way& create_way(osmium::memory::Buffer& buffer, const osmium::object_id_type id,
            std::vector<const osmium::NodeRef*>& node_refs, const tagmap& tags) {
        osmium::builder::WayBuilder way_builder(buffer);
        osmium::Way& way = static_cast<osmium::Way&>(way_builder.object());
        way.set_id(1);
        test_utils::set_dummy_osm_object_attributes(way);
        way_builder.set_user("");
        add_tags(buffer, &way_builder, tags);
        add_node_references(buffer, &way_builder, node_refs);
        return way;
    }

    osmium::Relation& create_relation(osmium::memory::Buffer& buffer, const osmium::object_id_type id, const tagmap& tags,
            std::vector<osmium::object_id_type>& member_ids, std::vector<osmium::item_type>& member_types,
            std::vector<std::string>& member_roles) {
        osmium::builder::RelationBuilder relation_builder(buffer);
        osmium::Relation& relation = static_cast<osmium::Relation&>(relation_builder.object());
        relation.set_id(1);
        set_dummy_osm_object_attributes(relation);
        relation_builder.set_user("");
        add_tags(buffer, &relation_builder, tags);
        add_relation_members(buffer, &relation_builder, member_ids, member_types, member_roles);
        return relation_builder.object();
    }

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

}


#endif /* TEST_INCLUDE_OBJECT_BUILDER_UTILITIES_HPP_ */
