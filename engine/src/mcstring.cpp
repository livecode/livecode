/* Copyright (C) 2003-2015 LiveCode Ltd.

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
#include "objdefs.h"
#include "mcstring.h"
#include "revbuild.h"

const uint1 MCisotranslations[256] =
    {
		  0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13, 173, 176,
		178, 179, 182, 183, 184, 185, 186, 189, 195, 197,  26, 198, 215, 218, 222, 223,
		 32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,
		 48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,
		 64,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,
		 80,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90,  91,  92,  93,  94,  95,
		 96,  97,  98,  99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
		112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127,
		240, 245, 226, 196, 227, 201, 160, 224, 246, 228,   1, 220, 206, 249, 250, 251,
		253, 212, 213, 210, 211, 165, 208, 209, 247, 170,   2, 221, 207, 254, 255, 217,
		202, 193, 162, 163, 219, 180,   3, 164, 172, 169, 187, 199, 194,   4, 168, 248,
		161, 177,   5,   6, 171, 181, 166, 225, 252,   7, 188, 200,  11,  12,  14, 192,
		203, 231, 229, 204, 128, 129, 174, 130, 233, 131, 230, 232, 237, 234, 235, 236,
         15, 132, 241, 238, 239, 205, 133,  16, 175, 244, 242, 243, 134,  17,  18, 167,
		136, 135, 137, 139, 138, 140, 190, 141, 143, 142, 144, 145, 147, 146, 148, 149,
		 19, 150, 152, 151, 153, 155, 154, 214, 191, 157, 156, 158, 159,  20,  21, 216
    };

const uint1 MCmactranslations[256] =
    {
          0, 138, 154, 166, 173, 178, 179, 185,   8,   9,  10, 188, 189,  13, 190, 208,
		215, 221, 222, 240, 253, 254,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,
		 32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,
		 48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,
		 64,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,
		 80,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90,  91,  92,  93,  94,  95,
		 96,  97,  98,  99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
		112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127,
		196, 197, 199, 201, 209, 214, 220, 225, 224, 226, 228, 227, 229, 231, 233, 232,
		234, 235, 237, 236, 238, 239, 241, 243, 242, 244, 246, 245, 250, 249, 251, 252,
		134, 176, 162, 163, 167, 149, 182, 223, 174, 169, 153, 180, 168,  14, 198, 216,
		 15, 177,  16,  17, 165, 181,  18,  19,  20,  21,  22, 170, 186,  23, 230, 248,
		191, 161, 172,  24, 131,  25,  27, 171, 187, 133, 160, 192, 195, 213, 140, 156,
		150, 151, 147, 148, 145, 146, 247,  28, 255, 159,  29, 164, 139, 155,  30,  31,
		135, 183, 130, 132, 137, 194, 202, 193, 203, 200, 205, 206, 207, 204, 211, 212,
		128, 210, 218, 219, 217, 129, 136, 152, 175, 141, 142, 143, 184, 144, 157, 158
    };


const char *MCtoolnames[] =
    {
        "undefined", "browse", "brush", "bucket",
        "button", "curve", "dropper", "eraser",
        "field", "graphic", "help", "image", "lasso",
        "line", "oval", "pencil", "pointer",
        "polygon", "rectangle", "regular polygon",
        "rounded rectangle", "scrollbar", "player", "browser",
        "select", "spray", "text"
    };

const uint4 MCbuildnumber = MC_BUILD_REVISION;

const char *MCcopystring = "Copy of ";
const char *MCstandardstring = "standard";
const char *MCdialogstring = "dialog";
const char *MCmovablestring = "movable";
const char *MCpalettestring = "palette";
const char *MCmodalstring = "modal";
const char *MCsheetstring = "sheet";
const char *MCdrawerstring = "drawer";
const char *MCmodelessstring = "modeless";
const char *MCtoplevelstring = "toplevel";
const char *MCtransparentstring = "transparent";
const char *MCopaquestring = "opaque";
const char *MCrectanglestring = "rectangle";
const char *MCshadowstring = "shadow";
const char *MCscrollingstring = "scrolling";
const char *MCroundrectstring = "roundrect";
const char *MCcheckboxstring = "checkbox";
const char *MCradiobuttonstring = "radiobutton";
const char *MClinestring = "line";
const char *MCpolygonstring = "polygon";
const char *MCarcstring = "arc";
const char *MCovalstring = "oval";
const char *MCregularstring = "regular";
const char *MCcurvestring = "curve";
const char *MCtextstring = "text";
const char *MCleftstring = "left";
const char *MCcenterstring = "center";
const char *MCrightstring = "right";
const char *MCjustifystring = "justify";
const char *MCplainstring = "plain";
const char *MCmixedstring = "mixed";
const char *MCboxstring = "box";
const char *MCthreedboxstring = "threedbox";
const char *MCunderlinestring = "underline";
const char *MCstrikeoutstring = "strikeout";
const char *MCgroupstring = "group";
const char *MClinkstring = "link";
const char *MCtruestring = "true";
const char *MCfalsestring = "false";
const char *MCshiftstring = "shift";
const char *MCcommandstring = "command";
const char *MCcontrolstring = "control";
const char *MCmod1string = "alt";
const char *MCpulldownstring = "pulldown";
const char *MCpopupstring = "popup";
const char *MCoptionstring = "option";
const char *MCcascadestring = "cascade";
const char *MCcombostring = "combobox";
const char *MCtabstring = "tabbed";
const char *MCstackstring = "stack";
const char *MCaudiostring = "audioclip";
const char *MCvideostring = "videoclip";
const char *MCdefaultstring = "default";
const char *MCtitlestring = "title";
const char *MCmenustring = "menu";
const char *MCminimizestring = "minimize";
const char *MCmaximizestring = "maximize";
const char *MCclosestring = "close";
const char *MCmetalstring = "metal";
const char *MCutilitystring = "system";
const char *MCnoshadowstring = "noshadow";
const char *MCbackgroundstring = "background";
const char *MCforcetaskbarstring = "forcetaskbar";
const char *MCunicodestring = "unicode";
const char *MCnativestring = "native";

const char *MCcardstring = "card";
const char *MCbuttonstring = "button";
const char *MCgraphicstring = "graphic";
const char *MCepsstring = "EPS";
const char *MCscrollbarstring = "scrollbar";
const char *MCplayerstring = "player";
const char *MCscalestring = "scale";
const char *MCprogressstring = "progress";
const char *MCimagestring = "image";
const char *MCfieldstring = "field";
const char *MCcolorstring = "colorPalette";
const char *MCmagnifierstring = "magnifier";
const char *MCwidgetstring = "widget";

const char *MCnotfoundstring = "not found";
const char *MClnfamstring = "Appearance Manager";
const char *MClnfmacstring = "Macintosh";
const char *MClnfmotifstring = "Motif";
const char *MClnfwinstring = "Windows 95";
const char *MCuntitledstring = "Untitled";
// MW-2012-08-29: [[ Bug 10309 ]] Update 'applicationstring' to be 'LiveCode'.
const char *MCapplicationstring = "livecode";
const char *MChelpnamestring = "Help";
const char *MChomenamestring = "Home";
const char *MCdonestring = "done";
const char *MCnullstring = "";
const char *MCintersectstring = "intersect";
const char *MCsurroundstring = "surround";
const char *MCtopstring = "top";
const char *MCbottomstring = "bottom";
const char *MCcancelstring = "Cancel";

const char *MCtextalignstrings[] = { MCleftstring, MCcenterstring, MCrightstring, MCjustifystring };

MCNameRef MCN_msg;
MCNameRef MCN_each;
MCNameRef MCN_it;

MCNameRef MCN_cancel;

MCNameRef MCN_default_text_font;
MCNameRef MCN_platform_string;
MCNameRef MCN_version_string;

MCNameRef MCN_metadata;
MCNameRef MCN_runs;
MCNameRef MCN_style;

MCNameRef MCN_down;
MCNameRef MCN_up;

MCNameRef MCN_empty;
MCNameRef MCN_files;
MCNameRef MCN_image;
MCNameRef MCN_objects;
MCNameRef MCN_private;
MCNameRef MCN_text;
//MCNameRef MCN_unicode;
MCNameRef MCN_styles;
MCNameRef MCN_styledtext;
MCNameRef MCN_rtftext;
MCNameRef MCN_htmltext;
MCNameRef MCN_png;
MCNameRef MCN_gif;
MCNameRef MCN_jpeg;
MCNameRef MCN_rtf;
MCNameRef MCN_html;
MCNameRef MCN_win_bitmap;
MCNameRef MCN_win_metafile;
MCNameRef MCN_win_enh_metafile;

MCNameRef MCN_browser;
MCNameRef MCN_command_line;
MCNameRef MCN_development;
MCNameRef MCN_development_cmdline;
MCNameRef MCN_helper_application;
MCNameRef MCN_installer;
MCNameRef MCN_mobile;
MCNameRef MCN_player;
MCNameRef MCN_server;
MCNameRef MCN_standalone_application;
MCNameRef MCN_embedded;
MCNameRef MCN_mobile_embedded;

MCNameRef MCN_all;
MCNameRef MCN_auto_key;
MCNameRef MCN_disk;
MCNameRef MCN_activate;
MCNameRef MCN_high_level;
MCNameRef MCN_system;

MCNameRef MCN_ansi;
MCNameRef MCN_arabic;
MCNameRef MCN_bulgarian;
MCNameRef MCN_chinese;
MCNameRef MCN_english;
MCNameRef MCN_greek;
MCNameRef MCN_hebrew;
MCNameRef MCN_japanese;
MCNameRef MCN_korean;
MCNameRef MCN_lithuanian;
MCNameRef MCN_polish;
MCNameRef MCN_roman;
MCNameRef MCN_russian;
MCNameRef MCN_simple_chinese;
MCNameRef MCN_thai;
MCNameRef MCN_turkish;
MCNameRef MCN_ukrainian;
MCNameRef MCN_unicode;
MCNameRef MCN_utf8;
MCNameRef MCN_vietnamese;
MCNameRef MCN_w_char;
MCNameRef MCN_asterisk_char;

MCNameRef MCN_plain;
MCNameRef MCN_bold;
MCNameRef MCN_italic;
MCNameRef MCN_bold_italic;

MCNameRef MCN_unknown;
MCNameRef MCN_x86;
MCNameRef MCN_x86_64;
MCNameRef MCN_motorola_powerpc;
MCNameRef MCN_i386;
MCNameRef MCN_arm;
// SN-2015-01-07: [[ iOS-64bit ]] ARM64 added
MCNameRef MCN_arm64;

MCNameRef MCN_local_mac;
MCNameRef MCN_local_win32;
MCNameRef MCN_android;
MCNameRef MCN_iphone;
MCNameRef MCN_wince;

MCNameRef MCN_mac_os;
MCNameRef MCN_win32;

MCNameRef MCN_done;

MCNameRef MCN_staticgray;
MCNameRef MCN_grayscale;
MCNameRef MCN_staticcolor;
MCNameRef MCN_pseudocolor;
MCNameRef MCN_truecolor;
MCNameRef MCN_directcolor;

MCNameRef MCN_bounds;
MCNameRef MCN_pixels;
MCNameRef MCN_opaque_pixels;

MCNameRef MCN_desktop;
MCNameRef MCN_documents;
MCNameRef MCN_engine;
MCNameRef MCN_fonts;
MCNameRef MCN_resources;
MCNameRef MCN_home;
MCNameRef MCN_start;
//MCNameRef MCN_system;
MCNameRef MCN_temporary;
MCNameRef MCN_support;

MCNameRef MCN_apple;
MCNameRef MCN_control;
MCNameRef MCN_extension;
MCNameRef MCN_preferences;

MCNameRef MCN_unhandled;
MCNameRef MCN_handled;
MCNameRef MCN_passed;

MCNameRef MCN_page_setup_dialog;
MCNameRef MCN_pagesetup;
MCNameRef MCN_print_dialog;
MCNameRef MCN_printer;
MCNameRef MCN_color_chooser;
MCNameRef MCN_color;
MCNameRef MCN_file_selector;
MCNameRef MCN_file;
//MCNameRef MCN_files;
MCNameRef MCN_folder;
MCNameRef MCN_folders;
MCNameRef MCN_answer_dialog;
MCNameRef MCN_ask_dialog;

//MCNameRef MCN_plain;
MCNameRef MCN_clear;
//MCNameRef MCN_color;
MCNameRef MCN_effect;
MCNameRef MCN_error;
//MCNameRef MCN_file;
//MCNameRef MCN_folder;
MCNameRef MCN_information;
MCNameRef MCN_password;
//MCNameRef MCN_printer;
MCNameRef MCN_program;
MCNameRef MCN_question;
MCNameRef MCN_record;
MCNameRef MCN_titled;
MCNameRef MCN_warning;

MCNameRef MCN_messagename;
MCNameRef MCN_hcstat;
MCNameRef MCM_apple_event;
MCNameRef MCM_arrow_key;
MCNameRef MCM_assert_error;
MCNameRef MCM_backspace_key;
MCNameRef MCM_close_background;
MCNameRef MCM_close_card;
MCNameRef MCM_close_control;
MCNameRef MCM_close_field;
MCNameRef MCM_close_stack;
MCNameRef MCM_close_stack_request;
MCNameRef MCM_color_changed;
MCNameRef MCM_command_key_down;
MCNameRef MCM_control_key_down;
MCNameRef MCM_copy_key;
MCNameRef MCM_current_time_changed;
MCNameRef MCM_cut_key;
MCNameRef MCM_debug_str;

// AL-2014-11-27: [[ NewIdeMEssages ]] Add deleteAudioclip message
MCNameRef MCM_delete_audioclip;
MCNameRef MCM_delete_background;
MCNameRef MCM_delete_button;
MCNameRef MCM_delete_card;
MCNameRef MCM_delete_eps;
MCNameRef MCM_delete_field;
MCNameRef MCM_delete_graphic;
MCNameRef MCM_delete_group;
MCNameRef MCM_delete_image;
MCNameRef MCM_delete_key;
MCNameRef MCM_delete_scrollbar;
MCNameRef MCM_delete_player;
MCNameRef MCM_delete_stack;
// AL-2014-11-27: [[ NewIdeMEssages ]] Add deleteVideoclip message
MCNameRef MCM_delete_videoclip;
MCNameRef MCM_delete_widget;
MCNameRef MCM_delete_url;
MCNameRef MCM_desktop_changed;
MCNameRef MCM_drag_drop;
MCNameRef MCM_drag_end;
MCNameRef MCM_drag_enter;
MCNameRef MCM_drag_leave;
MCNameRef MCM_drag_move;
MCNameRef MCM_drag_start;
MCNameRef MCM_edit_script;
MCNameRef MCM_enter_in_field;
MCNameRef MCM_enter_key;
MCNameRef MCM_error_dialog;
MCNameRef MCM_escape_key;
MCNameRef MCM_eval;
MCNameRef MCM_exit_field;
MCNameRef MCM_focus_in;
MCNameRef MCM_focus_out;
MCNameRef MCM_function_key;
MCNameRef MCM_get_cached_urls;
MCNameRef MCM_get_url;
MCNameRef MCM_get_url_status;
MCNameRef MCM_gradient_edit_ended;
MCNameRef MCM_gradient_edit_started;
MCNameRef MCM_help;
MCNameRef MCM_hot_spot_clicked;
MCNameRef MCM_icon_menu_pick;
MCNameRef MCM_icon_menu_opening;
MCNameRef MCM_status_icon_menu_pick;
MCNameRef MCM_status_icon_menu_opening;
MCNameRef MCM_status_icon_click;
MCNameRef MCM_status_icon_double_click;
MCNameRef MCM_iconify_stack;
MCNameRef MCM_id_changed;
MCNameRef MCM_idle;
MCNameRef MCM_internal;
MCNameRef MCM_internal2;
MCNameRef MCM_internal3;
MCNameRef MCM_key_down;
MCNameRef MCM_key_up;
MCNameRef MCM_keyboard_activated;
MCNameRef MCM_keyboard_deactivated;
MCNameRef MCM_library_stack;
MCNameRef MCM_link_clicked;
MCNameRef MCM_load_url;
MCNameRef MCM_main_stack_changed;

// MW-2013-03-20: [[ MainStacksChanged ]]
MCNameRef MCM_main_stacks_changed;

MCNameRef MCM_menu_pick;
MCNameRef MCM_message;
MCNameRef MCM_message_handled;
MCNameRef MCM_message_not_handled;
MCNameRef MCM_mouse_double_down;
MCNameRef MCM_mouse_double_up;
MCNameRef MCM_mouse_down;
MCNameRef MCM_mouse_down_in_backdrop;
MCNameRef MCM_mouse_enter;
MCNameRef MCM_mouse_leave;
MCNameRef MCM_mouse_move;
MCNameRef MCM_mouse_release;
MCNameRef MCM_mouse_still_down;
MCNameRef MCM_mouse_up;
MCNameRef MCM_mouse_up_in_backdrop;
MCNameRef MCM_mouse_within;
MCNameRef MCM_move_control;
MCNameRef MCM_move_stack;
MCNameRef MCM_move_stopped;
MCNameRef MCM_movie_touched;
MCNameRef MCM_name_changed;
// AL-2014-11-27: [[ NewIdeMEssages ]] Add newAudioclip message
MCNameRef MCM_new_audioclip;
MCNameRef MCM_new_background;
MCNameRef MCM_new_card;
MCNameRef MCM_new_stack;
MCNameRef MCM_new_tool;
// AL-2014-11-27: [[ NewIdeMEssages ]] Add newVideoclip message
MCNameRef MCM_new_videoclip;
MCNameRef MCM_node_changed;
MCNameRef MCM_object_selection_ended;
MCNameRef MCM_object_selection_started;
MCNameRef MCM_open_background;
MCNameRef MCM_open_card;
MCNameRef MCM_open_control;
MCNameRef MCM_open_field;
MCNameRef MCM_open_stack;
MCNameRef MCM_option_key_down;
MCNameRef MCM_paste_key;
MCNameRef MCM_play_paused;
MCNameRef MCM_play_rate_changed;
MCNameRef MCM_play_started;
MCNameRef MCM_play_stopped;
MCNameRef MCM_post_url;
MCNameRef MCM_preopen_background;
MCNameRef MCM_preopen_card;
MCNameRef MCM_preopen_control;
MCNameRef MCM_preopen_stack;
MCNameRef MCM_property_changed;
MCNameRef MCM_put_url;
MCNameRef MCM_qtdebugstr;
MCNameRef MCM_raw_key_down;
MCNameRef MCM_raw_key_up;
MCNameRef MCM_relaunch;
MCNameRef MCM_release_stack;
MCNameRef MCM_reload_stack;
MCNameRef MCM_resize_control;
MCNameRef MCM_resize_control_ended;
MCNameRef MCM_resize_control_started;
MCNameRef MCM_resize_stack;
MCNameRef MCM_resolution_error;
MCNameRef MCM_resume;
MCNameRef MCM_resume_stack;
MCNameRef MCM_return_in_field;
MCNameRef MCM_return_key;
MCNameRef MCM_save_stack_request;
MCNameRef MCM_script_error;
MCNameRef MCM_script_execution_error;
MCNameRef MCM_scrollbar_beginning;
MCNameRef MCM_scrollbar_drag;
MCNameRef MCM_scrollbar_end;
MCNameRef MCM_scrollbar_line_dec;
MCNameRef MCM_scrollbar_line_inc;
MCNameRef MCM_scrollbar_page_dec;
MCNameRef MCM_scrollbar_page_inc;
MCNameRef MCM_selected_object_changed;
MCNameRef MCM_selection_changed;
MCNameRef MCM_signal;
MCNameRef MCM_shell;
MCNameRef MCM_shut_down;
MCNameRef MCM_shut_down_request;
MCNameRef MCM_socket_error;
MCNameRef MCM_socket_closed;
MCNameRef MCM_socket_timeout;
MCNameRef MCM_start_up;
MCNameRef MCM_suspend;
MCNameRef MCM_suspend_stack;
MCNameRef MCM_tab_key;
MCNameRef MCM_text_changed;
MCNameRef MCM_trace;
MCNameRef MCM_trace_break;
MCNameRef MCM_trace_done;
MCNameRef MCM_trace_error;
MCNameRef MCM_undo_changed;
MCNameRef MCM_undo_key;
MCNameRef MCM_uniconify_stack;
MCNameRef MCM_unload_url;
MCNameRef MCM_update_screen;
MCNameRef MCM_update_var;

#ifdef FEATURE_PLATFORM_URL
MCNameRef MCM_url_progress;
#endif

#ifdef _MOBILE
MCNameRef MCN_firstname;
MCNameRef MCN_lastname;
MCNameRef MCN_middlename;
MCNameRef MCN_prefix;
MCNameRef MCN_suffix;
MCNameRef MCN_nickname;
MCNameRef MCN_firstnamephonetic;
MCNameRef MCN_lastnamephonetic;
MCNameRef MCN_middlenamephonetic;
MCNameRef MCN_organization;
MCNameRef MCN_jobtitle;
MCNameRef MCN_department;
MCNameRef MCN_note;
MCNameRef MCN_email;
MCNameRef MCN_phone;
MCNameRef MCN_address;

//MCNameRef MCN_home;
MCNameRef MCN_work;
MCNameRef MCN_other;
//MCNameRef MCN_mobile;
//MCNameRef MCN_iphone;
MCNameRef MCN_main;
MCNameRef MCN_homefax;
MCNameRef MCN_workfax;
MCNameRef MCN_otherfax;
MCNameRef MCN_pager;

MCNameRef MCN_street;
MCNameRef MCN_city;
MCNameRef MCN_state;
MCNameRef MCN_zip;
MCNameRef MCN_country;
MCNameRef MCN_countrycode;

MCNameRef MCM_touch_start;
MCNameRef MCM_touch_move;
MCNameRef MCM_touch_end;
MCNameRef MCM_touch_release;
MCNameRef MCM_motion_start;
MCNameRef MCM_motion_end;
MCNameRef MCM_motion_release;
MCNameRef MCM_acceleration_changed;
MCNameRef MCM_orientation_changed;
MCNameRef MCM_location_changed;
MCNameRef MCM_location_error;
MCNameRef MCM_heading_changed;
MCNameRef MCM_heading_error;
MCNameRef MCM_purchase_updated;
MCNameRef MCM_rotation_rate_changed;
MCNameRef MCM_tracking_error;
MCNameRef MCM_local_notification_received;
MCNameRef MCM_push_notification_received;
MCNameRef MCM_push_notification_registered;
MCNameRef MCM_push_notification_registration_error;
MCNameRef MCM_url_wake_up;
MCNameRef MCM_launch_data_changed;
MCNameRef MCM_browser_started_loading;
MCNameRef MCM_browser_finished_loading;
MCNameRef MCM_browser_load_failed;
MCNameRef MCM_sound_finished_on_channel;
MCNameRef MCM_ad_loaded;
MCNameRef MCM_ad_clicked;
MCNameRef MCM_ad_load_failed;
MCNameRef MCM_ad_resize_start;
MCNameRef MCM_ad_resize_end;
MCNameRef MCM_ad_expand_start;
MCNameRef MCM_ad_expand_end;
MCNameRef MCM_scroller_did_scroll;
MCNameRef MCM_scroller_begin_drag;
MCNameRef MCM_scroller_end_drag;
MCNameRef MCM_player_finished;
MCNameRef MCM_player_error;
MCNameRef MCM_player_property_available;
MCNameRef MCM_input_begin_editing;
MCNameRef MCM_input_end_editing;
MCNameRef MCM_input_return_key;
MCNameRef MCM_input_text_changed;
MCNameRef MCM_product_details_received;
MCNameRef MCM_product_request_error;

#endif

#ifdef _IOS_MOBILE
MCNameRef MCM_browser_load_request;
MCNameRef MCM_browser_load_requested;
MCNameRef MCM_scroller_begin_decelerate;
MCNameRef MCM_scroller_end_decelerate;
MCNameRef MCM_scroller_scroll_to_top;
MCNameRef MCM_player_progress_changed;
MCNameRef MCM_player_enter_fullscreen;
MCNameRef MCM_player_leave_fullscreen;
MCNameRef MCM_player_state_changed;
MCNameRef MCM_player_movie_changed;
MCNameRef MCM_player_stopped;
MCNameRef MCM_reachability_changed;
//MCNameRef MCM_product_details_received;
//MCNameRef MCM_product_request_error;
MCNameRef MCM_protected_data_available;
MCNameRef MCM_protected_data_unavailable;

// MW-2013-05-30: [[ RemoteControl ]] Message sent when a remote control event is received.
MCNameRef MCM_remote_control_received;
#endif

MCNameRef MCN_font_default;
MCNameRef MCN_font_usertext;
MCNameRef MCN_font_menutext;
MCNameRef MCN_font_content;
MCNameRef MCN_font_message;
MCNameRef MCN_font_tooltip;
MCNameRef MCN_font_system;

void MCU_initialize_names(void)
{
	/* UNCHECKED */ MCNameCreateWithCString("msg", MCN_msg);
	/* UNCHECKED */ MCNameCreateWithCString("each", MCN_each);
	/* UNCHECKED */ MCNameCreateWithCString("it", MCN_it);
	
    // SN-2014-08-11: [[ Bug 13144 ]] Cancel string should be 'Cancel', not 'cancel'
	/* UNCHECKED */ MCNameCreateWithCString("Cancel", MCN_cancel);

	/* UNCHECKED */ MCNameCreateWithCString(DEFAULT_TEXT_FONT, MCN_default_text_font);
	/* UNCHECKED */ MCNameCreateWithCString(PLATFORM_STRING, MCN_platform_string);
	/* UNCHECKED */ MCNameCreateWithCString(MC_BUILD_ENGINE_SHORT_VERSION, MCN_version_string);

	/* UNCHECKED */ MCNameCreateWithCString("metadata", MCN_metadata);
	/* UNCHECKED */ MCNameCreateWithCString("runs", MCN_runs);
	/* UNCHECKED */ MCNameCreateWithCString("style", MCN_style);

	/* UNCHECKED */ MCNameCreateWithCString("down", MCN_down);
	/* UNCHECKED */ MCNameCreateWithCString("up", MCN_up);

	/* UNCHECKED */ MCNameCreateWithCString("empty", MCN_empty);
	/* UNCHECKED */ MCNameCreateWithCString("files", MCN_files);
	/* UNCHECKED */ MCNameCreateWithCString("image", MCN_image);
	/* UNCHECKED */ MCNameCreateWithCString("objects", MCN_objects);
	/* UNCHECKED */ MCNameCreateWithCString("private", MCN_private);
	/* UNCHECKED */ MCNameCreateWithCString("text", MCN_text);
//	/* UNCHECKED */ MCNameCreateWithCString("unicode", MCN_unicode);
	/* UNCHECKED */ MCNameCreateWithCString("styles", MCN_styles);
    /* UNCHECKED */ MCNameCreateWithCString("styledtext", MCN_styledtext);
    /* UNCHECKED */ MCNameCreateWithCString("rtftext", MCN_rtftext);
    /* UNCHECKED */ MCNameCreateWithCString("htmltext", MCN_htmltext);
    /* UNCHECKED */ MCNameCreateWithCString("png", MCN_png);
    /* UNCHECKED */ MCNameCreateWithCString("gif", MCN_gif);
    /* UNCHECKED */ MCNameCreateWithCString("jpeg", MCN_jpeg);
	/* UNCHECKED */ MCNameCreateWithCString("windows bitmap", MCN_win_bitmap);
	/* UNCHECKED */ MCNameCreateWithCString("windows metafile", MCN_win_metafile);
	/* UNCHECKED */ MCNameCreateWithCString("windows enhanced metafile", MCN_win_enh_metafile);
	/* UNCHECKED */ MCNameCreateWithCString("rtf", MCN_rtf);
	/* UNCHECKED */ MCNameCreateWithCString("html", MCN_html);

	/* UNCHECKED */ MCNameCreateWithCString("browser", MCN_browser);
	/* UNCHECKED */ MCNameCreateWithCString("command line", MCN_command_line);
	/* UNCHECKED */ MCNameCreateWithCString("development", MCN_development);
    /* UNCHECKED */ MCNameCreateWithCString("development command line", MCN_development_cmdline);
	/* UNCHECKED */ MCNameCreateWithCString("helper application", MCN_helper_application);
	/* UNCHECKED */ MCNameCreateWithCString("installer", MCN_installer);
	/* UNCHECKED */ MCNameCreateWithCString("mobile", MCN_mobile);
	/* UNCHECKED */ MCNameCreateWithCString("player", MCN_player);
	/* UNCHECKED */ MCNameCreateWithCString("server", MCN_server);
	/* UNCHECKED */ MCNameCreateWithCString("standalone application", MCN_standalone_application);
	/* UNCHECKED */ MCNameCreateWithCString("embedded", MCN_embedded);
	/* UNCHECKED */ MCNameCreateWithCString("mobile embedded", MCN_mobile_embedded);
    
	/* UNCHECKED */ MCNameCreateWithCString("all", MCN_all);
	/* UNCHECKED */ MCNameCreateWithCString("autokey", MCN_auto_key);
	/* UNCHECKED */ MCNameCreateWithCString("disk", MCN_disk);
	/* UNCHECKED */ MCNameCreateWithCString("activate", MCN_activate);
	/* UNCHECKED */ MCNameCreateWithCString("highlevel", MCN_high_level);
	/* UNCHECKED */ MCNameCreateWithCString("system", MCN_system);

	/* UNCHECKED */ MCNameCreateWithCString("ansi", MCN_ansi);
	/* UNCHECKED */ MCNameCreateWithCString("arabic", MCN_arabic);
	/* UNCHECKED */ MCNameCreateWithCString("bulgarian", MCN_bulgarian);
	/* UNCHECKED */ MCNameCreateWithCString("chinese", MCN_chinese);
	/* UNCHECKED */ MCNameCreateWithCString("english", MCN_english);
	/* UNCHECKED */ MCNameCreateWithCString("greek", MCN_greek);
	/* UNCHECKED */ MCNameCreateWithCString("hebrew", MCN_hebrew);
	/* UNCHECKED */ MCNameCreateWithCString("japanese", MCN_japanese);
	/* UNCHECKED */ MCNameCreateWithCString("korean", MCN_korean);
	/* UNCHECKED */ MCNameCreateWithCString("lithuanian", MCN_lithuanian);
	/* UNCHECKED */ MCNameCreateWithCString("polish", MCN_polish);
	/* UNCHECKED */ MCNameCreateWithCString("roman", MCN_roman);
	/* UNCHECKED */ MCNameCreateWithCString("russian", MCN_russian);
	/* UNCHECKED */ MCNameCreateWithCString("simpleChinese", MCN_simple_chinese);
	/* UNCHECKED */ MCNameCreateWithCString("thai", MCN_thai);
	/* UNCHECKED */ MCNameCreateWithCString("turkish", MCN_turkish);
	/* UNCHECKED */ MCNameCreateWithCString("ukrainian", MCN_ukrainian);
	/* UNCHECKED */ MCNameCreateWithCString("unicode", MCN_unicode);
	/* UNCHECKED */ MCNameCreateWithCString("utf8", MCN_utf8);
	/* UNCHECKED */ MCNameCreateWithCString("vietnamese", MCN_vietnamese);
	/* UNCHECKED */ MCNameCreateWithCString("w", MCN_w_char);
	/* UNCHECKED */ MCNameCreateWithCString("*", MCN_asterisk_char);

	/* UNCHECKED */ MCNameCreateWithCString("plain", MCN_plain);
	/* UNCHECKED */ MCNameCreateWithCString("bold", MCN_bold);
	/* UNCHECKED */ MCNameCreateWithCString("italic", MCN_italic);
	/* UNCHECKED */ MCNameCreateWithCString("bold-italic", MCN_bold_italic);

	/* UNCHECKED */ MCNameCreateWithCString("unknown", MCN_unknown);
	/* UNCHECKED */ MCNameCreateWithCString("x86", MCN_x86);
    /* UNCHECKED */ MCNameCreateWithCString("x86_64", MCN_x86_64);
	/* UNCHECKED */ MCNameCreateWithCString("Motorola PowerPC", MCN_motorola_powerpc);
	/* UNCHECKED */ MCNameCreateWithCString("i386", MCN_i386);
	/* UNCHECKED */ MCNameCreateWithCString("ARM", MCN_arm);
    // SN-2015-01-07: [[ iOS-64bit ]] ARM64 added
    /* UNCHECKED */ MCNameCreateWithCString("arm64", MCN_arm64);

	/* UNCHECKED */ MCNameCreateWithCString("local Mac", MCN_local_mac);
	/* UNCHECKED */ MCNameCreateWithCString("local Win32", MCN_local_win32);
	/* UNCHECKED */ MCNameCreateWithCString("android", MCN_android);
	/* UNCHECKED */ MCNameCreateWithCString("iphone", MCN_iphone);
	/* UNCHECKED */ MCNameCreateWithCString("wince", MCN_wince);

	/* UNCHECKED */ MCNameCreateWithCString("Mac OS", MCN_mac_os);
	/* UNCHECKED */ MCNameCreateWithCString("Win32", MCN_win32);

	/* UNCHECKED */ MCNameCreateWithCString("done", MCN_done);

	/* UNCHECKED */ MCNameCreateWithCString("StaticGray", MCN_staticgray);
	/* UNCHECKED */ MCNameCreateWithCString("GrayScale", MCN_grayscale);
	/* UNCHECKED */ MCNameCreateWithCString("StaticColor", MCN_staticcolor);
	/* UNCHECKED */ MCNameCreateWithCString("PseudoColor", MCN_pseudocolor);
	/* UNCHECKED */ MCNameCreateWithCString("TrueColor", MCN_truecolor);
	/* UNCHECKED */ MCNameCreateWithCString("DirectColor", MCN_directcolor);

	/* UNCHECKED */ MCNameCreateWithCString("bounds", MCN_bounds);
	/* UNCHECKED */ MCNameCreateWithCString("pixels", MCN_pixels);
	/* UNCHECKED */ MCNameCreateWithCString("opaque pixels", MCN_opaque_pixels);

	/* UNCHECKED */ MCNameCreateWithCString("desktop", MCN_desktop);
	/* UNCHECKED */ MCNameCreateWithCString("documents", MCN_documents);
	/* UNCHECKED */ MCNameCreateWithCString("engine", MCN_engine);
    /* UNCHECKED */ MCNameCreateWithCString("resources", MCN_resources);
	/* UNCHECKED */ MCNameCreateWithCString("fonts", MCN_fonts);
	/* UNCHECKED */ MCNameCreateWithCString("home", MCN_home);
	/* UNCHECKED */ MCNameCreateWithCString("start", MCN_start);
//	/* UNCHECKED */ MCNameCreateWithCString("system", MCN_system);
	/* UNCHECKED */ MCNameCreateWithCString("temporary", MCN_temporary);
	/* UNCHECKED */ MCNameCreateWithCString("support", MCN_support);

	/* UNCHECKED */ MCNameCreateWithCString("Apple", MCN_apple);
	/* UNCHECKED */ MCNameCreateWithCString("Control", MCN_control);
	/* UNCHECKED */ MCNameCreateWithCString("Extension", MCN_extension);
	/* UNCHECKED */ MCNameCreateWithCString("Preferences", MCN_preferences);
	
	/* UNCHECKED */ MCNameCreateWithCString("unhandled", MCN_unhandled);
	/* UNCHECKED */ MCNameCreateWithCString("handled", MCN_handled);
	/* UNCHECKED */ MCNameCreateWithCString("passed", MCN_passed);

	/* UNCHECKED */ MCNameCreateWithCString("Page Setup Dialog", MCN_page_setup_dialog);
	/* UNCHECKED */ MCNameCreateWithCString("pagesetup", MCN_pagesetup);
	/* UNCHECKED */ MCNameCreateWithCString("Print Dialog", MCN_print_dialog);
	/* UNCHECKED */ MCNameCreateWithCString("printer", MCN_printer);
	/* UNCHECKED */ MCNameCreateWithCString("Color Chooser", MCN_color_chooser);
	/* UNCHECKED */ MCNameCreateWithCString("color", MCN_color);
	/* UNCHECKED */ MCNameCreateWithCString("File Selector", MCN_file_selector);
	/* UNCHECKED */ MCNameCreateWithCString("file", MCN_file);
	///* UNCHECKED */ MCNameCreateWithCString("files", MCN_files);
	/* UNCHECKED */ MCNameCreateWithCString("folder", MCN_folder);
	/* UNCHECKED */ MCNameCreateWithCString("folders", MCN_folders);
	/* UNCHECKED */ MCNameCreateWithCString("Answer Dialog", MCN_answer_dialog);
	/* UNCHECKED */ MCNameCreateWithCString("Ask Dialog", MCN_ask_dialog);

	///* UNCHECKED */ MCNameCreateWithCString("plain", MCN_plain);
	/* UNCHECKED */ MCNameCreateWithCString("clear", MCN_clear);
	///* UNCHECKED */ MCNameCreateWithCString("color", MCN_color);
	/* UNCHECKED */ MCNameCreateWithCString("effect", MCN_effect);
	/* UNCHECKED */ MCNameCreateWithCString("error", MCN_error);
	///* UNCHECKED */ MCNameCreateWithCString("file", MCN_file);
	///* UNCHECKED */ MCNameCreateWithCString("folder", MCN_folder);
	/* UNCHECKED */ MCNameCreateWithCString("information", MCN_information);
	/* UNCHECKED */ MCNameCreateWithCString("password", MCN_password);
	///* UNCHECKED */ MCNameCreateWithCString("printer", MCN_printer);
	/* UNCHECKED */ MCNameCreateWithCString("program", MCN_program);
	/* UNCHECKED */ MCNameCreateWithCString("question", MCN_question);
	/* UNCHECKED */ MCNameCreateWithCString("record", MCN_record);
	/* UNCHECKED */ MCNameCreateWithCString("titled", MCN_titled);
	/* UNCHECKED */ MCNameCreateWithCString("warning", MCN_warning);

	/* UNCHECKED */ MCNameCreateWithCString("Message Box", MCN_messagename);
	/* UNCHECKED */ MCNameCreateWithCString("HyperCard Import Status", MCN_hcstat);
	/* UNCHECKED */ MCNameCreateWithCString("appleEvent", MCM_apple_event);
	/* UNCHECKED */ MCNameCreateWithCString("arrowKey", MCM_arrow_key);
	/* UNCHECKED */ MCNameCreateWithCString("assertError", MCM_assert_error);
	/* UNCHECKED */ MCNameCreateWithCString("backspaceKey", MCM_backspace_key);
	/* UNCHECKED */ MCNameCreateWithCString("closeBackground", MCM_close_background);
	/* UNCHECKED */ MCNameCreateWithCString("closeCard", MCM_close_card);
	/* UNCHECKED */ MCNameCreateWithCString("closeControl", MCM_close_control);
	/* UNCHECKED */ MCNameCreateWithCString("closeField", MCM_close_field);
	/* UNCHECKED */ MCNameCreateWithCString("closeStack", MCM_close_stack);
	/* UNCHECKED */ MCNameCreateWithCString("closeStackRequest", MCM_close_stack_request);
	/* UNCHECKED */ MCNameCreateWithCString("colorChanged", MCM_color_changed);
	/* UNCHECKED */ MCNameCreateWithCString("commandKeyDown", MCM_command_key_down);
	/* UNCHECKED */ MCNameCreateWithCString("controlKeyDown", MCM_control_key_down);
	/* UNCHECKED */ MCNameCreateWithCString("copyKey", MCM_copy_key);
	/* UNCHECKED */ MCNameCreateWithCString("currentTimeChanged", MCM_current_time_changed);
	/* UNCHECKED */ MCNameCreateWithCString("cutKey", MCM_cut_key);
	/* UNCHECKED */ MCNameCreateWithCString("debugStr", MCM_debug_str);
	/* UNCHECKED */ MCNameCreateWithCString("deleteBackground", MCM_delete_background);
	/* UNCHECKED */ MCNameCreateWithCString("deleteButton", MCM_delete_button);
	/* UNCHECKED */ MCNameCreateWithCString("deleteCard", MCM_delete_card);
	/* UNCHECKED */ MCNameCreateWithCString("deleteEPS", MCM_delete_eps);
	/* UNCHECKED */ MCNameCreateWithCString("deleteField", MCM_delete_field);
	/* UNCHECKED */ MCNameCreateWithCString("deleteGraphic", MCM_delete_graphic);
	/* UNCHECKED */ MCNameCreateWithCString("deleteGroup", MCM_delete_group);
	/* UNCHECKED */ MCNameCreateWithCString("deleteImage", MCM_delete_image);
	/* UNCHECKED */ MCNameCreateWithCString("deleteKey", MCM_delete_key);
	/* UNCHECKED */ MCNameCreateWithCString("deleteScrollbar", MCM_delete_scrollbar);
	/* UNCHECKED */ MCNameCreateWithCString("deletePlayer", MCM_delete_player);
	/* UNCHECKED */ MCNameCreateWithCString("deleteStack", MCM_delete_stack);
	/* UNCHECKED */ MCNameCreateWithCString("deleteWidget", MCM_delete_widget);
	/* UNCHECKED */ MCNameCreateWithCString("deleteURL", MCM_delete_url);
	/* UNCHECKED */ MCNameCreateWithCString("desktopChanged", MCM_desktop_changed);
	/* UNCHECKED */ MCNameCreateWithCString("dragDrop", MCM_drag_drop);
	/* UNCHECKED */ MCNameCreateWithCString("dragEnd", MCM_drag_end);
	/* UNCHECKED */ MCNameCreateWithCString("dragEnter", MCM_drag_enter);
	/* UNCHECKED */ MCNameCreateWithCString("dragLeave", MCM_drag_leave);
	/* UNCHECKED */ MCNameCreateWithCString("dragMove", MCM_drag_move);
	/* UNCHECKED */ MCNameCreateWithCString("dragStart", MCM_drag_start);
	/* UNCHECKED */ MCNameCreateWithCString("editScript", MCM_edit_script);
	/* UNCHECKED */ MCNameCreateWithCString("enterInField", MCM_enter_in_field);
	/* UNCHECKED */ MCNameCreateWithCString("enterKey", MCM_enter_key);
	/* UNCHECKED */ MCNameCreateWithCString("errorDialog", MCM_error_dialog);
	/* UNCHECKED */ MCNameCreateWithCString("escapeKey", MCM_escape_key);
	/* UNCHECKED */ MCNameCreateWithCString("eval", MCM_eval);
	/* UNCHECKED */ MCNameCreateWithCString("exitField", MCM_exit_field);
	/* UNCHECKED */ MCNameCreateWithCString("focusIn", MCM_focus_in);
	/* UNCHECKED */ MCNameCreateWithCString("focusOut", MCM_focus_out);
	/* UNCHECKED */ MCNameCreateWithCString("functionKey", MCM_function_key);
	/* UNCHECKED */ MCNameCreateWithCString("getCachedURLs", MCM_get_cached_urls);
	/* UNCHECKED */ MCNameCreateWithCString("getURL", MCM_get_url);
	/* UNCHECKED */ MCNameCreateWithCString("getURLStatus", MCM_get_url_status);
	/* UNCHECKED */ MCNameCreateWithCString("gradientEditEnded", MCM_gradient_edit_ended);
	/* UNCHECKED */ MCNameCreateWithCString("gradientEditStarted", MCM_gradient_edit_started);
	/* UNCHECKED */ MCNameCreateWithCString("help", MCM_help);
	/* UNCHECKED */ MCNameCreateWithCString("hotSpotClicked", MCM_hot_spot_clicked);
	/* UNCHECKED */ MCNameCreateWithCString("iconMenuPick", MCM_icon_menu_pick);
	/* UNCHECKED */ MCNameCreateWithCString("iconMenuOpening", MCM_icon_menu_opening);
	/* UNCHECKED */ MCNameCreateWithCString("statusIconMenuPick", MCM_status_icon_menu_pick);
	/* UNCHECKED */ MCNameCreateWithCString("statusIconMenuOpening", MCM_status_icon_menu_opening);
	/* UNCHECKED */ MCNameCreateWithCString("statusIconClick", MCM_status_icon_click);
	/* UNCHECKED */ MCNameCreateWithCString("statusIconDoubleClick", MCM_status_icon_double_click);
	/* UNCHECKED */ MCNameCreateWithCString("iconifyStack", MCM_iconify_stack);
	/* UNCHECKED */ MCNameCreateWithCString("idChanged", MCM_id_changed);
	/* UNCHECKED */ MCNameCreateWithCString("idle", MCM_idle);
	/* UNCHECKED */ MCNameCreateWithCString("internal", MCM_internal);
	/* UNCHECKED */ MCNameCreateWithCString("internal2", MCM_internal2);
	/* UNCHECKED */ MCNameCreateWithCString("internal3", MCM_internal3);
	/* UNCHECKED */ MCNameCreateWithCString("keyDown", MCM_key_down);
	/* UNCHECKED */ MCNameCreateWithCString("keyUp", MCM_key_up);
	/* UNCHECKED */ MCNameCreateWithCString("keyboardActivated", MCM_keyboard_activated);
	/* UNCHECKED */ MCNameCreateWithCString("keyboardDeactivated", MCM_keyboard_deactivated);
	/* UNCHECKED */ MCNameCreateWithCString("libraryStack", MCM_library_stack);
	/* UNCHECKED */ MCNameCreateWithCString("linkClicked", MCM_link_clicked);
	/* UNCHECKED */ MCNameCreateWithCString("loadURL", MCM_load_url);
	/* UNCHECKED */ MCNameCreateWithCString("mainStackChanged", MCM_main_stack_changed);
	// MW-2013-03-20: [[ MainStacksChanged ]]
	/* UNCHECKED */ MCNameCreateWithCString("_mainStacksChanged", MCM_main_stacks_changed);
	/* UNCHECKED */ MCNameCreateWithCString("menuPick", MCM_menu_pick);
	/* UNCHECKED */ MCNameCreateWithCString("message", MCM_message);
	/* UNCHECKED */ MCNameCreateWithCString("messageHandled", MCM_message_handled);
	/* UNCHECKED */ MCNameCreateWithCString("messageNotHandled", MCM_message_not_handled);
	/* UNCHECKED */ MCNameCreateWithCString("mouseDoubleDown", MCM_mouse_double_down);
	/* UNCHECKED */ MCNameCreateWithCString("mouseDoubleUp", MCM_mouse_double_up);
	/* UNCHECKED */ MCNameCreateWithCString("mouseDown", MCM_mouse_down);
	/* UNCHECKED */ MCNameCreateWithCString("mouseDownInBackdrop", MCM_mouse_down_in_backdrop);
	/* UNCHECKED */ MCNameCreateWithCString("mouseEnter", MCM_mouse_enter);
	/* UNCHECKED */ MCNameCreateWithCString("mouseLeave", MCM_mouse_leave);
	/* UNCHECKED */ MCNameCreateWithCString("mouseMove", MCM_mouse_move);
	/* UNCHECKED */ MCNameCreateWithCString("mouseRelease", MCM_mouse_release);
	/* UNCHECKED */ MCNameCreateWithCString("mouseStillDown", MCM_mouse_still_down);
	/* UNCHECKED */ MCNameCreateWithCString("mouseUp", MCM_mouse_up);
	/* UNCHECKED */ MCNameCreateWithCString("mouseUpInBackdrop", MCM_mouse_up_in_backdrop);
	/* UNCHECKED */ MCNameCreateWithCString("mouseWithin", MCM_mouse_within);
	/* UNCHECKED */ MCNameCreateWithCString("moveControl", MCM_move_control);
	/* UNCHECKED */ MCNameCreateWithCString("moveStack", MCM_move_stack);
	/* UNCHECKED */ MCNameCreateWithCString("moveStopped", MCM_move_stopped);
	/* UNCHECKED */ MCNameCreateWithCString("movieTouched", MCM_movie_touched);
	/* UNCHECKED */ MCNameCreateWithCString("nameChanged", MCM_name_changed);
	/* UNCHECKED */ MCNameCreateWithCString("newBackground", MCM_new_background);
	/* UNCHECKED */ MCNameCreateWithCString("newCard", MCM_new_card);
	/* UNCHECKED */ MCNameCreateWithCString("newStack", MCM_new_stack);
	/* UNCHECKED */ MCNameCreateWithCString("newTool", MCM_new_tool);
	/* UNCHECKED */ MCNameCreateWithCString("nodeChanged", MCM_node_changed);
	/* UNCHECKED */ MCNameCreateWithCString("objectSelectionEnded", MCM_object_selection_ended);
	/* UNCHECKED */ MCNameCreateWithCString("objectSelectionStarted", MCM_object_selection_started);
	/* UNCHECKED */ MCNameCreateWithCString("openBackground", MCM_open_background);
	/* UNCHECKED */ MCNameCreateWithCString("openCard", MCM_open_card);
	/* UNCHECKED */ MCNameCreateWithCString("openControl", MCM_open_control);
	/* UNCHECKED */ MCNameCreateWithCString("openField", MCM_open_field);
	/* UNCHECKED */ MCNameCreateWithCString("openStack", MCM_open_stack);
	/* UNCHECKED */ MCNameCreateWithCString("optionKeyDown", MCM_option_key_down);
	/* UNCHECKED */ MCNameCreateWithCString("pasteKey", MCM_paste_key);
	/* UNCHECKED */ MCNameCreateWithCString("playPaused", MCM_play_paused);
    /* UNCHECKED */ MCNameCreateWithCString("playRateChanged", MCM_play_rate_changed);
	/* UNCHECKED */ MCNameCreateWithCString("playStarted", MCM_play_started);
	/* UNCHECKED */ MCNameCreateWithCString("playStopped", MCM_play_stopped);
	/* UNCHECKED */ MCNameCreateWithCString("postURL", MCM_post_url);
	/* UNCHECKED */ MCNameCreateWithCString("preOpenBackground", MCM_preopen_background);
	/* UNCHECKED */ MCNameCreateWithCString("preOpenCard", MCM_preopen_card);
	/* UNCHECKED */ MCNameCreateWithCString("preOpenControl", MCM_preopen_control);
	/* UNCHECKED */ MCNameCreateWithCString("preOpenStack", MCM_preopen_stack);
	/* UNCHECKED */ MCNameCreateWithCString("propertyChanged", MCM_property_changed);
	/* UNCHECKED */ MCNameCreateWithCString("putURL", MCM_put_url);
	/* UNCHECKED */ MCNameCreateWithCString("QTDebugStr", MCM_qtdebugstr);
	/* UNCHECKED */ MCNameCreateWithCString("rawKeyDown", MCM_raw_key_down);
	/* UNCHECKED */ MCNameCreateWithCString("rawKeyUp", MCM_raw_key_up);
	/* UNCHECKED */ MCNameCreateWithCString("relaunch", MCM_relaunch);
	/* UNCHECKED */ MCNameCreateWithCString("releaseStack", MCM_release_stack);
	/* UNCHECKED */ MCNameCreateWithCString("reloadStack", MCM_reload_stack);
	/* UNCHECKED */ MCNameCreateWithCString("resizeControl", MCM_resize_control);
	/* UNCHECKED */ MCNameCreateWithCString("resizeControlEnded", MCM_resize_control_ended);
	/* UNCHECKED */ MCNameCreateWithCString("resizeControlStarted", MCM_resize_control_started);
	/* UNCHECKED */ MCNameCreateWithCString("resizeStack", MCM_resize_stack);
	/* UNCHECKED */ MCNameCreateWithCString("resolutionError", MCM_resolution_error);
	/* UNCHECKED */ MCNameCreateWithCString("resume", MCM_resume);
	/* UNCHECKED */ MCNameCreateWithCString("resumeStack", MCM_resume_stack);
	/* UNCHECKED */ MCNameCreateWithCString("returnInField", MCM_return_in_field);
	/* UNCHECKED */ MCNameCreateWithCString("returnKey", MCM_return_key);
	/* UNCHECKED */ MCNameCreateWithCString("saveStackRequest", MCM_save_stack_request);
	/* UNCHECKED */ MCNameCreateWithCString("scriptParsingError", MCM_script_error);
	/* UNCHECKED */ MCNameCreateWithCString("scriptExecutionError", MCM_script_execution_error);
	/* UNCHECKED */ MCNameCreateWithCString("scrollbarBeginning", MCM_scrollbar_beginning);
	/* UNCHECKED */ MCNameCreateWithCString("scrollbarDrag", MCM_scrollbar_drag);
	/* UNCHECKED */ MCNameCreateWithCString("scrollbarEnd", MCM_scrollbar_end);
	/* UNCHECKED */ MCNameCreateWithCString("scrollbarLineDec", MCM_scrollbar_line_dec);
	/* UNCHECKED */ MCNameCreateWithCString("scrollbarLineInc", MCM_scrollbar_line_inc);
	/* UNCHECKED */ MCNameCreateWithCString("scrollbarPageDec", MCM_scrollbar_page_dec);
	/* UNCHECKED */ MCNameCreateWithCString("scrollbarPageInc", MCM_scrollbar_page_inc);
	/* UNCHECKED */ MCNameCreateWithCString("selectedObjectChanged", MCM_selected_object_changed);
	/* UNCHECKED */ MCNameCreateWithCString("selectionChanged", MCM_selection_changed);
	/* UNCHECKED */ MCNameCreateWithCString("shell", MCM_shell);
	/* UNCHECKED */ MCNameCreateWithCString("signal", MCM_signal);
	/* UNCHECKED */ MCNameCreateWithCString("shutDown", MCM_shut_down);
	/* UNCHECKED */ MCNameCreateWithCString("shutDownRequest", MCM_shut_down_request);
	/* UNCHECKED */ MCNameCreateWithCString("socketError", MCM_socket_error);
	/* UNCHECKED */ MCNameCreateWithCString("socketClosed", MCM_socket_closed);
	/* UNCHECKED */ MCNameCreateWithCString("socketTimeout", MCM_socket_timeout);
	/* UNCHECKED */ MCNameCreateWithCString("startUp", MCM_start_up);
	/* UNCHECKED */ MCNameCreateWithCString("suspend", MCM_suspend);
	/* UNCHECKED */ MCNameCreateWithCString("suspendStack", MCM_suspend_stack);
	/* UNCHECKED */ MCNameCreateWithCString("tabKey", MCM_tab_key);
	/* UNCHECKED */ MCNameCreateWithCString("textChanged", MCM_text_changed);
	/* UNCHECKED */ MCNameCreateWithCString("trace", MCM_trace);
	/* UNCHECKED */ MCNameCreateWithCString("traceBreak", MCM_trace_break);
	/* UNCHECKED */ MCNameCreateWithCString("traceDone", MCM_trace_done);
	/* UNCHECKED */ MCNameCreateWithCString("traceError", MCM_trace_error);
	/* UNCHECKED */ MCNameCreateWithCString("undoChanged", MCM_undo_changed);
	/* UNCHECKED */ MCNameCreateWithCString("undoKey", MCM_undo_key);
	/* UNCHECKED */ MCNameCreateWithCString("uniconifyStack", MCM_uniconify_stack);
	/* UNCHECKED */ MCNameCreateWithCString("unloadURL", MCM_unload_url);
	/* UNCHECKED */ MCNameCreateWithCString("updateScreen", MCM_update_screen);
	/* UNCHECKED */ MCNameCreateWithCString("updateVariable", MCM_update_var);

#ifdef FEATURE_PLATFORM_URL
	/* UNCHECKED */ MCNameCreateWithCString("urlProgress", MCM_url_progress);
#endif

    
    /* UNCHECKED */ MCNameCreateWithCString("deleteAudioclip", MCM_delete_audioclip);
    /* UNCHECKED */ MCNameCreateWithCString("deleteVideoclip", MCM_delete_videoclip);
    /* UNCHECKED */ MCNameCreateWithCString("newAudioclip", MCM_new_audioclip);
    /* UNCHECKED */ MCNameCreateWithCString("newVideoclip", MCM_new_videoclip);
    
#ifdef _MOBILE
	/* UNCHECKED */ MCNameCreateWithCString("firstname", MCN_firstname);
	/* UNCHECKED */ MCNameCreateWithCString("lastname", MCN_lastname);
	/* UNCHECKED */ MCNameCreateWithCString("middlename", MCN_middlename);
	/* UNCHECKED */ MCNameCreateWithCString("prefix", MCN_prefix);
	/* UNCHECKED */ MCNameCreateWithCString("suffix", MCN_suffix);
	/* UNCHECKED */ MCNameCreateWithCString("nickname", MCN_nickname);
	/* UNCHECKED */ MCNameCreateWithCString("firstnamephonetic", MCN_firstnamephonetic);
	/* UNCHECKED */ MCNameCreateWithCString("lastnamephonetic", MCN_lastnamephonetic);
	/* UNCHECKED */ MCNameCreateWithCString("middlenamephonetic", MCN_middlenamephonetic);
	/* UNCHECKED */ MCNameCreateWithCString("organization", MCN_organization);
	/* UNCHECKED */ MCNameCreateWithCString("jobtitle", MCN_jobtitle);
	/* UNCHECKED */ MCNameCreateWithCString("department", MCN_department);
	/* UNCHECKED */ MCNameCreateWithCString("note", MCN_note);

	/* UNCHECKED */ MCNameCreateWithCString("email", MCN_email);
	/* UNCHECKED */ MCNameCreateWithCString("phone", MCN_phone);
	/* UNCHECKED */ MCNameCreateWithCString("address", MCN_address);

//	/* UNCHECKED */ MCNameCreateWithCString("home", MCN_home);
	/* UNCHECKED */ MCNameCreateWithCString("work", MCN_work);
	/* UNCHECKED */ MCNameCreateWithCString("other", MCN_other);

//	/* UNCHECKED */ MCNameCreateWithCString("mobile", MCN_mobile);
//	/* UNCHECKED */ MCNameCreateWithCString("iphone", MCN_iphone);
	/* UNCHECKED */ MCNameCreateWithCString("main", MCN_main);
	/* UNCHECKED */ MCNameCreateWithCString("homefax", MCN_homefax);
	/* UNCHECKED */ MCNameCreateWithCString("workfax", MCN_workfax);
	/* UNCHECKED */ MCNameCreateWithCString("otherfax", MCN_otherfax);
	/* UNCHECKED */ MCNameCreateWithCString("pager", MCN_pager);
	
	/* UNCHECKED */ MCNameCreateWithCString("street", MCN_street);
	/* UNCHECKED */ MCNameCreateWithCString("city", MCN_city);
	/* UNCHECKED */ MCNameCreateWithCString("state", MCN_state);
	/* UNCHECKED */ MCNameCreateWithCString("zip", MCN_zip);
	/* UNCHECKED */ MCNameCreateWithCString("country", MCN_country);
	/* UNCHECKED */ MCNameCreateWithCString("countrycode", MCN_countrycode);
	
	
	/* UNCHECKED */ MCNameCreateWithCString("touchStart", MCM_touch_start);
	/* UNCHECKED */ MCNameCreateWithCString("touchMove", MCM_touch_move);
	/* UNCHECKED */ MCNameCreateWithCString("touchEnd", MCM_touch_end);
	/* UNCHECKED */ MCNameCreateWithCString("touchRelease", MCM_touch_release);
	/* UNCHECKED */ MCNameCreateWithCString("motionStart", MCM_motion_start);
	/* UNCHECKED */ MCNameCreateWithCString("motionEnd", MCM_motion_end);
	/* UNCHECKED */ MCNameCreateWithCString("motionRelease", MCM_motion_release);
	/* UNCHECKED */ MCNameCreateWithCString("accelerationChanged", MCM_acceleration_changed);
	/* UNCHECKED */ MCNameCreateWithCString("orientationChanged", MCM_orientation_changed);
	/* UNCHECKED */ MCNameCreateWithCString("locationChanged", MCM_location_changed);
	/* UNCHECKED */ MCNameCreateWithCString("locationError", MCM_location_error);
	/* UNCHECKED */ MCNameCreateWithCString("headingChanged", MCM_heading_changed);
	/* UNCHECKED */ MCNameCreateWithCString("headingError", MCM_heading_error);
	/* UNCHECKED */ MCNameCreateWithCString("purchaseStateUpdate", MCM_purchase_updated);
    /* UNCHECKED */ MCNameCreateWithCString("rotationRateChanged", MCM_rotation_rate_changed);
	/* UNCHECKED */ MCNameCreateWithCString("trackingError", MCM_tracking_error);
    /* UNCHECKED */ MCNameCreateWithCString("localNotificationReceived", MCM_local_notification_received);
    /* UNCHECKED */ MCNameCreateWithCString("pushNotificationReceived", MCM_push_notification_received);
    /* UNCHECKED */ MCNameCreateWithCString("pushNotificationRegistered", MCM_push_notification_registered);
    /* UNCHECKED */ MCNameCreateWithCString("pushNotificationRegistrationError", MCM_push_notification_registration_error);
    /* UNCHECKED */ MCNameCreateWithCString("urlWakeUp", MCM_url_wake_up);
	/* UNCHECKED */ MCNameCreateWithCString("launchDataChanged", MCM_launch_data_changed);
	/* UNCHECKED */ MCNameCreateWithCString("browserStartedLoading", MCM_browser_started_loading);
	/* UNCHECKED */ MCNameCreateWithCString("browserFinishedLoading", MCM_browser_finished_loading);
	/* UNCHECKED */ MCNameCreateWithCString("browserLoadFailed", MCM_browser_load_failed);
    /* UNCHECKED */ MCNameCreateWithCString("soundFinishedOnChannel", MCM_sound_finished_on_channel);
    /* UNCHECKED */ MCNameCreateWithCString("adLoaded", MCM_ad_loaded);
	/* UNCHECKED */ MCNameCreateWithCString("adClicked", MCM_ad_clicked);
    /* UNCHECKED */ MCNameCreateWithCString("adLoadFailed", MCM_ad_load_failed);
    /* UNCHECKED */ MCNameCreateWithCString("adResizeStart", MCM_ad_resize_start);
    /* UNCHECKED */ MCNameCreateWithCString("adResizeEnd", MCM_ad_resize_end);
    /* UNCHECKED */ MCNameCreateWithCString("adExpandStart", MCM_ad_expand_start);
    /* UNCHECKED */ MCNameCreateWithCString("adExpandEnd", MCM_ad_expand_end);
	/* UNCHECKED */ MCNameCreateWithCString("scrollerDidScroll", MCM_scroller_did_scroll);
	/* UNCHECKED */ MCNameCreateWithCString("scrollerBeginDrag", MCM_scroller_begin_drag);
	/* UNCHECKED */ MCNameCreateWithCString("scrollerEndDrag", MCM_scroller_end_drag);
	/* UNCHECKED */ MCNameCreateWithCString("playerFinished", MCM_player_finished);
	/* UNCHECKED */ MCNameCreateWithCString("playerError", MCM_player_error);
	/* UNCHECKED */ MCNameCreateWithCString("playerPropertyAvailable", MCM_player_property_available);
	/* UNCHECKED */ MCNameCreateWithCString("inputBeginEditing", MCM_input_begin_editing);
	/* UNCHECKED */ MCNameCreateWithCString("inputEndEditing", MCM_input_end_editing);
	/* UNCHECKED */ MCNameCreateWithCString("inputReturnKey", MCM_input_return_key);
	/* UNCHECKED */ MCNameCreateWithCString("inputTextChanged", MCM_input_text_changed);
    /* UNCHECKED */ MCNameCreateWithCString("productDetailsReceived", MCM_product_details_received);
    /* UNCHECKED */ MCNameCreateWithCString("productRequestError", MCM_product_request_error);
#endif
	
#ifdef _IOS_MOBILE
	/* UNCHECKED */ MCNameCreateWithCString("browserLoadRequest", MCM_browser_load_request);
	/* UNCHECKED */ MCNameCreateWithCString("browserLoadRequested", MCM_browser_load_requested);
	/* UNCHECKED */ MCNameCreateWithCString("scrollerBeginDecelerate", MCM_scroller_begin_decelerate);
	/* UNCHECKED */ MCNameCreateWithCString("scrollerEndDecelerate", MCM_scroller_end_decelerate);
	/* UNCHECKED */ MCNameCreateWithCString("scrollerScrollToTop", MCM_scroller_scroll_to_top);
	/* UNCHECKED */ MCNameCreateWithCString("playerProgressChanged", MCM_player_progress_changed);
	/* UNCHECKED */ MCNameCreateWithCString("playerEnterFullscreen", MCM_player_enter_fullscreen);
	/* UNCHECKED */ MCNameCreateWithCString("playerLeaveFullscreen", MCM_player_leave_fullscreen);
	/* UNCHECKED */ MCNameCreateWithCString("playerStateChanged", MCM_player_state_changed);
	/* UNCHECKED */ MCNameCreateWithCString("playerMovieChanged", MCM_player_movie_changed);
	/* UNCHECKED */ MCNameCreateWithCString("playerStopped", MCM_player_stopped);
	/* UNCHECKED */ MCNameCreateWithCString("reachabilityChanged", MCM_reachability_changed);
    /* UNCHECKED */ MCNameCreateWithCString("protectedDataDidBecomeAvailable", MCM_protected_data_available);
    /* UNCHECKED */ MCNameCreateWithCString("protectedDataWillBecomeUnavailable", MCM_protected_data_unavailable);
	
	// MW-2013-05-30: [[ RemoteControl ]] Message sent when a remote control event is received.
	/* UNCHECKED */ MCNameCreateWithCString("remoteControlReceived", MCM_remote_control_received);
#endif
    
    /* UNCHECKED */ MCNameCreateWithCString("(Default)", MCN_font_default);
    /* UNCHECKED */ MCNameCreateWithCString("(Styled Text)", MCN_font_usertext);
    /* UNCHECKED */ MCNameCreateWithCString("(Menu)", MCN_font_menutext);
    /* UNCHECKED */ MCNameCreateWithCString("(Text)", MCN_font_content);
    /* UNCHECKED */ MCNameCreateWithCString("(Message)", MCN_font_message);
    /* UNCHECKED */ MCNameCreateWithCString("(Tooltip)", MCN_font_tooltip);
    /* UNCHECKED */ MCNameCreateWithCString("(System)", MCN_font_system);
}

void MCU_finalize_names(void)
{
	MCNameDelete(MCN_msg);
	MCNameDelete(MCN_each);
	MCNameDelete(MCN_it);

	MCNameDelete(MCN_cancel);
	
	MCNameDelete(MCN_default_text_font);
    MCNameDelete(MCN_platform_string);
	MCNameDelete(MCN_version_string);

	MCNameDelete(MCN_metadata);
	MCNameDelete(MCN_runs);
	MCNameDelete(MCN_style);

	MCNameDelete(MCN_down);
	MCNameDelete(MCN_up);

	MCNameDelete(MCN_empty);
	MCNameDelete(MCN_files);
	MCNameDelete(MCN_image);
	MCNameDelete(MCN_objects);
	MCNameDelete(MCN_private);
	MCNameDelete(MCN_text);
//	MCNameDelete(MCN_unicode);
	MCNameDelete(MCN_styles);
    MCNameDelete(MCN_styledtext);
    MCNameDelete(MCN_rtftext);
    MCNameDelete(MCN_htmltext);
    MCNameDelete(MCN_png);
    MCNameDelete(MCN_gif);
    MCNameDelete(MCN_jpeg);
	MCNameDelete(MCN_win_bitmap);
	MCNameDelete(MCN_win_metafile);
	MCNameDelete(MCN_win_enh_metafile);
	MCNameDelete(MCN_rtf);
	MCNameDelete(MCN_html);

	MCNameDelete(MCN_browser);
	MCNameDelete(MCN_command_line);
	MCNameDelete(MCN_development);
    MCNameDelete(MCN_development_cmdline);
	MCNameDelete(MCN_helper_application);
	MCNameDelete(MCN_installer);
	MCNameDelete(MCN_mobile);
	MCNameDelete(MCN_player);
	MCNameDelete(MCN_server);
	MCNameDelete(MCN_standalone_application);
    MCNameDelete(MCN_embedded);
    MCNameDelete(MCN_mobile_embedded);
    
	MCNameDelete(MCN_all);
	MCNameDelete(MCN_auto_key);
	MCNameDelete(MCN_disk);
	MCNameDelete(MCN_activate);
	MCNameDelete(MCN_high_level);
	MCNameDelete(MCN_system);

	MCNameDelete(MCN_ansi);
	MCNameDelete(MCN_arabic);
	MCNameDelete(MCN_bulgarian);
	MCNameDelete(MCN_chinese);
	MCNameDelete(MCN_english);
	MCNameDelete(MCN_greek);
	MCNameDelete(MCN_hebrew);
	MCNameDelete(MCN_japanese);
	MCNameDelete(MCN_korean);
	MCNameDelete(MCN_lithuanian);
	MCNameDelete(MCN_polish);
	MCNameDelete(MCN_roman);
	MCNameDelete(MCN_russian);
	MCNameDelete(MCN_simple_chinese);
	MCNameDelete(MCN_thai);
	MCNameDelete(MCN_turkish);
	MCNameDelete(MCN_ukrainian);
	MCNameDelete(MCN_unicode);
	MCNameDelete(MCN_utf8);
	MCNameDelete(MCN_vietnamese);
	MCNameDelete(MCN_w_char);
	MCNameDelete(MCN_asterisk_char);

	MCNameDelete(MCN_plain);
	MCNameDelete(MCN_bold);
	MCNameDelete(MCN_italic);
	MCNameDelete(MCN_bold_italic);

	MCNameDelete(MCN_unknown);
	MCNameDelete(MCN_x86);
	MCNameDelete(MCN_x86_64);
    MCNameDelete(MCN_motorola_powerpc);
    MCNameDelete(MCN_i386);
    MCNameDelete(MCN_arm);
    // SN-2015-01-07: [[ iOS-64bit ]] ARM64 added
    MCNameDelete(MCN_arm64);

	MCNameDelete(MCN_local_mac);
	MCNameDelete(MCN_local_win32);
	MCNameDelete(MCN_android);
	MCNameDelete(MCN_iphone);
	MCNameDelete(MCN_wince);

	MCNameDelete(MCN_mac_os);
	MCNameDelete(MCN_win32);

	MCNameDelete(MCN_done);

	MCNameDelete(MCN_staticgray);
	MCNameDelete(MCN_grayscale);
	MCNameDelete(MCN_staticcolor);
	MCNameDelete(MCN_pseudocolor);
	MCNameDelete(MCN_truecolor);
	MCNameDelete(MCN_directcolor);

	MCNameDelete(MCN_bounds);
	MCNameDelete(MCN_pixels);
	MCNameDelete(MCN_opaque_pixels);

	MCNameDelete(MCN_desktop);
	MCNameDelete(MCN_documents);
	MCNameDelete(MCN_engine);
    MCNameDelete(MCN_resources);
	MCNameDelete(MCN_fonts);
	MCNameDelete(MCN_home);
	MCNameDelete(MCN_start);
//	MCNameDelete(MCN_system);
	MCNameDelete(MCN_temporary);

	MCNameDelete(MCN_apple);
	MCNameDelete(MCN_control);
	MCNameDelete(MCN_extension);
	MCNameDelete(MCN_preferences);
	
	MCNameDelete(MCN_unhandled);
	MCNameDelete(MCN_handled);
	MCNameDelete(MCN_passed);

	MCNameDelete(MCN_page_setup_dialog);
	MCNameDelete(MCN_pagesetup);
	MCNameDelete(MCN_print_dialog);
	MCNameDelete(MCN_printer);
	MCNameDelete(MCN_color_chooser);
	MCNameDelete(MCN_color);
	MCNameDelete(MCN_file_selector);
	MCNameDelete(MCN_file);
	//MCNameDelete(MCN_files);
	MCNameDelete(MCN_folder);
	MCNameDelete(MCN_folders);
	MCNameDelete(MCN_answer_dialog);
	MCNameDelete(MCN_ask_dialog);

	//MCNameDelete(MCN_plain);
	MCNameDelete(MCN_clear);
	//MCNameDelete(MCN_color);
	MCNameDelete(MCN_effect);
	MCNameDelete(MCN_error);
	//MCNameDelete(MCN_file);
	//MCNameDelete(MCN_folder);
	MCNameDelete(MCN_information);
	MCNameDelete(MCN_password);
	//MCNameDelete(MCN_printer);
	MCNameDelete(MCN_program);
	MCNameDelete(MCN_question);
	MCNameDelete(MCN_record);
	MCNameDelete(MCN_titled);
	MCNameDelete(MCN_warning);
	
	MCNameDelete(MCN_messagename);
    MCNameDelete(MCN_hcstat);
    
	MCNameDelete(MCM_apple_event);
	MCNameDelete(MCM_arrow_key);
	MCNameDelete(MCM_assert_error);
	MCNameDelete(MCM_backspace_key);
	MCNameDelete(MCM_close_background);
	MCNameDelete(MCM_close_card);
	MCNameDelete(MCM_close_control);
	MCNameDelete(MCM_close_field);
	MCNameDelete(MCM_close_stack);
	MCNameDelete(MCM_close_stack_request);
	MCNameDelete(MCM_color_changed);
	MCNameDelete(MCM_command_key_down);
	MCNameDelete(MCM_control_key_down);
	MCNameDelete(MCM_copy_key);
	MCNameDelete(MCM_current_time_changed);
	MCNameDelete(MCM_cut_key);
	MCNameDelete(MCM_debug_str);
	MCNameDelete(MCM_delete_background);
	MCNameDelete(MCM_delete_button);
	MCNameDelete(MCM_delete_card);
	MCNameDelete(MCM_delete_eps);
	MCNameDelete(MCM_delete_field);
	MCNameDelete(MCM_delete_graphic);
	MCNameDelete(MCM_delete_group);
	MCNameDelete(MCM_delete_image);
	MCNameDelete(MCM_delete_key);
	MCNameDelete(MCM_delete_scrollbar);
	MCNameDelete(MCM_delete_player);
	MCNameDelete(MCM_delete_stack);
	MCNameDelete(MCM_delete_widget);
	MCNameDelete(MCM_delete_url);
	MCNameDelete(MCM_desktop_changed);
	MCNameDelete(MCM_drag_drop);
	MCNameDelete(MCM_drag_end);
	MCNameDelete(MCM_drag_enter);
	MCNameDelete(MCM_drag_leave);
	MCNameDelete(MCM_drag_move);
	MCNameDelete(MCM_drag_start);
	MCNameDelete(MCM_edit_script);
	MCNameDelete(MCM_enter_in_field);
	MCNameDelete(MCM_enter_key);
	MCNameDelete(MCM_error_dialog);
	MCNameDelete(MCM_escape_key);
	MCNameDelete(MCM_eval);
	MCNameDelete(MCM_exit_field);
	MCNameDelete(MCM_focus_in);
	MCNameDelete(MCM_focus_out);
	MCNameDelete(MCM_function_key);
	MCNameDelete(MCM_get_cached_urls);
	MCNameDelete(MCM_get_url);
	MCNameDelete(MCM_get_url_status);
	MCNameDelete(MCM_gradient_edit_ended);
	MCNameDelete(MCM_gradient_edit_started);
	MCNameDelete(MCM_help);
	MCNameDelete(MCM_hot_spot_clicked);
	MCNameDelete(MCM_icon_menu_pick);
	MCNameDelete(MCM_icon_menu_opening);
	MCNameDelete(MCM_status_icon_menu_pick);
	MCNameDelete(MCM_status_icon_menu_opening);
	MCNameDelete(MCM_status_icon_click);
	MCNameDelete(MCM_status_icon_double_click);
	MCNameDelete(MCM_iconify_stack);
	MCNameDelete(MCM_id_changed);
	MCNameDelete(MCM_idle);
	MCNameDelete(MCM_internal);
	MCNameDelete(MCM_internal2);
	MCNameDelete(MCM_key_down);
	MCNameDelete(MCM_key_up);
	MCNameDelete(MCM_keyboard_activated);
	MCNameDelete(MCM_keyboard_deactivated);
	MCNameDelete(MCM_library_stack);
	MCNameDelete(MCM_link_clicked);
	MCNameDelete(MCM_load_url);
	MCNameDelete(MCM_main_stack_changed);
	MCNameDelete(MCM_menu_pick);
	MCNameDelete(MCM_message);
	MCNameDelete(MCM_message_handled);
	MCNameDelete(MCM_message_not_handled);
	MCNameDelete(MCM_mouse_double_down);
	MCNameDelete(MCM_mouse_double_up);
	MCNameDelete(MCM_mouse_down);
	MCNameDelete(MCM_mouse_down_in_backdrop);
	MCNameDelete(MCM_mouse_enter);
	MCNameDelete(MCM_mouse_leave);
	MCNameDelete(MCM_mouse_move);
	MCNameDelete(MCM_mouse_release);
	MCNameDelete(MCM_mouse_still_down);
	MCNameDelete(MCM_mouse_up);
	MCNameDelete(MCM_mouse_up_in_backdrop);
	MCNameDelete(MCM_mouse_within);
	MCNameDelete(MCM_move_control);
	MCNameDelete(MCM_move_stack);
	MCNameDelete(MCM_move_stopped);
	MCNameDelete(MCM_movie_touched);
	MCNameDelete(MCM_name_changed);
	MCNameDelete(MCM_new_background);
	MCNameDelete(MCM_new_card);
	MCNameDelete(MCM_new_stack);
	MCNameDelete(MCM_new_tool);
	MCNameDelete(MCM_node_changed);
	MCNameDelete(MCM_object_selection_ended);
	MCNameDelete(MCM_object_selection_started);
	MCNameDelete(MCM_open_background);
	MCNameDelete(MCM_open_card);
	MCNameDelete(MCM_open_control);
	MCNameDelete(MCM_open_field);
	MCNameDelete(MCM_open_stack);
	MCNameDelete(MCM_option_key_down);
	MCNameDelete(MCM_paste_key);
	MCNameDelete(MCM_play_paused);
    MCNameDelete(MCM_play_rate_changed);
	MCNameDelete(MCM_play_started);
	MCNameDelete(MCM_play_stopped);
	MCNameDelete(MCM_post_url);
	MCNameDelete(MCM_preopen_background);
	MCNameDelete(MCM_preopen_card);
	MCNameDelete(MCM_preopen_control);
	MCNameDelete(MCM_preopen_stack);
	MCNameDelete(MCM_property_changed);
	MCNameDelete(MCM_put_url);
	MCNameDelete(MCM_qtdebugstr);
	MCNameDelete(MCM_raw_key_down);
	MCNameDelete(MCM_raw_key_up);
	MCNameDelete(MCM_relaunch);
	MCNameDelete(MCM_release_stack);
	MCNameDelete(MCM_reload_stack);
	MCNameDelete(MCM_resize_control);
	MCNameDelete(MCM_resize_control_ended);
	MCNameDelete(MCM_resize_control_started);
	MCNameDelete(MCM_resize_stack);
	MCNameDelete(MCM_resolution_error);
	MCNameDelete(MCM_resume);
	MCNameDelete(MCM_resume_stack);
	MCNameDelete(MCM_return_in_field);
	MCNameDelete(MCM_return_key);
	MCNameDelete(MCM_save_stack_request);
	MCNameDelete(MCM_script_error);
	MCNameDelete(MCM_script_execution_error);
	MCNameDelete(MCM_scrollbar_beginning);
	MCNameDelete(MCM_scrollbar_drag);
	MCNameDelete(MCM_scrollbar_end);
	MCNameDelete(MCM_scrollbar_line_dec);
	MCNameDelete(MCM_scrollbar_line_inc);
	MCNameDelete(MCM_scrollbar_page_dec);
	MCNameDelete(MCM_scrollbar_page_inc);
	MCNameDelete(MCM_selected_object_changed);
	MCNameDelete(MCM_selection_changed);
	MCNameDelete(MCM_shell);
	MCNameDelete(MCM_signal);
	MCNameDelete(MCM_shut_down);
	MCNameDelete(MCM_shut_down_request);
	MCNameDelete(MCM_socket_error);
	MCNameDelete(MCM_socket_closed);
	MCNameDelete(MCM_socket_timeout);
	MCNameDelete(MCM_start_up);
	MCNameDelete(MCM_suspend);
	MCNameDelete(MCM_suspend_stack);
	MCNameDelete(MCM_tab_key);
	MCNameDelete(MCM_text_changed);
	MCNameDelete(MCM_trace);
	MCNameDelete(MCM_trace_break);
	MCNameDelete(MCM_trace_done);
	MCNameDelete(MCM_trace_error);
	MCNameDelete(MCM_undo_changed);
	MCNameDelete(MCM_undo_key);
	MCNameDelete(MCM_uniconify_stack);
	MCNameDelete(MCM_unload_url);
	MCNameDelete(MCM_update_screen);
	MCNameDelete(MCM_update_var);

#ifdef FEATURE_PLATFORM_URL
	MCNameDelete(MCM_url_progress);
#endif

    MCNameDelete(MCM_delete_audioclip);
    MCNameDelete(MCM_delete_videoclip);
    MCNameDelete(MCM_new_audioclip);
    MCNameDelete(MCM_new_videoclip);
    
#ifdef _MOBILE
	MCNameDelete(MCN_firstname);
	MCNameDelete(MCN_lastname);
	MCNameDelete(MCN_middlename);
	MCNameDelete(MCN_prefix);
	MCNameDelete(MCN_suffix);
	MCNameDelete(MCN_nickname);
	MCNameDelete(MCN_firstnamephonetic);
	MCNameDelete(MCN_lastnamephonetic);
	MCNameDelete(MCN_middlenamephonetic);
	MCNameDelete(MCN_organization);
	MCNameDelete(MCN_jobtitle);
	MCNameDelete(MCN_department);
	MCNameDelete(MCN_note);

	MCNameDelete(MCN_email);
	MCNameDelete(MCN_phone);
	MCNameDelete(MCN_address);
	
//	MCNameDelete(MCN_home);
	MCNameDelete(MCN_work);
	MCNameDelete(MCN_other);
//	MCNameDelete(MCN_mobile);
//	MCNameDelete(MCN_iphone);
	MCNameDelete(MCN_main);
	MCNameDelete(MCN_homefax);
	MCNameDelete(MCN_workfax);
	MCNameDelete(MCN_otherfax);
	MCNameDelete(MCN_pager);
	
	MCNameDelete(MCN_street);
	MCNameDelete(MCN_city);
	MCNameDelete(MCN_state);
	MCNameDelete(MCN_zip);
	MCNameDelete(MCN_country);
	MCNameDelete(MCN_countrycode);
	
	MCNameDelete(MCM_touch_start);
	MCNameDelete(MCM_touch_move);
	MCNameDelete(MCM_touch_end);
	MCNameDelete(MCM_touch_release);
	MCNameDelete(MCM_motion_start);
	MCNameDelete(MCM_motion_end);
	MCNameDelete(MCM_motion_release);
	MCNameDelete(MCM_acceleration_changed);
	MCNameDelete(MCM_orientation_changed);
	MCNameDelete(MCM_location_changed);
	MCNameDelete(MCM_location_error);
	MCNameDelete(MCM_heading_changed);
	MCNameDelete(MCM_heading_error);
	MCNameDelete(MCM_purchase_updated);
    MCNameDelete(MCM_rotation_rate_changed);
    MCNameDelete(MCM_tracking_error);
    MCNameDelete(MCM_local_notification_received);
    MCNameDelete(MCM_push_notification_received);
    MCNameDelete(MCM_push_notification_registered);
    MCNameDelete(MCM_push_notification_registration_error);
    MCNameDelete(MCM_url_wake_up);
	MCNameDelete(MCM_launch_data_changed);
	MCNameDelete(MCM_browser_started_loading);
	MCNameDelete(MCM_browser_finished_loading);
	MCNameDelete(MCM_browser_load_failed);
    MCNameDelete(MCM_sound_finished_on_channel);
    MCNameDelete(MCM_ad_loaded);
    MCNameDelete(MCM_ad_clicked);
    MCNameDelete(MCM_ad_resize_start);
    MCNameDelete(MCM_ad_resize_end);
    MCNameDelete(MCM_ad_expand_start);
    MCNameDelete(MCM_ad_expand_end);
	MCNameDelete(MCM_scroller_did_scroll);
	MCNameDelete(MCM_scroller_begin_drag);
	MCNameDelete(MCM_scroller_end_drag);
	MCNameDelete(MCM_player_finished);
	MCNameDelete(MCM_player_error);
	MCNameDelete(MCM_player_property_available);
	MCNameDelete(MCM_input_begin_editing);
	MCNameDelete(MCM_input_end_editing);
	MCNameDelete(MCM_input_return_key);
	MCNameDelete(MCM_input_text_changed);
#endif
	
#ifdef _IOS_MOBILE
	MCNameDelete(MCM_browser_load_request);
	MCNameDelete(MCM_browser_load_requested);
	MCNameDelete(MCM_scroller_begin_decelerate);
	MCNameDelete(MCM_scroller_end_decelerate);
	MCNameDelete(MCM_scroller_scroll_to_top);
	MCNameDelete(MCM_player_progress_changed);
	MCNameDelete(MCM_player_enter_fullscreen);
	MCNameDelete(MCM_player_leave_fullscreen);
	MCNameDelete(MCM_player_state_changed);
	MCNameDelete(MCM_player_movie_changed);
	MCNameDelete(MCM_player_stopped);
	MCNameDelete(MCM_reachability_changed);
#endif
    
    MCNameDelete(MCN_font_default);
    MCNameDelete(MCN_font_usertext);
    MCNameDelete(MCN_font_menutext);
    MCNameDelete(MCN_font_content);
    MCNameDelete(MCN_font_message);
    MCNameDelete(MCN_font_tooltip);
    MCNameDelete(MCN_font_system);
}
