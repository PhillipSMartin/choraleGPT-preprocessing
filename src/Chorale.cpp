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
std::unique_ptr<Part> Chorale::nullPart_ = nullptr;

std::string Chorale::get_title_from_xml() {
    XMLElement* _creditElement = try_get_child( doc_.RootElement(), "credit", /* verbose =*/ false );
    if (_creditElement) {
        XMLElement* _creditWordsElement = try_get_child( _creditElement, "credit-words", /* verbose =*/ false);
        if (_creditWordsElement)
            return _creditWordsElement->GetText();
    }
    return "";
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
        title_ = get_title_from_xml();
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

bool Chorale::load_part_xmls() {
    if (!isXmlLoaded_) {
         std::cerr << "XML file not loaded" << std::endl;
         return false;
    }

    // map part ids to part names
    // xml sample:
    //    <part-list>
    //         ...
    //         <score-part id="P1">
    //             <part-name>Soprano</part-name>
    //              ...
    //         </score-part>
    //         ...
    //    </part-list>

    partXmls_.clear();
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

    // get the parts themselves
    // xml sample
    //  <part id="P1">
    //     <measure implicit="yes" number="0">
    //     ...
    //     </measure>
    //     ...
    //  </part>

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

        // save it in dictionary, keyed by part name
        partXmls_[ _it->second ] = _partElement;
        _partElement = _partElement->NextSiblingElement("part");
    }
    return true;
}

tinyxml2::XMLElement* Chorale::get_part_xml( const std::string& partName ) const { 
    auto it = partXmls_.find(partName);
    return (it != partXmls_.end()) ? it->second : nullptr;
}

std::string Chorale::build_BWV( const std::string& xmlSource ) const {
    auto _it = std::find(xmlSource.rbegin(), xmlSource.rend(), '/');
    unsigned int _bwv = std::stoi(xmlSource.substr(std::distance(_it, xmlSource.rend())));
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

void Chorale::load_parts( const std::vector<std::string>& partsToParse ) {
    parts_.clear();

    // get each requested part name
    for (const std::string& _partName : partsToParse) {

        // instantiate a Part object and store it in a dictionary, keyed by part name
        parts_[ _partName ] = std::make_unique<Part>( bwv_, title_, _partName );

        // parts_.emplace(std::piecewise_construct,
        //     std::forward_as_tuple(_partName), // std::string
        //     std::forward_as_tuple(bwv_, title_, _partName)); // Part
    }
}

void Chorale::load_parts( std::vector<std::unique_ptr<Part>>& parts ) {
    // for (const auto& _part : parts) {
    //     parts_[_part->get_part_name()] = std::move(_part);
    // } 
}

bool Chorale::encode_parts()
{
    // load part xmls
    if (partXmls_.empty()) {
        if (!load_part_xmls()) {
            std::cerr << "Failed to build part list" << std::endl;
            return false;
        }
    }  

    for (auto& _it : parts_) {
        // retrieve xml for this part
        auto& _part = _it.second;
        XMLElement* _partXml = get_part_xml( _it.first );
        if (!_partXml) {
            std::cerr << "Part not found: " << _it.first << std::endl;
            return false;
        }  

        // encode it 
        if (_part->parse_xml( _partXml )) {
            // transpose it to C major or A minor
            _part->transpose();
            // normalize the meter, so that each part contains the same number of sub-beats
            _part->set_sub_beats( MIN_SUBBEATS );
        }
        else {
            std::cerr << "Failed to parse part: " << _it.first << " for " << bwv_ << std::endl;
            return false;
        }
    }

    return true;
}

std::unique_ptr<Part>& Chorale::get_part(const std::string& partName) {
    auto _it = parts_.find(partName);
    return (_it != parts_.end()) ? _it->second : nullPart_;
}
