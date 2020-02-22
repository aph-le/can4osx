//
//  peakUsbFd.c
//
//
// Copyright (c) 2014-2020 Alexander Philipp. All rights reserved.
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


/* header of standard C - libraries
------------------------------------------------------------------------------*/
#include <stdio.h>

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/IOMessage.h>
#include <IOKit/IOCFPlugIn.h>
#include <IOKit/usb/IOUSBLib.h>

#include <pthread.h>


/* header of project specific types
------------------------------------------------------------------------------*/
#include "can4osx.h"
#include "can4osx_debug.h"
#include "can4osx_internal.h"
#include "can4osx_usb_core.h"


/* constant definitions
------------------------------------------------------------------------------*/

/* local defined data types
------------------------------------------------------------------------------*/

/* list of local defined functions
------------------------------------------------------------------------------*/
static canStatus usbFdInitHardware(const CanHandle hnd, UInt16 productId);
static char* usbFdGetDeviceName(UInt16 productId);

/* global variables
------------------------------------------------------------------------------*/
CAN4OSX_HW_FUNC_T peakUsbFdHardwareFunctions = {
    .can4osxhwInitRef = usbFdInitHardware,
    .can4osxhwCanOpenChannel = NULL,
    .can4osxhwCanSetBusParamsRef = NULL,
    .can4osxhwCanSetBusParamsFdRef = NULL,
    .can4osxhwCanBusOnRef = NULL,
    .can4osxhwCanBusOffRef = NULL,
    .can4osxhwCanWriteRef = NULL,
    .can4osxhwCanReadRef = NULL,
    .can4osxhwCanCloseRef = NULL,
};


/* local defined variables
------------------------------------------------------------------------------*/
static CAN4OSX_DEVICE_NAME_T prId2Name[] = {
	{0x0012, "Peak USB-FD"},
	{0x0000, "Unknown Peak Device"},
};


static canStatus usbFdInitHardware(
		const CanHandle hnd,
		UInt16 productId
    )
{
Can4osxUsbDeviceHandleEntry *pSelf = &can4osxUsbDeviceHandle[hnd];
char* pDevName;
#if 0
	pSelf->privateData = calloc(1,sizeof(IXXUSBFDPRIVATEDATA_T));

    if ( pSelf->privateData != NULL ) {
    	IXXUSBFDPRIVATEDATA_T *pPriv = (IXXUSBFDPRIVATEDATA_T *)pSelf->privateData;
     	pPriv->pParent = pSelf;

      	pPriv->pTransBuff.bufferGDCqueueRef = dispatch_queue_create("com.can4osx.ixxusbfdmsgqueue", 0);
		pPriv->pTransBuff.bufferCount = 0u;
        pPriv->pTransBuff.bufferFirst = 0u;
        pPriv->pTransBuff.bufferSize = IXXCOMMANDBUF_SIZE;
        pthread_mutex_init(&(pPriv->mutex), NULL);

    } else {
        return(canERR_NOMEM);
    }
#endif
	/* create new endpoint buffer */
	pSelf->endpointBufferBulkInRef = calloc( 1 , pSelf->endpointMaxSizeBulkIn);
	pSelf->endpointBufferBulkOutRef = calloc( 1 , pSelf->endpointMaxSizeBulkOut);

	pDevName = usbFdGetDeviceName(productId);

    sprintf((char*)pSelf->devInfo.deviceString, "%s %d/%d",pDevName,
			pSelf->deviceChannel + 1, pSelf->deviceChannelCount);

    pSelf->devInfo.capability = 0u;
    pSelf->devInfo.capability |= canCHANNEL_CAP_CAN_FD;

    return(canOK);
}


static char* usbFdGetDeviceName(
		UInt16 productId
	)
{
char* pRetName = NULL;

	for (UInt i = 0; i < sizeof(prId2Name); i++)  {
		if (productId == prId2Name[i].productId)  {
			pRetName = prId2Name[i].pName;
			break;
		}
		pRetName = prId2Name[i].pName;;
	}

	return(pRetName);
}
