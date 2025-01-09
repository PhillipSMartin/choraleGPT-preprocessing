#pragma once
#include "TranspositionRule.h"
#include <iostream>
#include <sstream>
#include <string>
#include <tinyxml2.h>
#include <vector>

// An item in a Part's line vector
class Encoding {
    public:
        enum TokenType {
            NOTE,
            MARKER,
            CHORD,
            UNKNOWN
        };

    protected: 
        unsigned int duration_{0}; 
        size_t measureNumber_{0}; // origin 1 (0 for incomplete measure with anacrusis)
        size_t tickNumber_{0}; // origin 1 - position within measure in sub-beats

        TokenType tokenType_{UNKNOWN};
        bool isValid_{true};

        Encoding() = default;
        Encoding( unsigned int duration,
                TokenType tokenType,
                size_t measureNumber = 0,
                size_t subBeatNumber = 0) : 
            duration_{duration}, 
            tokenType_{tokenType} {}

        bool parse_xml( tinyxml2::XMLElement* element );    

    public:
        virtual ~Encoding() = default;
        
        // test type
        bool is_valid() const { return isValid_; }
        bool is_note() const { return tokenType_ == NOTE; }
        bool is_marker() const { return tokenType_ == MARKER; }
        bool is_chord() const { return tokenType_ == CHORD; }
        bool is_SOC() const;
        bool is_EOM() const;
        bool is_EOP() const;
        bool is_EOC() const;

        // getters 
        unsigned int get_duration() const { return duration_; }
        size_t get_measure_number() const { return measureNumber_; }
        size_t get_tick_number() const { return tickNumber_; }

        // setters
        void set_duration( unsigned int duration ) { duration_ = duration; }
        void set_measure_number( size_t measureNumber ) { measureNumber_ = measureNumber; }
        void set_tick_number( size_t tickNumber ) { tickNumber_ = tickNumber; }
        void set_location( size_t measureNumber, size_t tickNumber ) {
            measureNumber_ = measureNumber;
            tickNumber_ = tickNumber;
        }

        virtual std::string to_string() const { return std::to_string( duration_ ); }

        // two markers are equal only if they have the same marker type
        // other encodings are equal if they have the same token type
        bool operator==( const Encoding& other ) const;   
        bool operator!=( const Encoding& other ) const {
            return !(*this == other);
        }       
        friend std::ostream& operator <<( std::ostream& os, const Encoding& enc ) { return os << enc.to_string(); }
};

class Marker : public Encoding {
    public:
        enum MarkerType { 
            SOC, // start of chorale
            EOM, // end of measure
            EOP, // end of phrase
            EOC  // end of chorale
        };
        static inline const std::string SOC_STR = "[SOC]";
        static inline const std::string EOM_STR = "[EOM]";
        static inline const std::string EOP_STR = "[EOP]";
        static inline const std::string EOC_STR = "[EOC]";
        static inline const std::string UNK_STR = "[UNK]";       

    protected:
        MarkerType markerType_;

    public:
        Marker(MarkerType markerType, size_t measureNumber=0, size_t subBeatNumber= 0) : 
            Encoding{0, MARKER, measureNumber, subBeatNumber}, 
            markerType_{markerType} {}
        MarkerType get_marker_type() const { return markerType_; }

        std::string to_string() const override;

        bool operator==(const Marker& other) const {
            return markerType_ == other.markerType_;
        }
};

class Note : public Encoding {
    private:
        char pitch_{'R'};
        unsigned int octave_{0};
        int accidental_{0};
        bool tied_{false}; // tied from previous note

        static inline bool tie_started_{false}; // the next note we save should be marked tied_=true

    public:
        Note() : Encoding{0, NOTE} {}    
        Note(char pitch, unsigned int octave, unsigned int duration, int accidental=0, bool tied=false) : 
            Encoding{duration, NOTE}, 
            pitch_{pitch}, 
            octave_{octave},
            accidental_{accidental},
            tied_{tied} {}

        // Note with no pitch is a rest
        Note(unsigned int duration, size_t measureNumber=0, size_t subBeatNumber=0) : 
            Encoding{duration, NOTE, measureNumber, subBeatNumber} {}

        // Constructor to take xml
        Note(tinyxml2::XMLElement* note, size_t measureNumber=0, size_t subBeatNumber=0) : 
                Encoding{0, NOTE, measureNumber, subBeatNumber} { 
            parse_xml( note ); 
        }

        // Constructor to take encoding in format "pitch.octave.duration"
        Note(const std::string& encoding, size_t measureNumber=0, size_t subBeatNumber=0) : 
                Encoding{0, NOTE, measureNumber, subBeatNumber} { 
            try {
                parse_encoding( encoding );
            }
            catch (std::exception& e) {
                std::cerr << "Error parsing note: " << encoding << std::endl;
                isValid_ = false;
            }       
        }
    

        // getters
        char get_pitch() const { return pitch_; }
        unsigned int get_octave() const { return octave_; }
        int get_accidental() const { return accidental_; }
        bool get_tied() const { return tied_; }

        // setters
        void set_tied( bool tie ) { tied_ = tie; }

        std::string to_string() const override {
            return pitch_to_string() + '.' + Encoding::to_string(); 
        }  
        std::string pitch_to_string() const;

        bool parse_xml( tinyxml2::XMLElement* note ); 
        void transpose( const std::map<char, TranspositionRule>& rules );  

    private:
        // helper functions for Note( encoding ) constructor

        // expects encoding in format "pitch.octave.duration"
        void parse_encoding( const std::string& encoding );
        //  expects format "[+]pitch[half-step alteration]"
        void parse_pitch( const std::string& pitch );
};

class Chord : public Encoding {
    private:
        std::vector<Note> notes_;

    public:
        template<size_t N>
        Chord(const Note (&notes)[N], unsigned int duration) :
            Encoding{duration, CHORD},
            notes_{notes, notes + N} {}

        std::string to_string() const;
};
