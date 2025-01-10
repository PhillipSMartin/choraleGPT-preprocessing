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
        // nullptr to return when a part is not found
        static std::unique_ptr<Part> nullPart_;
        
        // the xml input
        std::string xmlSource_;
        tinyxml2::XMLDocument doc_;
        bool isXmlLoaded_ = false;

        // metadata
        std::string bwv_;       // BWV number
        std::string title_;     // chorale title (empty if not supplied by xml)

        // maps
        std::map<std::string, std::string> partIds_;
        std::map<std::string, tinyxml2::XMLElement*> partXmls_; // the xml elements for each part
        std::map<std::string, std::unique_ptr<Part>> parts_;    // the Part objects for each part

        // an ordered list of partIds in case the xml uses non-standard part ids and names
        static inline std::map<std::string, size_t> partNameIndices_ = { {"Soprano", 0},
            {"Alto", 1}, {"Tenor", 2}, {"Bass", 3} };
        std::vector<std::string> partIdList_;

    public:
        // if bwv is empty, we will generate it from the xmlSource
        Chorale( const std::string& xmlSource, const std::string& bwv = "") : 
            xmlSource_{xmlSource}, 
            bwv_{(bwv.length() == 0) ? build_BWV(xmlSource) : bwv}  {}

        // --- load functions ---

        // read and process the xmlSource
        bool load_xml();  

        // build parts_, mapping part names to empty Part objects
        void load_parts( const std::vector<std::string>& partsToParse ); 

        // buildl parts_ from existing Part objects
        void load_parts( std::vector<std::unique_ptr<Part>>& parts ); 
        
        // --- process functions ---

        // encode the xml in partXmls_ into the associated Part objects in parts_
        bool encode_parts();  

        // combine individual parts int parts_ into a new combined Part object with Chords instead of Notes 
        bool combine_parts( bool verbose = false );   

        // getters
        std::string get_BWV() const { return bwv_; }
        std::string get_title() const { return title_; }
        tinyxml2::XMLElement* get_part_xml( const std::string& partName ) const;
        std::unique_ptr<Part>& get_part( const std::string& partName );
 
    private:
        // --- helper function from load_xml() ---
        bool load_xml_from_file( const std::string& xmlSource ); 
        bool load_xml_from_url( const std::string& xmlSource );

        // --- helper functions for encode_parts() ---

        // build partIds_, mapping part ids to part names in partIds-
        bool load_part_ids();  
        // build partXmls_, mappting part names to their XML Elements
        bool load_part_xmls(); 

        // used by load_xml_from_url()
        static size_t curl_callback(void* contents, size_t size, size_t nmemb, void* userp);

        std::string get_title_from_xml();
        std::string build_BWV( const std::string& xmlSource ) const;
};

