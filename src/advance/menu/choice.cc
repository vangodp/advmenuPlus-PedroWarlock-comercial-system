/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 1999, 2000, 2001, 2002, 2003, 2005 Andrea Mazzoleni
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details. 
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "portable.h"

#include "choice.h"
#include "text.h"
#include "common.h"
#include "play.h"

using namespace std;

// ------------------------------------------------------------------------
// tristate

const string tristate(tristate_t v)
{
	switch (v) {
	case include : return "include";
	case exclude : return "exclude";
	case exclude_not : return "exclude_not";
	default:
		assert(0);
		return "include";
	}
}

bool tristate(tristate_t& v, const std::string& s)
{
	if (s == "include")
		v = include;
	else if (s == "exclude")
		v = exclude;
	else if (s == "exclude_not")
		v = exclude_not;
	else
		return false;
	return true;
}

// ------------------------------------------------------------------------
// choice

#define CHOICE_INDENT_1 " M"
#define CHOICE_INDENT_2 " +M"
#define CHOICE_INDENT_3 " OnlyM"

choice::choice(const string& Adesc, int Avalue, bool Aactive)
	: state(1), active(Aactive), line_separator(false) {
	association.value = Avalue;

	desc = Adesc;
	desc_def = Adesc;
}

choice::choice(const string& Adesc, void* Aptr, bool Aactive)
	: state(1), active(Aactive), line_separator(false)  {
	association.ptr = Aptr;

	desc = Adesc;
	desc_def = Adesc;
}

choice::choice(const string& Adesc, bool Abistate, int Avalue)
	: state(2), bistate(Abistate), active(true), line_separator(false) {
	association.value = Avalue;

	desc = Adesc;
	desc_def = " + \t" + Adesc;
	desc_not = "\t" + Adesc;
}

choice::choice(const string& Adesc, tristate_t Atristate, int Avalue)
	: state(3), tristate(Atristate), active(true), line_separator(false) {
	association.value = Avalue;

	desc = Adesc;
	desc_def = "\t" + Adesc;
	desc_not = " Not\t" + Adesc;
	desc_only = " Only\t" + Adesc;
}

choice::choice(const string& Adesc_def, const string& Adesc_not, const string& Adesc_only, tristate_t Atristate, int Avalue)
	: state(3), tristate(Atristate), active(true), line_separator(false) {
	association.value = Avalue;

	desc = Adesc_def;
	desc_def = Adesc_def;
	desc_not = Adesc_not;
	desc_only = Adesc_only;
}

const string& choice::desc_get() const
{
	return desc;
}

const string& choice::print_get() const
{
	if (state_get()==1) {
		return desc_def;
	} else if (state_get()==2) {
		if (bistate_get()) {
			return desc_def;
		} else {
			return desc_not;
		}
	} else {
		if (tristate_get() == exclude) {
			return desc_not;
		} else if (tristate_get() == exclude_not) {
			return desc_only;
		} else {
			return desc_def;
		}
	}
}
void scroll_draw(int x, int y, int dx, int pos, int delta, int max, int separators)
{
	// pos -> posicion del primer item mostrado relativa al total de items. (pos_base)
	// delta -> numero de items que se muestran (pos_rel_max)
	// max -> numero total de items (pos_max)
	// separators -> numero de lineas separadoras
	
	int dy = delta * int_font_dy_get() + separators * 3;
	
	if (max <= 1)
		return;

	if (pos >= max)
		pos = max - 1;
	if (pos + delta > max)
		delta = max - pos;

	int y0 = pos * (dy-dx-1) / (max-1);
	int y1 = dx + (pos+delta-1) * (dy-dx-1) / (max-1);

	// pinta el fondo
	int_clear_alpha(x, y, dx, dy, COLOR_CHOICE_HIDDEN_SELECT.foreground);
	// pinta el scrollbox o thumb o cursor
	if (dx<4)
		int_clear_alpha(x, y+y0+1, dx, y1-y0+1-2, COLOR_CHOICE_HIDDEN_SELECT.background);
	else
		int_clear_alpha(x+1, y+y0+1, dx-2, y1-y0+1-2, COLOR_CHOICE_HIDDEN_SELECT.background);
}

void choice_bag::draw(const string& title, int x, int y, int dx, int pos_base, int pos_rel, int rows, int separators)
{
	// si no caben todas las opciones pinta el scrollbar
	if (rows < size()) {
		scroll_draw(x+dx+1, y, (int_font_dx_get()/2)-2, pos_base, rows, size(), separators);
	}
	
	for(unsigned j=0;j<rows;++j) {
		int_color color;
		int_color colorf;

		choice_container::iterator i = begin() + pos_base + j;

		if (i->active_get()) {
			if (j==pos_rel) {
				color = COLOR_CHOICE_SELECT;
				colorf = COLOR_CHOICE_SELECT;
			} else {
				color = COLOR_CHOICE_NORMAL;
				colorf = COLOR_CHOICE_TITLE;
			}
		} else {
			if (j==pos_rel) {
				color = COLOR_CHOICE_HIDDEN_SELECT;
				colorf = COLOR_CHOICE_HIDDEN_SELECT;
			} else {
				color = COLOR_CHOICE_HIDDEN;
				colorf = COLOR_CHOICE_HIDDEN;
			}
		}

		int indent = 0;
		switch(i->state_get()) {
			case 1 : indent = int_put_width(CHOICE_INDENT_1); break;
			case 2 : indent = int_put_width(CHOICE_INDENT_2); break;
			case 3 : indent = int_put_width(CHOICE_INDENT_3); break;
		}

		int pos = 0;
		const string& desc = i->print_get();
		string tag = token_get(desc, pos, "\t");
		token_skip(desc, pos, "\t");
		string rest = token_get(desc, pos, "");
		string key;
		if (!rest.length()) {
			pos = 0;
			rest = token_get(tag, pos, "^");
			token_skip(desc, pos, "^");
			key = token_get(tag, pos, "");
			tag = "";
		}

		int_put_filled(x, y, dx, tag, colorf);

		int key_width = 0;
		if (int_put_width(key) + int_put_width(CHOICE_INDENT_1) + int_put_width(rest) < dx - indent) {
			key_width = int_put_width(key) + int_put_width(CHOICE_INDENT_1);
			int_put(x + dx - key_width, y, key_width, key, color);
		}

		bool in = false;
		int_put_special(in, x+indent, y, dx - indent - key_width, rest, colorf, color, color);

		y += int_font_dy_get();

		//	pinta las lineas separadoras
		if (separators)
			int_clear(x, y, dx, 3, COLOR_CHOICE_NORMAL.background);
		if (i->line_separator_get()) {
			int_clear(x+3, y + 1, dx-6, 1, COLOR_CHOICE_NORMAL.foreground);
			y += 3;
		}
	}
}

int choice_bag::run(const string& title, int x, int y, int dx, choice_container::iterator& pos)
{
	int key = EVENT_ESC;
	int done = 0;
	int border = int_font_dx_get() / 2 + 1;

	if (x < 0)
		x = (int_dx_get() - dx - border * 2) / 2;
	if (y < 0)
		y = (int_dy_get() - (size() + 1) * int_font_dy_get() - border * 2) / 2;

	int pos_rel_max = (int_dy_get() - y) / int_font_dy_get();
	pos_rel_max -= 2;
	if (pos_rel_max > size())
		pos_rel_max = size();
	int pos_base_upper = size() - pos_rel_max;
	int pos_max = size();

	int pos_base = 0;
	int pos_rel = 0;

	int lines_separator = 0;
	for(iterator i=begin();i!=end();++i) {
		if (i->line_separator_get())
			lines_separator++;
	}

	int dy = (title != "" ? int_font_dy_get()+border+1 : 0) + pos_rel_max * int_font_dy_get() + lines_separator * 3;

	int_box(x-border, y-border, dx+2*border, dy+border*2, 1, COLOR_CHOICE_NORMAL.foreground);
	int_clear(x-border+1, y-border+1, dx+2*border-2, dy+border*2-2, COLOR_CHOICE_NORMAL.background);
	if (title != "") {
		int_clear(x-border+1, y+int_font_dy_get()+border/2+1, dx+2*border-2, 1, COLOR_CHOICE_NORMAL.foreground);
		int_put_filled(x, y, dx, title, COLOR_CHOICE_TITLE);
		y += int_font_dy_get() + border+1;
	}

	unsigned count = 0;
	for(iterator i=begin();i!=end();++i) {
		if (i->state_get() == 2 && i->bistate_get()) {
			++count;
		}
	}
	// unselect a single element but set the cursor on it
	if (count == 1) {
		for(iterator i=begin();i!=end();++i) {
			if (i->state_get() == 2 && i->bistate_get()) {
				pos = i;
				pos->bistate_set(false);
			}
		}
	}

	pos_rel = pos - begin();
	if (pos_rel >= pos_rel_max) {
		pos_base = pos_rel - pos_rel_max + 1;
		pos_rel = pos_rel_max - 1;
	}

	while (!done) {
		draw(title, x, y, dx, pos_base, pos_rel, pos_rel_max, lines_separator);

		key = int_event_get();

		key = menu_key(key, pos_base, pos_rel, pos_rel_max, pos_base_upper, 1, pos_max);

		switch (key) {
			case EVENT_DEL :
				for(iterator i=begin();i!=end();++i) {
					switch (i->state_get()) {
					case 2 : i->bistate_set(false); break;
					case 3 : i->tristate_set(exclude); break;
					}
				}
			break;
			case EVENT_INS :
				for(iterator i=begin();i!=end();++i) {
					switch (i->state_get()) {
					case 2 : i->bistate_set(true); break;
					case 3 : i->tristate_set(include); break;
					}
				}
			break;
			case EVENT_SPACE :
				pos = begin() + pos_base + pos_rel;
				switch (pos->state_get()) {
				case 2 :
					pos->bistate_set(!pos->bistate_get());
					break;
				case 3 :
					switch (pos->tristate_get()) {
					case include : pos->tristate_set(exclude); break;
					case exclude : pos->tristate_set(exclude_not); break;
					case exclude_not : pos->tristate_set(include); break;
					}
					break;
				}
				key = int_event_clear();
				break;
			case EVENT_ENTER :
				pos = begin() + pos_base + pos_rel;
				if (pos->active_get()) {
					unsigned count = 0;
					for(iterator i=begin();i!=end();++i) {
						if (i->state_get() == 2 && i->bistate_get()) {
							++count;
						}
					}
					// select a single element if none is selected
					if (count == 0) {
						pos->bistate_set(true);
					}
					done = 1;
				}
				break;
			case EVENT_ESC :
			case EVENT_SETCOIN :
			case EVENT_ESCEMULE:
			case EVENT_MENU :
				done = 1;
				break;
		}
	}

	pos = begin() + pos_base + pos_rel;

	return key;
}

// -------------------------------------
// favorites

int choice_bag::favorites_run(const string& title, int x, int y, int dx, choice_container::iterator& pos)
{
	int key = EVENT_ESC;
	int done = 0;
	int border = int_font_dx_get() / 2 + 1;

	if (x < 0)
		x = (int_dx_get() - dx - border * 2) / 2;
	if (y < 0)
		y = (int_dy_get() - (size() + 1) * int_font_dy_get() - border * 2) / 2;

	int pos_rel_max = (int_dy_get() - y) / int_font_dy_get();
	pos_rel_max -= 2;
	if (pos_rel_max > size())
		pos_rel_max = size();
	int pos_base_upper = size() - pos_rel_max;
	int pos_max = size();

	int pos_base = 0;
	int pos_rel = 0;

	int lines_separator = 0;
	for(iterator i=begin();i!=end();++i) {
		if (i->line_separator_get())
			lines_separator++;
	}

	int dy = (pos_rel_max+1) * int_font_dy_get() + border + 1 + lines_separator * 3;

	int_box(x-border, y-border, dx+2*border, dy+border*2, 1, COLOR_CHOICE_NORMAL.foreground);
	int_clear(x-border+1, y-border+1, dx+2*border-2, dy+border*2-2, COLOR_CHOICE_NORMAL.background);
	int_clear(x-border+1, y+int_font_dy_get()+border/2+1, dx+2*border-2, 1, COLOR_CHOICE_NORMAL.foreground);
	int_put_filled(x, y, dx, title, COLOR_CHOICE_TITLE);
	y += int_font_dy_get() + border+1;
	
	unsigned count = 0;
	for(iterator i=begin();i!=end();++i) {
		if (i->state_get() == 2 && i->bistate_get()) {
			++count;
		}
	}

	pos_rel = pos - begin();
	if (pos_rel >= pos_rel_max) {
		pos_base = pos_rel - pos_rel_max + 1;
		pos_rel = pos_rel_max - 1;
	}

	while (!done) {
		draw(title, x, y, dx, pos_base, pos_rel, pos_rel_max, lines_separator);

		key = int_event_get();

		key = menu_key(key, pos_base, pos_rel, pos_rel_max, pos_base_upper, 1, pos_max);

		switch (key) {
			case EVENT_DEL :
				for(iterator i=begin();i!=end();++i) {
					i->bistate_set(false);
				}
			break;
			case EVENT_INS :
				for(iterator i=begin();i!=end();++i) {
					i->bistate_set(true);
				}
			break;
			case EVENT_SPACE :
				pos = begin() + pos_base + pos_rel;
				pos->bistate_set(!pos->bistate_get());
				key = int_event_clear();
				break;
			case EVENT_ENTER :
				done = 1;
				break;
			case EVENT_ESC :
			case EVENT_ESCEMULE:
			case EVENT_MENU :
				done = 1;
				break;
		}
	}

	pos = begin() + pos_base + pos_rel;

	return key;
}

void choice_bag::insert_line() {
	choice_bag::iterator i = end();
	if (i != begin())
		(i-1)->line_separator_set();
}

choice_container::iterator choice_bag::find_by_value(int value)
{
	choice_container::iterator i = begin();
	while (i != end()) {
		if (i->value_get() == value)
			return i;
		++i;
	}
	return i;
}

choice_container::iterator choice_bag::find_by_desc(const string& desc)
{
	choice_container::iterator i = begin();
	while (i != end()) {
		if (i->desc_get() == desc)
			return i;
		++i;
	}
	return i;
}

void menu_pos(int pos, int& pos_base, int& pos_rel, int pos_rel_max, int pos_base_upper, int coln, int pos_max)
{

	while (pos >= pos_base_upper + pos_rel_max) 
		pos -= coln;
	while (pos < 0)
		pos += coln;

	if (pos >= pos_max) { 
		pos_base = pos_base_upper;
		pos_rel = pos_rel_max - 1;
		if (pos_base + pos_rel >= pos_max) { 
			pos_rel = pos_max - 1 - pos_base;
			if (pos_rel < 0) { 
				pos_base = 0;
				pos_rel = pos_max ? pos_max - 1 : 0;
			}
		}
		return;
	}

	if (pos >= pos_base && pos < pos_base + pos_rel_max) { 
		pos_rel = pos - pos_base;
	} else if (pos < pos_base) { 
		pos_rel = pos % coln;
		pos_base = pos - pos_rel;
	} else { 
		pos_rel = pos % coln + pos_rel_max - coln;
		pos_base = pos - pos_rel;
		if (pos_base < 0) { 
			pos_base = 0;
			pos_rel = pos;
		}
	}
}

int menu_key(int key, int& pos_base, int& pos_rel, int pos_rel_max, int pos_base_upper, int coln, int pos_max)
{
	switch (key) {
		case EVENT_HOME :
			menu_pos(0, pos_base, pos_rel, pos_rel_max, pos_base_upper, coln, pos_max);
			break;
		case EVENT_END :
			menu_pos(pos_max, pos_base, pos_rel, pos_rel_max, pos_base_upper, coln, pos_max);
			break;
		case EVENT_LEFT :
			if (coln > 1) {
				menu_pos(pos_base + pos_rel - 1, pos_base, pos_rel, pos_rel_max, pos_base_upper, coln, pos_max);
				break;
			}
			// otherwise continue
		case EVENT_PGUP :
			if (pos_base >= pos_rel_max) {
				pos_base -= pos_rel_max;
			} else if (pos_base>0) {
				pos_base = 0;
			} else {
				pos_rel = 0;
			}
			break;
		case EVENT_RIGHT :
			if (coln > 1) {
				menu_pos(pos_base + pos_rel + 1, pos_base, pos_rel, pos_rel_max, pos_base_upper, coln, pos_max);
				break;
			}
			// otherwise continue
		case EVENT_PGDN :
			if (pos_base + pos_rel_max <= pos_base_upper) {
				pos_base += pos_rel_max;
			} else if (pos_base < pos_base_upper) {
				pos_base = pos_base_upper;
			} else {
				if (pos_max >= pos_base + 1)
					pos_rel = pos_max - pos_base - 1;
				else
					pos_rel = 0;
			}
			break;
		case EVENT_UP :
			menu_pos(pos_base + pos_rel - coln, pos_base, pos_rel, pos_rel_max, pos_base_upper, coln, pos_max);
			break;
		case EVENT_DOWN :
			menu_pos(pos_base + pos_rel + coln, pos_base, pos_rel, pos_rel_max, pos_base_upper, coln, pos_max);
			break;
		default:
			return key;
	}

	return EVENT_NONE;
}

int menu_key_custom(int key, int& pos_base, int& pos_rel, int pos_rel_max, int pos_base_upper, int coln, int pos_max)
{
	switch (key) {
		case EVENT_HOME :
			pos_base = -pos_rel;
			break;
		case EVENT_END :
			pos_base = pos_max - pos_rel - 1;
			break;
		case EVENT_LEFT :
			if (coln > 1) {
				if (pos_base + pos_rel - 1 <= 0)
					pos_base = -pos_rel;
				else
					pos_base--;
				break;
			}
			// otherwise continue
		case EVENT_PGUP :
			if (pos_base >= pos_rel_max)
				pos_base -= pos_rel_max;
			else
				pos_base = -pos_rel;
			break;
		case EVENT_RIGHT :
			if (coln > 1) {
				if (pos_base + pos_rel + 1 >= pos_max)
					pos_base = pos_max - pos_rel - 1;
				else
					pos_base++;
				break;
			}
			// otherwise continue
		case EVENT_PGDN :
			if (pos_base + pos_rel_max <= pos_max - pos_rel)
				pos_base += pos_rel_max;
			else
				pos_base = pos_max - pos_rel - 1;
			break;
		case EVENT_UP :
			if (pos_base + pos_rel - coln <= 0)
				pos_base = -pos_rel;
			else
				pos_base -= coln;
			break;
		case EVENT_DOWN :
			if (pos_base + pos_rel + coln >= pos_max)
				pos_base = pos_max - pos_rel - 1;
			else
				pos_base += coln;
			break;
		default:
			return key;
	}

	return EVENT_NONE;
}




