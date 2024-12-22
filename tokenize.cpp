#include "Chorale.h"
#include "Part.h"
#include <iostream>

using namespace tinyxml2;

// The MusicXML document
XMLDocument doc;

XMLElement* get_root( const char* filename ) {
    XMLError rc = doc.LoadFile( filename );
    if (rc != XML_SUCCESS) { 
        std::cerr << "Failed to load XML file. rc=" << rc << std::endl; 
        return nullptr;
    }   
     
    return doc.FirstChildElement("score-partwise");
}

XMLElement* get_next_measure_for_part_id( XMLElement* element, const char* partId ) { 
    if (!element) {
        std::cerr << "No element supplied" << std::endl;
        return nullptr;
    }

    // if element passed is a part, return first measure
    if (strcmp( element->Name(), "part" ) == 0) {
        // Return first measure 
        return element->FirstChildElement( "measure" );
    }

    // if element passed is a measure, return next measure
    else {
        return element->NextSiblingElement( "measure" );
    } 
} 

XMLElement* get_part( XMLElement* root, const char* partId ) {
    if (!root) {
        std::cerr << "No root supplied" << std::endl;
        return nullptr;
    }

    // Find the part with matching Id
    XMLElement* part = root->FirstChildElement("part");
    while (part && strcmp(part->Attribute("id"), partId) != 0) {
        part = part->NextSiblingElement("part");
    }

    return part;
}

const char* get_part_id( XMLElement* partList, const char* partName ) {
    if (!partList) {
        std::cerr << "No part list supplied" << std::endl;
        return nullptr;
    }
    XMLElement* scorePart = partList->FirstChildElement( "score-part" );
    while (scorePart) {
        XMLElement* partNameElement = scorePart->FirstChildElement( "part-name" );
        if (partNameElement && strcmp( partNameElement->GetText(), partName ) == 0) {
            const char* partId = scorePart->Attribute( "id" );
            std::cout << partName << " part id: " << partId << std::endl;
            return partId;
        }
        scorePart = scorePart->NextSiblingElement( "score-part" );
    }

    std::cerr << "Part not found: " << partName << std::endl;
    return nullptr;
}


int main( int argc, char** argv ) { 
    // load xml file passed as first argument
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <xml_file> [<part_name>]" << std::endl;
        return 1;
    }

    const char* _fileName = argv[1];
    const char* _partName = (argc > 2) ? argv[2] : "Soprano";

    Chorale _chorale{_fileName};
    if (!_chorale.load_xml_file())
        return 1;
    if (!_chorale.build_part_list())
        return 1;

    // XMLElement* root = get_root( argv[1] );
    // if (!root) {
    //     return 1;
    // }

    // // Find id of Soprano part
    // XMLElement* partList = root->FirstChildElement( "part-list" );
    // const char* partId = get_part_id(partList, _partName);
    // if (!partId) {
    //     return 1;
    // }

    // XMLElement* _partElement = get_part( root, partId );
    // Part  _part( _partName );
    // _part.parse_musicXml( _partElement );
    // std::cout << _part << std::endl;

    return 0;
}