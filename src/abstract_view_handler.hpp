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

#ifndef SRC_ABSTRACT_VIEW_HANDLER_HPP_
#define SRC_ABSTRACT_VIEW_HANDLER_HPP_

#include <gdalcpp.hpp>
#include <osmium/handler.hpp>
#include <osmium/osm/way.hpp>
#include "ogr_output_base.hpp"

class AbstractViewHandler : public osmium::handler::Handler, public OGROutputBase {
protected:
    /// ORG dataset
    gdalcpp::Dataset& m_dataset;

    /**
     * Check if all nodes of the way are valid.
     */
    bool all_nodes_valid(const osmium::WayNodeList& wnl);

public:
    AbstractViewHandler() = delete;

    AbstractViewHandler(gdalcpp::Dataset& dataset, Options& options, osmium::util::VerboseOutput& verbose_output);

    virtual ~AbstractViewHandler();

    /**
     * Get a pointer to the dataset being used. The ownership will stay at AbstractViewHandler.
     */
    gdalcpp::Dataset* get_dataset_pointer();

    /**
     * Build a string containing tags (length of key and value below 48 characters)
     * to be inserted into a "tag" column. The returned string is shorter than
     * MAX_FIELD_LENGTH characters. No keys or values will be truncated.
     *
     * \param tags TagList of the OSM object
     * \param not_include key whose value should not be included in the string of all tags.
     * If it is a null pointer, this check is skipped.
     *
     * \returns string with the tags
     */
    std::string tags_string(const osmium::TagList& tags, const char* not_include);
};



#endif /* SRC_ABSTRACT_VIEW_HANDLER_HPP_ */
