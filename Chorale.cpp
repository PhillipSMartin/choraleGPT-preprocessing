#include "Chorale.h"
#include "XmlUtils.h"
#include <iostream>

using namespace tinyxml2;
using namespace XmlUtils;

bool Chorale::load_xml_file() { 
    isXmlLoaded_ = XmlUtils::load_xml_file( doc_, fileName_ );
    if (!isXmlLoaded_) 
        return false;

    XMLElement* _creditElement = try_get_child( doc_.RootElement(), "credit" );
    if (_creditElement) {
        XMLElement* _creditWordsElement = try_get_child( _creditElement, "credit-words" );
        if (_creditWordsElement)
            title_ = _creditWordsElement->GetText();
    }

    return true;
}
// sudo apt-get install libcurl4-openssl-dev
// #include <curl/curl.h>

// bool Chorale::load_xml_file() {
//     CURL* curl = curl_easy_init();
//     std::string buffer;
    
//     curl_easy_setopt(curl, CURLOPT_URL, s3Url_.c_str());
//     curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
//     curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
    
//     CURLcode result = curl_easy_perform(curl);
//     curl_easy_cleanup(curl);
    
//     if (result != CURLE_OK)
//         return false;
        
//     isXmlLoaded_ = (doc_.Parse(buffer.c_str()) == XML_SUCCESS);
//     // ... rest of your existing code ...
//     return true;
// }


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

