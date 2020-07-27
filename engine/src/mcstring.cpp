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


const char * const MCtoolnames[] =
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

const char * const MCcopystring = "Copy of ";
const char * const MCstandardstring = "standard";
const char * const MCdialogstring = "dialog";
const char * const MCmovablestring = "movable";
const char * const MCpalettestring = "palette";
const char * const MCmodalstring = "modal";
const char * const MCsheetstring = "sheet";
const char * const MCdrawerstring = "drawer";
const char * const MCmodelessstring = "modeless";
const char * const MCtoplevelstring = "toplevel";
const char * const MCtransparentstring = "transparent";
const char * const MCopaquestring = "opaque";
const char * const MCrectanglestring = "rectangle";
const char * const MCshadowstring = "shadow";
const char * const MCscrollingstring = "scrolling";
const char * const MCroundrectstring = "roundrect";
const char * const MCcheckboxstring = "checkbox";
const char * const MCradiobuttonstring = "radiobutton";
const char * const MClinestring = "line";
const char * const MCpolygonstring = "polygon";
const char * const MCarcstring = "arc";
const char * const MCovalstring = "oval";
const char * const MCregularstring = "regular";
const char * const MCcurvestring = "curve";
const char * const MCtextstring = "text";
const char * const MCleftstring = "left";
const char * const MCcenterstring = "center";
const char * const MCrightstring = "right";
const char * const MCjustifystring = "justify";
const char * const MCplainstring = "plain";
const char * const MCmixedstring = "mixed";
const char * const MCboxstring = "box";
const char * const MCthreedboxstring = "threedbox";
const char * const MCunderlinestring = "underline";
const char * const MCstrikeoutstring = "strikeout";
const char * const MCgroupstring = "group";
const char * const MClinkstring = "link";
const char * const MCtruestring = "true";
const char * const MCfalsestring = "false";
const char * const MCdownstring = "down";
const char * const MCupstring = "up";
const char * const MCshiftstring = "shift";
const char * const MCcommandstring = "command";
const char * const MCcontrolstring = "control";
const char * const MCmod1string = "alt";
const char * const MCpulldownstring = "pulldown";
const char * const MCpopupstring = "popup";
const char * const MCoptionstring = "option";
const char * const MCcascadestring = "cascade";
const char * const MCcombostring = "combobox";
const char * const MCtabstring = "tabbed";
const char * const MCstackstring = "stack";
const char * const MCaudiostring = "audioclip";
const char * const MCvideostring = "videoclip";
const char * const MCdefaultstring = "default";
const char * const MCtitlestring = "title";
const char * const MCmenustring = "menu";
const char * const MCminimizestring = "minimize";
const char * const MCmaximizestring = "maximize";
const char * const MCclosestring = "close";
const char * const MCmetalstring = "metal";
const char * const MCutilitystring = "system";
const char * const MCnoshadowstring = "noshadow";
const char * const MCbackgroundstring = "background";
const char * const MCforcetaskbarstring = "forcetaskbar";
const char * const MCunicodestring = "unicode";
const char * const MCnativestring = "native";

const char * const MCcardstring = "card";
const char * const MCbuttonstring = "button";
const char * const MCgraphicstring = "graphic";
const char * const MCepsstring = "EPS";
const char * const MCscrollbarstring = "scrollbar";
const char * const MCplayerstring = "player";
const char * const MCscalestring = "scale";
const char * const MCprogressstring = "progress";
const char * const MCimagestring = "image";
const char * const MCfieldstring = "field";
const char * const MCcolorstring = "colorPalette";
const char * const MCmagnifierstring = "magnifier";
const char * const MCwidgetstring = "widget";

const char * const MCnotfoundstring = "not found";
const char * const MCplatformstring = PLATFORM_STRING;
const char * const MClnfamstring = "Appearance Manager";
const char * const MClnfmacstring = "Macintosh";
const char * const MClnfmotifstring = "Motif";
const char * const MClnfwinstring = "Windows 95";
const char * const MCuntitledstring = "Untitled";
// MW-2012-08-29: [[ Bug 10309 ]] Update 'applicationstring' to be 'LiveCode'.
const char * const MCapplicationstring = "livecode";
const char * const MCanswernamestring = "Answer Dialog";
const char * const MCasknamestring = "Ask Dialog";
const char * const MCfsnamestring = "File Selector";
const char * const MCcsnamestring = "Color Chooser";
const char * const MChelpnamestring = "Help";
const char * const MChomenamestring = "Home";
const char * const MChcstatnamestring = "HyperCard Import Status";
const char * const MCmessagenamestring = "Message Box";
const char * const MCdonestring = "done";
const char * const MCnullstring = "";
const char * const MCintersectstring = "intersect";
const char * const MCsurroundstring = "surround";
const char * const MCtopstring = "top";
const char * const MCbottomstring = "bottom";
const char * const MCcancelstring = "Cancel";

const char * const MCtextalignstrings[] = { MCleftstring, MCcenterstring, MCrightstring, MCjustifystring };

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
MCNameRef MCN_installer_cmdline;
MCNameRef MCN_mobile;
MCNameRef MCN_player;
MCNameRef MCN_server;
MCNameRef MCN_standalone_application;

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
MCNameRef MCN_folder;
MCNameRef MCN_folders;
MCNameRef MCN_answer_dialog;
MCNameRef MCN_ask_dialog;

MCNameRef MCN_clear;
MCNameRef MCN_effect;
MCNameRef MCN_error;
MCNameRef MCN_information;
MCNameRef MCN_password;
MCNameRef MCN_program;
MCNameRef MCN_question;
MCNameRef MCN_record;
MCNameRef MCN_titled;
MCNameRef MCN_warning;

MCNameRef MCN_messagename;
MCNameRef MCM_msgchanged;
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
MCNameRef MCM_new_audioclip;
MCNameRef MCM_new_background;
MCNameRef MCM_new_card;
MCNameRef MCM_new_stack;
MCNameRef MCM_new_tool;
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

MCNameRef MCN_work;
MCNameRef MCN_other;
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
MCNameRef MCM_nfc_tag_received;
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
MCNameRef MCM_protected_data_available;
MCNameRef MCM_protected_data_unavailable;
MCNameRef MCM_remote_control_received;
#endif

MCNameRef MCN_font_default;
MCNameRef MCN_font_usertext;
MCNameRef MCN_font_menutext;
MCNameRef MCN_font_content;
MCNameRef MCN_font_message;
MCNameRef MCN_font_tooltip;
MCNameRef MCN_font_system;

MCNameRef MCM_system_appearance_changed;

const struct { const char *cstring; MCNameRef *name_var; } kInitialNames[] =
{
	{ "msg", &MCN_msg },
	{ "each", &MCN_each },
	{ "it", &MCN_it },
	
	{ "Cancel", &MCN_cancel },

	{ DEFAULT_TEXT_FONT, &MCN_default_text_font },
	{ PLATFORM_STRING, &MCN_platform_string },
	{ MC_BUILD_ENGINE_SHORT_VERSION, &MCN_version_string },

	{ "metadata", &MCN_metadata },
	{ "runs", &MCN_runs },
	{ "style", &MCN_style },

	{ "down", &MCN_down },
	{ "up", &MCN_up },

	{ "empty", &MCN_empty },
	{ "files", &MCN_files },
	{ "image", &MCN_image },
	{ "objects", &MCN_objects },
	{ "private", &MCN_private },
	{ "text", &MCN_text },
	{ "styles", &MCN_styles },
    { "styledtext", &MCN_styledtext },
    { "rtftext", &MCN_rtftext },
    { "htmltext", &MCN_htmltext },
    { "png", &MCN_png },
    { "gif", &MCN_gif },
    { "jpeg", &MCN_jpeg },
	{ "windows bitmap", &MCN_win_bitmap },
	{ "windows metafile", &MCN_win_metafile },
	{ "windows enhanced metafile", &MCN_win_enh_metafile },
	{ "rtf", &MCN_rtf },
	{ "html", &MCN_html },

	{ "browser", &MCN_browser },
	{ "command line", &MCN_command_line },
	{ "development", &MCN_development },
    { "development command line", &MCN_development_cmdline },
	{ "helper application", &MCN_helper_application },
	{ "installer", &MCN_installer },
    { "installer command line", &MCN_installer_cmdline },
	{ "mobile", &MCN_mobile },
	{ "player", &MCN_player },
	{ "server", &MCN_server },
	{ "standalone application", &MCN_standalone_application },

	{ "all", &MCN_all },
	{ "autokey", &MCN_auto_key },
	{ "disk", &MCN_disk },
	{ "activate", &MCN_activate },
	{ "highlevel", &MCN_high_level },
	{ "system", &MCN_system },

	{ "ansi", &MCN_ansi },
	{ "arabic", &MCN_arabic },
	{ "bulgarian", &MCN_bulgarian },
	{ "chinese", &MCN_chinese },
	{ "english", &MCN_english },
	{ "greek", &MCN_greek },
	{ "hebrew", &MCN_hebrew },
	{ "japanese", &MCN_japanese },
	{ "korean", &MCN_korean },
	{ "lithuanian", &MCN_lithuanian },
	{ "polish", &MCN_polish },
	{ "roman", &MCN_roman },
	{ "russian", &MCN_russian },
	{ "simpleChinese", &MCN_simple_chinese },
	{ "thai", &MCN_thai },
	{ "turkish", &MCN_turkish },
	{ "ukrainian", &MCN_ukrainian },
	{ "unicode", &MCN_unicode },
	{ "utf8", &MCN_utf8 },
	{ "vietnamese", &MCN_vietnamese },
	{ "w", &MCN_w_char },
	{ "*", &MCN_asterisk_char },

	{ "plain", &MCN_plain },
	{ "bold", &MCN_bold },
	{ "italic", &MCN_italic },
	{ "bold-italic", &MCN_bold_italic },

	{ "unknown", &MCN_unknown },

	{ "local Mac", &MCN_local_mac },
	{ "local Win32", &MCN_local_win32 },
	{ "android", &MCN_android },
	{ "iphone", &MCN_iphone },
	{ "wince", &MCN_wince },

	{ "Mac OS", &MCN_mac_os },
	{ "Win32", &MCN_win32 },

	{ "done", &MCN_done },

	{ "StaticGray", &MCN_staticgray },
	{ "GrayScale", &MCN_grayscale },
	{ "StaticColor", &MCN_staticcolor },
	{ "PseudoColor", &MCN_pseudocolor },
	{ "TrueColor", &MCN_truecolor },
	{ "DirectColor", &MCN_directcolor },

	{ "bounds", &MCN_bounds },
	{ "pixels", &MCN_pixels },
	{ "opaque pixels", &MCN_opaque_pixels },

	{ "desktop", &MCN_desktop },
	{ "documents", &MCN_documents },
	{ "engine", &MCN_engine },
    { "resources", &MCN_resources },
	{ "fonts", &MCN_fonts },
	{ "home", &MCN_home },
	{ "start", &MCN_start },
	{ "temporary", &MCN_temporary },
	{ "support", &MCN_support },

	{ "Apple", &MCN_apple },
	{ "Control", &MCN_control },
	{ "Extension", &MCN_extension },
	{ "Preferences", &MCN_preferences },
	
	{ "unhandled", &MCN_unhandled },
	{ "handled", &MCN_handled },
	{ "passed", &MCN_passed },

	{ "Page Setup Dialog", &MCN_page_setup_dialog },
	{ "pagesetup", &MCN_pagesetup },
	{ "Print Dialog", &MCN_print_dialog },
	{ "printer", &MCN_printer },
	{ "Color Chooser", &MCN_color_chooser },
	{ "color", &MCN_color },
	{ "File Selector", &MCN_file_selector },
	{ "file", &MCN_file },
	{ "folder", &MCN_folder },
	{ "folders", &MCN_folders },
	{ "Answer Dialog", &MCN_answer_dialog },
	{ "Ask Dialog", &MCN_ask_dialog },

	{ "clear", &MCN_clear },
	{ "effect", &MCN_effect },
	{ "error", &MCN_error },
	{ "information", &MCN_information },
	{ "password", &MCN_password },
	{ "program", &MCN_program },
	{ "question", &MCN_question },
	{ "record", &MCN_record },
	{ "titled", &MCN_titled },
	{ "warning", &MCN_warning },

	{ "Message Box", &MCN_messagename },
	{ "msgchanged", &MCM_msgchanged },
	{ "HyperCard Import Status", &MCN_hcstat },
	{ "appleEvent", &MCM_apple_event },
	{ "arrowKey", &MCM_arrow_key },
	{ "assertError", &MCM_assert_error },
	{ "backspaceKey", &MCM_backspace_key },
	{ "closeBackground", &MCM_close_background },
	{ "closeCard", &MCM_close_card },
	{ "closeControl", &MCM_close_control },
	{ "closeField", &MCM_close_field },
	{ "closeStack", &MCM_close_stack },
	{ "closeStackRequest", &MCM_close_stack_request },
	{ "colorChanged", &MCM_color_changed },
	{ "commandKeyDown", &MCM_command_key_down },
	{ "controlKeyDown", &MCM_control_key_down },
	{ "copyKey", &MCM_copy_key },
	{ "currentTimeChanged", &MCM_current_time_changed },
	{ "cutKey", &MCM_cut_key },
	{ "debugStr", &MCM_debug_str },
	{ "deleteBackground", &MCM_delete_background },
	{ "deleteButton", &MCM_delete_button },
	{ "deleteCard", &MCM_delete_card },
	{ "deleteEPS", &MCM_delete_eps },
	{ "deleteField", &MCM_delete_field },
	{ "deleteGraphic", &MCM_delete_graphic },
	{ "deleteGroup", &MCM_delete_group },
	{ "deleteImage", &MCM_delete_image },
	{ "deleteKey", &MCM_delete_key },
	{ "deleteScrollbar", &MCM_delete_scrollbar },
	{ "deletePlayer", &MCM_delete_player },
	{ "deleteStack", &MCM_delete_stack },
	{ "deleteWidget", &MCM_delete_widget },
	{ "deleteURL", &MCM_delete_url },
	{ "desktopChanged", &MCM_desktop_changed },
	{ "dragDrop", &MCM_drag_drop },
	{ "dragEnd", &MCM_drag_end },
	{ "dragEnter", &MCM_drag_enter },
	{ "dragLeave", &MCM_drag_leave },
	{ "dragMove", &MCM_drag_move },
	{ "dragStart", &MCM_drag_start },
	{ "editScript", &MCM_edit_script },
	{ "enterInField", &MCM_enter_in_field },
	{ "enterKey", &MCM_enter_key },
	{ "errorDialog", &MCM_error_dialog },
	{ "escapeKey", &MCM_escape_key },
	{ "eval", &MCM_eval },
	{ "exitField", &MCM_exit_field },
	{ "focusIn", &MCM_focus_in },
	{ "focusOut", &MCM_focus_out },
	{ "functionKey", &MCM_function_key },
	{ "getCachedURLs", &MCM_get_cached_urls },
	{ "getURL", &MCM_get_url },
	{ "getURLStatus", &MCM_get_url_status },
	{ "gradientEditEnded", &MCM_gradient_edit_ended },
	{ "gradientEditStarted", &MCM_gradient_edit_started },
	{ "help", &MCM_help },
	{ "hotSpotClicked", &MCM_hot_spot_clicked },
	{ "iconMenuPick", &MCM_icon_menu_pick },
	{ "iconMenuOpening", &MCM_icon_menu_opening },
	{ "statusIconMenuPick", &MCM_status_icon_menu_pick },
	{ "statusIconMenuOpening", &MCM_status_icon_menu_opening },
	{ "statusIconClick", &MCM_status_icon_click },
	{ "statusIconDoubleClick", &MCM_status_icon_double_click },
	{ "iconifyStack", &MCM_iconify_stack },
	{ "idChanged", &MCM_id_changed },
	{ "idle", &MCM_idle },
	{ "internal", &MCM_internal },
	{ "internal2", &MCM_internal2 },
	{ "internal3", &MCM_internal3 },
	{ "keyDown", &MCM_key_down },
	{ "keyUp", &MCM_key_up },
	{ "keyboardActivated", &MCM_keyboard_activated },
	{ "keyboardDeactivated", &MCM_keyboard_deactivated },
	{ "libraryStack", &MCM_library_stack },
	{ "linkClicked", &MCM_link_clicked },
	{ "loadURL", &MCM_load_url },
	{ "mainStackChanged", &MCM_main_stack_changed },
	{ "_mainStacksChanged", &MCM_main_stacks_changed },
	{ "menuPick", &MCM_menu_pick },
	{ "message", &MCM_message },
	{ "messageHandled", &MCM_message_handled },
	{ "messageNotHandled", &MCM_message_not_handled },
	{ "mouseDoubleDown", &MCM_mouse_double_down },
	{ "mouseDoubleUp", &MCM_mouse_double_up },
	{ "mouseDown", &MCM_mouse_down },
	{ "mouseDownInBackdrop", &MCM_mouse_down_in_backdrop },
	{ "mouseEnter", &MCM_mouse_enter },
	{ "mouseLeave", &MCM_mouse_leave },
	{ "mouseMove", &MCM_mouse_move },
	{ "mouseRelease", &MCM_mouse_release },
	{ "mouseStillDown", &MCM_mouse_still_down },
	{ "mouseUp", &MCM_mouse_up },
	{ "mouseUpInBackdrop", &MCM_mouse_up_in_backdrop },
	{ "mouseWithin", &MCM_mouse_within },
	{ "moveControl", &MCM_move_control },
	{ "moveStack", &MCM_move_stack },
	{ "moveStopped", &MCM_move_stopped },
	{ "movieTouched", &MCM_movie_touched },
	{ "nameChanged", &MCM_name_changed },
	{ "newBackground", &MCM_new_background },
	{ "newCard", &MCM_new_card },
	{ "newStack", &MCM_new_stack },
	{ "newTool", &MCM_new_tool },
	{ "nodeChanged", &MCM_node_changed },
	{ "objectSelectionEnded", &MCM_object_selection_ended },
	{ "objectSelectionStarted", &MCM_object_selection_started },
	{ "openBackground", &MCM_open_background },
	{ "openCard", &MCM_open_card },
	{ "openControl", &MCM_open_control },
	{ "openField", &MCM_open_field },
	{ "openStack", &MCM_open_stack },
	{ "optionKeyDown", &MCM_option_key_down },
	{ "pasteKey", &MCM_paste_key },
	{ "playPaused", &MCM_play_paused },
    { "playRateChanged", &MCM_play_rate_changed },
	{ "playStarted", &MCM_play_started },
	{ "playStopped", &MCM_play_stopped },
	{ "postURL", &MCM_post_url },
	{ "preOpenBackground", &MCM_preopen_background },
	{ "preOpenCard", &MCM_preopen_card },
	{ "preOpenControl", &MCM_preopen_control },
	{ "preOpenStack", &MCM_preopen_stack },
	{ "propertyChanged", &MCM_property_changed },
	{ "putURL", &MCM_put_url },
	{ "QTDebugStr", &MCM_qtdebugstr },
	{ "rawKeyDown", &MCM_raw_key_down },
	{ "rawKeyUp", &MCM_raw_key_up },
	{ "relaunch", &MCM_relaunch },
	{ "releaseStack", &MCM_release_stack },
	{ "reloadStack", &MCM_reload_stack },
	{ "resizeControl", &MCM_resize_control },
	{ "resizeControlEnded", &MCM_resize_control_ended },
	{ "resizeControlStarted", &MCM_resize_control_started },
	{ "resizeStack", &MCM_resize_stack },
	{ "resolutionError", &MCM_resolution_error },
	{ "resume", &MCM_resume },
	{ "resumeStack", &MCM_resume_stack },
	{ "returnInField", &MCM_return_in_field },
	{ "returnKey", &MCM_return_key },
	{ "saveStackRequest", &MCM_save_stack_request },
	{ "scriptParsingError", &MCM_script_error },
	{ "scriptExecutionError", &MCM_script_execution_error },
	{ "scrollbarBeginning", &MCM_scrollbar_beginning },
	{ "scrollbarDrag", &MCM_scrollbar_drag },
	{ "scrollbarEnd", &MCM_scrollbar_end },
	{ "scrollbarLineDec", &MCM_scrollbar_line_dec },
	{ "scrollbarLineInc", &MCM_scrollbar_line_inc },
	{ "scrollbarPageDec", &MCM_scrollbar_page_dec },
	{ "scrollbarPageInc", &MCM_scrollbar_page_inc },
	{ "selectedObjectChanged", &MCM_selected_object_changed },
	{ "selectionChanged", &MCM_selection_changed },
	{ "shell", &MCM_shell },
	{ "signal", &MCM_signal },
	{ "shutDown", &MCM_shut_down },
	{ "shutDownRequest", &MCM_shut_down_request },
	{ "socketError", &MCM_socket_error },
	{ "socketClosed", &MCM_socket_closed },
	{ "socketTimeout", &MCM_socket_timeout },
	{ "startUp", &MCM_start_up },
	{ "suspend", &MCM_suspend },
	{ "suspendStack", &MCM_suspend_stack },
	{ "tabKey", &MCM_tab_key },
	{ "textChanged", &MCM_text_changed },
	{ "trace", &MCM_trace },
	{ "traceBreak", &MCM_trace_break },
	{ "traceDone", &MCM_trace_done },
	{ "traceError", &MCM_trace_error },
	{ "undoChanged", &MCM_undo_changed },
	{ "undoKey", &MCM_undo_key },
	{ "uniconifyStack", &MCM_uniconify_stack },
	{ "unloadURL", &MCM_unload_url },
	{ "updateScreen", &MCM_update_screen },
	{ "updateVariable", &MCM_update_var },
	{ "systemAppearanceChanged", &MCM_system_appearance_changed },

#ifdef FEATURE_PLATFORM_URL
	{ "urlProgress", &MCM_url_progress },
#endif
    
    { "deleteAudioclip", &MCM_delete_audioclip },
    { "deleteVideoclip", &MCM_delete_videoclip },
    { "newAudioclip", &MCM_new_audioclip },
    { "newVideoclip", &MCM_new_videoclip },
    
#ifdef _MOBILE
	{ "firstname", &MCN_firstname },
	{ "lastname", &MCN_lastname },
	{ "middlename", &MCN_middlename },
	{ "prefix", &MCN_prefix },
	{ "suffix", &MCN_suffix },
	{ "nickname", &MCN_nickname },
	{ "firstnamephonetic", &MCN_firstnamephonetic },
	{ "lastnamephonetic", &MCN_lastnamephonetic },
	{ "middlenamephonetic", &MCN_middlenamephonetic },
	{ "organization", &MCN_organization },
	{ "jobtitle", &MCN_jobtitle },
	{ "department", &MCN_department },
	{ "note", &MCN_note },

	{ "email", &MCN_email },
	{ "phone", &MCN_phone },
	{ "address", &MCN_address },

	{ "work", &MCN_work },
	{ "other", &MCN_other },

	{ "main", &MCN_main },
	{ "homefax", &MCN_homefax },
	{ "workfax", &MCN_workfax },
	{ "otherfax", &MCN_otherfax },
	{ "pager", &MCN_pager },
	
	{ "street", &MCN_street },
	{ "city", &MCN_city },
	{ "state", &MCN_state },
	{ "zip", &MCN_zip },
	{ "country", &MCN_country },
	{ "countrycode", &MCN_countrycode },
	
	
	{ "touchStart", &MCM_touch_start },
	{ "touchMove", &MCM_touch_move },
	{ "touchEnd", &MCM_touch_end },
	{ "touchRelease", &MCM_touch_release },
	{ "motionStart", &MCM_motion_start },
	{ "motionEnd", &MCM_motion_end },
	{ "motionRelease", &MCM_motion_release },
	{ "accelerationChanged", &MCM_acceleration_changed },
	{ "orientationChanged", &MCM_orientation_changed },
	{ "locationChanged", &MCM_location_changed },
	{ "locationError", &MCM_location_error },
	{ "headingChanged", &MCM_heading_changed },
	{ "headingError", &MCM_heading_error },
	{ "purchaseStateUpdate", &MCM_purchase_updated },
    { "rotationRateChanged", &MCM_rotation_rate_changed },
	{ "trackingError", &MCM_tracking_error },
    { "localNotificationReceived", &MCM_local_notification_received },
    { "pushNotificationReceived", &MCM_push_notification_received },
    { "pushNotificationRegistered", &MCM_push_notification_registered },
    { "pushNotificationRegistrationError", &MCM_push_notification_registration_error },
    { "urlWakeUp", &MCM_url_wake_up },
	{ "launchDataChanged", &MCM_launch_data_changed },
	{ "browserStartedLoading", &MCM_browser_started_loading },
	{ "browserFinishedLoading", &MCM_browser_finished_loading },
	{ "browserLoadFailed", &MCM_browser_load_failed },
    { "soundFinishedOnChannel", &MCM_sound_finished_on_channel },
    { "adLoaded", &MCM_ad_loaded },
	{ "adClicked", &MCM_ad_clicked },
    { "adLoadFailed", &MCM_ad_load_failed },
    { "adResizeStart", &MCM_ad_resize_start },
    { "adResizeEnd", &MCM_ad_resize_end },
    { "adExpandStart", &MCM_ad_expand_start },
    { "adExpandEnd", &MCM_ad_expand_end },
	{ "scrollerDidScroll", &MCM_scroller_did_scroll },
	{ "scrollerBeginDrag", &MCM_scroller_begin_drag },
	{ "scrollerEndDrag", &MCM_scroller_end_drag },
	{ "playerFinished", &MCM_player_finished },
	{ "playerError", &MCM_player_error },
	{ "playerPropertyAvailable", &MCM_player_property_available },
	{ "inputBeginEditing", &MCM_input_begin_editing },
	{ "inputEndEditing", &MCM_input_end_editing },
	{ "inputReturnKey", &MCM_input_return_key },
	{ "inputTextChanged", &MCM_input_text_changed },
    { "productDetailsReceived", &MCM_product_details_received },
    { "productRequestError", &MCM_product_request_error },
	{ "nfcTagReceived", &MCM_nfc_tag_received },
#endif
	
#ifdef _IOS_MOBILE
	{ "browserLoadRequest", &MCM_browser_load_request },
	{ "browserLoadRequested", &MCM_browser_load_requested },
	{ "scrollerBeginDecelerate", &MCM_scroller_begin_decelerate },
	{ "scrollerEndDecelerate", &MCM_scroller_end_decelerate },
	{ "scrollerScrollToTop", &MCM_scroller_scroll_to_top },
	{ "playerProgressChanged", &MCM_player_progress_changed },
	{ "playerEnterFullscreen", &MCM_player_enter_fullscreen },
	{ "playerLeaveFullscreen", &MCM_player_leave_fullscreen },
	{ "playerStateChanged", &MCM_player_state_changed },
	{ "playerMovieChanged", &MCM_player_movie_changed },
	{ "playerStopped", &MCM_player_stopped },
	{ "reachabilityChanged", &MCM_reachability_changed },
    { "protectedDataDidBecomeAvailable", &MCM_protected_data_available },
    { "protectedDataWillBecomeUnavailable", &MCM_protected_data_unavailable },
	{ "remoteControlReceived", &MCM_remote_control_received },
#endif
    
    { "(Default)", &MCN_font_default },
    { "(Styled Text)", &MCN_font_usertext },
    { "(Menu)", &MCN_font_menutext },
    { "(Text)", &MCN_font_content },
    { "(Message)", &MCN_font_message },
    { "(Tooltip)", &MCN_font_tooltip },
    { "(System)", &MCN_font_system },
};

void X_initialize_names(void)
{
    for(size_t i = 0; i < sizeof(kInitialNames) / sizeof(kInitialNames[0]); i++)
    {
        MCNameCreateWithNativeChars((const char_t*)kInitialNames[i].cstring, strlen(kInitialNames[i].cstring), *kInitialNames[i].name_var);
    }
}

void MCU_finalize_names(void)
{
    for(size_t i = 0; i < sizeof(kInitialNames) / sizeof(kInitialNames[0]); i++)
    {
        MCValueRelease(*kInitialNames[i].name_var);
    }
}
