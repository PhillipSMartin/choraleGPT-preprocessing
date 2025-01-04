#pragma once
#include "XmlUtils.h"
#include <map>
#include <string>
#include <vector>

class Chorale {
    private:
        static unsigned int lastBWV_;
        static char lastModifier_;
        std::string xmlSource_;
        tinyxml2::XMLDocument doc_;
        bool isXmlLoaded_ = false;

        std::string title_;
        std::map<std::string, tinyxml2::XMLElement*> xmlParts_;
        std::map<std::string, std::string> encodedParts_;

    public:
        Chorale( const std::string& xmlSource ) : xmlSource_( xmlSource ) {}
        bool load_xml();
        bool build_xml_parts();
        bool build_endcoded_parts( const std::vector<std::string>& partsToParse );

        std::string get_title() const { return title_; }
        std::string get_BWV() const;
        tinyxml2::XMLElement* get_xml_part( const std::string& partName ) const;
        std::string get_encoded_part( const std::string& partName ) const;
 
    private:
        bool load_xml_from_file( const std::string& xmlSource );
        bool load_xml_from_url( const std::string& xmlSource );
        static size_t curl_callback(void* contents, size_t size, size_t nmemb, void* userp);
        void parse_title_from_xml();
};