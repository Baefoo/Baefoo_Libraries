/*
 *
 * SUIMenu.cpp -- SerialUI Menu implementation
 * Copyright (C) 2013 Pat Deegan.  All rights reserved.
 *
 * See SUIMenu.h for usage details.
 *
 * http://www.flyingcarsandstuff.com/projects/SerialUI
 *
 *
 * This program library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See file LICENSE.txt for further informations on licensing
 * terms.
 *
 */



#include "includes/SUIConfig.h"

#include "SerialUI.h"
#include "includes/SUIMenu.h"
#include "includes/SUIStrings.h"
#include "includes/SUIPlatform.h"


#ifdef SUI_DYNAMIC_MEMORY_ALLOCATION_ENABLE
// dynamic memory management enabled
// stdlib included for malloc
#include <stdlib.h>
#define MENUITEMLIST	item_list
#define MENUFREE(pointer)	free(pointer)
#define MENUMEMSET(pointer, value, len)		memset(pointer, value, len);
#else
// using static memory, so we'll set
// MENUITEMLIST to the static list.
#define MENUITEMLIST	item_staticlist
#define MENUFREE(pointer)	;
#define MENUMEMSET(pointer, value, len)		\
		for (uint8_t _pi=0; _pi<len; _pi++) \
		{ \
			pointer[_pi] = value; \
		}

#endif


#define SUI_MENU_PROGMEM_STRING_ABS_MAXLEN		SUI_SERIALUI_PROGMEM_STRING_ABS_MAXLEN

#ifdef SUI_INCLUDE_DEBUG
// debug is ON
#define SUI_MENU_DEBUG_OUTPUT(msg)	sui_driver->debug_P(PSTR(msg));
#else
// debug is OFF, send debug messages into the void
#define SUI_MENU_DEBUG_OUTPUT(msg) ;
#endif



namespace SUI {

SUI_DeclareString(help_key, SUI_SERIALUI_HELP_KEY);
SUI_DeclareString(help_help, SUI_SERIALUI_HELP_HELP);

#ifdef SUI_MENU_ENABLE_SUBMENUS
SUI_DeclareString(up_key, SUI_SERIALUI_UP_KEY);
SUI_DeclareString(up_help, SUI_SERIALUI_UP_HELP);
SUI_DeclareString(submenu_help, SUI_SERIALUI_SUBMENU_HELP);
#endif

SUI_DeclareString(help_title_prefix, SUI_SERIALUI_HELP_TITLE_PREFIX);
SUI_DeclareString(help_key_commandprefix, SUI_SERIALUI_KEYHELP_COMMAND_PREFIX);
SUI_DeclareString(help_key_submenuprefix, SUI_SERIALUI_KEYHELP_SUBMENU_PREFIX);

SUI_DeclareString(unknown_sel, SUI_SERIALUI_UNKNOWN_SELECTION);
SUI_DeclareString(unknown_inmenu, SUI_SERIALUI_UNKNOWN_INMENU);
SUI_DeclareString(help_hint, SUI_SERIALUI_HELP_HINT);

SUI_DeclareString(ok_msg, SUI_SERIALUI_MESSAGE_OK);
SUI_DeclareString(error_generic, SUI_SERIALUI_MESSAGE_ERROR_GENERIC);
SUI_DeclareString(error_prefix, SUI_SERIALUI_MESSAGE_ERROR_PREFIX);
SUI_DeclareString(help_sep, SUI_SERIALUI_KEYHELP_SEP);



SUI_DeclareString(exit_key, SUI_SERIALUI_EXIT_KEY);
SUI_DeclareString(exit_help, SUI_SERIALUI_EXIT_HELP);

#ifdef SUI_SERIALUI_ECHO_WARNINGS
SUI_DeclareString(error_cantalloc_submenu, SUI_ERRORMSG_CANTALLOCATE_SUBMENU);
SUI_DeclareString(error_cantalloc_menuitem, SUI_ERRORMSG_CANTALLOCATE_MENUITEM);
SUI_DeclareString(error_cantalloc_key, SUI_ERRORMSG_CANTALLOCATE_KEY);

SUI_DeclareString(error_nomenuitems, SUI_ERRORMSG_NO_MENUITEMS_SET);

#endif

#ifndef SUI_DYNAMIC_MEMORY_ALLOCATION_ENABLE
#ifdef SUI_MENU_ENABLE_SUBMENUS
// dynamic memory is DISABLED and we want sub-menus
Menu Menu::submenu_staticlist[SUI_STATIC_MEMORY_NUM_SUBMENUS_TOTAL_MAXIMUM] = {};
uint8_t Menu::submenu_static_idx = 0;
#endif
#endif


#ifdef SUI_ENABLE_MODES
// Mode related strings
SUI_DeclareString(key_mode_user, SUI_STRINGS_MODE_USER);
SUI_DeclareString(key_mode_program, SUI_STRINGS_MODE_PROGRAM);
SUI_DeclareString(key_ping_command, SUI_STRINGS_PING_COMMAND);

// program-mode strings...
SUI_DeclareString(help_key_prog_commandprefix, SUI_SERIALUI_KEYHELP_COMMAND_PREFIX_PROG);
SUI_DeclareString(help_key_prog_submenuprefix, SUI_SERIALUI_KEYHELP_SUBMENU_PREFIX_PROG);

SUI_DeclareString(help_sep_prog, SUI_SERIALUI_KEYHELP_SEP_PROG);


// prog-mode enter info output
SUI_DeclareString(prog_mode_info_helpkey, SUI_SERIALUI_HELP_KEY);
SUI_DeclareString(prog_mode_info_moreprompt_string, SUI_SERIALUI_MOREDATA_STRING_PROMPT_PROG);
SUI_DeclareString(prog_mode_info_moreprompt_num, SUI_SERIALUI_MOREDATA_NUMERIC_PROMPT_PROG);

SUI_DeclareString(prog_mode_terminate_gui, SUI_SERIALUI_TERMINATE_GUI_PROG);

#ifdef SUI_ENABLE_STREAMPROMPTING
SUI_DeclareString(prog_mode_info_moreprompt_stream, SUI_SERIALUI_MOREDATA_STREAM_PROMPT_PROG);
#endif

SUI_DeclareString(prog_mode_info_EOT, SUI_SERIALUI_PROG_ENDOFTRANSMISSION);

SUI_DeclareString(prog_mode_info_VERSION, SERIAL_UI_VERSION_STRING);

#endif


size_t Menu::max_key_len = 0;

MenuItemDetailsStruct::MenuItemDetailsStruct(PGM_P key, MenuCommand_Callback cb, PGM_P help) :
			key_str(key), callback(cb), help_str(help)
{

}

MenuItemStruct::MenuItemStruct()
{

}
MenuItemStruct::MenuItemStruct(PGM_P key_pstr, PGM_P help_pstr,
		Menu * submenu_ptr, MenuCommand_Callback cmd_ptr)
{
	key = key_pstr;
	help = help_pstr;
	subMenu = submenu_ptr;
	command = cmd_ptr;
	key_size = strlen_P(key);
}









Menu::Menu(SerialUI * suidrv, PGM_P name, uint8_t num_items_hint, Menu * parent) :
		sui_driver(suidrv),
		menu_name(name), num_menu_items(0), max_menu_items(0), expand_list_amount(
				SUI_MENU_EXPANDITEMLIST_AMOUNT_DEFAULT), item_list(NULL),
				parent_menu(parent) {
	init(suidrv, name, num_items_hint, parent);
}
Menu::Menu() :
	menu_name(NULL), num_menu_items(0), max_menu_items(0), expand_list_amount(
					SUI_MENU_EXPANDITEMLIST_AMOUNT_DEFAULT), item_list(NULL),
					parent_menu(NULL)
{

}
#ifdef SUI_MENU_INCLUDE_DESTRUCTION_CLEANUP
Menu::~Menu()
{
	clear();
}

void Menu::clear()
{

	if (item_list)
	{
		for (uint8_t i=0; i<num_menu_items; i++)
		{
			if (item_list[i].subMenu)
			{
				item_list[i].subMenu->clear();
				item_list[i].subMenu = NULL;
			}
		}
		MENUFREE(item_list);
		item_list = NULL;
	}

	num_menu_items = 0;
	max_menu_items = 0;

}
#endif

void Menu::init(SerialUI * suidrv, PGM_P name, uint8_t num_items_hint, Menu * parent) {


	sui_driver = suidrv;
	menu_name = name;
	parent_menu = parent;
	num_menu_items = 0;
#ifdef SUI_DYNAMIC_MEMORY_ALLOCATION_ENABLE
	max_menu_items = 0;
#else
	max_menu_items = SUI_STATIC_MEMORY_NUM_ITEMS_MAXIMUM;
#endif

	expand_list_amount = SUI_MENU_EXPANDITEMLIST_AMOUNT_DEFAULT;
	item_list = NULL;


	// set our max_key_len to hold *at least*
	// the available built-in commands
	if (max_key_len < 1)
	{
		// max_key_len is now a class-level var, so
		// we only need to do this once.  It will
		// eventually hold the max key length for the
		// largest key in the whole menu system.
#ifdef SUI_ENABLE_MODES
		max_key_len = strlen_P(key_mode_program);
#else
		max_key_len = strlen_P(help_key);
		if (strlen_P(exit_key) > max_key_len)
		{
			max_key_len = strlen_P(exit_key);
		}
#endif
	}

#ifdef SUI_INCLUDE_EXTRA_SAFETYCHECKS
	if (strlen_P(menu_name) > SUI_MENU_PROGMEM_STRING_ABS_MAXLEN) {
		returnMessage(PSTR(SUI_ERRORMSG_MENUNAME_TOOLONG));
	}
#endif

#ifdef SUI_INCLUDE_DEBUG
	if (parent_menu)
	{
		// can't start doing output before SerialUI is initialized
		// so we only do this stuff for sub-menus
		SUI_MENU_DEBUG_OUTPUT("Initializing Menu ");
		showName();
		sui_driver->println(" ");
	}
#endif

	expandItemList(num_items_hint);
}

PGM_P Menu::name() {
	return menu_name;
}

void Menu::setName(PGM_P namestr) {
	menu_name = namestr;
}

Menu * Menu::parent() {
	return parent_menu;
}

bool Menu::addCommands(MenuItemDetails detailsList[], uint8_t number)
{

	for (uint8_t i=0; i<number; i++)
	{
		if (! this->addCommand(detailsList[i].key_str, detailsList[i].callback, detailsList[i].help_str))
			return false;

	}

	return true;

}

bool Menu::addCommand(PGM_P key_str, MenuCommand_Callback callback,
		PGM_P help_str) {

	MenuItem itm(key_str, help_str, NULL, callback);

#ifdef SUI_INCLUDE_EXTRA_SAFETYCHECKS
	if (itm.key_size > SUI_MENU_PROGMEM_STRING_ABS_MAXLEN) {

		returnMessage(PSTR(SUI_ERRORMSG_MENUITEM_KEY_TOOLONG));
	}

	if (help_str && strlen_P(help_str) > SUI_MENU_PROGMEM_STRING_ABS_MAXLEN) {
		sui_driver->println_P(help_str);
		returnMessage(PSTR(SUI_ERRORMSG_MENUITEM_HELP_TOOLONG));
	}

#endif


	return addMenuItem(&itm);

}
#ifdef SUI_MENU_ENABLE_SUBMENUS
Menu * Menu::subMenu(PGM_P key_str, PGM_P help_str) {

#ifdef SUI_DYNAMIC_MEMORY_ALLOCATION_ENABLE
	// dynamic memory is enabled, so we malloc a pointer to fresh
	// memory, and keep a reference to this block within the
	// MenuItem:
	Menu * sub_men = (Menu*) malloc(sizeof(Menu));
#else
	// we'll just use the next available sub-menu slot
	if (submenu_static_idx >= SUI_STATIC_MEMORY_NUM_SUBMENUS_TOTAL_MAXIMUM)
		return NULL;

	Menu * sub_men = &(submenu_staticlist[submenu_static_idx]);
	submenu_static_idx++;
#endif

	if (!sub_men) {

		SUI_MENU_DEBUG_OUTPUT("Submenu creation fail");
#ifdef SUI_SERIALUI_ECHO_WARNINGS
		returnMessage(error_cantalloc_submenu);
#endif
		return NULL;
	}
	sub_men->init(sui_driver, key_str, 0, this);

	MenuItem itm(key_str, help_str, sub_men, NULL);

	if (!addMenuItem(&itm)) {
		MENUFREE(sub_men);

		return NULL;
	}

#ifdef SUI_INCLUDE_DEBUG
	SUI_MENU_DEBUG_OUTPUT("Added submenu");
	sui_driver->showFreeRAM();
#endif
	return sub_men;

}
#endif

bool Menu::addMenuItem(MenuItem * itm) {

	if (num_menu_items >= max_menu_items) {
		// we need some more space for this item...


		if (!expandItemList()) {
			SUI_MENU_DEBUG_OUTPUT("Expansion failed!");
			return false;
		}
	}

	// no dynamic mem... we'll be using a static array
	MENUITEMLIST[num_menu_items] = *(itm);


	if (itm->key_size > max_key_len) {
		max_key_len = itm->key_size;
	}


	num_menu_items++;

	SUI_MENU_DEBUG_OUTPUT("Added menu item");

	return true;

}

bool Menu::expandItemList(uint8_t by_amount) {

#ifdef SUI_DYNAMIC_MEMORY_ALLOCATION_ENABLE
	// using dynamic memory allocations
	int menuitm_size = sizeof(MenuItem);
	MenuItem * new_list = NULL;

	if (!by_amount) {
		// set optional param to default value
		by_amount = expand_list_amount;
	}


	if (item_list) {
		// we've already got a list in hand... we want to expand it a bit
		new_list = (MenuItem*) realloc(item_list,
				menuitm_size * (num_menu_items + by_amount));


 #ifdef SUI_INCLUDE_DEBUG

		if (parent_menu)
		{
			// can't start doing output before SerialUI is initialized
			// so we only do this stuff for sub-menus
			SUI_MENU_DEBUG_OUTPUT("Dynamically expanding list of menu items for ");
			showName();
			sui_driver->println(" ");

			sui_driver->showFreeRAM();
		}
 #endif


	} else {
		// no list yet, malloc one please:
		new_list = (MenuItem*) malloc(menuitm_size * by_amount);

 #ifdef SUI_INCLUDE_DEBUG
		if (parent_menu)
		{
			// can't start doing output before SerialUI is initialized
			// so we only do this stuff for sub-menus
			SUI_MENU_DEBUG_OUTPUT("Allocating list of menu items for ");
			showName();
			sui_driver->println(" ");

			sui_driver->showFreeRAM();
		}
 #endif
	}


	if (new_list != NULL) {
		// we gots a new list, huzzah.

		// make note of our new space...
		max_menu_items += by_amount;


		// zero the newly allocated memory
		MENUMEMSET(&(new_list[num_menu_items]), 0, menuitm_size * by_amount);

		// keep that pointer
		item_list = new_list;

		// success!
		return true;
	}


	// abject failure...

 #ifdef SUI_SERIALUI_ECHO_WARNINGS
	returnMessage(error_cantalloc_menuitem);
 #endif

#endif
	// either there was an error, or we aren't using
	// dynamic memory allocations... in both cases:
	return false;

}

void Menu::enter() {

	SUI_MENU_DEBUG_OUTPUT("Entering menu");

	returnMessage(menu_name);
}

Menu * Menu::handleRequest() {

#ifdef SUI_INCLUDE_EXTRA_SAFETYCHECKS

	if (!num_menu_items) {

	#ifdef SUI_SERIALUI_ECHO_WARNINGS
		returnMessage(error_nomenuitems);
	#endif
		// error!
		return this;
	}
#endif

	char * key_entered = mallocReadKey();

	if (!key_entered) {
		// nothing entered
		sui_driver->println(' ');
		return this;
	}

#ifdef SUI_SERIALUI_ECHO_ON
	// echo
	if (sui_driver->echoCommands())
	{
#ifdef SUI_ENABLE_MODES
		if (sui_driver->mode() == SUIMode_User)
#endif
		{
			sui_driver->println(key_entered);
		}
	}

#endif

	MenuItem * itm = itemForKey(key_entered);
	if (itm) {
		// get rid of our malloc'ed key
		MENUFREE(key_entered);
		if (itm->command) {

			SUI_MENU_DEBUG_OUTPUT("Running command");

			(itm->command)();
			return this;
		}
#ifdef SUI_MENU_ENABLE_SUBMENUS
		else if (itm->subMenu) {

			SUI_MENU_DEBUG_OUTPUT("Entering submenu");

			itm->subMenu->enter();
			return itm->subMenu;
		}
#endif
		// get here, we have a problem.
		// error
		return this;
	}

	// not found...

#ifdef SUI_MENU_ENABLE_SUBMENUS
	if (parent_menu && strncmp_P(key_entered, up_key, strlen_P(up_key)) == 0) {
		// get rid of our malloc'ed key

		SUI_MENU_DEBUG_OUTPUT("Going up a level");

		MENUFREE(key_entered);
		return upLevel();
	}
#endif

	if (strncmp_P(key_entered, help_key, strlen_P(help_key)) == 0) {

		SUI_MENU_DEBUG_OUTPUT("Help request");

		// get rid of our malloc'ed key
		MENUFREE(key_entered);
		showHelp();
		return this;
	}

	if ( ( parent_menu == NULL) && strncmp_P(key_entered, exit_key, strlen_P(exit_key)) == 0) {
		// get rid of our malloc'ed key

		SUI_MENU_DEBUG_OUTPUT("Exit request");

		MENUFREE(key_entered);
		return NULL;
	}


#ifdef SUI_ENABLE_MODES
	// check for program mode command...
	if (strncmp_P(key_entered, key_mode_program, strlen_P(key_mode_program)) == 0)
	{
			SUI_MENU_DEBUG_OUTPUT("Entering program mode");
			sui_driver->setMode(SUIMode_Program);
			MENUFREE(key_entered);

			char sepChar[2];
			sepChar[0] = SUI_SERIALUI_PROG_STR_SEP_CHAR;
			sepChar[1] = '\0';

			char outBuf[SUI_SERIALUI_PROGMEM_STRING_ABS_MAXLEN];
			outBuf[0] = '\0';
			strcat(outBuf, sepChar);

			strcat_P(outBuf, prog_mode_info_VERSION);
			strcat(outBuf, sepChar);



#ifdef SUI_MENU_ENABLE_SUBMENUS
				strcat_P(outBuf, up_key);
#else
				strcat(outBuf, "X");
#endif	/* SUI_MENU_ENABLE_SUBMENUS */
			strcat(outBuf, sepChar);

			strcat_P(outBuf, exit_key);
			strcat(outBuf, sepChar);

			strcat_P(outBuf, error_generic);
			strcat(outBuf, sepChar);


			strcat_P(outBuf, prog_mode_info_helpkey);
			strcat(outBuf, sepChar);

			strcat_P(outBuf, help_key_prog_commandprefix);
			strcat(outBuf, sepChar);

			strcat_P(outBuf, help_key_prog_submenuprefix);
			strcat(outBuf, sepChar);


			strcat_P(outBuf, help_sep_prog);
			strcat(outBuf, sepChar);



			strcat_P(outBuf, prog_mode_info_moreprompt_string);
			strcat(outBuf, sepChar);



			strcat_P(outBuf, prog_mode_info_moreprompt_num);
			strcat(outBuf, sepChar);

			strcat(outBuf, SUI_SERIALUI_PROMPT);
			strcat(outBuf, sepChar);

			strcat_P(outBuf, prog_mode_info_EOT);
			strcat(outBuf, sepChar);

#ifdef SUI_ENABLE_STREAMPROMPTING
			strcat_P(outBuf, prog_mode_info_moreprompt_stream);
			strcat(outBuf, sepChar);
#endif


			strcat_P(outBuf, prog_mode_terminate_gui);
			strcat(outBuf, sepChar);

			sui_driver->print(strlen(outBuf) + 1, DEC);
			sui_driver->println(outBuf);

			return this;
	}

	if (strncmp_P(key_entered, key_mode_user, strlen_P(key_mode_user)) == 0) {
		SUI_MENU_DEBUG_OUTPUT("Entering 'user' mode");
		sui_driver->setMode(SUIMode_User);
		MENUFREE(key_entered);
		return this;
	}
#endif	/* SUI_ENABLE_MODES */

	if (strncmp_P(key_entered, key_ping_command, strlen_P(key_ping_command)) == 0) {

			MENUFREE(key_entered);
			pingRespond();
			return this;
		}


	// get rid of our malloc'ed key
	unknownCommand(key_entered);
	MENUFREE(key_entered);
	return this;

}

void Menu::printHelpKey(MenuItem * menuitem) {


	PGM_P prefix_cmd;
	PGM_P prefix_submenu;
	PGM_P help_sep_to_use;

	bool include_pretty_print = true;
#ifdef SUI_ENABLE_MODES
	if (sui_driver->mode() == SUIMode_User)
	{
		help_sep_to_use = help_sep;
		prefix_cmd = help_key_commandprefix;
		prefix_submenu = help_key_submenuprefix;
	} else {

		help_sep_to_use = help_sep_prog;
		prefix_cmd = help_key_prog_commandprefix;
		prefix_submenu = help_key_prog_submenuprefix;
		include_pretty_print = false;
	}
#else

	help_sep_to_use = help_sep;
	prefix_cmd = help_key_commandprefix;
	prefix_submenu = help_key_submenuprefix;
#endif



	if (menuitem->subMenu)
	{
		sui_driver->print_P(prefix_submenu);
	} else {
		sui_driver->print_P(prefix_cmd);
	}

	sui_driver->print_P(menuitem->key);

	if (menuitem->help || menuitem->subMenu) {
		if (include_pretty_print)
		{
			// we need to put in some spacing, for the help message
			for (uint8_t i = 0;
				i < (SUI_SERIALUI_KEYHELP_SEP_REPEATS_MAX - menuitem->key_size);
				i++) {

				sui_driver->print_P(help_sep_to_use);
			}
		} else {

			sui_driver->print_P(help_sep_to_use);
		}
	}
}

void Menu::showHelp() {

	bool in_program_mode = false;

#ifdef SUI_ENABLE_MODES
	if (sui_driver->mode() == SUIMode_Program)
	{
		in_program_mode = true;
	}
#endif

	if (! in_program_mode)
	{
		sui_driver->print_P(help_title_prefix);
		sui_driver->println_P(menu_name);
		sui_driver->println(' ');
	}

	for (uint8_t i = 0; i < num_menu_items; i++) {
		MenuItem * itm = &(MENUITEMLIST[i]);

		printHelpKey(itm);
		if (itm->help) {
			sui_driver->println_P(itm->help);
		}
#ifdef SUI_MENU_ENABLE_SUBMENUS
		else if (itm->subMenu) {
			sui_driver->println_P(submenu_help);
		}
#endif
		else {
			sui_driver->println(' ');
		}
	}

	if (in_program_mode)
		return; // don't need navigation help in program mode

	sui_driver->println(' ');

	if (parent_menu) {
#ifdef SUI_MENU_ENABLE_SUBMENUS
		sui_driver->print_P(help_key_commandprefix);
		sui_driver->print_P(up_key);
		for (uint8_t i = 0;
					i < SUI_SERIALUI_KEYHELP_SEP_REPEATS_MAX - strlen_P(up_key);
					i++) {

			sui_driver->print_P(help_sep);
		}

		sui_driver->println_P(up_help);
#endif
	} else {
		sui_driver->print_P(help_key_commandprefix);
		sui_driver->print_P(exit_key);
		for (uint8_t i = 0;
				i < SUI_SERIALUI_KEYHELP_SEP_REPEATS_MAX - strlen_P(exit_key);
				i++) {

			sui_driver->print_P(help_sep);
		}
		sui_driver->println_P(exit_help);
	}

	sui_driver->print_P(help_key_commandprefix);
	sui_driver->print_P(help_key);
	for (uint8_t i = 0;
			i < SUI_SERIALUI_KEYHELP_SEP_REPEATS_MAX - strlen_P(help_key);
			i++) {
		sui_driver->print_P(help_sep);
	}
	sui_driver->println_P(help_help);

}

#ifdef SUI_MENU_ENABLE_SUBMENUS
Menu * Menu::upLevel() {
	if (parent_menu) {
		parent_menu->enter();
		return parent_menu;
	}

	return this;
}
#endif

MenuItem * Menu::itemForKey(char * key_found) {

	size_t key_size = strlen(key_found);
	size_t cmp_size = 0;

	for (uint8_t i = 0; i < num_menu_items; i++) {
		MenuItem * itm = &(MENUITEMLIST[i]);

		// compare up to smallest of len(key entered) and len(key of this item)
		cmp_size = (key_size < itm->key_size) ? key_size : itm->key_size;

		if (strncmp_P(key_found, itm->key, cmp_size) == 0) {
			// have a match...
			return itm;
		}
	}

	return NULL;

}

void Menu::unknownCommand(char * key) {

	sui_driver->print_P(error_generic);
	sui_driver->print_P(unknown_sel);
	sui_driver->print(key);
	sui_driver->print_P(unknown_inmenu);
	sui_driver->println_P(menu_name);
	sui_driver->println_P(help_hint);
}

char * Menu::mallocReadKey() {

#ifdef SUI_INCLUDE_EXTRA_SAFETYCHECKS
	if (max_key_len < 1) {
		return NULL;
	}
#endif

#ifdef SUI_DYNAMIC_MEMORY_ALLOCATION_ENABLE
	// we'll need--at most--a container that is the size of our largest key for the menu
	char * akey = (char*) malloc(sizeof(char) * (max_key_len + 2));
#else
	char * akey = key_staticstr;
#endif

#ifdef SUI_INCLUDE_DEBUG
	SUI_MENU_DEBUG_OUTPUT("Post read key allocation")
	sui_driver->showFreeRAM();
#endif


	if (!akey) {

#ifdef SUI_SERIALUI_ECHO_WARNINGS
		returnMessage(error_cantalloc_key);
#endif

		return NULL;
	}

	MENUMEMSET(akey, 0, max_key_len + 2);

	uint8_t numRead = sui_driver->readBytesToEOL(akey, max_key_len + 1);
	if (!numRead) {
		MENUFREE(akey);
		return NULL;
	}

	// make sure we don't include any return/newlines in our key
	uint8_t num_nonEOL = 0;
	for (uint8_t i = 0; i < numRead; i++) {
		if (akey[i] == '\r' || akey[i] == '\n') {
			akey[i] = 0;
		} else {
			num_nonEOL ++;
		}
	}

	if (num_nonEOL)
	{
		// we had some valid, non-EOL, chars.
		return akey;
	}

	// no non-EOL
	MENUFREE(akey);
	return NULL;
}

void Menu::returnError(const char * errmsg) {
	if (errmsg) {
		sui_driver->print_P(error_prefix);
		sui_driver->println(errmsg);
	} else {
		sui_driver->println_P(error_generic);
	}
}

void Menu::returnError_P(PGM_P errmsg) {
	if (errmsg) {
		sui_driver->print_P(error_prefix);
		sui_driver->println_P(errmsg);
	} else {
		sui_driver->println_P(error_generic);
	}
}

void Menu::returnOK() {
	char p_buffer[4];
	strncpy_P(p_buffer, ok_msg, 4);
	sui_driver->println(p_buffer);

}

void Menu::returnMessage(PGM_P message) { sui_driver->println_P(message);}


void Menu::showName() { sui_driver->print_P(menu_name); }

void Menu::pingRespond() {
#ifdef SUI_ENABLE_STATE_TRACKER
	if (sui_driver->showTrackedState())
	{
		return;
	}
#endif

	sui_driver->println(' ');




}

} /* end namespace SUI */

