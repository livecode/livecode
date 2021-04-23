/* Copyright (C) 2016 LiveCode Ltd.
 
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


#include "util.h"
#include "font.h"
#include "sellst.h"
#include "stack.h"
#include "stacklst.h"
#include "card.h"
#include "field.h"
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
#include "objectstream.h"

#include "graphics_util.h"

#include "player-platform.h"

#include "exec-interface.h"

#include "stackfileformat.h"

//////////////////////////////////////////////////////////////////////

//// PLATFORM PLAYER

#include "platform.h"

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
    
            {0x2222, 0x2222, 0x2222},         /* dark gray */
            {0xFFFF, 0xFFFF, 0xFFFF},         /* white */
};

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

inline uint64_t _muludiv64(uint64_t p_multiplier, uint64_t p_numerator, uint64_t p_denominator)
{
    return ((p_multiplier * p_numerator) / p_denominator);
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
        MCGContextSetFillRGBAColor(p_gcontext, (m_player -> getcontrollerfontcolor() . red / 255.0) / 257.0, (m_player -> getcontrollerfontcolor() . green / 255.0) / 257.0, (m_player -> getcontrollerfontcolor() . blue / 255.0) / 257.0, 1.0f);
        MCGContextFill(p_gcontext);
    }
    
    void drawControllerVolumeWellButton(MCGContextRef p_gcontext, const MCRectangle& dirty)
    {
        MCRectangle t_volume_well;
        t_volume_well = getVolumeBarPartRect(dirty, kMCPlayerControllerPartVolumeWell);
        
        MCGBitmapEffects t_effects = MCGBitmapEffects();
        t_effects . has_inner_shadow = true;
		
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
				// click outside control area - close popup
				close();
                break;
        }
        return True;
    }
    
    // PM-2014-10-24 [[ Bug 13751 ]] Make sure the correct mup() is called on the volume selector
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
                MCRectangle t_volume_well;
                t_volume_well = getVolumeBarPartRect(m_volume_rect, kMCPlayerControllerPartVolumeWell);
                
                int32_t t_new_volume;
                
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
    
    virtual Boolean kdown(MCStringRef p_string, KeySym key)
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
        MCRectangle t_rate_well;
        t_rate_well = getRateBarPartRect(dirty, kMCPlayerControllerPartRateWell);
        
        MCGBitmapEffects t_effects = MCGBitmapEffects();
        t_effects . has_inner_shadow = true;

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
    
    virtual Boolean kdown(MCStringRef p_string, KeySym key)
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
	nextplayer = nil;
    rect.width = rect.height = 128;
    filename = MCValueRetain(kMCEmptyString);
    resolved_filename = MCValueRetain(kMCEmptyString);
	istmpfile = False;
	scale = 1.0;
	rate = 1.0;
	lasttime = 0;
	starttime = endtime = MAXUINT4;
    
	disposable = istmpfile = False;
	userCallbackStr = MCValueRetain(kMCEmptyString);
	formattedwidth = formattedheight = 0;
	loudness = 100;
    dontuseqt = MCdontuseQT;
    usingqt = False;
    
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
    
	m_left_balance = 100.0;
	m_right_balance = 100.0;
	m_audio_pan = 0.0;
	
    // MW-2014-07-16: [[ Bug ]] Put the player in the list.
    nextplayer = MCplayers;
    MCplayers = this;
}

MCPlayer::MCPlayer(const MCPlayer &sref) : MCControl(sref)
{
    nextplayer = nil;
    filename = MCValueRetain(sref.filename);
    resolved_filename = MCValueRetain(sref.resolved_filename);
	istmpfile = False;
	scale = 1.0;
	rate = sref.rate;
	lasttime = sref.lasttime;
	starttime = sref.starttime;
	endtime = sref.endtime;
	disposable = istmpfile = False;
	userCallbackStr = MCValueRetain(sref.userCallbackStr);
	formattedwidth = formattedheight = 0;
	loudness = sref.loudness;
    
    dontuseqt = sref.dontuseqt;
    usingqt = False;
    
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
    
	m_left_balance = sref.m_left_balance;
	m_right_balance = sref.m_right_balance;
	m_audio_pan = sref.m_audio_pan;
	
    // MW-2014-07-16: [[ Bug ]] Put the player in the list.
    nextplayer = MCplayers;
    MCplayers = this;
}

MCPlayer::~MCPlayer()
{
    removefromplayers();
    
	if (m_platform_player != nil)
		MCPlatformPlayerRelease(m_platform_player);
    
	MCValueRelease(filename);
    MCValueRelease(resolved_filename);
    MCValueRelease(userCallbackStr);
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
    prepare(kMCEmptyString);
	attachplayer();
}

void MCPlayer::close()
{
	MCControl::close();

    if (s_volume_popup != nil)
        s_volume_popup -> close();

	if (opened == 0)
	{
		state |= CS_CLOSING;
		playstop();
		state &= ~CS_CLOSING;

		detachplayer();

		if (m_platform_player != nil)
		{
			MCPlatformPlayerRelease(m_platform_player);
			m_platform_player = nullptr;
		}
	}
}

Boolean MCPlayer::kdown(MCStringRef p_string, KeySym key)
{
    if ((MCmodifierstate & MS_SHIFT) != 0)
        handle_shift_kdown(p_string, key);
    else
        handle_kdown(p_string, key);
    
    return True;
}

Boolean MCPlayer::kup(MCStringRef p_string, KeySym key)
{
    return False;
}

Boolean MCPlayer::mfocus(int2 x, int2 y)
{
	if (!(flags & F_VISIBLE || showinvisible())
        || (flags & F_DISABLED && getstack()->gettool(this) == T_BROWSE))
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
                // PM-2014-07-16: [[ Bug 12817 ]] Create selection when click and drag on the well while shift key is pressed
                if ((MCmodifierstate & MS_SHIFT) != 0)
                    handle_shift_mdown(which);
                else
                    handle_mdown(which);
                // Send mouseDown msg after mdown is passed to the controller, to prevent blocking if the mouseDown handler has an 'answer' command
                message_with_valueref_args(MCM_mouse_down, MCSTR("1"));
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
            if (message_with_valueref_args(MCM_mouse_down, MCSTR("2")) == ES_NORMAL)
                return True;
            break;
		case Button3:
            message_with_valueref_args(MCM_mouse_down, MCSTR("3"));
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
                    message_with_valueref_args(MCM_mouse_up, MCSTR("1"));
                else
                    message_with_valueref_args(MCM_mouse_release, MCSTR("1"));
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


MCRectangle MCPlayer::GetNativeViewRect(const MCRectangle &p_object_rect)
{
	return getvideorect(p_object_rect);
}

void MCPlayer::timer(MCNameRef mptr, MCParameter *params)
{
    if (MCNameIsEqualToCaseless(mptr, MCM_play_started))
    {
        state &= ~CS_PAUSED;
        redrawcontroller();
    }
    else if (MCNameIsEqualToCaseless(mptr, MCM_play_stopped))
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
    else if (MCNameIsEqualToCaseless(mptr, MCM_play_paused))
    {
        state |= CS_PAUSED;
        redrawcontroller();
        
        m_modify_selection_while_playing = false;
    }
    else if (MCNameIsEqualToCaseless(mptr, MCM_current_time_changed))
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
    else if (MCNameIsEqualToCaseless(mptr, MCM_internal))
    {
        handle_mstilldown(Button1);
        MCscreen -> addtimer(this, MCM_internal, MCblinkrate);
    }
    MCControl::timer(mptr, params);
}

void MCPlayer::toolchanged(Tool p_new_tool)
{
	if (p_new_tool != T_BROWSE && p_new_tool != T_HELP)
		playpause(True);

	MCControl::toolchanged(p_new_tool);
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
	MCPlayer *newplayer = new (nothrow) MCPlayer(*this);
	if (attach)
		newplayer->attach(p, invisible);
	return newplayer;
}

IO_stat MCPlayer::extendedsave(MCObjectOutputStream& p_stream, uint4 p_part, uint32_t p_version)
{
	return defaultextendedsave(p_stream, p_part, p_version);
}

IO_stat MCPlayer::extendedload(MCObjectInputStream& p_stream, uint32_t p_version, uint4 p_remaining)
{
	return defaultextendedload(p_stream, p_version, p_remaining);
}

IO_stat MCPlayer::save(IO_handle stream, uint4 p_part, bool p_force_ext, uint32_t p_version)
{
	IO_stat stat;
	if (!disposable)
	{
		if ((stat = IO_write_uint1(OT_PLAYER, stream)) != IO_NORMAL)
			return stat;
		if ((stat = MCControl::save(stream, p_part, p_force_ext, p_version)) != IO_NORMAL)
			return stat;
        
        // MW-2013-11-19: [[ UnicodeFileFormat ]] If sfv >= 7000, use unicode.
        if ((stat = IO_write_stringref_new(filename, stream, p_version >= kMCStackFileFormatVersion_7_0)) != IO_NORMAL)
			return stat;
		if ((stat = IO_write_uint4(starttime, stream)) != IO_NORMAL)
			return stat;
		if ((stat = IO_write_uint4(endtime, stream)) != IO_NORMAL)
			return stat;
		if ((stat = IO_write_int4((int4)(rate / 10.0 * MAXINT4),
		                          stream)) != IO_NORMAL)
			return stat;
        
        // MW-2013-11-19: [[ UnicodeFileFormat ]] If sfv >= 7000, use unicode.
        if ((stat = IO_write_stringref_new(userCallbackStr, stream, p_version >= kMCStackFileFormatVersion_7_0)) != IO_NORMAL)
			return stat;
	}
	return savepropsets(stream, p_version);
}

IO_stat MCPlayer::load(IO_handle stream, uint32_t version)
{
	IO_stat stat;
    
	if ((stat = MCObject::load(stream, version)) != IO_NORMAL)
		return checkloadstat(stat);
	if ((stat = IO_read_stringref_new(filename, stream, version >= kMCStackFileFormatVersion_7_0)) != IO_NORMAL)
        
        // MW-2013-11-19: [[ UnicodeFileFormat ]] If sfv >= 7000, use unicode.
		return checkloadstat(stat);

	uint32_t t_starttime, t_endtime;
	if ((stat = IO_read_uint4(&t_starttime, stream)) != IO_NORMAL)
		return checkloadstat(stat);
	if ((stat = IO_read_uint4(&t_endtime, stream)) != IO_NORMAL)
		return checkloadstat(stat);
	starttime = t_starttime;
	endtime = t_endtime;

	int4 trate;
	if ((stat = IO_read_int4(&trate, stream)) != IO_NORMAL)
		return checkloadstat(stat);
	rate = (real8)trate * 10.0 / MAXINT4;
	
	// MW-2013-11-19: [[ UnicodeFileFormat ]] If sfv >= 7000, use unicode.
	if ((stat = IO_read_stringref_new(userCallbackStr, stream, version >= kMCStackFileFormatVersion_7_0)) != IO_NORMAL)
		return checkloadstat(stat);
	return loadpropsets(stream, version);
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
bool MCPlayer::getversion(MCStringRef& r_string)
{
    extern void MCQTGetVersion(MCStringRef &r_version);
    MCQTGetVersion(r_string);
    return true;
}

void MCPlayer::freetmp()
{
	if (istmpfile)
	{
		MCS_unlink(filename);
		MCValueAssign(filename, kMCEmptyString);
	}
}

MCPlayerDuration MCPlayer::getmovieloadedtime()
{
    MCPlayerDuration loadedtime;
	if (m_platform_player != nil && hasfilename())
		MCPlatformGetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyLoadedTime, kMCPlatformPropertyTypePlayerDuration, &loadedtime);
	else
		loadedtime = 0;
	return loadedtime;
}

MCPlayerDuration MCPlayer::getduration() //get movie duration/length
{
	MCPlatformPlayerDuration duration;
	if (m_platform_player != nil && hasfilename())
		MCPlatformGetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyDuration, kMCPlatformPropertyTypePlayerDuration, &duration);
	else
		duration = 0;
	return duration;
}

MCPlayerDuration MCPlayer::gettimescale() //get moive time scale
{
	MCPlatformPlayerDuration timescale;
	if (m_platform_player != nil && hasfilename())
		MCPlatformGetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyTimescale, kMCPlatformPropertyTypePlayerDuration, &timescale);
	else
		timescale = 0;
	return timescale;
}

MCPlayerDuration MCPlayer::getmoviecurtime()
{
	MCPlatformPlayerDuration curtime;
	if (m_platform_player != nil && hasfilename())
		MCPlatformGetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyCurrentTime, kMCPlatformPropertyTypePlayerDuration, &curtime);
	else
		curtime = 0;
	return curtime;
}

void MCPlayer::setcurtime(MCPlayerDuration newtime, bool notify)
{
    newtime = MCMin(newtime, getduration());
	lasttime = newtime;
	if (m_platform_player != nil && hasfilename())
    {
		MCPlatformSetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyCurrentTime, kMCPlatformPropertyTypePlayerDuration, &newtime);
        if (notify)
            currenttimechanged();
    }
}

void MCPlayer::setselection(bool notify)
{
    if (m_platform_player != nil && hasfilename())
	{
        MCPlatformPlayerDuration t_current_start, t_current_finish;
        MCPlatformGetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyStartTime, kMCPlatformPropertyTypePlayerDuration, &t_current_start);
		MCPlatformGetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyFinishTime, kMCPlatformPropertyTypePlayerDuration, &t_current_finish);
        
        if (starttime != t_current_start || endtime != t_current_finish)
        {
            MCPlatformPlayerDuration t_st, t_et;
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
            MCPlatformSetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyFinishTime, kMCPlatformPropertyTypePlayerDuration, &t_et);
            MCPlatformSetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyStartTime, kMCPlatformPropertyTypePlayerDuration, &t_st);
            
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

void MCPlayer::setdontuseqt(bool noqt)
{
	dontuseqt = noqt;
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
		if (rate == 0.0f)
		{
			// Setting playrate to 0 should pause the player (if playing)
			MCPlatformStopPlayer(m_platform_player);
		}
		else
		{
			// start / resume at the new rate
			MCPlatformSetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyPlayRate, kMCPlatformPropertyTypeDouble, &rate);
			MCPlatformStartPlayer(m_platform_player, rate);
		}
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

Boolean MCPlayer::prepare(MCStringRef options)
{
	Boolean ok = False;
    
    if (state & CS_PREPARED)
        return True;

   	if (!opened)
		return False;
    
	if (m_platform_player == nil)
    {
        MCPlatformCreatePlayer(dontuseqt, m_platform_player);
    }

	if (m_platform_player == nil)
		return False;
		
#if defined(TARGET_PLATFORM_WINDOWS)
	if (!MCPlatformPlayerSetNativeParentView(m_platform_player, getstack()->getrealwindow()))
		return False;
#endif
	
    // PM-2015-01-26: [[ Bug 14435 ]] Avoid prepending the defaultFolder or the stack folder
    //  to the filename property. Use resolved_filename to set the "internal" absolute path
    MCAutoStringRef t_resolved_filename;
    bool t_path_resolved = false;
    t_path_resolved = resolveplayerfilename(filename, &t_resolved_filename);
    
    if (!t_path_resolved)
        MCValueAssign(resolved_filename, kMCEmptyString);
    else
        MCValueAssign(resolved_filename, *t_resolved_filename);

    if (MCStringBeginsWithCString(resolved_filename, (const char_t*)"https:", kMCStringOptionCompareCaseless)
            // SN-2014-08-14: [[ Bug 13178 ]] Check if the sentence starts with 'http:' instead of 'https'
            || MCStringBeginsWithCString(resolved_filename, (const char_t*)"http:", kMCStringOptionCompareCaseless)
            || MCStringBeginsWithCString(resolved_filename, (const char_t*)"ftp:", kMCStringOptionCompareCaseless)
            || MCStringBeginsWithCString(resolved_filename, (const char_t*)"file:", kMCStringOptionCompareCaseless)
            || MCStringBeginsWithCString(resolved_filename, (const char_t*)"rtsp:", kMCStringOptionCompareCaseless))
        MCPlatformSetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyURL, kMCPlatformPropertyTypeMCString, &resolved_filename);
	else
		MCPlatformSetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyFilename, kMCPlatformPropertyTypeMCString, &resolved_filename);
	
    if (!hasfilename())
        return True;
    
	MCRectangle t_movie_rect;
	MCPlatformGetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyMovieRect, kMCPlatformPropertyTypeRectangle, &t_movie_rect);
    
    // PM-2014-12-17: [[ Bug 14233 ]] If an invalid filename is used then keep the previous dimensions of the player rect instead of displaying only the controller
    // PM-2014-12-17: [[ Bug 14232 ]] Update the result in case a filename is invalid or the file is corrupted
    if (hasinvalidfilename() || !t_path_resolved)
    {
        MCresult->sets("could not create movie reference");
        return False;
    }
	
	resize(t_movie_rect);
	
	bool t_looping, t_play_selection, t_show_selection, t_mirrored;
	
	t_looping = getflag(F_LOOPING);
	t_show_selection = getflag(F_SHOW_SELECTION);
    t_play_selection = getflag(F_PLAY_SELECTION);
    t_mirrored = getflag(F_MIRRORED);
	
	MCPlatformSetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyCurrentTime, kMCPlatformPropertyTypePlayerDuration, &lasttime);
    MCPlatformSetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyLoop, kMCPlatformPropertyTypeBool, &t_looping);
    MCPlatformSetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyShowSelection, kMCPlatformPropertyTypeBool, &t_show_selection);
    MCPlatformSetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyMirrored, kMCPlatformPropertyTypeBool, &t_mirrored);
    
    // PM-2014-08-06: [[ Bug 13104 ]] When new movie is opened then playRate should be set to 0
    rate = 0.0;
    
	setselection(false);
	MCPlatformSetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyOnlyPlaySelection, kMCPlatformPropertyTypeBool, &t_play_selection);
	SynchronizeUserCallbacks();
	
	bool t_offscreen;
	t_offscreen = getflag(F_ALWAYS_BUFFER);
	MCPlatformSetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyOffscreen, kMCPlatformPropertyTypeBool, &t_offscreen);
	
	
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
	
	if (getflag(F_ALWAYS_BUFFER))
		return;
	
	if (getNativeLayer() != nil)
		return;
	
	SetNativeView(MCPlatformPlayerGetNativeView(m_platform_player));
}

// PM-2014-10-14: [[ Bug 13569 ]] Make sure changes to player are not visible in preOpenCard
void MCPlayer::detachplayer()
{
    if (m_platform_player == nil)
        return;
	
	SetNativeView(nil);
}

Boolean MCPlayer::playstart(MCStringRef options)
{
	if (!prepare(options) || !hasfilename())
		return False;
    
    // PM-2014-10-21: [[ Bug 13710 ]] Attach the player if not already attached
	attachplayer();
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
            // PM-2014-08-06: [[ Bug 13104 ]] Remember existing playrate when starting player after a pause
            double t_rate;
            MCPlatformGetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyPlayRate, kMCPlatformPropertyTypeDouble, &t_rate);
            rate = t_rate;
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
	if (m_platform_player != nil)
	{
		MCPlatformStopPlayer(m_platform_player);

		needmessage = getduration() > getmoviecurtime();
		detachplayer();
	}
    
    redrawcontroller();
    
	freetmp();
    
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


void MCPlayer::setfilename(MCStringRef vcname,
                           MCStringRef fname, Boolean istmp)
{
	// AL-2014-05-27: [[ Bug 12517 ]] Incoming strings can be nil
    MCNewAutoNameRef t_vcname;
    if (vcname != nil)
        MCNameCreate(vcname, &t_vcname);
    else
        t_vcname = kMCEmptyName;
	filename = MCValueRetain(fname != nil ? fname : kMCEmptyString);
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
		MCPlatformGetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyMovieRect, kMCPlatformPropertyTypeRectangle, &t_bounds);
	
	// IM-2016-04-22: [[ WindowsPlayer ]] Return player rect required to display video at preferred size
	return getplayerrectforvideorect(t_bounds);
}

uint2 MCPlayer::getloudness()
{
	if (getstate(CS_PREPARED))
		if (m_platform_player != nil)
			MCPlatformGetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyVolume, kMCPlatformPropertyTypeUInt16, &loudness);
	return loudness;
}

MCColor MCPlayer::getcontrollerfontcolor()
{
    // Default controller font color (darkgray)
    return controllercolors[0];
}

// PM-2014-09-16: [[ Bug 12834 ]] Allow setting the color of controller icons (backcolor property of player)
MCColor MCPlayer::getcontrollericoncolor()
{
    uint2 i;
    if (getcindex(DI_BACK, i))
        return colors[i];
    
    // Default controller icons color (white)
    return controllercolors[1];
}

// PM-2014-09-16: [[ Bug 13390 ]] use the MCObject colors list since we are using the standard color prop names, so no extra stuff needs to be saved
MCColor MCPlayer::getcontrollermaincolor()
{
    uint2 i;
    if (getcindex(DI_HILITE, i))
        return colors[i];
    
    // Default controller played area color (platform default - light blue)
    return MChilitecolor;
}

// PM-2014-09-16: [[ Bug 13391 ]] Changed default forecolor
MCColor MCPlayer::getcontrollerselectedareacolor()
{
    uint2 i;
    if (getcindex(DI_FORE, i))
        return colors[i];
    
    // Default controller selected area color (platform default - light gray)
    return MCselectioncolor;
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

double MCPlayer::getleftbalance()
{
	if (state & CS_PREPARED)
		if (m_platform_player != nil)
			MCPlatformGetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyLeftBalance, kMCPlatformPropertyTypeDouble, &m_left_balance);
	return m_left_balance;
}

void MCPlayer::setleftbalance(double p_left_balance)
{
	m_left_balance = p_left_balance;
	if (state & CS_PREPARED)
		if (m_platform_player != nil)
			MCPlatformSetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyLeftBalance, kMCPlatformPropertyTypeDouble, &m_left_balance);
}

double MCPlayer::getrightbalance()
{
	if (state & CS_PREPARED)
		if (m_platform_player != nil)
			MCPlatformGetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyRightBalance, kMCPlatformPropertyTypeDouble, &m_right_balance);
	return m_right_balance;
}

void MCPlayer::setrightbalance(double p_right_balance)
{
	m_right_balance = p_right_balance;
	if (state & CS_PREPARED)
		if (m_platform_player != nil)
			MCPlatformSetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyRightBalance, kMCPlatformPropertyTypeDouble, &m_right_balance);
}

double MCPlayer::getaudiopan()
{
	if (state & CS_PREPARED)
		if (m_platform_player != nil)
			MCPlatformGetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyPan, kMCPlatformPropertyTypeDouble, &m_audio_pan);
	return m_audio_pan;
}

void MCPlayer::setaudiopan(double p_pan)
{
	m_audio_pan = p_pan;
	if (state & CS_PREPARED)
		if (m_platform_player != nil)
			MCPlatformSetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyPan, kMCPlatformPropertyTypeDouble, &m_audio_pan);
}

void MCPlayer::setenabledtracks(uindex_t p_count, uint32_t *p_tracks_id)
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
			
            for (uindex_t i = 0; i < t_track_count; i++)
            {
				// If the list of enabledtracks we set contains 0 (empty), just skip it
				if (p_tracks_id[i] == 0)
					continue;
					
                uindex_t t_index;
                if (!MCPlatformFindPlayerTrackWithId(m_platform_player, p_tracks_id[i], t_index))
                    return;
                
                bool t_enabled;
                t_enabled = true;
                MCPlatformSetPlayerTrackProperty(m_platform_player, t_index, kMCPlatformPlayerTrackPropertyEnabled, kMCPlatformPropertyTypeBool, &t_enabled);
            }
            
			MCRectangle t_movie_rect;
			MCPlatformGetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyMovieRect, kMCPlatformPropertyTypeRectangle, &t_movie_rect);
			MCRectangle trect = resize(t_movie_rect);
			if (flags & F_SHOW_BORDER)
				trect = MCU_reduce_rect(trect, -borderwidth);
			setrect(trect);
		}
}

MCRectangle MCPlayer::resize(MCRectangle movieRect)
{
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
			// IM-2016-04-22: [[ WindowsPlayer ]] Use convenience method to get required player rect,
			//   centered on the current rect.
			trect = MCU_center_rect(trect, getplayerrectforvideorect(movieRect));
			rect = trect;
		}
	}
	else
		if (flags & F_SHOW_BORDER)
			trect = MCU_reduce_rect(trect, borderwidth);
	
	// MW-2011-10-24: [[ Bug 9800 ]] If the rect has changed, notify the layer.
	if (!MCU_equal_rect(rect, t_old_rect))
	{
		layer_rectchanged(t_old_rect, true);
		geometrychanged(rect);
	}
	
	return trect;
}


void MCPlayer::setcallbacks(MCStringRef p_callbacks)
{
    MCValueAssign(userCallbackStr, p_callbacks);
    SynchronizeUserCallbacks();
}

void MCPlayer::setmoviecontrollerid(integer_t p_id)
{    
}

integer_t MCPlayer::getmoviecontrollerid()
{
    // COCOA-TODO
    return (integer_t)NULL;
}

integer_t MCPlayer::getmediatypes()
{
    if (m_platform_player != nil)
    {
        MCPlatformPlayerMediaTypes t_types;
        MCPlatformGetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyMediaTypes, kMCPlatformPropertyTypePlayerMediaTypes, &t_types);

        return t_types;
    }
    
    return 0;
}

uinteger_t MCPlayer::getcurrentnode()
{
    uint2 i = 0;
    if (m_platform_player != nil)
        MCPlatformGetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyQTVRNode, kMCPlatformPropertyTypeUInt16, &i);
    return i;
}

bool MCPlayer::changecurrentnode(uinteger_t p_node_id)
{
    if (m_platform_player != nil)
    {
        MCPlatformSetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyQTVRNode, kMCPlatformPropertyTypeUInt16, &p_node_id);
        return true;
    }
    return false;
}

real8 MCPlayer::getpan()
{
    real8 pan = 0.0;
    if (m_platform_player != nil)
        MCPlatformGetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyQTVRPan, kMCPlatformPropertyTypeDouble, &pan);
    return pan;
}

bool MCPlayer::changepan(real8 pan)
{
    if (m_platform_player != nil)
        MCPlatformSetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyQTVRPan, kMCPlatformPropertyTypeDouble, &pan);
    
    return isbuffering();
}

real8 MCPlayer::gettilt()
{
    real8 tilt = 0.0;
    if (m_platform_player != nil)
        MCPlatformGetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyQTVRTilt, kMCPlatformPropertyTypeDouble, &tilt);
    return tilt;
}

bool MCPlayer::changetilt(real8 tilt)
{
    if (m_platform_player != nil)
        MCPlatformSetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyQTVRTilt, kMCPlatformPropertyTypeDouble, &tilt);
    return isbuffering();
}

real8 MCPlayer::getzoom()
{
    real8 zoom = 0.0;
    if (m_platform_player != nil)
        MCPlatformGetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyQTVRZoom, kMCPlatformPropertyTypeDouble, &zoom);
    return zoom;
}

bool MCPlayer::changezoom(real8 zoom)
{
    if (m_platform_player != nil)
        MCPlatformSetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyQTVRZoom, kMCPlatformPropertyTypeDouble, &zoom);
    return isbuffering();
}

void MCPlayer::gettracks(MCStringRef &r_tracks)
{
    if (getstate(CS_PREPARED) && m_platform_player != nil)
	{
		uindex_t t_track_count;
		MCPlatformCountPlayerTracks(m_platform_player, t_track_count);
        MCAutoListRef t_tracks_list;
        /* UNCHECKED */ MCListCreateMutable('\n', &t_tracks_list);
        
        for(uindex_t i = 0; i < t_track_count; i++)
        {
            MCAutoStringRef t_track;
            MCAutoStringRef t_name;
            
            uint32_t t_id;
            uint32_t t_offset, t_duration;
            MCPlatformGetPlayerTrackProperty(m_platform_player, i, kMCPlatformPlayerTrackPropertyId, kMCPlatformPropertyTypeUInt32, &t_id);
            MCPlatformGetPlayerTrackProperty(m_platform_player, i, kMCPlatformPlayerTrackPropertyMediaTypeName, kMCPlatformPropertyTypeNativeCString, &(&t_name));
            MCPlatformGetPlayerTrackProperty(m_platform_player, i, kMCPlatformPlayerTrackPropertyOffset, kMCPlatformPropertyTypeUInt32, &t_offset);
            MCPlatformGetPlayerTrackProperty(m_platform_player, i, kMCPlatformPlayerTrackPropertyDuration, kMCPlatformPropertyTypeUInt32, &t_duration);
            /* UNCHECKED */ MCStringFormat(&t_track, "%u,%@,%u,%u", t_id, *t_name, t_offset, t_duration);
            /* UNCHECKED */ MCListAppend(*t_tracks_list, *t_track);
        }
        /* UNCHECKED */ MCListCopyAsString(*t_tracks_list, r_tracks);
    }
    // PM-2015-04-22: [[ Bug 15264 ]] In case of invalid/non-existent file, return empty (as in LC 6.7.x)
    else
        r_tracks = MCValueRetain(kMCEmptyString);
}

uinteger_t MCPlayer::gettrackcount()
{
    uint2 i = 0;
    if (m_platform_player != nil)
    {
        uindex_t t_count;
        MCPlatformCountPlayerTracks(m_platform_player, t_count);
        i = t_count;
    }
    return i;
}

void MCPlayer::getnodes(MCStringRef &r_nodes)
{
	// COCOA-TODO: MCPlayer::getnodes();
    r_nodes = MCValueRetain(kMCEmptyString);
}

void MCPlayer::gethotspots(MCStringRef &r_nodes)
{
	// COCOA-TODO: MCPlayer::gethotspots();
    r_nodes = MCValueRetain(kMCEmptyString);
}

void MCPlayer::getconstraints(MCMultimediaQTVRConstraints &r_constraints)
{
    if (m_platform_player != nil)
        MCPlatformGetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyQTVRConstraints, kMCPlatformPropertyTypePlayerQTVRConstraints, (MCPlatformPlayerQTVRConstraints*)&(r_constraints));
}

void MCPlayer::getenabledtracks(uindex_t &r_count, uint32_t *&r_tracks_id)
{
    uinteger_t *t_track_ids;
    uindex_t t_count;
    
    t_track_ids = nil;
    t_count = 0;
    
    if (m_platform_player != nil)
    {
        uindex_t t_track_count;
        MCPlatformCountPlayerTracks(m_platform_player, t_track_count);
        t_count = 0;
        
        for(uindex_t i = 0; i < t_track_count; i++)
        {
            uint32_t t_id;
            bool t_enabled;
            MCPlatformGetPlayerTrackProperty(m_platform_player, i, kMCPlatformPlayerTrackPropertyId, kMCPlatformPropertyTypeUInt32, &t_id);
            MCPlatformGetPlayerTrackProperty(m_platform_player, i, kMCPlatformPlayerTrackPropertyEnabled, kMCPlatformPropertyTypeBool, &t_enabled);
            if (t_enabled)
            {
                MCMemoryReallocate(t_track_ids, ++t_count * sizeof(uinteger_t), t_track_ids);
                t_track_ids[t_count - 1] = t_id;
            }
        }
    }
    
    r_count = t_count;
    r_tracks_id = t_track_ids;
}

void MCPlayer::updatevisibility()
{
}

void MCPlayer::updatetraversal()
{
    // Does nothing on platform implementation
}

//
// End of virtual MCPlayerInterface's functions
////////////////////////////////////////////////////////////////////////////////

void MCPlayer::markerchanged(MCPlatformPlayerDuration p_time)
{
    // Search for the first marker with the given time, and dispatch the message.
    for(uindex_t i = 0; i < m_callback_count; i++)
        if (p_time == m_callbacks[i] . time)
        {
            MCExecContext ctxt(nil, nil, nil);
            
            MCParameter *t_param;
            t_param = new (nothrow) MCParameter;
            t_param -> set_argument(ctxt, m_callbacks[i] . parameter);
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
        
        MCPlayerDuration t_current_time;
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
        t_param = new (nothrow) MCParameter;
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
    if (m_platform_player == nil)
        return;
    
    // Free the existing callback table.
    for(uindex_t i = 0; i < m_callback_count; i++)
    {
        MCValueRelease(m_callbacks[i] . message);
        MCValueRelease(m_callbacks[i] . parameter);
    }
    MCMemoryDeleteArray(m_callbacks);
    m_callbacks = nil;
    m_callback_count = 0;
    
    // Now reparse the callback string and build the table.
    MCAutoStringRef t_callback;
    t_callback = userCallbackStr;
    
    uindex_t t_start_index, t_length;
    
	if (MCStringIsEmpty(*t_callback))
		t_length = 0;
	else
		t_length = MCStringGetLength(*t_callback);
	
    t_start_index = 0;
    
	while (t_start_index < t_length)
	{
		uindex_t t_comma_index, t_callback_index, t_end_index;
		if (!MCStringFirstIndexOfChar(*t_callback, ',', t_start_index, kMCStringOptionCompareExact, t_comma_index))
		{
            //search ',' as separator
			return;
		}
		
        // AL-2014-07-31: [[ Bug 12936 ]] Callbacks are one per line
        if (!MCStringFirstIndexOfChar(*t_callback, '\n', t_comma_index + 1, kMCStringOptionCompareExact, t_end_index))
            t_end_index = MCStringGetLength(*t_callback);
        
        /* UNCHECKED */ MCMemoryResizeArray(m_callback_count + 1, m_callbacks, m_callback_count);
        // Converts the first part to a number.
        MCAutoNumberRef t_time;
        
        // SN-2014-07-28: [[ Bug 12984 ]] MCNumberParseOffset expects the string to finish after the number
        MCAutoStringRef t_callback_substring;
        /* UNCHECKED */ MCStringCopySubstring(*t_callback, MCRangeMakeMinMax(t_start_index, t_comma_index), &t_callback_substring);
        
        // SN-2014-07-28: [[ Bug 12984 ]] Mimic the strtol behaviour in case of a parsing failure
        if (MCNumberParse(*t_callback_substring, &t_time))
            m_callbacks[m_callback_count - 1] . time = MCNumberFetchAsReal(*t_time);
        else
            m_callbacks[m_callback_count - 1] . time = 0;
        
        t_callback_index = t_comma_index + 1;
        while (isspace(MCStringGetCharAtIndex(*t_callback, t_callback_index))) //strip off preceding and trailing blanks
            ++t_callback_index;
        
        // See whether we can find a parameter for this callback
        uindex_t t_space_index;
        t_space_index = t_callback_index;
        
        while (t_space_index < t_end_index)
        {
            if (isspace(MCStringGetCharAtIndex(*t_callback, t_space_index)))
            {
                MCAutoStringRef t_param;
                /* UNCHECKED */ MCStringCopySubstring(*t_callback, MCRangeMakeMinMax(t_space_index, t_end_index), &t_param);
                /* UNCHECKED */ MCNameCreate(*t_param, m_callbacks[m_callback_count - 1] . parameter);
                break;
            }
            ++t_space_index;
        }
        
        MCAutoStringRef t_message;
        /* UNCHECKED */ MCStringCopySubstring(*t_callback, MCRangeMakeMinMax(t_callback_index, t_space_index), &t_message);
        /* UNCHECKED */ MCNameCreate(*t_message, m_callbacks[m_callback_count - 1] . message);
        
        // If no parameter is specified, use the time.
        if (m_callbacks[m_callback_count - 1] . parameter == nil)
        {
            MCAutoStringRef t_param;
            /* UNCHECKED */ MCStringCopySubstring(*t_callback, MCRangeMakeMinMax(t_start_index, t_comma_index), &t_param);
            /* UNCHECKED */ MCNameCreate(*t_param, m_callbacks[m_callback_count - 1] . parameter);
        }
		
        // Skip to the next callback, if there is one
        t_start_index = t_end_index + 1;
	}
    
    if (!hasfilename())
        return;
    
    // Now set the markers in the player so that we get notified.
    MCPlatformPlayerDurationArray t_markers;
    /* UNCHECKED */ MCMemoryNewArray(m_callback_count, t_markers . ptr);
    for(uindex_t i = 0; i < m_callback_count; i++)
        t_markers . ptr[i] = m_callbacks[i] . time;
    t_markers . count = m_callback_count;
    MCPlatformSetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyMarkers, kMCPlatformPropertyTypePlayerDurationArray, &t_markers);
    MCMemoryDeleteArray(t_markers . ptr);
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
			// IM-2016-04-22: [[ WindowsPlayer ]] Get rect in which to display video content.
			MCRectangle trect;
			trect = getvideorect(rect);
			
			MCImageDescriptor t_image;
			MCMemoryClear(&t_image, sizeof(t_image));
			t_image.filter = kMCGImageFilterNone;
            
			MCRectangle t_transformed_rect;
			t_transformed_rect = MCRectangleGetTransformedBounds(trect, dc->getdevicetransform());
			
			// IM-2014-05-14: [[ ImageRepUpdate ]] Wrap locked bitmap in MCGImage
			MCImageBitmap *t_bitmap = nil;
			
			if (MCPlatformLockPlayerBitmap(m_platform_player, MCGIntegerSizeMake(t_transformed_rect.width, t_transformed_rect.height), t_bitmap))
			{
				MCGRaster t_raster = MCImageBitmapGetMCGRaster(t_bitmap, true);
				
				// SN-2014-08-25: [[ Bug 13187 ]] We need to copy the raster
				if (dc -> gettype() == CONTEXT_TYPE_PRINTER)
					MCGImageCreateWithRaster(t_raster, t_image.image);
				else
					MCGImageCreateWithRasterNoCopy(t_raster, t_image.image);
				
				if (t_image . image != nil)
				{
					// IM-2017-05-11: [[ Bug 18939 ]] Set x/y scales for image, which may be scaled.
					t_image.x_scale = MCGFloat(MCGImageGetWidth(t_image.image)) / MCGFloat(trect.width);
					t_image.y_scale = MCGFloat(MCGImageGetHeight(t_image.image)) / MCGFloat(trect.height);
					dc -> drawimage(t_image, 0, 0, trect.width, trect.height, trect.x, trect.y);
				}
				
				MCGImageRelease(t_image.image);
				MCPlatformUnlockPlayerBitmap(m_platform_player, t_bitmap);
			}
			else
			{
				dc->setbackground(MCzerocolor);
				dc->fillrect(trect);
			}
		}
	}
 
    // Draw our controller
    if (getflag(F_SHOW_CONTROLLER))
    {
        drawcontroller(dc);
    }
    
	if (getflag(F_SHOW_BORDER))
    {
		if (getflag(F_3D))
			draw3d(dc, rect, ETCH_SUNKEN, borderwidth);
		else
			drawborder(dc, rect, borderwidth);
    }
	
	if (!p_isolated)
    {
		dc -> end();
    }
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
    MCGContextSetFillRGBAColor(t_gcontext, (getcontrollerfontcolor() . red / 255.0) / 257.0, (getcontrollerfontcolor() . green / 255.0) / 257.0, (getcontrollerfontcolor() . blue / 255.0) / 257.0, 1.0f);
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
        MCGContextSetFillRGBAColor(p_gcontext, (getcontrollermaincolor() . red / 255.0) / 257.0, (getcontrollermaincolor() . green / 255.0) / 257.0, (getcontrollermaincolor() . blue / 255.0) / 257.0, 1.0f);
        MCGContextFill(p_gcontext);
    }
    
    MCGContextSetFillRGBAColor(p_gcontext, (getcontrollericoncolor() . red / 255) / 257.0, (getcontrollericoncolor() . green / 255) / 257.0, (getcontrollericoncolor() . blue / 255) / 257.0, 1.0f);

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
    
    MCGContextSetStrokeRGBAColor(p_gcontext, (getcontrollericoncolor() . red / 255) / 257.0, (getcontrollericoncolor() . green / 255) / 257.0, (getcontrollericoncolor() . blue / 255) / 257.0, 1.0f);

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
    
    MCGContextSetFillRGBAColor(p_gcontext, (getcontrollericoncolor() . red / 255) / 257.0, (getcontrollericoncolor() . green / 255) / 257.0, (getcontrollericoncolor() . blue / 255) / 257.0, 1.0f);
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
    
    MCGBitmapEffects t_effects = MCGBitmapEffects();
    t_effects . has_inner_shadow = true;

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

    MCGContextSetFillRGBAColor(p_gcontext, (getcontrollericoncolor() . red / 255) / 257.0, (getcontrollericoncolor() . green / 255) / 257.0, (getcontrollericoncolor() . blue / 255) / 257.0, 1.0f);
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
    
    MCGContextSetFillRGBAColor(p_gcontext, (getcontrollericoncolor() . red / 255) / 257.0, (getcontrollericoncolor() . green / 255) / 257.0, (getcontrollericoncolor() . blue / 255) / 257.0, 1.0f);
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
        MCGContextSetFillRGBAColor(p_gcontext, (getcontrollermaincolor() . red / 255.0) / 257.0, (getcontrollermaincolor() . green / 255.0) / 257.0, (getcontrollermaincolor() . blue / 255.0) / 257.0, 1.0f);
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
    
    MCGContextSetFillRGBAColor(p_gcontext, (getcontrollericoncolor() . red / 255) / 257.0, (getcontrollericoncolor() . green / 255) / 257.0, (getcontrollericoncolor() . blue / 255) / 257.0, 1.0f);
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
        MCGContextSetFillRGBAColor(p_gcontext, (getcontrollermaincolor() . red / 255.0) / 257.0, (getcontrollermaincolor() . green / 255.0) / 257.0, (getcontrollermaincolor() . blue / 255.0) / 257.0, 1.0f);
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
    
    MCGContextSetFillRGBAColor(p_gcontext, (getcontrollericoncolor() . red / 255) / 257.0, (getcontrollericoncolor() . green / 255) / 257.0, (getcontrollericoncolor() . blue / 255) / 257.0, 1.0f);
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
    MCGContextSetFillRGBAColor(p_gcontext, (getcontrollerselectedareacolor() . red / 255.0) / 257.0, (getcontrollerselectedareacolor() . green / 255.0) / 257.0, (getcontrollerselectedareacolor() . blue / 255.0) / 257.0, 1.0f);
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

    MCGContextSetFillRGBAColor(p_gcontext, (getcontrollermaincolor() . red / 255.0) / 257.0, (getcontrollermaincolor() . green / 255.0) / 257.0, (getcontrollermaincolor() . blue / 255.0) / 257.0, 1.0f);
    
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

MCRectangle MCPlayer::getvideorect(const MCRectangle &p_player_rect)
{
	MCRectangle t_rect;
	t_rect = p_player_rect;
	
	if (getflag(F_SHOW_CONTROLLER))
		t_rect.height -= CONTROLLER_HEIGHT;
	
	if (getflag(F_SHOW_BORDER))
		t_rect = MCU_reduce_rect(t_rect, borderwidth);
	
	return t_rect;
}

MCRectangle MCPlayer::getplayerrectforvideorect(const MCRectangle &p_video_rect)
{
	MCRectangle t_rect;
	t_rect = p_video_rect;
	
	if (getflag(F_SHOW_CONTROLLER))
		t_rect.height += CONTROLLER_HEIGHT;
	
	if (getflag(F_SHOW_BORDER))
		t_rect = MCU_reduce_rect(t_rect, -borderwidth);
	
	return t_rect;
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
            
            MCPlayerDuration t_current_time, t_duration;
            t_current_time = getmoviecurtime();
            t_duration = getduration();
			t_current_time = MCMin(t_current_time, t_duration);
			
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
            MCPlayerDuration t_start_time, t_duration;
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
            MCPlayerDuration t_finish_time, t_duration;
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
            
            // The width and height of the volumeselector are CONTROLLER_HEIGHT / 2
            int32_t t_actual_height = t_volume_well_rect . height - CONTROLLER_HEIGHT / 2;
            
            return MCRectangleMake(p_rect . x + CONTROLLER_HEIGHT / 4 , t_volume_well_rect . y + t_volume_well_rect . height - t_actual_height * loudness / 100 - CONTROLLER_HEIGHT / 2, CONTROLLER_HEIGHT / 2, CONTROLLER_HEIGHT / 2 );
        }
            break;
        case kMCPlayerControllerPartSelectedArea:
        {
            MCPlayerDuration t_start_time, t_finish_time, t_duration;
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
            int32_t t_width = t_volume_well_rect . width;
            // Adjust y by 2 pixels
            return MCRectangleMake(t_volume_well_rect. x , t_volume_selector_rect . y + 2 , t_width, t_volume_well_rect . y + t_volume_well_rect . height - t_volume_selector_rect . y );
        }
            break;
            
        case kMCPlayerControllerPartPlayedArea:
        {
            MCPlayerDuration t_start_time, t_current_time, t_finish_time, t_duration;
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
            MCPlayerDuration t_loaded_time, t_duration;
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

            break;
        }
            
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
                    s_volume_popup = new (nothrow) MCPlayerVolumePopup;
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
            
            MCPlayerDuration t_new_time, t_duration;
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
            
            push_current_rate();

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
            
            push_current_rate();

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
                MCPlayerDuration t_duration;
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

void MCPlayer::push_current_rate()
{
    // get current rate
    double t_old_rate;
    MCPlatformGetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyPlayRate, kMCPlatformPropertyTypeDouble, &t_old_rate);
    m_rate_before_scrub_buttons_pressed = t_old_rate;
}

void MCPlayer::pop_current_rate()
{
    // get current rate
    double t_old_rate = m_rate_before_scrub_buttons_pressed;
    MCPlatformSetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyPlayRate, kMCPlatformPropertyTypeDouble, &t_old_rate);
}

void MCPlayer::handle_mfocus(int x, int y)
{
    if (state & CS_MFOCUSED)
    {
        switch(m_grabbed_part)
        {
            case kMCPlayerControllerPartVolumeSelector:
            {
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
                
                MCPlayerDuration t_new_time, t_duration;
                t_duration = getduration();
                
				x = MCClamp(x, t_part_well_rect.x, t_part_well_rect.x + t_part_well_rect.width);
                
                // PM-2014-08-22 [[ Bug 13257 ]] Make sure t_new_time will not overflow
                t_new_time = _muludiv64(t_duration, x - t_part_well_rect . x, t_part_well_rect . width);
                
                setcurtime(t_new_time, true);
                
                layer_redrawall();
                layer_redrawrect(getcontrollerpartrect(getcontrollerrect(), kMCPlayerControllerPartThumb));
            }
                break;
                
                
            case kMCPlayerControllerPartSelectionStart:
            {
                MCRectangle t_part_well_rect = getcontrollerpartrect(getcontrollerrect(), kMCPlayerControllerPartWell);
                MCPlayerDuration t_new_start_time, t_duration;
                t_duration = getduration();
                
				x = MCClamp(x, t_part_well_rect.x, t_part_well_rect.x + t_part_well_rect.width);
                
                // PM-2014-08-22 [[ Bug 13257 ]] Make sure t_new_start_time will not overflow
                t_new_start_time = _muludiv64(t_duration, x - t_part_well_rect . x, t_part_well_rect . width);
                
				starttime = MCMin(MCMin(t_new_start_time, endtime), t_duration);
                
                setselection(true);
                
            }
                break;
                
            case kMCPlayerControllerPartSelectionFinish:
            {
                MCRectangle t_part_well_rect = getcontrollerpartrect(getcontrollerrect(), kMCPlayerControllerPartWell);
                
                MCPlayerDuration t_new_finish_time, t_duration;
                t_duration = getduration();
                
				x = MCClamp(x, t_part_well_rect.x, t_part_well_rect.x + t_part_well_rect.width);

				// PM-2014-08-22 [[ Bug 13257 ]] Make sure t_new_finish_time will not overflow
                t_new_finish_time = _muludiv64(t_duration, x - t_part_well_rect . x, t_part_well_rect . width);
                
                if (t_new_finish_time <= starttime)
                    t_new_finish_time = starttime;
                
				endtime = MCMin(t_new_finish_time, t_duration);
                
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
            pop_current_rate();
            m_scrub_back_is_pressed = false;
            playpause(m_was_paused);
            layer_redrawall();
            break;
            
        case kMCPlayerControllerPartScrubForward:
            pop_current_rate();
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
            
            MCPlayerDuration t_new_time, t_old_time, t_duration, t_old_start, t_old_end;;
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
                s_rate_popup = new (nothrow) MCPlayerRatePopup;
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

Boolean MCPlayer::handle_kdown(MCStringRef p_string, KeySym key)
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
		if (MCObject::kdown(p_string, key))
			return True;
    
	return False;
}

// PM-2014-09-05: [[ Bug 13342 ]] Shift and spacebar creates selection
Boolean MCPlayer::handle_shift_kdown(MCStringRef p_string, KeySym key)
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
    MCPlayerDuration t_old_time;
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

void MCPlayer::setmirrored(bool p_mirrored)
{
    if (m_platform_player != nil && hasfilename())
    {
        bool t_mirrored;
        t_mirrored = p_mirrored;
        MCPlatformSetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyMirrored, kMCPlatformPropertyTypeBool, &t_mirrored);
    }
}

