#pragma once

#include <tinyxml2.h>

namespace XmlUtils {
    bool load_xml_file( tinyxml2::XMLDocument& doc, const char* fileName );
    void printElement(tinyxml2::XMLElement* element, int depth = 0);

    // attempt to get child; print error message if not found
    tinyxml2::XMLElement* try_get_child( tinyxml2::XMLElement* parent, const char* name, const char* context = nullptr );
}