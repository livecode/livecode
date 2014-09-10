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

extern const uint1 MCisotranslations[256];
extern const uint1 MCmactranslations[256];

extern const char *MCtoolnames[];

extern const uint4 MCbuildnumber;
extern const char *MCcopystring;
extern const char *MCstandardstring;
extern const char *MCdialogstring;
extern const char *MCmovablestring;
extern const char *MCpalettestring;
extern const char *MCsheetstring;
extern const char *MCdrawerstring;
extern const char *MCmodalstring;
extern const char *MCmodelessstring;
extern const char *MCtoplevelstring;
extern const char *MCtransparentstring;
extern const char *MCopaquestring;
extern const char *MCrectanglestring;
extern const char *MCshadowstring;
extern const char *MCscrollingstring;
extern const char *MCroundrectstring;
extern const char *MCcheckboxstring;
extern const char *MCradiobuttonstring;
extern const char *MClinestring;
extern const char *MCpolygonstring;
extern const char *MCarcstring;
extern const char *MCovalstring;
extern const char *MCregularstring;
extern const char *MCcurvestring;
extern const char *MCtextstring;
extern const char *MCleftstring;
extern const char *MCcenterstring;
extern const char *MCrightstring;
extern const char *MCjustifystring;
extern const char *MCplainstring;
extern const char *MCmixedstring;
extern const char *MCboxstring;
extern const char *MCthreedboxstring;
extern const char *MCunderlinestring;
extern const char *MCstrikeoutstring;
extern const char *MCgroupstring;
extern const char *MClinkstring;
extern const char *MCtruestring;
extern const char *MCfalsestring;
extern const char *MCshiftstring;
extern const char *MCcommandstring;
extern const char *MCcontrolstring;
extern const char *MCmod1string;
extern const char *MCpulldownstring;
extern const char *MCpopupstring;
extern const char *MCoptionstring;
extern const char *MCcascadestring;
extern const char *MCcombostring;
extern const char *MCtabstring;
extern const char *MCstackstring;
extern const char *MCaudiostring;
extern const char *MCvideostring;
extern const char *MCdefaultstring;
extern const char *MCtitlestring;
extern const char *MCmenustring;
extern const char *MCminimizestring;
extern const char *MCmaximizestring;
extern const char *MCclosestring;
extern const char *MCmetalstring;
extern const char *MCutilitystring;
extern const char *MCnoshadowstring;
extern const char *MCforcetaskbarstring;
extern const char *MCunicodestring;
extern const char *MCnativestring;

extern const char *MCbackgroundstring;
extern const char *MCcardstring;
extern const char *MCbuttonstring;
extern const char *MCgraphicstring;
extern const char *MCepsstring;
extern const char *MCscrollbarstring;
extern const char *MCplayerstring;
extern const char *MCscalestring;
extern const char *MCprogressstring;
extern const char *MCimagestring;
extern const char *MCfieldstring;
extern const char *MCcolorstring;
extern const char *MCmagnifierstring;

extern const char *MCnotfoundstring;
extern const char *MClnfamstring;
extern const char *MClnfmacstring;
extern const char *MClnfmotifstring;
extern const char *MClnfwinstring;
extern const char *MCuntitledstring;
extern const char *MCapplicationstring;
extern const char *MCasknamestring;
extern const char *MChelpnamestring;
extern const char *MChomenamestring;
extern const char *MChcstatnamestring;
extern const char *MCdonestring;
extern const char *MCnullstring;
extern const char *MCintersectstring;
extern const char *MCsurroundstring;
extern const char *MCtopstring;
extern const char *MCbottomstring;
extern const char *MCcancelstring;

extern const char *MCliststylestrings[];
extern const char *MCtextalignstrings[];

extern MCNameRef MCN_msg;
extern MCNameRef MCN_each;
extern MCNameRef MCN_it;

extern MCNameRef MCN_cancel;

extern MCNameRef MCN_default_text_font;
extern MCNameRef MCN_platform_string;
extern MCNameRef MCN_version_string;

extern MCNameRef MCN_style;
extern MCNameRef MCN_metadata;
extern MCNameRef MCN_runs;

extern MCNameRef MCN_down;
extern MCNameRef MCN_up;

extern MCNameRef MCN_empty;
extern MCNameRef MCN_files;
extern MCNameRef MCN_image;
extern MCNameRef MCN_objects;
extern MCNameRef MCN_private;
extern MCNameRef MCN_text;
//extern MCNameRef MCN_unicode;
extern MCNameRef MCN_styles;
extern MCNameRef MCN_rtf;
extern MCNameRef MCN_html;

extern MCNameRef MCN_browser;
extern MCNameRef MCN_command_line;
extern MCNameRef MCN_development;
extern MCNameRef MCN_helper_application;
extern MCNameRef MCN_installer;
extern MCNameRef MCN_mobile;
extern MCNameRef MCN_player;
extern MCNameRef MCN_server;
extern MCNameRef MCN_standalone_application;

extern MCNameRef MCN_all;
extern MCNameRef MCN_auto_key;
extern MCNameRef MCN_disk;
extern MCNameRef MCN_activate;
extern MCNameRef MCN_high_level;
extern MCNameRef MCN_system;

extern MCNameRef MCN_ansi;
extern MCNameRef MCN_arabic;
extern MCNameRef MCN_bulgarian;
extern MCNameRef MCN_chinese;
extern MCNameRef MCN_english;
extern MCNameRef MCN_greek;
extern MCNameRef MCN_hebrew;
extern MCNameRef MCN_japanese;
extern MCNameRef MCN_korean;
extern MCNameRef MCN_lithuanian;
extern MCNameRef MCN_polish;
extern MCNameRef MCN_roman;
extern MCNameRef MCN_russian;
extern MCNameRef MCN_simple_chinese;
extern MCNameRef MCN_thai;
extern MCNameRef MCN_turkish;
extern MCNameRef MCN_ukrainian;
extern MCNameRef MCN_unicode;
extern MCNameRef MCN_utf8;
extern MCNameRef MCN_vietnamese;
extern MCNameRef MCN_w_char;
extern MCNameRef MCN_asterisk_char;

extern MCNameRef MCN_plain;
extern MCNameRef MCN_bold;
extern MCNameRef MCN_italic;
extern MCNameRef MCN_bold_italic;

extern MCNameRef MCN_unknown;
extern MCNameRef MCN_x86;
extern MCNameRef MCN_x86_64;
extern MCNameRef MCN_motorola_powerpc;
extern MCNameRef MCN_i386;
extern MCNameRef MCN_arm;

extern MCNameRef MCN_local_mac;
extern MCNameRef MCN_local_win32;
extern MCNameRef MCN_android;
extern MCNameRef MCN_iphone;
extern MCNameRef MCN_wince;

extern MCNameRef MCN_mac_os;
extern MCNameRef MCN_win32;

extern MCNameRef MCN_done;

extern MCNameRef MCN_staticgray;
extern MCNameRef MCN_grayscale;
extern MCNameRef MCN_staticcolor;
extern MCNameRef MCN_pseudocolor;
extern MCNameRef MCN_truecolor;
extern MCNameRef MCN_directcolor;

extern MCNameRef MCN_bounds;
extern MCNameRef MCN_pixels;
extern MCNameRef MCN_opaque_pixels;

extern MCNameRef MCN_desktop;
extern MCNameRef MCN_documents;
extern MCNameRef MCN_engine;
extern MCNameRef MCN_fonts;
extern MCNameRef MCN_home;
extern MCNameRef MCN_start;
//extern MCNameRef MCN_system;
extern MCNameRef MCN_temporary;
extern MCNameRef MCN_support;

extern MCNameRef MCN_apple;
extern MCNameRef MCN_control;
extern MCNameRef MCN_extension;
extern MCNameRef MCN_preferences;

extern MCNameRef MCN_unhandled;
extern MCNameRef MCN_handled;
extern MCNameRef MCN_passed;

extern MCNameRef MCN_page_setup_dialog;
extern MCNameRef MCN_pagesetup;
extern MCNameRef MCN_print_dialog;
extern MCNameRef MCN_printer;
extern MCNameRef MCN_color_chooser;
extern MCNameRef MCN_color;
extern MCNameRef MCN_file_selector;
extern MCNameRef MCN_file;
//extern MCNameRef MCN_files;
extern MCNameRef MCN_folder;
extern MCNameRef MCN_folders;
extern MCNameRef MCN_answer_dialog;
extern MCNameRef MCN_ask_dialog;

//extern MCNameRef MCN_plain;
extern MCNameRef MCN_clear;
//extern MCNameRef MCN_color;
extern MCNameRef MCN_effect;
extern MCNameRef MCN_error;
//extern MCNameRef MCN_file;
//extern MCNameRef MCN_folder;
extern MCNameRef MCN_information;
extern MCNameRef MCN_password;
//extern MCNameRef MCN_printer;
extern MCNameRef MCN_program;
extern MCNameRef MCN_question;
extern MCNameRef MCN_record;
extern MCNameRef MCN_titled;
extern MCNameRef MCN_warning;

extern MCNameRef MCN_messagename;
extern MCNameRef MCN_hcstat;

extern MCNameRef MCM_apple_event;
extern MCNameRef MCM_arrow_key;
extern MCNameRef MCM_assert_error;
extern MCNameRef MCM_backspace_key;
extern MCNameRef MCM_close_background;
extern MCNameRef MCM_close_card;
extern MCNameRef MCM_close_control;
extern MCNameRef MCM_close_field;
extern MCNameRef MCM_close_stack;
extern MCNameRef MCM_close_stack_request;
extern MCNameRef MCM_color_changed;
extern MCNameRef MCM_command_key_down;
extern MCNameRef MCM_control_key_down;
extern MCNameRef MCM_copy_key;
extern MCNameRef MCM_current_time_changed;
extern MCNameRef MCM_cut_key;
extern MCNameRef MCM_debug_str;

extern MCNameRef MCM_delete_background;
extern MCNameRef MCM_delete_button;
extern MCNameRef MCM_delete_card;
extern MCNameRef MCM_delete_eps;
extern MCNameRef MCM_delete_field;
extern MCNameRef MCM_delete_graphic;
extern MCNameRef MCM_delete_group;
extern MCNameRef MCM_delete_image;
extern MCNameRef MCM_delete_scrollbar;
extern MCNameRef MCM_delete_player;
extern MCNameRef MCM_delete_stack;

extern MCNameRef MCM_delete_key;
extern MCNameRef MCM_delete_url;
extern MCNameRef MCM_desktop_changed;
extern MCNameRef MCM_drag_drop;
extern MCNameRef MCM_drag_end;
extern MCNameRef MCM_drag_enter;
extern MCNameRef MCM_drag_leave;
extern MCNameRef MCM_drag_move;
extern MCNameRef MCM_drag_start;
extern MCNameRef MCM_edit_script;
extern MCNameRef MCM_enter_in_field;
extern MCNameRef MCM_enter_key;
extern MCNameRef MCM_error_dialog;
extern MCNameRef MCM_escape_key;
extern MCNameRef MCM_eval;
extern MCNameRef MCM_exit_field;
extern MCNameRef MCM_focus_in;
extern MCNameRef MCM_focus_out;
extern MCNameRef MCM_function_key;
extern MCNameRef MCM_get_cached_urls;
extern MCNameRef MCM_get_url;
extern MCNameRef MCM_get_url_status;

// MM-2012-11-06: [[ Property Listener ]]
extern MCNameRef MCM_gradient_edit_ended;
extern MCNameRef MCM_gradient_edit_started;

extern MCNameRef MCM_help;
extern MCNameRef MCM_hot_spot_clicked;
extern MCNameRef MCM_icon_menu_pick;
extern MCNameRef MCM_icon_menu_opening;
extern MCNameRef MCM_status_icon_menu_pick;
extern MCNameRef MCM_status_icon_menu_opening;
extern MCNameRef MCM_status_icon_click;
extern MCNameRef MCM_status_icon_double_click;
extern MCNameRef MCM_iconify_stack;
extern MCNameRef MCM_id_changed;
extern MCNameRef MCM_idle;
extern MCNameRef MCM_license;
extern MCNameRef MCM_internal;
extern MCNameRef MCM_internal2;
extern MCNameRef MCM_internal3;
extern MCNameRef MCM_key_down;
extern MCNameRef MCM_key_up;
extern MCNameRef MCM_keyboard_activated;
extern MCNameRef MCM_keyboard_deactivated;
extern MCNameRef MCM_library_stack;
extern MCNameRef MCM_link_clicked;
extern MCNameRef MCM_load_url;
extern MCNameRef MCM_main_stack_changed;

// MW-2013-03-20: [[ MainStacksChanged ]]
extern MCNameRef MCM_main_stacks_changed;

extern MCNameRef MCM_menu_pick;
extern MCNameRef MCM_message;
extern MCNameRef MCM_message_handled;
extern MCNameRef MCM_message_not_handled;
extern MCNameRef MCM_mouse_double_down;
extern MCNameRef MCM_mouse_double_up;
extern MCNameRef MCM_mouse_down;
extern MCNameRef MCM_mouse_down_in_backdrop;
extern MCNameRef MCM_mouse_enter;
extern MCNameRef MCM_mouse_leave;
extern MCNameRef MCM_mouse_move;
extern MCNameRef MCM_mouse_release;
extern MCNameRef MCM_mouse_still_down;
extern MCNameRef MCM_mouse_up;
extern MCNameRef MCM_mouse_up_in_backdrop;
extern MCNameRef MCM_mouse_within;
extern MCNameRef MCM_move_control;
extern MCNameRef MCM_move_stack;
extern MCNameRef MCM_move_stopped;
extern MCNameRef MCM_movie_touched;
extern MCNameRef MCM_name_changed;
extern MCNameRef MCM_new_background;
extern MCNameRef MCM_new_card;
extern MCNameRef MCM_new_stack;
extern MCNameRef MCM_new_tool;
extern MCNameRef MCM_node_changed;

// MM-2012-11-05: [[ Object selection started/ended message ]]
extern MCNameRef MCM_object_selection_ended;
extern MCNameRef MCM_object_selection_started;

extern MCNameRef MCM_open_background;
extern MCNameRef MCM_open_control;
extern MCNameRef MCM_open_card;
extern MCNameRef MCM_open_field;
extern MCNameRef MCM_open_stack;
extern MCNameRef MCM_option_key_down;
extern MCNameRef MCM_paste_key;
extern MCNameRef MCM_play_paused;
extern MCNameRef MCM_play_started;
extern MCNameRef MCM_play_stopped;
extern MCNameRef MCM_post_url;
extern MCNameRef MCM_preopen_background;
extern MCNameRef MCM_preopen_card;
extern MCNameRef MCM_preopen_control;
extern MCNameRef MCM_preopen_stack;

// MM-2012-09-05: [[ Property Listener ]]
extern MCNameRef MCM_property_changed;

extern MCNameRef MCM_put_url;
extern MCNameRef MCM_qtdebugstr;
extern MCNameRef MCM_raw_key_down;
extern MCNameRef MCM_raw_key_up;
#ifdef FEATURE_RELAUNCH_SUPPORT
extern MCNameRef MCM_relaunch;
#endif
extern MCNameRef MCM_release_stack;
extern MCNameRef MCM_reload_stack;
extern MCNameRef MCM_resize_control;

// MM-2012-11-06: [[ Property Listener ]]
extern MCNameRef MCM_resize_control_ended;
extern MCNameRef MCM_resize_control_started;

extern MCNameRef MCM_resize_stack;
extern MCNameRef MCM_resolution_error;
extern MCNameRef MCM_resume;
extern MCNameRef MCM_resume_stack;
extern MCNameRef MCM_return_in_field;
extern MCNameRef MCM_return_key;
extern MCNameRef MCM_save_stack_request;
extern MCNameRef MCM_script_error;
extern MCNameRef MCM_script_execution_error;
extern MCNameRef MCM_scrollbar_beginning;
extern MCNameRef MCM_scrollbar_drag;
extern MCNameRef MCM_scrollbar_end;
extern MCNameRef MCM_scrollbar_line_dec;
extern MCNameRef MCM_scrollbar_line_inc;
extern MCNameRef MCM_scrollbar_page_dec;
extern MCNameRef MCM_scrollbar_page_inc;
extern MCNameRef MCM_selected_object_changed;
extern MCNameRef MCM_selection_changed;
extern MCNameRef MCM_signal;
extern MCNameRef MCM_shell;
extern MCNameRef MCM_shut_down;
extern MCNameRef MCM_shut_down_request;
extern MCNameRef MCM_socket_error;
extern MCNameRef MCM_socket_closed;
extern MCNameRef MCM_socket_timeout;
extern MCNameRef MCM_start_up;
extern MCNameRef MCM_suspend;
extern MCNameRef MCM_suspend_stack;
extern MCNameRef MCM_tab_key;
extern MCNameRef MCM_text_changed;
extern MCNameRef MCM_trace;
extern MCNameRef MCM_trace_break;
extern MCNameRef MCM_trace_done;
extern MCNameRef MCM_trace_error;

extern MCNameRef MCM_undo_changed;
extern MCNameRef MCM_undo_key;
extern MCNameRef MCM_uniconify_stack;
extern MCNameRef MCM_unload_url;
extern MCNameRef MCM_update_var;

#ifdef _MOBILE
extern MCNameRef MCN_firstname;
extern MCNameRef MCN_lastname;
extern MCNameRef MCN_middlename;
extern MCNameRef MCN_prefix;
extern MCNameRef MCN_suffix;
extern MCNameRef MCN_nickname;
extern MCNameRef MCN_firstnamephonetic;
extern MCNameRef MCN_lastnamephonetic;
extern MCNameRef MCN_middlenamephonetic;
extern MCNameRef MCN_organization;
extern MCNameRef MCN_jobtitle;
extern MCNameRef MCN_department;
extern MCNameRef MCN_note;

extern MCNameRef MCN_email;
extern MCNameRef MCN_phone;
extern MCNameRef MCN_address;

extern MCNameRef MCN_home;
extern MCNameRef MCN_work;
extern MCNameRef MCN_other;
extern MCNameRef MCN_mobile;
extern MCNameRef MCN_iphone;
extern MCNameRef MCN_main;
extern MCNameRef MCN_homefax;
extern MCNameRef MCN_workfax;
extern MCNameRef MCN_otherfax;
extern MCNameRef MCN_pager;

extern MCNameRef MCN_street;
extern MCNameRef MCN_city;
extern MCNameRef MCN_state;
extern MCNameRef MCN_zip;
extern MCNameRef MCN_country;
extern MCNameRef MCN_countrycode;

extern MCNameRef MCM_touch_start;
extern MCNameRef MCM_touch_move;
extern MCNameRef MCM_touch_end;
extern MCNameRef MCM_touch_release;

extern MCNameRef MCM_motion_start;
extern MCNameRef MCM_motion_end;
extern MCNameRef MCM_motion_release;

extern MCNameRef MCM_url_progress;

extern MCNameRef MCM_acceleration_changed;

extern MCNameRef MCM_orientation_changed;

extern MCNameRef MCM_location_changed;
extern MCNameRef MCM_location_error;

extern MCNameRef MCM_heading_changed;
extern MCNameRef MCM_heading_error;
extern MCNameRef MCM_purchase_updated;

extern MCNameRef MCM_rotation_rate_changed;
extern MCNameRef MCM_tracking_error;

extern MCNameRef MCM_local_notification_received;
extern MCNameRef MCM_push_notification_received;
extern MCNameRef MCM_push_notification_registered;
extern MCNameRef MCM_push_notification_registration_error;
extern MCNameRef MCM_url_wake_up;

extern MCNameRef MCM_browser_started_loading;
extern MCNameRef MCM_browser_finished_loading;
extern MCNameRef MCM_browser_load_failed;

extern MCNameRef MCM_sound_finished_on_channel;

extern MCNameRef MCM_ad_loaded;
extern MCNameRef MCM_ad_clicked;
extern MCNameRef MCM_ad_load_failed;
extern MCNameRef MCM_ad_resize_start;
extern MCNameRef MCM_ad_resize_end;
extern MCNameRef MCM_ad_expand_start;
extern MCNameRef MCM_ad_expand_end;


extern MCNameRef MCM_scroller_did_scroll;
extern MCNameRef MCM_scroller_begin_drag;
extern MCNameRef MCM_scroller_end_drag;

extern MCNameRef MCM_player_finished;
extern MCNameRef MCM_player_error;
extern MCNameRef MCM_player_property_available;

extern MCNameRef MCM_input_begin_editing;
extern MCNameRef MCM_input_end_editing;
extern MCNameRef MCM_input_return_key;
extern MCNameRef MCM_input_text_changed;
extern MCNameRef MCM_product_details_received;
extern MCNameRef MCM_product_request_error;
#endif

#ifdef _IOS_MOBILE
extern MCNameRef MCM_browser_load_request;
extern MCNameRef MCM_browser_load_requested;
extern MCNameRef MCM_scroller_begin_decelerate;
extern MCNameRef MCM_scroller_end_decelerate;
extern MCNameRef MCM_scroller_scroll_to_top;
extern MCNameRef MCM_player_progress_changed;
extern MCNameRef MCM_player_enter_fullscreen;
extern MCNameRef MCM_player_leave_fullscreen;
extern MCNameRef MCM_player_state_changed;
extern MCNameRef MCM_player_movie_changed;
extern MCNameRef MCM_player_stopped;
extern MCNameRef MCM_reachability_changed;
//extern MCNameRef MCM_product_details_received;
//extern MCNameRef MCM_product_request_error;
extern MCNameRef MCM_protected_data_available;
extern MCNameRef MCM_protected_data_unavailable;

// MW-2013-05-30: [[ RemoteControl ]] Message sent when a remote control event is received.
extern MCNameRef MCM_remote_control_received;
#endif

