#ifndef Header_TransformCharacters
#define Header_TransformCharacters

using std::map;
using std::pair;

using CharMap = map<pair<char,char>,int>;


const CharMap characters {

    //  Umlaut

    { { '"' , 'a' } , 0xE4 },
    { { '"' , 'e' } , 0xEB },
    { { '"' , 'i' } , 0xEF },
    { { '"' , 'o' } , 0xF6 },
    { { '"' , 'u' } , 0xFC },
    { { '"' , 'A' } , 0xC4 },
    { { '"' , 'E' } , 0xCB },
    { { '"' , 'I' } , 0xCF },
    { { '"' , 'O' } , 0xD6 },
    { { '"' , 'U' } , 0xDC },
    { { '"' , 's' } , 0xDF },

    //  Grave

    { { '`' , 'a' } , 0xE0 },
    { { '`' , 'e' } , 0xE8 },
    { { '`' , 'i' } , 0xEC },
    { { '`' , 'o' } , 0xF2 },
    { { '`' , 'u' } , 0xF9 },
    { { '`' , 'A' } , 0xC0 },
    { { '`' , 'E' } , 0xC8 },
    { { '`' , 'I' } , 0xCC },
    { { '`' , 'O' } , 0xD2 },
    { { '`' , 'U' } , 0xD9 },

    //  Acute

    { { '\'' , 'a' } , 0xE1 },
    { { '\'' , 'e' } , 0xE9 },
    { { '\'' , 'i' } , 0xED },
    { { '\'' , 'o' } , 0xF3 },
    { { '\'' , 'u' } , 0xFA },
    { { '\'' , 'y' } , 0xFD },
    { { '\'' , 'A' } , 0xC1 },
    { { '\'' , 'E' } , 0xC9 },
    { { '\'' , 'I' } , 0xCD },
    { { '\'' , 'O' } , 0xD3 },
    { { '\'' , 'U' } , 0xDA },
    { { '\'' , 'Y' } , 0xDD },

    //  Circumflex

    { { '^' , 'a' } , 0xE2 },
    { { '^' , 'e' } , 0xEA },
    { { '^' , 'i' } , 0xEE },
    { { '^' , 'o' } , 0xF4 },
    { { '^' , 'u' } , 0xFB },
    { { '^' , 'A' } , 0xC2 },
    { { '^' , 'E' } , 0xCA },
    { { '^' , 'I' } , 0xCE },
    { { '^' , 'O' } , 0xD4 },
    { { '^' , 'U' } , 0xDB },

    //  Tilde

    { { '~' , 'a' } , 0xE3 },
    { { '~' , 'n' } , 0xF1 },
    { { '~' , 'o' } , 0xF5 },
    { { '~' , 'A' } , 0xC3 },
    { { '~' , 'N' } , 0xD1 },
    { { '~' , 'O' } , 0xD5 },

    //  Cedille

    { { 'c' , 'c' } , 0xE7 },
    { { 'c' , 'C' } , 0xC7 }
};

#endif
