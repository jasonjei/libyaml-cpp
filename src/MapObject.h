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

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "yaml.h"

class MapObject {
public:
	MapObject();
	static MapObject processYaml(FILE* fh);
	static MapObject processYaml(const std::string& str);

	void SetLineWithInfiniteWidth(bool lineWithInfiniteWidth) { _lineWithInfiniteWidth = lineWithInfiniteWidth; }

	std::string exportYaml() const;
	std::string exportYamlWithUserOrder() const;
	std::string exportYamlWithUserOrderAndVersion(const std::string& version = "1.2") const;

	void exportYaml(const std::string& fileName) const;
	void exportYamlWithUserOrder(const std::string& fileName) const;
#ifdef WIN32
	void exportYaml(const std::wstring& fileName) const;
	void exportYamlWithUserOrder(const std::wstring& fileName) const;
#endif

	enum mapObjectType { MAP_OBJ_UNINIT, MAP_OBJ_VECTOR, MAP_OBJ_MAP, MAP_OBJ_VALUE, MAP_OBJ_FAILED };

	class mapMapObject;
	std::string value;
	std::vector<MapObject> mapObjects;
	std::shared_ptr<MapObject::mapMapObject> mapPtr;
	MapObject::mapObjectType _type;
	bool flow;
	yaml_scalar_style_t yamlScalarStyle = YAML_ANY_SCALAR_STYLE;

private:
	static void hardcoreYamlProcess(MapObject& yamlMap, yaml_parser_t* parser, yaml_event_t* event);
	static int write_handler(void* ctx, unsigned char* buffer, size_t size);
	static int yamlDocProlog(yaml_emitter_t* emitter, yaml_event_t* event);
	static int yamlDocEpilog(yaml_emitter_t* emitter, yaml_event_t* event);
	static int yamlSequence(yaml_emitter_t* emitter, yaml_event_t* event, const std::vector<MapObject>& mapObjects, int flow_style = 0);
	static int yamlSequenceWithUserOrder(yaml_emitter_t* emitter, yaml_event_t* event, const std::vector<MapObject>& mapObjects, int flow_style = 0);
	static int yamlMap(yaml_emitter_t* emitter, yaml_event_t* event, std::shared_ptr<MapObject::mapMapObject> mapObj);
	static int yamlMapWithUserOrder(yaml_emitter_t* emitter, yaml_event_t* event, std::shared_ptr<MapObject::mapMapObject> mapObj);

	bool _lineWithInfiniteWidth = true;

	static constexpr bool _debug =
#ifdef DEBUG
		true
#else
		false
#endif
		;
};

class MapObject::mapMapObject {
public:
	std::unordered_map<std::string, MapObject> map;
	std::vector<std::string> keys;
	bool nextIsKey;
	bool flow;
	mapMapObject() noexcept : nextIsKey(false), flow(false) {}
};

#endif
