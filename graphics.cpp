// $Id: drawing.cpp,v 1.7 2003/04/23 06:09:50 bush Exp $
// Written by:
//      Grant Macklem (Grant.Macklem@colorado.edu)
//      Gregory Schmelter (Gregory.Schmelter@colorado.edu)
//      Alan Schmidt (Alan.Schmidt@colorado.edu)
//      Ivan Stashak (Ivan.Stashak@colorado.edu)
// CSCI 4830/7818: API Programming
// University of Colorado at Boulder, Spring 2003
// http://www.cs.colorado.edu/~main/bgi
//

#define _CRT_SECURE_NO_WARNINGS

/*****************************************************************************
*
*   Includes and conditional defines (needed for g++)
*
*****************************************************************************/
#define _USE_MATH_DEFINES   // Actually use the definitions in math.h
#include <windows.h>        // Provides the Win32 API
#include <windowsx.h>       // Provides GDI helper macros
#include <math.h>           // For mathematical functions
#include <ocidl.h>          // IPicture
#include <olectl.h>         // Support for iPicture
#include <string.h>         // Provides strlen
#include <iostream>
#include <sstream>          // Provides ostringstream
#include <string>           // Provides string

#include "graphics.h"         // API routines

#include <stdio.h>              // Provides sprintf
#include <vector>               // MGM: Added for BGI__WindowTable
#include <queue>                // Provides queue<POINTS>

#include <stdio.h>
#include <io.h>
#include <direct.h>
#include <stdlib.h>

#include <assert.h>




#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif
#ifndef max
#define max(a,b) ((a) > (b) ? (a) : (b))
#endif


/*****************************************************************************
*
*   Helper functions
*
*****************************************************************************/

// This function returns a pointer to the internal data structure holding all
// necessary data for the window specified by hWnd.
//
WindowData* BGI__GetWindowDataPtr(HWND hWnd)
{
	// Get the handle to the current window from the table if none is
	// specified.  Otherwise, use the specified value
	if (hWnd == NULL && BGI__CurrentWindow >= 0 && BGI__CurrentWindow < BGI__WindowCount)
		hWnd = BGI__WindowTable[BGI__CurrentWindow];
	if (hWnd == NULL)
	{
		showerrorbox("Drawing operation was attempted when there was no current window.");
		exit(0);
	}
	// This gets the address of the WindowData structure associated with the window
	// TODOMGM: Change this function to GetWindowLongPtr and change the set function
	// elsewhere to SetWindowLongPtr.  We are using the short version now because
	// g++ does not support the long version.
	// return (WindowData*)GetWindowLongPTR( hWnd, GWLP_USERDATA );
	return (WindowData*)GetWindowLong(hWnd, GWL_USERDATA);
}


// This function returns the device context of the active page for the window
// given by hWnd.  This device context can be used for GDI drawing commands.
//
HDC BGI__GetWinbgiDC(HWND hWnd)
{
	// Get the handle to the current window from the table if none is
	// specified.  Otherwise, use the specified value
	if (hWnd == NULL)
		hWnd = BGI__WindowTable[BGI__CurrentWindow];
	// This gets the address of the WindowData structure associated with the window
	WindowData *pWndData = BGI__GetWindowDataPtr(hWnd);

	// MGM: Added mutex to prevent conflict with OnPaint thread.
	// Anyone who calls BGI_GetWinbgiDC must later call
	// BGI_ReleaseWinbgiDC.
	WaitForSingleObject(pWndData->hDCMutex, 5000);
	// This is the device context we want to draw to
	return pWndData->hDC[pWndData->ActivePage];
}


void BGI__ReleaseWinbgiDC(HWND hWnd)
{
	// Get the handle to the current window from the table if none is
	// specified.  Otherwise, use the specified value
	if (hWnd == NULL)
		hWnd = BGI__WindowTable[BGI__CurrentWindow];
	// This gets the address of the WindowData structure associated with the window
	WindowData *pWndData = BGI__GetWindowDataPtr(hWnd);

	// MGM: Added mutex to prevent conflict with OnPaint thread.
	// Anyone who calls BGI_GetWinbgiDC must later call
	// BGI_ReleaseWinbgiDC.
	ReleaseMutex(pWndData->hDCMutex);
}


// This function converts the coordinates the user gives the system (relative
// to the center of the desired object) to coordinates specifying the corners
// of the box surrounding the object.
//
void CenterToBox(int x, int y, int xradius, int yradius,
	int* left, int* top, int* right, int* bottom)
{
	*left = x - xradius;
	*top = y - yradius;
	*right = x + xradius;
	*bottom = y + yradius;
}


// This function converts coordinates of an arc, specified by a center, radii,
// and start and end angle to actual coordinates of the window of the start
// and end of the arc.
//
void ArcEndPoints(int x, int y, int xradius, int yradius, int stangle,
	int endangle, int* xstart, int* ystart, int* xend, int* yend)
{
	*xstart = int(xradius * cos(stangle  * M_PI / 180));
	*ystart = int(yradius * sin(stangle  * M_PI / 180));
	*xend = int(xradius * cos(endangle * M_PI / 180));
	*yend = int(yradius * sin(endangle * M_PI / 180));

	// These values must be in logical coordinates of the window.
	// Thus, we must translate from coordinates respective to the
	// center of the bounding box to the window.  Also, the direction
	// of positive y changes from up to down.
	*xstart += x;
	*ystart = -*ystart + y;
	*xend += x;
	*yend = -*yend + y;
}

// This function will refresh the area of the window specified by rect.  If
// want to update the entire screen, pass in NULL for rect.
// POSTCONDITION: The parameter rect has been updated to now refer to
//                device coordinates instead of logical coordinates.  Also,
//                if we are refreshing, then the region specified by rect
//                (in device coordinates) has been marked to repaint
void RefreshWindow(RECT* rect)
{
	HDC hDC;
	WindowData* pWndData = BGI__GetWindowDataPtr();
	POINT p[2];

	if (rect != NULL)
	{
		p[0].x = rect->left;
		p[0].y = rect->top;
		p[1].x = rect->right;
		p[1].y = rect->bottom;

		// Convert from the device points to logical points (viewport relative)
		hDC = BGI__GetWinbgiDC();
		LPtoDP(hDC, p, 2);
		BGI__ReleaseWinbgiDC();

		// Copy back into the rectangle
		rect->left = p[0].x;
		rect->top = p[0].y;
		rect->right = p[1].x;
		rect->bottom = p[1].y;
	}

	if (pWndData->refreshing || rect == NULL)
	{
		// Only invalidate the window if we are viewing what we are drawing.
		// The call to InvalidateRect can fail, but I don't know what to do if it does.
		if (pWndData->VisualPage == pWndData->ActivePage)
			InvalidateRect(pWndData->hWnd, rect, FALSE);
	}
}

bool getrefreshingbgi()
{
	return BGI__GetWindowDataPtr()->refreshing;
}


void setrefreshingbgi(bool value)
{
	BGI__GetWindowDataPtr()->refreshing = value;
}

void refreshallbgi()
{
	RefreshWindow(NULL);
}

void refreshbgi(int left, int top, int right, int bottom)
{
	// The update rectangle does not contain the right or bottom edge.  Thus
	// add 1 so the entire region is included.
	RECT rect;
	HDC hDC;
	WindowData* pWndData = BGI__GetWindowDataPtr();
	POINT p[2];

	p[0].x = min(left, right);
	p[0].y = min(top, bottom);
	p[1].x = max(left, right);
	p[1].y = max(top, bottom);

	// Convert from the device points to logical points (viewport relative)
	hDC = BGI__GetWinbgiDC();
	LPtoDP(hDC, p, 2);
	BGI__ReleaseWinbgiDC();

	// Copy into the rectangle
	rect.left = p[0].x;
	rect.top = p[0].y;
	rect.right = p[1].x;
	rect.bottom = p[1].y;

	// Only invalidate the window if we are viewing what we are drawing.
	if (pWndData->VisualPage == pWndData->ActivePage)
		InvalidateRect(pWndData->hWnd, &rect, FALSE);
}

/*****************************************************************************
*
*   The actual API calls are implemented below
*
*****************************************************************************/

// This function draws a circular arc, centered at (x,y) with the given radius.
// The arc travels from angle stangle to angle endangle.  The angles are given
// in degrees in standard mathematical notation, with 0 degrees along the
// vector (1,0) and travelling counterclockwise.
// POSTCONDITION: The arccoords variable (arcinfo) for the current window
//                is set with data resulting from this call.
//                The current position is not modified.
//
void arc(int x, int y, int stangle, int endangle, int radius)
{
	HDC hDC;
	WindowData* pWndData = BGI__GetWindowDataPtr();
	// Convert coordinates to those expected by GDI Arc
	int left, top, right, bottom;
	int xstart, ystart, xend, yend;

	// Convert center coordinates to box coordinates
	CenterToBox(x, y, radius, radius, &left, &top, &right, &bottom);
	// Convert given arc specifications to pixel start and end points.
	ArcEndPoints(x, y, radius, radius, stangle, endangle, &xstart, &ystart, &xend, &yend);

	// Draw to the current active page
	hDC = BGI__GetWinbgiDC();
	Arc(hDC, left, top, right, bottom, xstart, ystart, xend, yend);
	BGI__ReleaseWinbgiDC();

	// The update rectangle does not contain the right or bottom edge.  Thus
	// add 1 so the entire region is included.
	RECT rect = { left, top, right + 1, bottom + 1 };
	RefreshWindow(&rect);

	// Set the arccoords structure to relevant data.
	pWndData->arcInfo.x = x;
	pWndData->arcInfo.y = y;
	pWndData->arcInfo.xstart = xstart;
	pWndData->arcInfo.ystart = ystart;
	pWndData->arcInfo.xend = xend;
	pWndData->arcInfo.yend = yend;
}

// This function draws a 2D bar.
//
void bar(int left, int top, int right, int bottom)
{
	HDC hDC;
	WindowData* pWndData = BGI__GetWindowDataPtr();
	HBRUSH hBrush;
	int color;

	hDC = BGI__GetWinbgiDC();
	// Is it okay to use the currently selected brush to paint with?
	hBrush = (HBRUSH)GetCurrentObject(hDC, OBJ_BRUSH);
	// Set the text color for the fill pattern
	// Convert from BGI color to RGB color
	color = converttorgb(pWndData->fillInfo.color);
	SetTextColor(hDC, color);
	RECT r = { left, top, right, bottom };
	FillRect(hDC, &r, hBrush);
	// Reset the text color to the drawing color
	color = converttorgb(pWndData->drawColor);
	SetTextColor(hDC, color);
	BGI__ReleaseWinbgiDC();

	// The update rectangle does not contain the right or bottom edge.  Thus
	// add 1 so the entire region is included.
	RECT rect = { left, top, right + 1, bottom + 1 };
	RefreshWindow(&rect);
}


// This function draws a bar with a 3D outline.  The angle of the bar background is
// 30 degrees.
void bar3d(int left, int top, int right, int bottom, int depth, int topflag)
{
	HDC hDC;
	WindowData* pWndData = BGI__GetWindowDataPtr();
	int color;
	int dy;     // Distance to draw 3D bar up to
	POINT p[4]; // An array to hold vertices for the outline

	hDC = BGI__GetWinbgiDC();
	// Set the text color for the fill pattern
	// Convert from BGI color to RGB color
	color = converttorgb(pWndData->fillInfo.color);
	SetTextColor(hDC, color);
	Rectangle(hDC, left, top, right, bottom);
	// Reset the text color to the drawing color
	color = converttorgb(pWndData->drawColor);
	SetTextColor(hDC, color);

	// Draw the surrounding part.
	// The depth is specified to be the x-distance from the front line to the
	// back line, not the actual diagonal line length
	dy = (int)(depth * tan(30.0 * M_PI / 180.0));

	p[0].x = right;
	p[0].y = bottom;            // Bottom right of box
	p[1].x = right + depth;
	p[1].y = bottom - dy;       // Bottom right of outline
	p[2].x = right + depth;
	p[2].y = top - dy;          // Upper right of outline
	p[3].x = right;
	p[3].y = top;               // Upper right of box

								// A depth of zero is a way to draw a 2D bar with an outline.  No need to
								// draw the 3D outline in this case.
	if (depth != 0)
		Polyline(hDC, p, 4);

	// If the user specifies the top to be drawn
	if (topflag != 0)
	{
		p[0].x = right + depth;
		p[0].y = top - dy;          // Upper right of outline
		p[1].x = left + depth;
		p[1].y = top - dy;          // Upper left of outline
		p[2].x = left;
		p[2].y = top;               // Upper left of box
		Polyline(hDC, p, 3);
	}
	BGI__ReleaseWinbgiDC();

	// The update rectangle does not contain the right or bottom edge.  Thus
	// add 1 so the entire region is included.
	RECT rect = { left, top - dy, right + depth + 1, bottom + 1 };
	RefreshWindow(&rect);
}


// Thus function draws a circle centered at (x,y) of given radius.
//
void circle(int x, int y, int radius)
{
	HDC hDC;
	WindowData* pWndData = BGI__GetWindowDataPtr();
	int left, top, right, bottom;

	// Convert center coordinates to box coordinates
	CenterToBox(x, y, radius, radius, &left, &top, &right, &bottom);

	// When the start and end points are the same, Arc draws a complete ellipse
	hDC = BGI__GetWinbgiDC();
	Arc(hDC, left, top, right, bottom, x + radius, y, x + radius, y);
	BGI__ReleaseWinbgiDC();

	// The update rectangle does not contain the right or bottom edge.  Thus
	// add 1 so the entire region is included.
	RECT rect = { left, top, right + 1, bottom + 1 };
	RefreshWindow(&rect);
}


// This function clears the graphics screen (with the background color) and
// moves the current point to (0,0)
//
void cleardevice()
{
	HDC hDC;
	WindowData* pWndData = BGI__GetWindowDataPtr();
	int color;          // Background color to fill with
	RECT rect;          // The rectangle to fill
	HRGN hRGN;          // The clipping region (if any)
	int is_rgn;         // Whether or not a clipping region is present
	POINT p;            // Upper left point of window (convert from device to logical points)
	HBRUSH hBrush;      // Brush in the background color

						// Convert from BGI color to RGB color
	color = converttorgb(pWndData->bgColor);

	hDC = BGI__GetWinbgiDC();
	// Even though a viewport may be set, this function clears the entire screen.
	// Compute the origin in logical coordinates.
	p.x = 0;
	p.y = 0;
	DPtoLP(hDC, &p, 1);
	rect.left = p.x;
	rect.top = p.y;
	rect.right = pWndData->width;
	rect.bottom = pWndData->height;

	// Get the current clipping region, if any.  The region object must first
	// be created with some valid region.  If the GetClipRgn function
	// succeeds, the region info will be updated to reflect the new region.
	// However, this does not create a new region object.  That is, this
	// simply overwrites the memory of the old region object and we don't have
	// to worry about deleting the old region.
	hRGN = CreateRectRgn(0, 0, 5, 5);
	is_rgn = GetClipRgn(hDC, hRGN);
	// If there is a clipping region, select none
	if (is_rgn != 0)
		SelectClipRgn(hDC, NULL);

	// Fill hDC with background color
	hBrush = CreateSolidBrush(color);
	FillRect(hDC, &rect, hBrush);
	DeleteObject(hBrush);
	// Move the CP back to (0,0) (NOT viewport relative)
	moveto(p.x, p.y);

	// Select the old clipping region back into the device context
	if (is_rgn != 0)
		SelectClipRgn(hDC, hRGN);
	// Delete the region
	DeleteRgn(hRGN);
	BGI__ReleaseWinbgiDC();

	RefreshWindow(NULL);
}


// This function clears the current viewport (with the background color) and
// moves the current point to (0,0 (relative to the viewport)
//
void clearviewport()
{
	HDC hDC;
	WindowData* pWndData = BGI__GetWindowDataPtr();
	int color;
	RECT rect;
	HBRUSH hBrush;

	// Convert from BGI color to RGB color
	color = converttorgb(pWndData->bgColor);

	rect.left = 0;
	rect.top = 0;
	rect.right = pWndData->viewportInfo.right - pWndData->viewportInfo.left;
	rect.bottom = pWndData->viewportInfo.bottom - pWndData->viewportInfo.top;

	// Fill hDC with background color
	hDC = BGI__GetWinbgiDC();
	hBrush = CreateSolidBrush(color);
	FillRect(hDC, &rect, hBrush);
	DeleteObject(hBrush);
	BGI__ReleaseWinbgiDC();
	moveto(0, 0);

	RefreshWindow(NULL);
}


void drawpoly(int n_points, int* points)
{
	HDC hDC;
	WindowData* pWndData = BGI__GetWindowDataPtr();

	hDC = BGI__GetWinbgiDC();
	Polyline(hDC, (POINT*)points, n_points);
	BGI__ReleaseWinbgiDC();

	// One could compute the convex hull of these points and create the
	// associated region to update...
	RefreshWindow(NULL);
}


// This function draws an elliptical arc with the current drawing color,
// centered at (x,y) with major and minor axes given by xradius and yradius.
// The arc travels from angle stangle to angle endangle.  The angles are given
// in degrees in standard mathematical notation, with 0 degrees along the
// vector (1,0) and travelling counterclockwise.
// 
void ellipse(int x, int y, int stangle, int endangle, int xradius, int yradius)
{
	HDC hDC;
	WindowData* pWndData = BGI__GetWindowDataPtr();
	// Convert coordinates to those expected by GDI Arc
	int left, top, right, bottom;
	int xstart, ystart, xend, yend;


	// Convert center coordinates to box coordinates
	CenterToBox(x, y, xradius, yradius, &left, &top, &right, &bottom);
	// Convert given arc specifications to pixel start and end points.
	ArcEndPoints(x, y, xradius, yradius, stangle, endangle, &xstart, &ystart, &xend, &yend);

	// Draw to the current active page
	hDC = BGI__GetWinbgiDC();
	Arc(hDC, left, top, right, bottom, xstart, ystart, xend, yend);
	BGI__ReleaseWinbgiDC();

	// The update rectangle does not contain the right or bottom edge.  Thus
	// add 1 so the entire region is included.
	RECT rect = { left, top, right + 1, bottom + 1 };
	RefreshWindow(&rect);
}


// This function draws and ellipse centered at (x,y) with major and minor axes
// xradius and yradius.  It fills the ellipse with the current fill color and
// fill pattern.
//
void fillellipse(int x, int y, int xradius, int yradius)
{
	HDC hDC;
	WindowData* pWndData = BGI__GetWindowDataPtr();
	// Convert coordinates to those expected by GDI Ellipse
	int left, top, right, bottom;
	int color;

	// Convert center coordinates to box coordinates
	CenterToBox(x, y, xradius, yradius, &left, &top, &right, &bottom);

	// Set the text color for the fill pattern
	// Convert from BGI color to RGB color
	hDC = BGI__GetWinbgiDC();
	color = converttorgb(pWndData->fillInfo.color);
	SetTextColor(hDC, color);
	Ellipse(hDC, left, top, right, bottom);
	// Reset the text color to the drawing color
	color = converttorgb(pWndData->drawColor);
	SetTextColor(hDC, color);
	BGI__ReleaseWinbgiDC();

	// The update rectangle does not contain the right or bottom edge.  Thus
	// add 1 so the entire region is included.
	RECT rect = { left, top, right + 1, bottom + 1 };
	RefreshWindow(&rect);
}


void fillpoly(int n_points, int* points)
{
	HDC hDC;
	WindowData* pWndData = BGI__GetWindowDataPtr();
	int color;

	// Set the text color for the fill pattern
	// Convert from BGI color to RGB color
	hDC = BGI__GetWinbgiDC();
	color = converttorgb(pWndData->fillInfo.color);
	SetTextColor(hDC, color);

	Polygon(hDC, (POINT*)points, n_points);

	// Reset the text color to the drawing color
	color = converttorgb(pWndData->drawColor);
	SetTextColor(hDC, color);
	BGI__ReleaseWinbgiDC();

	RefreshWindow(NULL);
}


// This function fills an enclosed area bordered by a given color.  If the
// reference poitn (x,y) is within the closed area, the area is filled.  If
// it is outside the closed area, the outside area will be filled.  The
// current fill pattern and style is used.
//
void floodfill(int x, int y, int border)
{
	HDC hDC;
	WindowData* pWndData = BGI__GetWindowDataPtr();
	int color;

	// Set the text color for the fill pattern
	// Convert from BGI color to RGB color
	color = converttorgb(pWndData->fillInfo.color);
	border = converttorgb(border);
	hDC = BGI__GetWinbgiDC();
	SetTextColor(hDC, color);
	FloodFill(hDC, x, y, border);
	// Reset the text color to the drawing color
	color = converttorgb(pWndData->drawColor);
	SetTextColor(hDC, color);
	BGI__ReleaseWinbgiDC();

	RefreshWindow(NULL);
}


// This function draws a line from (x1,y1) to (x2,y2) using the current line
// style and thickness.  It does not update the current point.
//
void line(int x1, int y1, int x2, int y2)
{
	HDC hDC;
	WindowData* pWndData = BGI__GetWindowDataPtr();

	// The current position
	POINT cp;

	// Move to first point, save old point
	hDC = BGI__GetWinbgiDC();
	MoveToEx(hDC, x1, y1, &cp);
	// Draw the line
	LineTo(hDC, x2, y2);
	// Move the current point back to its original position
	MoveToEx(hDC, cp.x, cp.y, NULL);
	BGI__ReleaseWinbgiDC();

	// The update rectangle does not contain the right or bottom edge.  Thus
	// add 1 so the entire region is included.
	RECT rect = { min(x1,x2), min(y1,y2), max(x1,x2) + 1, max(y1,y2) + 1 };
	RefreshWindow(&rect);
}


// This function draws a line from the current point to a point that is a
// relative distance (dx,dy) away.  The current point is updated to the final
// point.
//
void linerel(int dx, int dy)
{
	HDC hDC;
	WindowData* pWndData = BGI__GetWindowDataPtr();

	// The current position
	POINT cp;

	hDC = BGI__GetWinbgiDC();
	GetCurrentPositionEx(hDC, &cp);
	LineTo(hDC, cp.x + dx, cp.y + dy);
	BGI__ReleaseWinbgiDC();

	// The update rectangle does not contain the right or bottom edge.  Thus
	// add 1 so the entire region is included.
	RECT rect = { min(cp.x,cp.x + dx), min(cp.y,cp.y + dy), max(cp.x,cp.x + dx) + 1, max(cp.y,cp.y + dy) + 1 };
	RefreshWindow(&rect);
}


// This function draws a line from the current point to (x,y).  The current
// point is updated to (x,y)
//
void lineto(int x, int y)
{
	HDC hDC;
	WindowData* pWndData = BGI__GetWindowDataPtr();

	// The current position
	POINT cp;

	hDC = BGI__GetWinbgiDC();
	GetCurrentPositionEx(hDC, &cp);
	LineTo(hDC, x, y);
	BGI__ReleaseWinbgiDC();

	// The update rectangle does not contain the right or bottom edge.  Thus
	// add 1 so the entire region is included.
	RECT rect = { min(cp.x,x), min(cp.y,y), max(cp.x,x) + 1, max(cp.y,y) + 1 };
	RefreshWindow(&rect);
}


// This function draws a pie slice centered at (x,y) with a given radius.  It
// is filled with the current fill pattern and color and outlined with the
// current line color.  The pie slice travels from angle stangle to angle
// endangle.  The angles are given in degrees in standard mathematical
// notation, with 0 degrees along the vector (1,0) and travelling
//counterclockwise.
// 
void pieslice(int x, int y, int stangle, int endangle, int radius)
{
	HDC hDC;
	WindowData* pWndData = BGI__GetWindowDataPtr();
	// Convert coordinates to those expected by GDI Pie
	int left, top, right, bottom;
	int xstart, ystart, xend, yend;
	int color;


	// Convert center coordinates to box coordinates
	CenterToBox(x, y, radius, radius, &left, &top, &right, &bottom);
	// Convert given arc specifications to pixel start and end points.
	ArcEndPoints(x, y, radius, radius, stangle, endangle, &xstart, &ystart, &xend, &yend);

	// Set the text color for the fill pattern
	// Convert from BGI color to RGB color
	color = converttorgb(pWndData->fillInfo.color);
	hDC = BGI__GetWinbgiDC();
	SetTextColor(hDC, color);
	Pie(hDC, left, top, right, bottom, xstart, ystart, xend, yend);
	// Reset the text color to the drawing color
	color = converttorgb(pWndData->drawColor);
	SetTextColor(hDC, color);
	BGI__ReleaseWinbgiDC();

	// The update rectangle does not contain the right or bottom edge.  Thus
	// add 1 so the entire region is included.
	RECT rect = { left, top, right + 1, bottom + 1 };
	RefreshWindow(&rect);
}

// This function plots a pixel in the specified color at point (x,y)
//
void putpixel(int x, int y, int color)
{
	HDC hDC;
	WindowData* pWndData = BGI__GetWindowDataPtr();

	color = converttorgb(color);
	// The call to SetPixelV might fail, but I don't know what to do if it does.
	hDC = BGI__GetWinbgiDC();
	SetPixelV(hDC, x, y, color);
	BGI__ReleaseWinbgiDC();

	// The update rectangle does not contain the right or bottom edge.  Thus
	// add 1 so the entire region is included.
	RECT rect = { x, y, x + 1, y + 1 };
	RefreshWindow(&rect);
}


// This function draws a rectangle border in the current line style, thickness, and color
//
void rectangle(int left, int top, int right, int bottom)
{
	HDC hDC;
	WindowData* pWndData = BGI__GetWindowDataPtr();

	POINT endpoints[5];         // Endpoints of the line
	endpoints[0].x = left;      // Upper left
	endpoints[0].y = top;
	endpoints[1].x = right;     // Upper right
	endpoints[1].y = top;
	endpoints[2].x = right;     // Lower right
	endpoints[2].y = bottom;
	endpoints[3].x = left;      // Lower left
	endpoints[3].y = bottom;
	endpoints[4].x = left;      // Upper left to complete rectangle
	endpoints[4].y = top;

	hDC = BGI__GetWinbgiDC();
	Polyline(hDC, endpoints, 5);
	BGI__ReleaseWinbgiDC();

	// The update rectangle does not contain the right or bottom edge.  Thus
	// add 1 so the entire region is included.
	RECT rect = { left, top, right + 1, bottom + 1 };
	RefreshWindow(&rect);
}


// This function draws an elliptical pie slice centered at (x,y) with major
// and minor radii given by xradius and yradius.  It is filled with the
// current fill pattern and color and outlined with the current line color.
// The pie slice travels from angle stangle to angle endangle.  The angles are
// given in degrees in standard mathematical notation, with 0 degrees along
// the vector (1,0) and travelling counterclockwise.
// 
void sector(int x, int y, int stangle, int endangle, int xradius, int yradius)
{
	HDC hDC;
	WindowData* pWndData = BGI__GetWindowDataPtr();
	// Convert coordinates to those expected by GDI Pie
	int left, top, right, bottom;
	int xstart, ystart, xend, yend;
	int color;


	// Convert center coordinates to box coordinates
	CenterToBox(x, y, xradius, yradius, &left, &top, &right, &bottom);
	// Convert given arc specifications to pixel start and end points.
	ArcEndPoints(x, y, xradius, yradius, stangle, endangle, &xstart, &ystart, &xend, &yend);

	// Set the text color for the fill pattern
	// Convert from BGI color to RGB color
	color = converttorgb(pWndData->fillInfo.color);
	hDC = BGI__GetWinbgiDC();
	SetTextColor(hDC, color);
	Pie(hDC, left, top, right, bottom, xstart, ystart, xend, yend);
	// Reset the text color to the drawing color
	color = converttorgb(pWndData->drawColor);
	SetTextColor(hDC, color);
	BGI__ReleaseWinbgiDC();

	// The update rectangle does not contain the right or bottom edge.  Thus
	// add 1 so the entire region is included.
	RECT rect = { left, top, right + 1, bottom + 1 };
	RefreshWindow(&rect);
}

// MGM modified imagesize so that it returns zero in the case of failure.
unsigned int imagesize(int left, int top, int right, int bottom)
{
	long width, height;   // Width and height of the image in pixels
	WindowData* pWndData; // Our own window data struct for active window
	HDC hDC;              // Device context for the active window
	HDC hMemoryDC;        // Memory device context for a copy of the image
	HBITMAP hOldBitmap;   // Handle to original bitmap of hMemDC
	HBITMAP hBitmap;      // Handle to bitmap that will be selected into hMemDC
	BITMAP b;             // The actual bitmap object for hBitmap
	long answer;          // Bytes needed to save this image
//	int tries;

	// Preliminary computations
	width = 1 + abs(right - left);
	height = 1 + abs(bottom - top);
	pWndData = BGI__GetWindowDataPtr();
	hDC = BGI__GetWinbgiDC();

	// Create the memory DC and select a new larger bitmap for it, saving the
	// original bitmap to restore later (before deleting).
	hMemoryDC = CreateCompatibleDC(hDC);
	hBitmap = CreateCompatibleBitmap(hDC, width, height);
	hOldBitmap = (HBITMAP)SelectObject(hMemoryDC, hBitmap);

	// Copy the requested region into hBitmap which is selected into hMemoryDC,
	// then get a copy of this actual bitmap so we can compute its size.
	BitBlt(hMemoryDC, 0, 0, width, height, hDC, left, top, SRCCOPY);
	GetObject(hBitmap, sizeof(BITMAP), &b);
	answer = sizeof(BITMAP) + b.bmHeight*b.bmWidthBytes;
	if (answer > UINT_MAX) answer = 0;

	// Delete resources
	BGI__ReleaseWinbgiDC();
	SelectObject(hMemoryDC, hOldBitmap); // Restore original bmp so it's deleted
	DeleteObject(hBitmap);               // Delete the bitmap we used
	DeleteDC(hMemoryDC);                 // Delete the memory dc and it's bmp

	return (unsigned int)answer;
}


void getimage(int left, int top, int right, int bottom, void *bitmap)
{
	long width, height;   // Width and height of the image in pixels
	WindowData* pWndData; // Our own window data struct for active window
	HDC hDC;              // Device context for the active window
	HDC hMemoryDC;        // Memory device context for a copy of the image
	HBITMAP hOldBitmap;   // Handle to original bitmap of hMemDC
	HBITMAP hBitmap;      // Handle to bitmap that will be selected into hMemDC
	BITMAP* pUser;        // A pointer into the user's buffer, used as a BITMAP
//	long answer;          // Bytes needed to save this image

						  // Preliminary computations
	pWndData = BGI__GetWindowDataPtr();
	hDC = BGI__GetWinbgiDC();
	width = 1 + abs(right - left);
	height = 1 + abs(bottom - top);

	// Create the memory DC and select a new larger bitmap for it, saving the
	// original bitmap to restore later (before deleting).
	hMemoryDC = CreateCompatibleDC(hDC);
	hBitmap = CreateCompatibleBitmap(hDC, width, height);
	hOldBitmap = (HBITMAP)SelectObject(hMemoryDC, hBitmap);

	// Grab the bitmap data from hDC and put it in hMemoryDC
	SelectObject(hMemoryDC, hBitmap);
	BitBlt(hMemoryDC, 0, 0, width, height, hDC, left, top, SRCCOPY);

	// Copy the device-dependent bitmap into the user's allocated space
	pUser = (BITMAP*)bitmap;
	GetObject(hBitmap, sizeof(BITMAP), pUser);
	pUser->bmBits = (BYTE*)bitmap + sizeof(BITMAP);
	GetBitmapBits(hBitmap, pUser->bmHeight*pUser->bmWidthBytes, pUser->bmBits);

	// Delete resources
	BGI__ReleaseWinbgiDC();
	SelectObject(hMemoryDC, hOldBitmap); // Restore original bmp so it's deleted
	DeleteObject(hBitmap);               // Delete the bitmap we used
	DeleteDC(hMemoryDC);                 // Delete the memory dc and it's bmp
}

void putimage(int left, int top, void *bitmap, int op)
{
	long width, height;   // Width and height of the image in pixels
	WindowData* pWndData; // Our own window data struct for active window
	HDC hDC;              // Device context for the active window
	HDC hMemoryDC;        // Memory device context for a copy of the image
	HBITMAP hOldBitmap;   // Handle to original bitmap of hMemDC
	HBITMAP hBitmap;      // Handle to bitmap that will be selected into hMemDC
	BITMAP* pUser;        // A pointer into the user's buffer, used as a BITMAP

						  // Preliminary computations
	pUser = (BITMAP*)bitmap;
	width = pUser->bmWidth;
	height = pUser->bmHeight;
	pWndData = BGI__GetWindowDataPtr();
	hDC = BGI__GetWinbgiDC();

	// Create the memory DC and select a new larger bitmap for it, saving the
	// original bitmap to restore later (before deleting).
	hMemoryDC = CreateCompatibleDC(hDC);
	hBitmap = CreateCompatibleBitmap(hDC, pUser->bmWidth, pUser->bmHeight);
	hOldBitmap = (HBITMAP)SelectObject(hMemoryDC, hBitmap);

	// Grab the bitmap data from the user's bitmap and put it in hMemoryDC
	SetBitmapBits(hBitmap, pUser->bmHeight*pUser->bmWidthBytes, pUser->bmBits);

	// Copy the bitmap from hMemoryDC to the active hDC:
	switch (op)
	{
	case COPY_PUT:
		BitBlt(hDC, left, top, width, height, hMemoryDC, 0, 0, SRCCOPY);
		break;
	case XOR_PUT:
		BitBlt(hDC, left, top, width, height, hMemoryDC, 0, 0, SRCINVERT);
		break;
	case OR_PUT:
		BitBlt(hDC, left, top, width, height, hMemoryDC, 0, 0, SRCPAINT);
		break;
	case AND_PUT:
		BitBlt(hDC, left, top, width, height, hMemoryDC, 0, 0, SRCAND);
		break;
	case NOT_PUT:
		BitBlt(hDC, left, top, width, height, hMemoryDC, 0, 0, NOTSRCCOPY);
		break;
	}
	RefreshWindow(NULL);


	// Delete resources
	BGI__ReleaseWinbgiDC();
	SelectObject(hMemoryDC, hOldBitmap); // Restore original bmp so it's deleted
	DeleteObject(hBitmap);               // Delete the bitmap we used
	DeleteDC(hMemoryDC);                 // Delete the memory dc and it's bmp
}

static LPPICTURE readipicture(const char* filename)
{
	// The only way that I have found to use OleLoadImage is to first put all
	// the picture information into a stream. Based on Serguei's implementation
	// and information from http://www.codeproject.com/bitmap/render.asp?print=true
	HANDLE hFile;        // Handle to the picture file
	DWORD dwSize;        // Size of that file
	HGLOBAL hGlobal;     // Handle for memory block
	LPVOID pvData;       // Pointer to first byte of that memory block
	BOOL bRead;          // Was file read okay?
	DWORD dwBytesRead;   // Number of bytes read from the file
	LPSTREAM pStr;       // Pointer to an IStream
	HRESULT hOK;         // Result of various OLE operations
	LPPICTURE pPicture;  // Picture read by OleLoadPicture

						 // Open the file. Page 943 Win32 Superbible
	hFile = CreateFile(
		filename,        // Name of the jpg, gif, or bmp
		GENERIC_READ,    // Open for reading
		FILE_SHARE_READ, // Allow others to read, too
		NULL,            // Security attributes
		OPEN_EXISTING,   // The file must previously exist
		0,               // Attributes if creating new file
		NULL             // Attribute templates if creating new file
		);
	if (hFile == INVALID_HANDLE_VALUE) return NULL;

	// Get the file size and check that it is not empty.
	dwSize = GetFileSize(hFile, NULL);
	if (dwSize == (DWORD)-1)
	{
		CloseHandle(hFile);
		// AfxMessageBox("Photo file was empty."); -- needs MFC;
		return NULL;
	}

	// Allocate memory based on file size and lock it so it can't be moved.
	hGlobal = GlobalAlloc(GMEM_MOVEABLE, dwSize);
	if (hGlobal == NULL)
	{
		CloseHandle(hFile);
		showerrorbox("Insufficient memory to read image");
		return NULL;
	}

	// Lock memory so it can't be moved, then read the file into that memory.
	pvData = GlobalLock(hGlobal);
	if (pvData != NULL)
	{
		dwBytesRead = 0; // To force whole file to be read
		bRead = ReadFile(hFile, pvData, dwSize, &dwBytesRead, NULL);
	}
	GlobalUnlock(hGlobal);
	CloseHandle(hFile);
	if ((pvData == NULL) || !bRead || (dwBytesRead != dwSize))
	{
		GlobalFree(hGlobal);
		// AfxMessage("Could not read photo file."; -- needs MFC
		return NULL;
	}

	// At this point, the file is in the hGlobal memory block.
	// We will now connect an IStream* to this global memory.
	pStr = NULL; // In case CreateStreamOnHGlobal doesn't set it.
	hOK = CreateStreamOnHGlobal(hGlobal, TRUE, &pStr);
	if (pStr == NULL)
	{
		GlobalFree(hGlobal);
		// AfxMessage("Could not create IStream."; -- needs MFC
		return NULL;
	}
	if (FAILED(hOK))
	{
		GlobalFree(hGlobal);
		pStr->Release();
		// AfxMessage("Could not create IStream."; -- needs MFC
		return NULL;
	}

	// Finally: Load the picture
	hOK = OleLoadPicture(pStr, dwSize, FALSE, IID_IPicture, (LPVOID *)&pPicture);
	pStr->Release();
	if (!SUCCEEDED(hOK) || (pPicture == NULL))
	{
		GlobalFree(hGlobal);
		// AfxMessage("Could not create IStream."; -- needs MFC
		return NULL;
	}

	// pPicture is now a pointer to our picture.
	GlobalFree(hGlobal);
	return pPicture;
}

void readimagefile(
	const char* filename,
	int left, int top, int right, int bottom
	)
{
	WindowData* pWndData; // Our own window data struct for active window
	HDC hDC;              // Device context for the active window
	LPPICTURE pPicture = NULL;                     // Picture object for this image
	long full_width, full_height;                  // Dimensions of full image
	long width, height;                            // Dimensions of drawn image
	OPENFILENAME ofn;     // Struct for opening a file
	TCHAR fn[MAX_PATH + 1]; // Space for storing the open file name
							// Get the filename, if needed
	if (filename == NULL)
	{
		ZeroMemory(&ofn, sizeof(OPENFILENAME));
		ZeroMemory(&fn, MAX_PATH + 1);
		ofn.lStructSize = sizeof(OPENFILENAME);
		ofn.lpstrFilter = _T("Image files (*.bmp, *.gif, *.jpg, *.ico, *.emf, *.wmf)\0*.BMP;*.GIF;*.JPG;*.ICO;*.EMF;*.WMF\0\0");
		ofn.lpstrFile = fn;
		ofn.nMaxFile = MAX_PATH + 1;
		ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
		if (!GetOpenFileName(&ofn)) return;
	}

	if (filename == NULL)
		pPicture = readipicture(fn);
	else
		pPicture = readipicture(filename);
	if (pPicture)
	{
		pWndData = BGI__GetWindowDataPtr();
		hDC = BGI__GetWinbgiDC();
		width = 1 + abs(right - left);
		height = 1 + abs(bottom - top);
		pPicture->get_Width(&full_width);
		pPicture->get_Height(&full_height);
		pPicture->Render(hDC, left, top, width, height, 0, full_height, full_width, -full_height, NULL);
		BGI__ReleaseWinbgiDC();
		pPicture->Release();
		RefreshWindow(NULL);
	}
}

void writeimagefile(
	const char* filename,
	int left, int top, int right, int bottom,
	bool active, HWND hwnd
	)
{
	long width, height;   // Width and height of the image in pixels
	WindowData* pWndData; // Our own window data struct for active window
	HDC hDC;              // Device context for the active window
	HDC hMemoryDC;        // Memory device context for a copy of the image
	HBITMAP hOldBitmap;   // Handle to original bitmap of hMemDC
	HBITMAP hBitmap;      // Handle to bitmap that will be selected into hMemDC
	HDIB hDIB;            // Handle to equivalent device independent bitmap
	OPENFILENAME ofn;     // Struct for opening a file
	TCHAR fn[MAX_PATH + 1]; // Space for storing the open file name
							// Get the filename, if needed
	if (filename == NULL)
	{
		ZeroMemory(&ofn, sizeof(OPENFILENAME));
		ZeroMemory(&fn, MAX_PATH + 1);
		ofn.lStructSize = sizeof(OPENFILENAME);
		ofn.lpstrFilter = _T("Bitmap files (*.bmp)\0*.BMP\0\0");
		ofn.lpstrFile = fn;
		ofn.nMaxFile = MAX_PATH + 1;
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_NOREADONLYRETURN | OFN_OVERWRITEPROMPT;
		if (!GetSaveFileName(&ofn)) return;
		if (strlen(fn) < 4 || (fn[strlen(fn) - 4] != '.' && strlen(fn) < MAX_PATH - 4))
			strcat(fn, ".BMP");
	}

	// Preliminary computations
	pWndData = BGI__GetWindowDataPtr(hwnd);
	WaitForSingleObject(pWndData->hDCMutex, 5000);
	if (active)
		hDC = pWndData->hDC[pWndData->ActivePage];
	else
		hDC = pWndData->hDC[pWndData->VisualPage];
	if (left < 0) left = 0;
	else if (left >= pWndData->width) left = pWndData->width - 1;
	if (right < 0) right = 0;
	else if (right >= pWndData->width) right = pWndData->width;
	if (bottom < 0) bottom = 0;
	else if (bottom >= pWndData->height) bottom = pWndData->height;
	if (top < 0) top = 0;
	else if (top >= pWndData->height) top = pWndData->height;
	width = 1 + abs(right - left);
	height = 1 + abs(bottom - top);

	// Create the memory DC and select a new larger bitmap for it, saving the
	// original bitmap to restore later (before deleting).
	hMemoryDC = CreateCompatibleDC(hDC);
	hBitmap = CreateCompatibleBitmap(hDC, width, height);

	hOldBitmap = (HBITMAP)SelectObject(hMemoryDC, hBitmap);

	// Grab the bitmap data from hDC and put it in hMemoryDC
	SelectObject(hMemoryDC, hBitmap);
	BitBlt(hMemoryDC, 0, 0, width, height, hDC, left, top, SRCCOPY);

	// Get the equivalent DIB and write it to the file
	hDIB = BitmapToDIB(hBitmap, NULL);
	if (filename == NULL)
		SaveDIB(hDIB, fn);
	else
		SaveDIB(hDIB, filename);

	// Delete resources
	ReleaseMutex(pWndData->hDCMutex);
	DestroyDIB(hDIB);
	SelectObject(hMemoryDC, hOldBitmap); // Restore original bmp so it's deleted
	DeleteObject(hBitmap);               // Delete the bitmap we used
	DeleteDC(hMemoryDC);                 // Delete the memory dc and it's bmp
}

void printimage(
	const char* title,
	double width_inches, double border_left_inches, double border_top_inches,
	int left, int top, int right, int bottom, bool active, HWND hwnd
	)
{
	static PRINTDLG pd_Printer;
	WindowData* pWndData; // Our own window data struct for visual window
	long width, height;   // Width and height of the image in pixels
	HDC hMemoryDC;        // Memory device context for a copy of the image
	HDC hDC;              // Device context for the visual window to print
	HBITMAP hBitmap;      // Handle to bitmap that will be selected into hMemDC
	HBITMAP hOldBitmap;   // Handle to original bitmap of hMemDC
	int titlelen;         // Length of the title
	int pixels_per_inch_x, pixels_per_inch_y;
	double factor_x, factor_y;
	DOCINFO di;

	// Set pd_Printer.hDC to a handle to the printer dc using a print dialog,
	// as shown on page 950-957 of Win32 Programming.  Note that the setup
	// is done the first time this function is called (since pd_Printer will
	// be all zeros) or if both hDevNames and hDevMode are later NULL, as
	// shown in Listing 13.3 on page 957.
	if (pd_Printer.hDevNames == NULL && pd_Printer.hDevMode == NULL)
	{
		memset(&pd_Printer, 0, sizeof(PRINTDLG));
		pd_Printer.lStructSize = sizeof(PRINTDLG);
		pd_Printer.Flags = PD_RETURNDEFAULT;
		// Get the default printer:
		if (!PrintDlg(&pd_Printer))
			return; // Failure
					// Set things up so next call to PrintDlg won't go back to default.
		pd_Printer.Flags &= ~PD_RETURNDEFAULT;
	}
	// Cause PrintDlg to return a DC in hDC; could set other flags here, too.
	pd_Printer.Flags |= PD_RETURNDC;
	if (!PrintDlg(&pd_Printer))
		return; // Failure or canceled

				// Get the window's hDC, width and height
	pWndData = BGI__GetWindowDataPtr(hwnd);
	WaitForSingleObject(pWndData->hDCMutex, 5000);
	if (active)
		hDC = pWndData->hDC[pWndData->ActivePage];
	else
		hDC = pWndData->hDC[pWndData->VisualPage];
	if (left < 0) left = 0;
	else if (left >= pWndData->width) left = pWndData->width - 1;
	if (right < 0) right = 0;
	else if (right >= pWndData->width) right = pWndData->width;
	if (bottom < 0) bottom = 0;
	else if (bottom >= pWndData->height) bottom = pWndData->height;
	if (top < 0) top = 0;
	else if (top >= pWndData->height) top = pWndData->height;
	width = 1 + abs(right - left);
	height = 1 + abs(bottom - top);

	// Create the memory DC and select a new larger bitmap for it, saving the
	// original bitmap to restore later (before deleting).
	hMemoryDC = CreateCompatibleDC(hDC);
	hBitmap = CreateCompatibleBitmap(hDC, width, height);
	hOldBitmap = (HBITMAP)SelectObject(hMemoryDC, hBitmap);

	// Copy the bitmap data from hDC and put it in hMemoryDC for printing
	SelectObject(hMemoryDC, hBitmap);
	BitBlt(hMemoryDC, 0, 0, width, height, hDC, left, top, SRCCOPY);

	// Determine the size factors for blowing up the photo.
	pixels_per_inch_x = GetDeviceCaps(pd_Printer.hDC, LOGPIXELSX);
	pixels_per_inch_y = GetDeviceCaps(pd_Printer.hDC, LOGPIXELSY);
	factor_x = pixels_per_inch_x * width_inches / width;
	factor_y = factor_x * pixels_per_inch_y / pixels_per_inch_x;

	// Set up a DOCINFO structure.
	memset(&di, 0, sizeof(DOCINFO));
	di.cbSize = sizeof(DOCINFO);
	di.lpszDocName = "Windows BGI";

	// StartDoc, print stuff, EndDoc
	if (StartDoc(pd_Printer.hDC, &di) != SP_ERROR)
	{
		StartPage(pd_Printer.hDC);
		if (title == NULL) title = pWndData->title.c_str();
		titlelen = strlen(title);
		if (titlelen > 0)
		{
			TextOut(pd_Printer.hDC, int(pixels_per_inch_x*border_left_inches), int(pixels_per_inch_y*border_top_inches), title, titlelen);
			border_top_inches += 0.25;
		}
		if (GetDeviceCaps(pd_Printer.hDC, RASTERCAPS) & RC_BITBLT)
		{
			StretchBlt(
				pd_Printer.hDC, int(pixels_per_inch_x*border_left_inches), int(pixels_per_inch_y*border_top_inches),
				int(width*factor_x),
				int(height*factor_y),
				hMemoryDC, 0, 0, width, height,
				SRCCOPY
				);
		}
		EndPage(pd_Printer.hDC);

		EndDoc(pd_Printer.hDC);
	}

	// Delete the resources
	ReleaseMutex(pWndData->hDCMutex);
	SelectObject(hMemoryDC, hOldBitmap); // Restore original bmp so it's deleted
	DeleteObject(hBitmap);               // Delete the bitmap we used
	DeleteDC(hMemoryDC);                 // Delete the memory dc and it's bmp
}

// $Id: misc.cpp,v 1.11 2003/05/07 23:41:23 schmidap Exp $
// Written by:
//      Grant Macklem (Grant.Macklem@colorado.edu)
//      Gregory Schmelter (Gregory.Schmelter@colorado.edu)
//      Alan Schmidt (Alan.Schmidt@colorado.edu)
//      Ivan Stashak (Ivan.Stashak@colorado.edu)
// CSCI 4830/7818: API Programming
// University of Colorado at Boulder, Spring 2003
// http://www.cs.colorado.edu/~main/bgi
//



/*****************************************************************************
*
*   Structures
*
*****************************************************************************/
struct LinePattern
{
	int width;
	DWORD pattern[16];
};



/*****************************************************************************
*
*   Global Variables
*
*****************************************************************************/
// Solid line:  0xFFFF
// Dotted line: 0011 0011 0011 0011b    dot space
// Center line: 0001 1110 0011 1111b    dot space dash space
// Dashed line: 0001 1111 0001 1111b    dash space
// The numbers in the pattern (of the LinePattern struct) represent the width
// in pixels of the first dash, then the first space, then the next dash, etc.
// A leading space is moved to the end.
// The dash is one longer than specified; the space is one shorter.
// Creating a geometric pen using the predefined constants produces
// poor results.  Thus, the above widths have been modifed:
// Space: 3 pixels
// Dash:  8 pixels
// Dot:   4 pixels
LinePattern SOLID = { 2,{ 16, 0 } };        // In reality, these are (see note above)
LinePattern DOTTED = { 2,{ 3, 4 } };         // 4, 3
LinePattern CENTER = { 4,{ 3, 4, 7, 4 } };   // 4, 3, 8, 3
LinePattern DASHED = { 2,{ 7, 4 } };          // 8, 3

											  // Color format:
											  // High byte: 0  Color is an index, BGI color
											  //     -- This is necessary since these colors are defined to be 0-15 in BGI
											  // High byte: 3  Color is an RGB value (page 244 of Win32 book)
											  //     -- Note the internal COLORREF structure has RGB values with a high byte
											  //     of 0, but this conflicts with the BGI color notation.
											  // We store the value the user gave internally (be it number 4 for RED or
											  // our RGB encoded value).  This is then converted when needed.

											  // From http://www.textmodegames.com/articles/coolgame.html
											  // Then used BGI graphics on my system for slight modification.
COLORREF BGI__Colors[16];
// These are set in graphdefaults in winbgi.cpp
/*
= {
RGB( 0, 0, 0 ),         // Black
RGB( 0, 0, 168),        // Blue
RGB( 0, 168, 0 ),       // Green
RGB( 0, 168, 168 ),     // Cyan
RGB( 168, 0, 0 ),       // Red
RGB( 168, 0, 168 ),     // Magenta
RGB( 168, 84, 0 ),      // Brown
RGB( 168, 168, 168 ),   // Light Gray
RGB( 84, 84, 84 ),      // Dark Gray
RGB( 84, 84, 252 ),     // Light Blue
RGB( 84, 252, 84 ),     // Light Green
RGB( 84, 252, 252 ),    // Light Cyan
RGB( 252, 84, 84 ),     // Light Red
RGB( 252, 84, 252 ),    // Light Magenta
RGB( 252, 252, 84 ),    // Yellow
RGB( 252, 252, 252 )    // White
};
*/


/*****************************************************************************
*
*   Prototypes
*
*****************************************************************************/
LinePattern CreateUserStyle();


/*****************************************************************************
*
*   Helper functions
*
*****************************************************************************/

// This function converts a given color (specified by the user) into a format
// native to windows.
//
int converttorgb(int color)
{
	// Convert from BGI color to RGB color
	if (IS_BGI_COLOR(color))
		color = BGI__Colors[color];
	else
		color &= 0x0FFFFFF;

	return color;
}

#include <iostream>
// This function creates a new pen in the current drawing color and selects it
// into all the memory DC's.
//
void CreateNewPen()
{
	WindowData* pWndData = BGI__GetWindowDataPtr();
	int color = pWndData->drawColor;;
	LinePattern style;
	LOGBRUSH lb;
	HPEN hPen;

	// Convert from BGI color to RGB color
	color = converttorgb(color);

	// Set the color and style of the logical brush
	lb.lbColor = color;
	lb.lbStyle = BS_SOLID;

	if (pWndData->lineInfo.linestyle == SOLID_LINE)   style = SOLID;
	if (pWndData->lineInfo.linestyle == DOTTED_LINE)  style = DOTTED;
	if (pWndData->lineInfo.linestyle == CENTER_LINE)  style = CENTER;
	if (pWndData->lineInfo.linestyle == DASHED_LINE)  style = DASHED;
	// TODO: If user specifies a 0 pattern, create a NULL pen.
	if (pWndData->lineInfo.linestyle == USERBIT_LINE) style = CreateUserStyle();

	// Round endcaps are default, set to square
	// Use a bevel join
	WaitForSingleObject(pWndData->hDCMutex, 5000);
	for (int i = 0; i < MAX_PAGES; i++)
	{
		hPen = ExtCreatePen(PS_GEOMETRIC | PS_ENDCAP_SQUARE
			| PS_JOIN_BEVEL | PS_USERSTYLE,   // Pen Style
			pWndData->lineInfo.thickness,      // Pen Width
			&lb,                               // Logical Brush
			style.width,                       // Bytes in pattern
			style.pattern);                   // Line Pattern
		DeletePen((HPEN)SelectObject(pWndData->hDC[i], hPen));
	}
	ReleaseMutex(pWndData->hDCMutex);
}


// The user style might appear reversed from what you expect.  With the
// original Borland graphics, the least significant bit specified the first
// pixel of the line.  Thus, if you reverse the bit string, the line is
// drawn as the pixels then appear.
LinePattern CreateUserStyle()
{
	WindowData* pWndData = BGI__GetWindowDataPtr();
	int style = pWndData->lineInfo.upattern;
	int zeroCount = 0;          // A count of the number of leading zeros
	int i = 0, j, sum = 0;      // i is number of dwords, sum is a running count of bits used
	LinePattern userPattern;    // The pattern

	style &= 0xFFFF;            // Only lower 16 bits matter

	if (style == 0)
	{
		userPattern.pattern[0] = 0;
		userPattern.pattern[1] = 16;
		userPattern.width = 2;
		return userPattern;
	}

	// If the pattern starts with a zero, count how many and store until
	// later
	if ((style & 1) == 0)
	{
		for (j = 0; !(style & 1); j++) style >>= 1;
		zeroCount = j;
		sum += j;
	}

	// See note above (in Global Variables) for dash being one pixel more,
	// space being one pixel less
	while (true)
	{
		// Get a count of the number of ones
		for (j = 0; style & 1; j++) style >>= 1;
		userPattern.pattern[i++] = j - 1;                     // Subtract one for dash
		sum += j;


		// Check if the pattern is now zero.
		if (style == 0)
		{
			if (sum != 16)
				userPattern.pattern[i++] = 16 - sum + 1;    // Add one for space
			break;
		}

		// Get a count of the number of zeros
		for (j = 0; !(style & 1); j++) style >>= 1;
		userPattern.pattern[i++] = j + 1;                   // Add one for space
		sum += j;
	}

	// If there were leading zeros, put them at the end
	if (zeroCount > 0)
	{
		// If i is even, we ended on a space.  Add the leading zeros to the back
		// end count.  If i is odd, we ended on a dash.  Append the leading zeros.
		if ((i % 2) == 0)
			userPattern.pattern[i - 1] += zeroCount;
		else
			userPattern.pattern[i++] = zeroCount;
	}
	else // If there were no leading zeros, check if we need to add a space
	{
		// If we ended on a dash and there are no more following zeros, put a
		// zero-length space.  This is necessary since the user may specify
		// a style of 0xFFFF.  In this case, a solid line is not created unless
		// there is a zero-length space at the end.
		if ((i % 2) != 0)
			userPattern.pattern[i++] = 0;
	}

	// Set the with to the number of array indices used
	userPattern.width = i;

	return userPattern;
}




/*****************************************************************************
*
*   The actual API calls are implemented below
*
*****************************************************************************/

// This function will pause the current thread for the specified number of
// milliseconds
//
void delay(int msec)
{
	Sleep(msec);
}


// This function returns information about the last call to arc.
//
void getarccoords(arccoordstype *arccoords)
{
	WindowData* pWndData = BGI__GetWindowDataPtr();

	*arccoords = pWndData->arcInfo;
}


// This function returns the current background color.
//
int getbkcolor()
{
	WindowData* pWndData = BGI__GetWindowDataPtr();

	return pWndData->bgColor;
}


// This function returns the current drawing color.
//
int getcolor()
{
	WindowData* pWndData = BGI__GetWindowDataPtr();

	return pWndData->drawColor;
}


// This function returns the user-defined fill pattern in the 8-byte area
// specified by pattern.
//
void getfillpattern(char *pattern)
{
	WindowData* pWndData = BGI__GetWindowDataPtr();

	memcpy(pattern, pWndData->uPattern, sizeof(pWndData->uPattern));
}


// This function returns the current fill settings.
//
void getfillsettings(fillsettingstype *fillinfo)
{
	WindowData* pWndData = BGI__GetWindowDataPtr();
	*fillinfo = pWndData->fillInfo;
}


// This function returns the current line settings.
//
void getlinesettings(linesettingstype *lineinfo)
{
	WindowData* pWndData = BGI__GetWindowDataPtr();
	*lineinfo = pWndData->lineInfo;
}


// This function returns the highest color possible in the current graphics
// mode.  For WinBGI, this is always WHITE (15), even though larger RGB
// colors are possible.
//
int getmaxcolor()
{
	return WHITE;
}


// This function returns the maximum x screen coordinate.
//
int getmaxx()
{
	WindowData* pWndData = BGI__GetWindowDataPtr();
	return pWndData->width - 1;
}


// This function returns the maximum y screen coordinate.
int getmaxy()
{
	WindowData* pWndData = BGI__GetWindowDataPtr();
	return pWndData->height - 1;
}

// This function returns the maximum height of a window for the current screen
int getmaxheight()
{
	int CaptionHeight = GetSystemMetrics(SM_CYCAPTION);   // Height of caption area
	int yBorder = GetSystemMetrics(SM_CYFIXEDFRAME);      // Height of border
	int TotalHeight = GetSystemMetrics(SM_CYSCREEN);      // Height of screen

	return TotalHeight - (CaptionHeight + 2 * yBorder);       // Calculate max height
}

// This function returns the maximum width of a window for the current screen
int getmaxwidth()
{
	int xBorder = GetSystemMetrics(SM_CXFIXEDFRAME);      // Width of border
	int TotalWidth = GetSystemMetrics(SM_CXSCREEN);       // Width of screen

	return TotalWidth - (2 * xBorder);       // Calculate max width
}

// This function returns the total window height, including borders
int getwindowheight()
{
	WindowData* pWndData = BGI__GetWindowDataPtr();
	int CaptionHeight = GetSystemMetrics(SM_CYCAPTION);   // Height of caption area
	int yBorder = GetSystemMetrics(SM_CYFIXEDFRAME);      // Height of border

	return pWndData->height + CaptionHeight + 2 * yBorder;    // Calculate total height
}

// This function returns the total window width, including borders
int getwindowwidth()
{
	WindowData* pWndData = BGI__GetWindowDataPtr();
	int xBorder = GetSystemMetrics(SM_CXFIXEDFRAME);      // Width of border

	return pWndData->width + 2 * xBorder;                     // Calculate total width
}

// MGM: Function to convert rgb values to a color that can be
// used with any bgi functions.  Numbers 0 to WHITE are the
// original bgi colors. Other colors are 0x03rrggbb.
// This used to be a macro.
int COLOR(int r, int g, int b)
{
	COLORREF color = RGB(r, g, b);
	int i;

	for (i = 0; i <= WHITE; i++)
	{
		if (color == BGI__Colors[i])
			return i;
	}

	return (0x03000000 | color);
}

int getdisplaycolor(int color)
{
	int save = getpixel(0, 0);
	int answer;

	putpixel(0, 0, color);
	answer = getpixel(0, 0);
	putpixel(0, 0, save);
	return answer;
}

int getpixel(int x, int y)
{
	HDC hDC = BGI__GetWinbgiDC();
	COLORREF color = GetPixel(hDC, x, y);
	BGI__ReleaseWinbgiDC();
	int i;

	if (color == CLR_INVALID)
		return CLR_INVALID;

	// If the color is a BGI color, return the index rather than the RGB value.
	for (i = 0; i <= WHITE; i++)
	{
		if (color == BGI__Colors[i])
			return i;
	}

	// If we got here, the color didn't match a BGI color.  Thus, convert to 
	// our RGB format.
	color |= 0x03000000;

	return color;
}


void getviewsettings(viewporttype *viewport)
{
	WindowData* pWndData = BGI__GetWindowDataPtr();
	*viewport = pWndData->viewportInfo;
}


// This function returns the x-cordinate of the current graphics position.
//
int getx()
{
	HDC hDC = BGI__GetWinbgiDC();
	POINT cp;

	GetCurrentPositionEx(hDC, &cp);
	BGI__ReleaseWinbgiDC();
	return cp.x;
}


// This function returns the y-cordinate of the current graphics position.
//
int gety()
{
	HDC hDC = BGI__GetWinbgiDC();
	POINT cp;

	GetCurrentPositionEx(hDC, &cp);
	BGI__ReleaseWinbgiDC();
	return cp.y;
}


// This function moves the current postion by dx pixels in the x direction and
// dy pixels in the y direction.
//
void moverel(int dx, int dy)
{
	HDC hDC = BGI__GetWinbgiDC();
	POINT cp;

	// Get the current position
	GetCurrentPositionEx(hDC, &cp);
	// Move to the new posotion
	MoveToEx(hDC, cp.x + dx, cp.y + dy, NULL);
	BGI__ReleaseWinbgiDC();
}


// This function moves the current point to position (x,y)
//
void moveto(int x, int y)
{
	HDC hDC = BGI__GetWinbgiDC();

	MoveToEx(hDC, x, y, NULL);
	BGI__ReleaseWinbgiDC();
}


void setbkcolor(int color)
{
	WindowData* pWndData = BGI__GetWindowDataPtr();

	pWndData->bgColor = color;

	// Convert from BGI color to RGB color
	color = converttorgb(color);

	WaitForSingleObject(pWndData->hDCMutex, 5000);
	for (int i = 0; i < MAX_PAGES; i++)
		SetBkColor(pWndData->hDC[i], color);
	ReleaseMutex(pWndData->hDCMutex);
}


void setcolor(int color)
{
	WindowData* pWndData = BGI__GetWindowDataPtr();

	// Update the color in our structure
	pWndData->drawColor = color;

	// Convert from BGI color to RGB color
	color = converttorgb(color);

	// Use that to set the text color for each page
	WaitForSingleObject(pWndData->hDCMutex, 5000);
	for (int i = 0; i < MAX_PAGES; i++)
		SetTextColor(pWndData->hDC[i], color);
	ReleaseMutex(pWndData->hDCMutex);

	// Create the new drawing pen
	CreateNewPen();
}


void setlinestyle(int linestyle, unsigned upattern, int thickness)
{
	WindowData* pWndData = BGI__GetWindowDataPtr();
	pWndData->lineInfo.linestyle = linestyle;
	pWndData->lineInfo.upattern = upattern;
	pWndData->lineInfo.thickness = thickness;

	// Create the new drawing pen
	CreateNewPen();
}


// The user calls this function to create a brush with a pattern they create
//
void setfillpattern(char *upattern, int color)
{
	WindowData* pWndData = BGI__GetWindowDataPtr();
	HBITMAP hBitmap;
	HBRUSH hBrush;
	unsigned short pattern[8];
	int i;

	// Copy the pattern to the storage for the window
	memcpy(pWndData->uPattern, upattern, sizeof(pWndData->uPattern));

	// Convert the pattern to create a brush
	for (i = 0; i < 8; i++)
		pattern[i] = (unsigned char)~upattern[i];       // Restrict to 8 bits

														// Set the settings for the structure
	pWndData->fillInfo.pattern = USER_FILL;
	pWndData->fillInfo.color = color;

	// Create the bitmap
	hBitmap = CreateBitmap(8, 8, 1, 1, pattern);
	// Create a brush for each DC
	WaitForSingleObject(pWndData->hDCMutex, 5000);
	for (int i = 0; i < MAX_PAGES; i++)
	{
		hBrush = CreatePatternBrush(hBitmap);
		// Select the new brush into the device context and delete the old one.
		DeleteBrush((HBRUSH)SelectBrush(pWndData->hDC[i], hBrush));
	}
	ReleaseMutex(pWndData->hDCMutex);
	// I'm not sure if it's safe to delete the bitmap here or not, but it
	// hasn't caused any problems.  The material I've found just says the
	// bitmap must be deleted in addition to the brush when finished.
	DeleteBitmap(hBitmap);

}


// If the USER_FILL pattern is passed, nothing is changed.
//
void setfillstyle(int pattern, int color)
{
	WindowData* pWndData = BGI__GetWindowDataPtr();
	HDC hDC = BGI__GetWinbgiDC();
	HBRUSH hBrush;
	// Unsigned char creates a truncation for some reason.
	unsigned short Slash[8] = { (unsigned char)~0xE0, (unsigned char)~0xC1, (unsigned char)~0x83, (unsigned char)~0x07, (unsigned char)~0x0E, (unsigned char)~0x1C, (unsigned char)~0x38, (unsigned char)~0x70 };
	unsigned short BkSlash[8] = { (unsigned char)~0x07, (unsigned char)~0x83, (unsigned char)~0xC1, (unsigned char)~0xE0, (unsigned char)~0x70, (unsigned char)~0x38, (unsigned char)~0x1C, (unsigned char)~0x0E };
	unsigned short Interleave[8] = { (unsigned char)~0xCC, (unsigned char)~0x33, (unsigned char)~0xCC, (unsigned char)~0x33, (unsigned char)~0xCC, (unsigned char)~0x33, (unsigned char)~0xCC, (unsigned char)~0x33 };
	unsigned short WideDot[8] = { (unsigned char)~0x80, (unsigned char)~0x00, (unsigned char)~0x08, (unsigned char)~0x00, (unsigned char)~0x80, (unsigned char)~0x00, (unsigned char)~0x08, (unsigned char)~0x00 };
	unsigned short CloseDot[8] = { (unsigned char)~0x88, (unsigned char)~0x00, (unsigned char)~0x22, (unsigned char)~0x00, (unsigned char)~0x88, (unsigned char)~0x00, (unsigned char)~0x22, (unsigned char)~0x00 };
	HBITMAP hBitmap;

	// Convert from BGI color to RGB color
	color = converttorgb(color);

	switch (pattern)
	{
	case EMPTY_FILL:
		hBrush = CreateSolidBrush(converttorgb(pWndData->bgColor));
		break;
	case SOLID_FILL:
		hBrush = CreateSolidBrush(color);
		break;
	case LINE_FILL:
		hBrush = CreateHatchBrush(HS_HORIZONTAL, color);
		break;
	case LTSLASH_FILL:
		hBrush = CreateHatchBrush(HS_BDIAGONAL, color);
		break;
	case SLASH_FILL:
		// The colors of the monochrome bitmap are drawn using the text color
		// and the current background color.
		// TODO: We may have to set the text color in every fill function
		// and then reset the text color to the user-specified text color
		// after the fill-draw routines are complete.
		hBitmap = CreateBitmap(8, 8, 1, 1, Slash);
		hBrush = CreatePatternBrush(hBitmap);
		DeleteBitmap(hBitmap);
		break;
	case BKSLASH_FILL:
		hBitmap = CreateBitmap(8, 8, 1, 1, BkSlash);
		hBrush = CreatePatternBrush(hBitmap);
		DeleteBitmap(hBitmap);
		break;
	case LTBKSLASH_FILL:
		hBrush = CreateHatchBrush(HS_FDIAGONAL, color);
		break;
	case HATCH_FILL:
		hBrush = CreateHatchBrush(HS_CROSS, color);
		break;
	case XHATCH_FILL:
		hBrush = CreateHatchBrush(HS_DIAGCROSS, color);
		break;
	case INTERLEAVE_FILL:
		hBitmap = CreateBitmap(8, 8, 1, 1, Interleave);
		hBrush = CreatePatternBrush(hBitmap);
		DeleteBitmap(hBitmap);
		break;
	case WIDE_DOT_FILL:
		hBitmap = CreateBitmap(8, 8, 1, 1, WideDot);
		hBrush = CreatePatternBrush(hBitmap);
		DeleteBitmap(hBitmap);
		break;
	case CLOSE_DOT_FILL:
		hBitmap = CreateBitmap(8, 8, 1, 1, CloseDot);
		hBrush = CreatePatternBrush(hBitmap);
		DeleteBitmap(hBitmap);
		break;
	case USER_FILL:
		return;
		break;
	default:
		pWndData->error_code = grError;
		return;
	}

	// TODO: Modify this so the brush is created in every DC
	pWndData->fillInfo.pattern = pattern;
	pWndData->fillInfo.color = color;

	// Select the new brush into the device context and delete the old one.
	DeleteBrush((HBRUSH)SelectBrush(hDC, hBrush));
	BGI__ReleaseWinbgiDC();
}


void setviewport(int left, int top, int right, int bottom, int clip)
{
	WindowData* pWndData = BGI__GetWindowDataPtr();
	HRGN hRGN = NULL;

	// Store the viewport information in the structure
	pWndData->viewportInfo.left = left;
	pWndData->viewportInfo.top = top;
	pWndData->viewportInfo.right = right;
	pWndData->viewportInfo.bottom = bottom;
	pWndData->viewportInfo.clip = clip;

	// If the drwaing should be clipped at the viewport boundary, create a
	// clipping region
	if (clip != 0)
		hRGN = CreateRectRgn(left, top, right, bottom);

	WaitForSingleObject(pWndData->hDCMutex, 5000);
	for (int i = 0; i < MAX_PAGES; i++)
	{
		SelectClipRgn(pWndData->hDC[i], hRGN);

		// Set the viewport origin to be the upper left corner
		SetViewportOrgEx(pWndData->hDC[i], left, top, NULL);

		// Move to the new origin
		MoveToEx(pWndData->hDC[i], 0, 0, NULL);
	}
	ReleaseMutex(pWndData->hDCMutex);
	// A copy of the region is used for the clipping region, so it is
	// safe to delete the region  (p. 369 Win32 API book)
	DeleteRgn(hRGN);
}


void setwritemode(int mode)
{
	HDC hDC = BGI__GetWinbgiDC();

	if (mode == COPY_PUT)
		SetROP2(hDC, R2_COPYPEN);
	if (mode == XOR_PUT)
		SetROP2(hDC, R2_XORPEN);
	BGI__ReleaseWinbgiDC();
}

// $Id: mouse.cpp,v 1.1 2003/05/06 03:32:35 bush Exp $
// Written by:
//      Grant Macklem (Grant.Macklem@colorado.edu)
//      Gregory Schmelter (Gregory.Schmelter@colorado.edu)
//      Alan Schmidt (Alan.Schmidt@colorado.edu)
//      Ivan Stashak (Ivan.Stashak@colorado.edu)
// CSCI 4830/7818: API Programming
// University of Colorado at Boulder, Spring 2003
// http://www.cs.colorado.edu/~main/bgi
//


/*****************************************************************************
*
*   Helper functions
*
*****************************************************************************/
// This function tests whether a given kind of mouse event is in range
// MGM: Added static to prevent linker conflicts
static bool MouseKindInRange(int kind)
{
	return ((kind >= WM_MOUSEFIRST) && (kind <= WM_MOUSELAST));
}


/*****************************************************************************
*
*   The actual API calls are implemented below
*   MGM: Moved ismouseclick function to top to stop g++ 3.2.3 internal error.
*****************************************************************************/
bool ismouseclick(int kind)
{
	WindowData *pWndData = BGI__GetWindowDataPtr();
	return (MouseKindInRange(kind) && pWndData->clicks[kind - WM_MOUSEFIRST].size());
}

void clearmouseclick(int kind)
{
	WindowData *pWndData = BGI__GetWindowDataPtr();

	// Clear the mouse event
	if (MouseKindInRange(kind) && pWndData->clicks[kind - WM_MOUSEFIRST].size())
		pWndData->clicks[kind - WM_MOUSEFIRST].pop();
}

void getmouseclick(int kind, int& x, int& y)
{
	WindowData *pWndData = BGI__GetWindowDataPtr();
	POINTS where; // POINT (short) to tell where mouse event happened.

				  // Check if mouse event is in range
	if (!MouseKindInRange(kind))
		return;

	// Set position variables to mouse location, or to NO_CLICK if no event occured
	if (MouseKindInRange(kind) && pWndData->clicks[kind - WM_MOUSEFIRST].size())
	{
		where = pWndData->clicks[kind - WM_MOUSEFIRST].front();
		pWndData->clicks[kind - WM_MOUSEFIRST].pop();
		x = where.x;
		y = where.y;
	}
	else
	{
		x = y = NO_CLICK;
	}
}

void setmousequeuestatus(int kind, bool status)
{
	if (MouseKindInRange(kind))
		BGI__GetWindowDataPtr()->mouse_queuing[kind - WM_MOUSEFIRST] = status;
}

// TODO: This may be viewport relative.  The documentation specifies with will range from 0 to getmaxx()
int mousex()
{
	WindowData *pWndData = BGI__GetWindowDataPtr();
	return pWndData->mouse.x;
}


// TODO: This may be viewport relative.  The documentation specifies with will range from 0 to getmaxy()
int mousey()
{
	WindowData *pWndData = BGI__GetWindowDataPtr();
	return pWndData->mouse.y;
}


void registermousehandler(int kind, void h(int, int))
{
	WindowData *pWndData = BGI__GetWindowDataPtr();
	if (MouseKindInRange(kind))
		pWndData->mouse_handlers[kind - WM_MOUSEFIRST] = h;
}


// $Id: palette.cpp,v 1.1 2003/07/08 03:42:28 bush Exp $
// Written by:
//      Grant Macklem (Grant.Macklem@colorado.edu)
//      Gregory Schmelter (Gregory.Schmelter@colorado.edu)
//      Alan Schmidt (Alan.Schmidt@colorado.edu)
//      Ivan Stashak (Ivan.Stashak@colorado.edu)
// CSCI 4830/7818: API Programming
// University of Colorado at Boulder, Spring 2003
// http://www.cs.colorado.edu/~main/bgi
//

/*****************************************************************************
*
*   Helper functions
*
*****************************************************************************/


/*****************************************************************************
*
*   The actual API calls are implemented below
*
*****************************************************************************/
palettetype *getdefaultpalette()
{
	static palettetype default_palette = { 16,
	{ BLACK, BLUE, GREEN, CYAN, RED, MAGENTA, BROWN, LIGHTGRAY,
		DARKGRAY, LIGHTBLUE, LIGHTGREEN, LIGHTCYAN, LIGHTRED,
		LIGHTMAGENTA, YELLOW, WHITE } };

	return &default_palette;
}


void getpalette(palettetype *palette)
{

}


int getpalettesize()
{
	return MAXCOLORS + 1;
}


void setallpalette(palettetype *palette)
{

}


void setpalette(int colornum, int color)
{

}


void setrgbpalette(int colornum, int red, int green, int blue)
{

}

// File: text.cpp
// Written by:
//      Grant Macklem (Grant.Macklem@colorado.edu)
//      Gregory Schmelter (Gregory.Schmelter@colorado.edu)
//      Alan Schmidt (Alan.Schmidt@colorado.edu)
//      Ivan Stashak (Ivan.Stashak@colorado.edu)
// CSCI 4830/7818: API Programming
// University of Colorado at Boulder, Spring 2003
// http://www.cs.colorado.edu/~main/bgi
//
// This file contains the code necessary to draw/modify text
//



/*****************************************************************************
*
*   Some very useful arrays -- Same as previous version for consistency
*   Also, the exported definition of bgiout.
*
*****************************************************************************/
std::ostringstream bgiout;
static int font_weight[] =
{
	FW_BOLD,    // DefaultFont
	FW_NORMAL,  // TriplexFont
	FW_NORMAL,  // SmallFont
	FW_NORMAL,  // SansSerifFont
	FW_NORMAL,  // GothicFont
	FW_NORMAL,  // ScriptFont
	FW_NORMAL,  // SimplexFont
	FW_NORMAL,  // TriplexScriptFont
	FW_NORMAL,  // ComplexFont
	FW_NORMAL,  // EuropeanFont
	FW_BOLD     // BoldFont
};
static int font_family[] =
{
	FIXED_PITCH | FF_DONTCARE,     // DefaultFont
	VARIABLE_PITCH | FF_ROMAN,     // TriplexFont
	VARIABLE_PITCH | FF_MODERN,    // SmallFont
	VARIABLE_PITCH | FF_DONTCARE,  // SansSerifFont
	VARIABLE_PITCH | FF_SWISS,     // GothicFont
	VARIABLE_PITCH | FF_SCRIPT,    // ScriptFont
	VARIABLE_PITCH | FF_DONTCARE,  // SimplexFont
	VARIABLE_PITCH | FF_SCRIPT,    // TriplexScriptFont
	VARIABLE_PITCH | FF_DONTCARE,  // ComplexFont
	VARIABLE_PITCH | FF_DONTCARE,  // EuropeanFont
	VARIABLE_PITCH | FF_DONTCARE   // BoldFont
};
static char* font_name[] =
{
	"Console",          // DefaultFont
	"Times New Roman",  // TriplexFont
	"Small Fonts",      // SmallFont
	"MS Sans Serif",    // SansSerifFont
	"Arial",            // GothicFont
	"Script",           // ScriptFont
	"Times New Roman",  // SimplexFont
	"Script",           // TriplexScriptFont
	"Courier New",      // ComplexFont
	"Times New Roman",  // EuropeanFont
	"Courier New Bold", // BoldFont
};

static struct { int width; int height; } font_metrics[][11] = {
	{ { 0,0 },{ 8,8 },{ 16,16 },{ 24,24 },{ 32,32 },{ 40,40 },{ 48,48 },{ 56,56 },{ 64,64 },{ 72,72 },{ 80,80 } }, // DefaultFont
	{ { 0,0 },{ 13,18 },{ 14,20 },{ 16,23 },{ 22,31 },{ 29,41 },{ 36,51 },{ 44,62 },{ 55,77 },{ 66,93 },{ 88,124 } }, // TriplexFont
	{ { 0,0 },{ 3,5 },{ 4,6 },{ 4,6 },{ 6,9 },{ 8,12 },{ 10,15 },{ 12,18 },{ 15,22 },{ 18,27 },{ 24,36 } }, // SmallFont
	{ { 0,0 },{ 11,19 },{ 12,21 },{ 14,24 },{ 19,32 },{ 25,42 },{ 31,53 },{ 38,64 },{ 47,80 },{ 57,96 },{ 76,128 } }, // SansSerifFont
	{ { 0,0 },{ 13,19 },{ 14,21 },{ 16,24 },{ 22,32 },{ 29,42 },{ 36,53 },{ 44,64 },{ 55,80 },{ 66,96 },{ 88,128 } }, // GothicFont

																													  // These may not be 100% correct
	{ { 0,0 },{ 11,19 },{ 12,21 },{ 14,24 },{ 19,32 },{ 25,42 },{ 31,53 },{ 38,64 },{ 47,80 },{ 57,96 },{ 76,128 } }, // ScriptFont
	{ { 0,0 },{ 11,19 },{ 12,21 },{ 14,24 },{ 19,32 },{ 25,42 },{ 31,53 },{ 38,64 },{ 47,80 },{ 57,96 },{ 76,128 } }, // SimplexFont
	{ { 0,0 },{ 13,18 },{ 14,20 },{ 16,23 },{ 22,31 },{ 29,41 },{ 36,51 },{ 44,62 },{ 55,77 },{ 66,93 },{ 88,124 } }, // TriplexScriptFont
	{ { 0,0 },{ 11,19 },{ 12,21 },{ 14,24 },{ 19,32 },{ 25,42 },{ 31,53 },{ 38,64 },{ 47,80 },{ 57,96 },{ 76,128 } }, // ComplexFont
	{ { 0,0 },{ 11,19 },{ 12,21 },{ 14,24 },{ 19,32 },{ 25,42 },{ 31,53 },{ 38,64 },{ 47,80 },{ 57,96 },{ 76,128 } }, // EuropeanFont
	{ { 0,0 },{ 11,19 },{ 12,21 },{ 14,24 },{ 19,32 },{ 25,42 },{ 31,53 },{ 38,64 },{ 47,80 },{ 57,96 },{ 76,128 } } // BoldFont
};

// horiz     LEFT_TEXT   0  left-justify text  
//           CENTER_TEXT 1  center text  
//           RIGHT_TEXT  2  right-justify text  
// vertical  BOTTOM_TEXT 0  bottom-justify text  
//           CENTER_TEXT 1  center text  
//           TOP_TEXT    2  top-justify text 
static UINT horiz_align[3] = { TA_LEFT, TA_CENTER, TA_RIGHT };
static UINT vert_align[3] = { TA_BOTTOM, TA_BASELINE, TA_TOP };

/*****************************************************************************
*
*   Some helper functions
*
*****************************************************************************/

// This function 
// POSTCONDITION:
//
static void set_align(WindowData* pWndData)
{
	UINT alignment = pWndData->alignment;

	// if we are vertical, things are swapped.
	if (pWndData->textInfo.direction == HORIZ_DIR)
	{
		alignment |= horiz_align[pWndData->textInfo.horiz];
		alignment |= vert_align[pWndData->textInfo.vert];
	}
	else
	{
		alignment |= horiz_align[pWndData->textInfo.vert];
		alignment |= vert_align[pWndData->textInfo.horiz];
	}

	// set the alignment for all valid pages.
	for (int i = 0; i < MAX_PAGES; i++)
		SetTextAlign(pWndData->hDC[i], alignment);
}

// This function updates the current hdc with the user defined font
// POSTCONDITION: text written to the current hdc will be in the new font
//
static void set_font(WindowData* pWndData)
{
	int mindex;
	double xscale, yscale;
	HFONT hFont;

	// get the scaling factors based on charsize
	if (pWndData->textInfo.charsize == 0)
	{
		xscale = pWndData->t_scale[0] / pWndData->t_scale[1];
		yscale = pWndData->t_scale[2] / pWndData->t_scale[3];

		// if font zero, only use factors.. else also multiply by 4
		if (pWndData->textInfo.font == 0)
			mindex = 0;
		else
			mindex = 4;
	}
	else
	{
		xscale = 1.0;
		yscale = 1.0;
		mindex = pWndData->textInfo.charsize;
	}

	// with the scaling decided, make a font.
	hFont = CreateFont(
		int(font_metrics[pWndData->textInfo.font][mindex].height * yscale),
		int(font_metrics[pWndData->textInfo.font][mindex].width  * xscale),
		pWndData->textInfo.direction * 900,
		(pWndData->textInfo.direction & 1) * 900,
		font_weight[pWndData->textInfo.font],
		FALSE,
		FALSE,
		FALSE,
		DEFAULT_CHARSET,
		OUT_DEFAULT_PRECIS,
		CLIP_DEFAULT_PRECIS,
		DEFAULT_QUALITY,
		font_family[pWndData->textInfo.font],
		font_name[pWndData->textInfo.font]
		);

	// assign the fonts to each of the hdcs
	for (int i = 0; i < MAX_PAGES; i++)
		SelectObject(pWndData->hDC[i], hFont);
}


/*****************************************************************************
*
*   The actual API calls are implemented below
*
*****************************************************************************/
// This function fills the textsettingstype structure pointed to by textinfo
// with information about the current text font, direction, size, and
// justification.
// POSTCONDITION: texttypeinfo has been filled with the proper information
//
void gettextsettings(struct textsettingstype *texttypeinfo)
{
	// if its null, leave.
	if (!texttypeinfo)
		return;

	WindowData* pWndData = BGI__GetWindowDataPtr();

	*texttypeinfo = pWndData->textInfo;
}

// This function prints textstring to the screen at the current position
// POSTCONDITION: text has been written to the screen using the current font,
//                direction, and size.  In addition, the current position has
//                been modified to reflect the text that was just output.
//
void outtext(char *textstring)
{
	HDC hDC = BGI__GetWinbgiDC();
	WindowData* pWndData = BGI__GetWindowDataPtr();

	// so we can clear the screen
	POINT cp;
	GetCurrentPositionEx(hDC, &cp);

	// check cp alignment
	if (pWndData->alignment != TA_UPDATECP)
	{
		pWndData->alignment = TA_UPDATECP;
		set_align(pWndData);
	}

	TextOut(hDC, 0, 0, (LPCTSTR)textstring, strlen(textstring));
	BGI__ReleaseWinbgiDC();
	RefreshWindow(NULL);
	// Todo: Change to refresh only the needed part.
}

// This function prints textstring to x,y
// POSTCONDITION: text has been written to the screen using the current font,
//                direction, and size. If a string is printed with the default
//                font using outtext or outtextxy, any part of the string that
//                extends outside the current viewport is truncated.
//
void outtextxy(int x, int y, char *textstring)
{
	HDC hDC = BGI__GetWinbgiDC();
	WindowData* pWndData = BGI__GetWindowDataPtr();

	// check alignment
	if (pWndData->alignment != TA_NOUPDATECP)
	{
		pWndData->alignment = TA_NOUPDATECP;
		set_align(pWndData);
	}

	TextOut(hDC, x, y, (LPCTSTR)textstring, strlen(textstring));
	BGI__ReleaseWinbgiDC();

	RefreshWindow(NULL);
	// Todo: Change to refresh only the needed part.
}



// This function sets the vertical and horizontal justification based on CP
// POSTCONDITION: Text output is justified around the current position as
//                has been specified.
//
void settextjustify(int horiz, int vert)
{
	WindowData* pWndData = BGI__GetWindowDataPtr();

	pWndData->textInfo.horiz = horiz;
	pWndData->textInfo.vert = vert;

	WaitForSingleObject(pWndData->hDCMutex, 5000);
	set_align(pWndData);
	ReleaseMutex(pWndData->hDCMutex);
}


// This function sets the font and it's properties that will be used
// by all text output related functions.
// POSTCONDITION: text output after a call to settextstyle should be
//                in the newly specified format.
//
void settextstyle(int font, int direction, int charsize)
{
	WindowData* pWndData = BGI__GetWindowDataPtr();

	pWndData->textInfo.font = font;
	pWndData->textInfo.direction = direction;
	pWndData->textInfo.charsize = charsize;

	WaitForSingleObject(pWndData->hDCMutex, 5000);
	set_font(pWndData);
	ReleaseMutex(pWndData->hDCMutex);
}

// This function sets the size of stroked fonts
// POSTCONDITION: these values will be used when charsize is zero in the
//                settextstyle assignments.  consequently output text will
//                be scaled by the appropriate x and y values when output.
//
void setusercharsize(int multx, int divx, int multy, int divy)
{
	WindowData* pWndData = BGI__GetWindowDataPtr();
	pWndData->t_scale[0] = multx;
	pWndData->t_scale[1] = divx;
	pWndData->t_scale[2] = multy;
	pWndData->t_scale[3] = divy;

	WaitForSingleObject(pWndData->hDCMutex, 5000);
	set_font(pWndData);
	ReleaseMutex(pWndData->hDCMutex);
}

// This function returns the height in pixels of textstring using the current
// text output settings.
// POSTCONDITION: the height of the string in pixels has been returned.
//
int textheight(char *textstring)
{
	HDC hDC = BGI__GetWinbgiDC();
	SIZE tb;

	GetTextExtentPoint32(hDC, (LPCTSTR)textstring, strlen(textstring), &tb);
	BGI__ReleaseWinbgiDC();

	return tb.cy;
}

// This function returns the width in pixels of textstring using the current
// text output settings.
// POSTCONDITION: the width of the string in pixels has been returned.
//
int textwidth(char *textstring)
{
	HDC hDC = BGI__GetWinbgiDC();
	SIZE tb;

	GetTextExtentPoint32(hDC, (LPCTSTR)textstring, strlen(textstring), &tb);
	BGI__ReleaseWinbgiDC();

	return tb.cx;
}

void outstreamxy(int x, int y, std::ostringstream& out)
{
	std::string all, line;
	int i;
	int startx = x;

	all = out.str();
	out.str("");

	moveto(x, y);
	for (i = 0; i < (int)all.length(); i++)
	{
		if (all[i] == '\n')
		{
			if (line.length() > 0)
				outtext((char *)line.c_str());
			y += textheight("X");
			x = startx;
			line.clear();
			moveto(x, y);
		}
		else
			line += all[i];
	}
	if (line.length() > 0)
		outtext((char *)line.c_str());
}

void outstream(std::ostringstream& out)
{
	outstreamxy(getx(), gety(), out);
}

// File: winbgi.cpp
// Written by:
//      Grant Macklem (Grant.Macklem@colorado.edu)
//      Gregory Schmelter (Gregory.Schmelter@colorado.edu)
//      Alan Schmidt (Alan.Schmidt@colorado.edu)
//      Ivan Stashak (Ivan.Stashak@colorado.edu)
// CSCI 4830/7818: API Programming
// University of Colorado at Boulder, Spring 2003
// http://www.cs.colorado.edu/~main/bgi
//

// The window message-handling function (in WindowThread.cpp)
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);


// This function is called whenever a process attaches itself to the DLL.
// Thus, it registers itself in the process' address space.
//
BOOL registerWindowClass()
{
	WNDCLASSEX wcex;                        // The main window class that we create

											// Set up the properties of the Windows BGI window class and register it
	wcex.cbSize = sizeof(WNDCLASSEX);     // Size of the strucutre
	wcex.style = CS_SAVEBITS | CS_DBLCLKS; // Use default class styles
	wcex.lpfnWndProc = WndProc;             // The message handling function
	wcex.cbClsExtra = 0;                    // No extra memory allocation
	wcex.cbWndExtra = 0;
	wcex.hInstance = BGI__hInstance;        // HANDLE for this program
	wcex.hIcon = 0;                         // Use default icon
	wcex.hIconSm = 0;                       // Default small icon
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);       // Set mouse cursor
	wcex.hbrBackground = GetStockBrush(BLACK_BRUSH);  // Background color
	wcex.lpszMenuName = 0;                  // Menu identification
	wcex.lpszClassName = _T("BGILibrary");// a c-string name for the window

	if (RegisterClassEx(&wcex) == 0)
	{
		showerrorbox();
		return FALSE;
	}
	return TRUE;
}


// This function unregisters the window class so that it can be registered
// again if using LoadLibrary and FreeLibrary
void unregisterWindowClass()
{
	UnregisterClass("BGILibrary", BGI__hInstance);
}


// This is the entry point for the DLL
// MGM: I changed it to a regular function rather than a DLL entry
// point (since I deleted the DLL).  As such, it is now called
// by initwindow.
bool DllMain_MGM(
	HINSTANCE hinstDll,   // Handle to DLL module
	DWORD Reason,         // Reason for calling function
	LPVOID Reserved       // reserved
	)
{
	// MGM: Add a static variable so that this work is done but once.
	static bool called = false;
	if (called) return true;
	called = true;

	switch (Reason)
	{
	case DLL_PROCESS_ATTACH:
		BGI__hInstance = hinstDll;                   // Set the global hInstance variable
		return (registerWindowClass()) !=0 ;          // Register the window class for this process
		break;
	case DLL_PROCESS_DETACH:
		unregisterWindowClass();
		break;
	}
	return TRUE;        // Ignore other initialization cases
}



void showerrorbox(const char* msg)
{
	LPVOID lpMsgBuf;

	if (msg == NULL)
	{
		// This code is taken from the MSDN help for FormatMessage
		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,          // Formatting options
			NULL,                                   // Location of message definiton
			GetLastError(),                        // Message ID
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			(LPTSTR)&lpMsgBuf,                     // Pointer to buffer
			0,                                      // Minimum size of buffer
			NULL);

		MessageBox(NULL, (LPCTSTR)lpMsgBuf, "Error", MB_OK | MB_ICONERROR);

		// Free the buffer
		LocalFree(lpMsgBuf);
	}
	else
		MessageBox(NULL, msg, "Error", MB_OK | MB_ICONERROR);
}


/*****************************************************************************
*
*   The actual API calls are implemented below
*   MGM: Rearranged order of functions by trial-and-error until internal
*   error in g++ 3.2.3 disappeared.
*****************************************************************************/


void graphdefaults()
{
	WindowData *pWndData = BGI__GetWindowDataPtr();
	HPEN hPen;      // The default drawing pen
	HBRUSH hBrush;  // The default filling brush
//	int bgi_color;                      // A bgi color number
//	COLORREF actual_color;              // The color that's actually put on the screen
	HDC hDC;

	// TODO: Do this for each DC

	// Set viewport to the entire screen and move current position to (0,0)
	setviewport(0, 0, pWndData->width, pWndData->height, 0);
	pWndData->mouse.x = 0;
	pWndData->mouse.y = 0;

	// Turn on autorefreshing
	pWndData->refreshing = true;

	// MGM: Grant suggested the following colors, but I have changed
	// them so that they will be consistent on any 16-color VGA
	// display.  It's also important to use 255-255-255 for white
	// to match some stock tools such as the stock WHITE brush
	/*
	BGI__Colors[0] = RGB( 0, 0, 0 );         // Black
	BGI__Colors[1] = RGB( 0, 0, 168);        // Blue
	BGI__Colors[2] = RGB( 0, 168, 0 );       // Green
	BGI__Colors[3] = RGB( 0, 168, 168 );     // Cyan
	BGI__Colors[4] = RGB( 168, 0, 0 );       // Red
	BGI__Colors[5] = RGB( 168, 0, 168 );     // Magenta
	BGI__Colors[6] = RGB( 168, 84, 0 );      // Brown
	BGI__Colors[7] = RGB( 168, 168, 168 );   // Light Gray
	BGI__Colors[8] = RGB( 84, 84, 84 );      // Dark Gray
	BGI__Colors[9] = RGB( 84, 84, 252 );     // Light Blue
	BGI__Colors[10] = RGB( 84, 252, 84 );    // Light Green
	BGI__Colors[11] = RGB( 84, 252, 252 );   // Light Cyan
	BGI__Colors[12] = RGB( 252, 84, 84 );    // Light Red
	BGI__Colors[13] = RGB( 252, 84, 252 );   // Light Magenta
	BGI__Colors[14] = RGB( 252, 252, 84 );   // Yellow
	BGI__Colors[15] = RGB( 252, 252, 252 );  // White
	for (bgi_color = 0; bgi_color <= WHITE; bgi_color++)
	{
	putpixel(0, 0, bgi_color);
	actual_color = GetPixel(hDC, 0, 0);
	BGI__Colors[bgi_color] = actual_color;
	}
	*/

	BGI__Colors[0] = RGB(0, 0, 0);         // Black
	BGI__Colors[1] = RGB(0, 0, 128);        // Blue
	BGI__Colors[2] = RGB(0, 128, 0);       // Green
	BGI__Colors[3] = RGB(0, 128, 128);     // Cyan
	BGI__Colors[4] = RGB(128, 0, 0);       // Red
	BGI__Colors[5] = RGB(128, 0, 128);     // Magenta
	BGI__Colors[6] = RGB(128, 128, 0);     // Brown
	BGI__Colors[7] = RGB(192, 192, 192);   // Light Gray
	BGI__Colors[8] = RGB(128, 128, 128);   // Dark Gray
	BGI__Colors[9] = RGB(128, 128, 255);   // Light Blue
	BGI__Colors[10] = RGB(128, 255, 128);  // Light Green
	BGI__Colors[11] = RGB(128, 255, 255);  // Light Cyan
	BGI__Colors[12] = RGB(255, 128, 128);  // Light Red
	BGI__Colors[13] = RGB(255, 128, 255);  // Light Magenta
	BGI__Colors[14] = RGB(255, 255, 0);  // Yellow
	BGI__Colors[15] = RGB(255, 255, 255);  // White

										   // Set background color to default (black)
	setbkcolor(BLACK);

	// Set drawing color to default (white)
	pWndData->drawColor = WHITE;
	// Set fill style and pattern to default (white solid)
	pWndData->fillInfo.pattern = SOLID_FILL;
	pWndData->fillInfo.color = WHITE;

	hDC = BGI__GetWinbgiDC();
	// Reset the pen and brushes for each DC
	for (int i = 0; i < MAX_PAGES; i++)
	{
		// Using Stock stuff is more efficient.
		// Create the default pen for drawing in the window.
		hPen = GetStockPen(WHITE_PEN);
		// Select this pen into the DC and delete the old pen.
		DeletePen(SelectPen(pWndData->hDC[i], hPen));

		// Create the default brush for painting in the window.
		hBrush = GetStockBrush(WHITE_BRUSH);
		// Select this brush into the DC and delete the old brush.
		DeleteBrush(SelectBrush(pWndData->hDC[i], hBrush));

		// Set the default text color for each page
		SetTextColor(pWndData->hDC[i], converttorgb(WHITE));
	}
	ReleaseMutex(pWndData->hDCMutex);

	// Set text font and justification to default
	pWndData->textInfo.horiz = LEFT_TEXT;
	pWndData->textInfo.vert = TOP_TEXT;
	pWndData->textInfo.font = DEFAULT_FONT;
	pWndData->textInfo.direction = HORIZ_DIR;
	pWndData->textInfo.charsize = 1;

	// set this something that it can never be to force set_align to be called
	pWndData->alignment = 2;

	pWndData->t_scale[0] = 1; // multx
	pWndData->t_scale[1] = 1; // divx
	pWndData->t_scale[2] = 1; // multy
	pWndData->t_scale[3] = 1; // divy

							  // Set the error code to Ok: There is no error
	pWndData->error_code = grOk;

	// Line style as well?
	pWndData->lineInfo.linestyle = SOLID_LINE;
	pWndData->lineInfo.thickness = NORM_WIDTH;

	// Set the default active and visual page
	if (pWndData->DoubleBuffer)
	{
		pWndData->ActivePage = 1;
		pWndData->VisualPage = 0;
	}
	else
	{
		pWndData->ActivePage = 0;
		pWndData->VisualPage = 0;
	}

	// Set the aspect ratios.  Unless Windows is doing something funky,
	// these should not need to be changed by the user to produce geometrically
	// correct shapes (circles, squares, etc).
	pWndData->x_aspect_ratio = 10000;
	pWndData->y_aspect_ratio = 10000;
}

using namespace std;
// The initwindow function is typicaly the first function called by the
// application.  It will create a separate thread for each window created.
// This thread is responsible for creating the window and processing all the
// messages.  It returns a positive integer value which the user can then
// use whenever he needs to reference the window.
// RETURN VALUE: If the window is successfully created, a nonnegative integer
//                  uniquely identifing the window.
//               On failure, -1.
//
int initwindow
(int width, int height, const char* title, int left, int top, bool dbflag, bool closeflag)
{
	HANDLE hThread;                     // Handle to the message pump thread
	int index;                          // Index of current window in the table
	HANDLE objects[2];                  // Handle to objects (thread and event) to ensure proper creation
	int code;                           // Return code of thread wait function

										// MGM: Call the DllMain, which used to be the DLL entry point.
	if (!DllMain_MGM(
		NULL,
		DLL_PROCESS_ATTACH,
		NULL))
		return -1;

	WindowData* pWndData = new WindowData;
	// Check if new failed
	if (pWndData == NULL)
		return -1;

	// Todo: check to make sure the events are created successfully
	pWndData->key_waiting = CreateEvent(NULL, FALSE, FALSE, NULL);
	pWndData->WindowCreated = CreateEvent(NULL, FALSE, FALSE, NULL);
	pWndData->width = width;
	pWndData->height = height;
	pWndData->initleft = left;
	pWndData->inittop = top;
	pWndData->title = title; // Converts to a string object

	hThread = CreateThread(NULL,                   // Security Attributes (use default)
		0,                      // Stack size (use default)
		BGI__ThreadInitWindow,  // Start routine
		(LPVOID)pWndData,       // Thread specific parameter
		0,                      // Creation flags (run immediately)
		&pWndData->threadID);  // Thread ID

							   // Check if the message pump thread was created
	if (hThread == NULL)
	{
		showerrorbox();
		delete pWndData;
		return -1;
	}

	// Create an array of events to wait for
	objects[0] = hThread;
	objects[1] = pWndData->WindowCreated;
	// We'll either wait to be signaled that the window was created
	// successfully or the thread will terminate and we'll have some problem.
	code = WaitForMultipleObjects(2,           // Number of objects to wait on
		objects,     // Array of objects
		FALSE,       // Whether all objects must be signaled
		INFINITE);  // How long to wait before timing out

	switch (code)
	{
	case WAIT_OBJECT_0:
		// Thread terminated without creating the window
		delete pWndData;
		return -1;
		break;
	case WAIT_OBJECT_0 + 1:
		// Successfully running
		break;
	}

	// Set index to the next available position
	index = BGI__WindowCount;
	// Increment the count
	++BGI__WindowCount;
	// Store the window in the next position of the vector
	BGI__WindowTable.push_back(pWndData->hWnd);
	// Set the current window to the newly created window
	BGI__CurrentWindow = index;

	// Set double-buffering and close behavior
	pWndData->DoubleBuffer = dbflag;
	pWndData->CloseBehavior = closeflag;

	// Set the bitmap info struct to NULL
	pWndData->pbmpInfo = NULL;

	// Set up the defaults for the window
	graphdefaults();

	// MGM: Draw diagonal line since otherwise BitBlt into window doesn't work.
	setcolor(BLACK);
	line(0, 0, width - 1, height - 1);
	// MGM: The black rectangle is because on my laptop, the background
	// is not automatically reset to black when the window opens.
	setfillstyle(SOLID_FILL, BLACK);
	bar(0, 0, width - 1, height - 1);
	// MGM: Reset the color and fillstyle to the default for a new window.
	setcolor(WHITE);
	setfillstyle(SOLID_FILL, WHITE);

	// Everything went well!  Return the window index to the user.
	return index;
}


void closegraph(int wid)
{
	if (wid == CURRENT_WINDOW)
		closegraph(BGI__CurrentWindow);
	else if (wid == ALL_WINDOWS)
	{
		for (int i = 0; i < BGI__WindowCount; i++)
			closegraph(i);
	}
	else if (wid >= 0 && wid <= BGI__WindowCount && BGI__WindowTable[wid] != NULL)
	{
		// DestroyWindow cannot delete a window created by another thread.
		// Thus, use SendMessage to close the requested window.
		// Destroying the window causes cls_OnDestroy to be called,
		// releasing any dynamic memory that's being used by the window.
		// The WindowData structure is released at the end of BGI__ThreadInitWindow,
		// which is reached when the message loop of BGI__ThreadInitWindow get WM_QUIT.
		SendMessage(BGI__WindowTable[wid], WM_DESTROY, 0, 0);

		// Remove the HWND from the BGI__WindowTable vector:
		BGI__WindowTable[wid] = NULL;

		// Reset the global BGI__CurrentWindow if needed:
		if (BGI__CurrentWindow == wid)
			BGI__CurrentWindow = NO_CURRENT_WINDOW;
	}
}


// This fuction detects the graphics driver and returns the highest resolution
// mode possible.  For WinBGI, this is always VGA/VGAHI
//
void detectgraph(int *graphdriver, int *graphmode)
{
	*graphdriver = VGA;
	*graphmode = VGAHI;
}


// This function returns the aspect ratio of the current window.  Unless there
// is something weird going on with Windows resolutions, these quantities
// will be equal and correctly proportioned geometric shapes will result.
//
void getaspectratio(int *xasp, int *yasp)
{
	WindowData *pWndData = BGI__GetWindowDataPtr();
	*xasp = pWndData->x_aspect_ratio;
	*yasp = pWndData->y_aspect_ratio;
}


// This function will return the next character waiting to be read in the
// window's keyboard buffer
//
int getch()
{
	char c;
	WindowData *pWndData = BGI__GetWindowDataPtr();

	// TODO: Start critical section
	// check queue empty
	// end critical section

	if (pWndData->kbd_queue.empty())
		WaitForSingleObject(pWndData->key_waiting, INFINITE);
	else
		// Make sure the event is not signaled.  It will remain signaled until a
		// thread is woken up by it.  This could cause problems if the queue was
		// not empty and we got characters from it, never having to wait.  If the
		// queue then becomes empty, the WaitForSingleObjectX will immediately
		// return since it's still signaled.  If no key was pressed, this would
		// obviously be an error.
		ResetEvent(pWndData->key_waiting);

	// TODO: Start critical section
	// access queue
	// end critical section

	c = pWndData->kbd_queue.front();             // Obtain the next element in the queue
	pWndData->kbd_queue.pop();                   // Remove the element from the queue

	return c;
}


// This function returns the name of the current driver in use.  For WinBGI,
// this is always "EGAVGA"
//
char *getdrivername()
{
	return "EGAVGA";
}


// This function gets the current graphics mode.  For WinBGI, this is always
// VGAHI
//
int getgraphmode()
{
	return VGAHI;
}


// This function returns the maximum mode the current graphics driver can
// display.  For WinBGI, this is always VGAHI.
//
int getmaxmode()
{
	return VGAHI;
}


// This function returns a string describing the current graphics mode.  It has the format
// "width*height MODE_NAME"  For WinBGI, this is the window size followed by VGAHI
//
char *getmodename(int mode_number)
{
	static char mode[20];
	WindowData *pWndData = BGI__GetWindowDataPtr();

	sprintf(mode, "%d*%d VGAHI", pWndData->width, pWndData->height);
	return mode;
}


// This function returns the range of possible graphics modes for the given graphics driver.
// If -1 is given for the driver, the current driver and mode is used.
//
void getmoderange(int graphdriver, int *lomode, int *himode)
{
	WindowData *pWndData = BGI__GetWindowDataPtr();
	int graphmode;

	// Use current driver modes
	if (graphdriver == -1)
		detectgraph(&graphdriver, &graphmode);

	switch (graphdriver)
	{
	case CGA:
		*lomode = CGAC0;
		*himode = CGAHI;
		break;
	case MCGA:
		*lomode = MCGAC0;
		*himode = MCGAHI;
		break;
	case EGA:
		*lomode = EGALO;
		*himode = EGAHI;
		break;
	case EGA64:
		*lomode = EGA64LO;
		*himode = EGA64HI;
		break;
	case EGAMONO:
		*lomode = *himode = EGAMONOHI;
		break;
	case HERCMONO:
		*lomode = *himode = HERCMONOHI;
		break;
	case ATT400:
		*lomode = ATT400C0;
		*himode = ATT400HI;
		break;
	case VGA:
		*lomode = VGALO;
		*himode = VGAHI;
		break;
	case PC3270:
		*lomode = *himode = PC3270HI;
		break;
	case IBM8514:
		*lomode = IBM8514LO;
		*himode = IBM8514HI;
		break;
	default:
		*lomode = *himode = -1;
		pWndData->error_code = grInvalidDriver;
		break;
	}
}


// This function returns an error string corresponding to the given error code.
// This code is returned by graphresult()
//
char *grapherrormsg(int errorcode)
{
	static char *msg[16] = { "No error", "Graphics not installed",
		"Graphics hardware not detected", "Device driver not found",
		"Invalid device driver file", "Insufficient memory to load driver",
		"Out of memory in scan fill", "Out of memory in flood fill",
		"Font file not found", "Not enough meory to load font",
		"Invalid mode for selected driver", "Graphics error",
		"Graphics I/O error", "Invalid font file",
		"Invalid font number", "Invalid device number" };

	if ((errorcode < -16) || (errorcode > 0))
		return NULL;
	else
		return msg[-errorcode];
}


// This function returns the error code from the most recent graphics operation.
// It also resets the error to grOk.
//
int graphresult()
{
	WindowData *pWndData = BGI__GetWindowDataPtr();
	int code;

	code = pWndData->error_code;
	pWndData->error_code = grOk;
	return code;

}


// This function uses the information in graphdriver and graphmode to select
// the appropriate size for the window to be created.
//
void initgraph(int *graphdriver, int *graphmode, char *pathtodriver)
{
	WindowData *pWndData;
	int width = 0, height = 0;
	bool valid = true;

	if (*graphdriver == DETECT)
		detectgraph(graphdriver, graphmode);

	switch (*graphdriver)
	{
	case MCGA:
		switch (*graphmode)
		{
		case MCGAC0:
		case MCGAC1:
		case MCGAC2:
		case MCGAC3:
			width = 320;
			height = 200;
			break;
		case MCGAMED:
			width = 640;
			height = 200;
			break;
		case MCGAHI:
			width = 640;
			height = 480;
			break;
		}
		break;
	case EGA:
		switch (*graphmode)
		{
		case EGALO:
			width = 640;
			height = 200;
			break;
		case EGAHI:
			width = 640;
			height = 350;
			break;
		}
		break;
	case EGA64:
		switch (*graphmode)
		{
		case EGALO:
			width = 640;
			height = 200;
			break;
		case EGAHI:
			width = 640;
			height = 350;
			break;
		}
		break;
	case EGAMONO:
		width = 640;
		height = 350;
		break;
	case HERCMONO:
		width = 720;
		width = 348;
		break;
	case ATT400:
		switch (*graphmode)
		{
		case ATT400C0:
		case ATT400C1:
		case ATT400C2:
		case ATT400C3:
			width = 320;
			height = 200;
			break;
		case ATT400MED:
			width = 640;
			height = 200;
			break;
		case ATT400HI:
			width = 640;
			height = 400;
			break;
		}
		break;
	case VGA:
		switch (*graphmode)
		{
		case VGALO:
			width = 640;
			height = 200;
			break;
		case VGAMED:
			width = 640;
			height = 350;
			break;
		case VGAHI:
			width = 640;
			height = 480;
			break;
		}
		break;
	case PC3270:
		width = 720;
		height = 350;
		break;
	case IBM8514:
		switch (*graphmode)
		{
		case IBM8514LO:
			width = 640;
			height = 480;
			break;
		case IBM8514HI:
			width = 1024;
			height = 768;
			break;
		}
		break;
	default:
		valid = false;
	case CGA:
		switch (*graphmode)
		{
		case CGAC0:
		case CGAC1:
		case CGAC2:
		case CGAC3:
			width = 320;
			height = 200;
			break;
		case CGAHI:
			width = 640;
			height = 200;
			break;
		}
		break;
	}

	// Create the window with with the specified dimensions
	initwindow(width, height);
	if (!valid)
	{
		pWndData = BGI__GetWindowDataPtr();
		pWndData->error_code = grInvalidDriver;

	}
}


// This function does not do any work in WinBGI since the graphics and text
// windows are always both open.
//
void restorecrtmode()
{ }


// This function returns true if there is a character waiting to be read
// from the window's keyboard buffer.
//
int kbhit()
{
	// TODO: start critical section
	// check queue empty
	// end critical section
	WindowData *pWndData = BGI__GetWindowDataPtr();

	return !pWndData->kbd_queue.empty();
}


// This function sets the aspect ratio of the current window.
//
void setaspectratio(int xasp, int yasp)
{
	WindowData *pWndData = BGI__GetWindowDataPtr();

	pWndData->x_aspect_ratio = xasp;
	pWndData->y_aspect_ratio = yasp;
}


void setgraphmode(int mode)
{
	// Reset graphics stuff to default
	graphdefaults();
	// Clear the screen
	cleardevice();
}




/*****************************************************************************
*
*   User-Controlled Window Functions
*
*****************************************************************************/

// This function returns the current window index to the user.  The user can
// use this return value to refer to the window at a later time.
//
int getcurrentwindow()
{
	return BGI__CurrentWindow;
}


// This function sets the current window to the value specified by the user.
// All future drawing activity will be sent to this window.  If the window
// index is invalid, the current window is unchanged
//
void setcurrentwindow(int window)
{
	if ((window < 0) || (window >= BGI__WindowCount) || BGI__WindowTable[window] == NULL)
		return;

	BGI__CurrentWindow = window;
}





/*****************************************************************************
*
*   Double buffering support
*
*****************************************************************************/

// This function returns the current active page for the current window.
//
int getactivepage()
{
	WindowData *pWndData = BGI__GetWindowDataPtr();
	return pWndData->ActivePage;
}


// This function returns the current visual page for the current window.
//
int getvisualpage()
{
	WindowData *pWndData = BGI__GetWindowDataPtr();
	return pWndData->VisualPage;
}


// This function changes the active page of the current window to the page
// specified by page.  If page refers to an invalid number, the current
// active page is unchanged.
//
void setactivepage(int page)
{
	WindowData *pWndData = BGI__GetWindowDataPtr();

	if ((page < 0) || (page > MAX_PAGES))
		return;

	pWndData->ActivePage = page;
}


// This function changes the visual page of the current window to the page
// specified by page.  If page refers to an invalid number, the current
// visual page is unchanged.  The graphics window is then redrawn with the
// new page.
//
void setvisualpage(int page)
{
	WindowData *pWndData = BGI__GetWindowDataPtr();

	if ((page < 0) || (page > MAX_PAGES))
		return;

	pWndData->VisualPage = page;

	// Redraw the entire current window.  No need to erase the background as
	// the new image is simply copied over the old one.
	InvalidateRect(pWndData->hWnd, NULL, FALSE);
}


// This function will swap the buffers if you have created a double-buffered
// window.  That is, by having the dbflag true when initwindow was called.
//
void swapbuffers()
{
	WindowData *pWndData = BGI__GetWindowDataPtr();

	if (pWndData->ActivePage == 0)
	{
		pWndData->VisualPage = 0;
		pWndData->ActivePage = 1;
	}
	else    // Active page is 1
	{
		pWndData->VisualPage = 1;
		pWndData->ActivePage = 0;
	}
	// Redraw the entire current window.  No need to erase the background as
	// the new image is simply copied over the old one.
	InvalidateRect(pWndData->hWnd, NULL, FALSE);
}

// File: WindowThread.cpp
// Written by:
//      Grant Macklem (Grant.Macklem@colorado.edu)
//      Gregory Schmelter (Gregory.Schmelter@colorado.edu)
//      Alan Schmidt (Alan.Schmidt@colorado.edu)
//      Ivan Stashak (Ivan.Stashak@colorado.edu)
// CSCI 4830/7818: API Programming
// University of Colorado at Boulder, Spring 2003
// http://www.cs.colorado.edu/~main/bgi
//

// This structure is how the user interacts with the window.  Upon creation,y
// the user gets the index into this array of the window he created.  We will
// use this array for any function which does not deal with the window
// directly, but relies on the current window.
// MGM: hInstance is no longer a handle to the DLL, but instead it is
// a "self" handle, returned by GetCurrentThread( ).
std::vector<HWND> BGI__WindowTable;
int BGI__WindowCount = 0;                    // Number of windows currently in use
int BGI__CurrentWindow = NO_CURRENT_WINDOW;  // Index to current window
HINSTANCE BGI__hInstance;   // Handle to the instance of the DLL (creating the window class)

#include <iostream>
using namespace std;
// ID numbers for new options that are added to the system menu:
#define BGI_PRINT_SMALL 1
#define BGI_PRINT_MEDIUM 2
#define BGI_PRINT_LARGE 3
#define BGI_SAVE_AS 4
// This is the window creation and message processing function.  Each new
// window is created in its own thread and handles all its own messages.
// It creates the window, sets the current window of the application to this
// window, and signals the main thread that the window has been created.
//
DWORD WINAPI BGI__ThreadInitWindow(LPVOID pThreadData)
{
	HWND hWindow;                       // A handle to the window
	MSG Message;                        // A windows event message
	HDC hDC;                            // The device context of the window
	HBITMAP hBitmap;                    // A compatible bitmap of the DC for the Memory DC
	HMENU hMenu;                        // Handle to the system menu
	int CaptionHeight, xBorder, yBorder;

	WindowData *pWndData = (WindowData*)pThreadData; // Thread creation data

	if (pWndData->title.size())
	{
		CaptionHeight = GetSystemMetrics(SM_CYCAPTION);   // Height of caption
	}
	else
	{
		CaptionHeight = 0;                                 // Height of caption
	}
	xBorder = GetSystemMetrics(SM_CXFIXEDFRAME);      // Width of border
	yBorder = GetSystemMetrics(SM_CYFIXEDFRAME);      // Height of border

	int height = pWndData->height + CaptionHeight + 2 * yBorder; // Calculate total height
	int width = pWndData->width + 2 * xBorder;                   // Calculate total width

	int top = pWndData->inittop;                                 // MGM: Initial top
	int left = pWndData->initleft;                               // MGM: Initial left

	hWindow = CreateWindowEx(0,                    // Extended window styles
		_T("BGILibrary"),   // What kind of window
		pWndData->title.c_str(),  // Title at top of the window
		pWndData->title.size()
		? (WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_DLGFRAME)
		: (WS_POPUP | WS_DLGFRAME),
		left, top, width, height,  // left top width height
		NULL,                 // HANDLE to the parent window
		NULL,                 // HANDLE to the menu
		BGI__hInstance,       // HANDLE to this program
		NULL);               // Address of window-creation data
							 // Check if the window was created
	if (hWindow == 0)
	{
		showerrorbox();
		return 0;
	}

	// Add the print options to the system menu, as shown in Chapter 10 (p 460) of Petzold
	hMenu = GetSystemMenu(hWindow, FALSE);
	AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
	AppendMenu(hMenu, MF_STRING, BGI_SAVE_AS, TEXT("Save image as..."));
	AppendMenu(hMenu, MF_STRING, BGI_PRINT_SMALL, TEXT("Print 2\" wide..."));
	AppendMenu(hMenu, MF_STRING, BGI_PRINT_MEDIUM, TEXT("Print 4.5\" wide..."));
	AppendMenu(hMenu, MF_STRING, BGI_PRINT_LARGE, TEXT("Print 7\" wide..."));
	AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);

	// Store the HANDLE in the structure
	pWndData->hWnd = hWindow;

	// Store the address of the WindowData structure in the window's user data
	// MGM TODO: Change this to SetWindowLongPtr:
	//  SetWindowLongPtr( hWindow, GWL_USERDATA, (LONG_PTR)pWndData );
	SetWindowLong(hWindow, GWL_USERDATA, (LONG)pWndData);

	// Set the default active and visual page.  These must be set here in
	// addition to setting all the defaults in initwindow because the paint
	// method depends on using the correct page.
	pWndData->ActivePage = 0;
	pWndData->VisualPage = 0;

	// Clear the mouse handler array and turn off queuing
	memset(pWndData->mouse_handlers, 0, (WM_MOUSELAST - WM_MOUSEFIRST + 1) * sizeof(Handler));
	memset(pWndData->mouse_queuing, 0, (WM_MOUSELAST - WM_MOUSEFIRST + 1) * sizeof(bool));

	// Create a memory Device Context used for drawing.  The image is copied from here
	// to the screen in the paint method.  The DC and bitmaps are deleted
	// in cls_OnDestroy()
	hDC = GetDC(hWindow);
	pWndData->hDCMutex = CreateMutex(NULL, FALSE, NULL);
	WaitForSingleObject(pWndData->hDCMutex, 5000);
	for (int i = 0; i < MAX_PAGES; i++)
	{
		pWndData->hDC[i] = CreateCompatibleDC(hDC);
		// Create a bitmap for the memory DC.  This is where the drawn image is stored.
		hBitmap = CreateCompatibleBitmap(hDC, pWndData->width, pWndData->height);
		pWndData->hOldBitmap[i] = (HBITMAP)SelectObject(pWndData->hDC[i], hBitmap);
	}
	ReleaseMutex(pWndData->hDCMutex);
	// Release the original DC and set up the mutex for the hDC array
	ReleaseDC(hWindow, hDC);

	// Make the window visible
	ShowWindow(hWindow, SW_SHOWNORMAL);           // Make the window visible
	UpdateWindow(hWindow);                        // Flush output buffer

												  // Tell the user thread that the window was created successfully
	SetEvent(pWndData->WindowCreated);

	// The message loop, which stops when a WM_QUIT message arrives
	while (GetMessage(&Message, NULL, 0, 0))
	{
		TranslateMessage(&Message);
		DispatchMessage(&Message);
	}

	// Free memory used by thread structure
	delete pWndData;
	return (DWORD)Message.wParam;
}


// This function handles the WM_CHAR message.  This message is sent whenever
// the user presses a key in the window (after a WM_KEYDOWN and WM_KEYUP
// message, a WM_CHAR message is added).  It adds the key pressed to the
// keyboard queue for the window.
//
void cls_OnChar(HWND hWnd, TCHAR ch, int repeat)
{
	// This gets the address of the WindowData structure associated with the window
	WindowData *pWndData = BGI__GetWindowDataPtr(hWnd);

	pWndData->kbd_queue.push((TCHAR)ch);// Add the key to the queue
	SetEvent(pWndData->key_waiting);    // Notify the waiting thread, if any
	FORWARD_WM_CHAR(hWnd, ch, repeat, DefWindowProc);
}

static void cls_OnClose(HWND hWnd)
{
	// This gets the address of the WindowData structure associated with the window
	WindowData *pWndData = BGI__GetWindowDataPtr(hWnd);

	exit(0);
}

// This function handles the destroy message.  It will cause the application
// to send WM_QUIT, which will then terminate the message pump thread.
//
static void cls_OnDestroy(HWND hWnd)
{
	// This gets the address of the WindowData structure associated with the window
	WindowData *pWndData = BGI__GetWindowDataPtr(hWnd);

	WaitForSingleObject(pWndData->hDCMutex, 5000);
	for (int i = 0; i < MAX_PAGES; i++)
	{
		// Delete the pen in the DC's
		DeletePen(SelectPen(pWndData->hDC[i], GetStockPen(WHITE_PEN)));

		// Delete the brush in the DC's
		DeleteBrush(SelectBrush(pWndData->hDC[i], GetStockBrush(WHITE_BRUSH)));

		// Here we clean up the memory device contexts used by the program.
		// This selects the original bitmap back into the memory DC.  The SelectObject
		// function returns the current bitmap which we then delete.
		DeleteObject(SelectObject(pWndData->hDC[i], pWndData->hOldBitmap[i]));
		// Finally, we delete the MemoryDC
		DeleteObject(pWndData->hDC[i]);
	}
	ReleaseMutex(pWndData->hDCMutex);
	// Clean up the bitmap memory
	DeleteBitmap(pWndData->hbitmap);

	// Delete the two events created
	CloseHandle(pWndData->key_waiting);
	CloseHandle(pWndData->WindowCreated);

	PostQuitMessage(0);
}


// This function handles the KEYDOWN message and will translate the virtual
// keys to the keys expected by the user
//
static void cls_OnKey(HWND hWnd, UINT vk, BOOL down, int repeat, UINT flags)
{
	// This gets the address of the WindowData structure associated with the window
	// TODO: Set event for each key
	WindowData *pWndData = BGI__GetWindowDataPtr(hWnd);

	switch (vk)
	{
	case VK_CLEAR:
		pWndData->kbd_queue.push((TCHAR)0);
		pWndData->kbd_queue.push((TCHAR)KEY_CENTER);
		break;
	case VK_PRIOR:
		pWndData->kbd_queue.push((TCHAR)0);
		pWndData->kbd_queue.push((TCHAR)KEY_PGUP);
		break;
	case VK_NEXT:
		pWndData->kbd_queue.push((TCHAR)0);
		pWndData->kbd_queue.push((TCHAR)KEY_PGDN);
		break;
	case VK_END:
		pWndData->kbd_queue.push((TCHAR)0);
		pWndData->kbd_queue.push((TCHAR)KEY_END);
		break;
	case VK_HOME:
		pWndData->kbd_queue.push((TCHAR)0);
		pWndData->kbd_queue.push((TCHAR)KEY_HOME);
		break;
	case VK_LEFT:
		pWndData->kbd_queue.push((TCHAR)0);
		pWndData->kbd_queue.push((TCHAR)KEY_LEFT);
		SetEvent(pWndData->key_waiting);
		break;
	case VK_UP:
		pWndData->kbd_queue.push((TCHAR)0);
		pWndData->kbd_queue.push((TCHAR)KEY_UP);
		SetEvent(pWndData->key_waiting);
		break;
	case VK_RIGHT:
		pWndData->kbd_queue.push((TCHAR)0);
		pWndData->kbd_queue.push((TCHAR)KEY_RIGHT);
		SetEvent(pWndData->key_waiting);
		break;
	case VK_DOWN:
		pWndData->kbd_queue.push((TCHAR)0);
		pWndData->kbd_queue.push((TCHAR)KEY_DOWN);
		SetEvent(pWndData->key_waiting);
		break;
	case VK_INSERT:
		pWndData->kbd_queue.push((TCHAR)0);
		pWndData->kbd_queue.push((TCHAR)KEY_INSERT);
		break;
	case VK_DELETE:
		pWndData->kbd_queue.push((TCHAR)0);
		pWndData->kbd_queue.push((TCHAR)KEY_DELETE);
		break;
	case VK_F1:
		pWndData->kbd_queue.push((TCHAR)0);
		pWndData->kbd_queue.push((TCHAR)KEY_F1);
		break;
	case VK_F2:
		pWndData->kbd_queue.push((TCHAR)0);
		pWndData->kbd_queue.push((TCHAR)KEY_F2);
		break;
	case VK_F3:
		pWndData->kbd_queue.push((TCHAR)0);
		pWndData->kbd_queue.push((TCHAR)KEY_F3);
		break;
	case VK_F4:
		pWndData->kbd_queue.push((TCHAR)0);
		pWndData->kbd_queue.push((TCHAR)KEY_F4);
		break;
	case VK_F5:
		pWndData->kbd_queue.push((TCHAR)0);
		pWndData->kbd_queue.push((TCHAR)KEY_F5);
		break;
	case VK_F6:
		pWndData->kbd_queue.push((TCHAR)0);
		pWndData->kbd_queue.push((TCHAR)KEY_F6);
		break;
	case VK_F7:
		pWndData->kbd_queue.push((TCHAR)0);
		pWndData->kbd_queue.push((TCHAR)KEY_F7);
		break;
	case VK_F8:
		pWndData->kbd_queue.push((TCHAR)0);
		pWndData->kbd_queue.push((TCHAR)KEY_F8);
		break;
	case VK_F9:
		pWndData->kbd_queue.push((TCHAR)0);
		pWndData->kbd_queue.push((TCHAR)KEY_F9);
		break;
	}

	FORWARD_WM_KEYDOWN(hWnd, vk, repeat, flags, DefWindowProc);
}

#include <iostream>
static void cls_OnPaint(HWND hWnd)
{
	PAINTSTRUCT ps;             // BeginPaint puts info about the paint request here
	HDC hSrcDC;                 // The device context to copy from
	WindowData *pWndData = BGI__GetWindowDataPtr(hWnd);
	int width, height;          // Area that needs to be redrawn
	POINT srcCorner;            // Logical coordinates of the source image upper left point
	BOOL success;               // Is the BitBlt successful?
//	int i;                      // Count for how many bitblts have been tried.

	WaitForSingleObject(pWndData->hDCMutex, INFINITE);
	BeginPaint(hWnd, &ps);
	hSrcDC = pWndData->hDC[pWndData->VisualPage];   // The source (memory) DC

													// Get the dimensions of the area that needs to be redrawn.
	width = ps.rcPaint.right - ps.rcPaint.left;
	height = ps.rcPaint.bottom - ps.rcPaint.top;


	// The region that needs to be updated is specified in device units (pixels) for the actual DC.
	// However, if a viewport is specified, the source image is referenced in logical
	// units.  Perform the conversion.
	// MGM TODO: When is the DPtoLP call needed?
	srcCorner.x = ps.rcPaint.left;
	srcCorner.y = ps.rcPaint.top;
	DPtoLP(hSrcDC, &srcCorner, 1);

	// MGM: Screen BitBlts are not always successful, although I don't know why.
	success = BitBlt(ps.hdc, ps.rcPaint.left, ps.rcPaint.top, width, height,
		hSrcDC, srcCorner.x, srcCorner.y, SRCCOPY);
	EndPaint(hWnd, &ps);  // Validates the rectangle
	ReleaseMutex(pWndData->hDCMutex);

	if (!success)
	{   // I would like to invalidate the rectangle again
		// since BitBlt wasn't successful, but the recursion seems
		// to hang some machines.
		// delay(100);
		// InvalidateRect( hWnd, &(ps.rcPaint), FALSE );
		// std::cout << "Failure in cls_OnPaint" << std:: endl;
	}
}

// The message-handler function for the window
//
LRESULT CALLBACK WndProc
(HWND hWnd, UINT uiMessage, WPARAM wParam, LPARAM lParam)
{
	const std::queue<POINTS> EMPTY;
	POINTS where;

	WindowData *pWndData = BGI__GetWindowDataPtr(hWnd);
	int type;           // Type of mouse message
	Handler handler;    // Registered mouse handler
	UINT uHitTest;

	// If this is a mouse message, set our internal state
	if (pWndData && (uiMessage >= WM_MOUSEFIRST) && (uiMessage <= WM_MOUSELAST))
	{
		type = uiMessage - WM_MOUSEFIRST;
		if (!(pWndData->mouse_queuing[type]) && pWndData->clicks[type].size())
		{
			pWndData->clicks[type] = EMPTY;
		}
		pWndData->clicks[type].push(where = MAKEPOINTS(lParam));  // Set the current position for the event type
		pWndData->mouse = where; // Set the current mouse position

								 // If the user has registered a mouse handler, call it now
		handler = pWndData->mouse_handlers[type];
		if (handler != NULL)
			handler(where.x, where.y);
	}

	switch (uiMessage)
	{
		HANDLE_MSG(hWnd, WM_CHAR, cls_OnChar);
		HANDLE_MSG(hWnd, WM_DESTROY, cls_OnDestroy);
		HANDLE_MSG(hWnd, WM_KEYDOWN, cls_OnKey);
		HANDLE_MSG(hWnd, WM_PAINT, cls_OnPaint);
	case WM_LBUTTONDBLCLK:
		return TRUE;
	case WM_NCHITTEST:
		uHitTest = DefWindowProc(hWnd, WM_NCHITTEST, wParam, lParam);
		if (uHitTest != HTCLIENT && pWndData && pWndData->title.size() == 0)
			return HTCAPTION;
		else
			return uHitTest;
	case WM_CLOSE:
		if (pWndData->CloseBehavior)
		{
			HANDLE_WM_CLOSE(hWnd, wParam, lParam, cls_OnClose);
		}
		return TRUE;
	case WM_SYSCOMMAND:
		switch (LOWORD(wParam))
		{
		case BGI_SAVE_AS: writeimagefile(NULL, 0, 0, INT_MAX, INT_MAX, false, hWnd); return 0;
		case BGI_PRINT_SMALL: printimage(NULL, 2.0, 0.75, 0.75, 0, 0, INT_MAX, INT_MAX, false, hWnd); return 0;
		case BGI_PRINT_MEDIUM: printimage(NULL, 4.5, 0.75, 0.75, 0, 0, INT_MAX, INT_MAX, false, hWnd); return 0;
		case BGI_PRINT_LARGE: printimage(NULL, 7.0, 0.75, 0.75, 0, 0, INT_MAX, INT_MAX, false, hWnd); return 0;
		}
		break;
	}
	return DefWindowProc(hWnd, uiMessage, wParam, lParam);
}

//*******************************************************************
//
//  file.c
//
//  Source file for Device-Independent Bitmap (DIB) API.  Provides
//  the following functions:
//
//  SaveDIB()           - Saves the specified dib in a file
//  LoadDIB()           - Loads a DIB from a file
//  DestroyDIB()        - Deletes DIB when finished using it
//
// Written by Microsoft Product Support Services, Developer Support.
// Copyright (C) 1991-1996 Microsoft Corporation. All rights reserved.
//*******************************************************************

#ifndef STRICT
#define     STRICT      // enable strict type checking
#endif

// Dib Header Marker - used in writing DIBs to files

#define DIB_HEADER_MARKER   ((WORD) ('M' << 8) | 'B')


/*********************************************************************
*
* Local Function Prototypes
*
*********************************************************************/


HANDLE ReadDIBFile(HANDLE);
BOOL SaveDIBFile(void);
BOOL WriteDIB(LPSTR, HANDLE);

/*************************************************************************
*
* LoadDIB()
*
* Loads the specified DIB from a file, allocates memory for it,
* and reads the disk file into the memory.
*
*
* Parameters:
*
* LPSTR lpFileName - specifies the file to load a DIB from
*
* Returns: A handle to a DIB, or NULL if unsuccessful.
*
* NOTE: The DIB API were not written to handle OS/2 DIBs; This
* function will reject any file that is not a Windows DIB.
*
*************************************************************************/

HDIB LoadDIB(const char* lpFileName)
{
	HDIB        hDIB;
	HANDLE      hFile;

	// Set the cursor to a hourglass, in case the loading operation
	// takes more than a sec, the user will know what's going on.

	SetCursor(LoadCursor(NULL, IDC_WAIT));

	if ((hFile = CreateFile(lpFileName, GENERIC_READ, FILE_SHARE_READ, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
		NULL)) != INVALID_HANDLE_VALUE)
	{
		hDIB = ReadDIBFile(hFile);
		CloseHandle(hFile);
		SetCursor(LoadCursor(NULL, IDC_ARROW));
		return hDIB;
	}
	else
	{
		showerrorbox("File not found");
		SetCursor(LoadCursor(NULL, IDC_ARROW));
		return NULL;
	}
}


/*************************************************************************
*
* SaveDIB()
*
* Saves the specified DIB into the specified file name on disk.  No
* error checking is done, so if the file already exists, it will be
* written over.
*
* Parameters:
*
* HDIB hDib - Handle to the dib to save
*
* LPSTR lpFileName - pointer to full pathname to save DIB under
*
* Return value: 0 if successful, or one of:
*        ERR_INVALIDHANDLE
*        ERR_OPEN
*        ERR_LOCK
*
*************************************************************************/

WORD SaveDIB(HDIB hDib, const char* lpFileName)
{
	BITMAPFILEHEADER    bmfHdr;     // Header for Bitmap file
	LPBITMAPINFOHEADER  lpBI;       // Pointer to DIB info structure
	HANDLE              fh;         // file handle for opened file
	DWORD               dwDIBSize;
	DWORD               dwWritten;

	if (!hDib)
		return ERR_INVALIDHANDLE;

	fh = CreateFile(lpFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);

	if (fh == INVALID_HANDLE_VALUE)
		return ERR_OPEN;

	// Get a pointer to the DIB memory, the first of which contains
	// a BITMAPINFO structure

	lpBI = (LPBITMAPINFOHEADER)GlobalLock(hDib);
	if (!lpBI)
	{
		CloseHandle(fh);
		return ERR_LOCK;
	}

	// Check to see if we're dealing with an OS/2 DIB.  If so, don't
	// save it because our functions aren't written to deal with these
	// DIBs.

	if (lpBI->biSize != sizeof(BITMAPINFOHEADER))
	{
		GlobalUnlock(hDib);
		CloseHandle(fh);
		return ERR_NOT_DIB;
	}

	// Fill in the fields of the file header

	// Fill in file type (first 2 bytes must be "BM" for a bitmap)

	bmfHdr.bfType = DIB_HEADER_MARKER;  // "BM"

										// Calculating the size of the DIB is a bit tricky (if we want to
										// do it right).  The easiest way to do this is to call GlobalSize()
										// on our global handle, but since the size of our global memory may have
										// been padded a few bytes, we may end up writing out a few too
										// many bytes to the file (which may cause problems with some apps,
										// like HC 3.0).
										//
										// So, instead let's calculate the size manually.
										//
										// To do this, find size of header plus size of color table.  Since the
										// first DWORD in both BITMAPINFOHEADER and BITMAPCOREHEADER conains
										// the size of the structure, let's use this.

										// Partial Calculation

	dwDIBSize = *(LPDWORD)lpBI + PaletteSize((LPSTR)lpBI);

	// Now calculate the size of the image

	// It's an RLE bitmap, we can't calculate size, so trust the biSizeImage
	// field

	if ((lpBI->biCompression == BI_RLE8) || (lpBI->biCompression == BI_RLE4))
		dwDIBSize += lpBI->biSizeImage;
	else
	{
		DWORD dwBmBitsSize;  // Size of Bitmap Bits only

							 // It's not RLE, so size is Width (DWORD aligned) * Height

		dwBmBitsSize = WIDTHBYTES((lpBI->biWidth)*((DWORD)lpBI->biBitCount)) *
			lpBI->biHeight;

		dwDIBSize += dwBmBitsSize;

		// Now, since we have calculated the correct size, why don't we
		// fill in the biSizeImage field (this will fix any .BMP files which 
		// have this field incorrect).

		lpBI->biSizeImage = dwBmBitsSize;
	}


	// Calculate the file size by adding the DIB size to sizeof(BITMAPFILEHEADER)

	bmfHdr.bfSize = dwDIBSize + sizeof(BITMAPFILEHEADER);
	bmfHdr.bfReserved1 = 0;
	bmfHdr.bfReserved2 = 0;

	// Now, calculate the offset the actual bitmap bits will be in
	// the file -- It's the Bitmap file header plus the DIB header,
	// plus the size of the color table.

	bmfHdr.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + lpBI->biSize +
		PaletteSize((LPSTR)lpBI);

	// Write the file header

	WriteFile(fh, (LPSTR)&bmfHdr, sizeof(BITMAPFILEHEADER), &dwWritten, NULL);

	// Write the DIB header and the bits -- use local version of
	// MyWrite, so we can write more than 32767 bytes of data

	WriteFile(fh, (LPSTR)lpBI, dwDIBSize, &dwWritten, NULL);

	GlobalUnlock(hDib);
	CloseHandle(fh);

	if (dwWritten == 0)
		return ERR_OPEN; // oops, something happened in the write
	else
		return 0; // Success code
}


/*************************************************************************
*
* DestroyDIB ()
*
* Purpose:  Frees memory associated with a DIB
*
* Returns:  Nothing
*
*************************************************************************/

WORD DestroyDIB(HDIB hDib)
{
	GlobalFree(hDib);
	return 0;
}


//************************************************************************
//
// Auxiliary Functions which the above procedures use
//
//************************************************************************


/*************************************************************************
*
* Function:  ReadDIBFile (int)
*
*  Purpose:  Reads in the specified DIB file into a global chunk of
*            memory.
*
*  Returns:  A handle to a dib (hDIB) if successful.
*            NULL if an error occurs.
*
* Comments:  BITMAPFILEHEADER is stripped off of the DIB.  Everything
*            from the end of the BITMAPFILEHEADER structure on is
*            returned in the global memory handle.
*
*
* NOTE: The DIB API were not written to handle OS/2 DIBs, so this
* function will reject any file that is not a Windows DIB.
*
*************************************************************************/

HANDLE ReadDIBFile(HANDLE hFile)
{
	BITMAPFILEHEADER    bmfHeader;
	DWORD               dwBitsSize;
	UINT                nNumColors;   // Number of colors in table
	HANDLE              hDIB;
	HANDLE              hDIBtmp;      // Used for GlobalRealloc() //MPB
	LPBITMAPINFOHEADER  lpbi;
	DWORD               offBits;
	DWORD               dwRead;

	// get length of DIB in bytes for use when reading

	dwBitsSize = GetFileSize(hFile, NULL);

	// Allocate memory for header & color table. We'll enlarge this
	// memory as needed.

	hDIB = GlobalAlloc(GMEM_MOVEABLE, (DWORD)(sizeof(BITMAPINFOHEADER) +
		256 * sizeof(RGBQUAD)));

	if (!hDIB)
		return NULL;

	lpbi = (LPBITMAPINFOHEADER)GlobalLock(hDIB);

	if (!lpbi)
	{
		GlobalFree(hDIB);
		return NULL;
	}

	// read the BITMAPFILEHEADER from our file

	if (!ReadFile(hFile, (LPSTR)&bmfHeader, sizeof(BITMAPFILEHEADER),
		&dwRead, NULL))
		goto ErrExit;

	if (sizeof(BITMAPFILEHEADER) != dwRead)
		goto ErrExit;

	if (bmfHeader.bfType != 0x4d42)  // 'BM'
		goto ErrExit;

	// read the BITMAPINFOHEADER

	if (!ReadFile(hFile, (LPSTR)lpbi, sizeof(BITMAPINFOHEADER), &dwRead,
		NULL))
		goto ErrExit;

	if (sizeof(BITMAPINFOHEADER) != dwRead)
		goto ErrExit;

	// Check to see that it's a Windows DIB -- an OS/2 DIB would cause
	// strange problems with the rest of the DIB API since the fields
	// in the header are different and the color table entries are
	// smaller.
	//
	// If it's not a Windows DIB (e.g. if biSize is wrong), return NULL.

	if (lpbi->biSize == sizeof(BITMAPCOREHEADER))
		goto ErrExit;

	// Now determine the size of the color table and read it.  Since the
	// bitmap bits are offset in the file by bfOffBits, we need to do some
	// special processing here to make sure the bits directly follow
	// the color table (because that's the format we are susposed to pass
	// back)

	if (!(nNumColors = (UINT)lpbi->biClrUsed))
	{
		// no color table for 24-bit, default size otherwise

		if (lpbi->biBitCount != 24)
			nNumColors = 1 << lpbi->biBitCount; // standard size table
	}

	// fill in some default values if they are zero

	if (lpbi->biClrUsed == 0)
		lpbi->biClrUsed = nNumColors;

	if (lpbi->biSizeImage == 0)
	{
		lpbi->biSizeImage = ((((lpbi->biWidth * (DWORD)lpbi->biBitCount) +
			31) & ~31) >> 3) * lpbi->biHeight;
	}

	// get a proper-sized buffer for header, color table and bits

	GlobalUnlock(hDIB);
	hDIBtmp = GlobalReAlloc(hDIB, lpbi->biSize + nNumColors *
		sizeof(RGBQUAD) + lpbi->biSizeImage, 0);

	if (!hDIBtmp) // can't resize buffer for loading
		goto ErrExitNoUnlock; //MPB
	else
		hDIB = hDIBtmp;

	lpbi = (LPBITMAPINFOHEADER)GlobalLock(hDIB);

	// read the color table

	ReadFile(hFile, (LPSTR)(lpbi)+lpbi->biSize,
		nNumColors * sizeof(RGBQUAD), &dwRead, NULL);

	// offset to the bits from start of DIB header

	offBits = lpbi->biSize + nNumColors * sizeof(RGBQUAD);

	// If the bfOffBits field is non-zero, then the bits might *not* be
	// directly following the color table in the file.  Use the value in
	// bfOffBits to seek the bits.

	if (bmfHeader.bfOffBits != 0L)
		SetFilePointer(hFile, bmfHeader.bfOffBits, NULL, FILE_BEGIN);

	if (ReadFile(hFile, (LPSTR)lpbi + offBits, lpbi->biSizeImage, &dwRead,
		NULL))
		goto OKExit;


ErrExit:
	GlobalUnlock(hDIB);

ErrExitNoUnlock:
	GlobalFree(hDIB);
	return NULL;

OKExit:
	GlobalUnlock(hDIB);
	return hDIB;
}

//**********************************************************************
//
//  dibutil.c
//
//  Source file for Device-Independent Bitmap (DIB) API.  Provides
//  the following functions:
//
//  CreateDIB()         - Creates new DIB
//  FindDIBBits()       - Sets pointer to the DIB bits
//  DIBWidth()          - Gets the width of the DIB
//  DIBHeight()         - Gets the height of the DIB
//  PaletteSize()       - Calculates the buffer size required by a palette
//  DIBNumColors()      - Calculates number of colors in the DIB's color table
//  CreateDIBPalette()  - Creates a palette from a DIB
//  DIBToBitmap()       - Creates a bitmap from a DIB
//  BitmapToDIB()       - Creates a DIB from a bitmap
//  PalEntriesOnDevice()- Gets the number of palette entries of a device
//  GetSystemPalette()  - Returns a handle to the current system palette
//  AllocRoomForDIB()   - Allocates memory for a DIB
//  ChangeDIBFormat()   - Changes a DIB's BPP and/or compression format
//  ChangeBitmapFormat()- Changes a bitmap to a DIB with specified BPP and
//                        compression format
//
// Written by Microsoft Product Support Services, Developer Support.
// Copyright (C) 1991-1996 Microsoft Corporation. All rights reserved.
//**********************************************************************

#ifndef STRICT
#define     STRICT      // enable strict type checking
#endif

/*************************************************************************
*
* CreateDIB()
*
* Parameters:
*
* DWORD dwWidth    - Width for new bitmap, in pixels
* DWORD dwHeight   - Height for new bitmap
* WORD  wBitCount  - Bit Count for new DIB (1, 4, 8, or 24)
*
* Return Value:
*
* HDIB             - Handle to new DIB
*
* Description:
*
* This function allocates memory for and initializes a new DIB by
* filling in the BITMAPINFOHEADER, allocating memory for the color
* table, and allocating memory for the bitmap bits.  As with all
* HDIBs, the header, colortable and bits are all in one contiguous
* memory block.  This function is similar to the CreateBitmap()
* Windows API.
*
* The colortable and bitmap bits are left uninitialized (zeroed) in the
* returned HDIB.
*
*
************************************************************************/

HDIB CreateDIB(DWORD dwWidth, DWORD dwHeight, WORD wBitCount)
{
	BITMAPINFOHEADER    bi;             // bitmap header
	LPBITMAPINFOHEADER  lpbi;           // pointer to BITMAPINFOHEADER
	DWORD               dwLen;          // size of memory block
	HDIB                hDIB;
	DWORD               dwBytesPerLine; // Number of bytes per scanline


										// Make sure bits per pixel is valid

	if (wBitCount <= 1)
		wBitCount = 1;
	else if (wBitCount <= 4)
		wBitCount = 4;
	else if (wBitCount <= 8)
		wBitCount = 8;
	else if (wBitCount <= 24)
		wBitCount = 24;
	else
		wBitCount = 4;  // set default value to 4 if parameter is bogus

						// initialize BITMAPINFOHEADER

	bi.biSize = sizeof(BITMAPINFOHEADER);
	bi.biWidth = dwWidth;         // fill in width from parameter
	bi.biHeight = dwHeight;       // fill in height from parameter
	bi.biPlanes = 1;              // must be 1
	bi.biBitCount = wBitCount;    // from parameter
	bi.biCompression = BI_RGB;
	bi.biSizeImage = 0;           // 0's here mean "default"
	bi.biXPelsPerMeter = 0;
	bi.biYPelsPerMeter = 0;
	bi.biClrUsed = 0;
	bi.biClrImportant = 0;

	// calculate size of memory block required to store the DIB.  This
	// block should be big enough to hold the BITMAPINFOHEADER, the color
	// table, and the bits

	dwBytesPerLine = WIDTHBYTES(wBitCount * dwWidth);
	dwLen = bi.biSize + PaletteSize((LPSTR)&bi) + (dwBytesPerLine * dwHeight);

	// alloc memory block to store our bitmap

	hDIB = GlobalAlloc(GHND, dwLen);

	// major bummer if we couldn't get memory block

	if (!hDIB)
		return NULL;

	// lock memory and get pointer to it

	lpbi = (LPBITMAPINFOHEADER)GlobalLock(hDIB);

	// use our bitmap info structure to fill in first part of
	// our DIB with the BITMAPINFOHEADER

	*lpbi = bi;

	// Since we don't know what the colortable and bits should contain,
	// just leave these blank.  Unlock the DIB and return the HDIB.

	GlobalUnlock(hDIB);

	//return handle to the DIB

	return hDIB;
}


/*************************************************************************
*
* FindDIBBits()
*
* Parameter:
*
* LPSTR lpDIB      - pointer to packed-DIB memory block
*
* Return Value:
*
* LPSTR            - pointer to the DIB bits
*
* Description:
*
* This function calculates the address of the DIB's bits and returns a
* pointer to the DIB bits.
*
************************************************************************/

LPSTR FindDIBBits(LPSTR lpDIB)
{
	return (lpDIB + *(LPDWORD)lpDIB + PaletteSize(lpDIB));
}


/*************************************************************************
*
* DIBWidth()
*
* Parameter:
*
* LPSTR lpDIB      - pointer to packed-DIB memory block
*
* Return Value:
*
* DWORD            - width of the DIB
*
* Description:
*
* This function gets the width of the DIB from the BITMAPINFOHEADER
* width field if it is a Windows 3.0-style DIB or from the BITMAPCOREHEADER
* width field if it is an OS/2-style DIB.
*
************************************************************************/


DWORD DIBWidth(LPSTR lpDIB)
{
	LPBITMAPINFOHEADER   lpbmi;  // pointer to a Win 3.0-style DIB
	LPBITMAPCOREHEADER   lpbmc;  // pointer to an OS/2-style DIB

								 // point to the header (whether Win 3.0 and OS/2)

	lpbmi = (LPBITMAPINFOHEADER)lpDIB;
	lpbmc = (LPBITMAPCOREHEADER)lpDIB;

	// return the DIB width if it is a Win 3.0 DIB

	if (lpbmi->biSize == sizeof(BITMAPINFOHEADER))
		return lpbmi->biWidth;
	else  // it is an OS/2 DIB, so return its width
		return (DWORD)lpbmc->bcWidth;
}


/*************************************************************************
*
* DIBHeight()
*
* Parameter:
*
* LPSTR lpDIB      - pointer to packed-DIB memory block
*
* Return Value:
*
* DWORD            - height of the DIB
*
* Description:
*
* This function gets the height of the DIB from the BITMAPINFOHEADER
* height field if it is a Windows 3.0-style DIB or from the BITMAPCOREHEADER
* height field if it is an OS/2-style DIB.
*
************************************************************************/

DWORD DIBHeight(LPSTR lpDIB)
{
	LPBITMAPINFOHEADER   lpbmi;  // pointer to a Win 3.0-style DIB
	LPBITMAPCOREHEADER   lpbmc;  // pointer to an OS/2-style DIB

								 // point to the header (whether OS/2 or Win 3.0

	lpbmi = (LPBITMAPINFOHEADER)lpDIB;
	lpbmc = (LPBITMAPCOREHEADER)lpDIB;

	// return the DIB height if it is a Win 3.0 DIB
	if (lpbmi->biSize == sizeof(BITMAPINFOHEADER))
		return lpbmi->biHeight;
	else  // it is an OS/2 DIB, so return its height
		return (DWORD)lpbmc->bcHeight;
}


/*************************************************************************
*
* PaletteSize()
*
* Parameter:
*
* LPSTR lpDIB      - pointer to packed-DIB memory block
*
* Return Value:
*
* WORD             - size of the color palette of the DIB
*
* Description:
*
* This function gets the size required to store the DIB's palette by
* multiplying the number of colors by the size of an RGBQUAD (for a
* Windows 3.0-style DIB) or by the size of an RGBTRIPLE (for an OS/2-
* style DIB).
*
************************************************************************/

WORD PaletteSize(LPSTR lpDIB)
{
	// calculate the size required by the palette
	if (IS_WIN30_DIB(lpDIB))
		return (DIBNumColors(lpDIB) * sizeof(RGBQUAD));
	else
		return (DIBNumColors(lpDIB) * sizeof(RGBTRIPLE));
}


/*************************************************************************
*
* DIBNumColors()
*
* Parameter:
*
* LPSTR lpDIB      - pointer to packed-DIB memory block
*
* Return Value:
*
* WORD             - number of colors in the color table
*
* Description:
*
* This function calculates the number of colors in the DIB's color table
* by finding the bits per pixel for the DIB (whether Win3.0 or OS/2-style
* DIB). If bits per pixel is 1: colors=2, if 4: colors=16, if 8: colors=256,
* if 24, no colors in color table.
*
************************************************************************/

WORD DIBNumColors(LPSTR lpDIB)
{
	WORD wBitCount;  // DIB bit count

					 // If this is a Windows-style DIB, the number of colors in the
					 // color table can be less than the number of bits per pixel
					 // allows for (i.e. lpbi->biClrUsed can be set to some value).
					 // If this is the case, return the appropriate value.


	if (IS_WIN30_DIB(lpDIB))
	{
		DWORD dwClrUsed;

		dwClrUsed = ((LPBITMAPINFOHEADER)lpDIB)->biClrUsed;
		if (dwClrUsed)

			return (WORD)dwClrUsed;
	}

	// Calculate the number of colors in the color table based on
	// the number of bits per pixel for the DIB.

	if (IS_WIN30_DIB(lpDIB))
		wBitCount = ((LPBITMAPINFOHEADER)lpDIB)->biBitCount;
	else
		wBitCount = ((LPBITMAPCOREHEADER)lpDIB)->bcBitCount;

	// return number of colors based on bits per pixel

	switch (wBitCount)
	{
	case 1:
		return 2;

	case 4:
		return 16;

	case 8:
		return 256;

	default:
		return 0;
	}
}


/*************************************************************************
*
* CreateDIBPalette()
*
* Parameter:
*
* HDIB hDIB        - specifies the DIB
*
* Return Value:
*
* HPALETTE         - specifies the palette
*
* Description:
*
* This function creates a palette from a DIB by allocating memory for the
* logical palette, reading and storing the colors from the DIB's color table
* into the logical palette, creating a palette from this logical palette,
* and then returning the palette's handle. This allows the DIB to be
* displayed using the best possible colors (important for DIBs with 256 or
* more colors).
*
************************************************************************/

HPALETTE CreateDIBPalette(HDIB hDIB)
{
	LPLOGPALETTE        lpPal;          // pointer to a logical palette
	HANDLE              hLogPal;        // handle to a logical palette
	HPALETTE            hPal = NULL;    // handle to a palette
	int                 i, wNumColors;  // loop index, number of colors in color table
	LPSTR               lpbi;           // pointer to packed-DIB
	LPBITMAPINFO        lpbmi;          // pointer to BITMAPINFO structure (Win3.0)
	LPBITMAPCOREINFO    lpbmc;          // pointer to BITMAPCOREINFO structure (OS/2)
	BOOL                bWinStyleDIB;   // Win3.0 DIB?

										// if handle to DIB is invalid, return NULL

	if (!hDIB)
		return NULL;

	// lock DIB memory block and get a pointer to it

	lpbi = (LPSTR)GlobalLock(hDIB);

	// get pointer to BITMAPINFO (Win 3.0)

	lpbmi = (LPBITMAPINFO)lpbi;

	// get pointer to BITMAPCOREINFO (OS/2 1.x)

	lpbmc = (LPBITMAPCOREINFO)lpbi;

	// get the number of colors in the DIB 

	wNumColors = DIBNumColors(lpbi);

	// is this a Win 3.0 DIB?

	bWinStyleDIB = IS_WIN30_DIB(lpbi);
	if (wNumColors)
	{
		// allocate memory block for logical palette

		hLogPal = GlobalAlloc(GHND, sizeof(LOGPALETTE) +
			sizeof(PALETTEENTRY) * wNumColors);

		// if not enough memory, clean up and return NULL

		if (!hLogPal)
		{
			GlobalUnlock(hDIB);
			return NULL;
		}

		// lock memory block and get pointer to it

		lpPal = (LPLOGPALETTE)GlobalLock(hLogPal);

		// set version and number of palette entries

		lpPal->palVersion = PALVERSION;
		lpPal->palNumEntries = wNumColors;

		// store RGB triples (if Win 3.0 DIB) or RGB quads (if OS/2 DIB)
		// into palette

		for (i = 0; i < wNumColors; i++)
		{
			if (bWinStyleDIB)
			{
				lpPal->palPalEntry[i].peRed = lpbmi->bmiColors[i].rgbRed;
				lpPal->palPalEntry[i].peGreen = lpbmi->bmiColors[i].rgbGreen;
				lpPal->palPalEntry[i].peBlue = lpbmi->bmiColors[i].rgbBlue;
				lpPal->palPalEntry[i].peFlags = 0;
			}
			else
			{
				lpPal->palPalEntry[i].peRed = lpbmc->bmciColors[i].rgbtRed;
				lpPal->palPalEntry[i].peGreen = lpbmc->bmciColors[i].rgbtGreen;
				lpPal->palPalEntry[i].peBlue = lpbmc->bmciColors[i].rgbtBlue;
				lpPal->palPalEntry[i].peFlags = 0;
			}
		}

		// create the palette and get handle to it

		hPal = CreatePalette(lpPal);

		// if error getting handle to palette, clean up and return NULL

		if (!hPal)
		{
			GlobalUnlock(hLogPal);
			GlobalFree(hLogPal);
			return NULL;
		}
	}

	// clean up 

	GlobalUnlock(hLogPal);
	GlobalFree(hLogPal);
	GlobalUnlock(hDIB);

	// return handle to DIB's palette
	return hPal;
}


/*************************************************************************
*
* DIBToBitmap()
*
* Parameters:
*
* HDIB hDIB        - specifies the DIB to convert
*
* HPALETTE hPal    - specifies the palette to use with the bitmap
*
* Return Value:
*
* HBITMAP          - identifies the device-dependent bitmap
*
* Description:
*
* This function creates a bitmap from a DIB using the specified palette.
* If no palette is specified, default is used.
*
* NOTE:
*
* The bitmap returned from this funciton is always a bitmap compatible
* with the screen (e.g. same bits/pixel and color planes) rather than
* a bitmap with the same attributes as the DIB.  This behavior is by
* design, and occurs because this function calls CreateDIBitmap to
* do its work, and CreateDIBitmap always creates a bitmap compatible
* with the hDC parameter passed in (because it in turn calls
* CreateCompatibleBitmap).
*
* So for instance, if your DIB is a monochrome DIB and you call this
* function, you will not get back a monochrome HBITMAP -- you will
* get an HBITMAP compatible with the screen DC, but with only 2
* colors used in the bitmap.
*
* If your application requires a monochrome HBITMAP returned for a
* monochrome DIB, use the function SetDIBits().
*
* Also, the DIBpassed in to the function is not destroyed on exit. This
* must be done later, once it is no longer needed.
*
************************************************************************/

HBITMAP DIBToBitmap(HDIB hDIB, HPALETTE hPal)
{
	LPSTR       lpDIBHdr, lpDIBBits;  // pointer to DIB header, pointer to DIB bits
	HBITMAP     hBitmap;            // handle to device-dependent bitmap
	HDC         hDC;                    // handle to DC
	HPALETTE    hOldPal = NULL;    // handle to a palette

								   // if invalid handle, return NULL 

	if (!hDIB)
		return NULL;

	// lock memory block and get a pointer to it

	lpDIBHdr = (LPSTR)GlobalLock(hDIB);

	// get a pointer to the DIB bits

	lpDIBBits = FindDIBBits(lpDIBHdr);

	// get a DC 

	hDC = GetDC(NULL);
	if (!hDC)
	{
		// clean up and return NULL

		GlobalUnlock(hDIB);
		return NULL;
	}

	// select and realize palette

	if (hPal)
		hOldPal = SelectPalette(hDC, hPal, FALSE);

	RealizePalette(hDC);

	// create bitmap from DIB info. and bits
	hBitmap = CreateDIBitmap(hDC, (LPBITMAPINFOHEADER)lpDIBHdr, CBM_INIT,
		lpDIBBits, (LPBITMAPINFO)lpDIBHdr, DIB_RGB_COLORS);

	// restore previous palette
	if (hOldPal)
		SelectPalette(hDC, hOldPal, FALSE);

	// clean up
	ReleaseDC(NULL, hDC);
	GlobalUnlock(hDIB);

	// return handle to the bitmap
	return hBitmap;
}


/*************************************************************************
*
* BitmapToDIB()
*
* Parameters:
*
* HBITMAP hBitmap  - specifies the bitmap to convert
*
* HPALETTE hPal    - specifies the palette to use with the bitmap
*
* Return Value:
*
* HDIB             - identifies the device-dependent bitmap
*
* Description:
*
* This function creates a DIB from a bitmap using the specified palette.
*
************************************************************************/

HDIB BitmapToDIB(HBITMAP hBitmap, HPALETTE hPal)
{
	BITMAP              bm;         // bitmap structure
	BITMAPINFOHEADER    bi;         // bitmap header
	LPBITMAPINFOHEADER  lpbi;       // pointer to BITMAPINFOHEADER
	DWORD               dwLen;      // size of memory block
	HANDLE              hDIB, h;    // handle to DIB, temp handle
	HDC                 hDC;        // handle to DC
	WORD                biBits;     // bits per pixel

									// check if bitmap handle is valid

	if (!hBitmap)
		return NULL;

	// fill in BITMAP structure, return NULL if it didn't work

	if (!GetObject(hBitmap, sizeof(bm), (LPSTR)&bm))
		return NULL;

	// if no palette is specified, use default palette

	if (hPal == NULL)
		hPal = (HPALETTE)GetStockObject(DEFAULT_PALETTE);

	// calculate bits per pixel

	biBits = bm.bmPlanes * bm.bmBitsPixel;

	// make sure bits per pixel is valid

	if (biBits <= 1)
		biBits = 1;
	else if (biBits <= 4)
		biBits = 4;
	else if (biBits <= 8)
		biBits = 8;
	else // if greater than 8-bit, force to 24-bit
		biBits = 24;

	// initialize BITMAPINFOHEADER

	bi.biSize = sizeof(BITMAPINFOHEADER);
	bi.biWidth = bm.bmWidth;
	bi.biHeight = bm.bmHeight;
	bi.biPlanes = 1;
	bi.biBitCount = biBits;
	bi.biCompression = BI_RGB;
	bi.biSizeImage = 0;
	bi.biXPelsPerMeter = 0;
	bi.biYPelsPerMeter = 0;
	bi.biClrUsed = 0;
	bi.biClrImportant = 0;

	// calculate size of memory block required to store BITMAPINFO

	dwLen = bi.biSize + PaletteSize((LPSTR)&bi);

	// get a DC

	hDC = GetDC(NULL);

	// select and realize our palette

	hPal = SelectPalette(hDC, hPal, FALSE);
	RealizePalette(hDC);

	// alloc memory block to store our bitmap

	hDIB = GlobalAlloc(GHND, dwLen);

	// if we couldn't get memory block

	if (!hDIB)
	{
		// clean up and return NULL

		SelectPalette(hDC, hPal, TRUE);
		RealizePalette(hDC);
		ReleaseDC(NULL, hDC);
		return NULL;
	}

	// lock memory and get pointer to it

	lpbi = (LPBITMAPINFOHEADER)GlobalLock(hDIB);

	/// use our bitmap info. to fill BITMAPINFOHEADER

	*lpbi = bi;

	// call GetDIBits with a NULL lpBits param, so it will calculate the
	// biSizeImage field for us    

	GetDIBits(hDC, hBitmap, 0, (UINT)bi.biHeight, NULL, (LPBITMAPINFO)lpbi,
		DIB_RGB_COLORS);

	// get the info. returned by GetDIBits and unlock memory block

	bi = *lpbi;
	GlobalUnlock(hDIB);

	// if the driver did not fill in the biSizeImage field, make one up 
	if (bi.biSizeImage == 0)
		bi.biSizeImage = WIDTHBYTES((DWORD)bm.bmWidth * biBits) * bm.bmHeight;

	// realloc the buffer big enough to hold all the bits

	dwLen = bi.biSize + PaletteSize((LPSTR)&bi) + bi.biSizeImage;

	if (h = GlobalReAlloc(hDIB, dwLen, 0))
		hDIB = h;
	else
	{
		// clean up and return NULL

		GlobalFree(hDIB);
		hDIB = NULL;
		SelectPalette(hDC, hPal, TRUE);
		RealizePalette(hDC);
		ReleaseDC(NULL, hDC);
		return NULL;
	}

	// lock memory block and get pointer to it */

	lpbi = (LPBITMAPINFOHEADER)GlobalLock(hDIB);

	// call GetDIBits with a NON-NULL lpBits param, and actualy get the
	// bits this time

	if (GetDIBits(hDC, hBitmap, 0, (UINT)bi.biHeight, (LPSTR)lpbi +
		(WORD)lpbi->biSize + PaletteSize((LPSTR)lpbi), (LPBITMAPINFO)lpbi,
		DIB_RGB_COLORS) == 0)
	{
		// clean up and return NULL

		GlobalUnlock(hDIB);
		hDIB = NULL;
		SelectPalette(hDC, hPal, TRUE);
		RealizePalette(hDC);
		ReleaseDC(NULL, hDC);
		return NULL;
	}

	bi = *lpbi;

	// clean up 
	GlobalUnlock(hDIB);
	SelectPalette(hDC, hPal, TRUE);
	RealizePalette(hDC);
	ReleaseDC(NULL, hDC);

	// return handle to the DIB
	return hDIB;
}


/*************************************************************************
*
* PalEntriesOnDevice()
*
* Parameter:
*
* HDC hDC          - device context
*
* Return Value:
*
* int              - number of palette entries on device
*
* Description:
*
* This function gets the number of palette entries on the specified device
*
************************************************************************/

int PalEntriesOnDevice(HDC hDC)
{
	int nColors;  // number of colors

				  // Find out the number of colors on this device.

	nColors = (1 << (GetDeviceCaps(hDC, BITSPIXEL) * GetDeviceCaps(hDC, PLANES)));

	assert(nColors);
	return nColors;
}


/*************************************************************************
*
* GetSystemPalette()
*
* Parameters:
*
* None
*
* Return Value:
*
* HPALETTE         - handle to a copy of the current system palette
*
* Description:
*
* This function returns a handle to a palette which represents the system
* palette.  The system RGB values are copied into our logical palette using
* the GetSystemPaletteEntries function.
*
************************************************************************/

HPALETTE GetSystemPalette(void)
{
	HDC hDC;                // handle to a DC
	static HPALETTE hPal = NULL;   // handle to a palette
	HANDLE hLogPal;         // handle to a logical palette
	LPLOGPALETTE lpLogPal;  // pointer to a logical palette
	int nColors;            // number of colors

							// Find out how many palette entries we want.

	hDC = GetDC(NULL);

	if (!hDC)
		return NULL;

	nColors = PalEntriesOnDevice(hDC);   // Number of palette entries

										 // Allocate room for the palette and lock it.

	hLogPal = GlobalAlloc(GHND, sizeof(LOGPALETTE) + nColors *
		sizeof(PALETTEENTRY));

	// if we didn't get a logical palette, return NULL

	if (!hLogPal)
		return NULL;

	// get a pointer to the logical palette

	lpLogPal = (LPLOGPALETTE)GlobalLock(hLogPal);

	// set some important fields

	lpLogPal->palVersion = PALVERSION;
	lpLogPal->palNumEntries = nColors;

	// Copy the current system palette into our logical palette

	GetSystemPaletteEntries(hDC, 0, nColors,
		(LPPALETTEENTRY)(lpLogPal->palPalEntry));

	// Go ahead and create the palette.  Once it's created,
	// we no longer need the LOGPALETTE, so free it.    

	hPal = CreatePalette(lpLogPal);

	// clean up

	GlobalUnlock(hLogPal);
	GlobalFree(hLogPal);
	ReleaseDC(NULL, hDC);

	return hPal;
}


/*************************************************************************
*
* AllocRoomForDIB()
*
* Parameters:
*
* BITMAPINFOHEADER - bitmap info header stucture
*
* HBITMAP          - handle to the bitmap
*
* Return Value:
*
* HDIB             - handle to memory block
*
* Description:
*
*  This routine takes a BITMAPINOHEADER, and returns a handle to global
*  memory which can contain a DIB with that header.  It also initializes
*  the header portion of the global memory.  GetDIBits() is used to determine
*  the amount of room for the DIB's bits.  The total amount of memory
*  needed = sizeof(BITMAPINFOHEADER) + size of color table + size of bits.
*
************************************************************************/

HANDLE AllocRoomForDIB(BITMAPINFOHEADER bi, HBITMAP hBitmap)
{
	DWORD               dwLen;
	HANDLE              hDIB;
	HDC                 hDC;
	LPBITMAPINFOHEADER  lpbi;
	HANDLE              hTemp;

	// Figure out the size needed to hold the BITMAPINFO structure
	// (which includes the BITMAPINFOHEADER and the color table).

	dwLen = bi.biSize + PaletteSize((LPSTR)&bi);
	hDIB = GlobalAlloc(GHND, dwLen);

	// Check that DIB handle is valid

	if (!hDIB)
		return NULL;

	// Set up the BITMAPINFOHEADER in the newly allocated global memory,
	// then call GetDIBits() with lpBits = NULL to have it fill in the
	// biSizeImage field for us.

	lpbi = (LPBITMAPINFOHEADER)GlobalLock(hDIB);
	*lpbi = bi;

	hDC = GetDC(NULL);

	GetDIBits(hDC, hBitmap, 0, (UINT)bi.biHeight, NULL, (LPBITMAPINFO)lpbi,
		DIB_RGB_COLORS);
	ReleaseDC(NULL, hDC);

	// If the driver did not fill in the biSizeImage field,
	// fill it in -- NOTE: this is a bug in the driver!

	if (lpbi->biSizeImage == 0)
		lpbi->biSizeImage = WIDTHBYTES((DWORD)lpbi->biWidth *
			lpbi->biBitCount) * lpbi->biHeight;

	// Get the size of the memory block we need

	dwLen = lpbi->biSize + PaletteSize((LPSTR)&bi) + lpbi->biSizeImage;

	// Unlock the memory block

	GlobalUnlock(hDIB);

	// ReAlloc the buffer big enough to hold all the bits 

	if (hTemp = GlobalReAlloc(hDIB, dwLen, 0))
		return hTemp;
	else
	{
		// Else free memory block and return failure

		GlobalFree(hDIB);
		return NULL;
	}
}


/*************************************************************************
*
* ChangeDIBFormat()
*
* Parameter:
*
* HDIB             - handle to packed-DIB in memory
*
* WORD             - desired bits per pixel
*
* DWORD            - desired compression format
*
* Return Value:
*
* HDIB             - handle to the new DIB if successful, else NULL
*
* Description:
*
* This function will convert the bits per pixel and/or the compression
* format of the specified DIB. Note: If the conversion was unsuccessful,
* we return NULL. The original DIB is left alone. Don't use code like the
* following:
*
*    hMyDIB = ChangeDIBFormat(hMyDIB, 8, BI_RLE4);
*
* The conversion will fail, but hMyDIB will now be NULL and the original
* DIB will now hang around in memory. We could have returned the old
* DIB, but we wanted to allow the programmer to check whether this
* conversion succeeded or failed.
*
************************************************************************/

HDIB ChangeDIBFormat(HDIB hDIB, WORD wBitCount, DWORD dwCompression)
{
	HDC                hDC;             // Handle to DC
	HBITMAP            hBitmap;         // Handle to bitmap
	BITMAP             Bitmap;          // BITMAP data structure
	BITMAPINFOHEADER   bi;              // Bitmap info header
	LPBITMAPINFOHEADER lpbi;            // Pointer to bitmap info
	HDIB               hNewDIB = NULL;  // Handle to new DIB
	HPALETTE           hPal, hOldPal;   // Handle to palette, prev pal
	WORD               DIBBPP, NewBPP;  // DIB bits per pixel, new bpp
	DWORD              DIBComp, NewComp;// DIB compression, new compression

										// Check for a valid DIB handle

	if (!hDIB)
		return NULL;

	// Get the old DIB's bits per pixel and compression format

	lpbi = (LPBITMAPINFOHEADER)GlobalLock(hDIB);
	DIBBPP = ((LPBITMAPINFOHEADER)lpbi)->biBitCount;
	DIBComp = ((LPBITMAPINFOHEADER)lpbi)->biCompression;
	GlobalUnlock(hDIB);

	// Validate wBitCount and dwCompression
	// They must match correctly (i.e., BI_RLE4 and 4 BPP or
	// BI_RLE8 and 8BPP, etc.) or we return failure
	if (wBitCount == 0)
	{
		NewBPP = DIBBPP;
		if ((dwCompression == BI_RLE4 && NewBPP == 4) ||
			(dwCompression == BI_RLE8 && NewBPP == 8) ||
			(dwCompression == BI_RGB))
			NewComp = dwCompression;
		else
			return NULL;
	}
	else if (wBitCount == 1 && dwCompression == BI_RGB)
	{
		NewBPP = wBitCount;
		NewComp = BI_RGB;
	}
	else if (wBitCount == 4)
	{
		NewBPP = wBitCount;
		if (dwCompression == BI_RGB || dwCompression == BI_RLE4)
			NewComp = dwCompression;
		else
			return NULL;
	}
	else if (wBitCount == 8)
	{
		NewBPP = wBitCount;
		if (dwCompression == BI_RGB || dwCompression == BI_RLE8)
			NewComp = dwCompression;
		else
			return NULL;
	}
	else if (wBitCount == 24 && dwCompression == BI_RGB)
	{
		NewBPP = wBitCount;
		NewComp = BI_RGB;
	}
	else
		return NULL;

	// Save the old DIB's palette

	hPal = CreateDIBPalette(hDIB);
	if (!hPal)
		return NULL;

	// Convert old DIB to a bitmap

	hBitmap = DIBToBitmap(hDIB, hPal);
	if (!hBitmap)
	{
		DeleteObject(hPal);
		return NULL;
	}

	// Get info about the bitmap
	GetObject(hBitmap, sizeof(BITMAP), (LPSTR)&Bitmap);

	// Fill in the BITMAPINFOHEADER appropriately

	bi.biSize = sizeof(BITMAPINFOHEADER);
	bi.biWidth = Bitmap.bmWidth;
	bi.biHeight = Bitmap.bmHeight;
	bi.biPlanes = 1;
	bi.biBitCount = NewBPP;
	bi.biCompression = NewComp;
	bi.biSizeImage = 0;
	bi.biXPelsPerMeter = 0;
	bi.biYPelsPerMeter = 0;
	bi.biClrUsed = 0;
	bi.biClrImportant = 0;

	// Go allocate room for the new DIB

	hNewDIB = AllocRoomForDIB(bi, hBitmap);
	if (!hNewDIB)
		return NULL;

	// Get a pointer to the new DIB

	lpbi = (LPBITMAPINFOHEADER)GlobalLock(hNewDIB);

	// Get a DC and select/realize our palette in it

	hDC = GetDC(NULL);
	hOldPal = SelectPalette(hDC, hPal, FALSE);
	RealizePalette(hDC);

	// Call GetDIBits and get the new DIB bits

	if (!GetDIBits(hDC, hBitmap, 0, (UINT)lpbi->biHeight,
		(LPSTR)lpbi + (WORD)lpbi->biSize + PaletteSize((LPSTR)lpbi),
		(LPBITMAPINFO)lpbi, DIB_RGB_COLORS))
	{
		GlobalUnlock(hNewDIB);
		GlobalFree(hNewDIB);
		hNewDIB = NULL;
	}

	// Clean up and return

	SelectPalette(hDC, hOldPal, TRUE);
	RealizePalette(hDC);
	ReleaseDC(NULL, hDC);

	// Unlock the new DIB's memory block
	if (hNewDIB)
		GlobalUnlock(hNewDIB);

	DeleteObject(hBitmap);
	DeleteObject(hPal);

	return hNewDIB;
}


/*************************************************************************
*
* ChangeBitmapFormat()
*
* Parameter:
*
* HBITMAP          - handle to a bitmap
*
* WORD             - desired bits per pixel
*
* DWORD            - desired compression format
*
* HPALETTE         - handle to palette
*
* Return Value:
*
* HDIB             - handle to the new DIB if successful, else NULL
*
* Description:
*
* This function will convert a bitmap to the specified bits per pixel
* and compression format. The bitmap and it's palette will remain
* after calling this function.
*
************************************************************************/

HDIB ChangeBitmapFormat(HBITMAP hBitmap, WORD wBitCount, DWORD dwCompression,
	HPALETTE hPal)
{
	HDC                hDC;          // Screen DC
	HDIB               hNewDIB = NULL; // Handle to new DIB
	BITMAP             Bitmap;       // BITMAP data structure
	BITMAPINFOHEADER   bi;           // Bitmap info. header
	LPBITMAPINFOHEADER lpbi;         // Pointer to bitmap header
	HPALETTE           hOldPal = NULL; // Handle to palette
	WORD               NewBPP;       // New bits per pixel
	DWORD              NewComp;      // New compression format

									 // Check for a valid bitmap handle

	if (!hBitmap)
		return NULL;

	if (wBitCount == 0)
	{
		NewComp = dwCompression;
		if (NewComp == BI_RLE4)
			NewBPP = 4;
		else if (NewComp == BI_RLE8)
			NewBPP = 8;
		else // Not enough info */
			return NULL;
	}
	else if (wBitCount == 1 && dwCompression == BI_RGB)
	{
		NewBPP = wBitCount;
		NewComp = BI_RGB;
	}
	else if (wBitCount == 4)
	{
		NewBPP = wBitCount;
		if (dwCompression == BI_RGB || dwCompression == BI_RLE4)
			NewComp = dwCompression;
		else
			return NULL;
	}
	else if (wBitCount == 8)
	{
		NewBPP = wBitCount;
		if (dwCompression == BI_RGB || dwCompression == BI_RLE8)
			NewComp = dwCompression;
		else
			return NULL;
	}
	else if (wBitCount == 24 && dwCompression == BI_RGB)
	{
		NewBPP = wBitCount;
		NewComp = BI_RGB;
	}
	else
		return NULL;

	// Get info about the bitmap

	GetObject(hBitmap, sizeof(BITMAP), (LPSTR)&Bitmap);

	// Fill in the BITMAPINFOHEADER appropriately

	bi.biSize = sizeof(BITMAPINFOHEADER);
	bi.biWidth = Bitmap.bmWidth;
	bi.biHeight = Bitmap.bmHeight;
	bi.biPlanes = 1;
	bi.biBitCount = NewBPP;
	bi.biCompression = NewComp;
	bi.biSizeImage = 0;
	bi.biXPelsPerMeter = 0;
	bi.biYPelsPerMeter = 0;
	bi.biClrUsed = 0;
	bi.biClrImportant = 0;

	// Go allocate room for the new DIB

	hNewDIB = AllocRoomForDIB(bi, hBitmap);
	if (!hNewDIB)
		return NULL;

	// Get a pointer to the new DIB

	lpbi = (LPBITMAPINFOHEADER)GlobalLock(hNewDIB);

	// If we have a palette, get a DC and select/realize it

	if (hPal)
	{
		hDC = GetDC(NULL);
		hOldPal = SelectPalette(hDC, hPal, FALSE);
		RealizePalette(hDC);
	}

	// Call GetDIBits and get the new DIB bits

	if (!GetDIBits(hDC, hBitmap, 0, (UINT)lpbi->biHeight, (LPSTR)lpbi +
		(WORD)lpbi->biSize + PaletteSize((LPSTR)lpbi), (LPBITMAPINFO)lpbi,
		DIB_RGB_COLORS))
	{
		GlobalUnlock(hNewDIB);
		GlobalFree(hNewDIB);
		hNewDIB = NULL;
	}

	// Clean up and return

	if (hOldPal)
	{
		SelectPalette(hDC, hOldPal, TRUE);
		RealizePalette(hDC);
		ReleaseDC(NULL, hDC);
	}

	// Unlock the new DIB's memory block

	if (hNewDIB)
		GlobalUnlock(hNewDIB);

	return hNewDIB;
}