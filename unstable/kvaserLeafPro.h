//
//  kvaserLeafPro.h
//
//
// Copyright (c) 2014 - 2017 Alexander Philipp. All rights reserved.
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

#ifndef can4osx_kvaserLeafPro_h
#define can4osx_kvaserLeafPro_h

extern Can4osxHwFunctions leafProHardwareFunctions;


# define LEAFPRO_MSG_FLAG_ERROR_FRAME   0x01
# define LEAFPRO_MSG_FLAG_OVERRUN       0x02
# define LEAFPRO_MSG_FLAG_NERR          0x04
# define LEAFPRO_MSG_FLAG_WAKEUP        0x08
# define LEAFPRO_MSG_FLAG_REMOTE_FRAME  0x10
# define LEAFPRO_MSG_FLAG_EXTENDED      0x20
# define LEAFPRO_MSG_FLAG_TXACK         0x40
# define LEAFPRO_MSG_FLAG_TXRQ          0x80

#define LEAFPRO_MSGFLAG_SSM_NACK        0x001000        // Single shot transmission failed.
#define LEAFPRO_MSGFLAG_ABL             0x002000        // Single shot transmission failed due to ArBitration Loss.
#define LEAFPRO_MSGFLAG_FDF             0x010000        // Message is an FD message (CAN FD)
#define LEAFPRO_MSGFLAG_BRS             0x020000        // Message is sent/received with bit rate switch (CAN FD)
#define LEAFPRO_MSGFLAG_ESI             0x040000        // Sender of the message is in error passive mode (CAN FD)

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
    UInt8           reserved;
    UInt8           padding[3];
    UInt32          bitRateFd;
    UInt8           tseg1Fd;
    UInt8           tseg2Fd;
    UInt8           sjwFd;
    UInt8           noSampFd;
    UInt8           open_as_canfd;
    UInt8           padding2[7];
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
    UInt8   data[12];
} __attribute__ ((packed)) proCmdLogMessage_t;

typedef struct {
    proCmdHead_t    header;
    UInt32  canId;
    UInt8   data[8];
    UInt8   dlc;
    UInt8   flags;
    UInt16  transId;
    UInt8   channel;
    UInt8   padding[11];
} __attribute__ ((packed)) proCmdTxMessage_t;


/* CAN FD */
typedef struct {
    proCmdHead_t    header;
    UInt16          len;
    UInt8           cmd;
    UInt8           padding;
} __attribute__ ((packed)) proCmdFdHead_t;

typedef struct {
    proCmdFdHead_t  fdHeader;
    UInt32          flags;
    UInt32          canId;
    UInt32          canId2;
    UInt32          control;
    UInt64          timestamp;
    UInt8           data[64];
} __attribute__ ((packed)) proCmdFdRxMessage_t;

typedef struct {
    proCmdFdHead_t  fdHeader;
    UInt32          flags;
    UInt32          canId;
    UInt32          canId2;
    UInt32          control;
    UInt8           databytes;
    UInt8           dlc;
    UInt8           reserved4[6];
    UInt8           data[64];
} __attribute__ ((packed)) proCmdFdTxMessage_t;

typedef struct {
    proCmdHead_t    header;
    UInt8           useExt;
    UInt8           reserved[27];
} __attribute__ ((packed)) proCmdGetSoftwareDetailsReq_t;


typedef struct {
    proCmdHead_t    header;
    UInt32    swOptions;
    UInt32    swVersion;
    UInt32    swName;
    UInt32    EAN[2];
    UInt32    maxBitrate;
    UInt32    padding[1];
} __attribute__ ((packed)) proCcmdGetSoftwareDetailsResp_t;

typedef struct  {
    UInt8   data[32];
} LeafProRaw_t;


typedef union {
    LeafProRaw_t                    raw;
    proCmdHead_t                    proCmdHead;
    proCmdRaw_t                     proCmdRaw;
    proCmdReadClockReq_t            proCmdReadClockReq;
    proCmdMapChannelReq_t           proCmdMapChannelReq;
    proCmdMapChannelResp_t          proCmdMapChannelResp;
    proCmdSetBusparamsReq_t         proCmdSetBusparamsReq;
    proCmdLogMessage_t              proCmdLogMessage;
    proCmdTxMessage_t               proCmdTxMessage;
    proCmdGetSoftwareDetailsReq_t   proCmdGetSoftwareDetailsReq;
    proCcmdGetSoftwareDetailsResp_t proCcmdGetSoftwareDetailsResp;
} __attribute__ ((packed)) proCommand_t;


typedef union  {
    proCmdFdHead_t      proCmdFdHead;
    proCmdFdRxMessage_t proCmdFdRxMessage;
    proCmdFdTxMessage_t proCmdFdTxMessage;
} __attribute__ ((packed)) proCommandExt_t;


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
    UInt8 canFd;
} LeafProPrivateData_t;

#endif /* can4osx_kvaserLeafPro_h */
