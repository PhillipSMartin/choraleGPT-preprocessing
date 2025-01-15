#include "Arguments.h"
#include "Chorale.h"
#include "CombinedPart.h"

#include <cmath>
#include <curl/curl.h>
#include <iostream>
#include <ranges>
#include <sstream>
#include <string_view>

using namespace tinyxml2;
using namespace XmlUtils;

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

bool Chorale::load_part_ids() {
    // xml sample:
    //    <part-list>
    //         ...
    //         <score-part id="P1">
    //             <part-name>Soprano</part-name>
    //              ...
    //         </score-part>
    //         ...
    //    </part-list>
    if (!isXmlLoaded_) {
         std::cerr << "XML file not loaded" << std::endl;
         return false;
    }

    partIds_.clear();

    // get the <part-list> element
    XMLElement* _partListElement = XmlUtils::try_get_child( doc_.RootElement(), "part-list" );
    if (_partListElement) {

        // get the first <score-part> element under <part-list>
        XMLElement* _scorePartElement = try_get_child( _partListElement, "score-part" );
        if (!_scorePartElement)
            return false;

        while (_scorePartElement) {
            // get the id attribute from the <score-part> element
            const char* _partId = _scorePartElement->Attribute( "id" );
            // get the part name from the <part-name> element (child of <score-part>)
            XMLElement* _partNameElement = try_get_child( _scorePartElement, "part-name" );
            if (!_partNameElement)
                return false;

            // save the part name in a temporary map keyed by part id
            partIds_[ _partId ] = _partNameElement->GetText();

            // get the next <score-part> element under <part-list>
            _scorePartElement = _scorePartElement->NextSiblingElement( "score-part" );
        }
    }

    return true;
}

bool Chorale::load_part_xmls() {
    // xml sample
    //  <part id="P1">
    //     <measure implicit="yes" number="0">
    //     ...
    //     </measure>
    //     ...
    //  </part>

    if (partIds_.empty()) {
         std::cerr << "Part ids not loaded" << std::endl;
         return false;
    }

    partXmls_.clear();

    // get the first <part> element 
    XMLElement* _partElement = try_get_child( doc_.RootElement(), "part");
    if (!_partElement)
        return false;

    while (_partElement) {
        // save the Part id so we have a record of the order they were imported
        partIdList_.push_back( _partElement->Attribute( "id" ) );

        // look up the part name for this part id 
        auto _it = partIds_.find( _partElement->Attribute( "id" ) );
        if (_it == partIds_.end())
        {
            std::cerr << "Part id not found in part_list: " << _partElement->Attribute( "id" ) << std::endl;
            return false;
        }

        // save the XML element in partXmls_ 
        partXmls_[ _it->second ] = _partElement;
        _partElement = _partElement->NextSiblingElement("part");
    }
    return true;
}

tinyxml2::XMLElement* Chorale::get_part_xml( const std::string& partName ) const { 
    auto it = partXmls_.find( partName );

    // if we can't find it, perhaps it's under a different name
    if (it == partXmls_.end()) {
        if (partIdList_.size() == 4) {
            // If four parts, assume non-standard part names

            // Get the name of the part in the appropriate position
            std::string _assumedPartId{partIdList_[partNameIndices_[partName]]};
            std::string _newPartName{partIds_.find(_assumedPartId)->second};
            std::cout << "Using part name " << _newPartName << " instead of " << partName 
                << " in " << bwv_ << std::endl;
            it = partXmls_.find( _newPartName );
        }
    }
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

    // get each requested part name (passed as a command-line argument)
    for (const std::string& _partName : partsToParse) {
        // instantiate a Part object and store it in a dictionary, keyed by part name
        parts_[ _partName ] = std::make_unique<Part>( bwv_, title_, _partName );
    }
}

void Chorale::load_parts( std::vector<std::unique_ptr<Part>>& parts ) {
    for (auto& _part : parts) {
        parts_[_part->get_part_name()] = std::move( _part );
    }
}

bool Chorale::encode_parts()
{
    // map part ids to part names
    if (partIds_.empty()) {
        if (!load_part_ids()) {
            std::cerr << "Failed to map part ids to part names for " << bwv_ << std::endl;
            return false;
        }
    }

    // load part xmls
    if (partXmls_.empty()) {
        if (!load_part_xmls()) {
            std::cerr << "Failed to build part list for " << bwv_ << std::endl;
            return false;
        }
    }  

    for (auto& _it : parts_) {
        // retrieve xml for this part
        auto& _part = _it.second;

        // encode it 
        if (_part->parse_xml( get_part_xml( _it.first ) )) {
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

bool Chorale::combine_parts( bool verbose, bool noEOM ) {
    combinedPart_ = std::make_unique<CombinedPart>( get_part( "Soprano" ),
        get_part( "Alto" ),
        get_part( "Tenor" ),
        get_part( "Bass" ) );  
    return combinedPart_->build( verbose, noEOM );
}
