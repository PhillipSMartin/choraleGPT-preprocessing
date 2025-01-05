#include "Arguments.h"
#include "Chorale.h"
#include "Part.h"
#include "XmlUtils.h"

#include <cmath>
#include <curl/curl.h>
#include <iostream>
#include <ranges>
#include <sstream>
#include <string_view>

using namespace tinyxml2;
using namespace XmlUtils;
using namespace std::literals;

unsigned int Chorale::lastBWV_{0};
char Chorale::lastModifier_{'`'};

void Chorale::parse_title_from_xml() {
    XMLElement* _creditElement = try_get_child( doc_.RootElement(), "credit", /* verbose =*/ false );
    if (_creditElement) {
        XMLElement* _creditWordsElement = try_get_child( _creditElement, "credit-words", /* verbose =*/ false);
        if (_creditWordsElement)
            title_ = _creditWordsElement->GetText();
    }
}

bool Chorale::load_xml() {
    isXmlLoaded_ = false;
    switch (Arguments::get_input_source_type( xmlSource_ )) {
        case Arguments::FILE:
            isXmlLoaded_ = load_xml_from_file( xmlSource_ );
            break;
        case Arguments::URL:
            isXmlLoaded_ = load_xml_from_url( xmlSource_ );
            break;
        default:
            std::cerr << "Invalid xml source type: " << xmlSource_ << std::endl;
            break;
    }

    if (isXmlLoaded_) {
        parse_title_from_xml();
    }
    return isXmlLoaded_;
}

bool Chorale::load_xml_from_file( const std::string& xmlSource ) { 
    return XmlUtils::load_from_file( doc_, xmlSource.c_str() );
}

bool Chorale::load_xml_from_url( const std::string& xmlSource ) {
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
        
    return XmlUtils::load_from_buffer( doc_, buffer.c_str() );
}

size_t Chorale::curl_callback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t realsize = size * nmemb;
    std::string* buffer = static_cast<std::string*>(userp);
    buffer->append((char*)contents, realsize);
    return realsize;
}

bool Chorale::build_xml_parts() {
    if (!isXmlLoaded_) {
         std::cerr << "XML file not loaded" << std::endl;
         return false;
    }

    // map part ids to part names
    xmlParts_.clear();
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
        xmlParts_[ _it->second ] = _partElement;
        _partElement = _partElement->NextSiblingElement("part");
    }
    return true;
}

tinyxml2::XMLElement* Chorale::get_xml_part( const std::string& partName ) const { 
    auto it = xmlParts_.find(partName);
    return (it != xmlParts_.end()) ? it->second : nullptr;
}

std::string Chorale::get_BWV() const {
    auto _it = std::find(xmlSource_.rbegin(), xmlSource_.rend(), '/');
    unsigned int _bwv = std::stoi(xmlSource_.substr(std::distance(_it, xmlSource_.rend())));
    std::ostringstream _bwvStream;

    // last two digits of bwv are a sub-group
    _bwvStream << "BWV "
                << static_cast<int>( std::floor( _bwv / 100 ) )
                << '.' 
                << _bwv % 100;

    // if we have already done this bwv, we must add a modifier character to the end
    if (_bwv == lastBWV_) {
        _bwvStream << ++lastModifier_;
    }
    else {
        lastBWV_ = _bwv;
        lastModifier_ = '`';
    }
    return _bwvStream.str();
}

bool Chorale::build_endcoded_parts( const std::vector<std::string>& partsToParse )
{
    encodedParts_.clear();

    // Get part list
    if (xmlParts_.empty()) {
        if (!build_xml_parts()) {
            std::cerr << "Failed to build part list" << std::endl;
            return false;
        }
    }

    std::string _bwv{ get_BWV() };
    for (const std::string& _partName : partsToParse) {
        Part _part{_bwv, title_, _partName};
        if (_part.parse_musicXml( get_xml_part( _partName ) )) {
            _part.transpose();
            encodedParts_[_partName] = _part.to_string();
        }
        else {
            std::cerr << "Failed to parse part: " << _partName << " for " << _bwv << std::endl;
        }
    }

    return encodedParts_.size() == partsToParse.size();
}

std::string Chorale::get_encoded_part(const std::string& partName) const {
    auto it = encodedParts_.find(partName);
    return (it != encodedParts_.end()) ? it->second : "";
}


