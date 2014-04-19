//
//  can4osx_internal.c
//
//
// Copyright (c) 2014 Alexander Philipp. All rights reserved.
//
//
// License: GPLv2
//
// ===============================================================================
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation version 2
// of the license.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// ===============================================================================
//
// Disclaimer:     IMPORTANT: THE SOFTWARE IS PROVIDED ON AN "AS IS" BASIS. THE AUTHOR MAKES NO
// WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION THE IMPLIED
// WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE, REGARDING THE SOFTWARE OR ITS USE AND OPERATION ALONE OR IN
// COMBINATION WITH YOUR PRODUCTS.
//
// IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
//                       GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION, MODIFICATION AND/OR DISTRIBUTION
// OF SOFTWARE, HOWEVER CAUSED AND WHETHER UNDER THEORY OF CONTRACT, TORT
// (INCLUDING NEGLIGENCE), STRICT LIABILITY OR OTHERWISE, EVEN IF THE AUTHOR HAS
// BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// ===============================================================================
//

#include <stdio.h>
#include <stdlib.h>

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/IOMessage.h>
#include <IOKit/IOCFPlugIn.h>
#include <IOKit/usb/IOUSBLib.h>

#include "can4osx_internal.h"




CanEventMsgBuf* CAN4OSX_CreateCanEventBuffer( UInt32 bufferSize )
{
	CanEventMsgBuf* bufferRef = malloc(sizeof(CanEventMsgBuf));
	if ( bufferRef == NULL ) {
		return NULL;
	}
    
	bufferRef->bufferSize = bufferSize;
	bufferRef->bufferCount = 0;
	bufferRef->bufferFirst = 0;
    
	bufferRef->canEventRef = malloc(bufferSize * sizeof(CanEvent));
    
	if ( bufferRef->canEventRef == NULL ) {
		free(bufferRef);
		return NULL;
	}
    
    bufferRef->bufferGDCqueueRef = dispatch_queue_create("com.can4osx.caneventqueue", 0);
	if ( bufferRef->bufferGDCqueueRef == NULL ) {
		CAN4OSX_ReleaseCanEventBuffer(bufferRef);
		return NULL;
	}
    
	return bufferRef;
}

void CAN4OSX_ReleaseCanEventBuffer( CanEventMsgBuf* bufferRef )
{
	if ( bufferRef != NULL ) {
        dispatch_release(bufferRef->bufferGDCqueueRef);
		free(bufferRef->canEventRef);
		free(bufferRef);
	}
}

UInt8 CAN4OSX_TestFullCanEventBuffer(CanEventMsgBuf* bufferRef)
{
	if (bufferRef->bufferCount == bufferRef->bufferSize) {
		return 1;
	} else {
		return 0;
	}
}

UInt8 CAN4OSX_TestEmptyCanEventBuffer(CanEventMsgBuf* bufferRef)
{
    if ( bufferRef->bufferCount == 0 ) {
        return 1;
    } else {
        return 0;
    }
}


UInt8 CAN4OSX_WriteCanEventBuffer(CanEventMsgBuf* bufferRef, CanEvent newEvent)
{
    __block UInt8 retval = 1;
    
    dispatch_sync(bufferRef->bufferGDCqueueRef, ^{
        if (CAN4OSX_TestFullCanEventBuffer(bufferRef)) {
            retval = 0;
        } else {
            bufferRef->canEventRef[(bufferRef->bufferFirst + bufferRef->bufferCount++) % bufferRef->bufferSize] = newEvent;
        }
    });
    
	return retval;
}

UInt8 CAN4OSX_ReadCanEventBuffer(CanEventMsgBuf* bufferRef, CanEvent* readEvent)
{
    __block UInt8 retval = 1;
    
    dispatch_sync(bufferRef->bufferGDCqueueRef, ^{
        if (CAN4OSX_TestEmptyCanEventBuffer(bufferRef)) {
            retval = 0;
        } else {
            bufferRef->bufferCount--;
            *readEvent = bufferRef->canEventRef[bufferRef->bufferFirst++ % bufferRef->bufferSize];
        }

    });
    
	return retval;

}



