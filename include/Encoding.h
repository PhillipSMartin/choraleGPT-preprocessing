#pragma once
#include "TranspositionRule.h"
#include <iostream>
#include <sstream>
#include <string>
#include <tinyxml2.h>
#include <vector>

// An item in a Part's line vector
class Encoding {
    protected: 
        unsigned int duration_{0}; 
        bool isValid_{true};

        Encoding() = default;
        Encoding( unsigned int duration ) : duration_{duration} {}
        bool parse_xml( tinyxml2::XMLElement* element );    

    public:
        bool is_valid() { return isValid_; }
        virtual bool is_note() { return false; }
        unsigned int get_duration() { return duration_; }
        void set_duration( unsigned int duration ) { duration_ = duration; }
        virtual std::string to_string() const { return std::to_string( duration_ ); }
        
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
        Marker(MarkerType markerType) : markerType_{markerType} {}
        MarkerType get_marker_type() { return markerType_; }

       std::string to_string() const override;
};

class Note : public Encoding {
    private:
        char pitch_{'R'};
        unsigned int octave_{0};
        int accidental_{0};
        bool tie_{false};

    public:
        Note() = default;
        Note(char pitch, unsigned int octave, unsigned int duration, int accidental=0, bool tie=false) : 
            Encoding{duration}, 
            pitch_{pitch}, 
            octave_{octave},
            accidental_{accidental},
            tie_{tie} {}

        // Note with no pitch is a rest
        Note(unsigned int duration) : Encoding{duration} {}

        // Constructor to parse xml
        Note(tinyxml2::XMLElement* note) { parse_xml( note ); }
        // Constructor to parse encoding in format "pitch.octave.duration"
        Note(const std::string& encoding);

        bool is_note() override { return true; }

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
        Chord(const std::vector<Note>& notes, unsigned int duration) : 
            Encoding{duration}, 
            notes_{notes} {} 
        Chord(const std::vector<Note>&& notes, unsigned int duration) : 
            Encoding{duration}, 
            notes_{std::move( notes )} {}
        std::string to_string() const;
};