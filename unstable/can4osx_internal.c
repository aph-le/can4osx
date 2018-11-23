//
//  can4osx_internal.c
//
//
// Copyright (c) 2014 - 2018 Alexander Philipp. All rights reserved.
//
//
// License: GPLv2
//
// =============================================================================
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
// Foundation, Inc., 51 Franklin Street,
// Fifth Floor, Boston, MA  02110-1301, USA.
//
// =============================================================================
//
// Disclaimer:     IMPORTANT: THE SOFTWARE IS PROVIDED ON AN "AS IS" BASIS. THE
// AUTHOR MAKES NO WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION
// THE IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A
// PARTICULAR PURPOSE, REGARDING THE SOFTWARE OR ITS USE AND OPERATION ALONE OR
// IN COMBINATION WITH YOUR PRODUCTS.
//
// IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL
// OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION, MODIFICATION
// AND/OR DISTRIBUTION OF SOFTWARE, HOWEVER CAUSED AND WHETHER UNDER THEORY OF
// CONTRACT, TORT (INCLUDING NEGLIGENCE), STRICT LIABILITY OR OTHERWISE, EVEN IF
// THE AUTHOR HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// =============================================================================
//

#include <stdio.h>
#include <stdlib.h>

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/IOMessage.h>
#include <IOKit/IOCFPlugIn.h>
#include <IOKit/usb/IOUSBLib.h>

#include <sys/time.h>

#include "can4osx_internal.h"
#include "can4osx_usb_core.h"
#include "can4osx_debug.h"


static UInt8 CAN4OSX_TestFullCanEventBuffer(CAN_EVENT_MSG_BUF_T* bufferRef);
static UInt8 CAN4OSX_TestEmptyCanEventBuffer(CAN_EVENT_MSG_BUF_T* bufferRef);


/******************************************************************************/
CAN_EVENT_MSG_BUF_T* CAN4OSX_CreateCanEventBuffer(
        UInt32 bufferSize
    )
{
	CAN_EVENT_MSG_BUF_T* bufferRef = malloc(sizeof(CAN_EVENT_MSG_BUF_T));
	if ( bufferRef == NULL )  {
		return(NULL);
	}

	bufferRef->bufferSize = bufferSize;
	bufferRef->bufferCount = 0;
	bufferRef->bufferFirst = 0;

	bufferRef->canMsgRef = malloc(bufferSize * sizeof(CanMsg));

	if ( bufferRef->canMsgRef == NULL )  {
		free(bufferRef);
		bufferRef = NULL;
		return(NULL);
	}

	bufferRef->bufferGDCqueueRef = dispatch_queue_create("com.can4osx.caneventqueue", 0);
	if ( bufferRef->bufferGDCqueueRef == NULL )  {
		CAN4OSX_ReleaseCanEventBuffer(bufferRef);
		return(NULL);
	}

	return(bufferRef);
}


/******************************************************************************/
void CAN4OSX_ReleaseCanEventBuffer(
		CAN_EVENT_MSG_BUF_T* bufferRef
	)
{
	if ( bufferRef != NULL )  {
		if (bufferRef->bufferGDCqueueRef != NULL)  {
			dispatch_release(bufferRef->bufferGDCqueueRef);
		}
		free(bufferRef->canMsgRef);
        bufferRef->canMsgRef = NULL;
        
		free(bufferRef);
		bufferRef = NULL;
	}
}


/******************************************************************************/
static UInt8 CAN4OSX_TestFullCanEventBuffer(
		CAN_EVENT_MSG_BUF_T* bufferRef
	)
{
	if (bufferRef->bufferCount == bufferRef->bufferSize)  {
		return(1);
	} else {
		return(0);
	}
}


/******************************************************************************/
static UInt8 CAN4OSX_TestEmptyCanEventBuffer(
		CAN_EVENT_MSG_BUF_T* bufferRef
	)
{
	if ( bufferRef->bufferCount == 0 )  {
		return(1);
	} else {
		return(0);
	}
}


/******************************************************************************/
UInt8 CAN4OSX_WriteCanEventBuffer(
		CAN_EVENT_MSG_BUF_T* bufferRef,
		CanMsg newEvent
	)
{
__block UInt8 retval = 1;

	dispatch_sync(bufferRef->bufferGDCqueueRef, ^{
		if (CAN4OSX_TestFullCanEventBuffer(bufferRef))  {
			retval = 0;
		} else {
			bufferRef->canMsgRef[(bufferRef->bufferFirst + bufferRef->bufferCount++) % bufferRef->bufferSize] = newEvent;
		}
	});

	return(retval);
}


/******************************************************************************/
UInt8 CAN4OSX_ReadCanEventBuffer(
		CAN_EVENT_MSG_BUF_T* bufferRef,
		CanMsg* readEvent
	)
{
__block UInt8 retval = 1;

	dispatch_sync(bufferRef->bufferGDCqueueRef, ^{
		if (CAN4OSX_TestEmptyCanEventBuffer(bufferRef))  {
			retval = 0;
		} else {
			bufferRef->bufferCount--;
            *readEvent = bufferRef->canMsgRef[bufferRef->bufferFirst++ % bufferRef->bufferSize];
		}

	});

	return(retval);

}


/******************************************************************************/
canStatus CAN4OSX_GetChannelData(
		Can4osxUsbDeviceHandleEntry* pSelf,
		SInt32 cmd,
		void* pBuffer,
		size_t bufsize
	)
{

	switch(cmd) {
		case canCHANNELDATA_CARD_SERIAL_NO:
			if (bufsize < 8)  {
				return(canERR_NOMEM);
			}
			memcpy(pBuffer, &(pSelf->devInfo.serialNumber), 8);
			break;
		case canCHANNELDATA_DEVDESCR_ASCII:
			if (bufsize > 128)  {
				bufsize = 128;
			}
			memcpy(pBuffer, pSelf->devInfo.deviceString, bufsize);
			break;
		case canCHANNELDATA_CHANNEL_CAP:
			if (bufsize < 4)  {
				return(canERR_NOMEM);
			}
			memcpy(pBuffer, &(pSelf->devInfo.capability), 4);
			break;
		default:
			break;
	}
	return(canOK);
}


/******************************************************************************/
/**
* \brief CAN4OSX_decodeFdDlc - decode the dlc to data length
*
* \return the data length code
*/
UInt8 CAN4OSX_decodeFdDlc(
		UInt8 dlc
	)
{
static const UInt8 len[16u] = {0u,1u,2u,3u,4u,5u,6u,7u,8u,12u,16u,20u,24u,32u,48u,64u};

	return(len[dlc]);
}


/******************************************************************************/
/**
* \brief CAN4OSX_decodeFdDlc - decode the dlc to data length
*
* \return the datalength
*/
UInt8 CAN4OSX_encodeFdDlc(
		UInt8 dlc
	)
{
	if (dlc == 12u)  {
		return(0x09);
	}
	if (dlc == 16u)  {
		return(0x0A);
	}
	if (dlc == 20u)  {
		return(0x0B);
	}
	if (dlc == 24u)  {
		return(0x0C);
	}
	if (dlc == 32u)  {
		return(0x0D);
	}
	if (dlc == 48u)  {
		return(0x0E);
	}
	if (dlc == 64u)  {
		return(0x0F);
	}
    /* invalid dlc */
	if (dlc > 8u)  {
		return(0xff);
	}

	return(dlc);
}


/******************************************************************************/
/**
* \brief OSX_getMilliseconds - get the actual milliseconds
*
* \return milliseconds
*/
UInt64 CAN$OSX_getMilliseconds(
		void
	)
{
struct timeval time;

	gettimeofday(&time, NULL);
    return(time.tv_sec * 1000) + (time.tv_usec / 1000);
}

