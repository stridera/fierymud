#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_MAPLINES 512
#define MAX_CODELINES 512

/* function prototypes */
int GetLine( char *, FILE *, int );
int WriteRoom( FILE *, int, int, int, int, FILE * );
char *LookupCode( char , int, int * );
char *GetField( char *, int , int * );



/* globals */
int countrooms = 1;
int grid, world, zone;
char *map[MAX_MAPLINES];	// to hold the map lines 
int maplines, maplinelength;
char *codes[MAX_CODELINES];	// to hold the code lines
int codelines;

int main( int argc, char *argv[] )
{
	FILE *mapfile, *codefile;

    /*	open input files  */
	if ( argc == 3 ) {
		mapfile = fopen( argv[1], "r" ); // open file for input
		codefile = fopen( argv[2], "r" ); // open file for input
	}
	else {
		printf( "Usage: mapgen mapfile codefile\n" );
		return 1;
	}

	if ( mapfile == NULL ) {
			printf(" unable to open file %s\n", argv[1] );
			return 1;
	}

	if ( codefile == NULL) {
			printf(" unable to open file %s\n", argv[2] );
			return 1;
	}


	/*******************/
	/* read in the map */
	/*******************/

	printf( "Reading mapfile...\n" );

	// read the grid size, world#, and zone#
	char temp[256];
	GetLine( temp, mapfile, 256 );
	if ( strncmp( temp, "GRID : ", 7 ) != 0 ) {
		printf( "Format error, line 1: Use GRID : gridsize\n" );
		return 1;
	}
	else {
		grid = atoi( temp + 7 );
	}

	GetLine( temp, mapfile, 256 );
	if ( strncmp( temp, "WORLD NO : ", 11 ) != 0 ) {
		printf( "Format error, line 2: Use WORLD NO :
worldnumber\n" );
		return 1;
	}
	else {
		world = atoi( temp + 11 );
	}

	GetLine( temp, mapfile, 256 );
	if ( strncmp( temp, "ZONE NO : ", 10 ) != 0 ) {
		printf( "Format error, line 3: Use ZONE NO : zonenumber\n"
);
		return 1;
	}
	else {
		zone = atoi( temp + 10 );
	}

	// read the map lines
	int linesread = 1;
	int rval;
	map[0] = new char [256];
	maplinelength = GetLine( map[0], mapfile, 256 );
	if ( maplinelength < 1 ) {
		printf( "Error, line 4: map width must be between 1 and
255 characters\n" );
		return 1;
	}

	for ( int i = 1; i < MAX_MAPLINES; i++ )
	{
		map[i] = new char [256];
		rval = GetLine( map[i], mapfile, 256 );

		if ( rval == -1 ) {
			// end of file
			printf( "%d map lines read\n", linesread );
			break;
		}
		
		if ( rval != maplinelength ) {
			// map line wrong length
			printf( "Error, line %d: map line is inconsistent
length\n", linesread + 4 );
			return 1;
		}

		++linesread;
	}

	if ( rval != -1 ) {
		printf( "Error: map is too long.  %d map lines read.\n",
linesread );
		return 1;
	}

	maplines = linesread;

	// check map can be broken up into grid
	if ( (maplinelength%grid)  || (maplines%grid) ) {
		printf( "Error: map size not divisible by grid size\n" );
		printf( "map size %dx%d\n", maplinelength, maplines );
		printf( "grid size %dx%d\n", grid, grid );
		return 1;
	}

	fclose( mapfile );

	// output the map file
	printf("GRID NO : %d\n", grid);
	printf("WORLD NO : %d\n", world);
	printf("ZONE NO : %d\n", zone);
	for ( i = 0; i < maplines; i++ ) {
		printf( "%s\n", map[i] );
	}

	/*********************/
	/* read in the codes */
	/*********************/

	printf( "\nReading codefile...\n" );

	linesread = 0;

	for ( i = 0; i < MAX_CODELINES; i++ ) 
	{
		codes[i] = new char [256];
		rval = GetLine( codes[i], codefile, 256 );

		if ( rval == -1 ) {
			// end of file
			printf( "%d code lines read\n", linesread );
			break;
		}
		
		if ( rval == -2 ) {
			// code line too long
			printf( "Error, line %d: code line too long, must
be < 256 chars\n", i + 1 );
			return 1;
		}

		++linesread;

	}

	if ( rval != -1  ) {
		printf( "Error: code file is too long, %d lines read\n",
linesread );
		return 1;
	}

	codelines = linesread;

	fclose( codefile );

	// output the codes
	for ( i = 0; i < codelines; i++ ) {
		printf( "%s\n", codes[i] );
	}

	/***********************/
	/* make the .wld files */
	/***********************/

	int numwldfiles = (maplinelength/grid)*(maplines/grid);
	int numwldfilerooms = grid*grid;
	int xoffset, yoffset;
	int room, wld;

	xoffset = 0;	// offset of current room relative to original map
	yoffset = 0;
	room = world;
	
	FILE *gridfile;
	gridfile = fopen( "vnums.map", "w" ); //open vnum file

	for ( int wldfile = 0; wldfile < numwldfiles; wldfile++ ) 
	{
		// do each wild file

		FILE *outfile;
		char wldname[256];
		wld = wldfile + zone;
		sprintf( wldname, "%d.wld", wld);
		outfile = fopen( wldname, "w" ); // open file for writing
		
		for ( int wldfileroom = 0; wldfileroom < numwldfilerooms;
wldfileroom++ )
		{
			// do each room
			WriteRoom( outfile, xoffset, yoffset, room,
wldfileroom, gridfile );
			++room;
		}

		// write the terminating $
		fprintf( outfile, "$\n$" );
		fclose( outfile );
		

		xoffset = (xoffset+grid)%maplinelength;
		if ( xoffset == 0 ) yoffset += grid;
	}
	fclose(gridfile);
	return 0;
}

int WriteRoom( FILE *outfile, int xoff, int yoff, int room, int roomoff,
FILE *gridfile )
{
	long numvector;
	/* write the room number */
	fprintf( gridfile, "%d ", room );
	fprintf( outfile, "#%d\n", room );
	
	// calculate location of current room, relative to little map
	int Xpos = roomoff%grid;
	int Ypos = roomoff/grid;

	if (countrooms == maplinelength) {
		fprintf( gridfile, "\n");
		countrooms = 1;
	}
	else
		countrooms++;

	/* write the room name */
	char *temp, description[256];
	int length;
	char symbol = map[yoff + Ypos][xoff + Xpos];
	temp = LookupCode( symbol, 1, &length );
	strncpy( description, temp, length ); 
	description[length] = '\0'; // add termination character
	fprintf( outfile, "%s~\n", description );

	/* write the map */
	/*
	char asciicode[256];
	for (int y = 0; y < grid; y++) {
		// do a row

		for (int x = 0; x < grid; x++) {

			int length;

			if ((y == Ypos) && (x == Xpos)) 
				fprintf( outfile, "&00&17&11@&00" );
			else {
				symbol = map[yoff + y][xoff + x];
				temp = LookupCode( symbol, 0, &length );

				// debug - just print char
				//fprintf( outfile, "%c", symbol );

				// write ascii code

				strncpy( asciicode, temp, length ); 
				asciicode[length] = '\0'; // add
termination character
				fprintf( outfile, "%s", asciicode );
			}
		}

		fprintf( outfile, "\n");
	}
	*/
	fprintf( outfile, " \n");
	fprintf( outfile, "~\n" );

	/* write the zone, bitvector and sector type */
	fprintf( outfile, "%d", zone );

	char bitvector[256];
	temp = LookupCode( symbol, 2, &length );
	strncpy( bitvector, temp, length ); 
	bitvector[length] = '\0'; // add termination character
	numvector = atol(bitvector);
	numvector += 65536L;
	ltoa(numvector, bitvector, 10);
	fprintf( outfile, " %s", bitvector);

	char sectortype[256];
	temp = LookupCode( symbol, 3, &length );
	strncpy( sectortype, temp, length ); 
	sectortype[length] = '\0'; // add termination character
	fprintf( outfile, " %s", sectortype );


	fprintf( outfile, "\n" );

	/* write the directions/exits out of room */

	// North
	if ( (yoff + Ypos) > 0 ) {
		fprintf( outfile, "D0\n" );
		symbol = map[yoff + Ypos - 1][xoff + Xpos];
		temp = LookupCode( symbol, 1, &length );

		// write description

		strncpy( description, temp, length ); 
		description[length] = '\0'; // add termination character
		fprintf( outfile, "%s\n", description );

		// separation chars
		fprintf( outfile, "~\n~\n" );

		// calculate where it goes
	
		int nextroom;
		if ( Ypos > 0 ) 
			nextroom = room - grid;
		else {
			//~~BUG Had to put in the + 400 to make map work
properly!
			nextroom = room - (grid * grid) * (grid - 1) -
grid + 400;
		}

		fprintf( outfile, "%d %d %d\n", 0, -1, nextroom );

	}

	// East
	if ( (xoff + Xpos + 1) < maplinelength ) {
		fprintf( outfile, "D1\n" );
		symbol = map[yoff + Ypos][xoff + Xpos + 1];
		temp = LookupCode( symbol, 1, &length );

		// write description

		strncpy( description, temp, length ); 
		description[length] = '\0'; // add termination character
		fprintf( outfile, "%s\n", description );

		// separation chars
		fprintf( outfile, "~\n~\n" );

		// calculate where it goes

		int nextroom;
		if ( Xpos + 1 < grid )
			nextroom = room + 1;
		else {
			nextroom = room + grid * (grid - 1) + 1;
		}

		fprintf( outfile, "%d %d %d\n", 0, -1, nextroom );
	}

	// South
	if ( (yoff + Ypos) < maplines - 1 ) {
		fprintf( outfile, "D2\n" );
		symbol = map[yoff + Ypos + 1][xoff + Xpos];
		temp = LookupCode( symbol, 1, &length );

		// write description

		strncpy( description, temp, length ); 
		description[length] = '\0'; // add termination character
		fprintf( outfile, "%s\n", description );

		// separation chars
		fprintf( outfile, "~\n~\n" );

		// calculate where it goes

		int nextroom;
		if ( Ypos + 1 < grid ) 
			nextroom = room + grid;
		else {
			//~~BUG Had to put in the - 400 to make map work
properly!
			nextroom = room + (grid * grid) * (grid - 1) +
grid - 400; 
		}

		fprintf( outfile, "%d %d %d\n", 0, -1, nextroom );
	}

	// West
	if ( (xoff + Xpos - 1) >= 0 ) {
		fprintf( outfile, "D3\n" );
		symbol = map[yoff + Ypos][xoff + Xpos - 1];
		temp = LookupCode( symbol, 1, &length );

		// write description

		strncpy( description, temp, length ); 
		description[length] = '\0'; // add termination character
		fprintf( outfile, "%s\n", description );

		// separation chars
		fprintf( outfile, "~\n~\n" );

		// calculate where it goes

		int nextroom;
		if ( Xpos > 0 )
			nextroom = room - 1;
		else {
			nextroom = room - grid * (grid - 1) - 1;
		}

		fprintf( outfile, "%d %d %d\n", 0, -1, nextroom );
	}

	// write room termination character
	fprintf( outfile, "S\n" );

	return 0;
}

char *LookupCode( char code, int field, int *length )
{
	char *rval;

	rval = NULL;
	for ( int i = 0; i < codelines; i++ ) {
		if ( codes[i][0] == code ) {
			// found it

			rval = GetField( codes[i], field, length );
			break;
		}
	}

	if ( rval == NULL ) {
		printf( "ERROR: LookupCode() - could not find code %c,
field %d\n", code, field );
		exit(1);
	}

	
	return rval;
}

char *GetField( char *string, int field, int *length )
{
	// each field consists of a string enclosed in '
	// eg. 'string'
	// fields begin with field 0

	char *ptr1, *ptr2;
	int i;

	// find start of field
	ptr1 = string;
	for ( i = 0; i < field; i++ ) {
		while ( (*ptr1 != '\'') && (*ptr1 != '\0') ) ++ptr1;
		if (*ptr1 == '\'') {
			// foung start of a field, move to end
			++ptr1;
			while ( (*ptr1 != '\'') && (*ptr1 != '\0') )
++ptr1;
			if ( *ptr1 == '\'' ) ++ptr1; // end of field
		}
	}

	// now pointing to just after end of field - 1

	// move to start of field
	while ( (*ptr1 != '\'') && (*ptr1 != '\0') ) ++ptr1;
	if (*ptr1 != '\0') ++ptr1; // move past leading '

	// find end of field
	if (*ptr1 != '\0') {
		ptr2 = ptr1;
		while ((*ptr2 != '\'') && (*ptr2 != '\0')) ++ptr2;
		//*ptr2 = '\0';
	}

	// calc length of field
	*length = ptr2 - ptr1;

	if ( (*ptr1 == '\0') || (*ptr2 == '\0') ) 
	{
		return NULL;	// field not found
	}
	else 
		return ptr1;

}

// globals
//int grid, world, zone;
//char *map[MAX_MAPLINES];	// to hold the map lines 
//int maplines, maplinelength;
//char *codes[MAX_CODELINES];	// to hold the code lines
//int codelines;

/*******************************************************************/
/* file I/O                                                        */
/*******************************************************************/

int GetLine(char *line, FILE *stream, int linelength)
{
	// read in a line, lines longer than linelength chars (includeing
\n) are split
	int temp, i;

	i = 0;
	temp = fgetc(stream);

	while ((temp != '\n') && (temp != EOF) && (i < linelength)) {
		line[i++] = temp;
		temp = fgetc(stream);
	}

	line[i] = '\0';

	if ((feof(stream) != 0) && (i == 0))
		return -1;	// end of file, no lines read
	else if (i > linelength)
		return -2;	// line to long
	else
		return i;	// return number of characters read
}
