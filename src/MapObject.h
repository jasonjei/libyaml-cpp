//
//  MapObject.h
//
//  Parses FH and creates a hashmap of all entries in YAML
//  into a map, vector, or string value.
//
//  Created by Levion on 1/24/13.
//  Copyright (c) 2013 Levion. All rights reserved.
//
 
#pragma once

#ifndef _MAPOBJECT_LEVION_H_
#define _MAPOBJECT_LEVION_H_


#include <algorithm>
#include <vector>
#include <string>
#include <unordered_map>
#include "yaml.h"
#include <stack>
#include <memory>
#include <stdlib.h>
 
class MapObject {
public:
	enum mapObjectType {
		MAP_OBJ_UNINIT, MAP_OBJ_VECTOR, MAP_OBJ_MAP, MAP_OBJ_VALUE, MAP_OBJ_FAILED
	};
 
	class mapMapObject;
	std::string value;
	std::vector<MapObject> mapObjects;
	std::shared_ptr<MapObject::mapMapObject> mapPtr;
	MapObject::mapObjectType _type;
	bool flow;
 
	MapObject();
	static MapObject processYaml(FILE* fh);
	static MapObject processYaml(std::string str);
 
	static void hardcoreYamlProcess(MapObject* yamlMap, yaml_parser_t *parser, yaml_event_t* event);
	void exportYaml(std::string fileName);
#ifdef WIN32
	void exportYaml(std::wstring fileName);
#endif
	static int write_handler(void *ctx, unsigned char *buffer, size_t size);
	int yamlDocProlog(yaml_emitter_t *emitter, yaml_event_t *event);
	int yamlDocEpilog(yaml_emitter_t *emitter, yaml_event_t *event);
	int yamlSequence(yaml_emitter_t *emitter, yaml_event_t *event, std::vector<MapObject>* mapObjects, int flow_style = 0);
	int yamlMap(yaml_emitter_t *emitter, yaml_event_t *event, std::shared_ptr<MapObject::mapMapObject> mapObj);
	std::string exportYaml();
};

class MapObject::mapMapObject {
public:
	std::unordered_map<std::string, MapObject> map;
	std::vector<std::string> keys;
	bool nextIsKey;
	bool flow;
	mapMapObject() : nextIsKey(false), flow(false) {
 
	}
};

#endif

