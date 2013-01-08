/*
    gen-info - a utility for extracting header info from Sega Genesis / Mega Drive ROMs
    copyright (C) 2004, 2005, Utkan Güngördü <utkan@freeconsole.org>


    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>


#define PACKAGE "gen-info"
#define VERSION "0.2.1"


static void usage()
{
	fprintf(stdout, "%s %s (%s)\n", PACKAGE, VERSION, __DATE__);
	fprintf(stdout, "a utility for extracting header info from SEGA Genesis/MD roms\n\n");

	fprintf(stdout, "usage: %s <romlist...>\n\n", PACKAGE);
	fprintf(stdout, "if no input file is given, %s will try stdin\n", PACKAGE);

	fprintf(stdout, "You may redistribute copies of this program\n");
	fprintf(stdout, "under the terms of the GNU General Public License.\n");
	fprintf(stdout, "For more information about these matters, see the file named COPYING.\n");
	fprintf(stdout, "Report bugs to <bug@freeconsole.org>.\n");
}


/* formatted text output. when len==0, b is assumed to be a null-terminated ascii string */
static void write_info(const char *s, size_t len, const char *b)
{
	if(!len) len = strlen(b);

	fprintf(stdout, "%-20s: ", s);
	fwrite(b, len, 1, stdout);
	fprintf(stdout, "\n");
}


static unsigned long read_msb_long(const unsigned char *p)
{
	unsigned long l;

	l =	((unsigned long)(p[0]))<<24 |
		((unsigned long)(p[1]))<<16 |
		((unsigned long)(p[2]))<<8  |
		((unsigned long)(p[3]));

	return l;
}


#ifndef __STRICT_ANSI__
inline
#endif
static char *itox(int n, char *s)
{
	sprintf(s, "0x%x", n);
	return s;
}


static int treat_file(const char *file)
{
	unsigned char header[0x200], *p;
	char s[0x200]; /* temporary variables */
	unsigned long checksum;
	FILE *fp;
	int i, c;

	if(!file) /* read from stdin */
	{
		for(i=0; i<0x200 && c != EOF; i++) header[i] = c = fgetc(stdin);
	}
	else
	{
		fp = fopen(file, "rb");
	
		if(fp == NULL)
		{
			fprintf(stderr, "couldn't open %s for reading: %s\n", file, strerror(errno));
			return 1;
		}
		
		fread(header, 0x200, 1, fp);
		fclose(fp);
	}

	write_info("system", 0x10, &header[0x100]);
	write_info("copyright", 0x10, &header[0x110]);
	write_info("name (domestic)", 0x30, &header[0x120]);
	write_info("name (overseas)", 0x30, &header[0x150]);

	/* type */
	if(!strncmp(&header[0x180], "GM", 2))
		write_info("type", 0, "game");
	else if(!strncmp(&header[0x180], "Al", 2))
		write_info("type", 0, "education");
	else
	{
		sprintf(s, "unknown (%c%c)", header[0x180], header[0x181]);
		write_info("type", 0, s);
	}

	write_info("product code", 0xb, &header[0x183]);

	/* checksum */
	checksum =	read_msb_long(&header[0x18e]) >> 16;

	sprintf(s, "0x%lx (%lu)", checksum, checksum);
	write_info("checksum", 0, s);

	p = &header[0x190];
	*s = '\0';
	while(p < &header[0x1a0])
	{
		switch(*p++)
		{
			case '0':
				strcat(s, "sms_joypad ");
				break;

			case '4':
				strcat(s, "team_play ");
				break;

			case '6':
				strcat(s, "6_button_joypad ");
				break;

			case 'J':
				strcat(s, "joypad ");
				break;

			case 'K':
				strcat(s, "keyboard ");
				break;

			case 'R':
				strcat(s, "serial(rs232c) ");
				break;

			case 'P':
				strcat(s, "printer ");
				break;

			case 'T':
				strcat(s, "tablet ");
				break;

			case 'B':
				strcat(s, "control_ball ");
				break;

			case 'V':
				strcat(s, "paddle ");
				break;

			case 'F':
				strcat(s, "fdd ");
				break;

			case 'C':
				strcat(s, "cd-rom ");
				break;

			case 'M':
				strcat(s, "mega_mouse ");
				break;

			case 'L':
				strcat(s, "activator ");
				break;
			
			case ' ':
				break;

			default:
				sprintf(s, "%s %c(?)", s, *(p-1));
		}
	}

	write_info("controller flags", 0, s);

	write_info("rom start address", 0, itox(read_msb_long(&header[0x1a0]), s));
	write_info("rom end address", 0, itox(read_msb_long(&header[0x1a4]), s));

	write_info("ram start address", 0, itox(read_msb_long(&header[0x1a8]), s));
	write_info("ram end address", 0, itox(read_msb_long(&header[0x1ac]), s));

	if(strncmp(&header[0x1b0], "RA", 2) || (header[0x1b2] & 0xa0)==0 || header[0x1b3] != 0x20)
		strcpy(s, "no sram either incorrect info");
	else
	{
		switch((header[0x1b2] & 0x18)>>3)
		{
			case 0:
				strcpy(s,"even_and_odd_adr");
			break;

			case 2:
				strcpy(s,"even_adr_only");
			break;

			case 3:
				strcpy(s,"odd_adr_only");
			break;
		}
	}

	write_info("sram flags", 0, s);

	write_info("sram start address", 0, itox(read_msb_long(&header[0x1b4]), s));
	write_info("sram end address", 0, itox(read_msb_long(&header[0x1b8]), s));

	if(strncmp(&header[0x1bc], "MO", 2))
		write_info("modem", 0, "no modem either incorrect info");
	else
	{
		write_info("modem firm", 4, &header[0x1be]);
		write_info("modem version", 4, &header[0x1c2]);
	}

	write_info("memo(?)", 40, &header[0x1c8]);

	p = &header[0x1f0];
	*s = '\0';
	while(p <= &header[0x1ff])
	{
		switch(*p++)
		{
			case 'E':
				strcat(s, "europe ");
			break;

			case 'J':
				strcat(s, "japan ");
			break;

			case 'U':
				strcat(s, "usa ");
			break;

			case 'A':
				strcat(s, "asia ");
			break;

			case 'B':
			case '4':
				strcat(s, "brazil ");
			break;

			case 'F':
				strcat(s, "france ");
			break;

			case '8':
				strcat(s, "hong-kong ");
			break;
		}
	}

	write_info("countries", 0, s);
	
	fprintf(stdout, "\n");
	
	return 0;
}


int main(int argc, char *argv[])
{
	if(argc == 1)
	{
		/* no input files, try stdin */
		treat_file(0);
		exit(0);
	}
	else if(argc<2)
	{
		usage();
		exit(0);
	}

	while(--argc)
		treat_file(*(++argv));

	return 0;
}
