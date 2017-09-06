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

void X_initialize_names(void)
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
    /* UNCHECKED */ MCNameCreateWithCString("installer command line", MCN_installer_cmdline);
	/* UNCHECKED */ MCNameCreateWithCString("mobile", MCN_mobile);
	/* UNCHECKED */ MCNameCreateWithCString("player", MCN_player);
	/* UNCHECKED */ MCNameCreateWithCString("server", MCN_server);
	/* UNCHECKED */ MCNameCreateWithCString("standalone application", MCN_standalone_application);

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
	/* UNCHECKED */ MCNameCreateWithCString("msgchanged", MCM_msgchanged);
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
	/* UNCHECKED */ MCNameCreateWithCString("nfcTagReceived", MCM_nfc_tag_received);
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
	MCValueRelease(MCN_msg);
	MCValueRelease(MCN_each);
	MCValueRelease(MCN_it);

	MCValueRelease(MCN_cancel);
	
	MCValueRelease(MCN_default_text_font);
    MCValueRelease(MCN_platform_string);
	MCValueRelease(MCN_version_string);

	MCValueRelease(MCN_metadata);
	MCValueRelease(MCN_runs);
	MCValueRelease(MCN_style);

	MCValueRelease(MCN_down);
	MCValueRelease(MCN_up);

	MCValueRelease(MCN_empty);
	MCValueRelease(MCN_files);
	MCValueRelease(MCN_image);
	MCValueRelease(MCN_objects);
	MCValueRelease(MCN_private);
	MCValueRelease(MCN_text);
//	MCValueRelease(MCN_unicode);
	MCValueRelease(MCN_styles);
    MCValueRelease(MCN_styledtext);
    MCValueRelease(MCN_rtftext);
    MCValueRelease(MCN_htmltext);
    MCValueRelease(MCN_png);
    MCValueRelease(MCN_gif);
    MCValueRelease(MCN_jpeg);
	MCValueRelease(MCN_win_bitmap);
	MCValueRelease(MCN_win_metafile);
	MCValueRelease(MCN_win_enh_metafile);
	MCValueRelease(MCN_rtf);
	MCValueRelease(MCN_html);

	MCValueRelease(MCN_browser);
	MCValueRelease(MCN_command_line);
	MCValueRelease(MCN_development);
    MCValueRelease(MCN_development_cmdline);
	MCValueRelease(MCN_helper_application);
	MCValueRelease(MCN_installer);
	MCValueRelease(MCN_installer_cmdline);
	MCValueRelease(MCN_mobile);
	MCValueRelease(MCN_player);
	MCValueRelease(MCN_server);
	MCValueRelease(MCN_standalone_application);

	MCValueRelease(MCN_all);
	MCValueRelease(MCN_auto_key);
	MCValueRelease(MCN_disk);
	MCValueRelease(MCN_activate);
	MCValueRelease(MCN_high_level);
	MCValueRelease(MCN_system);

	MCValueRelease(MCN_ansi);
	MCValueRelease(MCN_arabic);
	MCValueRelease(MCN_bulgarian);
	MCValueRelease(MCN_chinese);
	MCValueRelease(MCN_english);
	MCValueRelease(MCN_greek);
	MCValueRelease(MCN_hebrew);
	MCValueRelease(MCN_japanese);
	MCValueRelease(MCN_korean);
	MCValueRelease(MCN_lithuanian);
	MCValueRelease(MCN_polish);
	MCValueRelease(MCN_roman);
	MCValueRelease(MCN_russian);
	MCValueRelease(MCN_simple_chinese);
	MCValueRelease(MCN_thai);
	MCValueRelease(MCN_turkish);
	MCValueRelease(MCN_ukrainian);
	MCValueRelease(MCN_unicode);
	MCValueRelease(MCN_utf8);
	MCValueRelease(MCN_vietnamese);
	MCValueRelease(MCN_w_char);
	MCValueRelease(MCN_asterisk_char);

	MCValueRelease(MCN_plain);
	MCValueRelease(MCN_bold);
	MCValueRelease(MCN_italic);
	MCValueRelease(MCN_bold_italic);

	MCValueRelease(MCN_unknown);
	MCValueRelease(MCN_x86);
	MCValueRelease(MCN_x86_64);
    MCValueRelease(MCN_motorola_powerpc);
    MCValueRelease(MCN_i386);
    MCValueRelease(MCN_arm);
    // SN-2015-01-07: [[ iOS-64bit ]] ARM64 added
    MCValueRelease(MCN_arm64);

	MCValueRelease(MCN_local_mac);
	MCValueRelease(MCN_local_win32);
	MCValueRelease(MCN_android);
	MCValueRelease(MCN_iphone);
	MCValueRelease(MCN_wince);

	MCValueRelease(MCN_mac_os);
	MCValueRelease(MCN_win32);

	MCValueRelease(MCN_done);

	MCValueRelease(MCN_staticgray);
	MCValueRelease(MCN_grayscale);
	MCValueRelease(MCN_staticcolor);
	MCValueRelease(MCN_pseudocolor);
	MCValueRelease(MCN_truecolor);
	MCValueRelease(MCN_directcolor);

	MCValueRelease(MCN_bounds);
	MCValueRelease(MCN_pixels);
	MCValueRelease(MCN_opaque_pixels);

	MCValueRelease(MCN_desktop);
	MCValueRelease(MCN_documents);
	MCValueRelease(MCN_engine);
    MCValueRelease(MCN_resources);
	MCValueRelease(MCN_fonts);
	MCValueRelease(MCN_home);
	MCValueRelease(MCN_start);
//	MCValueRelease(MCN_system);
	MCValueRelease(MCN_temporary);

	MCValueRelease(MCN_apple);
	MCValueRelease(MCN_control);
	MCValueRelease(MCN_extension);
	MCValueRelease(MCN_preferences);
	
	MCValueRelease(MCN_unhandled);
	MCValueRelease(MCN_handled);
	MCValueRelease(MCN_passed);

	MCValueRelease(MCN_page_setup_dialog);
	MCValueRelease(MCN_pagesetup);
	MCValueRelease(MCN_print_dialog);
	MCValueRelease(MCN_printer);
	MCValueRelease(MCN_color_chooser);
	MCValueRelease(MCN_color);
	MCValueRelease(MCN_file_selector);
	MCValueRelease(MCN_file);
	//MCValueRelease(MCN_files);
	MCValueRelease(MCN_folder);
	MCValueRelease(MCN_folders);
	MCValueRelease(MCN_answer_dialog);
	MCValueRelease(MCN_ask_dialog);

	//MCValueRelease(MCN_plain);
	MCValueRelease(MCN_clear);
	//MCValueRelease(MCN_color);
	MCValueRelease(MCN_effect);
	MCValueRelease(MCN_error);
	//MCValueRelease(MCN_file);
	//MCValueRelease(MCN_folder);
	MCValueRelease(MCN_information);
	MCValueRelease(MCN_password);
	//MCValueRelease(MCN_printer);
	MCValueRelease(MCN_program);
	MCValueRelease(MCN_question);
	MCValueRelease(MCN_record);
	MCValueRelease(MCN_titled);
	MCValueRelease(MCN_warning);
	
	MCValueRelease(MCN_messagename);
	MCValueRelease(MCM_msgchanged);
	MCValueRelease(MCN_hcstat);
    
	MCValueRelease(MCM_apple_event);
	MCValueRelease(MCM_arrow_key);
	MCValueRelease(MCM_assert_error);
	MCValueRelease(MCM_backspace_key);
	MCValueRelease(MCM_close_background);
	MCValueRelease(MCM_close_card);
	MCValueRelease(MCM_close_control);
	MCValueRelease(MCM_close_field);
	MCValueRelease(MCM_close_stack);
	MCValueRelease(MCM_close_stack_request);
	MCValueRelease(MCM_color_changed);
	MCValueRelease(MCM_command_key_down);
	MCValueRelease(MCM_control_key_down);
	MCValueRelease(MCM_copy_key);
	MCValueRelease(MCM_current_time_changed);
	MCValueRelease(MCM_cut_key);
	MCValueRelease(MCM_debug_str);
	MCValueRelease(MCM_delete_background);
	MCValueRelease(MCM_delete_button);
	MCValueRelease(MCM_delete_card);
	MCValueRelease(MCM_delete_eps);
	MCValueRelease(MCM_delete_field);
	MCValueRelease(MCM_delete_graphic);
	MCValueRelease(MCM_delete_group);
	MCValueRelease(MCM_delete_image);
	MCValueRelease(MCM_delete_key);
	MCValueRelease(MCM_delete_scrollbar);
	MCValueRelease(MCM_delete_player);
	MCValueRelease(MCM_delete_stack);
	MCValueRelease(MCM_delete_widget);
	MCValueRelease(MCM_delete_url);
	MCValueRelease(MCM_desktop_changed);
	MCValueRelease(MCM_drag_drop);
	MCValueRelease(MCM_drag_end);
	MCValueRelease(MCM_drag_enter);
	MCValueRelease(MCM_drag_leave);
	MCValueRelease(MCM_drag_move);
	MCValueRelease(MCM_drag_start);
	MCValueRelease(MCM_edit_script);
	MCValueRelease(MCM_enter_in_field);
	MCValueRelease(MCM_enter_key);
	MCValueRelease(MCM_error_dialog);
	MCValueRelease(MCM_escape_key);
	MCValueRelease(MCM_eval);
	MCValueRelease(MCM_exit_field);
	MCValueRelease(MCM_focus_in);
	MCValueRelease(MCM_focus_out);
	MCValueRelease(MCM_function_key);
	MCValueRelease(MCM_get_cached_urls);
	MCValueRelease(MCM_get_url);
	MCValueRelease(MCM_get_url_status);
	MCValueRelease(MCM_gradient_edit_ended);
	MCValueRelease(MCM_gradient_edit_started);
	MCValueRelease(MCM_help);
	MCValueRelease(MCM_hot_spot_clicked);
	MCValueRelease(MCM_icon_menu_pick);
	MCValueRelease(MCM_icon_menu_opening);
	MCValueRelease(MCM_status_icon_menu_pick);
	MCValueRelease(MCM_status_icon_menu_opening);
	MCValueRelease(MCM_status_icon_click);
	MCValueRelease(MCM_status_icon_double_click);
	MCValueRelease(MCM_iconify_stack);
	MCValueRelease(MCM_id_changed);
	MCValueRelease(MCM_idle);
	MCValueRelease(MCM_internal);
	MCValueRelease(MCM_internal2);
	MCValueRelease(MCM_key_down);
	MCValueRelease(MCM_key_up);
	MCValueRelease(MCM_keyboard_activated);
	MCValueRelease(MCM_keyboard_deactivated);
	MCValueRelease(MCM_library_stack);
	MCValueRelease(MCM_link_clicked);
	MCValueRelease(MCM_load_url);
	MCValueRelease(MCM_main_stack_changed);
	MCValueRelease(MCM_menu_pick);
	MCValueRelease(MCM_message);
	MCValueRelease(MCM_message_handled);
	MCValueRelease(MCM_message_not_handled);
	MCValueRelease(MCM_mouse_double_down);
	MCValueRelease(MCM_mouse_double_up);
	MCValueRelease(MCM_mouse_down);
	MCValueRelease(MCM_mouse_down_in_backdrop);
	MCValueRelease(MCM_mouse_enter);
	MCValueRelease(MCM_mouse_leave);
	MCValueRelease(MCM_mouse_move);
	MCValueRelease(MCM_mouse_release);
	MCValueRelease(MCM_mouse_still_down);
	MCValueRelease(MCM_mouse_up);
	MCValueRelease(MCM_mouse_up_in_backdrop);
	MCValueRelease(MCM_mouse_within);
	MCValueRelease(MCM_move_control);
	MCValueRelease(MCM_move_stack);
	MCValueRelease(MCM_move_stopped);
	MCValueRelease(MCM_movie_touched);
	MCValueRelease(MCM_name_changed);
	MCValueRelease(MCM_new_background);
	MCValueRelease(MCM_new_card);
	MCValueRelease(MCM_new_stack);
	MCValueRelease(MCM_new_tool);
	MCValueRelease(MCM_node_changed);
	MCValueRelease(MCM_object_selection_ended);
	MCValueRelease(MCM_object_selection_started);
	MCValueRelease(MCM_open_background);
	MCValueRelease(MCM_open_card);
	MCValueRelease(MCM_open_control);
	MCValueRelease(MCM_open_field);
	MCValueRelease(MCM_open_stack);
	MCValueRelease(MCM_option_key_down);
	MCValueRelease(MCM_paste_key);
	MCValueRelease(MCM_play_paused);
    MCValueRelease(MCM_play_rate_changed);
	MCValueRelease(MCM_play_started);
	MCValueRelease(MCM_play_stopped);
	MCValueRelease(MCM_post_url);
	MCValueRelease(MCM_preopen_background);
	MCValueRelease(MCM_preopen_card);
	MCValueRelease(MCM_preopen_control);
	MCValueRelease(MCM_preopen_stack);
	MCValueRelease(MCM_property_changed);
	MCValueRelease(MCM_put_url);
	MCValueRelease(MCM_qtdebugstr);
	MCValueRelease(MCM_raw_key_down);
	MCValueRelease(MCM_raw_key_up);
	MCValueRelease(MCM_relaunch);
	MCValueRelease(MCM_release_stack);
	MCValueRelease(MCM_reload_stack);
	MCValueRelease(MCM_resize_control);
	MCValueRelease(MCM_resize_control_ended);
	MCValueRelease(MCM_resize_control_started);
	MCValueRelease(MCM_resize_stack);
	MCValueRelease(MCM_resolution_error);
	MCValueRelease(MCM_resume);
	MCValueRelease(MCM_resume_stack);
	MCValueRelease(MCM_return_in_field);
	MCValueRelease(MCM_return_key);
	MCValueRelease(MCM_save_stack_request);
	MCValueRelease(MCM_script_error);
	MCValueRelease(MCM_script_execution_error);
	MCValueRelease(MCM_scrollbar_beginning);
	MCValueRelease(MCM_scrollbar_drag);
	MCValueRelease(MCM_scrollbar_end);
	MCValueRelease(MCM_scrollbar_line_dec);
	MCValueRelease(MCM_scrollbar_line_inc);
	MCValueRelease(MCM_scrollbar_page_dec);
	MCValueRelease(MCM_scrollbar_page_inc);
	MCValueRelease(MCM_selected_object_changed);
	MCValueRelease(MCM_selection_changed);
	MCValueRelease(MCM_shell);
	MCValueRelease(MCM_signal);
	MCValueRelease(MCM_shut_down);
	MCValueRelease(MCM_shut_down_request);
	MCValueRelease(MCM_socket_error);
	MCValueRelease(MCM_socket_closed);
	MCValueRelease(MCM_socket_timeout);
	MCValueRelease(MCM_start_up);
	MCValueRelease(MCM_suspend);
	MCValueRelease(MCM_suspend_stack);
	MCValueRelease(MCM_tab_key);
	MCValueRelease(MCM_text_changed);
	MCValueRelease(MCM_trace);
	MCValueRelease(MCM_trace_break);
	MCValueRelease(MCM_trace_done);
	MCValueRelease(MCM_trace_error);
	MCValueRelease(MCM_undo_changed);
	MCValueRelease(MCM_undo_key);
	MCValueRelease(MCM_uniconify_stack);
	MCValueRelease(MCM_unload_url);
	MCValueRelease(MCM_update_screen);
	MCValueRelease(MCM_update_var);

#ifdef FEATURE_PLATFORM_URL
	MCValueRelease(MCM_url_progress);
#endif

    MCValueRelease(MCM_delete_audioclip);
    MCValueRelease(MCM_delete_videoclip);
    MCValueRelease(MCM_new_audioclip);
    MCValueRelease(MCM_new_videoclip);
    
#ifdef _MOBILE
	MCValueRelease(MCN_firstname);
	MCValueRelease(MCN_lastname);
	MCValueRelease(MCN_middlename);
	MCValueRelease(MCN_prefix);
	MCValueRelease(MCN_suffix);
	MCValueRelease(MCN_nickname);
	MCValueRelease(MCN_firstnamephonetic);
	MCValueRelease(MCN_lastnamephonetic);
	MCValueRelease(MCN_middlenamephonetic);
	MCValueRelease(MCN_organization);
	MCValueRelease(MCN_jobtitle);
	MCValueRelease(MCN_department);
	MCValueRelease(MCN_note);

	MCValueRelease(MCN_email);
	MCValueRelease(MCN_phone);
	MCValueRelease(MCN_address);
	
//	MCValueRelease(MCN_home);
	MCValueRelease(MCN_work);
	MCValueRelease(MCN_other);
//	MCValueRelease(MCN_mobile);
//	MCValueRelease(MCN_iphone);
	MCValueRelease(MCN_main);
	MCValueRelease(MCN_homefax);
	MCValueRelease(MCN_workfax);
	MCValueRelease(MCN_otherfax);
	MCValueRelease(MCN_pager);
	
	MCValueRelease(MCN_street);
	MCValueRelease(MCN_city);
	MCValueRelease(MCN_state);
	MCValueRelease(MCN_zip);
	MCValueRelease(MCN_country);
	MCValueRelease(MCN_countrycode);
	
	MCValueRelease(MCM_touch_start);
	MCValueRelease(MCM_touch_move);
	MCValueRelease(MCM_touch_end);
	MCValueRelease(MCM_touch_release);
	MCValueRelease(MCM_motion_start);
	MCValueRelease(MCM_motion_end);
	MCValueRelease(MCM_motion_release);
	MCValueRelease(MCM_acceleration_changed);
	MCValueRelease(MCM_orientation_changed);
	MCValueRelease(MCM_location_changed);
	MCValueRelease(MCM_location_error);
	MCValueRelease(MCM_heading_changed);
	MCValueRelease(MCM_heading_error);
	MCValueRelease(MCM_purchase_updated);
    MCValueRelease(MCM_rotation_rate_changed);
    MCValueRelease(MCM_tracking_error);
    MCValueRelease(MCM_local_notification_received);
    MCValueRelease(MCM_push_notification_received);
    MCValueRelease(MCM_push_notification_registered);
    MCValueRelease(MCM_push_notification_registration_error);
    MCValueRelease(MCM_url_wake_up);
	MCValueRelease(MCM_launch_data_changed);
	MCValueRelease(MCM_browser_started_loading);
	MCValueRelease(MCM_browser_finished_loading);
	MCValueRelease(MCM_browser_load_failed);
    MCValueRelease(MCM_sound_finished_on_channel);
    MCValueRelease(MCM_ad_loaded);
    MCValueRelease(MCM_ad_clicked);
    MCValueRelease(MCM_ad_resize_start);
    MCValueRelease(MCM_ad_resize_end);
    MCValueRelease(MCM_ad_expand_start);
    MCValueRelease(MCM_ad_expand_end);
	MCValueRelease(MCM_scroller_did_scroll);
	MCValueRelease(MCM_scroller_begin_drag);
	MCValueRelease(MCM_scroller_end_drag);
	MCValueRelease(MCM_player_finished);
	MCValueRelease(MCM_player_error);
	MCValueRelease(MCM_player_property_available);
	MCValueRelease(MCM_input_begin_editing);
	MCValueRelease(MCM_input_end_editing);
	MCValueRelease(MCM_input_return_key);
	MCValueRelease(MCM_input_text_changed);
	MCValueRelease(MCM_nfc_tag_received);
#endif
	
#ifdef _IOS_MOBILE
	MCValueRelease(MCM_browser_load_request);
	MCValueRelease(MCM_browser_load_requested);
	MCValueRelease(MCM_scroller_begin_decelerate);
	MCValueRelease(MCM_scroller_end_decelerate);
	MCValueRelease(MCM_scroller_scroll_to_top);
	MCValueRelease(MCM_player_progress_changed);
	MCValueRelease(MCM_player_enter_fullscreen);
	MCValueRelease(MCM_player_leave_fullscreen);
	MCValueRelease(MCM_player_state_changed);
	MCValueRelease(MCM_player_movie_changed);
	MCValueRelease(MCM_player_stopped);
	MCValueRelease(MCM_reachability_changed);
#endif
    
    MCValueRelease(MCN_font_default);
    MCValueRelease(MCN_font_usertext);
    MCValueRelease(MCN_font_menutext);
    MCValueRelease(MCN_font_content);
    MCValueRelease(MCN_font_message);
    MCValueRelease(MCN_font_tooltip);
    MCValueRelease(MCN_font_system);
}
