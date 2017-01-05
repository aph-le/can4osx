//
//  can4osx.h
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

#ifndef CAN4OSX_H
# define CAN4OSX_H

#include <CoreFoundation/CoreFoundation.h>

#define CAN4OSX_MAX_CHANNEL_COUNT 5

// KVASER LEAF STUFF


// qqq should this be in the api?
#define VCAN_STAT_OK                 0
#define VCAN_STAT_FAIL              -1    // -EIO
#define VCAN_STAT_TIMEOUT           -2    // -EAGAIN (TIMEDOUT)?
#define VCAN_STAT_NO_DEVICE         -3    // -ENODEV
#define VCAN_STAT_NO_RESOURCES      -4    // -EAGAIN
#define VCAN_STAT_NO_MEMORY         -5    // -ENOMEM
#define VCAN_STAT_SIGNALED          -6    // -ERESTARTSYS
#define VCAN_STAT_BAD_PARAMETER     -7    // -EINVAL


/** Used in \ref canSetBusParams() and \ref canSetBusParamsC200(). Indicate a bitrate of 1 Mbit/s. */
#define canBITRATE_1M        (-1)
/** Used in \ref canSetBusParams() and \ref canSetBusParamsC200(). Indicate a bitrate of 500 kbit/s. */
#define canBITRATE_500K      (-2)
/** Used in \ref canSetBusParams() and \ref canSetBusParamsC200(). Indicate a bitrate of 250 kbit/s. */
#define canBITRATE_250K      (-3)
/** Used in \ref canSetBusParams() and \ref canSetBusParamsC200(). Indicate a bitrate of 125 kbit/s. */
#define canBITRATE_125K      (-4)
/** Used in \ref canSetBusParams() and \ref canSetBusParamsC200(). Indicate a bitrate of 100 kbit/s. */
#define canBITRATE_100K      (-5)
/** Used in \ref canSetBusParams() and \ref canSetBusParamsC200(). Indicate a bitrate of 62 kbit/s. */
#define canBITRATE_62K       (-6)
/** Used in \ref canSetBusParams() and \ref canSetBusParamsC200(). Indicate a bitrate of 50 kbit/s. */
#define canBITRATE_50K       (-7)
/** Used in \ref canSetBusParams() and \ref canSetBusParamsC200(). Indicate a bitrate of 83 kbit/s. */
#define canBITRATE_83K       (-8)
/** Used in \ref canSetBusParams() and \ref canSetBusParamsC200(). Indicate a bitrate of 10 kbit/s. */
#define canBITRATE_10K       (-9)

//
// These are used in the call to canSetNotify().
//
#define canNOTIFY_NONE          0           // Turn notifications off.
#define canNOTIFY_RX            0x0001      // Notify on receive
#define canNOTIFY_TX            0x0002      // Notify on transmit
#define canNOTIFY_ERROR         0x0004      // Notify on error
#define canNOTIFY_STATUS        0x0008      // Notify on (some) status changes
#define canNOTIFY_ENVVAR        0x0010      // Notify on Envvar change


#define canMSG_MASK             0x00ff      // Used to mask the non-info bits
#define canMSG_RTR              0x0001      // Message is a remote request
#define canMSG_STD              0x0002      // Message has a standard ID
#define canMSG_EXT              0x0004      // Message has an extended ID
#define canMSG_WAKEUP           0x0008      // Message to be sent / was received in wakeup mode
#define canMSG_NERR             0x0010      // NERR was active during the message
#define canMSG_ERROR_FRAME      0x0020      // Message is an error frame
#define canMSG_TXACK            0x0040      // Message is a TX ACK (msg is really sent)
#define canMSG_TXRQ             0x0080      // Message is a TX REQUEST (msg is transfered to the chip)

#define canFDMSG_MASK            0xff0000
#define canFDMSG_FDF             0x010000    ///< Message is an FD message (CAN FD)
#define canFDMSG_BRS             0x020000    ///< Message is sent/received with bit rate switch (CAN FD)
#define canFDMSG_ESI             0x040000    ///< Sender of the message is in error passive mode (CAN FD)

#define canSTAT_ERROR_PASSIVE   0x00000001  // The circuit is error passive
#define canSTAT_BUS_OFF         0x00000002  // The circuit is Off Bus
#define canSTAT_ERROR_WARNING   0x00000004  // At least one error counter > 96
#define canSTAT_ERROR_ACTIVE    0x00000008  // The circuit is error active.
#define canSTAT_TX_PENDING      0x00000010  // There are messages pending transmission
#define canSTAT_RX_PENDING      0x00000020  // There are messages in the receive buffer
#define canSTAT_RESERVED_1      0x00000040
#define canSTAT_TXERR           0x00000080  // There has been at least one TX error
#define canSTAT_RXERR           0x00000100  // There has been at least one RX error of some sort
#define canSTAT_HW_OVERRUN      0x00000200  // The has been at least one HW buffer overflow
#define canSTAT_SW_OVERRUN      0x00000400  // The has been at least one SW buffer overflow


#define canOPEN_EXCLUSIVE           0x0008
#define canOPEN_REQUIRE_EXTENDED    0x0010

# define canOPEN_CAN_FD             0x0400


#define canCHANNELDATA_CHANNEL_CAP                1
#define canCHANNELDATA_TRANS_CAP                  2
#define canCHANNELDATA_CHANNEL_FLAGS              3   // available, etc
#define canCHANNELDATA_CARD_TYPE                  4
#define canCHANNELDATA_CARD_NUMBER                5
#define canCHANNELDATA_CHAN_NO_ON_CARD            6
#define canCHANNELDATA_CARD_SERIAL_NO             7
#define canCHANNELDATA_TRANS_SERIAL_NO            8
#define canCHANNELDATA_CARD_FIRMWARE_REV          9
#define canCHANNELDATA_CARD_HARDWARE_REV          10
#define canCHANNELDATA_CARD_UPC_NO                11
#define canCHANNELDATA_TRANS_UPC_NO               12

#define canCHANNELDATA_DEVDESCR_ASCII             26




//
// Don't forget to update canGetErrorText in canlib.c if this is changed!
//
typedef enum {
    canOK                  = 0,
    canERR_PARAM           = -1,     // Error in parameter
    canERR_NOMSG           = -2,     // No messages available
    canERR_NOTFOUND        = -3,     // Specified hw not found
    canERR_NOMEM           = -4,     // Out of memory
    canERR_NOCHANNELS      = -5,     // No channels avaliable
    canERR_INTERRUPTED     = -6,     // Interrupted by signals
    canERR_TIMEOUT         = -7,     // Timeout occurred
    canERR_NOTINITIALIZED  = -8,     // Lib not initialized
    canERR_NOHANDLES       = -9,     // Can't get handle
    canERR_INVHANDLE       = -10,    // Handle is invalid
    canERR_INIFILE         = -11,    // Error in the ini-file (16-bit only)
    canERR_DRIVER          = -12,    // CAN driver type not supported
    canERR_TXBUFOFL        = -13,    // Transmit buffer overflow
    canERR_RESERVED_1      = -14,
    canERR_HARDWARE        = -15,    // Some hardware error has occurred
    canERR_DYNALOAD        = -16,    // Can't find requested DLL
    canERR_DYNALIB         = -17,    // DLL seems to be wrong version
    canERR_DYNAINIT        = -18,    // Error when initializing DLL
    canERR_RESERVED_4      = -19,
    canERR_RESERVED_5      = -20,
    canERR_RESERVED_6      = -21,
    canERR_RESERVED_2      = -22,
    canERR_DRIVERLOAD      = -23,    // Can't find/load driver
    canERR_DRIVERFAILED    = -24,    // DeviceIOControl failed; use Win32 GetLastError()
    canERR_NOCONFIGMGR     = -25,    // Can't find req'd config s/w (e.g. CS/SS)
    canERR_NOCARD          = -26,    // The card was removed or not inserted
    canERR_RESERVED_7      = -27,
    canERR_REGISTRY        = -28,    // Error in the Registry
    canERR_LICENSE         = -29,    // The license is not valid.
    canERR_INTERNAL        = -30,    // Internal error in the driver.
    canERR_NO_ACCESS       = -31,    // Access denied
    canERR_NOT_IMPLEMENTED = -32,    // Requested function is not implemented
    
    // The last entry - a dummy so we know where NOT to place a comma.
    canERR__RESERVED       = -33
} canStatus;



typedef int CanHandle;




/* API functions */
/* The API functions are the canlib API calls from KVASER adapted to OSX */

typedef struct {
    CFNotificationCenterRef notifacionCenter;
    CFStringRef notificationString;
} CanNotificationType;


void canInitializeLibrary (void);

CanHandle canOpenChannel(int channel, int flags);

canStatus canBusOn (const CanHandle hndl);

canStatus canBusOff (const CanHandle hndl);

/* Needed to setup a notififaction to the nofication center */
canStatus canSetNotify (const CanHandle hnd, CanNotificationType notifyStruct, unsigned int notifyFlags, void *tag);

canStatus canSetBusParams (const CanHandle hnd, SInt32 freq, UInt32 tseg1, UInt32 tseg2, UInt32 sjw, UInt32 noSamp, UInt32 syncmode);

canStatus canSetBusParamsFd(const CanHandle hnd, SInt64 freq_brs, UInt32 tseg1, UInt32 tseg2, UInt32 sjw);

canStatus canRead (const CanHandle hnd, UInt32 *id, void *msg, UInt16 *dlc, UInt32 *flag, UInt32 *time);

canStatus canWrite (const CanHandle hnd,UInt32 id, void *msg, UInt16 dlc, UInt32 flag);

canStatus canReadStatus	(const CanHandle hnd, UInt32 *const flags);

canStatus canGetChannelData(const CanHandle hnd, SInt32 item, void* pBuffer, size_t bufsize);

canStatus canGetNumberOfChannels(int *channelCount);

#endif /* CAN4OSX_H */
