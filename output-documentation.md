# Contents of the SQlite file

This file documents the content of the produced output file(s).

## Stops

This layer contains all stop positions (`public_transport=stop_position`) and has following columns:

* `node_id`: ID of the node
* `lastchange`: last modified timestamp of the object
* `name`: value of `name=*`
* `public_transport`: always `stop_position`
* `railway`: value of `railway=*`
* `highway`: value of `highway=*`
* `ref`: value of `ref=*`
* `local_ref`: value of `local_ref=*`
* `operator`: value of `operator=*`
* `network`: value of `network=*`

## Platforms

There are two platforms layers. One contains nodes, the other one contains ways.

These layers contain platforms tagged with either `public_transport=platform` or
`railway=platform`.

Both layers have following columns:

* `lastchange`: last modified timestamp of the object
* `name`: value of `name=*`
* `public_transport`: value of `public_transport`
* `railway`: value of `railway=*`
* `highway`: value of `highway=*`
* `ref`: value of `ref=*`
* `local_ref`: value of `local_ref=*`
* `operator`: value of `operator=*`
* `network`: value of `network=*`

Only nodes layer:

* `node_id`: ID of the node

Only ways layer:

* `way_id`: ID of the way

## Stations

There are two stations layers. One contains nodes, the other one contains ways.

These layers contain nodes and ways which are tagged with `public_transport=station`,
`railway=station`, `railway=halt` or `amenity=bus_station`.

Both layers have following columns:

* `lastchange`: last modified timestamp of the object
* `name`: value of `name=*`
* `public_transport`: value of `public_transport`
* `railway`: value of `railway=*`
* `highway`: value of `highway=*`
* `amenity`: value of `amenity=*`
* `operator`: value of `operator=*`
* `network`: value of `network=*`

Only nodes layer:

* `node_id`: ID of the node

Only ways layer:

* `way_id`: ID of the way


## Stops_Only_Highway

This layer contains bus stops (`highway=bus_stop`) which do *not* have any `public_transport=*`
tag. It has following columns:

* `node_id`: ID of the node
* `lastchange`: last modified timestamp of the object
* `name`: value of `name=*`
* `public_transport`: value of `public_transport`
* `railway`: value of `railway=*`
* `highway`: value of `highway=*`
* `operator`: value of `operator=*`
* `network`: value of `network=*`


## On_Track

This layer contains nodes which should be referenced by a way (i.e. "be located on the
way and not next to it). Following objects are always contained in this layer:

* `public_transport=stop_position`

Following objects are contained if `--no-railway-details` was *not* set:

* `railway=signal`
* `railway=stop`
* `railway=buffer_stop`
* `railway=level_crossing`
* `railway=milestone`
* `railway=isolated_track_section`
* `railway=switch`
* `railway=railway_crossing`

The layer has following fields:

* `node_id`: ID of the node
* `lastchange`: last modified timestamp of the object
* `type`: value of either `railway=*` or `public_transport=*` OSM key depending on which rule it maches (see above)
* `error`: description of the error, currently always `not on a way`

## Points

This layer contains points (`railway=switch`) and their details. Following fields are available:

* `node_id`: ID of the node
* `lastchange`: last modified timestamp of the object
* `type`: type of switch (mainly values of `railway=*` key): `default`, `double_slip`, `single_slip` or `single_slip_incomplete`
* `ref`: reference number (value of `ref=*`)

Points with `type` set to `single_slip_incomplete` are single slip points which are not a via node of any turn restriction.


## Crossings

This layer contains correctly tagged crossings of railway features and highway features.
"Correctly" means that they are either tagged with `railway=level_crossing` or `railway=crossing`.

Following fields are available:

* `node_id`: ID of the node
* `lastchange`: last modified timestamp of the object
* `barrier`: mainly values of `crossing:barrier=*`: `no`, `yes`, `half`, `double_half`, `full`, `gates`. If `crossing:barrier=*` has a value but that value is not well-formed (i.e. one of the other ones), `barrier` will be set to `UNKNOWN`. If the tag is not set, `type will be set to `NONE`.
* `lights`: mainly values of `crossing:lights=*`: `no`, `yes`. If `crossing:lights=*` has a value but that value is not well-formed (i.e. one of the other ones), `lights` will be set to `UNKNOWN`. If the tag is not set, `type will be set to `NONE`.


## PTv2 Routes Valid

This layer contains route relations which are tagged with
`type=route` + `public_transport:version=2` and pass all tests.
Routes with non-fatal errors are contained in this layer.

* `rel_id`
* `from`
* `to`
* `via`
* `ref`
* `name`
* `route`
* `operator`

The multilinestring geometry only contains the way members which are used by the vehicle, no stops and no platforms.
The parts of the multilinestring are ordered (i.e. first segment, second segment, third segment, …) but not
directed.

## PTv2 Routes Invalid

This layer contains route relations which are tagged with
`type=route` + `public_transport:version=2` and fail one or many fatal validity tests.
Routes with non-fatal errors are not contained in this layer.

* `rel_id`
* `from`
* `to`
* `via`
* `ref`
* `name`
* `route`
* `operator`

Following fields are also available and either contain `T` for true or `F` for false:

* `error_over_non_rail`
* `error_over_rail`
* `error_unordered_gap`
* `error_wrong_structure`
* `no_stops_pltf_at_begin`
* `stoppltf_after_route`
* `non_way_empty_role`
* `stop_not_on_way`
* `no_way_members`
* `unknown_role`
* `unknown_route_type`
* `stop_is_not_node`

The multilinestring geometry contains all members of the relation which are ways including
platforms which are ways. There is no garantueed order of the parts of the multilinestring.

## PTv2 Error Lines

This layer contains route member ways which cause a route relation to fail a test or ways
which are located next to an error (e.g. gaps). This layer has following fields:

* `rel_id`: relation ID
* `way_id`: way ID
* `node_id`: ID of the node which is located next to the error or 0
* `error`: description of the error
 * `platform without proper tags`
 * `rail-guided over non-rail`
 * `road vehicle over non-road`
 * `route has only stops/platforms`
 * `stop/platform after route`
 * `roundabout after roundabout`
 * `unknown role 'ROLE'` where `ROLE` is the role of the member causing the error
 * `gap at this location`
 * `gap`
 * `stop is not a node`
 * `route has only stops/platforms`
* `from`
* `to`
* `via`
* `ref`
* `name`
* `route`


## PTv2 Error Points

This layer contains route member nodes which cause a route relation to fail a test or ways
which are located next to an error (e.g. gaps).

* `rel_id`: relation ID
* `way_id`: way ID, usually 0
* `node_id`: ID of the node which is located next to the error or causing it
* `error`: description of the error
 * `platform without proper tags`
 * `rail-guided over non-rail`
 * `road vehicle over non-road`
 * `route has only stops/platforms`
 * `stop/platform after route`
 * `roundabout after roundabout`
 * `unknown role 'ROLE'` where `ROLE` is the role of the member causing the error
 * `gap at this location`
 * `gap`
 * `stop is not a node`
 * `route has only stops/platforms`
* `from`
* `to`
* `via`
* `ref`
* `name`
* `route`
