#include "wizardry_name.h"
#include "game_support.h"
#include <iostream>
void cartridge_set_rumble(unsigned){}
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
std::string utf8(char32_t c){std::string s;if(c<128)s+=char(c);else if(c<2048){s+=char(0xc0|(c>>6));s+=char(0x80|(c&63));}else{s+=char(0xe0|(c>>12));s+=char(0x80|((c>>6)&63));s+=char(0x80|(c&63));}return s;}
int wmain(int argc,wchar_t** argv)try{
 require(argc==2,"ROM required");auto rom=read(argv[1]);auto id=identifyGame(rom).id;unsigned offset=id==GameId::Wizardry2?0x20dd2:id==GameId::Wizardry3?0x20dea:0x20d5c;
 const std::u32string keyboards[]={
  U"あいうえお かきくけこ さしすせそたちつてと なにぬねの はひふへほまみむめも や ゆ よ らりるれろわをんぁぃぅぇぉっゃゅょ-゛゜�",
  U"アイウエオ カキクケコ サシスセソタチツテト ナニヌネノ ハヒフヘホマミムメモ ヤ ユ ヨ ラリルレロワヲンァィゥェォッャュョ-゛゜�",
  U"0123456789 ^\"',._ABCDEFGHIJ !?#$%&KLMNOPQRST =+-*/\\UVWXYZ @()<>[] �",
  U"0123456789 ^\"',._abcdefghij !?#$%&klmnopqrst =+-*/\\uvwxyz @()<>[] �"};
 unsigned total=0;
 for(unsigned page=0;page<4;page++){
  require(keyboards[page].size()==67,"Keyboard expectations wrong");unsigned end=offset+(page<2?0x236:0x233),count=0;
  for(unsigned i=offset;i+3<end;i++)if(rom[i]==0&&rom[i+1]==0&&static_cast<unsigned char>(rom[i+2])==0x10){
   unsigned char rec[10]={1,1,static_cast<unsigned char>(rom[i+3])};require(count<67,"Too many keys");
   if(wizardryCharacterName(rec)!=utf8(keyboards[page][count])){std::cerr<<"page="<<page<<" key="<<count<<" code="<<unsigned(rec[2])<<"\n";require(false,"Keyboard glyph differs");}++count;++total;
  }
  require(count==67,"Keyboard layout not recognized");offset=end;
 }
 const unsigned char voiced[]={1,8,6,0x10,0x1a,0x8a,0x8b,0xb8,0xdd,0xfd};require(wizardryCharacterName(voiced)==u8"がざだばぱガパヴ","Voiced kana wrong");
 const unsigned char small[]={1,8,0x7b,0x7d,0x7f,1,3,0x1d,0xad,0xaf};require(wizardryCharacterName(small)==u8"ぁぃぅぇぉっァィ","Small kana wrong");
 const unsigned char mixed[]={1,8,0xae,'A',0x7c,'1',' ',0xdb,0xdd,'!'};require(wizardryCharacterName(mixed)==u8"アAあ1 ハパ!","Mixed name wrong");
 unsigned char length[]={1,1,'A','B','C','D','E','F','G','H'};require(wizardryCharacterName(length)=="A","Trailing bytes displayed");length[1]=0;require(wizardryCharacterName(length).empty(),"Empty name wrong");length[1]=255;require(wizardryCharacterName(length)=="ABCDEFGH","Corrupt length not bounded");
 const unsigned char unknown[]={1,2,0xab,0xac};require(wizardryCharacterName(unknown)==u8"��","Unknown glyph handling wrong");
 std::cout<<"PASS: "<<total<<" native keyboard glyphs, voicing, small kana, mixed names, length bounds and unknown glyphs\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}
