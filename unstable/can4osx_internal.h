//
//  can4osx_intern.h
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

#ifndef CAN4OSX_INTERN_H
#define CAN4OSX_INTERN_H 1

#include <stdio.h>

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/IOMessage.h>
#include <IOKit/IOCFPlugIn.h>
#include <IOKit/usb/IOUSBLib.h>

#include "can4osx.h"


/* internal buffers */
#define CAN4OSX_CAN_MAX_MSG_LEN 64

#define CAN4OSX_USB_INTERFACE IOUSBInterfaceInterface182

/* Structure for CAN_CHIP_STATE */
#define CHIPSTAT_BUSOFF              0x01
#define CHIPSTAT_ERROR_PASSIVE       0x02
#define CHIPSTAT_ERROR_WARNING       0x04
#define CHIPSTAT_ERROR_ACTIVE        0x08

#define CAN4OSX_GENMASK(h, l) \
	(((~0UL) << (l)) & (~0UL >> ((__SIZEOF_LONG__ * 8) - 1 - (h))))


typedef struct {
	UInt16 productId;
	char *pName;
} CAN4OSX_DEVICE_NAME_T;


typedef struct{
    UInt32 vendorId;
    UInt32 productId;
}CAN4OSX_DEV_ENTRY_T;

typedef struct {
    UInt32 canTimestamp;
    UInt32 canId;
    UInt32  canFlags;
    UInt8  canDlc;
    UInt8  canChannel;
    UInt8  padding;
    UInt8  canData[CAN4OSX_CAN_MAX_MSG_LEN];
} __attribute__ ((packed)) CanMsg;

typedef struct {
    UInt8 chipBusStatus;
    UInt8 chipTxErrorCounter;
    UInt8 chipRxErrorCounter;
} __attribute__ ((packed)) ChipState;

typedef union {
    CanMsg    canMsg;
    ChipState chipState;
} EventTagData;

/* holds the actual buffer */
typedef struct {
	int bufferSize;
	int bufferFirst;
	int bufferCount;
    dispatch_queue_t bufferGDCqueueRef;
	CanMsg *canMsgRef;
} CAN_EVENT_MSG_BUF_T;

typedef struct {
    canStatus (*can4osxhwInitRef) (const CanHandle hnd, UInt16 productId);
    CanHandle (*can4osxhwCanOpenChannel)(int channel, int flags);
    canStatus (*can4osxhwCanBusOnRef) (const CanHandle hndl);
    canStatus (*can4osxhwCanBusOffRef) (const CanHandle hnd);
    canStatus (*can4osxhwCanSetBusParamsRef) (const CanHandle hnd, SInt32 freq, UInt32 tseg1, UInt32 tseg2, UInt32 sjw, UInt32 noSamp, UInt32 syncmode);
    canStatus (*can4osxhwCanSetBusParamsFdRef) (const CanHandle hnd, SInt32 freq, UInt32 tseg1, UInt32 tseg2, UInt32 sjw);
    canStatus (*can4osxhwCanWriteRef) (const CanHandle hnd,UInt32 id, void *msg, UInt16 dlc, UInt32 flag);
    canStatus (*can4osxhwCanReadRef) (const CanHandle hnd, UInt32 *id, void *msg, UInt16 *dlc, UInt32 *flag, UInt32 *time);
    canStatus (*can4osxhwCanCloseRef) (const CanHandle hnd);
}CAN4OSX_HW_FUNC_T;

typedef struct {
   void (*bulkReadCompletion)(void *refCon, IOReturn result, void *arg0);
} CAN4OSX_USB_FUNC_T;


typedef struct {
    UInt8 rxErrorCounter;
    UInt8 txErrorCounter;
    UInt8 canState;
} CAN4OSX_DEV_STATE_T;

typedef struct {
    UInt64 serialNumber;
    UInt32 capability;
    UInt8  deviceString[128];
} CAN4OSX_DEV_INFO_T;


typedef struct {
	IOUSBDeviceInterface182 **can4osxDeviceInterface;
    CAN4OSX_USB_INTERFACE **can4osxInterfaceInterface;
    io_object_t				can4osxNotification;
    
    CAN_EVENT_MSG_BUF_T* canEventMsgBuff;
    
    CanNotificationType     canNotification;
    
    int deviceChannelCount;
    int deviceChannel;
    // virtual channel number
    int channelNumber;
    // BulkIn info/pointer
    int endpointMaxSizeBulkIn;
    int endpointNumberBulkIn;
    char* endpointBufferBulkInRef;
    // BulkOut info/pointer
    int endpointMaxSizeBulkOut;
    int endpointNumberBulkOut;
    UInt8* endpointBufferBulkOutRef;
    bool endpoitBulkOutBusy;
    
    void *privateData; //Here every instace can save private stuff
    
    CAN4OSX_DEV_STATE_T	canState;
    CAN4OSX_DEV_INFO_T	devInfo;
    CAN4OSX_HW_FUNC_T	hwFunctions;
    CAN4OSX_USB_FUNC_T	usbFunctions;
}Can4osxUsbDeviceHandleEntry;



extern Can4osxUsbDeviceHandleEntry can4osxUsbDeviceHandle[CAN4OSX_MAX_CHANNEL_COUNT];


CAN_EVENT_MSG_BUF_T* CAN4OSX_CreateCanEventBuffer( UInt32 bufferSize );
void CAN4OSX_ReleaseCanEventBuffer( CAN_EVENT_MSG_BUF_T* bufferRef );
UInt8 CAN4OSX_WriteCanEventBuffer(CAN_EVENT_MSG_BUF_T* bufferRef, CanMsg newEvent);
UInt8 CAN4OSX_ReadCanEventBuffer(CAN_EVENT_MSG_BUF_T* bufferRef, CanMsg* readEvent);

/* helper functions for all devices */
UInt8 CAN4OSX_decodeFdDlc(UInt8 dlc);
UInt8 CAN4OSX_encodeFdDlc(UInt8 dlc);
UInt64 CAN$OSX_getMilliseconds(void);

canStatus CAN4OSX_GetChannelData(Can4osxUsbDeviceHandleEntry* pSelf, SInt32 cmd, void* pBuffer, size_t bufsize);


#endif /* CAN4OSX_INTERN_H */
