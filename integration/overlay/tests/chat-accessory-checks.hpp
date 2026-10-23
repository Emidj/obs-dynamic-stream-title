// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "chat/emote-cache.hpp"
#include <stdexcept>
namespace oto::testing {
inline void checkChatAccessories(){
 const auto entries=chat::parseCatalog(2,R"json({"default_sets":[1],"sets":{"1":{"emoticons":[
 {"id":59847,"name":"Hat","modifier":true,"urls":{"2":"https://cdn.frankerfacez.com/emote/59847/2"}},
 {"id":1,"name":"Custom","modifier":true,"modifier_offset":"4px 1px 0 3px","urls":{"2":"https://cdn.frankerfacez.com/emote/1/2"}},
 {"id":2,"name":"Bad","modifier":true,"modifier_offset":"-999px url(evil)","urls":{"2":"https://cdn.frankerfacez.com/emote/2/2"}},
 {"id":3,"name":"Hidden","modifier":true,"modifier_flags":1,"urls":{"2":"https://cdn.frankerfacez.com/emote/3/2"}}
 ]}}})json",false);
 if(entries.size()!=4 || !entries[0].zeroWidth || entries[0].layerRect!=QRectF(0,0,.625,.625) || entries[1].layerRect!=QRectF(.075,.1,.9,.9) || entries[2].layerRect!=QRectF(0,0,1,1) || entries[3].zeroWidth)throw std::runtime_error("FFZ accessory metadata/offset validation");
 chat::Token base{"Base","https://cdn.frankerfacez.com/emote/4/2"},hat{"Hat",entries[0].url,true,-1,0,false,false,entries[0].layerRect};
 const auto layered=chat::composeTokens({base,{" ",{}},hat});
 if(layered.size()!=2 || layered[1].layerOf!=0 || layered[1].layerRect!=hat.layerRect)throw std::runtime_error("FFZ accessory not attached");
 const auto orphan=chat::composeTokens({hat});if(orphan[0].layerOf!=-1 || orphan[0].layerRect!=QRectF(0,0,1,1))throw std::runtime_error("FFZ orphan not reset");
}
}
