//
//  kvaserLeafPro.c
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

#define LEAFPRO_COMMAND_SIZE 32u

#define LEAFPRO_CMD_SET_BUSPARAMS_REQ           16u
#define LEAFPRO_CMD_CHIP_STATE_EVENT            20u
#define LEAFPRO_CMD_SET_DRIVERMODE_REQ          21u
#define LEAFPRO_CMD_START_CHIP_REQ              26u
#define LEAFPRO_CMD_START_CHIP_RESP             27u
#define LEAFPRO_CMD_TX_CAN_MESSAGE              33u
#define LEAFPRO_CMD_GET_CARD_INFO_REQ           34u
#define LEAFPRO_CMD_GET_CARD_INFO_RESP          35u
#define LEAFPRO_CMD_GET_SOFTWARE_INFO_REQ       38u
#define LEAFPRO_CMD_GET_SOFTWARE_INFO_RESP      39u
#define LEAFPRO_CMD_SET_BUSPARAMS_FD_REQ        69u
#define LEAFPRO_CMD_SET_BUSPARAMS_FD_RESP       70u
#define LEAFPRO_CMD_SET_BUSPARAMS_RESP          85u
#define LEAFPRO_CMD_LOG_MESSAGE                 106u
#define LEAFPRO_CMD_MAP_CHANNEL_REQ             200u
#define LEAFPRO_CMD_MAP_CHANNEL_RESP            201u
#define LEAFPRO_CMD_GET_SOFTWARE_DETAILS_REQ    202u
#define LEAFPRO_CMD_GET_SOFTWARE_DETAILS_RESP   203u
#define LEAFPRO_CMD_RX_MESSAGE_FD               226u

/* extended FD able command code */
#define LEAFPRO_CMD_CAN_FD                      255u

/* extended capabilty flag */
#define LEASPRO_SUPPORT_EXTENDED                0x200u




#define LEAFPRO_HE_ILLEGAL      0x3eu
#define LEAFPRO_HE_ROUTER       0x00u

#define LEAFPRO_TIMEOUT_ONE_MS 1000000
#define LEAFPRO_TIMEOUT_TEN_MS 10*LEAFPRO_TIMEOUT_ONE_MS

static char* pDeviceString = "Kvaser Leaf Pro v2";

static UInt32 getCommandSize(proCommand_t *pCmd);

static void LeafProDecodeCommand(Can4osxUsbDeviceHandleEntry *pSelf,
                                 proCommand_t *pCmd);
static void LeafProDecodeCommandExt(Can4osxUsbDeviceHandleEntry *pSelf,
                                 proCommandExt_t *pCmd);

static void LeafProMapChannels(Can4osxUsbDeviceHandleEntry *pSelf);

static void LeafProGetCardInfo(Can4osxUsbDeviceHandleEntry *pSelf);

static canStatus LeafProCanSetBusParams (const CanHandle hnd, SInt32 freq,
            unsigned int tseg1, unsigned int tseg2, unsigned int sjw,
            unsigned int noSamp, unsigned int syncmode);

static canStatus LeafProCanSetBusParamsFd(const CanHandle hnd, SInt32 freq_brs,
            UInt32 tseg1, UInt32 tseg2, UInt32 sjw);

static canStatus LeafProCanRead (const CanHandle hnd, UInt32 *id, void *msg,
            UInt16 *dlc, UInt32 *flag, UInt32 *time);

static canStatus LeafProCanWrite(const CanHandle hnd, UInt32 id, void *msg,
            UInt16 dlc, UInt32 flag);

static canStatus LeafProCanWriteExt(Can4osxUsbDeviceHandleEntry *pSelf,
            UInt32 id, void *pMsg, UInt16 dlc, UInt32 flag);

static canStatus LeafProCanTranslateBaud (SInt32 *const freq,
            unsigned int *const tseg1, unsigned int *const tseg2,
            unsigned int *const sjw, unsigned int *const nosamp,
            unsigned int *const syncMode);

static IOReturn LeafProCommandWait(Can4osxUsbDeviceHandleEntry *pSelf,
			proCommand_t *pCmd, UInt8 cmdNo);

static IOReturn LeafProWriteCommandWait(Can4osxUsbDeviceHandleEntry *pSelf,
            proCommand_t cmd, UInt8 reason);

static LeafProCommandMsgBuf_t* LeafProCreateCommandBuffer(UInt32 bufferSize);
static void LeafProReleaseCommandBuffer(LeafProCommandMsgBuf_t* pBufferRef);
static UInt8 LeafProTestFullCommandBuffer(LeafProCommandMsgBuf_t* bufferRef);
static UInt8 LeafProTestEmptyCommandBuffer(LeafProCommandMsgBuf_t* pBufferRef);
static UInt8 LeafProWriteCommandBuffer(LeafProCommandMsgBuf_t* pBufferRef,
                                       proCommand_t newCommand);

static UInt16 LeafProFillBulkPipeBuffer(LeafProCommandMsgBuf_t* bufferRef,
            UInt8 *pPipe, UInt16 maxPipeSize);
static IOReturn LeafProWriteBulkPipe(Can4osxUsbDeviceHandleEntry *pSelf);
static void LeafProReadFromBulkInPipe(Can4osxUsbDeviceHandleEntry *self);
static void LeafProBulkReadCompletion(void *refCon, IOReturn result,
            void *arg0);


//Hardware interface function
static canStatus LeafProInitHardware(const CanHandle hnd);
static CanHandle LeafProCanOpenChannel(int channel, int flags);
static canStatus LeafProCanStartChip(CanHandle hdl);
static canStatus LeafProCanStopChip(CanHandle hdl);

CAN4OSX_HW_FUNC_T leafProHardwareFunctions = {
    .can4osxhwInitRef = LeafProInitHardware,
    .can4osxhwCanOpenChannel = LeafProCanOpenChannel,
    .can4osxhwCanSetBusParamsRef = LeafProCanSetBusParams,
    .can4osxhwCanSetBusParamsFdRef = LeafProCanSetBusParamsFd,
    .can4osxhwCanBusOnRef = LeafProCanStartChip,
    .can4osxhwCanBusOffRef = LeafProCanStopChip,
    .can4osxhwCanWriteRef = LeafProCanWrite,
    .can4osxhwCanReadRef = LeafProCanRead,
    .can4osxhwCanCloseRef = NULL,
};


static canStatus LeafProInitHardware(
        const CanHandle hnd
    )
{
Can4osxUsbDeviceHandleEntry *pSelf = &can4osxUsbDeviceHandle[hnd];
pSelf->privateData = calloc(1,sizeof(LeafProPrivateData_t));

    if ( pSelf->privateData != NULL ) {
    LeafProPrivateData_t *pPriv = (LeafProPrivateData_t *)pSelf->privateData;
        
        pPriv->cmdBufferRef = LeafProCreateCommandBuffer(1000);
        if ( pPriv->cmdBufferRef == NULL ) {
            free(pPriv);
            return(canERR_NOMEM);
        }

        pPriv->semaTimeout = dispatch_semaphore_create(0);
        
    } else {
        return(canERR_NOMEM);
    }

    if (pSelf->deviceChannel == 0u) {
    	/* Set some device Infos */
    	pSelf->devInfo.capability = 0u;
    	pSelf->devInfo.capability |= canCHANNEL_CAP_CAN_FD;
    	sprintf((char*)pSelf->devInfo.deviceString, "%s",pDeviceString);
        /* Trigger next read */
        //LeafProReadFromBulkInPipe(pSelf);
 
    	/* Set up channels */
    	LeafProMapChannels(pSelf);
    
    	/* Get channel info */
    	LeafProGetCardInfo(pSelf);
    
        /* Trigger next read */
        LeafProReadFromBulkInPipe(pSelf);
    }
    
    return(canOK);
}


static CanHandle LeafProCanOpenChannel(
        int channel,
        int flags
    )
{
Can4osxUsbDeviceHandleEntry *pSelf = &can4osxUsbDeviceHandle[channel];
LeafProPrivateData_t *pPriv = (LeafProPrivateData_t *)pSelf->privateData;
    
    // set CAN Mode
    if ((flags & canOPEN_CAN_FD) ==  canOPEN_CAN_FD) {
        pPriv->canFd = 1;
    } else {
        pPriv->canFd = 0;
    }

    return (CanHandle)channel;
}


//Set bit timing
static canStatus LeafProCanSetBusParams (
        const CanHandle hnd,
        SInt32 freq,
        unsigned int tseg1,
        unsigned int tseg2,
        unsigned int sjw,
        unsigned int noSamp,
        unsigned int syncmode
    )
{
proCommand_t   cmd;
// FIXME UInt32         tmp, PScl;
int retVal;
    
Can4osxUsbDeviceHandleEntry *pSelf = &can4osxUsbDeviceHandle[hnd];
LeafProPrivateData_t *pPriv = (LeafProPrivateData_t *)pSelf->privateData;
    
    CAN4OSX_DEBUG_PRINT("leaf pro: _set_busparam\n");
    
    if ( canOK != LeafProCanTranslateBaud(&freq, &tseg1, &tseg2, &sjw,
                                          &noSamp, &syncmode)) {
        // TODO
        CAN4OSX_DEBUG_PRINT(" can4osx strange bitrate\n");
        return(-1);
    }
    
#if 0 //FIXME
    // Check bus parameters
    tmp = freq * (tseg1 + tseg2 + 1);
    if (tmp == 0) {
        CAN4OSX_DEBUG_PRINT("leaf: _set_busparams() tmp == 0!\n");
        return VCAN_STAT_BAD_PARAMETER;
    }
    
    PScl = 16000000UL / tmp;
    
    if (PScl </*=*/ 1 || PScl > 256) {
        CAN4OSX_DEBUG_PRINT("hwif_set_chip_param() prescaler wrong (%d)\n",
                PScl & 1 /* even */);
        return VCAN_STAT_BAD_PARAMETER;
    }
#endif

    memset(&cmd, 0 , sizeof(cmd));
    
    cmd.proCmdSetBusparamsReq.header.cmdNo = LEAFPRO_CMD_SET_BUSPARAMS_REQ;
    cmd.proCmdSetBusparamsReq.header.address = pPriv->address;
    cmd.proCmdSetBusparamsReq.header.transitionId = 0x0000;
    
    cmd.proCmdSetBusparamsReq.bitRate = freq;
    cmd.proCmdSetBusparamsReq.sjw     = (UInt8)sjw;
    cmd.proCmdSetBusparamsReq.tseg1   = (UInt8)tseg1;
    cmd.proCmdSetBusparamsReq.tseg2   = (UInt8)tseg2;
    cmd.proCmdSetBusparamsReq.noSamp  = noSamp;
    
    retVal = LeafProWriteCommandWait( pSelf, cmd,LEAFPRO_CMD_SET_BUSPARAMS_RESP);
    
    /* save locally */
    pPriv->freq = freq;
    pPriv->sjw = sjw;
    pPriv->tseg1 = tseg1;
    pPriv->tseg2 = tseg2;
    pPriv->nosamp = noSamp;
    
    return(retVal);
}


static canStatus LeafProCanSetBusParamsFd(
        const CanHandle hnd,
        SInt32 freq_brs,
        UInt32 tseg1,
        UInt32 tseg2,
        UInt32 sjw
    )
{
unsigned int syncmode;
unsigned int noSamp;
proCommand_t   cmd;
int            retVal;
    
Can4osxUsbDeviceHandleEntry *pSelf = &can4osxUsbDeviceHandle[hnd];
LeafProPrivateData_t *pPriv = (LeafProPrivateData_t *)pSelf->privateData;
    
    CAN4OSX_DEBUG_PRINT("leaf pro: _set_FD_busparam\n");
    
    if (pPriv->canFd == 0u)  {
        return(canERR_NOTINITIALIZED);
    }
    
    if ( canOK != LeafProCanTranslateBaud(&freq_brs, &tseg1, &tseg2, &sjw,
                                          &noSamp, &syncmode)) {
        // TODO
        CAN4OSX_DEBUG_PRINT(" can4osx strange bitrate\n");
        return(canERR_PARAM);
    }
    memset(&cmd, 0 , sizeof(cmd));
    
    cmd.proCmdSetBusparamsReq.header.cmdNo = LEAFPRO_CMD_SET_BUSPARAMS_FD_REQ;
    cmd.proCmdSetBusparamsReq.header.address = pPriv->address;
    cmd.proCmdSetBusparamsReq.header.transitionId = 0x0000;
    cmd.proCmdSetBusparamsReq.open_as_canfd = 1;
    
    cmd.proCmdSetBusparamsReq.bitRate = pPriv->freq;
    cmd.proCmdSetBusparamsReq.sjw     = pPriv->sjw;
    cmd.proCmdSetBusparamsReq.tseg1   = pPriv->tseg1;
    cmd.proCmdSetBusparamsReq.tseg2   = pPriv->tseg2;
    cmd.proCmdSetBusparamsReq.noSamp  = 1;
    cmd.proCmdSetBusparamsReq.bitRateFd = freq_brs;
    cmd.proCmdSetBusparamsReq.tseg1Fd = tseg1;
    cmd.proCmdSetBusparamsReq.tseg2Fd = tseg2;
    cmd.proCmdSetBusparamsReq.sjwFd = sjw;
    cmd.proCmdSetBusparamsReq.noSampFd = 1;
    
    retVal = LeafProWriteCommandWait( pSelf, cmd,
                LEAFPRO_CMD_SET_BUSPARAMS_FD_RESP);

    /* save locally */
    pPriv->fd_freq = freq_brs;
    pPriv->fd_sjw = sjw;
    pPriv->fd_tseg1 = tseg1;
    pPriv->fd_tseg2 = tseg2;
    pPriv->fd_nosamp = 0;

    return(canOK);
}

//Go bus on
static canStatus LeafProCanStartChip(
        CanHandle hdl
        )
{
int retVal = 0;
proCommand_t        cmd;
Can4osxUsbDeviceHandleEntry *pSelf = &can4osxUsbDeviceHandle[hdl];
LeafProPrivateData_t *pPriv = (LeafProPrivateData_t *)pSelf->privateData;

    memset(&cmd, 0u, sizeof(cmd));
    cmd.proCmdHead.cmdNo = LEAFPRO_CMD_SET_DRIVERMODE_REQ;
    cmd.proCmdHead.address = pPriv->address;
    cmd.proCmdRaw.data[0] = 0x01;
    LeafProWriteCommandWait(pSelf, cmd, LEAFPRO_CMD_MAP_CHANNEL_RESP);

    
    CAN4OSX_DEBUG_PRINT("CAN BusOn Command %d\n", hdl);
    memset(&cmd, 0u, sizeof(cmd));
    
    cmd.proCmdHead.cmdNo = LEAFPRO_CMD_START_CHIP_REQ;
    cmd.proCmdHead.address = pPriv->address;
    cmd.proCmdHead.transitionId = 1u;
    retVal = LeafProWriteCommandWait(pSelf, cmd, LEAFPRO_CMD_CHIP_STATE_EVENT);
    
    return(retVal);
}


static canStatus LeafProCanStopChip(
        CanHandle hdl
        )
{
    return 0;
}


/******************************************************************************/
static canStatus LeafProCanRead (
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
static canStatus LeafProCanWrite(
        const CanHandle hnd,
        UInt32 id,
        void *msg,
        UInt16 dlc,
        UInt32 flag
    )
{
Can4osxUsbDeviceHandleEntry *pSelf = &can4osxUsbDeviceHandle[hnd];

    if ( pSelf->privateData == NULL ) {
        return(canERR_INTERNAL);
    }
    
    LeafProPrivateData_t *pPriv = (LeafProPrivateData_t*)pSelf->privateData;
        
    if (pPriv->extendedMode == 0u)  {
        proCommand_t cmd;
        
        if (flag & canMSG_EXT)  {
            cmd.proCmdTxMessage.canId = LEAFPRO_EXT_MSG;
        } else {
            cmd.proCmdTxMessage.canId = 0u;
        }
        cmd.proCmdTxMessage.canId += id;
        cmd.proCmdTxMessage.dlc = dlc & 0x0F;
        memcpy(cmd.proCmdTxMessage.data, msg, 8);
        
        cmd.proCmdTxMessage.flags = 0;

        if ( flag & canMSG_RTR ) {
            cmd.proCmdTxMessage.flags |= LEAFPRO_MSG_FLAG_REMOTE_FRAME;
        }
    
        cmd.proCmdHead.cmdNo = LEAFPRO_CMD_TX_CAN_MESSAGE;
        cmd.proCmdHead.address = pPriv->address;
        cmd.proCmdHead.transitionId = 10;
        
        LeafProWriteCommandBuffer(pPriv->cmdBufferRef, cmd);

        LeafProWriteBulkPipe(pSelf);
    } else {
        (void)LeafProCanWriteExt(pSelf, id, msg, dlc, flag);
    }
        
    return(canOK);
}


static canStatus LeafProCanWriteExt(
        Can4osxUsbDeviceHandleEntry *pSelf,
        UInt32 id,
        void *pMsg,
        UInt16 dlc,
        UInt32 flag
    )
{
LeafProPrivateData_t *pPriv = (LeafProPrivateData_t*)pSelf->privateData;
proCommand_t extCmd;

    /* in extended mode we alway use this kind of command */
    extCmd.proCommandExt.proCmdFdHead.header.cmdNo = LEAFPRO_CMD_CAN_FD;
    
    LeafProWriteCommandBuffer(pPriv->cmdBufferRef, extCmd);

    LeafProWriteBulkPipe(pSelf);
    return(canOK);
}

                                 
/******************************************************************************/
// Translate from baud macro to bus params
/******************************************************************************/
static canStatus LeafProCanTranslateBaud (
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
            *tseg1    = 15;
            *tseg2    = 4;
            *sjw      = 4;
            break;

        case canFD_BITRATE_1M_80P:
            *freq     = 1000000L;
            *tseg1    = 31;
            *tseg2    = 8;
            *sjw      = 8;
            break;

        case canFD_BITRATE_500K_80P:
            *freq     = 500000L;
            *tseg1    = 63;
            *tseg2    = 16;
            *sjw      = 16;
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


#pragma mark LeafPro command stuff
/******************************************************************************/
/******************************************************************************/
/**************************** Command Stuff ***********************************/
/******************************************************************************/
/******************************************************************************/


static UInt32 getCommandSize(
        proCommand_t *pCmd
    )
{
    if (pCmd->proCmdHead.cmdNo == LEAFPRO_CMD_CAN_FD)  {
        return(((proCmdFdHead_t *)(pCmd))->len);
    } else {
        return(LEAFPRO_COMMAND_SIZE);
    }
}


static UInt8 calcExtendedCommandSize(
        UInt8 dlc
    )
{


    return(8u);
}


/******************************************************************************/
static void LeafProDecodeCommand(
        Can4osxUsbDeviceHandleEntry *pSelf, /**< pointer to my reference */
        proCommand_t *pCmd
    )
{
LeafProPrivateData_t *pPriv = (LeafProPrivateData_t *)pSelf->privateData;
CanMsg canMsg;

    CAN4OSX_DEBUG_PRINT("Pro-Decode cmd %d\n",(UInt8)pCmd->proCmdHead.cmdNo);

    switch (pCmd->proCmdHead.cmdNo) {
        case LEAFPRO_CMD_CAN_FD:
            CAN4OSX_DEBUG_PRINT("LEAFPRO_CMD_CAN_FD\n");
            LeafProDecodeCommandExt(pSelf, (proCommandExt_t *)pCmd);
            break;
        case LEAFPRO_CMD_LOG_MESSAGE:
            if ( pCmd->proCmdLogMessage.canId & LEAFPRO_EXT_MSG ) {
                canMsg.canId = pCmd->proCmdLogMessage.canId & ~LEAFPRO_EXT_MSG;
                canMsg.canFlags = canMSG_EXT;
            } else {
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
            /* classical CAN dlc */
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
            break;
        case LEAFPRO_CMD_MAP_CHANNEL_RESP:
            CAN4OSX_DEBUG_PRINT("LEAFPRO_CMD_MAP_CHANNEL_RESP chan %X\n",
                                pCmd->proCmdHead.transitionId);
            CAN4OSX_DEBUG_PRINT("LEAFPRO_CMD_MAP_CHANNEL_RESP adr %X\n",
                                pCmd->proCmdHead.address);
            if ((pCmd->proCmdHead.transitionId & 0xff) == 0x40u)  {
                CAN4OSX_DEBUG_PRINT("LEAFPRO_CMD_MAP_CHANNEL_RESP CAN\n");
                pPriv->address = (UInt8)pCmd->proCmdMapChannelResp.heAddress;
            }
            break;
        case LEAFPRO_CMD_GET_CARD_INFO_RESP:
        	pSelf->deviceChannelCount = pCmd->proCmdCardInfoResp.nchannels;
        	break;
        case LEAFPRO_CMD_GET_SOFTWARE_DETAILS_RESP:
            CAN4OSX_DEBUG_PRINT("LEAFPRO_CMD_GET_SOFTWARE_DETAILS_RESP\n");
            if (pCmd->proCcmdGetSoftwareDetailsResp.swOptions & LEASPRO_SUPPORT_EXTENDED)  {
                pPriv->extendedMode = 1u;
            }

            break;
        case LEAFPRO_CMD_GET_SOFTWARE_INFO_RESP:
            break;
        default:
            break;
    }
#if CAN4OSX_DEBUG
    CAN4OSX_DEBUG_PRINT("LEAFPRO_MESSAGE %d\n", pCmd->proCmdHead.cmdNo);
#endif /* CAN4OSX_DEBUG */
}


/******************************************************************************/
static void LeafProDecodeCommandExt(
        Can4osxUsbDeviceHandleEntry *pSelf, /**< pointer to my reference */
        proCommandExt_t *pCmd
    )
{
LeafProPrivateData_t *pPriv = (LeafProPrivateData_t *)pSelf->privateData;
CanMsg canMsg;

    switch (pCmd->proCmdFdHead.header.cmdNo)  {
        default:
            if (pCmd->proCmdFdRxMessage.flags & LEAFPRO_MSG_FLAG_ERROR_FRAME) {
                CAN4OSX_DEBUG_PRINT("LEAFPRO_MESSAGE ERROR_FRAME\n");
                break;
            }
            
            canMsg.canTimestamp = pCmd->proCmdFdRxMessage.timestamp;
        
            canMsg.canId = pCmd->proCmdFdRxMessage.canId;
            canMsg.canDlc = (pCmd->proCmdFdRxMessage.control>>8u) & 0x0fu;
            
            canMsg.canFlags = 0u;
            
            if (pCmd->proCmdFdRxMessage.flags & LEAFPRO_MSGFLAG_FDF)  {
                /* insanity check */
                if (pPriv->canFd == 0u)  {
                    return;
                }
            
                canMsg.canFlags |= canFDMSG_FDF;
                
                /* test for other FD flags */
                if (pCmd->proCmdFdRxMessage.flags & LEAFPRO_MSGFLAG_BRS)  {
                    canMsg.canFlags |= canFDMSG_BRS;
                }
                /* decode dlc to length */
                canMsg.canDlc = CAN4OSX_decodeFdDlc(canMsg.canDlc);
                
            } else {
                CAN4OSX_DEBUG_PRINT("LEAFPRO_MESSAGE CLASSIC\n");
                if (canMsg.canDlc > 8u)  {
                    canMsg.canDlc = 8u;
                }
            }
            
            if (pCmd->proCmdFdRxMessage.flags & LEAFPRO_MSG_FLAG_EXTENDED) {
                canMsg.canFlags |= canMSG_EXT;
            } else {
                CAN4OSX_DEBUG_PRINT("LEAFPRO_MESSAGE STD\n");
                canMsg.canFlags |= canMSG_STD;
            }
            
            memcpy(canMsg.canData, pCmd->proCmdFdRxMessage.data, canMsg.canDlc);
            
            CAN4OSX_WriteCanEventBuffer(pSelf->canEventMsgBuff,canMsg);
            
            if (pSelf->canNotification.notifacionCenter) {
                CFNotificationCenterPostNotification (pSelf->canNotification.notifacionCenter,
                    pSelf->canNotification.notificationString, NULL, NULL, true);
            }
            
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
        Can4osxUsbDeviceHandleEntry *pSelf /**< pointer to my reference */
    )
{
proCommand_t cmd;
proCommand_t resp;
UInt8 i = 0u;

    memset(&cmd, 0u, 32u);
    
    cmd.proCmdHead.cmdNo = LEAFPRO_CMD_MAP_CHANNEL_REQ;
    cmd.proCmdHead.address = LEAFPRO_HE_ROUTER;
    cmd.proCmdMapChannelReq.channel = 0u;
    
    strcpy(cmd.proCmdMapChannelReq.name, "CAN");
    cmd.proCmdHead.transitionId = 0x40;
    for (i = 0u ; i < 5u; i++)  {
    	cmd.proCmdHead.transitionId = 0x40 + i;
    	cmd.proCmdMapChannelReq.channel = i;
    	LeafProWriteCommandWait(pSelf, cmd, LEAFPRO_CMD_MAP_CHANNEL_RESP);
    	LeafProCommandWait(pSelf, &resp, LEAFPRO_CMD_MAP_CHANNEL_RESP);
    }
    
    /* do we really need that? */
    strcpy(cmd.proCmdMapChannelReq.name, "SYSDBG");
    cmd.proCmdMapChannelReq.channel = 0;
    cmd.proCmdHead.transitionId = 0x61;
    LeafProWriteCommandWait(pSelf, cmd, LEAFPRO_CMD_MAP_CHANNEL_RESP);
    LeafProCommandWait(pSelf, &resp, LEAFPRO_CMD_MAP_CHANNEL_RESP);

    return;
}


#pragma mark card info request
/******************************************************************************/
static void LeafProGetCardInfo(
        Can4osxUsbDeviceHandleEntry *pSelf /**< pointer to my reference */
    )
{
proCommand_t cmd;
proCommand_t resp;
IOReturn retVal;


    memset(&cmd, 0u, sizeof(cmd));
    cmd.proCmdHead.address = LEAFPRO_HE_ILLEGAL;

    cmd.proCmdHead.cmdNo = LEAFPRO_CMD_GET_CARD_INFO_REQ;
    LeafProWriteCommandWait(pSelf, cmd, LEAFPRO_CMD_GET_CARD_INFO_RESP);
    retVal = LeafProCommandWait(pSelf, &resp, LEAFPRO_CMD_GET_CARD_INFO_RESP);
	if (retVal == kIOReturnSuccess)  {
		pSelf->deviceChannelCount = resp.proCmdCardInfoResp.nchannels;
    }

    cmd.proCmdHead.cmdNo = LEAFPRO_CMD_GET_SOFTWARE_INFO_REQ;
    cmd.proCmdGetSoftwareDetailsReq.useExt = 1u;
    LeafProWriteCommandWait(pSelf, cmd, LEAFPRO_CMD_GET_SOFTWARE_INFO_RESP);
    
    cmd.proCmdHead.cmdNo = LEAFPRO_CMD_GET_SOFTWARE_DETAILS_REQ;
    LeafProWriteCommandWait(pSelf, cmd, LEAFPRO_CMD_GET_SOFTWARE_INFO_RESP);
    
    return;
}


#pragma mark Command Buffer
/******************************************************************************/
static LeafProCommandMsgBuf_t* LeafProCreateCommandBuffer(
        UInt32 bufferSize
    )
{
LeafProCommandMsgBuf_t* pBufferRef = malloc(sizeof(LeafProCommandMsgBuf_t));

    if ( pBufferRef == NULL ) {
        return(NULL);
    }
    
    pBufferRef->bufferSize = bufferSize;
    pBufferRef->bufferCount = 0u;
    pBufferRef->bufferFirst = 0u;
    
    pBufferRef->commandRef = malloc(bufferSize * sizeof(proCommand_t));
    
    if ( pBufferRef->commandRef == NULL ) {
        free(pBufferRef);
        return(NULL);
    }
    
    pBufferRef->bufferGDCqueueRef = dispatch_queue_create(
                                        "com.can4osx.leafprocommandqueue", 0u);
    if ( pBufferRef->bufferGDCqueueRef == NULL ) {
        LeafProReleaseCommandBuffer(pBufferRef);
        return(NULL);
    }
    
    return(pBufferRef);
}


/******************************************************************************/
static void LeafProReleaseCommandBuffer(
        LeafProCommandMsgBuf_t* pBufferRef
    )
{
    if ( pBufferRef != NULL ) {
        if (pBufferRef->bufferGDCqueueRef != NULL) {
            dispatch_release(pBufferRef->bufferGDCqueueRef);
        }
        free(pBufferRef->commandRef);
        free(pBufferRef);
    }
    return;
}


/******************************************************************************/
static UInt8 LeafProWriteCommandBuffer(
        LeafProCommandMsgBuf_t* pBufferRef,
        proCommand_t newCommand
    )
{
__block UInt8 retval = 1u;
    
    dispatch_sync(pBufferRef->bufferGDCqueueRef, ^{
        if (LeafProTestFullCommandBuffer(pBufferRef)) {
            retval = 0u;
        } else {
            pBufferRef->commandRef[(pBufferRef->bufferFirst +
                                    pBufferRef->bufferCount++)
                                   % pBufferRef->bufferSize] = newCommand;
        }
    });
    
    return(retval);
}


/******************************************************************************/
static UInt8 LeafReadCommandBuffer(
        LeafProCommandMsgBuf_t* pBufferRef,
        proCommand_t* pReadCommand
    )
{
__block UInt8 retval = 1u;
    
    dispatch_sync(pBufferRef->bufferGDCqueueRef, ^{
        if (LeafProTestEmptyCommandBuffer(pBufferRef)) {
            retval = 0u;
        } else {
            pBufferRef->bufferCount--;
            *pReadCommand = pBufferRef->commandRef[pBufferRef->bufferFirst++
                % pBufferRef->bufferSize];
        }
        
    });
    
    return(retval);
}


/******************************************************************************/
static UInt8 LeafProTestFullCommandBuffer(
        LeafProCommandMsgBuf_t* pBufferRef
    )
{
    if (pBufferRef->bufferCount == pBufferRef->bufferSize) {
        return 1u;
    } else {
        return 0u;
    }
}


/******************************************************************************/
static UInt8 LeafProTestEmptyCommandBuffer(
        LeafProCommandMsgBuf_t* pBufferRef
    )
{
    if ( pBufferRef->bufferCount == 0u ) {
        return(1u);
    } else {
        return(0u);
    }
}


#pragma mark USB-Stuff
/******************************************************************************/
/******************************************************************************/
/**************************** USB Low Level Stuff *****************************/
/******************************************************************************/
/******************************************************************************/

/******************************************************************************/
static void LeafProBulkWriteCompletion(
        void *refCon,
        IOReturn result,
        void *arg0
    )
{
Can4osxUsbDeviceHandleEntry *self = (Can4osxUsbDeviceHandleEntry *)refCon;
IOUSBInterfaceInterface **interface = self->can4osxInterfaceInterface;
UInt32 numBytesWritten = (UInt32)arg0;
    
    (void)numBytesWritten;
    
    if (result != kIOReturnSuccess) {
        CAN4OSX_DEBUG_PRINT("error from asynchronous bulk write (%08x)\n", result);
        (void)(*interface)->USBInterfaceClose(interface);
        (void)(*interface)->Release(interface);
        return;
    }
    
    self->endpoitBulkOutBusy = FALSE;
    
    LeafProWriteBulkPipe(self);
    
    return;
}


/******************************************************************************/
static IOReturn LeafProWriteBulkPipe(
        Can4osxUsbDeviceHandleEntry *pSelf /**< pointer to my reference */
    )
{
IOReturn retval = kIOReturnSuccess;
IOUSBInterfaceInterface **interface = pSelf->can4osxInterfaceInterface;
LeafProPrivateData_t *pPriv = (LeafProPrivateData_t *)pSelf->privateData;
    
    if ( pSelf->endpoitBulkOutBusy == FALSE ) {
        pSelf->endpoitBulkOutBusy = TRUE;
        
        if (0 < LeafProFillBulkPipeBuffer(pPriv->cmdBufferRef,
                                       pSelf->endpointBufferBulkOutRef,
                                       pSelf->endpointMaxSizeBulkOut )) {
            
            retval = (*interface)->WritePipeAsync(interface,
                                                  pSelf->endpointNumberBulkOut,
                                                  pSelf->endpointBufferBulkOutRef,
                                                  pSelf->endpointMaxSizeBulkOut,
                                                  LeafProBulkWriteCompletion,
                                                  (void*)pSelf);
            
            if (retval != kIOReturnSuccess) {
                CAN4OSX_DEBUG_PRINT("Unable to perform asynchronous bulk write (%08x)\n", retval);
                (void) (*interface)->USBInterfaceClose(interface);
                (void) (*interface)->Release(interface);
            }
        } else {
            pSelf->endpoitBulkOutBusy = FALSE;
        }
    }
    
    return(retval);
}


/******************************************************************************/
static UInt16 LeafProFillBulkPipeBuffer(
        LeafProCommandMsgBuf_t* bufferRef,
        UInt8 *pPipe,
        UInt16 maxPipeSize
    )
{
UInt16 fillState = 0u;
    
    while ( fillState < maxPipeSize ) {
        proCommand_t cmd;
        if ( LeafReadCommandBuffer(bufferRef, &cmd) ) {
            memcpy(pPipe, &cmd, LEAFPRO_COMMAND_SIZE);
            fillState += LEAFPRO_COMMAND_SIZE;
            pPipe += LEAFPRO_COMMAND_SIZE;
            //Will another command for in the pipe?
            if ( (fillState + LEAFPRO_COMMAND_SIZE) >= maxPipeSize ) {
                *pPipe = 0u;
                break;
            }
            
        } else {
            *pPipe = 0u;
            break;
        }
    }
    
    return(fillState);
}

/******************************************************************************/
static IOReturn LeafProCommandWait(
        Can4osxUsbDeviceHandleEntry *pSelf,  /**< pointer to my reference */
        proCommand_t *pCmd,
        UInt8 cmdNo
    )
{
IOReturn retVal = kIOReturnSuccess;
IOUSBInterfaceInterface **interface = pSelf->can4osxInterfaceInterface;

UInt32 size = LEAFPRO_COMMAND_SIZE;
UInt64 timeout;

    timeout = CAN$OSX_getMilliseconds() + (10 * 5u);
    do {
         (*interface)->ReadPipe(interface, pSelf->endpointNumberBulkIn, pCmd, &size);
         printf("Size:%d ",size);
        if(pCmd->proCmdHead.cmdNo == cmdNo)  {
            return(retVal);
        }
    } while(CAN$OSX_getMilliseconds() < timeout);
    
    return(kIOReturnError);
}


/******************************************************************************/
static IOReturn LeafProWriteCommandWait(
        Can4osxUsbDeviceHandleEntry *pSelf,  /**< pointer to my reference */
        proCommand_t cmd,
        UInt8 reason
    )
{
IOReturn retVal = kIOReturnSuccess;
IOUSBInterfaceInterface **interface = pSelf->can4osxInterfaceInterface;
LeafProPrivateData_t *pPriv = (LeafProPrivateData_t *)pSelf->privateData;
    
    if( pSelf->endpoitBulkOutBusy == FALSE ) {
        
        pSelf->endpoitBulkOutBusy = TRUE;
        
        retVal = (*interface)->WritePipe(interface,
                                         pSelf->endpointNumberBulkOut,
                                         &cmd, LEAFPRO_COMMAND_SIZE);
        
        if (retVal != kIOReturnSuccess) {
            CAN4OSX_DEBUG_PRINT("Unable to perform synchronous bulk write (%08x)\n", retVal);
            (void)(*interface)->USBInterfaceClose(interface);
            (void)(*interface)->Release(interface);
        }
        
        pSelf->endpoitBulkOutBusy = FALSE;
        
       
    } else {
        /* endpoint busy */
        LeafProWriteCommandBuffer(pPriv->cmdBufferRef, cmd);
    }

    
    pPriv->timeOutReason = reason;
    return(retVal);
    if (dispatch_semaphore_wait(pPriv->semaTimeout,
            dispatch_time(DISPATCH_TIME_NOW, LEAFPRO_TIMEOUT_TEN_MS * 10u)))  {
        return(canERR_TIMEOUT);
    } else {
        return(retVal);
    }
}


/******************************************************************************/
static void LeafProBulkReadCompletion(
        void *refCon,
        IOReturn result,
        void *arg0
    )
{
Can4osxUsbDeviceHandleEntry *pSelf = (Can4osxUsbDeviceHandleEntry *)refCon;
LeafProPrivateData_t *pPriv = (LeafProPrivateData_t *)pSelf->privateData;
IOUSBInterfaceInterface **interface = pSelf->can4osxInterfaceInterface;
UInt32 numBytesRead = (UInt32) arg0;
    
    if (result != kIOReturnSuccess) {
        (void)(*interface)->USBInterfaceClose(interface);
        (void)(*interface)->Release(interface);
        return;
    }
    
    if (numBytesRead > 0u) {
        int count = 0;
        proCommand_t *pCmd;
        int loopCounter = pSelf->endpointMaxSizeBulkIn;
        
        while ( count < numBytesRead ) {
            if (loopCounter-- == 0) break;
            
            pCmd = (proCommand_t *)&(pSelf->endpointBufferBulkInRef[count]);
            
            if (pCmd->proCmdHead.cmdNo != 0u) {
                count += getCommandSize(pCmd);
                LeafProDecodeCommand(pSelf, pCmd);
            } else {
                /* No command */
                count += pSelf->endpointMaxSizeBulkIn;;
                count &= -pSelf->endpointMaxSizeBulkIn;
            }
            
            /* See if we had to wait */
            if (pCmd->proCmdHead.cmdNo == pPriv->timeOutReason) {
                pPriv->timeOutReason = 0;
                dispatch_semaphore_signal(pPriv->semaTimeout);
            }
        }
    }
    
    /* Trigger next read */
    LeafProReadFromBulkInPipe(pSelf);
}


/******************************************************************************/
static void LeafProReadFromBulkInPipe(
	    Can4osxUsbDeviceHandleEntry *pSelf /**< pointer to my reference */
    )
{
IOReturn ret = (*(pSelf->can4osxInterfaceInterface))->ReadPipeAsync(pSelf->can4osxInterfaceInterface, pSelf->endpointNumberBulkIn, pSelf->endpointBufferBulkInRef, pSelf->endpointMaxSizeBulkIn, LeafProBulkReadCompletion, (void*)pSelf);
    
    if (ret != kIOReturnSuccess) {
        CAN4OSX_DEBUG_PRINT("Unable to read async interface (%08x)\n", ret);
    }
}


