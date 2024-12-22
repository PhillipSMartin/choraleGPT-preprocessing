 #include "Chorale.h"
 #include "XmlUtils.h"
 #include <iostream>

 using namespace tinyxml2;
 using namespace XmlUtils;

 bool Chorale::build_part_list() {
    if (!isXmlLoaded_) {
         std::cerr << "XML file not loaded" << std::endl;
         return false;
    }

    // map part ids to part names
    std::map<std::string, std::string> _part_ids;
    XMLElement* _partList = XmlUtils::try_get_child( doc_.RootElement(), "part-list" );
    if (_partList) {

        XMLElement* _scorePart = try_get_child( _partList, "score-part" );
        if (!_scorePart)
            return false;

        while (_scorePart) {
            const char* _partId = _scorePart->Attribute( "id" );
            XMLElement* _partName = try_get_child( _scorePart, "part-name" );
            if (!_partName)
                return false;

            _part_ids[ _partId ] = _partName->GetText();
            _scorePart = _scorePart->NextSiblingElement( "score-part" );
        }
    }

    XMLElement* _part = try_get_child( doc_.RootElement(), "part");
    if (!_part)
        return false;

    while (_part) {
        auto _it = _part_ids.find( _part->Attribute( "id" ) );
        if (_it == _part_ids.end())
        {
            std::cerr << "Part id not found in part_list: " << _part->Attribute( "id" ) << std::endl;
            return false;
        }
        part_list_[ _it->second ] = _part;
        _part = _part->NextSiblingElement("part");
    }
    return true;
}

