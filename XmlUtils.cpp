#include "XmlUtils.h"
#include <iostream>

using namespace tinyxml2;

namespace XmlUtils {
    XMLElement* try_get_child( tinyxml2::XMLElement* parent, const char* name, const char* context /* = nullptr */) {
        XMLElement* child = parent ? parent->FirstChildElement( name ) : nullptr;
        if (parent && !child) {
            std::cerr << "No " << name << " element found"; 
            if (context) {
                std::cerr << " for " << context;
            }
            std::cerr << std::endl;
        }

        return child;
    }

    void printElement(XMLElement* element, int depth /* = 0 */) {
        while (element) {
            // Print indentation
            for (int i = 0; i < depth; i++) {
                std::cout << "  ";
            }
            
            // Print element name and attributes
            std::cout << element->Name();
            const XMLAttribute* attr = element->FirstAttribute();
            while (attr) {
                std::cout << " " << attr->Name() << "=\"" << attr->Value() << "\"";
                attr = attr->Next();
            }
            std::cout << std::endl;
            
            // Recursively print child elements
            XMLElement* child = element->FirstChildElement();
            if (child) {
                printElement(child, depth + 1);
            }
            
            element = element->NextSiblingElement();
        }
    }

    bool load_xml_file( tinyxml2::XMLDocument& doc, const char* fileName ) {
        XMLError rc = doc.LoadFile( fileName );
        if (rc != XML_SUCCESS) { 
            std::cerr << "Failed to load XML file. rc=" << rc << std::endl; 
            return false;
        } 
        return true;
    }
}