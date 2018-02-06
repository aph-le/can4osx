//
//  ixxatUsbFd.c
//
//
// Copyright (c) 2018 Alexander Philipp. All rights reserved.
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

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/IOMessage.h>
#include <IOKit/IOCFPlugIn.h>
#include <IOKit/usb/IOUSBLib.h>


/* can4osx */
#include "can4osx.h"
#include "can4osx_debug.h"
#include "can4osx_internal.h"

/* Leaf functions */
#include "ixxatUsbFd.h"

#define IXXUSBFD_MAX_RX_ENDPOINT 4u

typedef struct {
	Can4osxUsbDeviceHandleEntry *pParent;
	UInt8 inEndPoint;
	UInt8 *pEndpointInBuffer[IXXUSBFD_MAX_RX_ENDPOINT];
    UInt8 *pEndpointOutBuffer;
    UInt8 canFd;
    UInt32  brp;
    UInt8   tseg1;
    UInt8   tseg2;
    UInt8   sjw;
    UInt32  fd_brp;
    UInt8   fd_tseg1;
    UInt8   fd_tseg2;
    UInt8   fd_sjw;
} usbFdPrivateData_t;


static char* pDeviceString = "IXXAT USB-to-USB FD";

static canStatus usbFdInitHardware(const CanHandle hnd);
static CanHandle usbFdCanOpenChannel(int channel, int flags);

static canStatus usbFdCanTranslateBaud (
        SInt32 *const freq,
        unsigned int *const tseg1,
        unsigned int *const tseg2,
        unsigned int *const sjw,
        unsigned int *const nosamp,
        unsigned int *const syncMode
    );

static canStatus usbFdSetPowerMode(Can4osxUsbDeviceHandleEntry *pSelf, UInt8 mode);
static canStatus usbFdGetDeviceCaps(Can4osxUsbDeviceHandleEntry *pSelf);

static canStatus usbFdSendCmd(Can4osxUsbDeviceHandleEntry *pSelf, IXXUSBFDMSGREQHEAD_T *pCmd);
static canStatus usbFdRecvCmd(Can4osxUsbDeviceHandleEntry *pSelf, ixxUsbFdMsgRespHead_t *pCmd, int value);

static void usbFdBulkReadCompletion(void *refCon, IOReturn result, void *arg0);
static void usbFdReadFromBulkInPipe(Can4osxUsbDeviceHandleEntry *self);



Can4osxHwFunctions ixxUsbFdHardwareFunctions = {
    .can4osxhwInitRef = usbFdInitHardware,
    .can4osxhwCanOpenChannel = usbFdCanOpenChannel,
    .can4osxhwCanSetBusParamsRef = NULL,
    .can4osxhwCanSetBusParamsFdRef = NULL,
    .can4osxhwCanBusOnRef = NULL,
    .can4osxhwCanBusOffRef = NULL,
    .can4osxhwCanWriteRef = NULL,
    .can4osxhwCanReadRef = NULL,
    .can4osxhwCanCloseRef = NULL,
};


static canStatus usbFdInitHardware(const CanHandle hnd)
{
    Can4osxUsbDeviceHandleEntry *self = &can4osxUsbDeviceHandle[hnd];
    self->privateData = calloc(1,sizeof(usbFdPrivateData_t));
    
    if ( self->privateData != NULL ) {
    	usbFdPrivateData_t *priv = (usbFdPrivateData_t *)self->privateData;
     	priv->pParent = self;
    
    	for (UInt8 i = 0u; i < IXXUSBFD_MAX_RX_ENDPOINT; i++)  {
     		priv->inEndPoint = i;
            priv->pEndpointInBuffer[i] = (UInt8 *)calloc(1,512);
        }
    //    LeafPrivateData *priv = (LeafPrivateData *)self->privateData;
        
    //    priv->cmdBufferRef = LeafCreateCommandBuffer(1000);
    //    if ( priv->cmdBufferRef == NULL ) {
    //        free(priv);
    //        return canERR_NOMEM;
    //    }
        
    //    priv->semaTimeout = dispatch_semaphore_create(0);
        
    } else {
        return(canERR_NOMEM);
    }
    // Set some device Infos
    self->deviceChannelCount = 0;
    
    sprintf((char*)self->devInfo.deviceString, "%s",pDeviceString);

    self->devInfo.capability = 0u;
    self->devInfo.capability |= canCHANNEL_CAP_CAN_FD;

    usbFdSetPowerMode(self, 0);
    usbFdGetDeviceCaps(self);


    // Trigger the read
    usbFdReadFromBulkInPipe(self);
    
    return(canOK);
}

static CanHandle usbFdCanOpenChannel(
        int channel,
        int flags
    )
{
Can4osxUsbDeviceHandleEntry *pSelf = &can4osxUsbDeviceHandle[channel];
usbFdPrivateData_t *pPriv = (usbFdPrivateData_t *)pSelf->privateData;
    
    // set CAN Mode
    if ((flags & canOPEN_CAN_FD) ==  canOPEN_CAN_FD) {
        pPriv->canFd = 1;
    } else {
        pPriv->canFd = 0;
    }

    return (CanHandle)channel;
}

/******************************************************************************/
// Translate from baud macro to bus params
/******************************************************************************/
static canStatus usbFdCanTranslateBaud (
        SInt32 *const freq,
        unsigned int *const tseg1,
        unsigned int *const tseg2,
        unsigned int *const sjw,
        unsigned int *const nosamp,
        unsigned int *const syncMode
    )
{
    switch (*freq) {
    
        case canFD_BITRATE_8M_60P:
            *freq     = 8000000L;
            *tseg1    = 2;
            *tseg2    = 2;
            *sjw      = 1;
            break;

        case canFD_BITRATE_4M_80P:
            *freq     = 4000000L;
            *tseg1    = 7;
            *tseg2    = 2;
            *sjw      = 2;
            break;

        case canFD_BITRATE_2M_80P:
            *freq     = 2000000L;
            *tseg1    = 29;
            *tseg2    = 10;
            *sjw      = 10;
            break;

        case canFD_BITRATE_1M_80P:
            *freq     = 1000000L;
            *tseg1    = 63;
            *tseg2    = 16;
            *sjw      = 16;
            break;

        case canFD_BITRATE_500K_80P:
            *freq     = 500000L;
            *tseg1    = 127;
            *tseg2    = 32;
            *sjw      = 32;
            break;

        case canBITRATE_1M:
            *freq     = 1000000L;
            *tseg1    = 6;
            *tseg2    = 1;
            *sjw      = 1;
            *nosamp   = 1;
            *syncMode = 0;
            break;
            
        case canBITRATE_500K:
            *freq     = 500000L;
            *tseg1    = 12;//6;
            *tseg2    = 3;//1;
            *sjw      = 1;
            *nosamp   = 1;
            *syncMode = 0;
            break;
            
        case canBITRATE_250K:
            *freq     = 250000L;
            *tseg1    = 6;
            *tseg2    = 1;
            *sjw      = 1;
            *nosamp   = 1;
            *syncMode = 0;
            break;
            
        case canBITRATE_125K:
            *freq     = 125000L;
            *tseg1    = 13;
            *tseg2    = 2;
            *sjw      = 1;
            *nosamp   = 1;
            *syncMode = 0;
            break;
            
        case canBITRATE_100K:
            *freq     = 100000L;
            *tseg1    = 13;
            *tseg2    = 2;
            *sjw      = 1;
            *nosamp   = 1;
            *syncMode = 0;
            break;
            
        case canBITRATE_83K:
            *freq     = 83333L;
            *tseg1    = 5;
            *tseg2    = 2;
            *sjw      = 2;
            *nosamp   = 1;
            *syncMode = 0;
            break;
            
        case canBITRATE_62K:
            *freq     = 62500L;
            *tseg1    = 10;
            *tseg2    = 5;
            *sjw      = 1;
            *nosamp   = 1;
            *syncMode = 0;
            break;
            
        case canBITRATE_50K:
            *freq     = 50000L;
            *tseg1    = 10;
            *tseg2    = 5;
            *sjw      = 1;
            *nosamp   = 1;
            *syncMode = 0;
            break;
            
        default:
            return(canERR_PARAM);
    }
    
    return(canOK);
}



static canStatus usbFdSetPowerMode(Can4osxUsbDeviceHandleEntry *pSelf, UInt8 mode)
{
UInt8 data[IXXUSBFD_CMD_BUFFER_SIZE] = {0};
ixxUsbFdDevPowerReq_t *pPowerReq;
ixxUsbFdDevPowerResp_t *pPowerResp;

	pPowerReq = (ixxUsbFdDevPowerReq_t *)data;
    pPowerResp = (ixxUsbFdDevPowerResp_t *)(data + sizeof(ixxUsbFdDevPowerReq_t));
    
    pPowerReq->header.reqSize = sizeof(ixxUsbFdDevPowerReq_t);
    pPowerReq->header.reqCode = 0x421;
    pPowerReq->header.reqSocket = 0xffff;
    pPowerReq->header.reqPort = 0xffff;
    pPowerReq->mode = mode;
    
    pPowerResp->header.respSize = sizeof(ixxUsbFdDevPowerResp_t);
    pPowerResp->header.retSize = 0u;
    pPowerResp->header.retCode = 0xffFFffFF;
    
    usbFdSendCmd(pSelf, (IXXUSBFDMSGREQHEAD_T *)pPowerReq);
    
    usbFdRecvCmd(pSelf, (ixxUsbFdMsgRespHead_t *)pPowerResp, 0xffff);
    
	if (pPowerResp->header.retCode != 0u)  {
		return(canERR_PARAM);
    }

	return(canOK);
}

static canStatus usbFdGetDeviceCaps(Can4osxUsbDeviceHandleEntry *pSelf)
{
UInt8 data[IXXUSBFD_CMD_BUFFER_SIZE] = {0};
IXXUSBFDDEVICECAPSREQ_T *pCapsReq;
IXXUSBFDDEVICECAPSRESP_T *pCapsResp;
int i;

	pCapsReq = (IXXUSBFDDEVICECAPSREQ_T *)data;
	pCapsResp = (IXXUSBFDDEVICECAPSRESP_T *)(data + sizeof(IXXUSBFDDEVICECAPSREQ_T));
	
	pCapsReq->header.reqSize = sizeof(IXXUSBFDDEVICECAPSREQ_T);
	pCapsReq->header.reqCode = 0x401;
	pCapsReq->header.reqPort = 0xffff;
	pCapsReq->header.reqSocket = 0xffff;
	
	pCapsResp->header.respSize = sizeof(IXXUSBFDDEVICECAPSRESP_T);
	pCapsResp->header.retSize = 0u;
	pCapsResp->header.retCode = 0xffFFffFF;
	
	usbFdSendCmd(pSelf, (IXXUSBFDMSGREQHEAD_T *)pCapsReq);
    usbFdRecvCmd(pSelf, (ixxUsbFdMsgRespHead_t *)pCapsResp, 0xffff);
    
    if (pCapsResp->header.retCode != 0u)  {
        return(canERR_PARAM);
    }
    
    for (i = 0; i < pCapsResp->caps.chanCount; i++)  {
    	if ((pCapsResp->caps.chanTypes[i] & 0x100) == 0x100)  {
     		pSelf->deviceChannelCount++;
        }
    }
    

	return(canOK);
}


static canStatus usbFdSendCmd(Can4osxUsbDeviceHandleEntry *pSelf, IXXUSBFDMSGREQHEAD_T *pCmd)
{
IOUSBDevRequest request;
IOReturn retVal;

	request.bmRequestType = USBmakebmRequestType(kUSBOut, kUSBVendor, kUSBDevice);
	request.bRequest = 0xff;
	request.wValue = pCmd->reqPort;
    request.wIndex = 0u;
	request.wLength = pCmd->reqSize + sizeof(ixxUsbFdMsgRespHead_t);
	request.pData = pCmd;

	for (int i = 0; i < 10; i++)  {
		retVal = (*(pSelf->can4osxDeviceInterface))->DeviceRequest(pSelf->can4osxDeviceInterface, &request);
		if (retVal == kIOReturnSuccess)  {
  			return(canOK);
        }
	}
	return(canERR_PARAM);
}


static canStatus usbFdRecvCmd(Can4osxUsbDeviceHandleEntry *pSelf, ixxUsbFdMsgRespHead_t *pCmd, int value)
{
IOUSBDevRequest request;
IOReturn retVal;
UInt16 sizeToRead = pCmd->respSize;

    request.bmRequestType = USBmakebmRequestType(kUSBIn, kUSBVendor, kUSBDevice);
    request.bRequest = 0xff;
    request.wLength = pCmd->respSize;
    request.wValue = value;
    request.wIndex = 0u;
    request.pData = pCmd;
    
    for (int i = 0; i < 10; i++)  {
        retVal = (*(pSelf->can4osxDeviceInterface))->DeviceRequest(pSelf->can4osxDeviceInterface, &request);
        if (retVal == kIOReturnSuccess)  {
        	if (sizeToRead <= pCmd->retSize)  {
              	return(canOK);
            }
        }
    }

	return(canERR_PARAM);
}


static void usbFdBulkReadCompletion(void *refCon, IOReturn result, void *arg0)
{
    Can4osxUsbDeviceHandleEntry *pSelf = (Can4osxUsbDeviceHandleEntry *)refCon;
    IOUSBInterfaceInterface **interface = pSelf->can4osxInterfaceInterface;
    UInt32 numBytesRead = (UInt32) arg0;
    (void)numBytesRead;
    
    CAN4OSX_DEBUG_PRINT("Asynchronous bulk read complete (%ld)\n", (long)numBytesRead);
    
    if (result != kIOReturnSuccess) {
        printf("Error from async bulk read (%08x)\n", result);
        (void) (*interface)->USBInterfaceClose(interface);
        (void) (*interface)->Release(interface);
        return;
    }

    usbFdReadFromBulkInPipe(pSelf);
}



static void usbFdReadFromBulkInPipe(Can4osxUsbDeviceHandleEntry *self)
{
    IOReturn ret = (*(self->can4osxInterfaceInterface))->ReadPipeAsync(self->can4osxInterfaceInterface, self->endpointNumberBulkIn++, ((usbFdPrivateData_t*)(self->privateData))->pEndpointInBuffer[0], self->endpointMaxSizeBulkIn, usbFdBulkReadCompletion, (void*)self);
    
     self->endpointNumberBulkIn++;
     ret = (*(self->can4osxInterfaceInterface))->ReadPipeAsync(self->can4osxInterfaceInterface, self->endpointNumberBulkIn++, ((usbFdPrivateData_t*)(self->privateData))->pEndpointInBuffer[1], self->endpointMaxSizeBulkIn, usbFdBulkReadCompletion, (void*)self);
     self->endpointNumberBulkIn++;
     ret = (*(self->can4osxInterfaceInterface))->ReadPipeAsync(self->can4osxInterfaceInterface, self->endpointNumberBulkIn++, ((usbFdPrivateData_t*)(self->privateData))->pEndpointInBuffer[2], self->endpointMaxSizeBulkIn, usbFdBulkReadCompletion, (void*)self);
     self->endpointNumberBulkIn++;
     ret = (*(self->can4osxInterfaceInterface))->ReadPipeAsync(self->can4osxInterfaceInterface, self->endpointNumberBulkIn++, ((usbFdPrivateData_t*)(self->privateData))->pEndpointInBuffer[3], self->endpointMaxSizeBulkIn, usbFdBulkReadCompletion, (void*)self);




    
    if (ret != kIOReturnSuccess) {
        CAN4OSX_DEBUG_PRINT("Unable to read async interface (%08x)\n", ret);
    }
}



