//
//  can4osx_usb_core.c
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

#include <stdio.h>
#include <stdlib.h>

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/IOMessage.h>
#include <IOKit/IOCFPlugIn.h>
#include <IOKit/usb/IOUSBLib.h>

#include <sys/time.h>

#include "can4osx_internal.h"
#include "can4osx_debug.h"




/******************************************************************************/
canStatus CAN4OSX_usbSendCommand(
		Can4osxUsbDeviceHandleEntry *pSelf,  /**< pointer to my reference */
		void *pCmd,
		size_t cmdLen
	)
{
IOReturn retVal = kIOReturnSuccess;
CAN4OSX_USB_INTERFACE **ppInterface = pSelf->can4osxInterfaceInterface;

	if( pSelf->endpoitBulkOutBusy == FALSE )  {
		pSelf->endpoitBulkOutBusy = TRUE;

		retVal = (*ppInterface)->WritePipe(ppInterface,
										 pSelf->endpointNumberBulkOut,
										 pCmd, (UInt32)cmdLen);

		if (retVal != kIOReturnSuccess)  {
			CAN4OSX_DEBUG_PRINT("Unable to perform synchronous bulk write (%08x)\n", retVal);
			(void)(*ppInterface)->USBInterfaceClose(ppInterface);
			(void)(*ppInterface)->Release(ppInterface);
		}

		pSelf->endpoitBulkOutBusy = FALSE;
	} else {
		retVal = kIOReturnError;
	}

	if (retVal != kIOReturnSuccess)  {
	    return(canERR_INTERNAL);
	}
	return(canOK);
}


/******************************************************************************/
void CAN4OSX_usbReadFromBulkInPipe(
		Can4osxUsbDeviceHandleEntry *pSelf /**< pointer to handle structure */
	)
{
IOReturn ret = (*(pSelf->can4osxInterfaceInterface))->ReadPipeAsync(pSelf->can4osxInterfaceInterface, pSelf->endpointNumberBulkIn, pSelf->endpointBufferBulkInRef, pSelf->endpointMaxSizeBulkIn, pSelf->usbFunctions.bulkReadCompletion, (void*)pSelf);

	if (ret != kIOReturnSuccess)  {
		CAN4OSX_DEBUG_PRINT("Unable to read async interface (%08x)\n", ret);
	}
}



