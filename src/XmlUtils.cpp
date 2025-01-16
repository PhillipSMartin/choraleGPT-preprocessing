#include "XmlUtils.h"
#include <iostream>

using namespace tinyxml2;

namespace XmlUtils {
/**
     * Tries to get the first child element of the given parent element with the specified name.
     * If the child element is not found and `verbose` is true, logs a message to `std::cerr`.
     *
     * @param parent The parent element to search for the child.
     * @param name The name of the child element to find.
     * @param verbose Whether to log a message if the child element is not found.
     * @return The first child element with the specified name, or `nullptr` if not found.
     */
        XMLElement* try_get_child( XMLElement* parent, const char* name, bool verbose /* = true */ ) {
        XMLElement* child = parent ? parent->FirstChildElement( name ) : nullptr;
        if (verbose && parent && !child) {
            std::cerr << "No " << name << " element found"; 
            std::cerr << std::endl;
        }

        return child;
    }

    /**
     * Recursively prints the XML element and its children to the console, with indentation.
     *
     * @param element The XML element to print.
     * @param depth The current depth of the element in the XML tree (default is 0).
     */
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

    /**
     * Loads an XML document from a file.
     *
     * @param doc The XMLDocument object to load the file into.
     * @param fileName The path to the XML file to load.
     * @return `true` if the file was loaded successfully, `false` otherwise.
     */
    bool load_from_file( XMLDocument& doc, const char* fileName ) {
        XMLError rc = doc.LoadFile( fileName );
        if (rc != XML_SUCCESS) { 
            std::cerr << "Failed to load XML file. Error=" << doc.ErrorName() << std::endl; 
            return false;
        } 
        return true;
    }

    /**
     * Loads an XML document from a memory buffer.
     *
     * @param doc The XMLDocument object to load the XML data into.
     * @param buffer The memory buffer containing the XML data.
     * @return `true` if the XML data was loaded successfully, `false` otherwise.
     */
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