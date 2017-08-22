/*
 *  © 2017 Geofabrik GmbH
 *
 *  This file is part of osmi_simple_views.
 *
 *  osmi_simple_views is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  osmi_simple_views is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with osmi_simple_views. If not, see <http://www.gnu.org/licenses/>.
 */

#include "abstract_view_handler.hpp"

AbstractViewHandler::AbstractViewHandler(gdalcpp::Dataset& dataset, std::string& output_format, osmium::util::VerboseOutput& verbose_output, int epsg) :
        OGROutputBase(verbose_output, output_format, epsg),
        m_dataset(dataset) {
    m_dataset.enable_auto_transactions(10000);
}

AbstractViewHandler::~AbstractViewHandler() {}

bool AbstractViewHandler::all_nodes_valid(const osmium::WayNodeList& wnl) {
    for (const osmium::NodeRef& nd_ref : wnl) {
        if (!nd_ref.location().valid()) {
            m_verbose_output << "Invalid location for node " << nd_ref.ref() << "\n";
            return false;
        }
    }
    return true;
}

gdalcpp::Dataset* AbstractViewHandler::get_dataset_pointer() {
    return &m_dataset;
}

std::string AbstractViewHandler::tags_string(const osmium::TagList& tags, const char* not_include) {
    std::string tag_str;
    // only add tags to the tags string if their key and value are shorter than 50 characters
    for (const osmium::Tag& t : tags) {
        if (not_include && !strcmp(t.key(), not_include)) {
            continue;
        }
        size_t add_length = strlen(t.key()) + strlen(t.value()) + 2;
        if (add_length < 50 && tag_str.length() + add_length < MAX_FIELD_LENGTH) {
            tag_str += t.key();
            tag_str += '=';
            tag_str += t.value();
            tag_str += '|';
        }
    }
    // remove last | from tag_str
    tag_str.pop_back();
    return tag_str;
}
