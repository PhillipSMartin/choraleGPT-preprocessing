#pragma once
#include "XmlUtils.h"
#include <map>
#include <string>

class Chorale {
    private:
        const char* fileName_;
        tinyxml2::XMLDocument doc_;
        bool isXmlLoaded_ = false;

        std::map<std::string, tinyxml2::XMLElement*> partList_;

    public:
        Chorale( const char* fileName ) : fileName_( fileName ) {}
        bool load_xml_file() { 
            return isXmlLoaded_ = XmlUtils::load_xml_file( doc_, fileName_ );
        }

        bool build_part_list();
        tinyxml2::XMLElement* get_part( const std::string partName ) const;
};