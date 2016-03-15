//
//  kvaserLeafPro.c
//  can4osx_cmd
//
//
// Copyright (c) 2016 Alexander Philipp. All rights reserved.
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

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/IOMessage.h>
#include <IOKit/IOCFPlugIn.h>
#include <IOKit/usb/IOUSBLib.h>


/* can4osx */
#include "can4osx.h"
#include "can4osx_debug.h"
#include "can4osx_internal.h"

#include "kvaserLeafPro.h"

#define LEAFPRO_COMMAND_SIZE 32
# define LEAFPRO_TIMEOUT_ONE_MS 1000000
# define LEAFPRO_TIMEOUT_TEN_MS 10*LEAFPRO_TIMEOUT_ONE_MS

static char* pDeviceString = "Kvaser Leaf Pro v.2";

static void LeafProDecodeCommand(Can4osxUsbDeviceHandleEntry *pSelf, proCommand_t *pCmd);

static void LeafProMapChannels(Can4osxUsbDeviceHandleEntry *pSelf);

static IOReturn LeafProWriteCommandWait(Can4osxUsbDeviceHandleEntry *pSelf, proCommand_t cmd, UInt8 reason);


static void LeafProReadFromBulkInPipe(Can4osxUsbDeviceHandleEntry *self);
static void LeafProBulkReadCompletion(void *refCon, IOReturn result, void *arg0);


//Hardware interface function
static canStatus LeafProInitHardware(const CanHandle hnd);

Can4osxHwFunctions leafProHardwareFunctions = {
    .can4osxhwInitRef = LeafProInitHardware,
    .can4osxhwCanSetBusParamsRef = NULL,
    .can4osxhwCanBusOnRef = NULL,
    .can4osxhwCanBusOffRef = NULL,
    .can4osxhwCanWriteRef = NULL,
    .can4osxhwCanReadRef = NULL,
    .can4osxhwCanCloseRef = NULL,
};

static canStatus LeafProInitHardware(const CanHandle hnd)
{
    Can4osxUsbDeviceHandleEntry *pSelf = &can4osxUsbDeviceHandle[hnd];
    pSelf->privateData = calloc(1,sizeof(LeafProPrivateData));

    if ( pSelf->privateData != NULL ) {
        LeafProPrivateData *priv = (LeafProPrivateData *)pSelf->privateData;
                
        priv->semaTimeout = dispatch_semaphore_create(0);
        
    } else {
        return canERR_NOMEM;
    }

    
    // Set some device Infos
    sprintf((char*)pSelf->devInfo.deviceString, "%s",pDeviceString);
    
    // Trigger next read
    LeafProReadFromBulkInPipe(pSelf);
    
    // Set up channels
    LeafProMapChannels(pSelf);
    
    return canOK;
}

#pragma mark LeafPro command stuff
static void LeafProDecodeCommand(Can4osxUsbDeviceHandleEntry *pSelf, proCommand_t *pCmd)
{
    CAN4OSX_DEBUG_PRINT("Pro-Decode command %d",(UInt8)pCmd->proCmdHead.cmdNo);
}


#pragma mark Leaf Pro mapping Stuff
static void LeafProMapChannels(Can4osxUsbDeviceHandleEntry *pSelf)
{
    proCommand_t cmd;
    
    cmd.proCmdHead.cmdNo = LEAFPRO_CMD_MAP_CHANNEL_REQ;
        
    LeafProWriteCommandWait(pSelf, cmd, LEAFPRO_CMD_MAP_CHANNEL_RESP);


}



#pragma mark USB-Stuff

static IOReturn LeafProWriteCommandWait(Can4osxUsbDeviceHandleEntry *pSelf, proCommand_t cmd, UInt8 reason)
{
    IOReturn retVal = kIOReturnSuccess;
    IOUSBInterfaceInterface **interface = pSelf->can4osxInterfaceInterface;
    LeafProPrivateData *pPriv = (LeafProPrivateData *)pSelf->privateData;
    
    if( pSelf->endpoitBulkOutBusy == FALSE ) {
        
        pSelf->endpoitBulkOutBusy = TRUE;
        
        retVal = (*interface)->WritePipe( interface, pSelf->endpointNumberBulkOut, &cmd, LEAFPRO_COMMAND_SIZE);
        
        if (retVal != kIOReturnSuccess) {
            CAN4OSX_DEBUG_PRINT("Unable to perform synchronous bulk write (%08x)\n", retVal);
            (void) (*interface)->USBInterfaceClose(interface);
            (void) (*interface)->Release(interface);
        }
        
        pSelf->endpoitBulkOutBusy = FALSE;
    } else {
        //Endpoint busy
        //LeafPrivateData *priv = (LeafPrivateData *)self->privateData;
        //LeafWriteCommandBuffer(priv->cmdBufferRef, cmd);
    }
    
    pPriv->timeOutReason = reason;
    
    if ( dispatch_semaphore_wait(pPriv->semaTimeout, dispatch_time(DISPATCH_TIME_NOW, LEAFPRO_TIMEOUT_TEN_MS)) ) {
        return canERR_TIMEOUT;
    } else {
        return retVal;
    }
}


static void LeafProBulkReadCompletion(void *refCon, IOReturn result, void *arg0)
{
    Can4osxUsbDeviceHandleEntry *pSelf = (Can4osxUsbDeviceHandleEntry *)refCon;
    LeafProPrivateData *pPriv = (LeafProPrivateData *)pSelf->privateData;
    IOUSBInterfaceInterface **interface = pSelf->can4osxInterfaceInterface;
    UInt32 numBytesRead = (UInt32) arg0;
    
    CAN4OSX_DEBUG_PRINT("Asynchronous bulk read complete (%ld)\n", (long)numBytesRead);
    
    if (result != kIOReturnSuccess) {
        printf("Error from async bulk read (%08x)\n", result);
        (void) (*interface)->USBInterfaceClose(interface);
        (void) (*interface)->Release(interface);
        return;
    }
    
    if (numBytesRead > 0u) {
        int count = 0;
        proCommand_t *pCmd;
        int loopCounter = pSelf->endpointMaxSizeBulkIn;
        
        while ( count < numBytesRead ) {
            if (loopCounter-- == 0) break;
            
            pCmd = (proCommand_t *)&(pSelf->endpointBufferBulkInRef[count]);
            switch (pCmd->proCmdHead.cmdNo) {
                case 255:
                    // Special Command
                    break;
                case 0:
                    // No comand
                    count += pSelf->endpointMaxSizeBulkIn;;
                    count &= -pSelf->endpointMaxSizeBulkIn;
                    break;
                default:
                    // Hopefully a real command
                    LeafProDecodeCommand(pSelf, pCmd);
                    count += LEAFPRO_COMMAND_SIZE;
                    break;
            }
            if (pCmd->proCmdHead.cmdNo == pPriv->timeOutReason) {
                pPriv->timeOutReason = 0;
                dispatch_semaphore_signal(pPriv->semaTimeout);
            }
        }
    }
    
    // Trigger next read
    LeafProReadFromBulkInPipe(pSelf);
}

static void LeafProReadFromBulkInPipe(Can4osxUsbDeviceHandleEntry *pSelf)
{
    IOReturn ret = (*(pSelf->can4osxInterfaceInterface))->ReadPipeAsync(pSelf->can4osxInterfaceInterface, pSelf->endpointNumberBulkIn, pSelf->endpointBufferBulkInRef, pSelf->endpointMaxSizeBulkIn, LeafProBulkReadCompletion, (void*)pSelf);
    
    if (ret != kIOReturnSuccess) {
        CAN4OSX_DEBUG_PRINT("Unable to read async interface (%08x)\n", ret);
    }
}