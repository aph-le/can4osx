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


static char* pDeviceString = "IXXAT USB-to-CAN FD";

static canStatus usbFdInitHardware(const CanHandle hnd);
static CanHandle usbFdCanOpenChannel(int channel, int flags);
static canStatus usbFdCanStartChip(CanHandle hdl);
static canStatus usbFdCanStopChip(CanHandle hnl);

static canStatus usbFdCanSetBusParams (
        const CanHandle hnd,
        SInt32 freq,
        unsigned int tseg1,
        unsigned int tseg2,
        unsigned int sjw,
        unsigned int noSamp,
        unsigned int syncmode
    );

static canStatus usbFdCanRead (
        const   CanHandle hnd,
        UInt32  *id,
        void    *msg,
        UInt16  *dlc,
        UInt32  *flag,
        UInt32  *time
        );

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
static canStatus usbFdSetBitrates(Can4osxUsbDeviceHandleEntry *pSelf);

static canStatus usbFdSendCmd(Can4osxUsbDeviceHandleEntry *pSelf, IXXUSBFDMSGREQHEAD_T *pCmd);
static canStatus usbFdRecvCmd(Can4osxUsbDeviceHandleEntry *pSelf, IXXUSBFDMSGRESPHEAD_T *pCmd, int value);

static void usbFdBulkReadCompletion(void *refCon, IOReturn result, void *arg0);
static void usbFdReadFromBulkInPipe(Can4osxUsbDeviceHandleEntry *self);



Can4osxHwFunctions ixxUsbFdHardwareFunctions = {
    .can4osxhwInitRef = usbFdInitHardware,
    .can4osxhwCanOpenChannel = usbFdCanOpenChannel,
    .can4osxhwCanSetBusParamsRef = usbFdCanSetBusParams,
    .can4osxhwCanSetBusParamsFdRef = NULL,
    .can4osxhwCanBusOnRef = usbFdCanStartChip,
    .can4osxhwCanBusOffRef = usbFdCanStopChip,
    .can4osxhwCanWriteRef = NULL,
    .can4osxhwCanReadRef = usbFdCanRead,
    .can4osxhwCanCloseRef = NULL,
};


static canStatus usbFdInitHardware(
		const CanHandle hnd
    )
{
    Can4osxUsbDeviceHandleEntry *pSelf = &can4osxUsbDeviceHandle[hnd];
    pSelf->privateData = calloc(1,sizeof(usbFdPrivateData_t));
    
    if ( pSelf->privateData != NULL ) {
    	usbFdPrivateData_t *priv = (usbFdPrivateData_t *)pSelf->privateData;
     	priv->pParent = pSelf;
    
    	for (UInt8 i = 0u; i < IXXUSBFD_MAX_RX_ENDPOINT; i++)  {
     		priv->inEndPoint = i;
            priv->pEndpointInBuffer[i] = (UInt8 *)calloc(1,512);
        }
    } else {
        return(canERR_NOMEM);
    }
    // Set some device Infos
    pSelf->deviceChannelCount = 0;
    
    sprintf((char*)pSelf->devInfo.deviceString, "%s",pDeviceString);

    pSelf->devInfo.capability = 0u;
    pSelf->devInfo.capability |= canCHANNEL_CAP_CAN_FD;

    usbFdSetPowerMode(pSelf, 0);
    usbFdGetDeviceCaps(pSelf);

    // Trigger the read
    usbFdReadFromBulkInPipe(pSelf);
    
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

//Set bit timing
static canStatus usbFdCanSetBusParams (
        const CanHandle hnd,
        SInt32 freq,
        unsigned int tseg1,
        unsigned int tseg2,
        unsigned int sjw,
        unsigned int noSamp,
        unsigned int syncmode
    )
{
Can4osxUsbDeviceHandleEntry *pSelf = &can4osxUsbDeviceHandle[hnd];
usbFdPrivateData_t *pPriv = (usbFdPrivateData_t *)pSelf->privateData;
    
    CAN4OSX_DEBUG_PRINT("leaf pro: _set_busparam\n");
    
    if ( canOK != usbFdCanTranslateBaud(&freq, &tseg1, &tseg2, &sjw,
                                          &noSamp, &syncmode)) {
        // TODO
        CAN4OSX_DEBUG_PRINT(" can4osx strange bitrate\n");
        return(-1);
    }
    
    /* save locally */
    pPriv->brp = freq;
    pPriv->sjw = sjw;
    pPriv->tseg1 = tseg1;
    pPriv->tseg2 = tseg2;

	usbFdSetBitrates(pSelf);

    return(canOK);
}


//Go bus on
static canStatus usbFdCanStartChip(
        CanHandle hdl
    )
{
Can4osxUsbDeviceHandleEntry *pSelf = &can4osxUsbDeviceHandle[hdl];
UInt8 data[IXXUSBFD_CMD_BUFFER_SIZE] = {0};
IXXUSBFDCANSTARTREQ_T *pReq;
IXXUSBFDCANSTARTRESP_T *pResp;
    
    pReq = (IXXUSBFDCANSTARTREQ_T *)data;
    pResp = (IXXUSBFDCANSTARTRESP_T *)(data + sizeof(IXXUSBFDCANSTARTREQ_T));
    
    pReq->header.reqSize = sizeof(IXXUSBFDCANSTARTREQ_T);
    pReq->header.reqCode = 0x326;
    pReq->header.reqPort = 0u;//FIXME
    pReq->header.reqSocket = 0xffff;
    
    pResp->header.respSize = sizeof(IXXUSBFDCANSTARTRESP_T);
    pResp->header.retSize = 0u;
    pResp->header.retCode = 0xffFFffFF;
    pResp->startTime = 0u;
    
    usbFdSendCmd(pSelf, (IXXUSBFDMSGREQHEAD_T *)pReq);
    usbFdRecvCmd(pSelf, (IXXUSBFDMSGRESPHEAD_T *)pResp, 0 /* FIXME */);
    
    if (pResp->header.retCode != 0u)  {
    	return(canERR_INTERNAL);
    }

	return(canOK);
}


static canStatus usbFdCanStopChip(
        CanHandle hdl
    )
{
Can4osxUsbDeviceHandleEntry *pSelf = &can4osxUsbDeviceHandle[hdl];
UInt8 data[IXXUSBFD_CMD_BUFFER_SIZE] = {0};
IXXUSBFDCANSTOPREQ_T *pReq;
IXXUSBFDCANSTOPRESP_T *pResp;

    pReq = (IXXUSBFDCANSTOPREQ_T *)data;
    pResp = (IXXUSBFDCANSTOPRESP_T *)(data + sizeof(IXXUSBFDCANSTOPREQ_T));
 
    pReq->header.reqSize = sizeof(IXXUSBFDCANSTOPREQ_T);
    pReq->header.reqCode = 0x327;
    pReq->header.reqPort = 0u;//FIXME
    pReq->header.reqSocket = 0xffff;
    pReq->action = 0x3;
    
    pResp->header.respSize = sizeof(IXXUSBFDCANSTOPRESP_T);
    pResp->header.retSize = 0u;
    pResp->header.retCode = 0xffFFffFF;
    
    usbFdSendCmd(pSelf, (IXXUSBFDMSGREQHEAD_T *)pReq);
    usbFdRecvCmd(pSelf, (IXXUSBFDMSGRESPHEAD_T *)pResp, 0 /* FIXME */);

	return(canOK);
}

/******************************************************************************/
static canStatus usbFdCanRead (
        const   CanHandle hnd,
        UInt32  *id,
        void    *msg,
        UInt16  *dlc,
        UInt32  *flag,
        UInt32  *time
    )
{
Can4osxUsbDeviceHandleEntry *pSelf = &can4osxUsbDeviceHandle[hnd];
    
    if ( pSelf->privateData != NULL ) {
        
        CanMsg canMsg;
        
        if ( CAN4OSX_ReadCanEventBuffer(pSelf->canEventMsgBuff, &canMsg) ) {
            
            *id = canMsg.canId;
            *dlc = canMsg.canDlc;
            *time =canMsg.canTimestamp;
            
            memcpy(msg, canMsg.canData, *dlc);
            
            *flag = canMsg.canFlags;
            
            return canOK;
        } else {
            return canERR_NOMSG;
        }
    } else {
        return canERR_INTERNAL;
    }
    
}


/******************************************************************************/
// Translate from baud macro to bus params
/******************************************************************************/
static canStatus usbFdCanTranslateBaud (
        SInt32 *const pPrescaler,
        unsigned int *const tseg1,
        unsigned int *const tseg2,
        unsigned int *const sjw,
        unsigned int *const nosamp,
        unsigned int *const syncMode
    )
{
    switch (*pPrescaler) {
    
        case canFD_BITRATE_8M_60P:
            *pPrescaler     = 8000000L;
            *tseg1    = 2;
            *tseg2    = 2;
            *sjw      = 1;
            break;

        case canFD_BITRATE_4M_80P:
            *pPrescaler     = 4000000L;
            *tseg1    = 7;
            *tseg2    = 2;
            *sjw      = 2;
            break;

        case canFD_BITRATE_2M_80P:
            *pPrescaler     = 2000000L;
            *tseg1    = 29;
            *tseg2    = 10;
            *sjw      = 10;
            break;

        case canFD_BITRATE_1M_80P:
            *pPrescaler     = 1000000L;
            *tseg1    = 63;
            *tseg2    = 16;
            *sjw      = 16;
            break;

        case canFD_BITRATE_500K_80P:
            *pPrescaler = 16;
            *tseg1    = 15;
            *tseg2    = 4;
            *sjw      = 1;
            break;

        case canBITRATE_1M:
            *pPrescaler = 5;
            *tseg1    = 13;
            *tseg2    = 2;
            *sjw      = 1;
            *nosamp   = 1;
            *syncMode = 0;
            break;
            
        case canBITRATE_500K:
            *pPrescaler = 10;
            *tseg1    = 13;
            *tseg2    = 2;
            *sjw      = 1;
            *nosamp   = 1;
            *syncMode = 0;
            break;
            
        case canBITRATE_250K:
            *pPrescaler = 20;
            *tseg1    = 13;
            *tseg2    = 2;
            *sjw      = 1;
            *nosamp   = 1;
            *syncMode = 0;
            break;
            
        case canBITRATE_125K:
            *pPrescaler = 40;
            *tseg1    = 13;
            *tseg2    = 2;
            *sjw      = 1;
            *nosamp   = 1;
            *syncMode = 0;
            break;
            
        case canBITRATE_100K:
            *pPrescaler     = 100000L;
            *tseg1    = 13;
            *tseg2    = 2;
            *sjw      = 1;
            *nosamp   = 1;
            *syncMode = 0;
            break;
            
        case canBITRATE_83K:
            *pPrescaler     = 83333L;
            *tseg1    = 5;
            *tseg2    = 2;
            *sjw      = 2;
            *nosamp   = 1;
            *syncMode = 0;
            break;
            
        case canBITRATE_62K:
            *pPrescaler     = 62500L;
            *tseg1    = 10;
            *tseg2    = 5;
            *sjw      = 1;
            *nosamp   = 1;
            *syncMode = 0;
            break;
            
        case canBITRATE_50K:
            *pPrescaler     = 50000L;
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


static canStatus usbFdSetPowerMode(
		Can4osxUsbDeviceHandleEntry *pSelf, /**< pointer to handle structure */
        UInt8 mode
    )
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
    
    usbFdRecvCmd(pSelf, (IXXUSBFDMSGRESPHEAD_T *)pPowerResp, 0xffff);
    
	if (pPowerResp->header.retCode != 0u)  {
		return(canERR_PARAM);
    }

	return(canOK);
}


static canStatus usbFdGetDeviceCaps(
		Can4osxUsbDeviceHandleEntry *pSelf /**< pointer to handle structure */
    )
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
    usbFdRecvCmd(pSelf, (IXXUSBFDMSGRESPHEAD_T *)pCapsResp, 0xffff);
    
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


static canStatus usbFdSetBitrates(
		Can4osxUsbDeviceHandleEntry *pSelf /**< pointer to handle structure */
    )
{
UInt8 data[IXXUSBFD_CMD_BUFFER_SIZE] = {0};
usbFdPrivateData_t *pPriv = (usbFdPrivateData_t *)pSelf->privateData;
IXXUSBFDCANINITREQ_T *pReq;
IXXUSBFDCANINITRESP_T *pResp;

	if (pPriv == NULL)  {
		return(canERR_INTERNAL);
    }
    
    pReq = (IXXUSBFDCANINITREQ_T *)data;
    pResp = (IXXUSBFDCANINITRESP_T *)(data + sizeof(IXXUSBFDCANINITREQ_T));
    
    pReq->header.reqSize = sizeof(IXXUSBFDCANINITREQ_T);
    pReq->header.reqCode = 0x337;
    pReq->header.reqSocket = 0xffff;
    pReq->header.reqPort = 0;  //FIXME

    pReq->exMode = 0u;
    pReq->opMode = (IXXATUSBFD_OPMODE_EXTENDED | IXXATUSBFD_OPMODE_STANDARD);
    if (pPriv->canFd)  {
    	pReq->exMode = (IXXATUSBFD_EXMODE_EXTDATA | IXXATUSBFD_EXMODE_ISOFD | IXXATUSBFD_EXMODE_FASTDATA);
    }
    pReq->stdBitrate.mode = 1;
    pReq->stdBitrate.bps = pPriv->brp;//1u;
    pReq->stdBitrate.tseg1 = pPriv->tseg1;
    pReq->stdBitrate.tseg2 = pPriv->tseg2;
    pReq->stdBitrate.sjw = pPriv->sjw;
    pReq->stdBitrate.tdo = 0u;
    if (pPriv->canFd)  {
	    pReq->fdBitrate.bps = 1u;
    	pReq->fdBitrate.tseg1 = pPriv->fd_tseg1;
    	pReq->fdBitrate.tseg2 = pPriv->fd_tseg2;
    	pReq->fdBitrate.sjw = pPriv->fd_sjw;
    	pReq->fdBitrate.tdo = 1u + pPriv->fd_tseg1;
    }
    
    pResp->header.respSize = sizeof(IXXUSBFDCANINITRESP_T);
    pResp->header.retSize = 0u;
    pResp->header.retCode = 0xffFFffFF;

    usbFdSendCmd(pSelf, (IXXUSBFDMSGREQHEAD_T *)pReq);
    usbFdRecvCmd(pSelf, (IXXUSBFDMSGRESPHEAD_T *)pResp, 0 /* FIXME */);

	return(canOK);
}


static canStatus usbFdSendCmd(
		Can4osxUsbDeviceHandleEntry *pSelf, /**< pointer to handle structure */
        IXXUSBFDMSGREQHEAD_T *pCmd
    )
{
IOUSBDevRequest request;
IOReturn retVal;

	request.bmRequestType = USBmakebmRequestType(kUSBOut, kUSBVendor, kUSBDevice);
	request.bRequest = 0xff;
	request.wValue = pCmd->reqPort;
    request.wIndex = 0u;
	request.wLength = pCmd->reqSize + sizeof(IXXUSBFDMSGRESPHEAD_T);
	request.pData = pCmd;

	for (int i = 0; i < 10; i++)  {
		retVal = (*(pSelf->can4osxDeviceInterface))->DeviceRequest(pSelf->can4osxDeviceInterface, &request);
		if (retVal == kIOReturnSuccess)  {
  			return(canOK);
        }
	}
	return(canERR_PARAM);
}


static canStatus usbFdRecvCmd(
		Can4osxUsbDeviceHandleEntry *pSelf, /**< pointer to handle structure */
        IXXUSBFDMSGRESPHEAD_T *pCmd,
        int value
    )
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


static void usbFdDeocdeMsg(Can4osxUsbDeviceHandleEntry *pSelf, IXXUSBFDCANMSG_T* pMsg)
{
CanMsg canMsg;

	switch (pMsg->flags & IXXUSBFD_MSG_FLAG_TYPE)  {
    case 0x00:
    	canMsg.canId = pMsg->canId;
    	canMsg.canDlc = (pMsg->flags & IXXUSBFD_MSG_FLAG_DLC ) >> 16;
     
        /* decode dlc to length */
    	canMsg.canDlc = CAN4OSX_decodeFdDlc(canMsg.canDlc);
     
     	canMsg.canFlags = 0u;
     	if (pMsg->flags & IXXUSBFD_MSG_FLAG_EDL)  {
      		canMsg.canFlags |= canFDMSG_FDF;
        } else {
        	if (canMsg.canDlc > 8u)  {
         		canMsg.canDlc = 8u;
            }
        }
        if (pMsg->flags & IXXUSBFD_MSG_FLAG_FDR)  {
            canMsg.canFlags |= canFDMSG_BRS;
        }
        if (pMsg->flags & IXXUSBFD_MSG_FLAG_EXT)  {
            canMsg.canFlags |= canMSG_EXT;
        } else {
            canMsg.canFlags |= canMSG_STD;
        }
        if (pMsg->flags & IXXUSBFD_MSG_FLAG_RTR)  {
            canMsg.canFlags |= canMSG_RTR;
        } else {
        	memcpy(canMsg.canData, pMsg->data, canMsg.canDlc);
        }
        
        canMsg.canTimestamp = pMsg->time;
      
        CAN4OSX_WriteCanEventBuffer(pSelf->canEventMsgBuff,canMsg);
        
        if (pSelf->canNotification.notifacionCenter) {
                CFNotificationCenterPostNotification (pSelf->canNotification.notifacionCenter,
                    pSelf->canNotification.notificationString, NULL, NULL, true);
        }
     
     	break;
    default:
    	break;
    }

}


static void usbFdBulkReadCompletion(void *refCon, IOReturn result, void *arg0)
{
    Can4osxUsbDeviceHandleEntry *pSelf = (Can4osxUsbDeviceHandleEntry *)refCon;
    IOUSBInterfaceInterface **interface = pSelf->can4osxInterfaceInterface;
    UInt32 numBytesRead = (UInt32) arg0;
    UInt32 count = 0u;
    IXXUSBFDCANMSG_T *pMsg;
    UInt8 data[512];
    memcpy(data, pSelf->endpointBufferBulkInRef, 512);
    
    CAN4OSX_DEBUG_PRINT("Asynchronous bulk read complete (%ld)\n", (long)numBytesRead);
    
    if (result != kIOReturnSuccess) {
        printf("Error from async bulk read (%08x)\n", result);
        (void) (*interface)->USBInterfaceClose(interface);
        (void) (*interface)->Release(interface);
        return;
    }
    
    while (count < numBytesRead)  {
    	pMsg = (IXXUSBFDCANMSG_T *)&(pSelf->endpointBufferBulkInRef[count]);
     	usbFdDeocdeMsg(pSelf, pMsg);
    	count += pMsg->size;
     	count++;
    }

    usbFdReadFromBulkInPipe(pSelf);
}



static void usbFdReadFromBulkInPipe(
		Can4osxUsbDeviceHandleEntry *pSelf /**< pointer to handle structure */
    )
{
IOReturn ret = (*(pSelf->can4osxInterfaceInterface))->ReadPipeAsync(pSelf->can4osxInterfaceInterface, pSelf->endpointNumberBulkIn + 2, pSelf->endpointBufferBulkInRef, pSelf->endpointMaxSizeBulkIn, usbFdBulkReadCompletion, (void*)pSelf);

#if 0
    IOReturn ret = (*(self->can4osxInterfaceInterface))->ReadPipeAsync(self->can4osxInterfaceInterface, self->endpointNumberBulkIn, ((usbFdPrivateData_t*)(self->privateData))->pEndpointInBuffer[0], self->endpointMaxSizeBulkIn, usbFdBulkReadCompletion, (void*)self);
    (*(self->can4osxInterfaceInterface))->ReadPipeAsync(self->can4osxInterfaceInterface, self->endpointNumberBulkIn + 2, ((usbFdPrivateData_t*)(self->privateData))->pEndpointInBuffer[0], self->endpointMaxSizeBulkIn, usbFdBulkReadCompletion, (void*)self);
    (*(self->can4osxInterfaceInterface))->ReadPipeAsync(self->can4osxInterfaceInterface, self->endpointNumberBulkIn + 4, ((usbFdPrivateData_t*)(self->privateData))->pEndpointInBuffer[0], self->endpointMaxSizeBulkIn, usbFdBulkReadCompletion, (void*)self);
    (*(self->can4osxInterfaceInterface))->ReadPipeAsync(self->can4osxInterfaceInterface, self->endpointNumberBulkIn + 8, ((usbFdPrivateData_t*)(self->privateData))->pEndpointInBuffer[0], self->endpointMaxSizeBulkIn, usbFdBulkReadCompletion, (void*)self);
#endif
    if (ret != kIOReturnSuccess) {
        CAN4OSX_DEBUG_PRINT("Unable to read async interface (%08x)\n", ret);
    }
}



