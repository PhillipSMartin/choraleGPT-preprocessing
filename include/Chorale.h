#pragma once
#include "Part.h"
#include "XmlUtils.h"

#include <map>
#include <optional>
#include <string>
#include <vector>

class Chorale {
    private:
        static unsigned int lastBWV_;
        static char lastModifier_;
         // if sub_beats in the musicXml is less than this, we will multiply all durations to increase it to this value
        static inline const unsigned int MIN_SUBBEATS = 8;  

        std::string xmlSource_;
        tinyxml2::XMLDocument doc_;
        bool isXmlLoaded_ = false;

        std::string title_;
        std::map<std::string, tinyxml2::XMLElement*> partXmls_;
        std::map<std::string, Part> parts_;

    public:
        Chorale( const std::string& xmlSource ) : xmlSource_( xmlSource ) {}
        
        bool load_xml();
        bool load_part_xmls();
        void load_parts( const std::vector<std::string>& partsToParse );
        bool encode_parts();

        std::string get_title() const { return title_; }
        std::string get_BWV() const;
        tinyxml2::XMLElement* get_part_xml( const std::string& partName ) const;
        std::optional<std::reference_wrapper<Part>> get_part( const std::string& partName );
 
    private:
        bool load_xml_from_file( const std::string& xmlSource );
        bool load_xml_from_url( const std::string& xmlSource );
        static size_t curl_callback(void* contents, size_t size, size_t nmemb, void* userp);
        void parse_title_from_xml();
};