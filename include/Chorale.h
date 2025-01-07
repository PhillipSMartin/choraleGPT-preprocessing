#pragma once
#include "Part.h"
#include "XmlUtils.h"

#include <map>
#include <string>
#include <vector>

class Chorale {
    private:
        // if a new chorale is instantiated with the same BWV as the previous chorale,
        //  we append a modifier (e.g. 'a', 'b', etc.) to this chorale's BWV
        static unsigned int lastBWV_;
        static char lastModifier_;
         // if sub_beats in the musicXml is less than this, we will multiply all durations to increase it to this value
        static inline const unsigned int MIN_SUBBEATS = 8;  
        static std::unique_ptr<Part> nullPart_;
        
        std::string xmlSource_;
        tinyxml2::XMLDocument doc_;
        bool isXmlLoaded_ = false;

        std::string bwv_;
        std::string title_;
        std::map<std::string, tinyxml2::XMLElement*> partXmls_;
        std::map<std::string, std::unique_ptr<Part>> parts_;

    public:
        // if bwv is empty, we will generate it from the xmlSource
        Chorale( const std::string& xmlSource, const std::string& bwv = "") : 
            xmlSource_{xmlSource}, 
            bwv_{(bwv.length() == 0) ? build_BWV(xmlSource) : bwv}  {}
        bool load_xml();
        bool load_part_xmls();
        void load_parts( const std::vector<std::string>& partsToParse );
        void load_parts( std::vector<std::unique_ptr<Part>>& parts );
        bool encode_parts();

        std::string get_BWV() const { return bwv_; }
        std::string get_title() const { return title_; }
        tinyxml2::XMLElement* get_part_xml( const std::string& partName ) const;
        std::unique_ptr<Part>& get_part( const std::string& partName );
 
    private:
        bool load_xml_from_file( const std::string& xmlSource );
        bool load_xml_from_url( const std::string& xmlSource );
        static size_t curl_callback(void* contents, size_t size, size_t nmemb, void* userp);
        std::string get_title_from_xml();
        std::string build_BWV( const std::string& xmlSource ) const;
};

