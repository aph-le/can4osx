//
//  kvaserLeaf.h
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

#ifndef can4osx_cmd_kvaserLeaf_h
#define can4osx_cmd_kvaserLeaf_h


# define CMD_RX_STD_MESSAGE                12
# define CMD_TX_STD_MESSAGE                13
# define CMD_RX_EXT_MESSAGE                14
# define CMD_TX_EXT_MESSAGE                15
# define CMD_SET_BUSPARAMS_REQ             16
# define CMD_GET_BUSPARAMS_REQ             17
# define CMD_GET_BUSPARAMS_RESP            18
# define CMD_GET_CHIP_STATE_REQ            19
# define CMD_CHIP_STATE_EVENT              20
# define CMD_SET_DRIVERMODE_REQ            21
# define CMD_GET_DRIVERMODE_REQ            22
# define CMD_GET_DRIVERMODE_RESP           23
# define CMD_RESET_CHIP_REQ                24
# define CMD_RESET_CARD_REQ                25
# define CMD_START_CHIP_REQ                26
# define CMD_START_CHIP_RESP               27
# define CMD_STOP_CHIP_REQ                 28
# define CMD_STOP_CHIP_RESP                29
# define CMD_READ_CLOCK_REQ                30
# define CMD_READ_CLOCK_RESP               31
# define CMD_GET_CARD_INFO_2               32
// 33 may be used
# define CMD_GET_CARD_INFO_REQ             34
# define CMD_GET_CARD_INFO_RESP            35
# define CMD_GET_INTERFACE_INFO_REQ        36
# define CMD_GET_INTERFACE_INFO_RESP       37
# define CMD_GET_SOFTWARE_INFO_REQ         38
# define CMD_GET_SOFTWARE_INFO_RESP        39
# define CMD_GET_BUSLOAD_REQ               40
# define CMD_GET_BUSLOAD_RESP              41
# define CMD_RESET_STATISTICS              42
# define CMD_CHECK_LICENSE_REQ             43
# define CMD_CHECK_LICENSE_RESP            44
# define CMD_ERROR_EVENT                   45
// 46, 47 reserved
# define CMD_FLUSH_QUEUE                   48
# define CMD_RESET_ERROR_COUNTER           49
# define CMD_TX_ACKNOWLEDGE                50
# define CMD_CAN_ERROR_EVENT               51
# define CMD_MEMO_WRITE_SECTOR_REQ                    52
# define CMD_MEMO_WRITE_SECTOR_RESP                   53
# define CMD_MEMO_ERASE_SECTOR_REQ                    54
# define CMD_MEMO_WRITE_CONFIG_REQ                    55
# define CMD_MEMO_WRITE_CONFIG_RESP                   56
# define CMD_MEMO_READ_CONFIG_REQ                     57
# define CMD_MEMO_READ_CONFIG_RESP                    58
# define CMD_MEMO_ERASE_SECTOR_RESP                   59
# define CMD_TX_REQUEST                               60
# define CMD_SET_HEARTBEAT_RATE_REQ                   61
# define CMD_HEARTBEAT_RESP                           62
# define CMD_SET_AUTO_TX_BUFFER                       63
# define CMD_MEMO_GET_FILESYSTEM_INFO_STRUCT_REQ      64
# define CMD_MEMO_GET_FILESYSTEM_INFO_STRUCT_RESP     65
# define CMD_MEMO_GET_DISK_INFO_STRUCT_REQ            66
# define CMD_MEMO_GET_DISK_INFO_STRUCT_RESP           67
# define CMD_MEMO_GET_DISK_HW_INFO_STRUCT_REQ         68
# define CMD_MEMO_GET_DISK_HW_INFO_STRUCT_RESP        69
# define CMD_MEMO_FORMAT_DISK_REQ                     70
# define CMD_MEMO_FORMAT_DISK_RESP                    71
# define CMD_AUTO_TX_BUFFER_REQ                       72
# define CMD_AUTO_TX_BUFFER_RESP                      73
# define CMD_SET_TRANSCEIVER_MODE_REQ                 74
# define CMD_TREF_SOFNR                               75
# define CMD_SOFTSYNC_ONOFF                           76
# define CMD_USB_THROTTLE                             77
# define CMD_SOUND                                    78

// 79 may be used

# define CMD_SELF_TEST_REQ                            80
# define CMD_SELF_TEST_RESP                           81
# define CMD_MEMO_GET_RTC_REQ                         82
# define CMD_MEMO_GET_RTC_RESP                        83
# define CMD_MEMO_SET_RTC_REQ                         84
# define CMD_MEMO_SET_RTC_RESP                        85

# define CMD_SET_IO_PORTS_REQ                         86
# define CMD_GET_IO_PORTS_REQ                         87
# define CMD_GET_IO_PORTS_RESP                        88

# define CMD_MEMO_READ_SECTOR_REQ                     89
# define CMD_MEMO_READ_SECTOR_RESP                    90

# define CMD_GET_TRANSCEIVER_INFO_REQ                 97
# define CMD_GET_TRANSCEIVER_INFO_RESP                98

# define CMD_MEMO_CONFIG_MODE_REQ                     99
# define CMD_MEMO_CONFIG_MODE_RESP                   100

# define CMD_LED_ACTION_REQ                          101
# define CMD_LED_ACTION_RESP                         102

# define CMD_INTERNAL_DUMMY                          103
# define CMD_MEMO_REINIT_DISK_REQ                    104
# define CMD_MEMO_CPLD_PRG                           105
# define CMD_LOG_MESSAGE                             106
# define CMD_LOG_TRIG                                107
# define CMD_LOG_RTC_TIME                            108


# define SOUND_SUBCOMMAND_INIT         0
# define SOUND_SUBCOMMAND_BEEP         1
# define SOUND_SUBCOMMAND_NOTE         2
# define SOUND_SUBCOMMAND_DISABLE      3

# define LED_SUBCOMMAND_ALL_LEDS_ON    0
# define LED_SUBCOMMAND_ALL_LEDS_OFF   1
# define LED_SUBCOMMAND_LED_0_ON       2
# define LED_SUBCOMMAND_LED_0_OFF      3
# define LED_SUBCOMMAND_LED_1_ON       4
# define LED_SUBCOMMAND_LED_1_OFF      5
# define LED_SUBCOMMAND_LED_2_ON       6
# define LED_SUBCOMMAND_LED_2_OFF      7
# define LED_SUBCOMMAND_LED_3_ON       8
# define LED_SUBCOMMAND_LED_3_OFF      9

# define LEAF_MSG_FLAG_REMOTE_FRAME  0x10

# define LEAF_EXT_MSG 0x80000000

# define LEAF_MSG_FLAG_ERROR_FRAME   0x01
# define LEAF_MSG_FLAG_OVERRUN       0x02
# define LEAF_MSG_FLAG_NERR          0x04
# define LEAF_MSG_FLAG_WAKEUP        0x08
# define LEAF_MSG_FLAG_REMOTE_FRAME  0x10
# define LEAF_MSG_FLAG_RESERVED_1    0x20
# define LEAF_MSG_FLAG_TXACK         0x40
# define LEAF_MSG_FLAG_TXRQ          0x80



#define M16C_BUS_RESET    0x01    // Chip is in Reset state
#define M16C_BUS_ERROR    0x10    // Chip has seen a bus error
#define M16C_BUS_PASSIVE  0x20    // Chip is error passive
#define M16C_BUS_OFF      0x40    // Chip is bus off



# define LEAF_TIMEOUT_ONE_MS 1000000
# define LEAF_TIMEOUT_TEN_MS 10*LEAF_TIMEOUT_ONE_MS


// Header for every command.
typedef struct {
    UInt8 cmdLen;  // The length of the whole packet (i.e. including this byte)
    UInt8 cmdNo;
} __attribute__ ((packed)) cmdHead;

typedef struct {
    UInt8  cmdLen;
    UInt8  cmdNo;
    UInt8  channel;
    UInt8  flags;     // MSGFLAG_*
    UInt16 time[3];
    UInt8  dlc;
    UInt8  timeOffset;
    UInt32 ident;        // incl. CAN_IDENT_IS_EXTENDED
    UInt8  data[8];
} __attribute__ ((packed)) cmdLogMessage;

typedef struct {
    UInt8 cmdLen;
    UInt8 cmdNo;
    UInt8 transId;
    UInt8 padding;
} __attribute__ ((packed)) cmdGetSoftwareInfoReq;

typedef struct {
    UInt8  cmdLen;
    UInt8  cmdNo;
    UInt8  transId;
    UInt8  padding0;
    UInt32 swOptions;
    UInt32 firmwareVersion;
    UInt16 maxOutstandingTx;
    UInt16 padding1;      // Currently unused
    UInt32 padding[4];     // Currently unused
} __attribute__ ((packed)) cmdGetSoftwareInfoResp;

typedef struct {
    UInt8 cmdLen;
    UInt8 cmdNo;
    UInt8 transId;
    SInt8 dataLevel;
} __attribute__ ((packed)) cmdGetCardInfoReq;

typedef struct {
    UInt8  cmdLen;
    UInt8  cmdNo;
    UInt8  transId;
    UInt8  channelCount;
    UInt32 serialNumber;
    UInt32 padding1;       // Unused right now
    UInt32 clockResolution;
    UInt32 mfgDate;
    UInt8  EAN[8];         // LSB..MSB, then the check digit.
    UInt8  hwRevision;
    UInt8  usbHsMode;
    UInt8  hwType;         // HWTYPE_xxx (only f/w 1.2 and after)
    UInt8  canTimeStampRef;
} __attribute__ ((packed)) cmdGetCardInfoResp;

typedef struct {
    UInt8  cmdLen;
    UInt8  cmdNo;
    UInt8  transId;
    UInt8  channel;
    UInt32 bitRate;
    UInt8  tseg1;
    UInt8  tseg2;
    UInt8  sjw;
    UInt8  noSamp;
} __attribute__ ((packed)) cmdSetBusparamsReq;

typedef struct {
    UInt8 cmdLen;
    UInt8 cmdNo;
    UInt8 transId;
    UInt8 channel;
} __attribute__ ((packed)) cmdStartChipReq;

typedef struct {
    UInt8  cmdLen;
    UInt8  cmdNo;
    UInt8  channel;
    UInt8  transId;
    UInt8  rawMessage[14];
    UInt8  _padding0;
    UInt8  flags;
} __attribute__ ((packed)) cmdTxCanMessage;

typedef struct {
    UInt8  cmdLen;
    UInt8  cmdNo;
    UInt8  transId;
    UInt8  channel;
    UInt16 time[3];
    UInt8 txErrorCounter;
    UInt8 rxErrorCounter;
    UInt8 busStatus;
    UInt8 padding;
    UInt16 padding2;
} __attribute__ ((packed)) cmdChipStateEvent;



typedef union {
    cmdHead                 head;
    cmdLogMessage           logMessage;
    cmdTxCanMessage         txCanMessage;
    cmdGetCardInfoReq       getCardInfoReq;
    cmdGetCardInfoResp      getCardInfoResp;
    cmdGetSoftwareInfoReq   getSoftwareReq;
    cmdGetSoftwareInfoResp  getSoftwareResp;
    cmdSetBusparamsReq      setBusparamsReq;
    cmdStartChipReq         startChipReq;
    cmdChipStateEvent       chipStateEvent;
} __attribute__ ((packed)) leafCmd;



//holds the actual buffer
typedef struct {
	int bufferSize;
	int bufferFirst;
	int bufferCount;
    dispatch_queue_t bufferGDCqueueRef;
	leafCmd *commandRef;
} LeafCommandMsgBuf;


typedef struct {
    LeafCommandMsgBuf *cmdBufferRef;
    dispatch_semaphore_t semaTimeout;
} LeafPrivateData;


extern Can4osxHwFunctions leafHardwareFunctions;



/* Leaf API */


canStatus LeafCanTranslateBaud (SInt32 *const freq,
                                UInt32 *const tseg1,
                                UInt32 *const tseg2,
                                UInt32 *const sjw,
                                UInt32 *const nosamp,
                                UInt32 *const syncMode);


#endif
