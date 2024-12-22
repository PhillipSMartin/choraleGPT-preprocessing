#pragma once

#include "XmlUtils.h"
#include <string>
#include <tinyxml2.h>


class Part {
    public:
        enum Key {
            C = 0,
            C_SHARP = 7,
            D_FLAT = -5,
            D = 2,
            E_FLAT = -3,
            E = 4,
            F = -1,
            F_SHARP = 6,
            G_FLAT = -6,
            G = 1,
            A_FLAT = -4,
            A = 3,
            B_FLAT = -2,
            B = 5
        };
        enum Mode {
            MAJOR,
            MINOR
        };

        // special tokens
        static constexpr const char* SOC = "<soc>";  // start of chorale
        static constexpr const char* EOM = "<eom>";  // end of measure
        static constexpr const char* EOP = "<eop>";  // end of phrase
        static constexpr const char* EOC = "<eoc>";  // end of chorale

    private:
        int beatsPerMeasure_ = 0;
        int divisionsPerBeat_ = 0;
        Key key_ = Key::C;
        Mode mode_ = Mode::MAJOR;
        std::string partName_;

        // line_ consists of a sequence of space-separated words:
        //  SOC: always the first word
        //  <note>: a note token of the format <pitch>.<duration>
        //      <pitch> consists of a capital letter (A-G) and an octave number or 'R' to indicate a rest
        //          optionally followed by +1, +2, -1, or -2, indicating the number
        //          of half-steps above or below the given note
        //      <duration> is a number indicating the number of beats the note
        //  EOP: end of phrase (precedes EOM if a phrase ends at a measure)
        //  EOM: end of measure 
        //  EOC: always the last word
        std::string line_;

    public:
        Part(const std::string partName) : partName_{partName} {}

        // parse the MusicXML 'Part' element, set variables accordingly, 
        //  and return true if successful
        // error messages are written to std::cerr if unsuccessful
        bool parse_musicXml( tinyxml2::XMLElement* part );

        std::string get_line() const { return line_; }
        std::string get_partName() const { return partName_; }
        int get_beatsPerMeasure() const { return beatsPerMeasure_; }
        int get_divisionsPerBeat() const { return divisionsPerBeat_; }
        Key get_key() const { return key_; }
        Mode get_mode() const { return mode_; }

        friend std::ostream& operator <<( std::ostream& os, const Part& part);

    private:
        // parse the MusicXML 'attributes' element, set variables accordingly
        bool parse_attributes( tinyxml2::XMLElement* attributes );

        // parse the MusicXML 'measure' element, append words to line_ accordingly
        bool parse_measure( tinyxml2::XMLElement* measure );

        // parse the MusicXML 'note' element, append words to line_ accordingly
        bool parse_note( tinyxml2::XMLElement* note );

        std::string key_to_string() const {
            switch (key_) {
                case Key::C: return "C";
                case Key::C_SHARP: return "C_SHARP";
                case Key::D_FLAT: return "D_FLAT";
                case Key::D: return "D";
                case Key::E_FLAT: return "E_FLAT";
                case Key::E: return "E";
                case Key::F: return "F";
                case Key::F_SHARP: return "F_SHARP";
                case Key::G_FLAT: return "G_FLAT";
                case Key::G: return "G";
                case Key::A_FLAT: return "A_FLAT";
                case Key::A: return "A";
                case Key::B_FLAT: return "B_FLAT";
                case Key::B: return "B";
                default: return "UNKNOWN";
            }
        }

        std::string mode_to_string() const {
            return mode_ == Mode::MAJOR ? "MAJOR" : "MINOR";
        }

        tinyxml2::XMLElement* try_get_child( tinyxml2::XMLElement* parent, const char* childName ) {
            return XmlUtils::try_get_child( parent, childName, partName_.c_str() );
        }
};