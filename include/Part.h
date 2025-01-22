#pragma once

#include "Arguments.h"
#include "Encoding.h"
#include "XmlUtils.h"

#include <map>
#include <memory>
#include <string>
#include <tinyxml2.h>
#include <vector>

struct PartPrintOptions {
    bool printHeader = true;
    bool printEOM = true;
    bool printEndTokensAsPeriod = false;
    bool consolidateBeat = false;
    bool printOnlyStartingTokenforEachBeat = false;

    PartPrintOptions() = default;
    PartPrintOptions( const Arguments& args ) :
        printHeader{!args.noHeader()},
        printEOM{!args.noEOM()},
        printEndTokensAsPeriod{args.endTokens()},
        consolidateBeat{args.consolidateBeat()},
        printOnlyStartingTokenforEachBeat{args.startingTokensOnly()} {}
};


// class containing info about a single voice part
class Part {
    public:
        enum Mode {
            MAJOR,
            MINOR
        };

        // parameters for the header of the encoding
        static inline const std::string SOH = "[";
        static inline const std::string ID = "ID: ";  
        static inline const std::string PART = "PART: ";
        static inline const std::string KEY = "KEY: ";
        static inline const std::string BEATS = "BEATS: ";
        static inline const std::string SUB_BEATS = "SUB-BEATS: ";
        static inline const std::string EOH = "]";
        static inline const std::string DELIM = ", ";
        static inline const std::string MAJOR_STR = "Major";
        static inline const std::string MINOR_STR = "Minor";

    protected:
        // for converting key_ to a displayable string
        static constexpr const char* circle_of_fifths_[] = {
           "Gb", "Db", "Ab", "Eb", "Bb", "F", "C", "G", "D", "A", "E", "B", "F#", "C#", "G#", "D#", "A#",
        };
        // the index of C in the above circle of fifths array (evaluated at compile time)
        static consteval size_t index_of_C() {
            for (size_t i = 0; i < sizeof(circle_of_fifths_)/sizeof(circle_of_fifths_[0]); i++) {
                if (circle_of_fifths_[i] == "C") 
                   return i;
            }
            return static_cast<size_t>( -1 ); 
        }

        std::string id_;            // identifier for piece this part belongs to, e.g. "BWV 10.1"
        std::string title_;         // title of piece if provided, e.g. "Jesu, meine Freude"
        std::string partName_;      // name of part within the piece, e.g. "Soprano"     

        // if we are in 4/4 time, beatsPerMeasure_ is 4
        // subBeatsPerBeat_ represents the granularity
        // if the shortest note in 4/4 time is an eighth note, subBeatsPerBeat_ is 2
        size_t beatsPerMeasure_{1};   
        size_t subBeatsPerBeat_{1};  

        // a negative number for key_ represents the number of flats in the key signature  
        // a positive number represents the number of sharps in the key signature
        // we normalize all parts so that key_ is 0
        int key_ = 0;   
        Mode mode_ = Mode::MAJOR;


        // encodings_ is a vector of words
        //  SOC: always the first word
        //  a note is presented in the format <pitch>.<octave>.<duration>
        //      <pitch> consists of a capital letter (A-G) or 'R' to indicate a rest,
        //            optionally followed by 1, 2, -1, or -2, indicating the number
        //            of half-steps above or below the given note
        //          if the pitch is tied from the previous beat, the token starts with a '+'
        //      <octave> is always 0 for a rest
        //      <duration> is a number indicating the number of subbeats the note or rest is held
        //  EOM: end of measure - final EOM omitted if measure is incomplete
        //  EOP: end of phrase (precedes EOM if a phrase ends at a measure)
        //  EOC: always the last word
        std::vector<std::unique_ptr<Encoding>> encodings_;

        // next position in encodings_ (origin 1)
        size_t currentMeasure_{1};  // incremented when an EOM is added
        size_t nextTick_{1}; // incremented when a note or chord is added
        size_t tick_to_beat( size_t tick ) const {
            return (tick - 1) / subBeatsPerBeat_ + 1;
        }
        size_t tick_to_sub_beat( size_t tick ) const {
            return (tick - 1) % subBeatsPerBeat_ + 1;
        }

        int ticks_remaining() const; // returns number of ticks left in current measure
        void handle_upbeat(); // adjust ticks for incomplete first measure

    public:
        Part() = default;
        virtual ~Part() = default;

        Part(const std::string& id, const std::string& title, const std::string& partName) : id_{id}, title_{title}, partName_{partName} {}
        Part(const Part& other) {
            *this = other;
        }
        Part(Part&& other) noexcept = default;

        Part& operator=(const Part& other) {
            if (this != &other) {
                id_ = other.id_;
                title_ = other.title_;
                partName_ = other.partName_;
                beatsPerMeasure_ = other.beatsPerMeasure_;
                subBeatsPerBeat_ = other.subBeatsPerBeat_;
                key_ = other.key_;
                mode_ = other.mode_;
                currentMeasure_ = other.currentMeasure_;
                nextTick_ = other.nextTick_;
                
                encodings_.clear();
                for (const auto& encoding : other.encodings_) {
                    encodings_.push_back(encoding->clone());
                }
            }
            return *this;
        }

        Part& operator=(Part&& other) noexcept = default;

        // these methods print an error to cerr and return false if they faile

        // parse the MusicXML 'Part' element  
        bool parse_xml( tinyxml2::XMLElement* part );
        // parse the encoding (performed on the musicXml in a previous run) 
        bool parse_encoding( const std::string& part );
        // transpose part to the key with given number of sharps (if plus) or flats (if minus)
        bool transpose( int key = 0 );       

        // getters
        std::string get_id() const { return id_; }
        std::string get_title() const { return title_; }
        std::string get_part_name() const { return partName_; }
        std::string get_header() const;
        int get_beats_per_measure() const { return beatsPerMeasure_; }
        int get_sub_beats() const { return subBeatsPerBeat_; }
        int get_key() const { return key_; }
        Mode get_mode() const { return mode_; }

        // setters
        void set_beats_per_measure( size_t beatsPerMeasure ) {
            beatsPerMeasure_ = beatsPerMeasure;
        }
        void set_sub_beats( size_t subBeats );

        // access encodings_
        std::unique_ptr<Encoding> pop_encoding();
        void push_encoding( std::unique_ptr<Encoding>& encoding );
        std::unique_ptr<Encoding>& get_last_encoding() {
            return encodings_.back();
        }

        // conversions to facillitate printing
        std::string key_to_string() const {
            return static_cast<std::string>( circle_of_fifths_[key_ + index_of_C() + (mode_ == Mode::MINOR ? 3 : 0)] ) 
                + "-" 
                + mode_to_string();
        }
        std::string mode_to_string() const {
            return (mode_ == Mode::MAJOR) ? MAJOR_STR : MINOR_STR;
        }
        std::string location_to_string( const Encoding* encoding ) const;
        std::string to_string() const;
        std::string to_string( const PartPrintOptions& options ) const;

        friend std::ostream& operator <<( std::ostream& os, const Part& part );

    private:
        // helper functions for parse_xml()

        // parse the MusicXML 'attributes' element, set variables accordingly
        bool parse_attributes( tinyxml2::XMLElement* attributes );
        // parse the MusicXML 'measure' element, append words to encodings_ accordingly
        bool parse_measure( tinyxml2::XMLElement* measure );

        // returns the specified child of a given XML element or nullptr
        tinyxml2::XMLElement* try_get_child( tinyxml2::XMLElement* parent, const char* childName, bool verbose = true );

        // helper functions for parse_encoding()

        // save info from header
        bool import_header( const std::string& header );
        std::string find_header_value( const std::string& header, const std::string& key ) const;
        bool import_encodings( const std::string& line );
        std::unique_ptr<Encoding> make_encoding( const std::string& encoding ) const;
        bool import_key( const std::string& keyString );

        // helper functions for transpose()

        // transpose to a key one level higher in the circle of fifths
        void transpose_up( Note& note, bool byFifth ) {
            if (byFifth) {
                note.transpose( transposeUpAFifthRules );
            }
            else { 
                note.transpose( transposeDownAFourthRules );
            }
        }
        // transpose to a key one level lower in the circle of fifths
        void transpose_down( Note& note, bool byFifth ) {
            if (byFifth) {
                note.transpose( transposeDownAFifthRules );
            }
            else {
                note.transpose( transposeUpAFourthRules );
            }
        }

        // helper functions for to_string()
        void print_header( std::ostream& os, const PartPrintOptions& opts ) const;
        void print_marker( std::ostream& os, const PartPrintOptions& opts, const Encoding& marker ) const;
        void print_note( std::ostream& os, const PartPrintOptions& opts, const Encoding& note, 
            bool startsBeat, bool endsBeat ) const;

};