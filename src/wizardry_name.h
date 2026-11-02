#pragma once
#include <algorithm>
#include <string>
#include <string_view>

// GBC Wizardry I/II/III: registration marker, byte length, eight glyphs.
// The four native naming keyboards share this encoding. Kana voicing is
// stored as a precomposed glyph, not a following dakuten byte.
inline std::string wizardryCharacterName(const unsigned char* record){
    static constexpr std::u32string_view glyphs=
        U"�ぇえぉおかがきぎくぐけげこごさ" // 00
        U"ざしじすずせぜそぞただちぢっつづ" // 10
        U" !\"#$%&'()*+,-./" // 20
        U"0123456789:;<=>?" // 30
        U"@ABCDEFGHIJKLMNO" // 40
        U"PQRSTUVWXYZ[\\]^_" // 50
        U"うabcdefghijklmno" // 60
        U"pqrstuvwxyzぁあぃいぅ" // 70
        U"てでとどなにぬねのはばぱひびぴふ" // 80
        U"ぶぷへべぺほぼぽまみむめもゃやゅ" // 90
        U"ゆょよらりるれろわをん��ァアィ" // A0
        U"イゥウェエォオカガキギクグケゲコ" // B0
        U"ゴサザシジスズセゼソゾタダチヂッ" // C0
        U"ツヅテデトドナニヌネノハバパヒビ" // D0
        U"ピフブプヘベペホボポマミムメモャ" // E0
        U"ヤュユョヨラリルレロワヲンヴ゛゜"; // F0
    static_assert(glyphs.size()==256,"Complete GBC Wizardry glyph table required");
    std::string result;
    for(unsigned i=0;i<std::min(unsigned(record[1]),8u);i++){
        char32_t c=glyphs[record[2+i]];
        if(c<128)result.push_back(static_cast<char>(c));
        else if(c<2048){result.push_back(static_cast<char>(0xc0|(c>>6)));result.push_back(static_cast<char>(0x80|(c&63)));}
        else{result.push_back(static_cast<char>(0xe0|(c>>12)));result.push_back(static_cast<char>(0x80|((c>>6)&63)));result.push_back(static_cast<char>(0x80|(c&63)));}
    }
    return result;
}
