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
    }
}

void Chorale::load_parts( std::vector<std::unique_ptr<Part>>& parts ) {
    for (auto& _part : parts) {
        parts_[_part->get_part_name()] = std::move(_part);
    }
}

bool Chorale::encode_parts()
{
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

bool Chorale::combine_parts() {
    parts_["Combined"] = std::make_unique<Part>( bwv_, title_, "Combined" );

    // get tokens for each part
    auto& _sopranoPart = get_part("Soprano");
    auto& _altoPart = get_part("Alto");
    auto& _tenorPart = get_part("Tenor");
    auto& _bassPart = get_part("Bass");

    // add a new combined part
    auto& _combinedParts = get_part("Combined");   
    _combinedParts->set_sub_beats( _sopranoPart->get_sub_beats() );
    _combinedParts->set_beats_per_measure( _sopranoPart->get_beats_per_measure() );

    // keep track of position and define lambdas for error messages
    unsigned int _measureNo = 0;
    unsigned int _subBeatNo = 0;;
    unsigned int _subBeatsPerBeat = _sopranoPart->get_sub_beats();

    auto show_progress = [&_measureNo, &_subBeatNo, &_subBeatsPerBeat, this]() -> std::string {
        return "at m. " + std::to_string( _measureNo + 1 ) 
            + ", beat " + std::to_string( (_subBeatNo / _subBeatsPerBeat ) + 1 )
            + " in " + bwv_;
    };

    // get the first token for each part
    auto _sopranoToken = _sopranoPart->pop_encoding();
    auto _altoToken = _altoPart->pop_encoding();
    auto _tenorToken = _tenorPart->pop_encoding();
    auto _bassToken = _bassPart->pop_encoding();

    // define lambda to display current tokens  
    auto show_tokens = [&_sopranoToken, 
            &_altoToken,
            &_tenorToken, 
            &_bassToken]() -> std::string {
        return "Soprano: " + (_sopranoToken ? _sopranoToken->to_string() : "<NULL>") + '\n' +
            + "Alto: " + (_altoToken ? _altoToken->to_string() : "<NULL>") + '\n' +
            + "Tenor: " + (_tenorToken ? _tenorToken->to_string() : "<NULL>") + '\n' +
            + "Bass: " +(_bassToken ?  _bassToken->to_string() : "<NULL>");
    };

    // define booleans to see if we need a new token for a given part
    bool _needSopranoToken = false;
    bool _needAltoToken = false;
    bool _needTenorToken = false;
    bool _needBassToken = false;
    bool _done = false;

    // define lambda to get next token for a given part if necessary
    auto get_next_token = []( std::unique_ptr<Encoding>& _token,  
                std::unique_ptr<Part>& _part,
                bool& needToken) {
            if (needToken) {
                _token = _part->pop_encoding();
                needToken = false;
            }   
        };

    // define lambda to reduce duration of a token
    auto reduce_duration = []( Note* note,
                const unsigned int reduction,
                bool& needToken) {

            // calcuate number of sub-beats left
            int _newDuration = note->get_duration() - reduction;

            // if we are done with this note, get another
            if (_newDuration <= 0) {
                needToken = true;
            }
            // otherwise next note is tied to this one
            else {
                note->set_duration( _newDuration );
                note->set_tie( true );
            }
        };

    while (!_done) {
        // if we need another token, get it
        get_next_token( _sopranoToken, _sopranoPart, _needSopranoToken );
        get_next_token( _altoToken, _altoPart, _needAltoToken );
        get_next_token( _tenorToken, _tenorPart, _needTenorToken );
        get_next_token( _bassToken, _bassPart, _needBassToken );

        // if we ran out of tokens, we have a problem
        if (!_sopranoToken || !_altoToken || !_tenorToken || !_bassToken) {
            std::cerr << "Total duration of parts does not match " << show_progress() << std::endl;
            std::cerr << show_tokens() << std::endl;
            return false;
        }

        // if we have a marker in one part not matched in the others, we have a problem
        if (*_sopranoToken != *_altoToken 
            || *_sopranoToken != *_tenorToken 
            || *_sopranoToken != *_bassToken) {
            std::cerr << "Incompatible encodings " << show_progress() << std::endl;
            std::cerr << show_tokens() << std::endl;
            return false;
        }

        // if we have a Marker, add it to the combined parts
        if (_sopranoToken->is_marker() ) {
            auto _marker = dynamic_cast<const Marker*>( _sopranoToken.get() );

            // if it is EOC, we are done
            if (_marker->get_marker_type() == Marker::MarkerType::EOC) {
                _done = true;
            }
            // if it is EOM, we have finished a measure
            else if (_marker->get_marker_type() == Marker::MarkerType::EOM) {
                _measureNo++;
                _subBeatNo = 0;
            }

 
            _combinedParts->push_encoding( _sopranoToken );
            std::cout << "Added marker: " << _combinedParts->get_last_encoding()->to_string() << std::endl;         
           _needSopranoToken = _needAltoToken = _needTenorToken = _needBassToken = true;

            continue;
        }

        // if we have a Note, build a chord and add it to combined parts
        if (_sopranoToken->is_note()) {
            auto _sopranoNote = dynamic_cast<Note*>( _sopranoToken.get() );
            auto _altoNote = dynamic_cast<Note*>( _altoToken.get() );
            auto _tenorNote = dynamic_cast<Note*>( _tenorToken.get() );
            auto _bassNote = dynamic_cast<Note*>( _bassToken.get() );  

            // find the shortest duration among all the parts     
            unsigned int _shortestDuration = std::min( {_sopranoNote->get_duration(), 
                _altoNote->get_duration(), 
                _tenorNote->get_duration(), 
                _bassNote->get_duration()} );

            // add the note to the combined parts
            Note _notes[4] = { *_sopranoNote, *_altoNote, *_tenorNote, *_bassNote };

            auto chord = std::make_unique<Chord>(_notes, _shortestDuration);
            auto encoding = std::unique_ptr<Encoding>(chord.release());
            _combinedParts->push_encoding(encoding);

            // reduce durations
            reduce_duration( _sopranoNote, _shortestDuration, _needSopranoToken );
            reduce_duration( _altoNote, _shortestDuration, _needAltoToken );
            reduce_duration( _tenorNote, _shortestDuration, _needTenorToken );
            reduce_duration( _bassNote, _shortestDuration, _needBassToken );

            std::cout << "Added chord " << show_progress() << ":  " << _combinedParts->get_last_encoding()->to_string() << std::endl;
            _subBeatNo += _shortestDuration;
        }
    }

    return true;
}
