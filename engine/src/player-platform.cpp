/* Copyright (C) 2003-2013 Runtime Revolution Ltd.
 
 This file is part of LiveCode.
 
 LiveCode is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License v3 as published by the Free
 Software Foundation.
 
 LiveCode is distributed in the hope that it will be useful, but WITHOUT ANY
 WARRANTY; without even the implied warranty of MERCHANTABILITY or
 FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 for more details.
 
 You should have received a copy of the GNU General Public License
 along with LiveCode.  If not see <http://www.gnu.org/licenses/>.  */


#include "prefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "mcio.h"
#include "core.h"

#include "execpt.h"
#include "util.h"
#include "font.h"
#include "sellst.h"
#include "stack.h"
#include "stacklst.h"
#include "card.h"
#include "field.h"
#include "player-platform.h"
#include "aclip.h"
#include "mcerror.h"
#include "param.h"
#include "globals.h"
#include "mode.h"
#include "context.h"
#include "osspec.h"
#include "redraw.h"
#include "gradient.h"
#include "dispatch.h"

#include "graphics_util.h"

//////////////////////////////////////////////////////////////////////

//// PLATFORM PLAYER

#include "platform.h"

static MCPlatformPlayerMediaType ppmediatypes[] =
{
	kMCPlatformPlayerMediaTypeVideo,
	kMCPlatformPlayerMediaTypeAudio,
	kMCPlatformPlayerMediaTypeText,
	kMCPlatformPlayerMediaTypeQTVR,
	kMCPlatformPlayerMediaTypeSprite,
	kMCPlatformPlayerMediaTypeFlash,
};

static const char *ppmediastrings[] =
{
	"video",
	"audio",
	"text",
	"qtvr",
	"sprite",
	"flash"
};

#define CONTROLLER_HEIGHT 26
#define SELECTION_RECT_WIDTH CONTROLLER_HEIGHT / 2
// PM-2014-07-17: [[ Bug 12835 ]] Adjustments to prevent selectedArea and playedArea to be drawn without taking into account the width of the well
#define PLAYER_MIN_WIDTH 5 * CONTROLLER_HEIGHT + 2 * SELECTION_RECT_WIDTH
#define LIGHTGRAY 1
#define PURPLE 2
#define SOMEGRAY 3
#define DARKGRAY 4
#define MIN_RATE -3
#define MAX_RATE 3




static MCColor controllercolors[] = {
    {0, 0x8000, 0x8000, 0x8000, 0, 0},         /* 50% gray */
    
    {0, 0xCCCC, 0xCCCC, 0xCCCC, 0, 0},         /* 20% gray -- 80% white */
    
    {0, 0xa8a8, 0x0101, 0xffff, 0, 0},         /* Purple */
    
    //{0, 0xcccc, 0x9999, 0xffff, 0, 0},         /* Magenda */
    
    {0, 0x2b2b, 0x2b2b, 0x2b2b, 0, 0},         /* gray */
    
    {0, 0x2222, 0x2222, 0x2222, 0, 0},         /* dark gray */
    
    
};

inline MCGColor MCGColorMakeRGBA(MCGFloat p_red, MCGFloat p_green, MCGFloat p_blue, MCGFloat p_alpha)
{
	return ((uint8_t)(p_red * 255) << 16) | ((uint8_t)(p_green * 255) << 8) | ((uint8_t)(p_blue * 255) << 0) | ((uint8_t)(p_alpha * 255) << 24);
}

inline void MCGraphicsContextAngleAndDistanceToXYOffset(int p_angle, int p_distance, MCGFloat &r_x_offset, MCGFloat &r_y_offset)
{
	r_x_offset = floor(0.5f + p_distance * cos(p_angle * M_PI / 180.0));
	r_y_offset = floor(0.5f + p_distance * sin(p_angle * M_PI / 180.0));
}

inline void setRamp(MCGColor *&r_colors, MCGFloat *&r_stops)
{
    if (r_colors == nil)
    /* UNCHECKED */ MCMemoryNewArray(3, r_colors);
    r_colors[0] = MCGColorMakeRGBA(183 / 255.0, 183 / 255.0, 183 / 255.0, 1.0f);
    r_colors[1] = MCGColorMakeRGBA(1.0f, 1.0f, 1.0f, 1.0f);
    r_colors[2] = MCGColorMakeRGBA(183 / 255.0, 183 / 255.0, 183 / 255.0, 1.0f);
    
    if (r_stops == nil)
    /* UNCHECKED */ MCMemoryNewArray(3, r_stops);
    r_stops[0] = (MCGFloat)0.00000;
    r_stops[1] = (MCGFloat)0.50000;
    r_stops[2] = (MCGFloat)1.00000;
}

inline void setTransform(MCGAffineTransform &r_transform, MCGFloat origin_x, MCGFloat origin_y, MCGFloat primary_x, MCGFloat primary_y, MCGFloat secondary_x, MCGFloat secondary_y)
{
    MCGAffineTransform t_transform;
    t_transform . a = primary_x - origin_x;
    t_transform . b = primary_y - origin_y;
    t_transform . c = secondary_x - origin_x;
    t_transform . d = secondary_y - origin_y;
    
    t_transform . tx = origin_x;
    t_transform . ty = origin_y;
    r_transform = t_transform;
}

inline MCGPoint MCRectangleScalePoints(MCRectangle p_rect, MCGFloat p_x, MCGFloat p_y)
{
    return MCGPointMake(p_rect . x + p_x * p_rect . width, p_rect . y + p_y * p_rect . height);
}

inline uint32_t _muludiv64(uint32_t p_multiplier, uint32_t p_numerator, uint32_t p_denominator)
{
    return (uint32_t)((((uint64_t)p_multiplier) * p_numerator) / p_denominator);
}

//////////////////////////////////////////////////////////////////////

class MCPlayerVolumePopup: public MCStack
{
public:
    MCPlayerVolumePopup(void)
    {
        setname_cstring("Player Volume");
        state |= CS_NO_MESSAGES;
        
        cards = NULL;
        cards = MCtemplatecard->clone(False, False);
        cards->setparent(this);
        cards->setstate(True, CS_NO_MESSAGES);
        
        parent = nil;
        
        m_font = nil;
        
        m_player = nil;
    }
    
    ~MCPlayerVolumePopup(void)
    {
    }
    
    // This will be called when the stack is closed, either directly
    // or indirectly if the popup is cancelled by clicking outside
    // or pressing escape.
    void close(void)
    {
        MCStack::close();
        MCdispatcher -> removemenu();
        if (m_player != nil)
            m_player -> popup_closed();
    }
    
    // This is called to render the stack.
    void render(MCContext *dc, const MCRectangle& dirty)
    {
        // draw the volume control content here.
        MCGContextRef t_gcontext = nil;
        dc -> lockgcontext(t_gcontext);
        
        drawControllerVolumeBarButton(t_gcontext, dirty);
        drawControllerVolumeWellButton(t_gcontext, dirty);
        drawControllerVolumeAreaButton(t_gcontext, dirty);
        drawControllerVolumeSelectorButton(t_gcontext, dirty);
        dc -> unlockgcontext(t_gcontext);
    }
    void drawControllerVolumeBarButton(MCGContextRef p_gcontext, const MCRectangle& dirty)
    {
        MCRectangle t_volume_bar_rect = dirty;
        MCGContextAddRectangle(p_gcontext, MCRectangleToMCGRectangle(t_volume_bar_rect));
        MCGContextSetFillRGBAColor(p_gcontext, (m_player -> getcontrollerbackcolor() . red / 255.0) / 257.0, (m_player -> getcontrollerbackcolor() . green / 255.0) / 257.0, (m_player -> getcontrollerbackcolor() . blue / 255.0) / 257.0, 1.0f);
        MCGContextFill(p_gcontext);
    }
    
    void drawControllerVolumeWellButton(MCGContextRef p_gcontext, const MCRectangle& dirty)
    {
        MCRectangle t_volume_bar_rect = dirty;
        MCRectangle t_volume_well;
        t_volume_well = getVolumeBarPartRect(dirty, kMCPlayerControllerPartVolumeWell);
        
        MCGBitmapEffects t_effects;
        t_effects . has_drop_shadow = false;
        t_effects . has_outer_glow = false;
        t_effects . has_inner_glow = false;
        t_effects . has_inner_shadow = true;
        t_effects . has_color_overlay = false;
        
        MCGShadowEffect t_inner_shadow;
        t_inner_shadow . color = MCGColorMakeRGBA(0.0f, 0.0f, 0.0f, 56.0 / 255.0);
        t_inner_shadow . blend_mode = kMCGBlendModeClear;
        t_inner_shadow . size = 0;
        t_inner_shadow . spread = 0;
        
        MCGFloat t_x_offset, t_y_offset;
        int t_distance = t_volume_well . width / 5;
        // Make sure we always have an inner shadow
        if (t_distance == 0)
            t_distance = 1;
        
        MCGraphicsContextAngleAndDistanceToXYOffset(235, t_distance, t_x_offset, t_y_offset);
        t_inner_shadow . x_offset = t_x_offset;
        t_inner_shadow . y_offset = t_y_offset;
        t_inner_shadow . knockout = false;
        
        t_effects . inner_shadow = t_inner_shadow;
        
        MCGContextSetFillRGBAColor(p_gcontext, 0.0f, 0.0f, 0.0f, 1.0f);
        MCGContextSetShouldAntialias(p_gcontext, true);
        
        MCGRectangle t_rounded_rect = MCRectangleToMCGRectangle(t_volume_well);
        
        MCGContextAddRoundedRectangle(p_gcontext, t_rounded_rect, MCGSizeMake(30, 30));
        MCGContextBeginWithEffects(p_gcontext, t_rounded_rect, t_effects);
        MCGContextFill(p_gcontext);
        
        /////////////////////////////////////////
        /* TODO: Update this ugly way of adding the inner shadow 'manually' */
        MCRectangle t_shadow = MCRectangleMake(t_volume_well . x + t_volume_well . width - 2, t_volume_well . y + 1, 2, t_volume_well . height - 2);
        
        MCGContextSetShouldAntialias(p_gcontext, true);
        
        MCGContextSetFillRGBAColor(p_gcontext, 56.0 / 255.0, 56.0 / 255.0, 56.0 / 255.0, 1.0f); // GRAY
        MCGRectangle t_shadow_rect = MCRectangleToMCGRectangle(t_shadow);
        
        MCGContextAddRoundedRectangle(p_gcontext, t_shadow_rect, MCGSizeMake(10, 10));
        
        MCGContextFill(p_gcontext);
        ////////////////////////////////////////
        
        MCGContextEnd(p_gcontext);
    }
    
    void drawControllerVolumeAreaButton(MCGContextRef p_gcontext, const MCRectangle& dirty)
    {
        MCRectangle t_volume_area;
        t_volume_area = getVolumeBarPartRect(dirty, kMCPlayerControllerPartVolumeArea);
        // Adjust to look prettier
        t_volume_area . x ++;
        t_volume_area . width -= 2;
        
        MCGContextSetFillRGBAColor(p_gcontext, (m_player -> getcontrollermaincolor() . red / 255.0) / 257.0, (m_player -> getcontrollermaincolor() . green / 255.0) / 257.0, (m_player -> getcontrollermaincolor() . blue / 255.0) / 257.0, 1.0f);
        
        MCGRectangle t_grect = MCRectangleToMCGRectangle(t_volume_area);
        MCGContextAddRoundedRectangle(p_gcontext, t_grect, MCGSizeMake(30, 30));
        MCGContextFill(p_gcontext);
    }
    
    void drawControllerVolumeSelectorButton(MCGContextRef p_gcontext, const MCRectangle& dirty)
    {
        MCRectangle t_volume_selector_rect = getVolumeBarPartRect(dirty, kMCPlayerControllerPartVolumeSelector);
        
        MCAutoPointer<MCGColor> t_colors;
        MCAutoPointer<MCGFloat> t_stops;
        setRamp(&t_colors, &t_stops);
        
        MCGAffineTransform t_transform;
        float origin_x = t_volume_selector_rect.x + t_volume_selector_rect.width / 2.0;
        float origin_y = t_volume_selector_rect.y + t_volume_selector_rect.height;
        float primary_x = t_volume_selector_rect.x + t_volume_selector_rect.width / 2.0;
        float primary_y = t_volume_selector_rect.y;
        float secondary_x = t_volume_selector_rect.x - t_volume_selector_rect.width / 2.0;
        float secondary_y = t_volume_selector_rect.y + t_volume_selector_rect.height;
        
        setTransform(t_transform, origin_x, origin_y, primary_x, primary_y, secondary_x, secondary_y);
        
        MCGContextSetFillGradient(p_gcontext, kMCGGradientFunctionLinear, *t_stops, *t_colors, 3, false, false, 1, t_transform, kMCGImageFilterNone);
        
        MCGContextSetShouldAntialias(p_gcontext, true);
        MCGContextAddArc(p_gcontext, MCRectangleScalePoints(t_volume_selector_rect, 0.5, 0.5), MCGSizeMake(0.7 * t_volume_selector_rect . width, 0.7 * t_volume_selector_rect . height), 0, 0, 360);
        
        MCGContextFill(p_gcontext);
    }
    
    
    MCRectangle getVolumeBarPartRect(const MCRectangle& p_volume_bar_rect, int p_part)
    {
        switch (p_part)
        {
                
            case kMCPlayerControllerPartVolumeWell:
            {
                int32_t t_width = CONTROLLER_HEIGHT / 5;
                
                int32_t t_x_offset = (p_volume_bar_rect . width - t_width) / 2;
                
                return MCRectangleMake(p_volume_bar_rect . x + t_x_offset, p_volume_bar_rect . y + t_x_offset, t_width, p_volume_bar_rect . height - 2 * t_x_offset);
            }
                break;
            case kMCPlayerControllerPartVolumeArea:
            {
                MCRectangle t_volume_well_rect = getVolumeBarPartRect(p_volume_bar_rect, kMCPlayerControllerPartVolumeWell);
                MCRectangle t_volume_selector_rect = getVolumeBarPartRect(p_volume_bar_rect, kMCPlayerControllerPartVolumeSelector);
                int32_t t_bar_height = t_volume_well_rect . height;
                int32_t t_bar_width = t_volume_well_rect . width;
                
                // Adjust y by 2 pixels
                return MCRectangleMake(t_volume_well_rect. x , t_volume_selector_rect . y + 2 , t_bar_width, t_volume_well_rect . y + t_volume_well_rect . height - t_volume_selector_rect . y - 4);
            }
                break;
                
            case kMCPlayerControllerPartVolumeSelector:
            {
                MCRectangle t_volume_well_rect = getVolumeBarPartRect(p_volume_bar_rect, kMCPlayerControllerPartVolumeWell);
                
                // The width and height of the volumeselector are p_volume_bar_rect . width / 2
                int32_t t_actual_height = t_volume_well_rect . height - p_volume_bar_rect . width / 2;
                
                int32_t t_x_offset = p_volume_bar_rect . width / 4;
                
                return MCRectangleMake(p_volume_bar_rect . x + t_x_offset , t_volume_well_rect . y + t_volume_well_rect . height - t_actual_height * m_player -> getloudness() / 100 - p_volume_bar_rect . width / 2, p_volume_bar_rect . width / 2, p_volume_bar_rect . width / 2 );
            }
                break;
        }
        
        return MCRectangleMake(0, 0, 0, 0);
    }
    
    int getvolumerectpart(int x, int y)
    {
    
        if (MCU_point_in_rect(getVolumeBarPartRect(m_volume_rect, kMCPlayerControllerPartVolumeSelector), x, y))
            return kMCPlayerControllerPartVolumeSelector;
        
        else if (MCU_point_in_rect(getVolumeBarPartRect(m_volume_rect, kMCPlayerControllerPartVolumeWell), x, y))
            return kMCPlayerControllerPartVolumeWell;
        
        else if (MCU_point_in_rect(m_volume_rect, x, y))
            return kMCPlayerControllerPartVolumeBar;
        
        else
            return kMCPlayerControllerPartUnknown;
    }

    // Mouse handling methods are similar to the control ones.
    
    Boolean mdown(uint2 which)
    {
        int t_part;
        t_part = getvolumerectpart(MCmousex, MCmousey);
        
        switch(t_part)
        {
            case kMCPlayerControllerPartVolumeSelector:
            {
                m_grabbed_part = t_part;
            }
                break;
                
            case kMCPlayerControllerPartVolumeWell:
            case kMCPlayerControllerPartVolumeBar:
            {
                MCRectangle t_part_volume_selector_rect = getVolumeBarPartRect(m_volume_rect, kMCPlayerControllerPartVolumeSelector);
                MCRectangle t_volume_well;
                t_volume_well = getVolumeBarPartRect(m_volume_rect, kMCPlayerControllerPartVolumeWell);
                int32_t t_new_volume, t_height;
                
                t_height = t_volume_well . height;
                t_new_volume = (t_volume_well . y + t_volume_well . height - MCmousey) * 100 / (t_height);
                
                if (t_new_volume < 0)
                    t_new_volume = 0;
                if (t_new_volume > 100)
                    t_new_volume = 100;
                
                m_player -> updateloudness(t_new_volume);
                m_player -> setloudness();
                m_player -> layer_redrawall();
                dirtyall();
            }
                break;
                
            default:
                break;
        }
        return True;
    }
    
    Boolean mup(uint2 which, bool release)
    {
        m_grabbed_part = kMCPlayerControllerPartUnknown;
        return True;
    }
    
    Boolean mfocus(int2 x, int2 y)
    {
        MCmousex = x;
        MCmousey = y;
        switch(m_grabbed_part)
        {
            case kMCPlayerControllerPartVolumeSelector:
            {
                MCRectangle t_part_volume_selector_rect = getVolumeBarPartRect(m_volume_rect, kMCPlayerControllerPartVolumeSelector);
                MCRectangle t_volume_well;
                t_volume_well = getVolumeBarPartRect(m_volume_rect, kMCPlayerControllerPartVolumeWell);
                
                int32_t t_new_volume, t_height;
                
                t_new_volume = (t_volume_well. y + t_volume_well . height - MCmousey ) * 100 / (t_volume_well . height);
                
                if (t_new_volume < 0)
                    t_new_volume = 0;
                if (t_new_volume > 100)
                    t_new_volume = 100;
                
                m_player -> updateloudness(t_new_volume);
                
                m_player -> setloudness();
                m_player -> layer_redrawall();
                dirtyall();
            }
                break;
                            
            default:
                break;
        }
        
        
        return True;
    }
    
    Boolean kdown(const char *string, KeySym key)
    {
        if (key == XK_Escape)
        {
            close();
            return True;
        }
        return False;
    }
    
    //////////
    
    void openpopup(MCPlayer *p_player)
    {
        MCRectangle t_player_rect;
        t_player_rect = p_player -> getactiverect();
        
        // Compute the rect in screen coords.
        MCRectangle t_rect;
        
        MCU_set_rect(t_rect, t_player_rect . x, t_player_rect . y + t_player_rect . height - CONTROLLER_HEIGHT - 80 - 1, CONTROLLER_HEIGHT, 80);
        t_rect = MCU_recttoroot(p_player -> getstack(), t_rect);
        
        m_player = p_player;
        m_volume_rect = t_rect;
        m_volume_rect . x = m_volume_rect . y = 0;
        
        MCdispatcher -> addmenu(this);
        
        openrect(t_rect, WM_POPUP, NULL, WP_ASRECT, OP_NONE);
    }
    
private:
    MCPlayer *m_player;
    MCRectangle m_volume_rect;
    int m_grabbed_part;
};

static MCPlayerVolumePopup *s_volume_popup = nil;

//////////////////////////////////////////////////////////////////////

// PM-2014-09-01: [[ Bug 13119 ]] Added support for setting the playrate property using a scrollbar. The scrollbar appears on a popup window that opens when shift + clicking on the scrubBack/scrubForward buttons. Just as the volume popup window, it closes when clicking anywhere outside this window, or if pressing Esc key

class MCPlayerRatePopup: public MCStack
{
public:
    MCPlayerRatePopup(void)
    {
        setname_cstring("Player Rate");
        state |= CS_NO_MESSAGES;
        
        cards = NULL;
        cards = MCtemplatecard->clone(False, False);
        cards->setparent(this);
        cards->setstate(True, CS_NO_MESSAGES);
        
        parent = nil;
        
        m_font = nil;
        
        m_player = nil;
        m_grabbed_part = -1;
    }
    
    ~MCPlayerRatePopup(void)
    {
    }
    
    // This will be called when the stack is closed, either directly
    // or indirectly if the popup is cancelled by clicking outside
    // or pressing escape.
    void close(void)
    {
        MCStack::close();
        MCdispatcher -> removemenu();
        // TODO: This popup_closed is for the volume, not the rate
        if (m_player != nil)
            m_player -> popup_closed();
    }
    
    // This is called to render the stack.
    void render(MCContext *dc, const MCRectangle& dirty)
    {
        // draw the rate control content here.
        MCGContextRef t_gcontext = nil;
        dc -> lockgcontext(t_gcontext);
        
        drawControllerRateBarButton(t_gcontext, dirty);
        drawControllerRateWellButton(t_gcontext, dirty);
        drawControllerRateSelectorButton(t_gcontext, dirty);
        dc -> unlockgcontext(t_gcontext);
    }
    void drawControllerRateBarButton(MCGContextRef p_gcontext, const MCRectangle& dirty)
    {
        MCRectangle t_rate_bar_rect = dirty;
        MCGContextAddRectangle(p_gcontext, MCRectangleToMCGRectangle(t_rate_bar_rect));
        MCGContextSetFillRGBAColor(p_gcontext, (m_player -> getcontrollermaincolor() . red / 255.0) / 257.0, (m_player -> getcontrollermaincolor() . green / 255.0) / 257.0, (m_player -> getcontrollermaincolor() . blue / 255.0) / 257.0, 1.0f);
        MCGContextFill(p_gcontext);
    }
    
    void drawControllerRateWellButton(MCGContextRef p_gcontext, const MCRectangle& dirty)
    {
        MCRectangle t_rate_bar_rect = dirty;
        MCRectangle t_rate_well;
        t_rate_well = getRateBarPartRect(dirty, kMCPlayerControllerPartRateWell);
        
        MCGBitmapEffects t_effects;
        t_effects . has_drop_shadow = false;
        t_effects . has_outer_glow = false;
        t_effects . has_inner_glow = false;
        t_effects . has_inner_shadow = true;
        t_effects . has_color_overlay = false;
        
        MCGShadowEffect t_inner_shadow;
        t_inner_shadow . color = MCGColorMakeRGBA(0.0f, 0.0f, 0.0f, 56.0 / 255.0);
        t_inner_shadow . blend_mode = kMCGBlendModeClear;
        t_inner_shadow . size = 0;
        t_inner_shadow . spread = 0;
        
        MCGFloat t_x_offset, t_y_offset;
        int t_distance = t_rate_well . width / 5;
        // Make sure we always have an inner shadow
        if (t_distance == 0)
            t_distance = 1;
        
        MCGraphicsContextAngleAndDistanceToXYOffset(235, t_distance, t_x_offset, t_y_offset);
        t_inner_shadow . x_offset = t_x_offset;
        t_inner_shadow . y_offset = t_y_offset;
        t_inner_shadow . knockout = false;
        
        t_effects . inner_shadow = t_inner_shadow;
        
        MCGContextSetFillRGBAColor(p_gcontext, 0.0f, 0.0f, 0.0f, 1.0f);
        MCGContextSetShouldAntialias(p_gcontext, true);
        
        MCGRectangle t_rounded_rect = MCRectangleToMCGRectangle(t_rate_well);
        
        MCGContextAddRoundedRectangle(p_gcontext, t_rounded_rect, MCGSizeMake(30, 30));
        MCGContextBeginWithEffects(p_gcontext, t_rounded_rect, t_effects);
        MCGContextFill(p_gcontext);
        
        /////////////////////////////////////////
        /* TODO: Update this ugly way of adding the inner shadow 'manually' */
        MCRectangle t_shadow = MCRectangleMake(t_rate_well . x + 1, t_rate_well . y + t_rate_well . height  - 2 , t_rate_well . width - 2, 2);
        
        MCGContextSetShouldAntialias(p_gcontext, true);
        
        MCGContextSetFillRGBAColor(p_gcontext, 56.0 / 255.0, 56.0 / 255.0, 56.0 / 255.0, 1.0f); // GRAY
        MCGRectangle t_shadow_rect = MCRectangleToMCGRectangle(t_shadow);
        
        MCGContextAddRoundedRectangle(p_gcontext, t_shadow_rect, MCGSizeMake(10, 10));
        
        MCGContextFill(p_gcontext);
        ////////////////////////////////////////
        
        MCGContextEnd(p_gcontext);
    }
    

    void drawControllerRateSelectorButton(MCGContextRef p_gcontext, const MCRectangle& dirty)
    {
        MCRectangle t_rate_selector_rect = getRateBarPartRect(dirty, kMCPlayerControllerPartRateSelector);
        
        MCAutoPointer<MCGColor> t_colors;
        MCAutoPointer<MCGFloat> t_stops;
        setRamp(&t_colors, &t_stops);
        
        MCGAffineTransform t_transform;
        float origin_x = t_rate_selector_rect.x + t_rate_selector_rect.width / 2.0;
        float origin_y = t_rate_selector_rect.y + t_rate_selector_rect.height;
        float primary_x = t_rate_selector_rect.x + t_rate_selector_rect.width / 2.0;
        float primary_y = t_rate_selector_rect.y;
        float secondary_x = t_rate_selector_rect.x - t_rate_selector_rect.width / 2.0;
        float secondary_y = t_rate_selector_rect.y + t_rate_selector_rect.height;
        
        setTransform(t_transform, origin_x, origin_y, primary_x, primary_y, secondary_x, secondary_y);
        
        MCGContextSetFillGradient(p_gcontext, kMCGGradientFunctionLinear, *t_stops, *t_colors, 3, false, false, 1, t_transform, kMCGImageFilterNone);
        
        MCGContextSetShouldAntialias(p_gcontext, true);
        MCGContextAddArc(p_gcontext, MCRectangleScalePoints(t_rate_selector_rect, 0.5, 0.5), MCGSizeMake(0.7 * t_rate_selector_rect . width, 0.7 * t_rate_selector_rect . height), 0, 0, 360);
        
        MCGContextFill(p_gcontext);
    }
    
    
    MCRectangle getRateBarPartRect(const MCRectangle& p_rate_bar_rect, int p_part)
    {
        switch (p_part)
        {
                
            case kMCPlayerControllerPartRateWell:
            {
                int32_t t_height = 2 * CONTROLLER_HEIGHT / 5;
                
                int32_t t_x_offset = (p_rate_bar_rect . width - t_height) / 2;
                
                return MCRectangleMake(p_rate_bar_rect . x + 5, p_rate_bar_rect . y + t_height, p_rate_bar_rect . width - 2 * 5, p_rate_bar_rect . height - 2 * t_height);
            }
                break;
                
            case kMCPlayerControllerPartRateSelector:
            {
                MCRectangle t_rate_well_rect = getRateBarPartRect(p_rate_bar_rect, kMCPlayerControllerPartRateWell);
                
                // The width and height of the rateselector are p_rate_bar_rect . height / 2
                int32_t t_rate_selector_width = p_rate_bar_rect . height / 2;
                
                int32_t t_y_offset = p_rate_bar_rect . height / 4;
                
                return MCRectangleMake(t_rate_well_rect . x + t_rate_well_rect . width / 2 - t_rate_selector_width / 2 + float((t_rate_well_rect . width / 2 - t_rate_selector_width / 4) * m_player -> getplayrate()) / MAX_RATE, t_y_offset, t_rate_selector_width, t_rate_selector_width);
            }
                break;
        }
        
        return MCRectangleMake(0, 0, 0, 0);
    }
    
    int getraterectpart(int x, int y)
    {
        
        if (MCU_point_in_rect(getRateBarPartRect(m_rate_rect, kMCPlayerControllerPartRateSelector), x, y))
            return kMCPlayerControllerPartRateSelector;
        
        else if (MCU_point_in_rect(getRateBarPartRect(m_rate_rect, kMCPlayerControllerPartRateWell), x, y))
            return kMCPlayerControllerPartRateWell;
        
        else if (MCU_point_in_rect(m_rate_rect, x, y))
            return kMCPlayerControllerPartRateBar;
        
        else
            return kMCPlayerControllerPartUnknown;
    }
    
    // Mouse handling methods are similar to the control ones.
    
    Boolean mdown(uint2 which)
    {
        int t_part;
        t_part = getraterectpart(MCmousex, MCmousey);
        
        switch(t_part)
        {
            case kMCPlayerControllerPartRateSelector:
            {
                m_grabbed_part = t_part;
            }
                break;
                
            case kMCPlayerControllerPartRateWell:
            case kMCPlayerControllerPartRateBar:
            {
            
                MCRectangle t_part_rate_selector_rect = getRateBarPartRect(m_rate_rect, kMCPlayerControllerPartRateSelector);
                MCRectangle t_part_rate_well_rect = getRateBarPartRect(m_rate_rect, kMCPlayerControllerPartRateWell);
                real8 t_new_rate;
                int32_t t_width;
                
                t_width = t_part_rate_well_rect . width;
        
                t_new_rate = MAX_RATE * float((MCmousex - (t_part_rate_well_rect . x + t_part_rate_well_rect . width / 2 ) ) )/ (t_width / 2);
                
                m_player -> updateplayrate(t_new_rate);
                m_player -> setplayrate();
                m_player -> layer_redrawall();
                dirtyall();
            }
                break;
                
            default:
                break;
        }
        return True;
    }
    
    Boolean mup(uint2 which, bool release)
    {
        // PM-2014-09-30: [[ Bug 13119 ]] Make sure a playRateChanged message is sent when the mouse is up/released, but not when creating the ratepopup
        if (m_grabbed_part != -1)
            m_player -> timer(MCM_play_rate_changed,nil);
        m_grabbed_part = kMCPlayerControllerPartUnknown;
        return True;
    }
    
    Boolean mfocus(int2 x, int2 y)
    {
        MCmousex = x;
        MCmousey = y;
        switch(m_grabbed_part)
        {
            case kMCPlayerControllerPartRateSelector:
            {
                MCRectangle t_part_rate_selector_rect = getRateBarPartRect(m_rate_rect, kMCPlayerControllerPartRateSelector);
                MCRectangle t_part_rate_well_rect = getRateBarPartRect(m_rate_rect, kMCPlayerControllerPartRateWell);
                real8 t_new_rate;
                int32_t t_width;
                
                t_width = t_part_rate_well_rect . width;
                
                t_new_rate = MAX_RATE * float((MCmousex - (t_part_rate_well_rect . x + t_part_rate_well_rect . width / 2 ) ) )/ (t_width / 2);
                
                m_player -> updateplayrate(t_new_rate);
                m_player -> setplayrate();
                m_player -> layer_redrawall();
                dirtyall();
            }
                break;
                
            default:
                break;
        }
        
        
        return True;
    }
    
    Boolean kdown(const char *string, KeySym key)
    {
        if (key == XK_Escape)
        {
            close();
            return True;
        }
        return False;
    }
    
    //////////
    
    void openpopup(MCPlayer *p_player)
    {
        MCRectangle t_player_rect;
        t_player_rect = p_player -> getactiverect();
        
        // Compute the rect in screen coords.
        MCRectangle t_rect;
        
        MCU_set_rect(t_rect, t_player_rect . x + t_player_rect . width - 2 * CONTROLLER_HEIGHT, t_player_rect . y + t_player_rect . height - CONTROLLER_HEIGHT, 2 * CONTROLLER_HEIGHT, CONTROLLER_HEIGHT);
        t_rect = MCU_recttoroot(p_player -> getstack(), t_rect);
        
        m_player = p_player;
        m_rate_rect = t_rect;
        m_rate_rect . x = m_rate_rect . y = 0;
        
        MCdispatcher -> addmenu(this);
        
        openrect(t_rect, WM_POPUP, NULL, WP_ASRECT, OP_NONE);
    }
    
private:
    MCPlayer *m_player;
    MCRectangle m_rate_rect;
    int m_grabbed_part;
};

static MCPlayerRatePopup *s_rate_popup = nil;

//////////////////////////////////////////////////////////////////////


//-----------------------------------------------------------------------------
// Control Implementation
//

#define XANIM_WAIT 10.0
#define XANIM_COMMAND 1024

MCPlayer::MCPlayer()
{
	flags |= F_TRAVERSAL_ON;
	nextplayer = NULL;
	rect.width = rect.height = 128;
	filename = NULL;
	istmpfile = False;
	scale = 1.0;
	rate = 1.0;
	lasttime = 0;
	starttime = endtime = MAXUINT4;
    
    // Default controller back area color (darkgray)
    controllerbackcolor . red = 34 * 257;
    controllerbackcolor . green = 34 * 257;
    controllerbackcolor . blue = 34 * 257;
    
    // Default controller played area color (purple)
    controllermaincolor . red = 168 * 257;
    controllermaincolor . green = 1 * 257;
    controllermaincolor . blue = 255 * 257;
    
    // Default controller selected area color (some gray)
    selectedareacolor . red = 43 * 257;
    selectedareacolor . green = 43 * 257;
    selectedareacolor . blue = 43 * 257;
    
	disposable = istmpfile = False;
	userCallbackStr = NULL;
	formattedwidth = formattedheight = 0;
	loudness = 100;
    
    // PM-2014-05-29: [[ Bugfix 12501 ]] Initialize m_callbacks/m_callback_count to prevent a crash when setting callbacks
    m_callback_count = 0;
    m_callbacks = NULL;
    
	m_platform_player = nil;
    
    m_grabbed_part = kMCPlayerControllerPartUnknown;
    m_was_paused = True;
    m_inside = False;
    m_show_volume = false;
    m_scrub_back_is_pressed = false;
    m_scrub_forward_is_pressed = false;
    m_modify_selection_while_playing = false;
    
    // MW-2014-07-16: [[ Bug ]] Put the player in the list.
    nextplayer = MCplayers;
    MCplayers = this;
    
    // PM-2104-10-14: [[ Bug 13569 ]] Make sure changes to player in preOpenCard are not visible
    m_is_attached = false;
    m_should_attach = false;
}

MCPlayer::MCPlayer(const MCPlayer &sref) : MCControl(sref)
{
	nextplayer = NULL;
	filename = strclone(sref.filename);
	istmpfile = False;
	scale = 1.0;
	rate = sref.rate;
	lasttime = sref.lasttime;
	starttime = sref.starttime;
    controllerbackcolor = sref.controllerbackcolor;
    controllermaincolor = sref.controllermaincolor;
    selectedareacolor = sref.selectedareacolor;
	endtime = sref.endtime;
	disposable = istmpfile = False;
	userCallbackStr = strclone(sref.userCallbackStr);
	formattedwidth = formattedheight = 0;
	loudness = sref.loudness;
    
    // PM-2014-05-29: [[ Bugfix 12501 ]] Initialize m_callbacks/m_callback_count to prevent a crash when setting callbacks
    m_callback_count = 0;
    m_callbacks = NULL;
	
	m_platform_player = nil;
    
    m_grabbed_part = kMCPlayerControllerPartUnknown;
    m_was_paused = True;
    m_inside = False;
    m_show_volume = false;
    m_scrub_back_is_pressed = false;
    m_scrub_forward_is_pressed = false;
    m_modify_selection_while_playing = false;
    
    // MW-2014-07-16: [[ Bug ]] Put the player in the list.
    nextplayer = MCplayers;
    MCplayers = this;
}

MCPlayer::~MCPlayer()
{
	// OK-2009-04-30: [[Bug 7517]] - Ensure the player is actually closed before deletion, otherwise dangling references may still exist.
	while (opened)
		close();
	
	playstop();
    
    // MW-2014-07-16: [[ Bug ]] Remove the player from the player's list.
	if (MCplayers != NULL)
	{
		if (MCplayers == this)
			MCplayers = nextplayer;
		else
		{
			MCPlayer *tptr = MCplayers;
			while (tptr->nextplayer != NULL && tptr->nextplayer != this)
				tptr = tptr->nextplayer;
			if (tptr->nextplayer == this)
                tptr->nextplayer = nextplayer;
		}
	}
	nextplayer = NULL;
    
	if (m_platform_player != nil)
		MCPlatformPlayerRelease(m_platform_player);
    
	delete filename;
	delete userCallbackStr;
}

Chunk_term MCPlayer::gettype() const
{
	return CT_PLAYER;
}

const char *MCPlayer::gettypestring()
{
	return MCplayerstring;
}

MCRectangle MCPlayer::getactiverect(void)
{
	return MCU_reduce_rect(getrect(), getflag(F_SHOW_BORDER) ? borderwidth : 0);
}

void MCPlayer::open()
{
	MCControl::open();
	prepare(MCnullstring);
    // PM-2014-10-15: [[ Bug 13650 ]] Check for nil to prevent a crash
    // PM-2014-10-21: [[ Bug 13710 ]] Check if the player is already attached
    
    if (m_platform_player != nil && !m_is_attached && m_should_attach)
    {
        MCPlatformAttachPlayer(m_platform_player, getstack() -> getwindow());
        m_is_attached = true;
        m_should_attach = false;
    }
}

void MCPlayer::close()
{
	MCControl::close();
	if (opened == 0)
	{
		state |= CS_CLOSING;
		playstop();
		state &= ~CS_CLOSING;
	}
    
    if (s_volume_popup != nil)
        s_volume_popup -> close();
    
    // PM-2014-10-15: [[ Bug 13650 ]] Check for nil to prevent a crash
    // PM-2014-10-21: [[ Bug 13710 ]] Detach the player only if already attached
    if (m_platform_player != nil && m_is_attached)
    {
        MCPlatformDetachPlayer(m_platform_player);
        m_is_attached = false;
    }
    // PM-2014-11-03: [[ Bug 13917 ]] m_platform_player should be recreated when reopening a recently closed stack, to take into account if the value of dontuseqt has changed in the meanwhile
    if (m_platform_player != nil)
        m_platform_player = nil;
}

Boolean MCPlayer::kdown(const char *string, KeySym key)
{
    if ((MCmodifierstate & MS_SHIFT) != 0)
        handle_shift_kdown(string, key);
    else
        handle_kdown(string, key);
}

Boolean MCPlayer::kup(const char *string, KeySym key)
{
    return False;
}

Boolean MCPlayer::mfocus(int2 x, int2 y)
{
	if (!(flags & F_VISIBLE || MCshowinvisibles)
        || flags & F_DISABLED && getstack()->gettool(this) == T_BROWSE)
		return False;
    
    Boolean t_success;
    t_success = MCControl::mfocus(x, y);
    if (t_success)
        handle_mfocus(x,y);
    return t_success;
}

void MCPlayer::munfocus()
{
	getstack()->resetcursor(True);
	MCControl::munfocus();
}

Boolean MCPlayer::mdown(uint2 which)
{
    if (state & CS_MFOCUSED || flags & F_DISABLED)
		return False;
    if (state & CS_MENU_ATTACHED)
		return MCObject::mdown(which);
	state |= CS_MFOCUSED;
	if (flags & F_TRAVERSAL_ON && !(state & CS_KFOCUSED))
		getstack()->kfocusset(this);
    
	switch (which)
	{
        case Button1:
            switch (getstack()->gettool(this))
		{
            case T_BROWSE:
                message_with_args(MCM_mouse_down, "1");
                // PM-2014-07-16: [[ Bug 12817 ]] Create selection when click and drag on the well while shift key is pressed
                if ((MCmodifierstate & MS_SHIFT) != 0)
                    handle_shift_mdown(which);
                else
                    handle_mdown(which);
                MCscreen -> addtimer(this, MCM_internal, MCblinkrate);
                break;
            case T_POINTER:
            case T_PLAYER:  //when the movie object is in editing mode
                start(True); //starting draggin or resizing
                playpause(True);  //pause the movie
                break;
            case T_HELP:
                break;
            default:
                return False;
		}
            break;
		case Button2:
            if (message_with_args(MCM_mouse_down, "2") == ES_NORMAL)
                return True;
            break;
		case Button3:
            message_with_args(MCM_mouse_down, "3");
            break;
	}
	return True;
}

Boolean MCPlayer::mup(uint2 which, bool p_release) //mouse up
{
	if (!(state & CS_MFOCUSED))
		return False;
	if (state & CS_MENU_ATTACHED)
		return MCObject::mup(which, p_release);
	state &= ~CS_MFOCUSED;
	if (state & CS_GRAB)
	{
		ungrab(which);
		return True;
	}
	switch (which)
	{
        case Button1:
            switch (getstack()->gettool(this))
		{
            case T_BROWSE:
                if (!p_release && MCU_point_in_rect(rect, mx, my))
                    message_with_args(MCM_mouse_up, "1");
                else
                    message_with_args(MCM_mouse_release, "1");
                MCscreen -> cancelmessageobject(this, MCM_internal);
                handle_mup(which);
                break;
            case T_PLAYER:
            case T_POINTER:
                end(true, p_release);       //stop dragging or moving the movie object, will change controller size
                break;
            case T_HELP:
                help();
                break;
            default:
                return False;
		}
            break;
        case Button2:
        case Button3:
            if (!p_release && MCU_point_in_rect(rect, mx, my))
                message_with_args(MCM_mouse_up, which);
            else
                message_with_args(MCM_mouse_release, which);
            break;
	}
	return True;
}

Boolean MCPlayer::doubledown(uint2 which)
{
    // PM-2014-08-11: [[ Bug 13063 ]] Treat a doubledown on the controller as a single mdown
    // PM-2014-10-22: [[ Bug 13752 ]] If on edit mode, treat a doubledown on the controller as a MCControl::doubledown
    if (hittestcontroller(mx, my) == kMCPlayerControllerPartUnknown || (which == Button1 && getstack() -> gettool(this) == T_POINTER))
        return MCControl::doubledown(which);
    if (which == Button1 && getstack() -> gettool(this) == T_BROWSE)
    {
        if ((MCmodifierstate & MS_SHIFT) != 0)
            handle_shift_mdown(which);
        else
            handle_mdown(which);
    }
    return True;
}

Boolean MCPlayer::doubleup(uint2 which)
{
    // PM-2014-08-11: [[ Bug 13063 ]] Treat a doubleup on the controller as a single mup
    // PM-2014-10-22: [[ Bug 13752 ]] If on edit mode, treat a doubledown on the controller as a MCControl::doubledown
    if (hittestcontroller(mx, my) == kMCPlayerControllerPartUnknown || (which == Button1 && getstack() -> gettool(this) == T_POINTER))
        return MCControl::doubleup(which);
    if (which == Button1 && getstack() -> gettool(this) == T_BROWSE)
        handle_mup(which);
    return True;
}

void MCPlayer::setrect(const MCRectangle &nrect)
{
	rect = nrect;
	
	if (m_platform_player != nil)
	{
		MCRectangle trect = MCU_reduce_rect(rect, getflag(F_SHOW_BORDER) ? borderwidth : 0);
        
        if (getflag(F_SHOW_CONTROLLER))
            trect . height -= CONTROLLER_HEIGHT;
        
        // MW-2014-04-09: [[ Bug 11922 ]] Make sure we use the view not device transform
        //   (backscale factor handled in platform layer).
		trect = MCRectangleGetTransformedBounds(trect, getstack()->getviewtransform());
		MCPlatformSetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyRect, kMCPlatformPropertyTypeRectangle, &trect);
	}
}

void MCPlayer::timer(MCNameRef mptr, MCParameter *params)
{
    if (MCNameIsEqualTo(mptr, MCM_play_started, kMCCompareCaseless))
    {
        state &= ~CS_PAUSED;
        redrawcontroller();
    }
    else if (MCNameIsEqualTo(mptr, MCM_play_stopped, kMCCompareCaseless))
    {
        state |= CS_PAUSED;
        redrawcontroller();
        
        m_modify_selection_while_playing = false;
        
        if (disposable)
        {
            playstop();
            return; //obj is already deleted, do not pass msg up.
        }
    }
    else if (MCNameIsEqualTo(mptr, MCM_play_paused, kMCCompareCaseless))
    {
        state |= CS_PAUSED;
        redrawcontroller();
        
        m_modify_selection_while_playing = false;
    }
    else if (MCNameIsEqualTo(mptr, MCM_current_time_changed, kMCCompareCaseless))
    {
        // If params is nil then this did not originate from the player!
        if (params != nil)
        {
            // Update the current time in the parameter and make sure we allow another
            // currentTimeChanged message to be posted.
            state &= ~CS_CTC_PENDING;
            params -> setn_argument(getmoviecurtime());
        }
    }
    else if (MCNameIsEqualTo(mptr, MCM_internal, kMCCompareCaseless))
    {
        handle_mstilldown(Button1);
        MCscreen -> addtimer(this, MCM_internal, MCblinkrate);
    }
    MCControl::timer(mptr, params);
}

Exec_stat MCPlayer::getprop(uint4 parid, Properties which, MCExecPoint &ep, Boolean effective)
{
	uint2 i = 0;
	switch (which)
	{
#ifdef /* MCPlayer::getprop */ LEGACY_EXEC
        case P_FILE_NAME:
            if (filename == NULL)
                ep.clear();
            else
                ep.setsvalue(filename);
            break;
        case P_DONT_REFRESH:
            ep.setboolean(getflag(F_DONT_REFRESH));
            break;
        case P_CURRENT_TIME:
            ep.setint(getmoviecurtime());
            break;
        case P_DURATION:
            ep.setint(getduration());
            break;
        case P_LOOPING:
            ep.setboolean(getflag(F_LOOPING));
            break;
        case P_PAUSED:
            ep.setboolean(ispaused());
            break;
        case P_ALWAYS_BUFFER:
            ep.setboolean(getflag(F_ALWAYS_BUFFER));
            break;
            // PM-2014-09-02: [[ Bug 13092 ]] Added status property
        case P_STATUS:
        {
            if(getmovieloadedtime() != 0 && getmovieloadedtime() < getduration())
                ep.setcstring("loading");
            else if (!ispaused())
                ep.setcstring("playing");
            else if (ispaused())
                ep.setcstring("paused");
        }
            break;
        case P_PLAY_RATE:
            ep.setr8(rate, ep.getnffw(), ep.getnftrailing(), ep.getnfforce());
            return ES_NORMAL;
        case P_START_TIME:
            if (starttime == MAXUINT4)
                ep.clear();
            else
                ep.setnvalue(starttime);//for QT, this is the selection start time
            break;
        case P_END_TIME:
            if (endtime == MAXUINT4)
                ep.clear();
            else
                ep.setnvalue(endtime); //for QT, this is the selection's end time
            break;
        case P_SHOW_BADGE:
            ep.setboolean(getflag(F_SHOW_BADGE));
            break;
        case P_SHOW_CONTROLLER:
            ep.setboolean(getflag(F_SHOW_CONTROLLER));
            break;
        case P_PLAY_SELECTION:
            ep.setboolean(getflag(F_PLAY_SELECTION));
            break;
        case P_HILITE_COLOR:
            ep.setcolor(controllermaincolor);
            break;
        case P_FORE_COLOR:
            ep.setcolor(selectedareacolor);
            break;
        case P_SHOW_SELECTION:
            ep.setboolean(getflag(F_SHOW_SELECTION));
            break;
        case P_CALLBACKS:
            ep.setsvalue(userCallbackStr);
            break;
        case P_TIME_SCALE:
            ep.setint(gettimescale());
            break;
        case P_FORMATTED_HEIGHT:
            ep.setint(getpreferredrect().height);
            break;
        case P_FORMATTED_WIDTH:
            ep.setint(getpreferredrect().width);
            break;
        case P_MOVIE_CONTROLLER_ID:
            ep.setint((int)NULL);
            break;
        case P_PLAY_LOUDNESS:
            ep.setint(getloudness());
            break;
        case P_TRACK_COUNT:
            if (m_platform_player != nil)
            {
                uindex_t t_count;
                MCPlatformCountPlayerTracks(m_platform_player, t_count);
                i = t_count;
            }
            ep.setint(i);
            break;
        case P_TRACKS:
            gettracks(ep);
            break;
        case P_ENABLED_TRACKS:
            getenabledtracks(ep);
            break;
        case P_MEDIA_TYPES:
            ep.clear();
            if (m_platform_player != nil)
            {
                MCPlatformPlayerMediaTypes t_types;
                MCPlatformGetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyMediaTypes, kMCPlatformPropertyTypePlayerMediaTypes, &t_types);
                bool first = true;
                for (i = 0 ; i < sizeof(ppmediatypes) / sizeof(ppmediatypes[0]) ; i++)
                    if ((t_types & (1 << ppmediatypes[i])) != 0)
                    {
                        ep.concatcstring(ppmediastrings[i], EC_COMMA, first);
                        first = false;
                    }
            }
            break;
        // PM-2014-08-19 [[ Bug 13121 ]] Property for the download progress of the movie
        case P_MOVIE_LOADED_TIME:
            ep.clear();
            ep.setint(getmovieloadedtime());
            break;
        case P_CURRENT_NODE:
			if (m_platform_player != nil)
				MCPlatformGetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyQTVRNode, kMCPlatformPropertyTypeUInt16, &i);
            ep.setint(i);
            break;
        case P_PAN:
		{
			real8 pan = 0.0;
			if (m_platform_player != nil)
				MCPlatformGetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyQTVRPan, kMCPlatformPropertyTypeDouble, &pan);
            
			ep.setr8(pan, ep.getnffw(), ep.getnftrailing(), ep.getnfforce());
		}
            break;
        case P_TILT:
		{
			real8 tilt = 0.0;
			if (m_platform_player != nil)
				MCPlatformGetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyQTVRTilt, kMCPlatformPropertyTypeDouble, &tilt);
            
			ep.setr8(tilt, ep.getnffw(), ep.getnftrailing(), ep.getnfforce());
		}
            break;
        case P_ZOOM:
		{
			real8 zoom = 0.0;
			if (m_platform_player != nil)
				MCPlatformGetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyQTVRZoom, kMCPlatformPropertyTypeDouble, &zoom);
            
			ep.setr8(zoom, ep.getnffw(), ep.getnftrailing(), ep.getnfforce());
		}
            break;
        case P_CONSTRAINTS:
			ep.clear();
			if (m_platform_player != nil)
			{
				MCPlatformPlayerQTVRConstraints t_constraints;
				MCPlatformGetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyQTVRConstraints, kMCPlatformPropertyTypePlayerQTVRConstraints, &t_constraints);
				ep.appendstringf("%lf,%lf\n", t_constraints . x_min, t_constraints . x_max);
				ep.appendstringf("%lf,%lf\n", t_constraints . y_min, t_constraints . y_max);
				ep.appendstringf("%lf,%lf", t_constraints . z_min, t_constraints . z_max);
			}
            break;
        case P_NODES:
            getnodes(ep);
            break;
        case P_HOT_SPOTS:
            gethotspots(ep);
            break;
#endif /* MCPlayer::getprop */
        default:
            return MCControl::getprop(parid, which, ep, effective);
	}
	return ES_NORMAL;
}

Exec_stat MCPlayer::setprop(uint4 parid, Properties p, MCExecPoint &ep, Boolean effective)
{
	Boolean dirty = False;
	Boolean wholecard = False;
	uint4 ctime;
	MCString data = ep.getsvalue();
    
	switch (p)
	{
#ifdef /* MCPlayer::setprop */ LEGACY_EXEC
        case P_FILE_NAME:
            if (filename == NULL || data != filename)
            {
                delete filename;
                filename = NULL;
                playstop();
                starttime = MAXUINT4; //clears the selection
                endtime = MAXUINT4;
                if (data != MCnullmcstring)
                    filename = data.clone();
                prepare(MCnullstring);
                
                // PM-2014-10-20: [[ Bug 13711 ]] Make sure we attach the player after prepare()
                // PM-2014-10-21: [[ Bug 13710 ]] Check if the player is already attached
                if (m_platform_player != nil && !m_is_attached && m_should_attach)
                {
                    MCPlatformAttachPlayer(m_platform_player, getstack() -> getwindow());
                    m_is_attached = true;
                    m_should_attach = false;
                }

                dirty = wholecard = True;
            }
            break;
        case P_DONT_REFRESH:
            if (!MCU_matchflags(data, flags, F_DONT_REFRESH, dirty))
            {
                MCeerror->add(EE_OBJECT_NAB, 0, 0, data);
                return ES_ERROR;
            }
            break;
        case P_ALWAYS_BUFFER:
            if (!MCU_matchflags(data, flags, F_ALWAYS_BUFFER, dirty))
            {
                MCeerror->add(EE_OBJECT_NAB, 0, 0, data);
                return ES_ERROR;
            }
            
            // The actual buffering state is determined upon redrawing - therefore
            // we trigger a redraw to ensure we don't unbuffer when it is
            // needed.
            
            if (opened)
                dirty = True;
            break;
        case P_CALLBACKS:
            delete userCallbackStr;
            if (data.getlength() == 0)
            {
                userCallbackStr = NULL;
                // PM-2014-08-21: [[ Bug 13243 ]] Free the existing callback table.
                for(uindex_t i = 0; i < m_callback_count; i++)
                {
                    MCNameDelete(m_callbacks[i] . message);
                    MCNameDelete(m_callbacks[i] . parameter);
                }
                MCMemoryDeleteArray(m_callbacks);
                m_callbacks = nil;
                m_callback_count = 0;
            }
            else
            {
                userCallbackStr = data.clone();
            }
			SynchronizeUserCallbacks();
            break;
        case P_CURRENT_TIME:
            if (!MCU_stoui4(data, ctime))
            {
                MCeerror->add(EE_OBJECT_NAN, 0, 0, data);
                return ES_ERROR;
            }
            setcurtime(ctime, false);
            if (isbuffering())
                dirty = True;
            else
                redrawcontroller();
            break;
        case P_LOOPING:
            if (!MCU_matchflags(data, flags, F_LOOPING, dirty))
            {
                MCeerror->add(EE_OBJECT_NAB, 0, 0, data);
                return ES_ERROR;
            }
            if (dirty)
                setlooping((flags & F_LOOPING) != 0); //set/unset movie looping
            break;
        case P_PAUSED:
            playpause(data == MCtruemcstring); //pause or unpause the player
            break;
        case P_PLAY_RATE:
            if (!MCU_stor8(data, rate))
            {
                MCeerror->add(EE_OBJECT_NAN, 0, 0, data);
                return ES_ERROR;
            }
            setplayrate();
            break;
        case P_START_TIME: //this is the selection start time
            if (data.getlength() == 0)
                starttime = endtime = MAXUINT4;
            else
            {
                if (!MCU_stoui4(data, starttime))
                {
                    MCeerror->add(EE_OBJECT_NAN, 0, 0, data);
                    return ES_ERROR;
                }
                
                if (endtime == MAXUINT4) //if endtime is not set, set it to the length of movie
                    endtime = getduration();
                else if (starttime > endtime)
                    endtime = starttime;
            }
            setselection(false);
            break;
        case P_END_TIME: //this is the selection end time
            if (data.getlength() == 0)
                starttime = endtime = MAXUINT4;
            else
            {
                if (!MCU_stoui4(data, endtime))
                {
                    MCeerror->add(EE_OBJECT_NAN, 0, 0, data);
                    return ES_ERROR;
                }
                
                if (starttime == MAXUINT4)
                    starttime = 0;
                else if (starttime > endtime)
                    starttime = endtime;
            }
            setselection(false);
            break;
        case P_TRAVERSAL_ON:
            if (MCControl::setprop(parid, p, ep, effective) != ES_NORMAL)
                return ES_ERROR;
            break;
        case P_SHOW_BADGE: //if in the buffering mode we do not want to show/hide the badge
            if (!(flags & F_ALWAYS_BUFFER))
            { //if always buffer flag is not set
                if (!MCU_matchflags(data, flags, F_SHOW_BADGE, dirty))
                {
                    MCeerror->add(EE_OBJECT_NAB, 0, 0, data);
                    return ES_ERROR;
                }
                if (dirty && !isbuffering()) //we are not actually buffering, let's show/hide the badge
                    showbadge((flags & F_SHOW_BADGE) != 0); //show/hide movie's badge
            }
            break;
        case P_SHOW_CONTROLLER:
            if (!MCU_matchflags(data, flags, F_SHOW_CONTROLLER, dirty))
            {
                MCeerror->add(EE_OBJECT_NAB, 0, 0, data);
                return ES_ERROR;
            }
            if (dirty)
            {
                showcontroller((flags & F_VISIBLE) != 0
                               && (flags & F_SHOW_CONTROLLER) != 0);
                dirty = False;
            }
            break;
        case P_PLAY_SELECTION: //make movie play only the selected part
            if (!MCU_matchflags(data, flags, F_PLAY_SELECTION, dirty))
            {
                MCeerror->add
                (EE_OBJECT_NAB, 0, 0, data);
                return ES_ERROR;
            }
            if (dirty)
                playselection((flags & F_PLAY_SELECTION) != 0);
            break;
        case P_FORE_COLOR:
        {
            MCColor t_color;
			char *t_colorname = NULL;
			if (!MCscreen->parsecolor(data, &t_color, &t_colorname))
			{
				MCeerror->add
				(EE_COLOR_BADSELECTEDCOLOR, 0, 0, data);
				return ES_ERROR;
			}
			if (t_colorname != NULL)
				delete t_colorname;
            selectedareacolor = t_color;
            dirty = True;
        }
            break;
            
        case P_HILITE_COLOR:
        {
            MCColor t_color;
			char *t_colorname = NULL;
			if (!MCscreen->parsecolor(data, &t_color, &t_colorname))
			{
				MCeerror->add
				(EE_COLOR_BADSELECTEDCOLOR, 0, 0, data);
				return ES_ERROR;
			}
			if (t_colorname != NULL)
				delete t_colorname;
            controllermaincolor = t_color;
            dirty = True;
        }
            break;
   
        case P_SHOW_SELECTION: //means make movie editable
            if (!MCU_matchflags(data, flags, F_SHOW_SELECTION, dirty))
            {
                MCeerror->add
                (EE_OBJECT_NAB, 0, 0, data);
                return ES_ERROR;
            }
            if (dirty)
                editmovie((flags & F_SHOW_SELECTION) != 0);
            break;
        case P_SHOW_BORDER:
        case P_BORDER_WIDTH:
        {
            if (MCControl::setprop(parid, p, ep, effective) != ES_NORMAL)
                return ES_ERROR;
            setrect(rect);
            dirty = True;
        }
            break;
        case P_MOVIE_CONTROLLER_ID:
            break;
        case P_PLAY_LOUDNESS:
            if (!MCU_stoui2(data, loudness))
            {
                MCeerror->add(EE_OBJECT_NAN, 0, 0, data);
                return ES_ERROR;
            }
            loudness = MCU_max(0, loudness);
            loudness = MCU_min(loudness, 100);
            setloudness();
            // PM-2014-09-02: [[ Bug 13309 ]] Make sure the volume icon of the controller will be redrawn
            dirty = True;
            break;
        case P_ENABLED_TRACKS:
            if (!setenabledtracks(data))
            {
                MCeerror->add(EE_OBJECT_NAN, 0, 0, data);
                return ES_ERROR;
            }
            dirty = wholecard = True;
            break;
        case P_CURRENT_NODE:
		{
			uint2 nodeid;
			if (!MCU_stoui2(data,nodeid))
			{
				MCeerror->add(EE_OBJECT_NAN, 0, 0, data);
				return ES_ERROR;
			}
			if (m_platform_player != nil)
				MCPlatformSetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyQTVRNode, kMCPlatformPropertyTypeUInt16, &nodeid);
		}
            break;
        case P_PAN:
		{
			real8 pan;
			if (!MCU_stor8(data, pan))
			{
				MCeerror->add(EE_OBJECT_NAN, 0, 0, data);
				return ES_ERROR;
			}
			if (m_platform_player != nil)
				MCPlatformSetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyQTVRPan, kMCPlatformPropertyTypeDouble, &pan);
            
			if (isbuffering())
				dirty = True;
		}
            break;
        case P_TILT:
		{
			real8 tilt;
			if (!MCU_stor8(data, tilt))
			{
				MCeerror->add(EE_OBJECT_NAN, 0, 0, data);
				return ES_ERROR;
			}
			if (m_platform_player != nil)
				MCPlatformSetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyQTVRTilt, kMCPlatformPropertyTypeDouble, &tilt);
            
			if (isbuffering())
				dirty = True;
		}
            break;
        case P_ZOOM:
		{
			real8 zoom;
			if (!MCU_stor8(data, zoom))
			{
				MCeerror->add(EE_OBJECT_NAN, 0, 0, data);
				return ES_ERROR;
			}
            
			if (m_platform_player != nil)
				MCPlatformSetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyQTVRZoom, kMCPlatformPropertyTypeDouble, &zoom);
            
			if (isbuffering())
				dirty = True;
		}
            break;
        case P_VISIBLE:
        case P_INVISIBLE:
		{
			uint4 oldflags = flags;
			Exec_stat stat = MCControl::setprop(parid, p, ep, effective);
            
			if (m_platform_player != nil)
			{
				bool t_visible;
				t_visible = getflag(F_VISIBLE);
				MCPlatformSetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyVisible, kMCPlatformPropertyTypeBool, &t_visible);
			}
            
			return stat;
		}
            break;
#endif /* MCPlayer::setprop */
        default:
            return MCControl::setprop(parid, p, ep, effective);
	}
	if (dirty && opened && flags & F_VISIBLE)
	{
		// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
		layer_redrawall();
	}
	return ES_NORMAL;
}

// MW-2011-09-23: Make sure we sync the buffer state at this point, rather than
//   during drawing.
void MCPlayer::select(void)
{
	MCControl::select();
	syncbuffering(nil);
}

// MW-2011-09-23: Make sure we sync the buffer state at this point, rather than
//   during drawing.
void MCPlayer::deselect(void)
{
	MCControl::deselect();
	syncbuffering(nil);
}

MCControl *MCPlayer::clone(Boolean attach, Object_pos p, bool invisible)
{
	MCPlayer *newplayer = new MCPlayer(*this);
	if (attach)
		newplayer->attach(p, invisible);
	return newplayer;
}

IO_stat MCPlayer::extendedsave(MCObjectOutputStream& p_stream, uint4 p_part)
{
	return defaultextendedsave(p_stream, p_part);
}

IO_stat MCPlayer::extendedload(MCObjectInputStream& p_stream, const char *p_version, uint4 p_remaining)
{
	return defaultextendedload(p_stream, p_version, p_remaining);
}

IO_stat MCPlayer::save(IO_handle stream, uint4 p_part, bool p_force_ext)
{
	IO_stat stat;
	if (!disposable)
	{
		if ((stat = IO_write_uint1(OT_PLAYER, stream)) != IO_NORMAL)
			return stat;
		if ((stat = MCControl::save(stream, p_part, p_force_ext)) != IO_NORMAL)
			return stat;
		if ((stat = IO_write_string(filename, stream)) != IO_NORMAL)
			return stat;
		if ((stat = IO_write_uint4(starttime, stream)) != IO_NORMAL)
			return stat;
		if ((stat = IO_write_uint4(endtime, stream)) != IO_NORMAL)
			return stat;
		if ((stat = IO_write_int4((int4)(rate / 10.0 * MAXINT4),
		                          stream)) != IO_NORMAL)
			return stat;
		if ((stat = IO_write_string(userCallbackStr, stream)) != IO_NORMAL)
			return stat;
	}
	return savepropsets(stream);
}

IO_stat MCPlayer::load(IO_handle stream, const char *version)
{
	IO_stat stat;
    
	if ((stat = MCObject::load(stream, version)) != IO_NORMAL)
		return stat;
	if ((stat = IO_read_string(filename, stream)) != IO_NORMAL)
		return stat;
	if ((stat = IO_read_uint4(&starttime, stream)) != IO_NORMAL)
		return stat;
	if ((stat = IO_read_uint4(&endtime, stream)) != IO_NORMAL)
		return stat;
	int4 trate;
	if ((stat = IO_read_int4(&trate, stream)) != IO_NORMAL)
		return stat;
	rate = (real8)trate * 10.0 / MAXINT4;
	if ((stat = IO_read_string(userCallbackStr, stream)) != IO_NORMAL)
		return stat;
	return loadpropsets(stream);
}

// MW-2011-09-23: Ensures the buffering state is consistent with current flags
//   and state.
void MCPlayer::syncbuffering(MCContext *p_dc)
{
	bool t_should_buffer;
	
	// MW-2011-09-13: [[ Layers ]] If the layer is dynamic then the player must be buffered.
	t_should_buffer = getstate(CS_SELECTED) || getflag(F_ALWAYS_BUFFER) || getstack() -> getstate(CS_EFFECT) || (p_dc != nil && p_dc -> gettype() != CONTEXT_TYPE_SCREEN) || !MCModeMakeLocalWindows() || layer_issprite();
    
    // MW-2014-04-24: [[ Bug 12249 ]] If we are not in browse mode for this object, then it should be buffered.
    t_should_buffer = t_should_buffer || getstack() -> gettool(this) != T_BROWSE;
	
	if (m_platform_player != nil)
		MCPlatformSetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyOffscreen, kMCPlatformPropertyTypeBool, &t_should_buffer);
}

// MW-2007-08-14: [[ Bug 1949 ]] On Windows ensure we load and unload QT if not
//   currently in use.
void MCPlayer::getversion(MCExecPoint &ep)
{
    extern void MCQTGetVersion(MCExecPoint& ep);
    MCQTGetVersion(ep);
}

void MCPlayer::freetmp()
{
	if (istmpfile)
	{
		MCS_unlink(filename);
		delete filename;
		filename = NULL;
	}
}
uint4 MCPlayer::getmovieloadedtime()
{
    uint4 loadedtime;
	if (m_platform_player != nil && hasfilename())
		MCPlatformGetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyLoadedTime, kMCPlatformPropertyTypeUInt32, &loadedtime);
	else
		loadedtime = 0;
	return loadedtime;
}

uint4 MCPlayer::getduration() //get movie duration/length
{
	uint4 duration;
	if (m_platform_player != nil && hasfilename())
		MCPlatformGetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyDuration, kMCPlatformPropertyTypeUInt32, &duration);
	else
		duration = 0;
	return duration;
}

uint4 MCPlayer::gettimescale() //get moive time scale
{
	uint4 timescale;
	if (m_platform_player != nil && hasfilename())
		MCPlatformGetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyTimescale, kMCPlatformPropertyTypeUInt32, &timescale);
	else
		timescale = 0;
	return timescale;
}

uint4 MCPlayer::getmoviecurtime()
{
	uint4 curtime;
	if (m_platform_player != nil && hasfilename())
		MCPlatformGetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyCurrentTime, kMCPlatformPropertyTypeUInt32, &curtime);
	else
		curtime = 0;
	return curtime;
}

void MCPlayer::setcurtime(uint4 newtime, bool notify)
{
	lasttime = newtime;
	if (m_platform_player != nil && hasfilename())
    {
		MCPlatformSetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyCurrentTime, kMCPlatformPropertyTypeUInt32, &newtime);
        if (notify)
            currenttimechanged();
    }
}

void MCPlayer::setselection(bool notify)
{
    if (m_platform_player != nil && hasfilename())
	{
        uint32_t t_current_start, t_current_finish;
        MCPlatformGetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyStartTime, kMCPlatformPropertyTypeUInt32, &t_current_start);
		MCPlatformGetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyFinishTime, kMCPlatformPropertyTypeUInt32, &t_current_finish);
        
        if (starttime != t_current_start || endtime != t_current_finish)
        {
            uint32_t t_st, t_et;
            if (starttime == MAXUINT4 || endtime == MAXUINT4)
                t_st = t_et = 0;
            else
            {
                t_st = starttime;
                t_et = endtime;
            }
            
            // PM-2014-08-06: [[ Bug 13064 ]] 
            // If we first set StartTime and FinishTime is not set (= 0), then startTime becomes 0 (Since if StartTime > FinishTime then StartTime = FinishTime)
            // For this reason, we first set FinishTime 
            MCPlatformSetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyFinishTime, kMCPlatformPropertyTypeUInt32, &t_et);
            MCPlatformSetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyStartTime, kMCPlatformPropertyTypeUInt32, &t_st);
            
            if (notify)
                selectionchanged();
            
            // MW-2014-07-22: [[ Bug 12870 ]] Make sure controller rect redrawn when setting selection
            //   by script.
            layer_redrawrect(getcontrollerrect());
        }
        
        if (!m_modify_selection_while_playing)
            playselection(getflag(F_PLAY_SELECTION));
	}
}

void MCPlayer::setlooping(Boolean loop)
{
	if (m_platform_player != nil && hasfilename())
	{
		bool t_loop;
		t_loop = loop;
		MCPlatformSetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyLoop, kMCPlatformPropertyTypeBool, &t_loop);
	}
}

real8 MCPlayer::getplayrate()
{
    if (rate < MIN_RATE)
        return MIN_RATE;
    else if (rate > MAX_RATE)
        return MAX_RATE;
    else
        return rate;
}

void MCPlayer::updateplayrate(real8 p_rate)
{
    if (p_rate < MIN_RATE)
        rate = MIN_RATE;
    else if (p_rate > MAX_RATE)
        rate = MAX_RATE;
    else
        rate = p_rate;
}
void MCPlayer::setplayrate()
{
	if (m_platform_player != nil && hasfilename())
	{
		MCPlatformSetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyPlayRate, kMCPlatformPropertyTypeDouble, &rate);
		if (rate != 0.0f)
        // PM-2014-05-28: [[ Bug 12523 ]] Take into account the playRate property
			MCPlatformStartPlayer(m_platform_player, rate);
	}
    
	if (rate != 0)
    {
        if (getstate(CS_PAUSED))
            timer(MCM_play_started, nil);
		state = state & ~CS_PAUSED;
    }
	else
    {
        if (!getstate(CS_PAUSED))
            timer(MCM_play_paused, nil);
		state = state | CS_PAUSED;
    }
    
    redrawcontroller();
}

void MCPlayer::showbadge(Boolean show)
{
#if 0
	if (m_platform_player != nil)
	{
		bool t_show;
		t_show = show;
		MCPlatformSetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyShowBadge, kMCPlatformPropertyTypeBool, &t_show);
	}
#endif
}

void MCPlayer::editmovie(Boolean edit)
{

	if (m_platform_player != nil && hasfilename())
	{
		bool t_edit;
		t_edit = edit;
		MCPlatformSetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyShowSelection, kMCPlatformPropertyTypeBool, &t_edit);
	}

}

void MCPlayer::playselection(Boolean play)
{
	if (m_platform_player != nil && hasfilename())
	{
		bool t_play;
		t_play = play;
		MCPlatformSetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyOnlyPlaySelection, kMCPlatformPropertyTypeBool, &t_play);
	}
}

Boolean MCPlayer::ispaused()
{
	if (m_platform_player != nil && hasfilename())
		return !MCPlatformPlayerIsPlaying(m_platform_player);
    
    return True;
}

void MCPlayer::showcontroller(Boolean show)
{
    // The showController property has changed, this means we must do two things - resize
    // the movie rect and then redraw ourselves to make sure we can see the controller.
    
    if (m_platform_player != nil)
	{
        // PM-2014-05-28: [[ Bug 12524 ]] Resize the rect height to avoid stretching of the movie when showing/hiding controller
        MCRectangle drect;
        drect = rect;
        
        // MW-2014-07-16: [[ QTSupport ]] We always use our own controller now.
        int t_height;
        t_height = CONTROLLER_HEIGHT;
                
        if (show )
            drect . height += t_height;  // This is the height of the default QTKit controller
        else
            drect . height -= t_height;
        
        layer_setrect(drect, true);
	}
}

Boolean MCPlayer::prepare(const char *options)
{
    // For osversion < 10.8 we have to have QT initialized.
    if (MCmajorosversion < 0x1080)
    {
        extern bool MCQTInit(void);
        if (!MCQTInit())
            return False;
    }

	Boolean ok = False;
    m_should_attach = false;
    
	if (state & CS_PREPARED)
		return True;
    
    // Fixes the issue of invisible player being created by script
	if (!hasfilename())
        return True;
    
	if (!opened)
		return False;
    
	if (m_platform_player == nil)
		MCPlatformCreatePlayer(m_platform_player);
    
	if (strnequal(filename, "https:", 6) || strnequal(filename, "http:", 5) || strnequal(filename, "ftp:", 4) || strnequal(filename, "file:", 5) || strnequal(filename, "rtsp:", 5))
		MCPlatformSetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyURL, kMCPlatformPropertyTypeNativeCString, &filename);
	else
		MCPlatformSetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyFilename, kMCPlatformPropertyTypeNativeCString, &filename);
	
	MCRectangle t_movie_rect;
	MCPlatformGetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyMovieRect, kMCPlatformPropertyTypeRectangle, &t_movie_rect);
	
	MCRectangle trect = resize(t_movie_rect);
	
    // Adjust so that the controller isn't included in the movie rect.
    if (getflag(F_SHOW_CONTROLLER))
        trect . height -= CONTROLLER_HEIGHT;
    
	// IM-2011-11-12: [[ Bug 11320 ]] Transform player rect to device coords
    // MW-2014-04-09: [[ Bug 11922 ]] Make sure we use the view not device transform
    //   (backscale factor handled in platform layer).
	trect = MCRectangleGetTransformedBounds(trect, getstack()->getviewtransform());
	
	MCPlatformSetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyRect, kMCPlatformPropertyTypeRectangle, &trect);
	
	bool t_looping, t_play_selection, t_show_controller, t_show_selection;
	
	t_looping = getflag(F_LOOPING);
	t_show_selection = getflag(F_SHOW_SELECTION);
    t_play_selection = getflag(F_PLAY_SELECTION);
	
	MCPlatformSetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyCurrentTime, kMCPlatformPropertyTypeUInt32, &lasttime);
    MCPlatformSetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyLoop, kMCPlatformPropertyTypeBool, &t_looping);
    MCPlatformSetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyShowSelection, kMCPlatformPropertyTypeBool, &t_show_selection);
    
    // PM-2014-08-06: [[ Bug 13104 ]] When new movie is opened then playRate should be set to 0
    rate = 0.0;
    
	setselection(false);
	MCPlatformSetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyOnlyPlaySelection, kMCPlatformPropertyTypeBool, &t_play_selection);
	SynchronizeUserCallbacks();
	
	bool t_offscreen;
	t_offscreen = getflag(F_ALWAYS_BUFFER);
	MCPlatformSetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyOffscreen, kMCPlatformPropertyTypeBool, &t_offscreen);
	
	bool t_visible;
	t_visible = getflag(F_VISIBLE);
	MCPlatformSetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyVisible, kMCPlatformPropertyTypeBool, &t_visible);
	
    if (m_is_attached)
    {
        MCPlatformDetachPlayer(m_platform_player);
        m_is_attached = false;
        m_should_attach = true;
    }
    else
        m_should_attach = true;
    	
	layer_redrawall();
	
	setloudness();
	
	MCresult -> clear(False);
	
	ok = True;
	
	if (ok)
	{
		state |= CS_PREPARED | CS_PAUSED;
	}
    
	return ok;
}

// PM-2014-10-14: [[ Bug 13569 ]] Make sure changes to player are not visible in preOpenCard
void MCPlayer::attachplayer()
{
    if (m_platform_player == nil)
        return;
    
    // Make sure we attach the player only if it was previously detached by detachplayer().
    if (!m_is_attached && m_should_attach)
    {
        MCPlatformAttachPlayer(m_platform_player, getstack() -> getwindow());
        m_is_attached = true;
        m_should_attach = false;
    }
}

// PM-2014-10-14: [[ Bug 13569 ]] Make sure changes to player are not visible in preOpenCard
void MCPlayer::detachplayer()
{
    if (m_platform_player == nil)
        return;
    
    if (m_is_attached)
    {
        MCPlatformDetachPlayer(m_platform_player);
        m_is_attached = false;
        m_should_attach = true;
    }
}

Boolean MCPlayer::playstart(const char *options)
{
	if (!prepare(options))
		return False;
    
    // PM-2014-10-21: [[ Bug 13710 ]] Attach the player if not already attached
    if (m_platform_player != nil && !m_is_attached)
    {
        MCPlatformAttachPlayer(m_platform_player, getstack() -> getwindow());
        m_is_attached = true;
    }
	playpause(False);
	return True;
}

Boolean MCPlayer::playpause(Boolean on)
{
	if (!(state & CS_PREPARED))
		return False;
    
	Boolean ok;
	ok = False;
    
    if (on)
        m_modify_selection_while_playing = false;
    
	if (m_platform_player != nil)
	{
		if (!on)
        {
            playselection(getflag(F_PLAY_SELECTION) && !m_modify_selection_while_playing);
            // PM-2014-08-06: [[ Bug 13104 ]] Force playRate to 1.0 (needed when starting player by pressing space/enter keys 
            rate = 1.0;
			MCPlatformStartPlayer(m_platform_player, rate);
		}
        else
        {
			MCPlatformStopPlayer(m_platform_player);
            // PM-2014-08-06: [[ Bug 13104 ]] Make sure playRate is zero when player is paused
            rate = 0.0;
        }
		ok = True;
	}
	
	if (ok)
    {
        if (getstate(CS_PAUSED) && !on)
            timer(MCM_play_started, nil);
        else if (!getstate(CS_PAUSED) && on)
            timer(MCM_play_paused, nil);
		setstate(on, CS_PAUSED);
        
        redrawcontroller();
    }
    
	return ok;
}

void MCPlayer::playstepforward()
{
	if (!getstate(CS_PREPARED))
		return;
    
	if (m_platform_player != nil)
		MCPlatformStepPlayer(m_platform_player, 1);
}


void MCPlayer::playstepback()
{
	if (!getstate(CS_PREPARED))
		return;
	
	if (m_platform_player != nil)
		MCPlatformStepPlayer(m_platform_player, -1);
}

Boolean MCPlayer::playstop()
{
	formattedwidth = formattedheight = 0;
	if (!getstate(CS_PREPARED))
		return False;
    
	Boolean needmessage = True;
	
	state &= ~(CS_PREPARED | CS_PAUSED);
	lasttime = 0;
    
    m_modify_selection_while_playing = false;
	
    // PM-2014-10-21: [[ Bug 13710 ]] Detach the player only if already attached
	if (m_platform_player != nil && m_is_attached)
	{
		MCPlatformStopPlayer(m_platform_player);

		needmessage = getduration() > getmoviecurtime();
		
		MCPlatformDetachPlayer(m_platform_player);
        m_is_attached = false;
	}
    
    redrawcontroller();
    
	freetmp();
    
    /*
	if (MCplayers != NULL)
	{
		if (MCplayers == this)
			MCplayers = nextplayer;
		else
		{
			MCPlayer *tptr = MCplayers;
			while (tptr->nextplayer != NULL && tptr->nextplayer != this)
				tptr = tptr->nextplayer;
			if (tptr->nextplayer == this)
                tptr->nextplayer = nextplayer;
		}
	}
	nextplayer = NULL;*/
    
	if (disposable)
	{
		//if (needmessage)
		//	getcard()->message_with_args(MCM_play_stopped, getname());
		delete this;
	}
	//else
		//if (needmessage)
	//		message_with_args(MCM_play_stopped, getname());
    
	return True;
}


void MCPlayer::setfilename(const char *vcname,
                           char *fname, Boolean istmp)
{
	setname_cstring(vcname);
	filename = fname;
	istmpfile = istmp;
	disposable = True;
}

void MCPlayer::setvolume(uint2 tloudness)
{
}

MCRectangle MCPlayer::getpreferredrect()
{
	if (!getstate(CS_PREPARED))
	{
		MCRectangle t_bounds;
		MCU_set_rect(t_bounds, 0, 0, formattedwidth, formattedheight);
		return t_bounds;
	}
    
    MCRectangle t_bounds;
	MCU_set_rect(t_bounds, 0, 0, 0, 0);
	if (m_platform_player != nil)
    {
		MCPlatformGetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyMovieRect, kMCPlatformPropertyTypeRectangle, &t_bounds);
        // PM-2014-04-28: [[Bug 12299]] Make sure the correct MCRectangle is returned
        return t_bounds;
    }
}

uint2 MCPlayer::getloudness()
{
	if (getstate(CS_PREPARED))
		if (m_platform_player != nil)
			MCPlatformGetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyVolume, kMCPlatformPropertyTypeUInt16, &loudness);
	return loudness;
}

MCColor MCPlayer::getcontrollerbackcolor()
{
    return controllerbackcolor;
}

MCColor MCPlayer::getcontrollermaincolor()
{
    return controllermaincolor;
}

void MCPlayer::updateloudness(int2 newloudness)
{
    loudness = newloudness;
}

void MCPlayer::setloudness()
{
	if (state & CS_PREPARED)
		if (m_platform_player != nil)
			MCPlatformSetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyVolume, kMCPlatformPropertyTypeUInt16, &loudness);
}

void MCPlayer::gettracks(MCExecPoint &ep)
{
	ep . clear();
    
	if (getstate(CS_PREPARED))
		if (m_platform_player != nil)
		{
			uindex_t t_track_count;
			MCPlatformCountPlayerTracks(m_platform_player, t_track_count);
			for(uindex_t i = 0; i < t_track_count; i++)
			{
				uint32_t t_id;
				MCAutoPointer<char> t_name;
				uint32_t t_offset, t_duration;
				MCPlatformGetPlayerTrackProperty(m_platform_player, i, kMCPlatformPlayerTrackPropertyId, kMCPlatformPropertyTypeUInt32, &t_id);
				MCPlatformGetPlayerTrackProperty(m_platform_player, i, kMCPlatformPlayerTrackPropertyMediaTypeName, kMCPlatformPropertyTypeNativeCString, &(&t_name));
				MCPlatformGetPlayerTrackProperty(m_platform_player, i, kMCPlatformPlayerTrackPropertyOffset, kMCPlatformPropertyTypeUInt32, &t_offset);
                // MW-2014-07-11: [[ Bug 12757 ]] Fetch the duration and store it in t_duration
				MCPlatformGetPlayerTrackProperty(m_platform_player, i, kMCPlatformPlayerTrackPropertyDuration, kMCPlatformPropertyTypeUInt32, &t_duration);
                // PM-2014-07-14: [[ Bug 12809 ]] Make sure each track is displayed on a separate line
				ep . concatuint(t_id, EC_RETURN, i == 0);
				ep . concatcstring(*t_name, EC_COMMA, false);
				ep . concatuint(t_offset, EC_COMMA, false);
				ep . concatuint(t_duration, EC_COMMA, false);
			}
		}
}

void MCPlayer::getenabledtracks(MCExecPoint &ep)
{
	ep.clear();
    
	if (getstate(CS_PREPARED))
		if (m_platform_player != nil)
		{
			uindex_t t_track_count;
			MCPlatformCountPlayerTracks(m_platform_player, t_track_count);
			for(uindex_t i = 0; i < t_track_count; i++)
			{
				uint32_t t_id;
				uint32_t t_enabled;
				MCPlatformGetPlayerTrackProperty(m_platform_player, i, kMCPlatformPlayerTrackPropertyId, kMCPlatformPropertyTypeUInt32, &t_id);
				MCPlatformGetPlayerTrackProperty(m_platform_player, i, kMCPlatformPlayerTrackPropertyEnabled, kMCPlatformPropertyTypeBool, &t_enabled);
				if (t_enabled)
					ep . concatuint(t_id, EC_RETURN, i == 0);
			}
		}
}

Boolean MCPlayer::setenabledtracks(const MCString &s)
{
	if (getstate(CS_PREPARED))
		if (m_platform_player != nil)
		{
			uindex_t t_track_count;
			MCPlatformCountPlayerTracks(m_platform_player, t_track_count);
			for(uindex_t i = 0; i < t_track_count; i++)
			{
				bool t_enabled;
				t_enabled = false;
				MCPlatformSetPlayerTrackProperty(m_platform_player, i, kMCPlatformPlayerTrackPropertyEnabled, kMCPlatformPropertyTypeBool, &t_enabled);
			}
			char *data = s.clone();
			char *sptr = data;
			while (*sptr)
			{
				char *tptr;
				if ((tptr = strchr(sptr, '\n')) != NULL)
					*tptr++ = '\0';
				else
					tptr = &sptr[strlen(sptr)];
				if (strlen(sptr) != 0)
				{
					uindex_t t_index;
					if (!MCPlatformFindPlayerTrackWithId(m_platform_player, strtol(sptr, NULL, 10), t_index))
					{
						delete data;
						return False;
					}
					
					bool t_enabled;
					t_enabled = true;
					MCPlatformSetPlayerTrackProperty(m_platform_player, t_index, kMCPlatformPlayerTrackPropertyEnabled, kMCPlatformPropertyTypeBool, &t_enabled);
				}
				sptr = tptr;
			}
			delete data;
			MCRectangle t_movie_rect;
			MCPlatformGetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyMovieRect, kMCPlatformPropertyTypeRectangle, &t_movie_rect);
			MCRectangle trect = resize(t_movie_rect);
			if (flags & F_SHOW_BORDER)
				trect = MCU_reduce_rect(trect, -borderwidth);
			setrect(trect);
		}
    
	return True;
}

void MCPlayer::getnodes(MCExecPoint &ep)
{
	ep.clear();
	// COCOA-TODO: MCPlayer::getnodes();
}

void MCPlayer::gethotspots(MCExecPoint &ep)
{
	ep.clear();
	// COCOA-TODO: MCPlayer::gethotspots();
}


MCRectangle MCPlayer::resize(MCRectangle movieRect)
{
	int2 x, y;
	MCRectangle trect = rect;
	
	// MW-2011-10-24: [[ Bug 9800 ]] Store the current rect for layer notification.
	MCRectangle t_old_rect;
	t_old_rect = rect;
	
	// MW-2011-10-01: [[ Bug 9762 ]] These got inverted sometime.
	formattedheight = movieRect.height;
	formattedwidth = movieRect.width;
	
	if (!(flags & F_LOCK_LOCATION))
	{
		if (formattedheight == 0)
		{ // audio clip
			trect.height = CONTROLLER_HEIGHT;
			rect = trect;
		}
		else
		{
			x = trect.x + (trect.width >> 1);
			y = trect.y + (trect.height >> 1);
			trect.width = (uint2)(formattedwidth * scale);
			trect.height = (uint2)(formattedheight * scale);
            
            if (flags & F_SHOW_CONTROLLER)
                trect.height += CONTROLLER_HEIGHT;
            
			trect.x = x - (trect.width >> 1);
			trect.y = y - (trect.height >> 1);
			if (flags & F_SHOW_BORDER)
				rect = MCU_reduce_rect(trect, -borderwidth);
			else
				rect = trect;
		}
	}
	else
		if (flags & F_SHOW_BORDER)
			trect = MCU_reduce_rect(trect, borderwidth);
	
	// MW-2011-10-24: [[ Bug 9800 ]] If the rect has changed, notify the layer.
	if (!MCU_equal_rect(rect, t_old_rect))
		layer_rectchanged(t_old_rect, true);
	
	return trect;
}

void MCPlayer::markerchanged(uint32_t p_time)
{
    // Search for the first marker with the given time, and dispatch the message.
    for(uindex_t i = 0; i < m_callback_count; i++)
        if (p_time == m_callbacks[i] . time)
        {
            MCExecPoint ep;
            ep . setnameref_unsafe(m_callbacks[i] . parameter);
            
            MCParameter *t_param;
            t_param = new MCParameter;
            t_param -> set_argument(ep);
            MCscreen -> addmessage(this, m_callbacks[i] . message, 0, t_param);
            
            // MW-2014-08-25: [[ Bug 13267 ]] Make sure we terminate the current wait so updates and messages get sent.
            MCPlatformBreakWait();
        }
}

void MCPlayer::selectionchanged(void)
{
    timer(MCM_selection_changed, nil);
}

void MCPlayer::currenttimechanged(void)
{
    if (m_modify_selection_while_playing)
    {
        if ((MCmodifierstate & MS_SHIFT) == 0)
            playpause(True);
        
        uint32_t t_current_time;
        t_current_time = getmoviecurtime();
        
        if (t_current_time < endtime && t_current_time > starttime)
            starttime = t_current_time;
        if (t_current_time > endtime)
            endtime = t_current_time;
        
        setselection(true);
    }
    
    // FG-2014-08-14: [[ Bug 13099 ]] redrawcontroller () should be called before currenttimechanged message is sent, or else player becomes unresponsive if alwaysbuffer is true
    redrawcontroller();
    
    // PM-2014-05-26: [[Bug 12512]] Make sure we pass the param to the currenttimechanged message
    if (!getstate(CS_CTC_PENDING))
    {
        state |= CS_CTC_PENDING;
        
        MCParameter *t_param;
        t_param = new MCParameter;
        t_param -> setn_argument(getmoviecurtime());
        MCscreen -> addmessage(this, MCM_current_time_changed, 0, t_param);
        
        // MW-2014-08-25: [[ Bug 13267 ]] Make sure we terminate the current wait so updates and messages get sent.
        MCPlatformBreakWait();
    }
}

void MCPlayer::moviefinished(void)
{
    // PM-2014-08-06: [[ Bug 13104 ]] Set rate to zero when movie finish
    rate = 0.0;
    // PM-2014-12-02: [[ Bug 14141 ]] Delay the playStopped message to prevent IDE hang in case where the player's filename is set in the playStopped message (AVFoundation does not like nested callbacks)
    MCscreen -> delaymessage(this, MCM_play_stopped);
}

void MCPlayer::SynchronizeUserCallbacks(void)
{
    if (userCallbackStr == nil)
        return;
    
    if (m_platform_player == nil)
        return;
    
    // Free the existing callback table.
    for(uindex_t i = 0; i < m_callback_count; i++)
    {
        MCNameDelete(m_callbacks[i] . message);
        MCNameDelete(m_callbacks[i] . parameter);
    }
    MCMemoryDeleteArray(m_callbacks);
    m_callbacks = nil;
    m_callback_count = 0;
    
    // Now reparse the callback string and build the table.
    char *cblist = strclone(userCallbackStr);
	char *str;
	str = cblist;
	while (*str)
	{
		char *ptr, *data1, *data2;
		if ((data1 = strchr(str, ',')) == NULL)
		{
            //search ',' as separator
			delete cblist;
			return;
		}
		*data1 = '\0';
		data1 ++;
		if ((data2 = strchr(data1, '\n')) != NULL)// more than one callback
			*data2++ = '\0';
		else
			data2 = data1 + strlen(data1);
        
        /* UNCHECKED */ MCMemoryResizeArray(m_callback_count + 1, m_callbacks, m_callback_count);
        m_callbacks[m_callback_count - 1] . time = strtol(str, NULL, 10);
        
        while (isspace(*data1))//strip off preceding and trailing blanks
            data1++;
        ptr = data1;
        while (*ptr)
        {
            if (isspace(*ptr))
            {
                *ptr++ = '\0';
                /* UNCHECKED */ MCNameCreateWithCString(ptr, m_callbacks[m_callback_count - 1] . parameter);
                break;
            }
            ptr++;
        }
        
        /* UNCHECKED */ MCNameCreateWithCString(data1, m_callbacks[m_callback_count - 1] . message);
        
        // If no parameter is specified, use the time.
        if (m_callbacks[m_callback_count - 1] . parameter == nil)
        /* UNCHECKED */ MCNameCreateWithCString(str, m_callbacks[m_callback_count - 1] . parameter);
		
        str = data2;
	}
	delete cblist;
    
    if (!hasfilename())
        return True;
    
    // Now set the markers in the player so that we get notified.
    array_t<uint32_t> t_markers;
    /* UNCHECKED */ MCMemoryNewArray(m_callback_count, t_markers . ptr);
    for(uindex_t i = 0; i < m_callback_count; i++)
        t_markers . ptr[i] = m_callbacks[i] . time;
    t_markers . count = m_callback_count;
    MCPlatformSetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyMarkers, kMCPlatformPropertyTypeUInt32Array, &t_markers);
    MCMemoryDeleteArray(t_markers . ptr);
    
	return True;
}

Boolean MCPlayer::isbuffering(void)
{
	if (m_platform_player == nil || !hasfilename())
		return false;
	
	bool t_buffering;
	MCPlatformGetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyOffscreen, kMCPlatformPropertyTypeBool, &t_buffering);
	
	return t_buffering;
}


//-----------------------------------------------------------------------------
//  Redraw Management

// MW-2011-09-06: [[ Redraw ]] Added 'sprite' option - if true, ink and opacity are not set.
void MCPlayer::draw(MCDC *dc, const MCRectangle& p_dirty, bool p_isolated, bool p_sprite)
{
	MCRectangle dirty;
	dirty = p_dirty;
    
	if (!p_isolated)
	{
		// MW-2011-09-06: [[ Redraw ]] If rendering as a sprite, don't change opacity or ink.
		if (!p_sprite)
		{
			dc -> setopacity(blendlevel * 255 / 100);
			dc -> setfunction(ink);
		}
        
		// MW-2009-06-11: [[ Bitmap Effects ]]
		if (m_bitmap_effects == NULL)
			dc -> begin(false);
		else
		{
			if (!dc -> begin_with_effects(m_bitmap_effects, rect))
				return;
			dirty = dc -> getclip();
		}
	}
    
	if (MClook == LF_MOTIF && state & CS_KFOCUSED && !(extraflags & EF_NO_FOCUS_BORDER))
		drawfocus(dc, p_dirty);
    
    //if (!(state & CS_CLOSING))
		//prepare(MCnullstring);
	
	if (m_platform_player != nil && hasfilename())
	{
        // SN-2014-08-25: [[ Bug 13187 ]] syncbuffering relocated
        //syncbuffering(dc);
        
		bool t_offscreen;
		MCPlatformGetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyOffscreen, kMCPlatformPropertyTypeBool, &t_offscreen);
		
		if (t_offscreen)
		{
			MCRectangle trect = MCU_reduce_rect(rect, flags & F_SHOW_BORDER ? borderwidth : 0);
            
			MCImageDescriptor t_image;
			MCMemoryClear(&t_image, sizeof(t_image));
			t_image.filter = kMCGImageFilterNone;
            
			// IM-2014-05-14: [[ ImageRepUpdate ]] Wrap locked bitmap in MCGImage
			MCImageBitmap *t_bitmap = nil;
			MCPlatformLockPlayerBitmap(m_platform_player, t_bitmap);
            
			MCGRaster t_raster = MCImageBitmapGetMCGRaster(t_bitmap, true);
            
            // SN-2014-08-25: [[ Bug 13187 ]] We need to copy the raster
            if (dc -> gettype() == CONTEXT_TYPE_PRINTER)
                MCGImageCreateWithRaster(t_raster, t_image.image);
            else
                MCGImageCreateWithRasterNoCopy(t_raster, t_image.image);
			if (t_image . image != nil)
				dc -> drawimage(t_image, 0, 0, trect.width, trect.height, trect.x, trect.y);
			MCGImageRelease(t_image.image);
			MCPlatformUnlockPlayerBitmap(m_platform_player, t_bitmap);
		}
	}
 
    // Draw our controller
    if (getflag(F_SHOW_CONTROLLER))
    {
        drawcontroller(dc);
    }
    
	if (getflag(F_SHOW_BORDER))
		if (getflag(F_3D))
			draw3d(dc, rect, ETCH_SUNKEN, borderwidth);
		else
			drawborder(dc, rect, borderwidth);
	
	if (!p_isolated)
	{
		if (getstate(CS_SELECTED))
			drawselected(dc);
	}
    
	if (!p_isolated)
		dc -> end();
}


void MCPlayer::drawcontroller(MCDC *dc)
{
    MCRectangle t_rect;
    t_rect = getcontrollerrect();
    
    // Adjust to cover empty pixels
    t_rect . x --;
    t_rect . y --;
    t_rect . width ++;
    t_rect . height ++;
    
    // SN-2014-08-25: [[ Bug 13187 ]] We need to clip to the size of the controller
    dc -> save();
    dc -> cliprect(t_rect);
    
    MCGContextRef t_gcontext = nil;
    dc -> lockgcontext(t_gcontext);
    
    // SN-2014-08-25: [[ Bug 13187 ]] Fill up the controller background color after clipping
    MCGContextAddRectangle(t_gcontext, MCRectangleToMCGRectangle(t_rect));
    MCGContextSetFillRGBAColor(t_gcontext, (controllerbackcolor . red / 255.0) / 257.0, (controllerbackcolor . green / 255.0) / 257.0, (controllerbackcolor . blue / 255.0) / 257.0, 1.0f);
    MCGContextFill(t_gcontext);
    
    drawControllerVolumeButton(t_gcontext);
    
    drawControllerPlayPauseButton(t_gcontext);
    drawControllerWellButton(t_gcontext);
    drawControllerBufferedAreaButton(t_gcontext);
    
    // PM-2014-07-15 [[ Bug 12818 ]] If the duration of the selection is 0 then selection handles are invisible
    if (getflag(F_SHOW_SELECTION) && endtime - starttime != 0)
    {
        drawControllerSelectedAreaButton(t_gcontext);
    }
    
    drawControllerScrubForwardButton(t_gcontext);
    drawControllerScrubBackButton(t_gcontext);
    
    drawControllerPlayedAreaButton(t_gcontext);
    
    // PM-2014-07-15 [[ Bug 12818 ]] If the duration of the selection is 0 then selection handles are invisible
    if (getflag(F_SHOW_SELECTION) && endtime - starttime != 0)
    {
        drawControllerSelectionStartButton(t_gcontext);
        drawControllerSelectionFinishButton(t_gcontext);
    }
    
    drawControllerThumbButton(t_gcontext);
    
    dc -> unlockgcontext(t_gcontext);
    
    // SN-2014-08-25: [[ Bug 13187 ]] Restore the context to its previous state
    dc -> restore();
}

void MCPlayer::drawControllerVolumeButton(MCGContextRef p_gcontext)
{
    MCRectangle t_rect;
    t_rect = getcontrollerrect();
    MCRectangle t_volume_rect = getcontrollerpartrect(t_rect, kMCPlayerControllerPartVolume);
        
    if (m_show_volume)
    {
        MCGContextAddRectangle(p_gcontext, MCRectangleToMCGRectangle(t_volume_rect));
        MCGContextSetFillRGBAColor(p_gcontext, (controllermaincolor . red / 255.0) / 257.0, (controllermaincolor . green / 255.0) / 257.0, (controllermaincolor . blue / 255.0) / 257.0, 1.0f);
        MCGContextFill(p_gcontext);
    }
    
    MCGContextSetFillRGBAColor(p_gcontext, 257 / 257.0, 257 / 257.0, 257 / 257.0, 1.0f); // WHITE

    MCGContextSetShouldAntialias(p_gcontext, true);
    
    MCGContextBeginPath(p_gcontext);
    MCGContextMoveTo(p_gcontext, MCRectangleScalePoints(t_volume_rect, 0.28 , 0.4));
    MCGContextLineTo(p_gcontext, MCRectangleScalePoints(t_volume_rect, 0.28 , 0.6));
    MCGContextLineTo(p_gcontext, MCRectangleScalePoints(t_volume_rect, 0.4 , 0.6));
    MCGContextLineTo(p_gcontext, MCRectangleScalePoints(t_volume_rect, 0.5 , 0.7));
    MCGContextLineTo(p_gcontext, MCRectangleScalePoints(t_volume_rect, 0.5 , 0.3));
    MCGContextLineTo(p_gcontext, MCRectangleScalePoints(t_volume_rect, 0.4 , 0.4));
    MCGContextCloseSubpath(p_gcontext);
    MCGContextFill(p_gcontext);
    
    if (getloudness() > 30)
    {
        MCGContextMoveTo(p_gcontext, MCRectangleScalePoints(t_volume_rect, 0.6 , 0.4));
        MCGContextLineTo(p_gcontext, MCRectangleScalePoints(t_volume_rect, 0.6 , 0.6));
    }
    
    if (getloudness() > 60)
    {
        MCGContextMoveTo(p_gcontext, MCRectangleScalePoints(t_volume_rect, 0.7 , 0.35));
        MCGContextLineTo(p_gcontext, MCRectangleScalePoints(t_volume_rect, 0.7 , 0.65));
    }
    
    if (getloudness() > 95)
    {
        MCGContextMoveTo(p_gcontext, MCRectangleScalePoints(t_volume_rect, 0.8 , 0.3));
        MCGContextLineTo(p_gcontext, MCRectangleScalePoints(t_volume_rect, 0.8 , 0.7));
    }
    
    MCGContextSetStrokeRGBAColor(p_gcontext, 257 / 257.0, 257 / 257.0, 257 / 257.0, 1.0f); // WHITE
    MCGContextSetStrokeWidth(p_gcontext, t_volume_rect . width / 20.0 );
    MCGContextStroke(p_gcontext);
}

void MCPlayer::drawControllerPlayPauseButton(MCGContextRef p_gcontext)
{
    MCRectangle t_rect;
    t_rect = getcontrollerrect();
    MCRectangle t_playpause_rect = getcontrollerpartrect(t_rect, kMCPlayerControllerPartPlay);
    MCGContextSetShouldAntialias(p_gcontext, true);
    
    if (ispaused())
    {
        MCGContextMoveTo(p_gcontext, MCRectangleScalePoints(t_playpause_rect, 0.35, 0.3));
        MCGContextLineTo(p_gcontext, MCRectangleScalePoints(t_playpause_rect, 0.35, 0.7));
        MCGContextLineTo(p_gcontext, MCRectangleScalePoints(t_playpause_rect, 0.68, 0.5));
        MCGContextCloseSubpath(p_gcontext);
    }
    
    else
    {
        MCGRectangle t_grect1, t_grect2;
        
        t_grect1 = MCGRectangleMake(t_playpause_rect . x + 0.3 * t_playpause_rect . width, t_playpause_rect . y + 0.3 * t_playpause_rect . height, 0.15 * t_playpause_rect . width, 0.4 * t_playpause_rect . height);
        MCGContextAddRectangle(p_gcontext, t_grect1);
        
        t_grect2 = MCGRectangleMake(t_playpause_rect . x + 0.55 * t_playpause_rect . width, t_playpause_rect . y + 0.3 * t_playpause_rect . height, 0.15 * t_playpause_rect . width, 0.4 * t_playpause_rect . height);
        MCGContextAddRectangle(p_gcontext, t_grect2);
        
    }
    
    MCGContextSetFillRGBAColor(p_gcontext, 257 / 257.0, 257 / 257.0, 257 / 257.0, 1.0f); // WHITE
    MCGContextFill(p_gcontext);
}

void MCPlayer::drawControllerWellButton(MCGContextRef p_gcontext)
{
    MCRectangle t_rect;
    t_rect = getcontrollerrect();
    MCRectangle t_drawn_well_rect = getcontrollerpartrect(t_rect, kMCPlayerControllerPartWell);
    // Adjust to look prettier. The same settings for y and height should apply to kMCPlayerControllerPartSelectedArea and kMCPlayerControllerPartPlayedArea
    t_drawn_well_rect . y = t_drawn_well_rect . y + 2 * CONTROLLER_HEIGHT / 5;
    t_drawn_well_rect . height = CONTROLLER_HEIGHT / 5;
    
    // PM-2014-07-17: [[ Bug 12833 ]] Reduce the length of the drawn well so as to fix alignment issues with the start/end point of selectedArea and playedArea 
    t_drawn_well_rect . x += 4;
    t_drawn_well_rect . width -= 10;
    
    MCGBitmapEffects t_effects;
	t_effects . has_drop_shadow = false;
	t_effects . has_outer_glow = false;
	t_effects . has_inner_glow = false;
	t_effects . has_inner_shadow = true;
    t_effects . has_color_overlay = false;
    
    MCGShadowEffect t_inner_shadow;
    t_inner_shadow . color = MCGColorMakeRGBA(56.0 / 255.0, 56.0 / 255.0, 56.0 / 255.0, 56.0 / 255.0);
    t_inner_shadow . blend_mode = kMCGBlendModeClear;
    t_inner_shadow . size = 0;
    t_inner_shadow . spread = 0;
    
    MCGFloat t_x_offset, t_y_offset;
    int t_distance = t_drawn_well_rect . height / 5;
    
    // Make sure we always have an inner shadow
    if (t_distance == 0)
        t_distance = 1;
    
    MCGraphicsContextAngleAndDistanceToXYOffset(270, t_distance, t_x_offset, t_y_offset);
    
    t_inner_shadow . x_offset = t_x_offset;
    t_inner_shadow . y_offset = t_y_offset;
    t_inner_shadow . knockout = false;
    
    t_effects . inner_shadow = t_inner_shadow;
  
    MCGContextSetShouldAntialias(p_gcontext, true);
    
    MCGContextSetFillRGBAColor(p_gcontext, 0.0f, 0.0f, 0.0f, 1.0f); // BLACK
    MCGRectangle t_rounded_rect = MCRectangleToMCGRectangle(t_drawn_well_rect);
    
    MCGContextAddRoundedRectangle(p_gcontext, t_rounded_rect, MCGSizeMake(10, 10));
    
    MCGContextBeginWithEffects(p_gcontext, t_rounded_rect, t_effects);
    
    MCGContextFill(p_gcontext);

    /////////////////////////////////////////
    /* TODO: Update this ugly way of adding the inner shadow 'manually' */
    MCRectangle t_shadow = MCRectangleMake(t_drawn_well_rect . x + 1, t_drawn_well_rect . y + t_drawn_well_rect . height - 2, t_drawn_well_rect . width - 2, 2);
    
    MCGContextSetShouldAntialias(p_gcontext, true);
    
    MCGContextSetFillRGBAColor(p_gcontext, 56.0 / 255.0, 56.0 / 255.0, 56.0 / 255.0, 1.0f); // GRAY
    MCGRectangle t_shadow_rect = MCRectangleToMCGRectangle(t_shadow);
    
    MCGContextAddRoundedRectangle(p_gcontext, t_shadow_rect, MCGSizeMake(10, 10));
    
    MCGContextFill(p_gcontext);
    ////////////////////////////////////////
    
    MCGContextEnd(p_gcontext);
}

void MCPlayer::drawControllerThumbButton(MCGContextRef p_gcontext)
{
    MCRectangle t_rect;
    t_rect = getcontrollerrect();
    MCRectangle t_drawn_thumb_rect = getcontrollerpartrect(t_rect, kMCPlayerControllerPartThumb);
    // Adjust to look prettier. Note that these adjustments should match hittestcontroller in cases where thumb overlaps with selectionStart/selectionFinish handles
    t_drawn_thumb_rect . y = t_drawn_thumb_rect . y + 2 * CONTROLLER_HEIGHT / 7;
    t_drawn_thumb_rect . height = 3 * CONTROLLER_HEIGHT / 7;
    t_drawn_thumb_rect . width = CONTROLLER_HEIGHT / 3;
       
    MCAutoPointer<MCGColor> t_colors;
    MCAutoPointer<MCGFloat> t_stops;
    setRamp(&t_colors, &t_stops);
    
    MCGAffineTransform t_transform;
    
    float origin_x = t_drawn_thumb_rect.x + t_drawn_thumb_rect.width / 2.0;
	float origin_y = t_drawn_thumb_rect.y + t_drawn_thumb_rect.height;
	float primary_x = t_drawn_thumb_rect.x + t_drawn_thumb_rect.width / 2.0;
	float primary_y = t_drawn_thumb_rect.y;
	float secondary_x = t_drawn_thumb_rect.x - t_drawn_thumb_rect.width / 2.0;
	float secondary_y = t_drawn_thumb_rect.y + t_drawn_thumb_rect.height;
    
    setTransform(t_transform, origin_x, origin_y, primary_x, primary_y, secondary_x, secondary_y);
    
    ////////////////////////////////////////////////////////
    
    ///////////////////////////////////////////////////////
    
    MCGContextSetFillGradient(p_gcontext, kMCGGradientFunctionLinear, *t_stops, *t_colors, 3, false, false, 1, t_transform, kMCGImageFilterNone);
    
    MCGContextSetShouldAntialias(p_gcontext, true);
    
    MCGContextAddArc(p_gcontext, MCRectangleScalePoints(t_drawn_thumb_rect, 0.5, 0.5), MCGSizeMake(1.2 * t_drawn_thumb_rect . width, 0.8 * t_drawn_thumb_rect . height), 0, 0, 360);
    
    MCGContextFill(p_gcontext);
}

void MCPlayer::drawControllerSelectionStartButton(MCGContextRef p_gcontext)
{
    MCRectangle t_rect;
    t_rect = getcontrollerrect();
    MCRectangle t_drawn_selection_start_rect = getcontrollerpartrect(t_rect, kMCPlayerControllerPartSelectionStart);
    
    MCGContextSetShouldAntialias(p_gcontext, true);
    
    MCGContextBeginPath(p_gcontext);
    
    // PM-2014-07-16: [[ Bug 12816 ]] Change the appearance of selection handles so as not to obscure player thumb
    MCGContextMoveTo(p_gcontext, MCRectangleScalePoints(t_drawn_selection_start_rect, 0.3, 0.05));
    MCGContextLineTo(p_gcontext, MCRectangleScalePoints(t_drawn_selection_start_rect, 0.7, 0.05));
    MCGContextLineTo(p_gcontext, MCRectangleScalePoints(t_drawn_selection_start_rect, 0.7, 0.1));
    MCGContextLineTo(p_gcontext, MCRectangleScalePoints(t_drawn_selection_start_rect, 0.5, 0.25));
    MCGContextLineTo(p_gcontext, MCRectangleScalePoints(t_drawn_selection_start_rect, 0.3, 0.1));
    MCGContextCloseSubpath(p_gcontext);
    
    MCGContextMoveTo(p_gcontext, MCRectangleScalePoints(t_drawn_selection_start_rect, 0.3, 0.93));
    MCGContextLineTo(p_gcontext, MCRectangleScalePoints(t_drawn_selection_start_rect, 0.7, 0.93));
    MCGContextLineTo(p_gcontext, MCRectangleScalePoints(t_drawn_selection_start_rect, 0.7, 0.88));
    MCGContextLineTo(p_gcontext, MCRectangleScalePoints(t_drawn_selection_start_rect, 0.5, 0.73));
    MCGContextLineTo(p_gcontext, MCRectangleScalePoints(t_drawn_selection_start_rect, 0.3, 0.88));
    MCGContextCloseSubpath(p_gcontext);

    
    MCGContextSetFillRGBAColor(p_gcontext, 257 / 257.0, 257 / 257.0, 257 / 257.0, 1.0f); // WHITE
    MCGContextFill(p_gcontext);
}

void MCPlayer::drawControllerSelectionFinishButton(MCGContextRef p_gcontext)
{
    MCRectangle t_rect;
    t_rect = getcontrollerrect();
    MCRectangle t_drawn_selection_finish_rect = getcontrollerpartrect(t_rect, kMCPlayerControllerPartSelectionFinish);
    
    MCGContextSetShouldAntialias(p_gcontext, true);
    
    MCGContextBeginPath(p_gcontext);
    
    // PM-2014-07-16: [[ Bug 12816 ]] Change the appearance of selection handles so as not to obscure player thumb
    MCGContextMoveTo(p_gcontext, MCRectangleScalePoints(t_drawn_selection_finish_rect, 0.3, 0.05));
    MCGContextLineTo(p_gcontext, MCRectangleScalePoints(t_drawn_selection_finish_rect, 0.7, 0.05));
    MCGContextLineTo(p_gcontext, MCRectangleScalePoints(t_drawn_selection_finish_rect, 0.7, 0.1));
    MCGContextLineTo(p_gcontext, MCRectangleScalePoints(t_drawn_selection_finish_rect, 0.5, 0.25));
    MCGContextLineTo(p_gcontext, MCRectangleScalePoints(t_drawn_selection_finish_rect, 0.3, 0.1));
    MCGContextCloseSubpath(p_gcontext);
    
    MCGContextMoveTo(p_gcontext, MCRectangleScalePoints(t_drawn_selection_finish_rect, 0.3, 0.93));
    MCGContextLineTo(p_gcontext, MCRectangleScalePoints(t_drawn_selection_finish_rect, 0.7, 0.93));
    MCGContextLineTo(p_gcontext, MCRectangleScalePoints(t_drawn_selection_finish_rect, 0.7, 0.88));
    MCGContextLineTo(p_gcontext, MCRectangleScalePoints(t_drawn_selection_finish_rect, 0.5, 0.73));
    MCGContextLineTo(p_gcontext, MCRectangleScalePoints(t_drawn_selection_finish_rect, 0.3, 0.88));
    MCGContextCloseSubpath(p_gcontext);
    
    MCGContextSetFillRGBAColor(p_gcontext, 257 / 257.0, 257 / 257.0, 257 / 257.0, 1.0f); // WHITE
    MCGContextFill(p_gcontext);

}

void MCPlayer::drawControllerScrubForwardButton(MCGContextRef p_gcontext)
{
    MCRectangle t_rect;
    t_rect = getcontrollerrect();
    MCRectangle t_scrub_forward_rect = getcontrollerpartrect(t_rect, kMCPlayerControllerPartScrubForward);
    
    if (m_scrub_forward_is_pressed)
    {
        MCGContextAddRectangle(p_gcontext, MCRectangleToMCGRectangle(t_scrub_forward_rect));
        MCGContextSetFillRGBAColor(p_gcontext, (controllermaincolor . red / 255.0) / 257.0, (controllermaincolor . green / 255.0) / 257.0, (controllermaincolor . blue / 255.0) / 257.0, 1.0f);
        MCGContextFill(p_gcontext);
    }
    
    MCGContextSetShouldAntialias(p_gcontext, true);
    
    MCGRectangle t_grect;
    t_grect = MCGRectangleMake(t_scrub_forward_rect . x + 0.3 * t_scrub_forward_rect . width, t_scrub_forward_rect . y + 0.3 * t_scrub_forward_rect . height, 0.15 * t_scrub_forward_rect . width, 0.4 * t_scrub_forward_rect . height);
    MCGContextBeginPath(p_gcontext);
    MCGContextAddRectangle(p_gcontext, t_grect);
    
    MCGContextMoveTo(p_gcontext, MCRectangleScalePoints(t_scrub_forward_rect, 0.55, 0.3));
    MCGContextLineTo(p_gcontext, MCRectangleScalePoints(t_scrub_forward_rect, 0.55, 0.7));
    MCGContextLineTo(p_gcontext, MCRectangleScalePoints(t_scrub_forward_rect, 0.75, 0.5));
    MCGContextCloseSubpath(p_gcontext);
     
    MCGContextSetFillRGBAColor(p_gcontext, 257 / 257.0, 257 / 257.0, 257 / 257.0, 1.0f); // WHITE
    MCGContextFill(p_gcontext);
}

void MCPlayer::drawControllerScrubBackButton(MCGContextRef p_gcontext)
{
    MCRectangle t_rect;
    t_rect = getcontrollerrect();
    MCRectangle t_scrub_back_rect = getcontrollerpartrect(t_rect, kMCPlayerControllerPartScrubBack);
    
    if (m_scrub_back_is_pressed)
    {
        MCGContextAddRectangle(p_gcontext, MCRectangleToMCGRectangle(t_scrub_back_rect));
        MCGContextSetFillRGBAColor(p_gcontext, (controllermaincolor . red / 255.0) / 257.0, (controllermaincolor . green / 255.0) / 257.0, (controllermaincolor . blue / 255.0) / 257.0, 1.0f);
        MCGContextFill(p_gcontext);
    }
    
    MCGContextSetShouldAntialias(p_gcontext, true);
    MCGContextBeginPath(p_gcontext);
    MCGContextMoveTo(p_gcontext, MCRectangleScalePoints(t_scrub_back_rect, 0.2, 0.5));
    MCGContextLineTo(p_gcontext, MCRectangleScalePoints(t_scrub_back_rect, 0.4, 0.3));
    MCGContextLineTo(p_gcontext, MCRectangleScalePoints(t_scrub_back_rect, 0.4, 0.7));
    
    MCGRectangle t_grect;
    t_grect = MCGRectangleMake(t_scrub_back_rect . x + 0.5 * t_scrub_back_rect . width, t_scrub_back_rect . y + 0.3 * t_scrub_back_rect . height, 0.15 * t_scrub_back_rect . width, 0.4 * t_scrub_back_rect . height);
    
    MCGContextAddRectangle(p_gcontext, t_grect);
    MCGContextCloseSubpath(p_gcontext);
    
    MCGContextSetFillRGBAColor(p_gcontext, 257 / 257.0, 257 / 257.0, 257 / 257.0, 1.0f); // WHITE
    MCGContextFill(p_gcontext);
}

void MCPlayer::drawControllerSelectedAreaButton(MCGContextRef p_gcontext)
{
    MCRectangle t_drawn_selected_area;
    t_drawn_selected_area = getcontrollerpartrect(getcontrollerrect(), kMCPlayerControllerPartSelectedArea);
    // Adjust to look prettier. The same settings for y and height should apply to kMCPlayerControllerPartWell and kMCPlayerControllerPartPlayedArea
    t_drawn_selected_area . y = t_drawn_selected_area . y + 3 * CONTROLLER_HEIGHT / 7;
    t_drawn_selected_area . height = CONTROLLER_HEIGHT / 7;
    
    MCGContextAddRectangle(p_gcontext, MCRectangleToMCGRectangle(t_drawn_selected_area));
    MCGContextSetFillRGBAColor(p_gcontext, (selectedareacolor . red / 255.0) / 257.0, (selectedareacolor . green / 255.0) / 257.0, (selectedareacolor . blue / 255.0) / 257.0, 1.0f);
    MCGContextFill(p_gcontext);
}

void MCPlayer::drawControllerPlayedAreaButton(MCGContextRef p_gcontext)
{
    MCRectangle t_drawn_played_area;
    t_drawn_played_area = getcontrollerpartrect(getcontrollerrect(), kMCPlayerControllerPartPlayedArea);
    // Adjust to look prettier. The same settings for y and height should apply to kMCPlayerControllerPartWell and kMCPlayerControllerPartSelectedArea
    t_drawn_played_area . y = t_drawn_played_area . y + 3 * CONTROLLER_HEIGHT / 7;
    t_drawn_played_area . height = CONTROLLER_HEIGHT / 7;
    t_drawn_played_area . x--;

    
    MCGContextSetFillRGBAColor(p_gcontext, (controllermaincolor . red / 255.0) / 257.0, (controllermaincolor . green / 255.0) / 257.0, (controllermaincolor . blue / 255.0) / 257.0, 1.0f);
    
    MCGRectangle t_rounded_rect = MCRectangleToMCGRectangle(t_drawn_played_area);
    MCGContextAddRoundedRectangle(p_gcontext, t_rounded_rect, MCGSizeMake(30, 30));    
    MCGContextFill(p_gcontext);
}

void MCPlayer::drawControllerBufferedAreaButton(MCGContextRef p_gcontext)
{
    MCRectangle t_drawn_buffered_area;
    t_drawn_buffered_area = getcontrollerpartrect(getcontrollerrect(), kMCPlayerControllerPartBuffer);
    // Adjust to look prettier. The same settings for y and height should apply to kMCPlayerControllerPartWell and kMCPlayerControllerPartSelectedArea
    t_drawn_buffered_area . y = t_drawn_buffered_area . y + 3 * CONTROLLER_HEIGHT / 7;
    t_drawn_buffered_area . height = CONTROLLER_HEIGHT / 7;
    t_drawn_buffered_area . x--;
    
    
    MCGContextSetFillRGBAColor(p_gcontext, 62 / 257.0, 62 / 257.0, 62 / 257.0, 1.0f); // Some DARK GREY
    
    MCGRectangle t_rounded_rect = MCRectangleToMCGRectangle(t_drawn_buffered_area);
    MCGContextAddRoundedRectangle(p_gcontext, t_rounded_rect, MCGSizeMake(30, 30));
    MCGContextFill(p_gcontext);
}


int MCPlayer::hittestcontroller(int x, int y)
{
    MCRectangle t_rect;
    t_rect = getcontrollerrect();
    
    // PM-2014-07-16 [[ Bug 12816 ]] Handle case where player thumb and selection handles overlap
    if (MCU_point_in_rect(getcontrollerpartrect(t_rect, kMCPlayerControllerPartThumb), x, y) && MCU_point_in_rect(getcontrollerpartrect(t_rect, kMCPlayerControllerPartSelectionStart), x, y))
    {
        MCRectangle t_thumb_rect;
        t_thumb_rect = getcontrollerpartrect(getcontrollerrect(), kMCPlayerControllerPartThumb);
        
        // Look in drawControllerThumbButton and match the dimensions of the drawn thumb rect
        MCRectangle t_drawn_thumb_rect;
        t_drawn_thumb_rect = MCRectangleMake(t_thumb_rect . x, t_thumb_rect . y + 2 * CONTROLLER_HEIGHT / 7, CONTROLLER_HEIGHT / 3, 3 * CONTROLLER_HEIGHT / 7);
        
        if (MCU_point_in_rect(t_drawn_thumb_rect, x, y))
            return kMCPlayerControllerPartThumb;
        else
            return kMCPlayerControllerPartSelectionStart;
    }
    
    else if (MCU_point_in_rect(getcontrollerpartrect(t_rect, kMCPlayerControllerPartThumb), x, y) && MCU_point_in_rect(getcontrollerpartrect(t_rect, kMCPlayerControllerPartSelectionFinish), x, y))
    {
        MCRectangle t_thumb_rect;
        t_thumb_rect = getcontrollerpartrect(getcontrollerrect(), kMCPlayerControllerPartThumb);
        
        // Look in drawControllerThumbButton and match the dimensions of the drawn thumb rect
        MCRectangle t_drawn_thumb_rect;
        t_drawn_thumb_rect = MCRectangleMake(t_thumb_rect . x, t_thumb_rect . y + 2 * CONTROLLER_HEIGHT / 7, CONTROLLER_HEIGHT / 3, 3 * CONTROLLER_HEIGHT / 7);
        
        if (MCU_point_in_rect(t_drawn_thumb_rect, x, y))
            return kMCPlayerControllerPartThumb;
        else
            return kMCPlayerControllerPartSelectionFinish;
    }
    
    else if (MCU_point_in_rect(getcontrollerpartrect(t_rect, kMCPlayerControllerPartPlay), x, y))
        return kMCPlayerControllerPartPlay;
    
    else if (MCU_point_in_rect(getcontrollerpartrect(t_rect, kMCPlayerControllerPartVolume), x, y))
        return kMCPlayerControllerPartVolume;
    
    else if (MCU_point_in_rect(getcontrollerpartrect(t_rect, kMCPlayerControllerPartScrubBack), x, y))
        return kMCPlayerControllerPartScrubBack;
    
    else if (MCU_point_in_rect(getcontrollerpartrect(t_rect, kMCPlayerControllerPartScrubForward), x, y))
        return kMCPlayerControllerPartScrubForward;
    
    else if (MCU_point_in_rect(getcontrollerpartrect(t_rect, kMCPlayerControllerPartThumb), x, y))
        return kMCPlayerControllerPartThumb;
    
    else if (MCU_point_in_rect(getcontrollerpartrect(t_rect, kMCPlayerControllerPartSelectionStart), x, y))
        return kMCPlayerControllerPartSelectionStart;
    
    else if (MCU_point_in_rect(getcontrollerpartrect(t_rect, kMCPlayerControllerPartSelectionFinish), x, y))
        return kMCPlayerControllerPartSelectionFinish;
    
    else if (MCU_point_in_rect(getcontrollerpartrect(t_rect, kMCPlayerControllerPartWell), x, y))
        return kMCPlayerControllerPartWell;
    
    else if (MCU_point_in_rect(getcontrollerpartrect(t_rect, kMCPlayerControllerPartVolumeSelector), x, y))
        return kMCPlayerControllerPartVolumeSelector;
    
    else if (MCU_point_in_rect(getcontrollerpartrect(t_rect, kMCPlayerControllerPartVolumeWell), x, y))
        return kMCPlayerControllerPartVolumeWell;
    
    else if (MCU_point_in_rect(getcontrollerpartrect(t_rect, kMCPlayerControllerPartVolumeBar), x, y))
        return kMCPlayerControllerPartVolumeBar;
    
    else
        return kMCPlayerControllerPartUnknown;
}

void MCPlayer::drawcontrollerbutton(MCDC *dc, const MCRectangle& p_rect)
{
    dc -> setforeground(dc -> getwhite());
    dc -> fillrect(p_rect, true);
    
    dc -> setforeground(dc -> getblack());
    dc -> setlineatts(1, LineSolid, CapButt, JoinMiter);
    
    dc -> drawrect(p_rect, true);
}

MCRectangle MCPlayer::getcontrollerrect(void)
{
    MCRectangle t_rect;
    t_rect = rect;
    
    if (getflag(F_SHOW_BORDER))
        t_rect = MCU_reduce_rect(t_rect, borderwidth);
    
    t_rect . y = t_rect . y + t_rect . height - CONTROLLER_HEIGHT;
    t_rect . height = CONTROLLER_HEIGHT;
    
    return t_rect;
}

MCRectangle MCPlayer::getcontrollerpartrect(const MCRectangle& p_rect, int p_part)
{
    switch(p_part)
    {
        case kMCPlayerControllerPartVolume:
            return MCRectangleMake(p_rect . x, p_rect . y, CONTROLLER_HEIGHT, CONTROLLER_HEIGHT);
        case kMCPlayerControllerPartPlay:
            return MCRectangleMake(p_rect . x + CONTROLLER_HEIGHT, p_rect . y, CONTROLLER_HEIGHT, CONTROLLER_HEIGHT);
            
        case kMCPlayerControllerPartThumb:
        {
            if (m_platform_player == nil)
                return MCRectangleMake(0, 0, 0, 0);
            
            uint32_t t_current_time, t_duration;
            t_current_time = getmoviecurtime();
            t_duration = getduration();
            
            if (t_current_time >= t_duration)
                t_current_time = t_duration;
            
            MCRectangle t_well_rect;
            t_well_rect = getcontrollerpartrect(p_rect, kMCPlayerControllerPartWell);
            
            // The width of the thumb is CONTROLLER_HEIGHT / 2
            int t_active_well_width;
            t_active_well_width = t_well_rect . width - CONTROLLER_HEIGHT / 2;
            
            int t_thumb_left = 0;
            if (t_duration != 0)
                // PM-2014-08-22 [[ Bug 13257 ]] Make sure t_thumb_left will not overflow
                t_thumb_left = _muludiv64(t_active_well_width, t_current_time, t_duration);
            
            return MCRectangleMake(t_well_rect . x + t_thumb_left + CONTROLLER_HEIGHT / 8 - 1, t_well_rect . y, CONTROLLER_HEIGHT / 2, t_well_rect . height);
        }
            break;
            
        case kMCPlayerControllerPartWell:
            // PM-2014-07-08: [[ Bug 12763 ]] Make sure controller elememts are not broken when player width becomes too small
            // Now, reducing the player width below a threshold results in gradually removing controller elements from right to left
            // i.e first the scrubBack/scrubForward buttons will be removed, then the well/thumb rects etc. This behaviour is similar to the old player that used QuickTime
            
            if (p_rect . width < 3 * CONTROLLER_HEIGHT)
                return MCRectangleMake(0,0,0,0);
        
            // PM-2014-07-17: [[ Bug 12835 ]] Adjustments to prevent selectedArea and playedArea to be drawn without taking into account the width of the well
            if (p_rect . width < PLAYER_MIN_WIDTH)
                return MCRectangleMake(p_rect . x + 2 * CONTROLLER_HEIGHT, p_rect . y, p_rect . width - 2 * CONTROLLER_HEIGHT, CONTROLLER_HEIGHT);
            
            // PM-2014-08-06 : [[ Bug 13006 ]] Make controller well slightly wider
            return MCRectangleMake(p_rect . x + 2 * CONTROLLER_HEIGHT, p_rect . y , p_rect . width - 4 * CONTROLLER_HEIGHT, CONTROLLER_HEIGHT );
            
        case kMCPlayerControllerPartScrubBack:
            // PM-2014-07-08: [[ Bug 12763 ]] Make sure controller elememts are not broken when player width becomes too small
            if (p_rect . width < PLAYER_MIN_WIDTH)
                return MCRectangleMake(0,0,0,0);
        
            return MCRectangleMake(p_rect . x + p_rect . width - 2 * CONTROLLER_HEIGHT, p_rect . y, CONTROLLER_HEIGHT, CONTROLLER_HEIGHT);
            
        case kMCPlayerControllerPartScrubForward:
            // PM-2014-07-08: [[ Bug 12763 ]] Make sure controller elememts are not broken when player width becomes too small
            if (p_rect . width < PLAYER_MIN_WIDTH)
                return MCRectangleMake(0,0,0,0);
            
            return MCRectangleMake(p_rect . x + p_rect . width - CONTROLLER_HEIGHT, p_rect . y, CONTROLLER_HEIGHT, CONTROLLER_HEIGHT);
            
        case kMCPlayerControllerPartSelectionStart:
        {
            uint32_t t_start_time, t_duration;
            t_start_time = getstarttime();
            t_duration = getduration();
            
            MCRectangle t_well_rect, t_thumb_rect;
            t_well_rect = getcontrollerpartrect(p_rect, kMCPlayerControllerPartWell);
            t_thumb_rect = getcontrollerpartrect(p_rect, kMCPlayerControllerPartThumb);
            
            int t_active_well_width;
            t_active_well_width = t_well_rect . width - t_thumb_rect . width;
            
            int t_selection_start_left = 0;
            if (t_duration != 0)
                // PM-2014-08-22 [[ Bug 13257 ]] Make sure t_selection_start_left will not overflow
                t_selection_start_left = _muludiv64(t_active_well_width, t_start_time, t_duration);
            
            return MCRectangleMake(t_well_rect . x + t_selection_start_left, t_well_rect . y, SELECTION_RECT_WIDTH, t_well_rect . height);
        }
            
        case kMCPlayerControllerPartSelectionFinish:
        {
            uint32_t t_finish_time, t_duration;
            t_finish_time = getendtime();
            t_duration = getduration();
            
            MCRectangle t_well_rect, t_thumb_rect;
            t_well_rect = getcontrollerpartrect(p_rect, kMCPlayerControllerPartWell);
            t_thumb_rect = getcontrollerpartrect(p_rect, kMCPlayerControllerPartThumb);
            
            int t_active_well_width;
            t_active_well_width = t_well_rect . width - t_thumb_rect . width;;
            
            int t_selection_finish_left = t_active_well_width;
            if (t_duration != 0)
                // PM-2014-08-22 [[ Bug 13257 ]] Make sure t_selection_finish_left will not overflow
                t_selection_finish_left = _muludiv64(t_active_well_width, t_finish_time, t_duration);
            
            
            // PM-2014-07-09: [[ Bug 12750 ]] Make sure progress thumb and selectionFinish handle light up
            return MCRectangleMake(t_well_rect . x + t_selection_finish_left, t_well_rect . y , SELECTION_RECT_WIDTH, t_well_rect . height);
        }
            break;
            
        case kMCPlayerControllerPartVolumeBar:
            return MCRectangleMake(p_rect . x , p_rect . y - 3 * CONTROLLER_HEIGHT, CONTROLLER_HEIGHT, 3 * CONTROLLER_HEIGHT);
            
        case kMCPlayerControllerPartVolumeSelector:
        {
            MCRectangle t_volume_well_rect = getcontrollerpartrect(getcontrollerrect(), kMCPlayerControllerPartVolumeWell);
            MCRectangle t_volume_bar_rect = getcontrollerpartrect(getcontrollerrect(), kMCPlayerControllerPartVolumeBar);
            
            // The width and height of the volumeselector are CONTROLLER_HEIGHT / 2
            int32_t t_actual_height = t_volume_well_rect . height - CONTROLLER_HEIGHT / 2;
            
            int32_t t_x_offset = t_volume_well_rect . y - t_volume_bar_rect . y;
            
            return MCRectangleMake(p_rect . x + CONTROLLER_HEIGHT / 4 , t_volume_well_rect . y + t_volume_well_rect . height - t_actual_height * loudness / 100 - CONTROLLER_HEIGHT / 2, CONTROLLER_HEIGHT / 2, CONTROLLER_HEIGHT / 2 );
        }
            break;
        case kMCPlayerControllerPartSelectedArea:
        {
            uint32_t t_start_time, t_finish_time, t_duration;
            t_start_time = getstarttime();
            t_finish_time = getendtime();
            t_duration = getduration();
            
            MCRectangle t_well_rect, t_thumb_rect;
            t_well_rect = getcontrollerpartrect(p_rect, kMCPlayerControllerPartWell);
            t_thumb_rect = getcontrollerpartrect(p_rect, kMCPlayerControllerPartThumb);
            
            int t_active_well_width;
            t_active_well_width = t_well_rect . width - t_thumb_rect . width;
            
            int t_selection_start_left, t_selection_finish_left;
            if (t_duration == 0)
            {
                t_selection_start_left = 0;
                t_selection_finish_left = t_active_well_width;
            }
            else
            {
                // PM-2014-08-22 [[ Bug 13257 ]] Make sure vars will not overflow
                t_selection_start_left = _muludiv64(t_active_well_width, t_start_time, t_duration);
                t_selection_finish_left = _muludiv64(t_active_well_width, t_finish_time, t_duration);
            }
            
            return MCRectangleMake(t_well_rect . x + t_selection_start_left + t_thumb_rect . width / 2, t_well_rect . y, t_selection_finish_left - t_selection_start_left, t_well_rect . height);
        }
            break;
            
        case kMCPlayerControllerPartVolumeArea:
        {
            MCRectangle t_volume_well_rect = getcontrollerpartrect(getcontrollerrect(), kMCPlayerControllerPartVolumeWell);
            MCRectangle t_volume_selector_rect = getcontrollerpartrect(getcontrollerrect(), kMCPlayerControllerPartVolumeSelector);
            int32_t t_bar_height = t_volume_well_rect . height;
            int32_t t_bar_width = t_volume_well_rect . width;
            int32_t t_width = t_volume_well_rect . width;
            
            int32_t t_x_offset = (t_bar_width - t_width) / 2;
            // Adjust y by 2 pixels
            return MCRectangleMake(t_volume_well_rect. x , t_volume_selector_rect . y + 2 , t_width, t_volume_well_rect . y + t_volume_well_rect . height - t_volume_selector_rect . y );
        }
            break;
            
        case kMCPlayerControllerPartPlayedArea:
        {
            uint32_t t_start_time, t_current_time, t_finish_time, t_duration;
            t_duration = getduration();
            
            // PM-2014-07-15 [[ Bug 12818 ]] If the duration of the selection is 0 then the player ignores the selection
            if (getflag(F_SHOW_SELECTION) && endtime - starttime != 0)
            {
                t_start_time = getstarttime();
                t_finish_time = getendtime();
            }
            else
            {
                t_start_time = 0;
                t_finish_time = t_duration;
            }
            
            t_current_time = getmoviecurtime();
            
            if (t_current_time == 0)
                t_current_time = t_start_time;
            if (t_current_time > t_finish_time)
                t_current_time = t_finish_time;
            if (t_current_time < t_start_time)
                t_current_time = t_start_time;
            
            MCRectangle t_well_rect, t_thumb_rect;
            t_well_rect = getcontrollerpartrect(p_rect, kMCPlayerControllerPartWell);
            t_thumb_rect = getcontrollerpartrect(p_rect, kMCPlayerControllerPartThumb);
            
            int t_active_well_width;
            t_active_well_width = t_well_rect . width - t_thumb_rect . width;
            
            int t_selection_start_left, t_current_time_left;
            if (t_duration == 0)
            {
                t_selection_start_left = 0;
                t_current_time_left = 0;
            }
            else
            {
                // PM-2014-08-22 [[ Bug 13257 ]] Make sure vars will not overflow
                t_selection_start_left = _muludiv64(t_active_well_width, t_start_time, t_duration);
                t_current_time_left = _muludiv64(t_active_well_width, t_current_time, t_duration);
            }
            
            return MCRectangleMake(t_well_rect . x + t_selection_start_left + t_thumb_rect . width / 2, t_well_rect . y, t_current_time_left - t_selection_start_left, t_well_rect . height);
        }
            break;
            
        case kMCPlayerControllerPartBuffer:
        {
            uint32_t t_loaded_time, t_duration;
            t_duration = getduration();
            t_loaded_time = getmovieloadedtime();
            
            MCRectangle t_well_rect, t_thumb_rect;
            t_well_rect = getcontrollerpartrect(p_rect, kMCPlayerControllerPartWell);
            t_thumb_rect = getcontrollerpartrect(p_rect, kMCPlayerControllerPartThumb);
            
            int t_active_well_width;
            t_active_well_width = t_well_rect . width - t_thumb_rect . width;
            
            int t_loaded_time_left;
            if (t_duration == 0)
            {
                t_loaded_time_left = 0;
            }
            else
            {
                t_loaded_time_left = _muludiv64(t_active_well_width, t_loaded_time, t_duration);
            }
            
            return MCRectangleMake(t_well_rect . x + t_thumb_rect . width / 2, t_well_rect . y, t_loaded_time_left, t_well_rect . height);
        }
            break;
            
        case kMCPlayerControllerPartVolumeWell:
        {
            MCRectangle t_volume_bar_rect = getcontrollerpartrect(getcontrollerrect(), kMCPlayerControllerPartVolumeBar);
            int32_t t_width = CONTROLLER_HEIGHT / 4;
            
            int32_t t_x_offset = (t_volume_bar_rect . width - t_width) / 2;
            
            return MCRectangleMake(t_volume_bar_rect . x + t_x_offset, t_volume_bar_rect . y + t_x_offset, t_width, t_volume_bar_rect . height - 2 * t_x_offset);
        }
            break;
        default:
            break;
    }
    
    return MCRectangleMake(0, 0, 0, 0);
}

void MCPlayer::redrawcontroller(void)
{
    if (!getflag(F_SHOW_CONTROLLER))
        return;
    
    layer_redrawrect(getcontrollerrect());
}

void MCPlayer::handle_mdown(int p_which)
{
    if (!getflag(F_SHOW_CONTROLLER))
        return;
    
    m_inside = true;
    
    int t_part;
    t_part = hittestcontroller(mx, my);
    
    switch(t_part)
    {
        case kMCPlayerControllerPartPlay:
        {
            // MW-2014-07-18: [[ Bug 12825 ]] When play button clicked, previous behavior was to
            //   force rate to 1.0.
            if (!getstate(CS_PREPARED) || ispaused())
                rate = 1.0;
            if (getstate(CS_PREPARED))
            {
                playpause(!ispaused());
            }
            else
            {
                playstart(nil);
            }
            layer_redrawrect(getcontrollerpartrect(getcontrollerrect(), kMCPlayerControllerPartPlay));
        }
        break;
        case kMCPlayerControllerPartVolume:
        {
            if (!m_show_volume)
                m_show_volume = true;
            else
                m_show_volume = false;
            
            if (m_show_volume)
            {
                if (s_volume_popup == nil)
                {
                    s_volume_popup = new MCPlayerVolumePopup;
                    s_volume_popup -> setparent(MCdispatcher);
                    MCdispatcher -> add_transient_stack(s_volume_popup);
                }
                
                s_volume_popup -> openpopup(this);
            }
            
            layer_redrawrect(getcontrollerpartrect(getcontrollerrect(), kMCPlayerControllerPartVolumeBar));
            layer_redrawrect(getcontrollerpartrect(getcontrollerrect(), kMCPlayerControllerPartVolume));
            layer_redrawall();
        }
            break;
            
        case kMCPlayerControllerPartVolumeSelector:
        {
            m_grabbed_part = t_part;
        }
            break;
            
        case kMCPlayerControllerPartVolumeWell:
        case kMCPlayerControllerPartVolumeBar:
        {
            if (!m_show_volume)
                return;
            MCRectangle t_part_volume_selector_rect = getcontrollerpartrect(getcontrollerrect(), kMCPlayerControllerPartVolumeSelector);
            MCRectangle t_volume_well;
            t_volume_well = getcontrollerpartrect(getcontrollerrect(), kMCPlayerControllerPartVolumeWell);
            int32_t t_new_volume, t_height;
            
            t_height = getcontrollerpartrect(getcontrollerrect(), kMCPlayerControllerPartVolumeWell) . height;
            t_new_volume = (t_volume_well . y + t_volume_well . height - my) * 100 / (t_height);
            
            if (t_new_volume < 0)
                t_new_volume = 0;
            if (t_new_volume > 100)
                t_new_volume = 100;
            
            loudness = t_new_volume;
            
            setloudness();
            layer_redrawall();            
        }
            break;
            
            
        case kMCPlayerControllerPartWell:
        {
            // MW-2014-07-22: [[ Bug 12871 ]] If we click in the well without shift and there
            //   is a selection, the selection is removed.
            endtime = starttime;
            setselection(true);
            
            MCRectangle t_part_well_rect = getcontrollerpartrect(getcontrollerrect(), kMCPlayerControllerPartWell);
            
            uint32_t t_new_time, t_duration;
            t_duration = getduration();
            
            // PM-2014-08-22 [[ Bug 13257 ]] Make sure t_new_time will not overflow
            t_new_time = _muludiv64(t_duration, mx - t_part_well_rect . x, t_part_well_rect . width);
            
            // PM-2014-07-09: [[ Bug 12753 ]] If video is playing and we click before the starttime, don't allow video to be played outside the selection
            if (!ispaused() && t_new_time < starttime && getflag(F_PLAY_SELECTION))
                t_new_time = starttime;
            
            setcurtime(t_new_time, true);
            
            layer_redrawall();
            layer_redrawrect(getcontrollerpartrect(getcontrollerrect(), kMCPlayerControllerPartThumb));
        }
            break;
        case kMCPlayerControllerPartScrubBack:
            // PM-2014-08-12: [[ Bug 13120 ]] Cmd + click on scrub buttons starts playing in the appropriate direction
            if (hasfilename() && (MCmodifierstate & MS_CONTROL) != 0)
            {
                double t_rate;
                t_rate = -1.0;
                MCPlatformSetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyPlayRate, kMCPlatformPropertyTypeDouble, &t_rate);
                break;
            }
            
            // PM-2014-08-12: [[ Bug 13120 ]] Option (alt) + click on the scrub buttons takes to beginning / end
            if (hasfilename() && (MCmodifierstate & MS_ALT) != 0)
            {
                uint32_t t_zero_time;
                t_zero_time = 0;
                setcurtime(t_zero_time, true);
                break;
            }
            
            m_scrub_back_is_pressed = true;
            // This is needed for handle_mup
            m_was_paused = ispaused();
            
            if(ispaused())
                playstepback();
            else
            {
                playstepback();
                playpause(!ispaused());
            }
            m_grabbed_part = t_part;
            layer_redrawrect(getcontrollerpartrect(getcontrollerrect(), kMCPlayerControllerPartScrubBack));
            break;
            
        case kMCPlayerControllerPartScrubForward:
            // PM-2014-08-12: [[ Bug 13120 ]] Cmd + click on scrub buttons starts playing in the appropriate direction
            if (hasfilename() && (MCmodifierstate & MS_CONTROL) != 0)
            {
                double t_rate;
                t_rate = 1.0;
                MCPlatformSetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyPlayRate, kMCPlatformPropertyTypeDouble, &t_rate);
                break;
            }
            
            // PM-2014-08-12: [[ Bug 13120 ]] Option (alt) + click on the scrub buttons takes to beginning / end
            if (hasfilename() && (MCmodifierstate & MS_ALT) != 0)
            {
                uint32_t t_duration;
                t_duration = getduration();
                setcurtime(t_duration, true);
                break;
            }

            m_scrub_forward_is_pressed = true;
            // This is needed for handle_mup
            m_was_paused = ispaused();
            
            if(ispaused())
                playstepforward();
            else
            {
                playstepforward();
                playpause(!ispaused());
            }
            m_grabbed_part = t_part;
            layer_redrawrect(getcontrollerpartrect(getcontrollerrect(), kMCPlayerControllerPartScrubForward));
            break;
            
        case kMCPlayerControllerPartSelectionStart:
        case kMCPlayerControllerPartSelectionFinish:
        case kMCPlayerControllerPartThumb:
            m_grabbed_part = t_part;
            break;
            
        default:
            break;
    }
}

void MCPlayer::handle_mfocus(int x, int y)
{
    if (state & CS_MFOCUSED)
    {
        switch(m_grabbed_part)
        {
            case kMCPlayerControllerPartVolumeSelector:
            {
                MCRectangle t_part_volume_selector_rect = getcontrollerpartrect(getcontrollerrect(), kMCPlayerControllerPartVolumeSelector);
                MCRectangle t_volume_well, t_volume_bar;
                t_volume_well = getcontrollerpartrect(getcontrollerrect(), kMCPlayerControllerPartVolumeWell);
                t_volume_bar = getcontrollerpartrect(getcontrollerrect(), kMCPlayerControllerPartVolumeBar);
                
                int32_t t_new_volume, t_height, t_offset;
                t_offset = t_volume_well . y - t_volume_bar . y;
                
                t_height = getcontrollerpartrect(getcontrollerrect(), kMCPlayerControllerPartVolumeWell) . height;
                
                t_new_volume = (getcontrollerrect() . y - t_offset - y ) * 100 / (t_height);
                
                if (t_new_volume < 0)
                    t_new_volume = 0;
                if (t_new_volume > 100)
                    t_new_volume = 100;
                loudness = t_new_volume;
                
                setloudness();
                
                layer_redrawall();
            }
                break;
                
            case kMCPlayerControllerPartThumb:
            {
                MCRectangle t_part_well_rect = getcontrollerpartrect(getcontrollerrect(), kMCPlayerControllerPartWell);
                
                int32_t t_new_time, t_duration;
                t_duration = getduration();
                
                if (x < t_part_well_rect . x)
                    x = t_part_well_rect . x;
                
                // PM-2014-08-22 [[ Bug 13257 ]] Make sure t_new_time will not overflow
                t_new_time = _muludiv64(t_duration, x - t_part_well_rect . x, t_part_well_rect . width);
                
                if (t_new_time < 0)
                    t_new_time = 0;
                setcurtime(t_new_time, true);
                
                layer_redrawall();
                layer_redrawrect(getcontrollerpartrect(getcontrollerrect(), kMCPlayerControllerPartThumb));
            }
                break;
                
                
            case kMCPlayerControllerPartSelectionStart:
            {
                MCRectangle t_part_well_rect = getcontrollerpartrect(getcontrollerrect(), kMCPlayerControllerPartWell);
                uint32_t t_new_start_time, t_duration;
                t_duration = getduration();
                
                // PM-2014-07-08: [[Bug 12759]] Make sure we don't drag the selection start beyond the start point of the player
                if (x <= t_part_well_rect . x)
                    x = t_part_well_rect . x;
                
                // PM-2014-08-22 [[ Bug 13257 ]] Make sure t_new_start_time will not overflow
                t_new_start_time = _muludiv64(t_duration, x - t_part_well_rect . x, t_part_well_rect . width);
                
                if (t_new_start_time >= endtime)
                    t_new_start_time = endtime;
                
                if (t_new_start_time <= 0)
                    starttime = 0;
                else if (t_new_start_time > getduration())
                    starttime = getduration();
                else
                    starttime = t_new_start_time;
                
                setselection(true);
                
            }
                break;
                
            case kMCPlayerControllerPartSelectionFinish:
            {
                MCRectangle t_part_well_rect = getcontrollerpartrect(getcontrollerrect(), kMCPlayerControllerPartWell);
                
                uint32_t t_new_finish_time, t_duration;
                t_duration = getduration();
                
                // PM-2014-08-22 [[ Bug 13257 ]] Make sure t_new_finish_time will not overflow
                t_new_finish_time = _muludiv64(t_duration, x - t_part_well_rect . x, t_part_well_rect . width);
                
                if (t_new_finish_time <= starttime)
                    t_new_finish_time = starttime;
                
                if (t_new_finish_time <= 0)
                    endtime = 0;
                else if (t_new_finish_time > getduration())
                    endtime = getduration();
                else
                    endtime = t_new_finish_time;
                
                setselection(true);
                
            }
                break;
                
            case kMCPlayerControllerPartScrubBack:
            case kMCPlayerControllerPartScrubForward:
                if (MCU_point_in_rect(getcontrollerpartrect(getcontrollerrect(), m_grabbed_part), x, y))
                {
                    m_inside = True;
                }
                else
                {
                    m_inside = False;
                }
                break;
            default:
                break;
        }
    }
    
}

void MCPlayer::handle_mstilldown(int p_which)
{
    switch (m_grabbed_part)
    {
        case kMCPlayerControllerPartScrubForward:
        {
            uint32_t t_current_time, t_duration;
            t_current_time = getmoviecurtime();
            t_duration = getduration();
            
            if (t_current_time > t_duration)
                t_current_time = t_duration;
            
            double t_rate;
            if (m_inside)
            {
                t_rate = 2.0;
            }
            else
                t_rate = 0.0;
            
            if (hasfilename())
                MCPlatformSetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyPlayRate, kMCPlatformPropertyTypeDouble, &t_rate);
        }
            break;
            
        case kMCPlayerControllerPartScrubBack:
        {
            uint32_t t_current_time, t_duration;
            t_current_time = getmoviecurtime();
            t_duration = getduration();
            
            if (t_current_time < 0.0)
                t_current_time = 0.0;
            
            double t_rate;
            if (m_inside)
            {
                t_rate = -2.0;
            }
            else
                t_rate = 0.0;
            
            if (hasfilename())
                MCPlatformSetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyPlayRate, kMCPlatformPropertyTypeDouble, &t_rate);
        }
            break;
            
        default:
            break;
    }
    
}

void MCPlayer::handle_mup(int p_which)
{
    switch (m_grabbed_part)
    {
        case kMCPlayerControllerPartScrubBack:
            m_scrub_back_is_pressed = false;
            playpause(m_was_paused);
            layer_redrawall();
            break;
            
        case kMCPlayerControllerPartScrubForward:
            m_scrub_forward_is_pressed = false;
            playpause(m_was_paused);
            layer_redrawall();
            break;
         
        case kMCPlayerControllerPartSelectionStart:
        case kMCPlayerControllerPartSelectionFinish:
            setselection(true);
            layer_redrawrect(getcontrollerrect());
            break;


        default:
            break;
    }
    
    m_grabbed_part = kMCPlayerControllerPartUnknown;
}

void MCPlayer::popup_closed(void)
{
    if (!m_show_volume)
        return;
    
    m_show_volume = false;
    layer_redrawall();
}

// PM-2014-07-16: [[ Bug 12817 ]] Create selection when click and drag on the well while shift key is pressed
void MCPlayer::handle_shift_mdown(int p_which)
{
    if (!getflag(F_SHOW_CONTROLLER))
        return;
    
    int t_part;
    t_part = hittestcontroller(mx, my);
    
    switch(t_part)
    {
        case kMCPlayerControllerPartThumb:
        case kMCPlayerControllerPartWell:
        {
            // PM-2014-09-30: [[ Bug 13540 ]] shift+clicking on controller well/thumb/play button does something only if showSelection is true
            if (!getflag(F_SHOW_SELECTION))
            {
                handle_mdown(p_which);
                return;
            }
                
            
            MCRectangle t_part_well_rect = getcontrollerpartrect(getcontrollerrect(), kMCPlayerControllerPartWell);
            MCRectangle t_part_thumb_rect = getcontrollerpartrect(getcontrollerrect(), kMCPlayerControllerPartThumb);
            
            uint32_t t_new_time, t_old_time, t_duration, t_old_start, t_old_end;;
            t_old_time = getmoviecurtime();
            t_duration = getduration();
            
            // If there was previously no selection, then take it to be currenttime, currenttime.
            if (starttime == MAXUINT4 || endtime == MAXUINT4 || starttime == endtime)
                starttime = endtime = t_old_time;
            
            t_old_start = getstarttime();
            t_old_end = getendtime();
            
            // PM-2014-08-22 [[ Bug 13257 ]] Make sure t_new_time will not overflow
            t_new_time = _muludiv64(t_duration, mx - t_part_well_rect . x, t_part_well_rect . width);


            // PM-2014-09-10: [[ Bug 13389 ]]
            // If click on the left half of the thumb, adjust starttime
            // If click on the right half of the thumb, adjust endtime
            if (t_part == kMCPlayerControllerPartThumb)
            {
                if (mx < t_part_thumb_rect.x + t_part_thumb_rect . width / 2)
                {
                    if (starttime == endtime)
                        endtime = t_old_time;
                    
                    m_grabbed_part = kMCPlayerControllerPartSelectionStart;

                }
                else
                {
                    if (starttime == endtime)
                        starttime = t_old_time;
                    
                    m_grabbed_part = kMCPlayerControllerPartSelectionFinish;
                }
            }

            else if (t_part == kMCPlayerControllerPartWell)
            {
                // If click before current starttime, adjust that.
                // If click after current endtime, adjust that.
                // If click first half of current selection, adjust start.
                // If click last half of current selection, adjust end.
                if (t_new_time <= (t_old_end + t_old_start) / 2)
                {
                    // PM-2014-08-05: [[ Bug 13065 ]] If there was previously no selection, then
                    // endTime is set as currentTime when mouse is clicked
                    // startTime is set to currentTime and then updated as thumb dragged to the left
                    if (starttime == endtime)
                        endtime = t_old_time;
                    
                    starttime = t_new_time;
                    m_grabbed_part = kMCPlayerControllerPartSelectionStart;
                }
                else
                {
                    // PM-2014-08-05: [[ Bug 13065 ]] If there was previously no selection, then
                    // startTime is set as currentTime when mouse is clicked
                    // endTime is set to currentTime and then updated as thumb dragged to the right
                    if (starttime == endtime)
                        starttime = t_old_time;
                    
                    endtime = t_new_time;
                    m_grabbed_part = kMCPlayerControllerPartSelectionFinish;
                }
            }
            
            if (hasfilename())
                setselection(true);

            layer_redrawrect(getcontrollerrect());
        }
            break;
            
        case kMCPlayerControllerPartPlay:
            // PM-2014-09-30: [[ Bug 13540 ]] shift+clicking on controller well/thumb/play button does something only if showSelection is true
            if (!getflag(F_SHOW_SELECTION))
            {
                handle_mdown(p_which);
                return;
            }
            
            shift_play();
            break;
          
        // PM-2014-09-01: [[ Bug 13119 ]] Shift + click on scrub buttons creates a playrate scrollbar
        case kMCPlayerControllerPartScrubBack:
        case kMCPlayerControllerPartScrubForward:
        {
            
            if (s_rate_popup == nil)
            {
                s_rate_popup = new MCPlayerRatePopup;
                s_rate_popup -> setparent(MCdispatcher);
                MCdispatcher -> add_transient_stack(s_rate_popup);
            }
            
            s_rate_popup -> openpopup(this);
            
            
            layer_redrawrect(getcontrollerpartrect(getcontrollerrect(), kMCPlayerControllerPartRateBar));
            layer_redrawall();
        }
            break;
                     
        default:
            break;
    }

}

Boolean MCPlayer::handle_kdown(const char *string, KeySym key)
{
    if (state & CS_PREPARED)
    {
        switch (key)
        {
            case XK_Return:
                playpause(!ispaused());
                break;
            case XK_space:
                playpause(!ispaused());
                break;
            case XK_Right:
            {
                if(ispaused())
                    playstepforward();
                else
                {
                    playstepforward();
                    playpause(!ispaused());
                }
            }
                break;
            case XK_Left:
            {
                if(ispaused())
                    playstepback();
                else
                {
                    playstepback();
                    playpause(!ispaused());
                }
            }
                break;
            default:
                break;
        }
        // PM-2014-07-14: [[ Bug 12810 ]] Make sure we redraw the controller after using keyboard shortcuts to control playback
        layer_redrawrect(getcontrollerrect());
    }
	if (!(state & CS_NO_MESSAGES))
		if (MCObject::kdown(string, key))
			return True;
    
	return False;
}

// PM-2014-09-05: [[ Bug 13342 ]] Shift and spacebar creates selection
Boolean MCPlayer::handle_shift_kdown(const char *string, KeySym key)
{
    if (state & CS_PREPARED)
    {
        switch (key)
        {
            case XK_space:
                shift_play();
                break;
            
            default:
                break;
        }
        // PM-2014-07-14: [[ Bug 12810 ]] Make sure we redraw the controller after using keyboard shortcuts to control playback
        layer_redrawrect(getcontrollerrect());
    }
    
    return True;
}

void MCPlayer::shift_play()
{
    m_modify_selection_while_playing = true;
    
    // PM-2014-08-05: [[ Bug 13063 ]] Make sure shift + play sets the starttime to the currenttime if there is previously no selection
    uint32_t t_old_time;
    t_old_time = getmoviecurtime();
    
    // If there was previously no selection, then take it to be currenttime, currenttime.
    if (starttime == endtime || starttime == MAXUINT4 || endtime == MAXUINT4)
        starttime = endtime = t_old_time;
    
    
    if (hasfilename())
    {
        // MW-2014-07-18: [[ Bug 12825 ]] When play button clicked, previous behavior was to
        //   force rate to 1.0.
        if (!getstate(CS_PREPARED) || ispaused())
            rate = 1.0;
        if (getstate(CS_PREPARED))
        {
            playpause(!ispaused());
        }
        else
        {
            playstart(nil);
        }
        layer_redrawrect(getcontrollerpartrect(getcontrollerrect(), kMCPlayerControllerPartPlay));
        
        if (ispaused())
            endtime = getmoviecurtime();
        setselection(true);
    }

}
