// The winbgim library, Version 6.0, August 9, 2004
// Written by:
//      Grant Macklem (Grant.Macklem@colorado.edu)
//      Gregory Schmelter (Gregory.Schmelter@colorado.edu)
//      Alan Schmidt (Alan.Schmidt@colorado.edu)
//      Ivan Stashak (Ivan.Stashak@colorado.edu)
//      Michael Main (Michael.Main@colorado.edu)
// CSCI 4830/7818: API Programming
// University of Colorado at Boulder, Spring 2003


// ---------------------------------------------------------------------------
//                          Notes
// ---------------------------------------------------------------------------
// * This library is still under development.
// * Please see http://www.cs.colorado.edu/~main/bgi for information on
// * using this library with the mingw32 g++ compiler.
// * This library only works with Windows API level 4.0 and higher (Windows 95, NT 4.0 and newer)
// * This library may not be compatible with 64-bit versions of Windows
// ---------------------------------------------------------------------------


// ---------------------------------------------------------------------------
//                          Macro Guard and Include Directives
// ---------------------------------------------------------------------------
#ifndef WINBGI_H
#define WINBGI_H
#include <windows.h>        // Provides the mouse message types
#include <limits.h>         // Provides INT_MAX
#include <sstream>          // Provides std::ostringstream
// ---------------------------------------------------------------------------



// ---------------------------------------------------------------------------
//                          Definitions
// ---------------------------------------------------------------------------
// Definitions for the key pad extended keys are added here.  When one
// of these keys are pressed, getch will return a zero followed by one
// of these values. This is the same way that it works in conio for
// dos applications.
#define KEY_HOME        71
#define KEY_UP          72
#define KEY_PGUP        73
#define KEY_LEFT        75
#define KEY_CENTER      76
#define KEY_RIGHT       77
#define KEY_END         79
#define KEY_DOWN        80
#define KEY_PGDN        81
#define KEY_INSERT      82
#define KEY_DELETE      83
#define KEY_F1          59
#define KEY_F2          60
#define KEY_F3          61
#define KEY_F4          62
#define KEY_F5          63
#define KEY_F6          64
#define KEY_F7          65
#define KEY_F8          66
#define KEY_F9          67

// Line thickness settings
#define NORM_WIDTH      1
#define THICK_WIDTH     3

// Character Size and Direction
#define USER_CHAR_SIZE  0
#define HORIZ_DIR       0
#define VERT_DIR        1


// Constants for closegraph
#define CURRENT_WINDOW -1
#define ALL_WINDOWS -2
#define NO_CURRENT_WINDOW -3

// The standard Borland 16 colors
#define MAXCOLORS       15
enum colors { BLACK, BLUE, GREEN, CYAN, RED, MAGENTA, BROWN, LIGHTGRAY, DARKGRAY,
              LIGHTBLUE, LIGHTGREEN, LIGHTCYAN, LIGHTRED, LIGHTMAGENTA, YELLOW, WHITE };

// The standard line styles
enum line_styles { SOLID_LINE, DOTTED_LINE, CENTER_LINE, DASHED_LINE, USERBIT_LINE };

// The standard fill styles
enum fill_styles { EMPTY_FILL, SOLID_FILL, LINE_FILL, LTSLASH_FILL, SLASH_FILL,
                   BKSLASH_FILL, LTBKSLASH_FILL, HATCH_FILL, XHATCH_FILL, INTERLEAVE_FILL,
                   WIDE_DOT_FILL, CLOSE_DOT_FILL, USER_FILL };

// The various graphics drivers
enum graphics_drivers { DETECT, CGA, MCGA, EGA, EGA64, EGAMONO, IBM8514, HERCMONO,
                        ATT400, VGA, PC3270 };

// Various modes for each graphics driver
enum graphics_modes { CGAC0, CGAC1, CGAC2, CGAC3, CGAHI, 
                      MCGAC0 = 0, MCGAC1, MCGAC2, MCGAC3, MCGAMED, MCGAHI,
                      EGALO = 0, EGAHI,
                      EGA64LO = 0, EGA64HI,
                      EGAMONOHI = 3,
                      HERCMONOHI = 0,
                      ATT400C0 = 0, ATT400C1, ATT400C2, ATT400C3, ATT400MED, ATT400HI,
                      VGALO = 0, VGAMED, VGAHI,
                      PC3270HI = 0,
                      IBM8514LO = 0, IBM8514HI };

// Borland error messages for the graphics window.
#define NO_CLICK        -1      // No mouse event of the current type in getmouseclick
enum graph_errors { grInvalidVersion = -18, grInvalidDeviceNum = -15, grInvalidFontNum,
                    grInvalidFont, grIOerror, grError, grInvalidMode, grNoFontMem,
                    grFontNotFound, grNoFloodMem, grNoScanMem, grNoLoadMem,
                    grInvalidDriver, grFileNotFound, grNotDetected, grNoInitGraph,
                    grOk };

// Write modes
enum putimage_ops{ COPY_PUT, XOR_PUT, OR_PUT, AND_PUT, NOT_PUT };

// Text Modes
enum horiz { LEFT_TEXT, CENTER_TEXT, RIGHT_TEXT };
enum vertical { BOTTOM_TEXT, VCENTER_TEXT, TOP_TEXT }; // middle not needed other than as seperator
enum font_names { DEFAULT_FONT, TRIPLEX_FONT, SMALL_FONT, SANS_SERIF_FONT,
             GOTHIC_FONT, SCRIPT_FONT, SIMPLEX_FONT, TRIPLEX_SCR_FONT,
			 COMPLEX_FONT, EUROPEAN_FONT, BOLD_FONT };
// ---------------------------------------------------------------------------



// ---------------------------------------------------------------------------
//                              Structures
// ---------------------------------------------------------------------------
// This structure records information about the last call to arc.  It is used
// by getarccoords to get the location of the endpoints of the arc.
struct arccoordstype
{
    int x, y;                   // Center point of the arc
    int xstart, ystart;         // The starting position of the arc
    int xend, yend;             // The ending position of the arc.
};


// This structure defines the fill style for the current window.  Pattern is
// one of the system patterns such as SOLID_FILL.  Color is the color to
// fill with
struct fillsettingstype
{
    int pattern;                // Current fill pattern
    int color;                  // Current fill color
};


// This structure records information about the current line style.
// linestyle is one of the line styles such as SOLID_LINE, upattern is a
// 16-bit pattern for user defined lines, and thickness is the width of the
// line in pixels.
struct linesettingstype
{
    int linestyle;              // Current line style
    unsigned upattern;          // 16-bit user line pattern
    int thickness;              // Width of the line in pixels
};


// This structure records information about the text settings.
struct textsettingstype
{
    int font;                   // The font in use
    int direction;              // Text direction
    int charsize;               // Character size
    int horiz;                  // Horizontal text justification
    int vert;                   // Vertical text justification
};


// This structure records information about the viewport
struct viewporttype
{
    int left, top,              // Viewport bounding box
        right, bottom;
    int clip;                   // Whether to clip image to viewport
};


// This structure records information about the palette.
struct palettetype
{
    unsigned char size;
    signed char colors[MAXCOLORS + 1];
};
// ---------------------------------------------------------------------------



// ---------------------------------------------------------------------------
//                          API Entries
// ---------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif

// Drawing Functions
void arc( int x, int y, int stangle, int endangle, int radius );
void bar( int left, int top, int right, int bottom );
void bar3d( int left, int top, int right, int bottom, int depth, int topflag );
void circle( int x, int y, int radius );
void cleardevice( );
void clearviewport( );
void drawpoly(int n_points, int* points);
void ellipse( int x, int y, int stangle, int endangle, int xradius, int yradius );
void fillellipse( int x, int y, int xradius, int yradius );
void fillpoly(int n_points, int* points);
void floodfill( int x, int y, int border );
void line( int x1, int y1, int x2, int y2 );
void linerel( int dx, int dy );
void lineto( int x, int y );
void pieslice( int x, int y, int stangle, int endangle, int radius );
void putpixel( int x, int y, int color );
void rectangle( int left, int top, int right, int bottom );
void sector( int x, int y, int stangle, int endangle, int xradius, int yradius );

// Miscellaneous Functions
int getdisplaycolor( int color );
int converttorgb( int color );
void delay( int msec );
void getarccoords( arccoordstype *arccoords );
int getbkcolor( );
int getcolor( );
void getfillpattern( char *pattern );
void getfillsettings( fillsettingstype *fillinfo );
void getlinesettings( linesettingstype *lineinfo );
int getmaxcolor( );
int getmaxheight( );
int getmaxwidth( );
int getmaxx( );
int getmaxy( );
bool getrefreshingbgi( );
int getwindowheight( );
int getwindowwidth( );
int getpixel( int x, int y );
void getviewsettings( viewporttype *viewport );
int getx( );
int gety( );
void moverel( int dx, int dy );
void moveto( int x, int y );
void refreshbgi(int left, int top, int right, int bottom);
void refreshallbgi( );    
void setbkcolor( int color );
void setcolor( int color );
void setfillpattern( char *upattern, int color );
void setfillstyle( int pattern, int color );
void setlinestyle( int linestyle, unsigned upattern, int thickness );
void setrefreshingbgi(bool value);
void setviewport( int left, int top, int right, int bottom, int clip );
void setwritemode( int mode );

// Window Creation / Graphics Manipulation
void closegraph( int wid=ALL_WINDOWS );
void detectgraph( int *graphdriver, int *graphmode );
void getaspectratio( int *xasp, int *yasp );
char *getdrivername( );
int getgraphmode( );
int getmaxmode( );
char *getmodename( int mode_number );
void getmoderange( int graphdriver, int *lomode, int *himode );
void graphdefaults( );
char *grapherrormsg( int errorcode );
int graphresult( );
void initgraph( int *graphdriver, int *graphmode, char *pathtodriver );
int initwindow
    ( int width, int height, const char* title="Windows BGI", int left=0, int top=0, bool dbflag=false, bool closeflag=true );
int installuserdriver( char *name, int *fp );    // Not available in WinBGI
int installuserfont( char *name );               // Not available in WinBGI
int registerbgidriver( void *driver );           // Not available in WinBGI
int registerbgifont( void *font );               // Not available in WinBGI
void restorecrtmode( );
void setaspectratio( int xasp, int yasp );
unsigned setgraphbufsize( unsigned bufsize );    // Not available in WinBGI
void setgraphmode( int mode );
void showerrorbox( const char *msg = NULL );

// User Interaction
int getch( );
int kbhit( );

// User-Controlled Window Functions (winbgi.cpp)
int getcurrentwindow( );
void setcurrentwindow( int window );
    
// Double buffering support (winbgi.cpp)
int getactivepage( );
int getvisualpage( );
void setactivepage( int page );
void setvisualpage( int page );
void swapbuffers( );

// Image Functions (drawing.cpp)
unsigned imagesize( int left, int top, int right, int bottom );
void getimage( int left, int top, int right, int bottom, void *bitmap );
void putimage( int left, int top, void *bitmap, int op );
void printimage(
    const char* title=NULL,	
    double width_inches=7, double border_left_inches=0.75, double border_top_inches=0.75,
    int left=0, int top=0, int right=INT_MAX, int bottom=INT_MAX,
    bool active=true, HWND hwnd=NULL
    );
void readimagefile(
    const char* filename=NULL,
    int left=0, int top=0, int right=INT_MAX, int bottom=INT_MAX
    );
void writeimagefile(
    const char* filename=NULL,
    int left=0, int top=0, int right=INT_MAX, int bottom=INT_MAX,
    bool active=true, HWND hwnd=NULL
    );

// Text Functions (text.cpp)
void gettextsettings(struct textsettingstype *texttypeinfo);
void outtext(char *textstring);
void outtextxy(int x, int y, char *textstring);
void settextjustify(int horiz, int vert);
void settextstyle(int font, int direction, int charsize);
void setusercharsize(int multx, int divx, int multy, int divy);
int textheight(char *textstring);
int textwidth(char *textstring);
extern std::ostringstream bgiout;    
void outstream(std::ostringstream& out=bgiout);
void outstreamxy(int x, int y, std::ostringstream& out=bgiout);    
    
// Mouse Functions (mouse.cpp)
void clearmouseclick( int kind );
void clearresizeevent( );
void getmouseclick( int kind, int& x, int& y );
bool ismouseclick( int kind );
bool isresizeevent( );
int mousex( );
int mousey( );
void registermousehandler( int kind, void h( int, int ) );
void setmousequeuestatus( int kind, bool status=true );

// Palette Functions
palettetype *getdefaultpalette( );
void getpalette( palettetype *palette );
int getpalettesize( );
void setallpalette( palettetype *palette );
void setpalette( int colornum, int color );
void setrgbpalette( int colornum, int red, int green, int blue );

// Color Macros
#define IS_BGI_COLOR(v)     ( ((v) >= 0) && ((v) < 16) )
#define IS_RGB_COLOR(v)     ( (v) & 0x03000000 )
#define RED_VALUE(v)        int(GetRValue( converttorgb(v) ))
#define GREEN_VALUE(v)      int(GetGValue( converttorgb(v) ))
#define BLUE_VALUE(v)       int(GetBValue( converttorgb(v) ))
#undef COLOR
int COLOR(int r, int g, int b); // No longer a macro

#ifdef __cplusplus
}
#endif
// ---------------------------------------------------------------------------

#endif // WINBGI_H

/*    PortTool v2.2     dibapi.h          */

/*
 *  dibapi.h
 *
 *  ** Win32 Version **
 *
 *  Copyright (C) 1991-1996 Microsoft Corporation. All rights reserved
 *
 *  Header file for Device-Independent Bitmap (DIB) API.  Provides
 *  function prototypes and constants for the following functions:
 *
 *  BitmapToDIB()        - Creates a DIB from a bitmap
 *  ChangeBitmapFormat() - Changes a bitmap to a specified DIB format
 *  ChangeDIBFormat()    - Changes a DIB's BPP and/or compression format
 *  CopyScreenToBitmap() - Copies entire screen to a standard Bitmap
 *  CopyScreenToDIB()    - Copies entire screen to a DIB
 *  CopyWindowToBitmap() - Copies a window to a standard Bitmap
 *  CopyWindowToDIB()    - Copies a window to a DIB
 *  CreateDIBPalette()   - Creates a palette from a DIB
 *  CreateDIB()          - Creates a new DIB
 *  DestroyDIB()         - Deletes DIB when finished using it
 *  DIBError()           - Displays message box with error message
 *  DIBHeight()          - Gets the DIB height
 *  DIBNumColors()       - Calculates number of colors in the DIB's color table
 *  DIBToBitmap()        - Creates a bitmap from a DIB
 *  DIBWidth()           - Gets the DIB width
 *  FindDIBBits()        - Sets pointer to the DIB bits
 *  GetSystemPalette()   - Gets the current palette
 *  LoadDIB()            - Loads a DIB from a file
 *  PaintBitmap()        - Displays standard bitmap in the specified DC
 *  PaintDIB()           - Displays DIB in the specified DC
 *  PalEntriesOnDevice() - Gets the number of palette entries
 *  PaletteSize()        - Calculates the buffer size required by a palette
 *  PrintDIB()           - Prints the specified DIB
 *  PrintScreen()        - Prints the entire screen
 *  PrintWindow()        - Prints all or part of a window
 *  SaveDIB()            - Saves the specified dib in a file
 *
 * See the file DIBAPI.TXT for more information about these functions.
 *
 */


/* Handle to a DIB */
#define HDIB HANDLE


/* Print Area selection */
#define PW_WINDOW        1
#define PW_CLIENT        2


/* Print Options selection */
#define PW_BESTFIT       1
#define PW_STRETCHTOPAGE 2
#define PW_SCALE         3

/* DIB Macros*/

// WIDTHBYTES performs DWORD-aligning of DIB scanlines.  The "bits"
// parameter is the bit count for the scanline (biWidth * biBitCount),
// and this macro returns the number of DWORD-aligned bytes needed 
// to hold those bits.

#define WIDTHBYTES(bits)    (((bits) + 31) / 32 * 4)

/* Error constants */
enum {
      ERR_MIN = 0,                     // All error #s >= this value
      ERR_NOT_DIB = 0,                 // Tried to load a file, NOT a DIB!
      ERR_MEMORY,                      // Not enough memory!
      ERR_READ,                        // Error reading file!
      ERR_LOCK,                        // Error on a GlobalLock()!
      ERR_OPEN,                        // Error opening a file!
      ERR_CREATEPAL,                   // Error creating palette.
      ERR_GETDC,                       // Couldn't get a DC.
      ERR_CREATEDDB,                   // Error create a DDB.
      ERR_STRETCHBLT,                  // StretchBlt() returned failure.
      ERR_STRETCHDIBITS,               // StretchDIBits() returned failure.
      ERR_SETDIBITSTODEVICE,           // SetDIBitsToDevice() failed.
      ERR_STARTDOC,                    // Error calling StartDoc().
      ERR_NOGDIMODULE,                 // Couldn't find GDI module in memory.
      ERR_SETABORTPROC,                // Error calling SetAbortProc().
      ERR_STARTPAGE,                   // Error calling StartPage().
      ERR_NEWFRAME,                    // Error calling NEWFRAME escape.
      ERR_ENDPAGE,                     // Error calling EndPage().
      ERR_ENDDOC,                      // Error calling EndDoc().
      ERR_SETDIBITS,                   // Error calling SetDIBits().
      ERR_FILENOTFOUND,                // Error opening file in GetDib()
      ERR_INVALIDHANDLE,               // Invalid Handle
      ERR_DIBFUNCTION,                 // Error on call to DIB function
      ERR_MAX                          // All error #s < this value
     };



/* Function prototypes */

HDIB BitmapToDIB (HBITMAP hBitmap, HPALETTE hPal);
HDIB ChangeBitmapFormat (HBITMAP	hBitmap,
                                   WORD     wBitCount,
                                   DWORD    dwCompression,
                                   HPALETTE hPal);
HDIB ChangeDIBFormat (HDIB hDIB, WORD wBitCount,
                                DWORD dwCompression);
HBITMAP CopyScreenToBitmap (LPRECT);
HDIB CopyScreenToDIB (LPRECT);
HBITMAP CopyWindowToBitmap (HWND, WORD);
HDIB CopyWindowToDIB (HWND, WORD);
HPALETTE CreateDIBPalette (HDIB);
HDIB CreateDIB(DWORD, DWORD, WORD);
WORD DestroyDIB (HDIB);
void DIBError (int ErrNo);
DWORD DIBHeight (LPSTR lpDIB);
WORD DIBNumColors (LPSTR lpDIB);
HBITMAP DIBToBitmap (HDIB hDIB, HPALETTE hPal);
DWORD DIBWidth (LPSTR lpDIB);
LPSTR FindDIBBits (LPSTR lpDIB);
HPALETTE GetSystemPalette (void);
HDIB LoadDIB (const LPSTR);
BOOL PaintBitmap (HDC, LPRECT, HBITMAP, LPRECT, HPALETTE);
BOOL PaintDIB (HDC, LPRECT, HDIB, LPRECT, HPALETTE);
int PalEntriesOnDevice (HDC hDC);
WORD PaletteSize (LPSTR lpDIB);
WORD PrintDIB (HDIB, WORD, WORD, WORD, LPSTR);
WORD PrintScreen (LPRECT, WORD, WORD, WORD, LPSTR);
WORD PrintWindow (HWND, WORD, WORD, WORD, WORD, LPSTR);
WORD SaveDIB (HDIB, const char*);

#define PALVERSION   0x300

/* DIB macros */
#define IS_WIN30_DIB(lpbi)  ((*(LPDWORD)(lpbi)) == sizeof(BITMAPINFOHEADER))
#define RECTWIDTH(lpRect)     ((lpRect)->right - (lpRect)->left)
#define RECTHEIGHT(lpRect)    ((lpRect)->bottom - (lpRect)->top)

/* function prototypes */
HANDLE          AllocRoomForDIB(BITMAPINFOHEADER bi, HBITMAP hBitmap);

// $Id: winbgitypes.h,v 1.13 2003/07/21 23:14:45 bush Exp $
// Written by:
//      Grant Macklem (Grant.Macklem@colorado.edu)
//      Gregory Schmelter (Gregory.Schmelter@colorado.edu)
//      Alan Schmidt (Alan.Schmidt@colorado.edu)
//      Ivan Stashak (Ivan.Stashak@colorado.edu)
// CSCI 4830/7818: API Programming
// University of Colorado at Boulder, Spring 2003
// http://www.cs.colorado.edu/~main/bgi

#ifndef WINBGITYPES_H
#define WINBGITYPES_H

#include <windows.h>            // Provides the Win32 API
#include <tchar.h>              // Provides the _T macro
#include <queue>                // Provides STL queue class
#include <string>               // Provides STL string class

// Define maximum pages used for drawing.
#define MAX_PAGES 4
typedef void (*Handler)(int, int);

// ---------------------------------------------------------------------------
//                              Structures
// ---------------------------------------------------------------------------
// This structure gives all necessary information to the ThreadInitWindow
// function which creates a new window and processes its messages
struct WindowData
{
    int width;                  // Width of window to create
    int height;                 // Height of window to create
    int initleft, inittop;      // Initial top and left coordinates on screen
    std::string title;          // Title at the top of the window
    std::queue<TCHAR> kbd_queue;// Queue of keyboard characters
    arccoordstype arcInfo;      // Information about the last arc drawn
    fillsettingstype fillInfo;  // Information about the fill style
    char uPattern[8];           // A user-defined fill style
    linesettingstype lineInfo;  // Information about the line style
    textsettingstype textInfo;  // Information about the text style
    viewporttype viewportInfo;  // Information about the viewport
    HWND hWnd;                  // Handle to the window created
    HDC hDC[MAX_PAGES];         // Device contexts used for double buffering
    HBITMAP hOldBitmap[MAX_PAGES]; // The bitmaps generated with CreateCompatibleBitmap
    int VisualPage;             // The current device context used for painting the window
    int ActivePage;             // The current device context used for drawing
    bool DoubleBuffer;          // Whether the user wants a double buffered window (DOUBLE_BUFFER in initwindow)
    bool CloseBehavior;         // false (do nothing); true (exit program)
    int drawColor;              // The current drawing color (That the user gave us)
    int bgColor;                // The current background color (That the user gave us)
    // TODO: Maybe cahnge bgColor to always be the 0 index into the palette
    HANDLE key_waiting;         // Event signaled when a key is pressed
    HANDLE WindowCreated;       // Running event
    DWORD threadID;             // ID of thread
    int error_code;             // Error code used by graphresult (usually grOk)
    int x_aspect_ratio;         // Horizontal Aspect Ratio
    int y_aspect_ratio;         // Vertical Aspect Ratio
    HBITMAP hbitmap;            // The bitmap of the image the user loaded
    POINT ipBitmap;             // Location to draw the image
    PBITMAPINFO pbmpInfo;       // Bitmap header info
    int t_scale[4];		// scaling factor for fonts multx, divx, multy, divy
    UINT alignment;		// current alignment
    POINTS mouse;               // Current location of the mouse
    std::queue<POINTS> clicks[WM_MOUSELAST - WM_MOUSEFIRST + 1];   // Array to hold the coordinates of the clicks
    bool mouse_queuing[WM_MOUSELAST - WM_MOUSEFIRST + 1]; // Array to tell whether mouse events should be queued
    Handler mouse_handlers[WM_MOUSELAST - WM_MOUSEFIRST + 1];   // Array of mouse event handlers
    bool refreshing;            // True if autorefershing should be done after each drawing event
    HANDLE hDCMutex;            // A mutex so that only one thread at a time can access the hDC array.
};
// maybe need current position for lines, text, etc.
// palette settings
// graph error result
// lock on window






// ---------------------------------------------------------------------------
//                              Prototypes
// ---------------------------------------------------------------------------
// The entry point for each new window thread (WindowThread.cpp)
DWORD WINAPI BGI__ThreadInitWindow( LPVOID pThreadData );

// Returns a DC for the window specified by hWnd.  If hWnd is NULL, the
// current window us used (drawing.cpp)
HDC BGI__GetWinbgiDC( HWND hWnd = NULL );
void BGI__ReleaseWinbgiDC( HWND hWnd = NULL );

// Returns a pointer to the window data structure associated with hWnd.
// If hWnd is NULL, the current window is used (drawing.cpp)
WindowData* BGI__GetWindowDataPtr( HWND hWnd = NULL );

// Refreshes an area of the window:
void RefreshWindow( RECT* rect );

// ---------------------------------------------------------------------------
//                            Global Variables
// ---------------------------------------------------------------------------
#include <vector>                      // MGM: Added for WindowTable
extern std::vector<HWND> BGI__WindowTable;  // WindowThread.cpp
extern int BGI__WindowCount;         // Number of windows currently in use, WindowThread.cpp
extern int BGI__CurrentWindow;       // Index to current window, WindowThread.cpp
extern COLORREF BGI__Colors[16];  // The RGB values for the Borland 16 colors, misc.cpp
extern HINSTANCE BGI__hInstance;     // Handle to the instance of the DLL
                                // (creating the window class) WindowThread.cpp


#endif  // WINBGITYPES_H




