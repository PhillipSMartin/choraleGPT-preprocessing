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
        std::map<std::string, tinyxml2::XMLElement*> partList_;

    public:
        Chorale( std::string xmlSource ) : xmlSource_( xmlSource ) {}
        bool load();
        bool build_part_list();

        std::string get_title() const { return title_; }
        std::string get_BWV() const;
        tinyxml2::XMLElement* get_part( const std::string partName ) const;
        std::vector<std::string> encode_parts( const std::vector<std::string>& partsToParse );

    private:
        bool load_from_file( std::string xmlSource );
        bool load_from_url( std::string xmlSource );
        static size_t curl_callback(void* contents, size_t size, size_t nmemb, void* userp);
        void parse_title();
};