#pragma once

#include "XmlUtils.h"
#include <map>
#include <string>
#include <tinyxml2.h>
#include <vector>

static inline const int MIN_SUBBEATS = 8;

struct TranspositionRule {
    char newPitch;
    int octaveChange;
    int alterChange;
};

class Part {
    public:
        enum Mode {
            MAJOR,
            MINOR
        };

        // special tokens
        static inline const std::string SOC = "[SOC]";  // start of chorale
        static inline const std::string EOM = "[EOM]";  // end of measure
        static inline const std::string EOP = "[EOP]";  // end of phrase
        static inline const std::string EOC = "[EOC]";  // end of chorale

        // header parameters
        static inline const std::string ID = "[ID: ";  
        static inline const std::string PART = ", PART: ";

        static inline const std::string KEY = ", KEY: ";
        static inline const std::string BEATS = ", BEATS: ";
        static inline const std::string SUB_BEATS = ", SUB-BEATS: ";
        static inline const std::string EOH = "]";

    private:
        static inline const std::string circle_of_fifths_[] = {
           "Gb", "Db", "Ab", "Eb", "Bb", "F", "C", "G", "D", "A", "E", "B", "F#", "C#", "G#", "D#", "A#",
        };
        static int index_of_C() {
            for (int i = 0; i < sizeof(circle_of_fifths_)/sizeof(circle_of_fifths_[0]); i++) {
                if (circle_of_fifths_[i] == "C") 
                   return i;
            }
            return -1; 
        }

        std::string id_;            // identifier for piece this part belongs to, e.g. "BWV 10.1"
        std::string title_;         // title of piece if provided, e.g. "Jesu, meine Freude"
        std::string partName_;      // name of part within the piece, e.g. "Soprano"     

        int beatsPerMeasure_ = 0;
        int subBeats_ = 0;
        int key_ = 0;   // a negative number represents the number of flats in the key signature  
                        // a positive number represents the number of sharps in the key signature
        Mode mode_ = Mode::MAJOR;

        // line_ is a vector of words
        //  SOC: always the first word
        //  <note>: a note word in the format <pitch>.<octave>.<duration>
        //      <pitch> consists of a capital letter (A-G) or 'R' to indicate a rest,
        //          optionally followed by +1, +2, -1, or -2, indicating the number
        //          of half-steps above or below the given note
        //      <octave> is always 0 for a rest
        //      <duration> is a number indicating the number of beats the note or rest is held
        //  EOP: end of phrase (precedes EOM if a phrase ends at a measure)
        //  EOM: end of measure 
        //  EOC: always the last word
        std::vector<std::string> line_;

    public:
        Part(const std::string id, const std::string title, const std::string partName) : id_{id}, title_{title}, partName_{partName} {}

        // parse the MusicXML 'Part' element, set variables accordingly, 
        //  and return true if successful
        // error messages are written to std::cerr if unsuccessful
        bool parse_musicXml( tinyxml2::XMLElement* part );
        bool transpose( int key = 0 ); 

        std::string get_partName() const { return partName_; }
        std::string get_header() const;
        int get_beatsPerMeasure() const { return beatsPerMeasure_; }
        int get_subBeats() const { return subBeats_; }
        int get_key() const { return key_; }
        Mode get_mode() const { return mode_; }
        std::vector<std::string> get_line() const { return line_; }


        std::string key_to_string() const {
            return circle_of_fifths_[key_ + index_of_C() + (mode_ == Mode::MINOR ? 3 : 0)] + "-" + mode_to_string();
        }
        std::string mode_to_string() const {
            return mode_ == Mode::MAJOR ? "Major" : "Minor";
        }
        std::string to_string() const;

        friend std::ostream& operator <<( std::ostream& os, const Part& part );

    private:
        // parse the MusicXML 'attributes' element, set variables accordingly
        bool parse_attributes( tinyxml2::XMLElement* attributes );
        // parse the MusicXML 'measure' element, append words to line_ accordingly
        bool parse_measure( tinyxml2::XMLElement* measure );
        // parse the MusicXML 'note' element, append words to line_ accordingly
        bool parse_note( tinyxml2::XMLElement* note );

        tinyxml2::XMLElement* try_get_child( tinyxml2::XMLElement* parent, const char* childName );

        std::string transpose_up( const std::string& note ) const;
        std::string transpose_down( const std::string& note ) const;
        std::string transpose_note( const std::string& word, const std::map<char, TranspositionRule>& rules ) const;
};