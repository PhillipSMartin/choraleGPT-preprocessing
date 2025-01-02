#include "XmlUtils.h"
#include <iostream>

using namespace tinyxml2;

namespace XmlUtils {
    XMLElement* try_get_child( XMLElement* parent, const char* name, bool verbose /* = true */ ) {
        XMLElement* child = parent ? parent->FirstChildElement( name ) : nullptr;
        if (verbose && parent && !child) {
            std::cerr << "No " << name << " element found"; 
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
            if (child && (depth <= 1)) {
                printElement(child, depth + 1);
            }
            
            element = element->NextSiblingElement();
        }
    }

    bool load_from_file( XMLDocument& doc, const char* fileName ) {
        XMLError rc = doc.LoadFile( fileName );
        if (rc != XML_SUCCESS) { 
            std::cerr << "Failed to load XML file. Error=" << doc.ErrorName() << std::endl; 
            return false;
        } 
        return true;
    }

    bool load_from_buffer( XMLDocument& doc, const char* buffer ) {
        if (!buffer || buffer[0] == '\0') {
            std::cerr << "Empty or null buffer provided" << std::endl;
            return false;
        } 

        XMLError rc = doc.Parse(buffer);
        if (rc != XML_SUCCESS) { 
            std::cerr << "Failed to parse XML file. Error=" << doc.ErrorName() << std::endl; 
            return false;
        } 

        // Verify document has content
        if (!doc.RootElement()) {
            std::cerr << "Parsed document has no root element" << std::endl;
            return false;
        }

        XMLElement* _error = doc.FirstChildElement("Error");
        if (_error) {
            XMLElement* _message = _error->FirstChildElement("Message");
            if (_message) {
                std::cout << "Error downloading document: " << _message->GetText() << std::endl;
                return false;
            }
        }

        return true;
    }
}