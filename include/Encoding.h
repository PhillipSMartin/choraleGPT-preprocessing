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
        TokenType tokenType_{UNKNOWN};
        bool isValid_{true};

        Encoding() = default;
        Encoding( unsigned int duration, TokenType tokenType ) : 
            duration_{duration}, 
            tokenType_{tokenType} {}
        bool parse_xml( tinyxml2::XMLElement* element );    

    public:
        bool is_valid() { return isValid_; }
        bool is_note() { return tokenType_ == NOTE; }
        bool is_marker() { return tokenType_ == MARKER; }
        bool is_chord() { return tokenType_ == CHORD; }

        unsigned int get_duration() const { return duration_; }
        void set_duration( unsigned int duration ) { duration_ = duration; }

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
        Marker(MarkerType markerType) : Encoding{0, MARKER}, markerType_{markerType} {}
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
        bool tie_{false};

    public:
        Note() : Encoding{0, NOTE} {}    
        Note(char pitch, unsigned int octave, unsigned int duration, int accidental=0, bool tie=false) : 
            Encoding{duration, NOTE}, 
            pitch_{pitch}, 
            octave_{octave},
            accidental_{accidental},
            tie_{tie} {}

        // Note with no pitch is a rest
        Note(unsigned int duration) : Encoding{duration, NOTE} {}

        // Constructor to parse xml
        Note(tinyxml2::XMLElement* note) : Encoding{0, NOTE} { parse_xml( note ); }
        // Constructor to parse encoding in format "pitch.octave.duration"
        Note(const std::string& encoding) : Encoding{0, NOTE} {
            try {
                parse_encoding( encoding );
            }
            catch (std::exception& e) {
                std::cerr << "Error parsing note: " << encoding << std::endl;
                isValid_ = false;
            }
}
    

        // Getters
        char get_pitch() const { return pitch_; }
        unsigned int get_octave() const { return octave_; }
        int get_accidental() const { return accidental_; }
        bool get_tie() const { return tie_; }
        void set_tie( bool tie ) { tie_ = tie; }

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

        // Chord(const std::vector<Note>& notes, unsigned int duration) : 
        //     Encoding{duration, CHORD}, 
        //     notes_{notes} {} 
        // Chord(const std::vector<Note>&& notes, unsigned int duration) : 
        //     Encoding{duration, CHORD}, 
        //     notes_{std::move( notes )} {}
        std::string to_string() const;
};
