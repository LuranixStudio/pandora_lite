#pragma once
// Navigation adapted from Michael Conors' 1011 custom.cpp / main.cpp,
// supplied in KingsleydotDev/ImGui-Menus. Ported to public ImGui 1.91 APIs.
#include "menu_fonts.hpp"
namespace menu1011 {
inline ImFont* icons=nullptr;
inline ImFont* title=nullptr;
inline void fonts(){
 auto& io=ImGui::GetIO();ImFontConfig config;config.FontDataOwnedByAtlas=false;
 io.Fonts->AddFontFromMemoryTTF(font_binary,sizeof font_binary,15.f,&config);
 icons=io.Fonts->AddFontFromMemoryTTF(icons_binary,sizeof icons_binary,17.f,&config);
 title=io.Fonts->AddFontFromMemoryTTF(font_bold_binary,sizeof font_bold_binary,21.f,&config);
}
inline bool tab(const char* icon,const char* text,bool selected,float expansion,ImU32 accent){
 ImVec2 p=ImGui::GetCursorScreenPos();float w=ImGui::GetContentRegionAvail().x;
 bool pressed=ImGui::InvisibleButton(text,{w,30});auto* d=ImGui::GetWindowDrawList();
 if(ImGui::IsItemHovered())d->AddRectFilled(p,{p.x+w,p.y+30},IM_COL32(40,44,41,180),3);
 if(selected)d->AddRectFilled({p.x+w-3,p.y+2},{p.x+w,p.y+28},accent,2);
 auto col=selected?IM_COL32(245,248,245,255):IM_COL32(110,117,111,255);
 if(icons)d->AddText(icons,17,{p.x+12,p.y+6},col,icon);
 if(expansion>.25f)d->AddText({p.x+42,p.y+7},col,text);
 if(ImGui::IsItemHovered()&&expansion<.25f)ImGui::SetTooltip("%s",text);
 return pressed;
}
}
