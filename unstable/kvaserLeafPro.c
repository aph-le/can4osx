//
//  kvaserLeafPro.c
//
//
// Copyright (c) 2014 - 2016 Alexander Philipp. All rights reserved.
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

static char* pDeviceString = "Kvaser Leaf Pro v2";

static void LeafProDecodeCommand(Can4osxUsbDeviceHandleEntry *pSelf,
                                 proCommand_t *pCmd);

static void LeafProMapChannels(Can4osxUsbDeviceHandleEntry *pSelf);

static canStatus LeafProCanSetBusParams ( const CanHandle hnd, SInt32 freq,
                                         unsigned int tseg1,
                                         unsigned int tseg2,
                                         unsigned int sjw,
                                         unsigned int noSamp,
                                         unsigned int syncmode );

static canStatus LeafProCanRead (const CanHandle hnd,
                                 UInt32 *id,
                                 void *msg,
                                 UInt16 *dlc,
                                 UInt16 *flag,
                                 UInt32 *time);


static canStatus LeafProCanTranslateBaud (SInt32 *const freq,
                                          unsigned int *const tseg1,
                                          unsigned int *const tseg2,
                                          unsigned int *const sjw,
                                          unsigned int *const nosamp,
                                          unsigned int *const syncMode);

static IOReturn LeafProWriteCommandWait(Can4osxUsbDeviceHandleEntry *pSelf,
                                        proCommand_t cmd,
                                        UInt8 reason);

static LeafProCommandMsgBuf_t* LeafProCreateCommandBuffer(UInt32 bufferSize);
static void LeafProReleaseCommandBuffer(LeafProCommandMsgBuf_t* pBufferRef);


static void LeafProReadFromBulkInPipe(Can4osxUsbDeviceHandleEntry *self);
static void LeafProBulkReadCompletion(void *refCon,
                                      IOReturn result,
                                      void *arg0);


//Hardware interface function
static canStatus LeafProInitHardware(const CanHandle hnd);
static CanHandle LeafProCanOpenChannel(int channel, int flags);
static canStatus LeafProCanStartChip(CanHandle hdl);

Can4osxHwFunctions leafProHardwareFunctions = {
    .can4osxhwInitRef = LeafProInitHardware,
    .can4osxhwCanOpenChannel = LeafProCanOpenChannel,
    .can4osxhwCanSetBusParamsRef = LeafProCanSetBusParams,
    .can4osxhwCanBusOnRef = LeafProCanStartChip,
    .can4osxhwCanBusOffRef = NULL,
    .can4osxhwCanWriteRef = NULL,
    .can4osxhwCanReadRef = LeafProCanRead,
    .can4osxhwCanCloseRef = NULL,
};


static canStatus LeafProInitHardware(const CanHandle hnd)
{
    Can4osxUsbDeviceHandleEntry *pSelf = &can4osxUsbDeviceHandle[hnd];
    pSelf->privateData = calloc(1,sizeof(LeafProPrivateData));

    if ( pSelf->privateData != NULL ) {
        LeafProPrivateData *pPriv = (LeafProPrivateData *)pSelf->privateData;
        
        pPriv->cmdBufferRef = LeafProCreateCommandBuffer(1000);
        if ( pPriv->cmdBufferRef == NULL ) {
            free(pPriv);
            return canERR_NOMEM;
        }

        pPriv->semaTimeout = dispatch_semaphore_create(0);
        
    } else {
        return canERR_NOMEM;
    }

    
    // Set some device Infos
    sprintf((char*)pSelf->devInfo.deviceString, "%s",pDeviceString);
    
    // Trigger next read
    LeafProReadFromBulkInPipe(pSelf);
    
    
    return canOK;
}


static CanHandle LeafProCanOpenChannel(int channel, int flags)
{
    Can4osxUsbDeviceHandleEntry *pSelf = &can4osxUsbDeviceHandle[channel];

    // Set up channels
    LeafProMapChannels(pSelf);

    return (CanHandle)channel;
}


//Set bit timing
static canStatus LeafProCanSetBusParams (const CanHandle hnd,
                                         SInt32 freq,
                                         unsigned int tseg1,
                                         unsigned int tseg2,
                                         unsigned int sjw,
                                         unsigned int noSamp,
                                         unsigned int syncmode
                                         )
{
    proCommand_t        cmd;
    UInt32         tmp, PScl;
    int            retVal;
    Can4osxUsbDeviceHandleEntry *pSelf = &can4osxUsbDeviceHandle[hnd];
    LeafProPrivateData *pPriv = (LeafProPrivateData *)pSelf->privateData;
    
    CAN4OSX_DEBUG_PRINT("leaf pro: _set_busparam\n");
    
    if ( canOK != LeafProCanTranslateBaud( &freq, &tseg1, &tseg2, &sjw, &noSamp, &syncmode)) {
        // TODO
        CAN4OSX_DEBUG_PRINT(" can4osx strange bitrate\n");
        return -1;
    }
    
    
    // Check bus parameters
    tmp = freq * (tseg1 + tseg2 + 1);
    if (tmp == 0) {
        CAN4OSX_DEBUG_PRINT("leaf: _set_busparams() tmp == 0!\n");
        return VCAN_STAT_BAD_PARAMETER;
    }
    
    PScl = 16000000UL / tmp;
    
    if (PScl <= 1 || PScl > 256) {
        CAN4OSX_DEBUG_PRINT("hwif_set_chip_param() prescaler wrong (%d)\n",
                PScl & 1 /* even */);
        return VCAN_STAT_BAD_PARAMETER;
    }
    memset(&cmd, 0 , sizeof(cmd));
    
    cmd.proCmdSetBusparamsReq.header.cmdNo = LEAFPRO_CMD_SET_BUSPARAMS_REQ;
    cmd.proCmdSetBusparamsReq.header.address = pPriv->address;
    cmd.proCmdSetBusparamsReq.header.transitionId = 1;
    
    cmd.proCmdSetBusparamsReq.bitRate = freq;
    cmd.proCmdSetBusparamsReq.sjw     = (UInt8)sjw;
    cmd.proCmdSetBusparamsReq.tseg1   = (UInt8)tseg1;
    cmd.proCmdSetBusparamsReq.tseg2   = (UInt8)tseg2;
    cmd.proCmdSetBusparamsReq.channel = (UInt8)0;//vChan->channel;
    cmd.proCmdSetBusparamsReq.noSamp  = 1;
    
    
    retVal = LeafProWriteCommandWait( pSelf, cmd,
                LEAFPRO_CMD_SET_BUSPARAMS_RESP);
    
    
    return retVal;
}

//Go bus on
static canStatus LeafProCanStartChip(CanHandle hdl)
{
    int retVal = 0;
    proCommand_t        cmd;
    Can4osxUsbDeviceHandleEntry *pSelf = &can4osxUsbDeviceHandle[hdl];
    LeafProPrivateData *pPriv = (LeafProPrivateData *)pSelf->privateData;
    
    CAN4OSX_DEBUG_PRINT("CAN BusOn Command %d\n", hdl);
    
    cmd.proCmdHead.cmdNo = LEAFPRO_CMD_START_CHIP_REQ;
    cmd.proCmdHead.address = pPriv->address;
    cmd.proCmdHead.transitionId = 1u;
    retVal = LeafProWriteCommandWait( pSelf, cmd, LEAFPRO_CMD_CHIP_STATE_EVENT);
    
    return retVal;
}

/******************************************************************************/
static canStatus LeafProCanRead (
    const   CanHandle hnd,
    UInt32  *id,
    void    *msg,
    UInt16  *dlc,
    UInt16  *flag,
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
static canStatus LeafProCanTranslateBaud (SInt32 *const freq,
                                unsigned int *const tseg1,
                                unsigned int *const tseg2,
                                unsigned int *const sjw,
                                unsigned int *const nosamp,
                                unsigned int *const syncMode)
{
    switch (*freq) {
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
            *tseg1    = 6;
            *tseg2    = 1;
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
            return canERR_PARAM;
    }
    
    return canOK;
}


#pragma mark LeafPro command stuff
/******************************************************************************/
/******************************************************************************/
/**************************** Command Stuff ***********************************/
/******************************************************************************/
/******************************************************************************/
static void LeafProDecodeCommand(Can4osxUsbDeviceHandleEntry *pSelf, proCommand_t *pCmd)
{
    CAN4OSX_DEBUG_PRINT("Pro-Decode command %d\n",(UInt8)pCmd->proCmdHead.cmdNo);
    LeafProPrivateData *pPriv = (LeafProPrivateData *)pSelf->privateData;
    
    switch (pCmd->proCmdHead.cmdNo) {
        case LEAFPRO_CMD_LOG_MESSAGE:
            {
            CanMsg canMsg;
            
                if ( pCmd->proCmdLogMessage.canId & LEAFPRO_EXT_MSG ) {
                    canMsg.canId = pCmd->proCmdLogMessage.canId & ~LEAFPRO_EXT_MSG;
                    canMsg.canFlags = canMSG_EXT;
                }  else {
                    canMsg.canId = pCmd->proCmdLogMessage.canId;
                    canMsg.canFlags = canMSG_STD;
                }
            
                if (pCmd->proCmdLogMessage.flags & LEAFPRO_MSG_FLAG_OVERRUN) {
                    //canMsg.canFlags |= canMSGERR_HW_OVERRUN | canMSGERR_SW_OVERRUN;
                }
                if (pCmd->proCmdLogMessage.flags & LEAFPRO_MSG_FLAG_REMOTE_FRAME) {
                    canMsg.canFlags |= canMSG_RTR;
                }
                if (pCmd->proCmdLogMessage.flags & LEAFPRO_MSG_FLAG_ERROR_FRAME) {
                    canMsg.canFlags |= canMSG_ERROR_FRAME;
                }
                if (pCmd->proCmdLogMessage.flags & LEAFPRO_MSG_FLAG_TXACK) {
                    canMsg.canFlags |= canMSG_TXACK;
                }
                if (pCmd->proCmdLogMessage.flags & LEAFPRO_MSG_FLAG_TXRQ) {
                    canMsg.canFlags |= canMSG_TXRQ;
                }
            
            
                if ( pCmd->proCmdLogMessage.dlc > 8u ) {
                    pCmd->proCmdLogMessage.dlc = 8u;
                }
            
                canMsg.canDlc = pCmd->proCmdLogMessage.dlc;
            
                memcpy(canMsg.canData, pCmd->proCmdLogMessage.data,
                       pCmd->proCmdLogMessage.dlc);
            
                // FIXME canMsg.canTimestamp = LeafCalculateTimeStamp(pCmd->proCmdLogMessage.time, 24) * 10;
            
            
                CAN4OSX_WriteCanEventBuffer(pSelf->canEventMsgBuff,canMsg);
                if (pSelf->canNotification.notifacionCenter) {
                    CFNotificationCenterPostNotification (pSelf->canNotification.notifacionCenter,
                                                      pSelf->canNotification.notificationString, NULL, NULL, true);
                }
            
            
                CAN4OSX_DEBUG_PRINT("PRO_CMD_LOG_MESSAGE Channel: Id: %X Flags: %X\n",
                                    pCmd->proCmdLogMessage.canId,
                                    pCmd->proCmdLogMessage.flags);
            
            }
            break;
            
        case LEAFPRO_CMD_MAP_CHANNEL_RESP:
            CAN4OSX_DEBUG_PRINT("LEAFPRO_CMD_MAP_CHANNEL_RESP chan %X\n",
                                pCmd->proCmdHead.transitionId);
            CAN4OSX_DEBUG_PRINT("LEAFPRO_CMD_MAP_CHANNEL_RESP adr %X\n",
                                pCmd->proCmdHead.address);
            pPriv->address = (UInt8)pCmd->proCmdMapChannelResp.heAddress;
            break;
            
        default:
            break;
    }
}


#pragma mark Leaf Pro mapping Stuff
/******************************************************************************/
/******************************************************************************/
/**************************** Mapping Stuff ***********************************/
/******************************************************************************/
/******************************************************************************/
static void LeafProMapChannels(
    Can4osxUsbDeviceHandleEntry
    *pSelf
    )
{
    proCommand_t cmd;
    
    cmd.proCmdHead.cmdNo = LEAFPRO_CMD_MAP_CHANNEL_REQ;
    cmd.proCmdHead.transitionId = 0x40;
    cmd.proCmdHead.address = 0x00;
    
    cmd.proCmdMapChannelReq.channel = 0;
    
    strcpy(cmd.proCmdMapChannelReq.name, "CAN");
        
    LeafProWriteCommandWait(pSelf, cmd, LEAFPRO_CMD_MAP_CHANNEL_RESP);

}


#pragma mark Command Buffer
static LeafProCommandMsgBuf_t* LeafProCreateCommandBuffer(UInt32 bufferSize)
{
    LeafProCommandMsgBuf_t* pBufferRef = malloc(sizeof(LeafProCommandMsgBuf_t));
    if ( pBufferRef == NULL ) {
        return NULL;
    }
    
    pBufferRef->bufferSize = bufferSize;
    pBufferRef->bufferCount = 0;
    pBufferRef->bufferFirst = 0;
    
    pBufferRef->commandRef = malloc(bufferSize * sizeof(proCommand_t));
    
    if ( pBufferRef->commandRef == NULL ) {
        free(pBufferRef);
        return NULL;
    }
    
    pBufferRef->bufferGDCqueueRef = dispatch_queue_create(
                                        "com.can4osx.leafprocommandqueue", 0);
    if ( pBufferRef->bufferGDCqueueRef == NULL ) {
        LeafProReleaseCommandBuffer(pBufferRef);
        return NULL;
    }
    
    return pBufferRef;
}


static void LeafProReleaseCommandBuffer( LeafProCommandMsgBuf_t* pBufferRef )
{
    if ( pBufferRef != NULL ) {
        if (pBufferRef->bufferGDCqueueRef != NULL) {
            dispatch_release(pBufferRef->bufferGDCqueueRef);
        }
        free(pBufferRef->commandRef);
        free(pBufferRef);
    }
}


#pragma mark USB-Stuff
/******************************************************************************/
/******************************************************************************/
/**************************** USB Low Level Stuff *****************************/
/******************************************************************************/
/******************************************************************************/
static IOReturn LeafProWriteCommandWait(
    Can4osxUsbDeviceHandleEntry *pSelf,
    proCommand_t cmd,
    UInt8 reason
    )
{
    IOReturn retVal = kIOReturnSuccess;
    IOUSBInterfaceInterface **interface = pSelf->can4osxInterfaceInterface;
    LeafProPrivateData *pPriv = (LeafProPrivateData *)pSelf->privateData;
    
    if( pSelf->endpoitBulkOutBusy == FALSE ) {
        
        pSelf->endpoitBulkOutBusy = TRUE;
        
        retVal = (*interface)->WritePipe(interface,
                                         pSelf->endpointNumberBulkOut,
                                         &cmd, LEAFPRO_COMMAND_SIZE);
        
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
    
    if ( dispatch_semaphore_wait(pPriv->semaTimeout,
                                 dispatch_time(DISPATCH_TIME_NOW,
                                               LEAFPRO_TIMEOUT_TEN_MS)) )
    {
        return canERR_TIMEOUT;
    } else {
        return retVal;
    }
}


static void LeafProBulkReadCompletion(
    void *refCon,
    IOReturn result,
    void *arg0
    )
{
    Can4osxUsbDeviceHandleEntry *pSelf = (Can4osxUsbDeviceHandleEntry *)refCon;
    LeafProPrivateData *pPriv = (LeafProPrivateData *)pSelf->privateData;
    IOUSBInterfaceInterface **interface = pSelf->can4osxInterfaceInterface;
    UInt32 numBytesRead = (UInt32) arg0;
    
    CAN4OSX_DEBUG_PRINT("Asynchronous bulk read complete (%ld)\n",
                        (long)numBytesRead);
    
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