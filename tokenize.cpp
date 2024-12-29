#include "Arguments.h"
#include "Chorale.h"
#include "Part.h"

#include <iostream>
#include <string>

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
    Arguments args;
    if (!args.parse_command_line(argc, argv))
        return 1;

    // Initialze Chorale 
    Chorale _chorale{args.get_xml_source()};

    bool _loaded = false;
    switch (args.get_xml_source_type()) {
        case Arguments::FILE:
            _loaded = _chorale.load_from_file();
            break;
        case Arguments::URL:
            _loaded = _chorale.load_from_url();
            break;
    }
    if (!_loaded)
        return 1;

    // Print requested parts
    if (!_chorale.build_part_list())
        return 1;
    std::cout << "Chorale: " << _chorale.get_title() << "\n\n";
    for (const std::string& _partName : args.get_parts_to_parse()) {
        Part _part{_chorale.get_BWV(), _partName};
        _part.parse_musicXml( _chorale.get_part( _partName ) );
        std::cout << _part << '\n';
        _part.transpose();
        std::cout << _part << '\n';
    }
    std::cout << std::endl;

    return 0;
}