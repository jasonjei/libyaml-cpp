#include "MapObject.h"

#include <algorithm>
#include <stack>
#include <sstream>

using namespace std;

MapObject::MapObject() : mapPtr(make_shared<mapMapObject>()), flow(false) {}

MapObject MapObject::processYaml(FILE* fh) {
	MapObject yamlMap;
	if (fh == NULL) {
		yamlMap._type = MapObject::MAP_OBJ_FAILED;
		if constexpr (_debug) fputs("Failed to open file!\n", stderr);
		return yamlMap;
	}

	yaml_parser_t parser;
	if (!yaml_parser_initialize(&parser)) {
		yamlMap._type = MapObject::MAP_OBJ_FAILED;
		if constexpr (_debug) fputs("Failed to initialize parser!\n", stderr);
		return yamlMap;
	}

	yaml_parser_set_input_file(&parser, fh);

	yaml_event_t event;
	hardcoreYamlProcess(yamlMap, &parser, &event);

	yaml_event_delete(&event);
	yaml_parser_delete(&parser);

	fclose(fh);

	return yamlMap;
}

MapObject MapObject::processYaml(const string& str) {
	MapObject yamlMap;

	yaml_parser_t parser;
	if (!yaml_parser_initialize(&parser)) {
		yamlMap._type = MapObject::MAP_OBJ_FAILED;
		if constexpr (_debug) fputs("Failed to initialize parser!\n", stderr);
		return yamlMap;
	}

	yaml_parser_set_input_string(&parser, reinterpret_cast<const unsigned char*>(str.c_str()), str.size());

	yaml_event_t event;
	hardcoreYamlProcess(yamlMap, &parser, &event);

	yaml_event_delete(&event);
	yaml_parser_delete(&parser);

	return yamlMap;
}

void MapObject::hardcoreYamlProcess(MapObject& yamlMap, yaml_parser_t* parser, yaml_event_t* event) {
	stack<MapObject*> stack;
	string lastKey;
	bool done = false;

	do {
		yaml_parser_parse(parser, event);

		switch (event->type) {
		case YAML_NO_EVENT:
			if constexpr (_debug) puts("No event!");
			done = true;
			break;
			/* Stream start/end */
		case YAML_STREAM_START_EVENT:
			if constexpr (_debug) puts("STREAM START");
			break;
		case YAML_STREAM_END_EVENT:
			if constexpr (_debug) puts("STREAM END");
			break;
			/* Block delimeters */
		case YAML_DOCUMENT_START_EVENT:
			if constexpr (_debug) puts("<b>Start Document</b>");
			stack.push(&yamlMap);
			break;
		case YAML_DOCUMENT_END_EVENT:
			if constexpr (_debug) puts("<b>End Document</b>");
			if (stack.top()->mapPtr->nextIsKey) {
				stack.top()->value = stack.top()->mapPtr->keys.front();
				stack.top()->mapPtr->keys.pop_back();
			}

			stack.pop();
			break;
		case YAML_SEQUENCE_START_EVENT:
			if constexpr (_debug) puts("<b>Start Sequence</b>");
			if (stack.top()->mapPtr->keys.size() && stack.top()->mapPtr->nextIsKey) {
				lastKey = stack.top()->mapPtr->keys.back();
				stack.top()->mapPtr->map[lastKey] = MapObject();
				stack.top()->mapPtr->nextIsKey = false;
				stack.push(&stack.top()->mapPtr->map[lastKey]);
			}
			else if (stack.top()->_type == MAP_OBJ_VECTOR) {
				stack.top()->mapObjects.push_back(MapObject());
				stack.push(&stack.top()->mapObjects.back());
			}
			else
				stack.push(stack.top());

			if (event->data.sequence_start.style == YAML_FLOW_SEQUENCE_STYLE) stack.top()->flow = true;

			stack.top()->_type = MAP_OBJ_VECTOR;
			break;
		case YAML_SEQUENCE_END_EVENT:
			if constexpr (_debug) puts("<b>End Sequence</b>");
			stack.pop();
			break;
		case YAML_MAPPING_START_EVENT:
			if constexpr (_debug) puts("<b>Start Mapping</b>");
			if (stack.top()->mapPtr->keys.size() && stack.top()->mapPtr->nextIsKey)
				stack.push(&stack.top()->mapPtr->map[stack.top()->mapPtr->keys.back()]);
			else if (stack.top()->_type == MAP_OBJ_VECTOR) {
				stack.top()->mapObjects.push_back(MapObject());
				stack.push(&stack.top()->mapObjects.back());
			}
			else {
				stack.push(stack.top());
			}

			if (event->data.mapping_start.style == YAML_FLOW_MAPPING_STYLE) stack.top()->mapPtr->flow = true;

			stack.top()->_type = MAP_OBJ_MAP;
			break;
		case YAML_MAPPING_END_EVENT:
			if constexpr (_debug) puts("<b>End Mapping</b>");
			if (stack.top()->mapPtr->keys.size()) {
				stack.top()->mapPtr->keys.erase(unique(stack.top()->mapPtr->keys.begin(), stack.top()->mapPtr->keys.end()), stack.top()->mapPtr->keys.end());
			}
			stack.pop();
			if (!stack.empty() && stack.top()->_type == MAP_OBJ_MAP) {
				stack.top()->mapPtr->nextIsKey = false;
			}
			break;
			/* Data */
		case YAML_ALIAS_EVENT:
			if constexpr (_debug) printf("Got alias (anchor %s)\n", event->data.alias.anchor);
			break;
		case YAML_SCALAR_EVENT:
			if constexpr (_debug) printf("Got scalar (value %s)\n", event->data.scalar.value);
			if (stack.top()->_type == MAP_OBJ_MAP && !stack.top()->mapPtr->nextIsKey) {
				stack.top()->mapPtr->keys.push_back(reinterpret_cast<char*>(event->data.scalar.value));
				stack.top()->mapPtr->nextIsKey = true;
			}
			else if (stack.top()->_type == MAP_OBJ_MAP && stack.top()->mapPtr->nextIsKey) {
				stack.top()->mapPtr->map[stack.top()->mapPtr->keys.back()] = MapObject();
				stack.top()->mapPtr->map[stack.top()->mapPtr->keys.back()].value = reinterpret_cast<char*>(event->data.scalar.value);
				stack.top()->mapPtr->nextIsKey = false;
			}
			else if (stack.top()->_type == MAP_OBJ_VECTOR) {
				stack.top()->mapObjects.push_back(MapObject());
				stack.top()->mapObjects.back().value = reinterpret_cast<char*>(event->data.scalar.value);
			}
			else {
				stack.top()->value = reinterpret_cast<char*>(event->data.scalar.value);
				stack.top()->_type = MAP_OBJ_VALUE;
			}
			break;
		default:
			break;
		}

		if (event->type == YAML_DOCUMENT_END_EVENT) done = true;

		yaml_event_delete(event);

	} while (!done);
	yaml_parser_delete(parser);
}

void MapObject::exportYaml(const string& fileName) const {
	yaml_emitter_t emitter;
	yaml_emitter_initialize(&emitter);

	if (_lineWithInfiniteWidth)
		yaml_emitter_set_width(&emitter, -1);

	FILE* const file = fopen(fileName.c_str(), "wb");
	yaml_emitter_set_output_file(&emitter, file);
	yaml_emitter_set_encoding(&emitter, YAML_UTF8_ENCODING);

	yaml_event_t event;
	if (!yamlDocProlog(&emitter, &event)) return;

	if (mapPtr->map.size())
		yamlMap(&emitter, &event, mapPtr);
	else if (mapObjects.size())
		yamlSequence(&emitter, &event, mapObjects, flow ? 1 : 0);
	else {
		yaml_scalar_event_initialize(&event, NULL, NULL, reinterpret_cast<yaml_char_t*>(const_cast<char*>(value.c_str())), static_cast<int>(value.length()), 1, 1, yamlScalarStyle);
		yaml_emitter_emit(&emitter, &event);
	}

	yamlDocEpilog(&emitter, &event);
	yaml_emitter_flush(&emitter);
	fclose(file);

	yaml_event_delete(&event);
	yaml_emitter_delete(&emitter);
}

void MapObject::exportYamlWithVersion(const string& fileName, const yaml_version_directive_t& version) const {
	yaml_emitter_t emitter;
	yaml_emitter_initialize(&emitter);

	if (_lineWithInfiniteWidth)
		yaml_emitter_set_width(&emitter, -1);

	FILE* const file = fopen(fileName.c_str(), "wb");
	yaml_emitter_set_output_file(&emitter, file);
	yaml_emitter_set_encoding(&emitter, YAML_UTF8_ENCODING);

	yaml_event_t event;
	if (!yamlDocProlog(&emitter, &event, version)) return;

	if (mapPtr->map.size())
		yamlMap(&emitter, &event, mapPtr);
	else if (mapObjects.size())
		yamlSequence(&emitter, &event, mapObjects, flow ? 1 : 0);
	else {
		yaml_scalar_event_initialize(&event, NULL, NULL, reinterpret_cast<yaml_char_t*>(const_cast<char*>(value.c_str())), static_cast<int>(value.length()), 1, 1, yamlScalarStyle);
		yaml_emitter_emit(&emitter, &event);
	}

	yamlDocEpilog(&emitter, &event);
	yaml_emitter_flush(&emitter);
	fclose(file);

	yaml_event_delete(&event);
	yaml_emitter_delete(&emitter);
}

void MapObject::exportYamlWithUserOrder(const string& fileName) const {
	yaml_emitter_t emitter;
	yaml_emitter_initialize(&emitter);

	if (_lineWithInfiniteWidth)
		yaml_emitter_set_width(&emitter, -1);

	FILE* const file = fopen(fileName.c_str(), "wb");
	yaml_emitter_set_output_file(&emitter, file);
	yaml_emitter_set_encoding(&emitter, YAML_UTF8_ENCODING);

	yaml_event_t event;
	if (!yamlDocProlog(&emitter, &event)) return;

	if (mapPtr->map.size())
		yamlMapWithUserOrder(&emitter, &event, mapPtr);
	else if (mapObjects.size())
		yamlSequenceWithUserOrder(&emitter, &event, mapObjects, flow ? 1 : 0);
	else {
		yaml_scalar_event_initialize(&event, NULL, NULL, reinterpret_cast<yaml_char_t*>(const_cast<char*>(value.c_str())), static_cast<int>(value.length()), 1, 1, yamlScalarStyle);
		yaml_emitter_emit(&emitter, &event);
	}

	yamlDocEpilog(&emitter, &event);
	yaml_emitter_flush(&emitter);
	fclose(file);

	yaml_event_delete(&event);
	yaml_emitter_delete(&emitter);
}

void MapObject::exportYamlWithUserOrderAndVersion(const string& fileName, const yaml_version_directive_t& version) const {
	yaml_emitter_t emitter;
	yaml_emitter_initialize(&emitter);

	if (_lineWithInfiniteWidth)
		yaml_emitter_set_width(&emitter, -1);

	FILE* const file = fopen(fileName.c_str(), "wb");
	yaml_emitter_set_output_file(&emitter, file);
	yaml_emitter_set_encoding(&emitter, YAML_UTF8_ENCODING);

	yaml_event_t event;
	if (!yamlDocProlog(&emitter, &event, version)) return;

	if (mapPtr->map.size())
		yamlMapWithUserOrder(&emitter, &event, mapPtr);
	else if (mapObjects.size())
		yamlSequenceWithUserOrder(&emitter, &event, mapObjects, flow ? 1 : 0);
	else {
		yaml_scalar_event_initialize(&event, NULL, NULL, reinterpret_cast<yaml_char_t*>(const_cast<char*>(value.c_str())), static_cast<int>(value.length()), 1, 1, yamlScalarStyle);
		yaml_emitter_emit(&emitter, &event);
	}

	yamlDocEpilog(&emitter, &event);
	yaml_emitter_flush(&emitter);
	fclose(file);

	yaml_event_delete(&event);
	yaml_emitter_delete(&emitter);
}

#ifdef WIN32
void MapObject::exportYaml(const wstring& fileName) const {
	yaml_emitter_t emitter;
	yaml_emitter_initialize(&emitter);

	if (_lineWithInfiniteWidth)
		yaml_emitter_set_width(&emitter, -1);

	FILE* const file;
	_wfopen_s(&file, fileName.c_str(), _T("wb"));

	yaml_emitter_set_output_file(&emitter, file);
	yaml_emitter_set_encoding(&emitter, YAML_UTF8_ENCODING);

	yaml_event_t event;
	if (!yamlDocProlog(&emitter, &event)) return;

	if (mapPtr->map.size())
		yamlMap(&emitter, &event, mapPtr);
	else if (mapObjects.size())
		yamlSequence(&emitter, &event, mapObjects, flow ? 1 : 0);
	else {
		yaml_scalar_event_initialize(&event, NULL, NULL, reinterpret_cast<yaml_char_t*>(const_cast<char*>(value.c_str())), static_cast<int>(value.length()), 1, 1, yamlScalarStyle);
		yaml_emitter_emit(&emitter, &event);
	}

	yamlDocEpilog(&emitter, &event);
	yaml_emitter_flush(&emitter);
	fclose(file);

	yaml_event_delete(&event);
	yaml_emitter_delete(&emitter);
}

void MapObject::exportYamlWithVersion(const wstring& fileName) const {
	yaml_emitter_t emitter;
	yaml_emitter_initialize(&emitter);

	if (_lineWithInfiniteWidth)
		yaml_emitter_set_width(&emitter, -1);

	FILE* const file;
	_wfopen_s(&file, fileName.c_str(), _T("wb"));

	yaml_emitter_set_output_file(&emitter, file);
	yaml_emitter_set_encoding(&emitter, YAML_UTF8_ENCODING);

	yaml_event_t event;
	if (!yamlDocProlog(&emitter, &event, version)) return;

	if (mapPtr->map.size())
		yamlMap(&emitter, &event, mapPtr);
	else if (mapObjects.size())
		yamlSequence(&emitter, &event, mapObjects, flow ? 1 : 0);
	else {
		yaml_scalar_event_initialize(&event, NULL, NULL, reinterpret_cast<yaml_char_t*>(const_cast<char*>(value.c_str())), static_cast<int>(value.length()), 1, 1, yamlScalarStyle);
		yaml_emitter_emit(&emitter, &event);
	}

	yamlDocEpilog(&emitter, &event);
	yaml_emitter_flush(&emitter);
	fclose(file);

	yaml_event_delete(&event);
	yaml_emitter_delete(&emitter);
}

void MapObject::exportYamlWithUserOrder(const wstring& fileName) const {
	yaml_emitter_t emitter;
	yaml_emitter_initialize(&emitter);

	if (_lineWithInfiniteWidth)
		yaml_emitter_set_width(&emitter, -1);

	FILE* const file;
	_wfopen_s(&file, fileName.c_str(), _T("wb"));

	yaml_emitter_set_output_file(&emitter, file);
	yaml_emitter_set_encoding(&emitter, YAML_UTF8_ENCODING);

	yaml_event_t event;
	if (!yamlDocProlog(&emitter, &event)) return;

	if (mapPtr->map.size())
		yamlMapWithUserOrder(&emitter, &event, mapPtr);
	else if (mapObjects.size())
		yamlSequenceWithUserOrder(&emitter, &event, mapObjects, flow ? 1 : 0);
	else {
		yaml_scalar_event_initialize(&event, NULL, NULL, reinterpret_cast<yaml_char_t*>(const_cast<char*>(value.c_str())), static_cast<int>(value.length()), 1, 1, yamlScalarStyle);
		yaml_emitter_emit(&emitter, &event);
	}

	yamlDocEpilog(&emitter, &event);
	yaml_emitter_flush(&emitter);
	fclose(file);

	yaml_event_delete(&event);
	yaml_emitter_delete(&emitter);
}

void MapObject::exportYamlWithUserOrderAndVersion(const wstring& fileName) const {
	yaml_emitter_t emitter;
	yaml_emitter_initialize(&emitter);

	if (_lineWithInfiniteWidth)
		yaml_emitter_set_width(&emitter, -1);

	FILE* const file;
	_wfopen_s(&file, fileName.c_str(), _T("wb"));

	yaml_emitter_set_output_file(&emitter, file);
	yaml_emitter_set_encoding(&emitter, YAML_UTF8_ENCODING);

	yaml_event_t event;
	if (!yamlDocProlog(&emitter, &event, version)) return;

	if (mapPtr->map.size())
		yamlMapWithUserOrder(&emitter, &event, mapPtr);
	else if (mapObjects.size())
		yamlSequenceWithUserOrder(&emitter, &event, mapObjects, flow ? 1 : 0);
	else {
		yaml_scalar_event_initialize(&event, NULL, NULL, reinterpret_cast<yaml_char_t*>(const_cast<char*>(value.c_str())), static_cast<int>(value.length()), 1, 1, yamlScalarStyle);
		yaml_emitter_emit(&emitter, &event);
	}

	yamlDocEpilog(&emitter, &event);
	yaml_emitter_flush(&emitter);
	fclose(file);

	yaml_event_delete(&event);
	yaml_emitter_delete(&emitter);
}
#endif

int MapObject::write_handler(void* ctx, unsigned char* buffer, size_t size) {
	reinterpret_cast<string*>(ctx)->append(reinterpret_cast<char*>(buffer), size);
	return 1;
}

string MapObject::exportYaml() const {
	yaml_emitter_t emitter;
	yaml_emitter_initialize(&emitter);

	if (_lineWithInfiniteWidth)
		yaml_emitter_set_width(&emitter, -1);

	string exportValue = "";
	yaml_emitter_set_output(&emitter, write_handler, &exportValue);
	yaml_emitter_set_encoding(&emitter, YAML_UTF8_ENCODING);

	yaml_event_t event;
	if (!yamlDocProlog(&emitter, &event)) return "";

	if (mapPtr->map.size())
		yamlMap(&emitter, &event, mapPtr);
	else if (mapObjects.size())
		yamlSequence(&emitter, &event, mapObjects);
	else {
		yaml_scalar_event_initialize(&event, NULL, NULL, reinterpret_cast<yaml_char_t*>(const_cast<char*>(value.c_str())), static_cast<int>(value.length()), 1, 1, yamlScalarStyle);
		yaml_emitter_emit(&emitter, &event);
	}

	yamlDocEpilog(&emitter, &event);

	yaml_event_delete(&event);
	yaml_emitter_delete(&emitter);

	return exportValue;
}

string MapObject::exportYamlWithVersion(const yaml_version_directive_t& version) const {
	yaml_emitter_t emitter;
	yaml_emitter_initialize(&emitter);

	if (_lineWithInfiniteWidth)
		yaml_emitter_set_width(&emitter, -1);

	string exportValue = "";
	yaml_emitter_set_output(&emitter, write_handler, &exportValue);
	yaml_emitter_set_encoding(&emitter, YAML_UTF8_ENCODING);

	yaml_event_t event;
	if (!yamlDocProlog(&emitter, &event, version)) return "";

	if (mapPtr->map.size())
		yamlMap(&emitter, &event, mapPtr);
	else if (mapObjects.size())
		yamlSequence(&emitter, &event, mapObjects);
	else {
		yaml_scalar_event_initialize(&event, NULL, NULL, reinterpret_cast<yaml_char_t*>(const_cast<char*>(value.c_str())), static_cast<int>(value.length()), 1, 1, yamlScalarStyle);
		yaml_emitter_emit(&emitter, &event);
	}

	yamlDocEpilog(&emitter, &event);

	yaml_event_delete(&event);
	yaml_emitter_delete(&emitter);

	return exportValue;
}

string MapObject::exportYamlWithUserOrder() const {
	yaml_emitter_t emitter;
	yaml_emitter_initialize(&emitter);

	if (_lineWithInfiniteWidth)
		yaml_emitter_set_width(&emitter, -1);

	string exportValue = "";
	yaml_emitter_set_output(&emitter, write_handler, &exportValue);
	yaml_emitter_set_encoding(&emitter, YAML_UTF8_ENCODING);

	yaml_event_t event;
	if (!yamlDocProlog(&emitter, &event)) return "";

	if (mapPtr->map.size())
		yamlMapWithUserOrder(&emitter, &event, mapPtr);
	else if (mapObjects.size())
		yamlSequenceWithUserOrder(&emitter, &event, mapObjects);
	else {
		yaml_scalar_event_initialize(&event, NULL, NULL, reinterpret_cast<yaml_char_t*>(const_cast<char*>(value.c_str())), static_cast<int>(value.length()), 1, 1, yamlScalarStyle);
		yaml_emitter_emit(&emitter, &event);
	}

	yamlDocEpilog(&emitter, &event);

	yaml_event_delete(&event);
	yaml_emitter_delete(&emitter);

	return exportValue;
}

string MapObject::exportYamlWithUserOrderAndVersion(const yaml_version_directive_t& version) const {
	yaml_emitter_t emitter;
	yaml_emitter_initialize(&emitter);

	if (_lineWithInfiniteWidth)
		yaml_emitter_set_width(&emitter, -1);

	string exportValue = "";
	yaml_emitter_set_output(&emitter, write_handler, &exportValue);
	yaml_emitter_set_encoding(&emitter, YAML_UTF8_ENCODING);

	yaml_event_t event;
	if (!yamlDocProlog(&emitter, &event, version)) return "";

	if (mapPtr->map.size())
		yamlMapWithUserOrder(&emitter, &event, mapPtr);
	else if (mapObjects.size())
		yamlSequenceWithUserOrder(&emitter, &event, mapObjects);
	else {
		yaml_scalar_event_initialize(&event, NULL, NULL, reinterpret_cast<yaml_char_t*>(const_cast<char*>(value.c_str())), static_cast<int>(value.length()), 1, 1, yamlScalarStyle);
		yaml_emitter_emit(&emitter, &event);
	}

	yamlDocEpilog(&emitter, &event);

	yaml_event_delete(&event);
	yaml_emitter_delete(&emitter);

	return exportValue;
}

int MapObject::yamlMap(yaml_emitter_t* emitter, yaml_event_t* event, shared_ptr<MapObject::mapMapObject> mapObj) {
	if (mapObj->map.size()) {
		yaml_mapping_start_event_initialize(event, NULL, NULL, true, mapObj->flow ? YAML_FLOW_MAPPING_STYLE : YAML_BLOCK_MAPPING_STYLE);
		yaml_emitter_emit(emitter, event);

		for (const auto& [key, mapObject] : mapObj->map) {
			yaml_scalar_event_initialize(event, NULL, NULL, reinterpret_cast<yaml_char_t*>(const_cast<char*>(key.c_str())), static_cast<int>(key.length()), 1, 1, YAML_ANY_SCALAR_STYLE);
			yaml_emitter_emit(emitter, event);

			if (mapObject.mapPtr->map.size())
				yamlMap(emitter, event, mapObject.mapPtr);
			else if (mapObject.mapObjects.size())
				yamlSequence(emitter, event, mapObject.mapObjects, mapObject.flow ? 1 : 0);
			else {
				yaml_scalar_event_initialize(event, NULL, NULL, reinterpret_cast<yaml_char_t*>(const_cast<char*>(mapObject.value.c_str())), static_cast<int>(mapObject.value.length()), 1, 1, mapObject.yamlScalarStyle);
				yaml_emitter_emit(emitter, event);
			}
		}

		yaml_mapping_end_event_initialize(event);
		yaml_emitter_emit(emitter, event);
	}
	return 1;
}

int MapObject::yamlMapWithUserOrder(yaml_emitter_t* emitter, yaml_event_t* event, shared_ptr<MapObject::mapMapObject> mapObj) {
	if (mapObj->map.size()) {
		yaml_mapping_start_event_initialize(event, NULL, NULL, true, mapObj->flow ? YAML_FLOW_MAPPING_STYLE : YAML_BLOCK_MAPPING_STYLE);
		yaml_emitter_emit(emitter, event);

		for (const string& key : mapObj->keys) {
			const MapObject& mapObject = mapObj->map.at(key);
			yaml_scalar_event_initialize(event, NULL, NULL, reinterpret_cast<yaml_char_t*>(const_cast<char*>(key.c_str())), static_cast<int>(key.length()), 1, 1, YAML_ANY_SCALAR_STYLE);
			yaml_emitter_emit(emitter, event);

			if (mapObject.mapPtr->map.size())
				yamlMapWithUserOrder(emitter, event, mapObject.mapPtr);
			else if (mapObject.mapObjects.size())
				yamlSequenceWithUserOrder(emitter, event, mapObject.mapObjects, mapObject.flow ? 1 : 0);
			else {
				yaml_scalar_event_initialize(event, NULL, NULL, reinterpret_cast<yaml_char_t*>(const_cast<char*>(mapObject.value.c_str())), static_cast<int>(mapObject.value.length()), 1, 1, mapObject.yamlScalarStyle);
				yaml_emitter_emit(emitter, event);
			}
		}

		yaml_mapping_end_event_initialize(event);
		yaml_emitter_emit(emitter, event);
	}
	return 1;
}

int MapObject::yamlSequence(yaml_emitter_t* emitter, yaml_event_t* event, const vector<MapObject>& mapObjects, int flow_style) {
	if (mapObjects.size()) {
		yaml_sequence_start_event_initialize(event, NULL, NULL, true, !flow_style ? YAML_BLOCK_SEQUENCE_STYLE : YAML_FLOW_SEQUENCE_STYLE);
		yaml_emitter_emit(emitter, event);

		for (const MapObject& mapObject : mapObjects) {
			if (mapObject.mapPtr->map.size())
				yamlMap(emitter, event, mapObject.mapPtr);
			else if (mapObject.mapObjects.size())
				yamlSequence(emitter, event, mapObject.mapObjects, mapObject.flow ? 1 : 0);
			else if (!mapObject.value.empty()) {
				yaml_scalar_event_initialize(event, NULL, NULL, reinterpret_cast<yaml_char_t*>(const_cast<char*>(mapObject.value.c_str())), static_cast<int>(mapObject.value.length()), 1, 1, mapObject.yamlScalarStyle);
				yaml_emitter_emit(emitter, event);
			}
		}

		yaml_sequence_end_event_initialize(event);
		yaml_emitter_emit(emitter, event);
	}
	return 1;
}

int MapObject::yamlSequenceWithUserOrder(yaml_emitter_t* emitter, yaml_event_t* event, const vector<MapObject>& mapObjects,
	int flow_style) {
	if (mapObjects.size()) {
		yaml_sequence_start_event_initialize(event, NULL, NULL, true, !flow_style ? YAML_BLOCK_SEQUENCE_STYLE : YAML_FLOW_SEQUENCE_STYLE);
		yaml_emitter_emit(emitter, event);

		for (const MapObject& mapObject : mapObjects) {
			if (mapObject.mapPtr->map.size())
				yamlMapWithUserOrder(emitter, event, mapObject.mapPtr);
			else if (mapObject.mapObjects.size())
				yamlSequenceWithUserOrder(emitter, event, mapObject.mapObjects, mapObject.flow ? 1 : 0);
			else if (!mapObject.value.empty()) {
				yaml_scalar_event_initialize(event, NULL, NULL, reinterpret_cast<yaml_char_t*>(const_cast<char*>(mapObject.value.c_str())), static_cast<int>(mapObject.value.length()), 1, 1, mapObject.yamlScalarStyle);
				yaml_emitter_emit(emitter, event);
			}
		}

		yaml_sequence_end_event_initialize(event);
		yaml_emitter_emit(emitter, event);
	}
	return 1;
}

int MapObject::yamlDocProlog(yaml_emitter_t* emitter, yaml_event_t* event, const optional<yaml_version_directive_t>& version) {
	yaml_stream_start_event_initialize(event, YAML_UTF8_ENCODING);

	if (!yaml_emitter_emit(emitter, event)) {
		yaml_emitter_delete(emitter);
		return 0;
	}

	if (version.has_value()) {
		yaml_version_directive_t versionCopy = *version;
		yaml_document_start_event_initialize(event, &versionCopy, NULL, NULL, true);
	}
	else {
		yaml_document_start_event_initialize(event, NULL, NULL, NULL, true);
	}

	yaml_emitter_emit(emitter, event);

	return 1;
}

int MapObject::yamlDocEpilog(yaml_emitter_t* emitter, yaml_event_t* event) {
	yaml_document_end_event_initialize(event, true);
	yaml_emitter_emit(emitter, event);

	yaml_stream_end_event_initialize(event);
	yaml_emitter_emit(emitter, event);

	return 1;
}
