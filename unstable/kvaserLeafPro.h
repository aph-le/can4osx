//
//  kvaserLeafPro.h
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

#ifndef can4osx_logExample_kvaserLeafPro_h
#define can4osx_logExample_kvaserLeafPro_h


// Info
/*
 setSeq setz letzten 8 bit von transId
 
 
 */

Can4osxHwFunctions leafProHardwareFunctions;

#define LEAFPRO_CMD_SET_BUSPARAMS_REQ           16
#define LEAFPRO_CMD_SET_BUSPARAMS_RESP          85

#define LEAFPRO_CMD_CHIP_STATE_EVENT            20
#define LEAFPRO_CMD_START_CHIP_REQ              26
#define LEAFPRO_CMD_START_CHIP_RESP             27

#define LEAFPRO_CMD_LOG_MESSAGE                 106

#define LEAFPRO_CMD_MAP_CHANNEL_REQ             200
#define LEAFPRO_CMD_MAP_CHANNEL_RESP            201


# define LEAFPRO_MSG_FLAG_ERROR_FRAME   0x01
# define LEAFPRO_MSG_FLAG_OVERRUN       0x02
# define LEAFPRO_MSG_FLAG_NERR          0x04
# define LEAFPRO_MSG_FLAG_WAKEUP        0x08
# define LEAFPRO_MSG_FLAG_REMOTE_FRAME  0x10
# define LEAFPRO_MSG_FLAG_EXTENDED      0x20
# define LEAFPRO_MSG_FLAG_TXACK         0x40
# define LEAFPRO_MSG_FLAG_TXRQ          0x80

# define LEAFPRO_EXT_MSG 0x80000000



// Header for every command.
typedef struct {
    UInt8   cmdNo;
    UInt8   address;
    UInt16  transitionId;
} __attribute__ ((packed)) proCmdHead_t;

typedef struct {
    proCmdHead_t    header;
    UInt8           data[28];
} __attribute__ ((packed)) proCmdRaw_t;


typedef struct {
    proCmdHead_t  header;
    UInt8       flags;
} __attribute__ ((packed)) proCmdReadClockReq_t;

typedef struct {
    proCmdHead_t    header;
    char            name[16];
    UInt8           channel;
    UInt8           reserved[11];
} __attribute__ ((packed)) proCmdMapChannelReq_t;

typedef struct {
    proCmdHead_t    header;
    UInt8   heAddress;         // Opaque for the driver.
    UInt8   position;          // Physical "position" in device
    UInt16  flags;             // Various flags, to be defined
    UInt8   reserved1[24];
} __attribute__ ((packed)) proCmdMapChannelResp_t;

typedef struct {
    proCmdHead_t    header;
    UInt32          bitRate;
    UInt8           tseg1;
    UInt8           tseg2;
    UInt8           sjw;
    UInt8           noSamp;
    UInt8           channel;
    UInt8           padding[19];
} __attribute__ ((packed)) proCmdSetBusparamsReq_t;

typedef struct {
    proCmdHead_t    header;
    UInt8   cmdLen;
    UInt8   cmdNo;
    UInt8   channel;
    UInt8   flags;
    UInt16  time[3];
    UInt8   dlc;
    UInt8   padding;
    UInt32  canId;
    UInt8   data[8];
} __attribute__ ((packed)) proCmdLogMessage_t;

typedef union {
    proCmdHead_t proCmdHead;
    proCmdRaw_t proCmdRaw;
    proCmdReadClockReq_t proCmdReadClockReq;
    proCmdMapChannelReq_t proCmdMapChannelReq;
    proCmdMapChannelResp_t proCmdMapChannelResp;
    proCmdSetBusparamsReq_t proCmdSetBusparamsReq;
    proCmdLogMessage_t proCmdLogMessage;
} __attribute__ ((packed)) proCommand_t;


//holds the actual cmd buffer
typedef struct {
    int bufferSize;
    int bufferFirst;
    int bufferCount;
    dispatch_queue_t bufferGDCqueueRef;
    proCommand_t *commandRef;
} LeafProCommandMsgBuf_t;



typedef struct {
    LeafProCommandMsgBuf_t *cmdBufferRef;
    dispatch_semaphore_t semaTimeout;
    UInt8 timeOutReason;
    UInt8 address;
} LeafProPrivateData;


#endif
