#include "Chorale.h"
#include "Part.h"
#include "XmlUtils.h"

#include <cmath>
#include <curl/curl.h>
#include <iostream>
#include <ranges>
#include <string_view>

using namespace tinyxml2;
using namespace XmlUtils;
using namespace std::literals;

bool Chorale::parse_title() {
    XMLElement* _creditElement = try_get_child( doc_.RootElement(), "credit", /* verbose =*/ false );
    if (_creditElement) {
        XMLElement* _creditWordsElement = try_get_child( _creditElement, "credit-words", /* verbose =*/ false);
        if (_creditWordsElement)
            title_ = _creditWordsElement->GetText();
    }

    return true;
}

bool Chorale::load_from_file( std::string xmlSource ) { 
    isXmlLoaded_ = XmlUtils::load_from_file( doc_, xmlSource.c_str() );
    if (!isXmlLoaded_) 
        return false;

    return parse_title();
}

bool Chorale::load_from_url( std::string xmlSource ) {
    CURL* curl = curl_easy_init();
    std::string buffer;
    
    curl_easy_setopt(curl, CURLOPT_URL, xmlSource.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
    
    CURLcode result = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    
    if (result != CURLE_OK) {
        std::cerr << "Failed to download XML file, rc=" << result << std::endl;
        return false;
    }
        
    isXmlLoaded_ = XmlUtils::load_from_buffer( doc_, buffer.c_str() );
    if (!isXmlLoaded_)  {
        auto _rc = doc_.SaveFile("data/test.xml");
        if (_rc != XML_SUCCESS) { 
            std::cerr << "Failed to save XML file. Error=" << doc_.ErrorName() << std::endl; 
            return false;
        } 
        return false;
    }

    return parse_title();
}

size_t Chorale::curl_callback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t realsize = size * nmemb;
    std::string* buffer = static_cast<std::string*>(userp);
    buffer->append((char*)contents, realsize);
    return realsize;
}

bool Chorale::build_part_list() {
    if (!isXmlLoaded_) {
         std::cerr << "XML file not loaded" << std::endl;
         return false;
    }

    // map part ids to part names
    std::map<std::string, std::string> _partIds;
    XMLElement* _partListElement = XmlUtils::try_get_child( doc_.RootElement(), "part-list" );
    if (_partListElement) {

        XMLElement* _scorePartElement = try_get_child( _partListElement, "score-part" );
        if (!_scorePartElement)
            return false;

        while (_scorePartElement) {
            const char* _partId = _scorePartElement->Attribute( "id" );
            XMLElement* _partNameElement = try_get_child( _scorePartElement, "part-name" );
            if (!_partNameElement)
                return false;

            _partIds[ _partId ] = _partNameElement->GetText();
            _scorePartElement = _scorePartElement->NextSiblingElement( "score-part" );
        }
    }

    XMLElement* _partElement = try_get_child( doc_.RootElement(), "part");
    if (!_partElement)
        return false;

    while (_partElement) {
        auto _it = _partIds.find( _partElement->Attribute( "id" ) );
        if (_it == _partIds.end())
        {
            std::cerr << "Part id not found in part_list: " << _partElement->Attribute( "id" ) << std::endl;
            return false;
        }
        partList_[ _it->second ] = _partElement;
        _partElement = _partElement->NextSiblingElement("part");
    }
    return true;
}

tinyxml2::XMLElement* Chorale::get_part( const std::string partName ) const { 
    auto it = partList_.find(partName);
    return (it != partList_.end()) ? it->second : nullptr;
}

std::string Chorale::get_BWV() const {
    auto _it = std::find(xmlSource_.rbegin(), xmlSource_.rend(), '/');
    unsigned int _bwv = std::stoi(xmlSource_.substr(std::distance(_it, xmlSource_.rend())));
    return "BWV "s + std::to_string( static_cast<int>( std::floor( _bwv / 100 ) ) ) + "."s + std::to_string(_bwv % 100);
}

std::vector<std::string> Chorale::encode_parts( const std::vector<std::string>& partsToParse )
{
    std::vector<std::string> _parts;

    // Get part list
    if (!build_part_list()) {
        std::cerr << "Failed to build part list" << std::endl;
        return _parts;
    }

    for (const std::string& _partName : partsToParse) {
        Part _part{get_BWV(), _partName};
        if (_part.parse_musicXml( get_part( _partName ) )) {
            _part.transpose();
            _parts.push_back( _part.to_string() );
        }
        else {
            std::cerr << "Failed to parse part: " << _partName << std::endl;
        }
    }

    return _parts;
}

