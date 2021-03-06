#include "MapObject.h"


MapObject::MapObject() : mapPtr(std::make_shared<mapMapObject>()), flow(false) {
}
 
MapObject MapObject::processYaml(FILE* fh) {
	int debug = 0;
 
#ifdef DEBUG
	debug = 0;
#endif
 
	MapObject yamlMap;
 
	if (fh == NULL) {
		yamlMap._type = MapObject::MAP_OBJ_FAILED;
		if (debug)
			fputs("Failed to open file!\n", stderr);
		return yamlMap;
	}
 
	yaml_parser_t parser;
	yaml_event_t  event;
 
	if (! yaml_parser_initialize(&parser)) {
		yamlMap._type = MapObject::MAP_OBJ_FAILED;
		if (debug)
			fputs("Failed to initialize parser!\n", stderr);
		return yamlMap;
	}
 
	yaml_parser_set_input_file(&parser, fh);
 
	hardcoreYamlProcess(&yamlMap, &parser, &event);
 
	yaml_event_delete(&event);
	yaml_parser_delete(&parser);
 
	fclose(fh);
 
	return yamlMap;
}
 
MapObject MapObject::processYaml(std::string str) {
	int debug = 0;
 
#ifdef DEBUG
	debug = 1;
#endif
 
	MapObject yamlMap;
 
	yaml_parser_t parser;
	yaml_event_t  event;
 
	if (! yaml_parser_initialize(&parser)) {
		yamlMap._type = MapObject::MAP_OBJ_FAILED;
		if (debug)
			fputs("Failed to initialize parser!\n", stderr);
		return yamlMap;
	}
 
	yaml_parser_set_input_string(&parser, (const unsigned char *) str.c_str(), str.size());
 
	hardcoreYamlProcess(&yamlMap, &parser, &event);
 
	yaml_event_delete(&event);
	yaml_parser_delete(&parser);
 
	return yamlMap;
}
 
void MapObject::hardcoreYamlProcess(MapObject* yamlMap, yaml_parser_t *parser, yaml_event_t *event) {
	int debug = 0;
 
#ifdef DEBUG
	debug = 0;
#endif
 
	std::stack<MapObject*> stack;
	std::string lastKey;
	int done = 0;
 
	do {
		yaml_parser_parse(parser, event);
		MapObject newMapObj;
 
		switch (event->type) {
		case YAML_NO_EVENT:
			if (debug)
				puts("No event!");
			done = 1;
			break;
			/* Stream start/end */
		case YAML_STREAM_START_EVENT:
			if (debug)
				puts("STREAM START");
			break;
		case YAML_STREAM_END_EVENT:
			if (debug)
				puts("STREAM END");
			break;
			/* Block delimeters */
		case YAML_DOCUMENT_START_EVENT:
			if (debug)
				puts("<b>Start Document</b>");
			stack.push(yamlMap);
			break;
		case YAML_DOCUMENT_END_EVENT:
			if (debug)
				puts("<b>End Document</b>");
			if (stack.top()->mapPtr->nextIsKey) {
				stack.top()->value = stack.top()->mapPtr->keys.front();
				stack.top()->mapPtr->keys.pop_back();
			}
 
			stack.pop();
			break;
		case YAML_SEQUENCE_START_EVENT:
			if (debug)
				puts("<b>Start Sequence</b>");
			if (stack.top()->mapPtr->keys.size() && stack.top()->mapPtr->nextIsKey) {
				lastKey = stack.top()->mapPtr->keys.back();
				stack.top()->mapPtr->map[lastKey] = newMapObj;
				stack.top()->mapPtr->nextIsKey = false;
				stack.push(&stack.top()->mapPtr->map[lastKey]);
			}
			else if (stack.top()->_type == MAP_OBJ_VECTOR) {
				stack.top()->mapObjects.push_back(newMapObj);
				stack.push(&stack.top()->mapObjects.back());
			}
			else
				stack.push(stack.top());
 
			if (event->data.sequence_start.style == YAML_FLOW_SEQUENCE_STYLE)
				stack.top()->flow = true;
 
			stack.top()->_type = MAP_OBJ_VECTOR;
			break;
		case YAML_SEQUENCE_END_EVENT:
			if (debug)
				puts("<b>End Sequence</b>");
			stack.pop();
			break;
		case YAML_MAPPING_START_EVENT:
			if (debug)
				puts("<b>Start Mapping</b>");
			if (stack.top()->mapPtr->keys.size() && stack.top()->mapPtr->nextIsKey)
				stack.push(&stack.top()->mapPtr->map[stack.top()->mapPtr->keys.back()]);
			else if (stack.top()->_type == MAP_OBJ_VECTOR) {
				stack.top()->mapObjects.push_back(newMapObj);
				stack.push(&stack.top()->mapObjects.back());
			}
			else {
				stack.push(stack.top());
			}
 
			if (event->data.mapping_start.style == YAML_FLOW_MAPPING_STYLE)
				stack.top()->mapPtr->flow = true;
 
			stack.top()->_type = MAP_OBJ_MAP;
			break;
		case YAML_MAPPING_END_EVENT:
			if (debug)
				puts("<b>End Mapping</b>");
			if (stack.top()->mapPtr->keys.size()) {
				stack.top()->mapPtr->keys.erase(std::unique(stack.top()->mapPtr->keys.begin(), stack.top()->mapPtr->keys.end()), stack.top()->mapPtr->keys.end());
			}
			stack.pop();
			if (! stack.empty() && stack.top()->_type == MAP_OBJ_MAP) {
				stack.top()->mapPtr->nextIsKey = false;
			}
			break;
			/* Data */
		case YAML_ALIAS_EVENT:
			if (debug)
				printf("Got alias (anchor %s)\n", event->data.alias.anchor);
			break;
		case YAML_SCALAR_EVENT:
			if (debug)
				printf("Got scalar (value %s)\n", event->data.scalar.value);
			if (stack.top()->_type == MAP_OBJ_MAP && ! stack.top()->mapPtr->nextIsKey) {
				stack.top()->mapPtr->keys.push_back(((char*) event->data.scalar.value));
				stack.top()->mapPtr->nextIsKey = true;
			}
			else if (stack.top()->_type == MAP_OBJ_MAP && stack.top()->mapPtr->nextIsKey) {
				stack.top()->mapPtr->map[stack.top()->mapPtr->keys.back()] = newMapObj;
				stack.top()->mapPtr->map[stack.top()->mapPtr->keys.back()].value = ((char*) event->data.scalar.value);
				stack.top()->mapPtr->nextIsKey = false;
			}
			else if (stack.top()->_type == MAP_OBJ_VECTOR) {
				stack.top()->mapObjects.push_back(newMapObj);
				stack.top()->mapObjects.back().value = ((char*) event->data.scalar.value);
			}
			else {
				stack.top()->value = ((char*) event->data.scalar.value);
				stack.top()->_type = MAP_OBJ_VALUE;
			}
			break;
		}
 
		if (event->type == YAML_DOCUMENT_END_EVENT)
			done = 1;
 
		yaml_event_delete(event);
 
	} while (! done);
	yaml_parser_delete(parser);
}
 
void MapObject::exportYaml(std::string fileName) {
	yaml_emitter_t emitter;
	yaml_event_t event;
 
	FILE *file;
	file = fopen(fileName.c_str(), "wb");
 
	yaml_emitter_initialize(&emitter);
	yaml_emitter_set_output_file(&emitter, file);
	yaml_emitter_set_encoding(&emitter, YAML_UTF8_ENCODING);
 
	if (! yamlDocProlog(&emitter, &event))
		return;
 
	if (this->mapPtr->map.size()) yamlMap(&emitter, &event, this->mapPtr);
	else if (this->mapObjects.size()) yamlSequence(&emitter, &event, &this->mapObjects, (this->flow == true ? 1 : 0));
	else {
		yaml_scalar_event_initialize(&event, NULL, NULL, (yaml_char_t*) (this->value.c_str()), (int) this->value.length(), 1, 1, YAML_ANY_SCALAR_STYLE);
		yaml_emitter_emit(&emitter, &event);
	}
 
	yamlDocEpilog(&emitter, &event);
	yaml_emitter_flush(&emitter);
	fclose(file);
 
	yaml_event_delete(&event);
	yaml_emitter_delete(&emitter);
}

#ifdef WIN32
void MapObject::exportYaml(std::wstring fileName) {
	yaml_emitter_t emitter;
	yaml_event_t event;
 
	FILE *file;
	_wfopen_s(&file, fileName.c_str(), _T("wb"));
 
	yaml_emitter_initialize(&emitter);
	yaml_emitter_set_output_file(&emitter, file);
	yaml_emitter_set_encoding(&emitter, YAML_UTF8_ENCODING);
 
	if (! yamlDocProlog(&emitter, &event))
		return;
 
	if (this->mapPtr->map.size()) yamlMap(&emitter, &event, this->mapPtr);
	else if (this->mapObjects.size()) yamlSequence(&emitter, &event, &this->mapObjects, (this->flow == true ? 1 : 0));
	else {
		yaml_scalar_event_initialize(&event, NULL, NULL, (yaml_char_t*) (this->value.c_str()), (int) this->value.length(), 1, 1, YAML_ANY_SCALAR_STYLE);
		yaml_emitter_emit(&emitter, &event);
	}
 
	yamlDocEpilog(&emitter, &event);
	yaml_emitter_flush(&emitter);
	fclose(file);
 
	yaml_event_delete(&event);
	yaml_emitter_delete(&emitter);
}
#endif
 
int MapObject::write_handler(void *ctx, unsigned char *buffer, size_t size) {
	std::string* exportValue = (std::string*) ctx;
	char* signedChar = reinterpret_cast<char*>(buffer);
 
	exportValue->append((signedChar), size);
	return 1;
}
 
std::string MapObject::exportYaml() {
	yaml_emitter_t emitter;
	yaml_event_t event;
 
	std::string exportValue = "";
 
	yaml_emitter_initialize(&emitter);
	yaml_emitter_set_output(&emitter, write_handler, &exportValue);
	yaml_emitter_set_encoding(&emitter, YAML_UTF8_ENCODING);
 
	if (! yamlDocProlog(&emitter, &event))
		return "";
 
	if (this->mapPtr->map.size()) yamlMap(&emitter, &event, this->mapPtr);
	else if (this->mapObjects.size()) yamlSequence(&emitter, &event, &this->mapObjects);
	else {
		yaml_scalar_event_initialize(&event, NULL, NULL, (yaml_char_t*) (this->value.c_str()), (int) this->value.length(), 1, 1, YAML_ANY_SCALAR_STYLE);
		yaml_emitter_emit(&emitter, &event);
	}
 
	yamlDocEpilog(&emitter, &event);
 
	yaml_event_delete(&event);
	yaml_emitter_delete(&emitter);
 
	return exportValue;
}
 
int MapObject::yamlMap(yaml_emitter_t *emitter, yaml_event_t *event, std::shared_ptr<MapObject::mapMapObject> mapObj) {
	if (mapObj->map.size()) {
		yaml_mapping_start_event_initialize(event, NULL, NULL, true, (mapObj->flow == true ? YAML_FLOW_MAPPING_STYLE : YAML_BLOCK_MAPPING_STYLE));
		yaml_emitter_emit(emitter, event);
 
		for (std::unordered_map<std::string, MapObject>::iterator it = mapObj->map.begin(); it != mapObj->map.end(); it++) {
			yaml_scalar_event_initialize(event, NULL, NULL, (yaml_char_t*) (it->first.c_str()), (int) it->first.length(), 1, 1, YAML_ANY_SCALAR_STYLE);
			yaml_emitter_emit(emitter, event);
 
			if (it->second.mapPtr->map.size())
				yamlMap(emitter, event, it->second.mapPtr);
			else if (it->second.mapObjects.size())
				yamlSequence(emitter, event, &it->second.mapObjects, (it->second.flow == true ? 1 : 0));
			else {
				yaml_scalar_event_initialize(event, NULL, NULL, (yaml_char_t*) it->second.value.c_str(), (int) it->second.value.length(), 1, 1, YAML_ANY_SCALAR_STYLE);
				yaml_emitter_emit(emitter, event);
			}
		}
 
		yaml_mapping_end_event_initialize(event);
		yaml_emitter_emit(emitter, event);
	}
	return 1;
}
 
int MapObject::yamlSequence(yaml_emitter_t *emitter, yaml_event_t *event, std::vector<MapObject>* mapObjects, int flow_style) {
	if (mapObjects->size()) {
		yaml_sequence_start_event_initialize(event, NULL, NULL, true, (flow_style == false ? YAML_BLOCK_SEQUENCE_STYLE : YAML_FLOW_SEQUENCE_STYLE));
		yaml_emitter_emit(emitter, event);
 
		for (auto it = mapObjects->begin(); it != mapObjects->end(); it++) {
			if (it->mapPtr->map.size())
				yamlMap(emitter, event, it->mapPtr);
			else if (it->mapObjects.size())
				yamlSequence(emitter, event, &it->mapObjects, (it->flow == true ? 1 : 0));
			else if (! it->value.empty()) {
				yaml_scalar_event_initialize(event, NULL, NULL, (yaml_char_t*) it->value.c_str(), (int) it->value.length(), 1, 1, YAML_ANY_SCALAR_STYLE);
				yaml_emitter_emit(emitter, event);
			}
		}
 
		yaml_sequence_end_event_initialize(event);
		yaml_emitter_emit(emitter, event);
	}
	return 1;
}
 
int MapObject::yamlDocProlog(yaml_emitter_t *emitter, yaml_event_t *event) {
	yaml_stream_start_event_initialize(event, YAML_UTF8_ENCODING);
 
	if (!yaml_emitter_emit(emitter, event)) {
		yaml_emitter_delete(emitter);
		return 0;
	}
 
	yaml_document_start_event_initialize(event, NULL, NULL, NULL, true);
	yaml_emitter_emit(emitter, event);
 
	return 1;
}
 
int MapObject::yamlDocEpilog(yaml_emitter_t *emitter, yaml_event_t *event) {
	yaml_document_end_event_initialize(event, true);
	yaml_emitter_emit(emitter, event);
 
	yaml_stream_end_event_initialize(event);
	yaml_emitter_emit(emitter, event);
 
	return 1;
}
