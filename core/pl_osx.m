/*
Copyright (C) 1996-2001 Id Software, Inc.
Copyright (C) 2002-2005 John Fitzgibbons and others
Copyright (C) 2007-2008 Kristian Duske
Copyright (C) 2010-2014 QuakeSpasm developers

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "quakedef.h"
#if defined(SDL_FRAMEWORK) || defined(NO_SDL_CONFIG)
#if defined(USE_SDL2)
#include <SDL2/SDL.h>
#else
#include <SDL/SDL.h>
#endif
#else
#include "SDL.h"
#endif
#import <Cocoa/Cocoa.h>

void PL_SetWindowIcon (void)
{
/* nothing to do on OS X */
}

void PL_VID_Shutdown (void)
{
}

#if MAC_OS_X_VERSION_MIN_REQUIRED < 1060
#define NSPasteboardTypeString NSStringPboardType
#endif
#define MAX_CLIPBOARDTXT	MAXCMDLINE	/* 256 */
char *PL_GetClipboardData (void)
{
    char *data			= NULL;
    NSPasteboard* pasteboard	= [NSPasteboard generalPasteboard];
    NSArray* types		= [pasteboard types];

    if ([types containsObject: NSPasteboardTypeString]) {
	NSString* clipboardString = [pasteboard stringForType: NSPasteboardTypeString];
	if (clipboardString != NULL && [clipboardString length] > 0) {
		const char* srcdata = NULL;

#if (MAC_OS_X_VERSION_MIN_REQUIRED < 1040)	/* for ppc builds targeting 10.3 and older */
		if ([clipboardString canBeConvertedToEncoding:NSASCIIStringEncoding])
			srcdata = [clipboardString cString];
#else
		srcdata = [clipboardString cStringUsingEncoding: NSASCIIStringEncoding];
#endif

		if (srcdata != NULL)
		{
			size_t sz = [clipboardString length] + 1;
			sz = q_min((size_t)(MAX_CLIPBOARDTXT), sz);
			data = (char *) Z_Malloc((int)sz);
			q_strlcpy (data, srcdata, sz);
		}
	}
    }
    return data;
}

#ifndef MAC_OS_X_VERSION_10_12
#define NSAlertStyleCritical NSCriticalAlertStyle
#endif
void PL_ErrorDialog(const char *errorMsg)
{
#if (MAC_OS_X_VERSION_MIN_REQUIRED < 1040)	/* ppc builds targeting 10.3 and older */
    NSString* msg = [NSString stringWithCString:errorMsg];
#else
    NSString* msg = [NSString stringWithCString:errorMsg encoding:NSASCIIStringEncoding];
#endif
#if MAC_OS_X_VERSION_MIN_REQUIRED < 1030
    NSRunCriticalAlertPanel (@"Quake Error", @"%@", @"OK", nil, nil, msg);
#else
    NSAlert *alert = [[[NSAlert alloc] init] autorelease];
    alert.alertStyle = NSAlertStyleCritical;
    alert.messageText = @"Quake Error";
    alert.informativeText = msg;
    [alert runModal];
#endif
}

