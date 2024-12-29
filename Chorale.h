#pragma once
#include "XmlUtils.h"
#include <map>
#include <string>

class Chorale {
    private:
        std::string xmlSource_;
        tinyxml2::XMLDocument doc_;
        bool isXmlLoaded_ = false;

        std::string title_;
        std::map<std::string, tinyxml2::XMLElement*> partList_;

    public:
        Chorale( const char* xmlSource ) : xmlSource_( xmlSource ) {}
        bool load_from_file();
        bool load_from_url();
        bool build_part_list();

        std::string get_title() const { return title_; }
        std::string get_BWV() const;
        tinyxml2::XMLElement* get_part( const std::string partName ) const;

    private:
        static size_t curl_callback(void* contents, size_t size, size_t nmemb, void* userp);
        bool parse_chorale_data();
};