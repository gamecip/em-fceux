/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2015 Valtteri "tsone" Heikkila
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
#include "../../fceu.h"
#include "../../drawing.h"


extern uint8 *XBuf;


typedef struct t_SplashRow
{
	uint8 x, y;
	const char* text;
} SplashRow;

typedef struct t_SplashPage
{
	const SplashRow* rows;
	int numRows;
	int delay;
} SplashPage;

static int s_splashFC = 0;
static int s_splashPage = 0;

static const SplashRow s_splashPageRows0[] =
{
	{  8*8, 12*8, "WELCOME TO EM-FCEUX" },
};
static const SplashRow s_splashPageRows1[] =
{
	{  3*8,  8*8, "TO PLAY A GAME, DRAG AND DROP A ROM" },
	{  4*8, 10*8, "FILE (.NES OR .ZIP) IN THIS WINDOW." },
	{  4*8, 13*8, "TO PLAY IT AGAIN, CHOOSE IT FROM" },
	{  5*8, 15*8, "THE GAME STACK ON THE RIGHT -->" },
};
static const SplashRow s_splashPageRows2[] =
{
	{  3*8, 10*8, "<-- TO OPEN SETTINGS TAB, CLICK THE" },
	{  8*8, 12*8, "TRIANGLE ON THE LEFT." },
};
static const SplashRow s_splashPageRows3[] =
{
	{  3*8, 10*8, "FOR MORE HELP, CLICK '?' ICON IN THE" },
	{  4*8, 12*8, "TOP RIGHT CORNER OF THIS WINDOW." },
};
static const SplashPage s_splashPages[] =
{
	{ s_splashPageRows0, sizeof(s_splashPageRows0) / sizeof(*s_splashPageRows0),  2*60 },
	{ s_splashPageRows1, sizeof(s_splashPageRows1) / sizeof(*s_splashPageRows1), 13*60 },
	{ s_splashPageRows2, sizeof(s_splashPageRows2) / sizeof(*s_splashPageRows2),  7*60 },
	{ s_splashPageRows3, sizeof(s_splashPageRows3) / sizeof(*s_splashPageRows3),  8*60 },
};

static const int s_splashNumPages = sizeof(s_splashPages) / sizeof(*s_splashPages);

void DrawSplash()
{
	char buf[128];

	memset(XBuf, 0x1D1D1D1D, 256*256);

	const SplashPage *page = &s_splashPages[s_splashPage];
	if (s_splashFC > page->delay) {
		s_splashPage = (s_splashPage + 1) % s_splashNumPages;
		page = &s_splashPages[s_splashPage];
		s_splashFC = 0;
	}

	int numChars = s_splashFC / 2;
	for (int i = 0; i < page->numRows; ++i) {
		const SplashRow* row = &page->rows[i];
		uint8 *dst = XBuf + 256*row->y + row->x;

		int len = strlen(row->text);
		if (numChars >= len) {
			DrawTextTransWH(dst, (uint8*) row->text, 0x30, 256, 8, -1);
		} else {
			strncpy(buf, row->text, numChars);
			buf[numChars] = '_';
			buf[numChars + 1] = 0;
			DrawTextTransWH(dst, (uint8*) buf, 0x30, 256, 8, -1);
		}

		numChars -= len;
		if (numChars <= 0) {
			break;
		}
	}
	
	s_splashFC = (s_splashFC + 1);
}
