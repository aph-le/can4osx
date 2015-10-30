//
// can4osx.c
//
// Copyright (c) 2014 - 2015 Alexander Philipp. All rights reserved.
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

#include "can4osx.h"
#include "can4osx_debug.h"
#include "can4osx_internal.h"

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/IOMessage.h>
#include <IOKit/IOCFPlugIn.h>
#include <IOKit/usb/IOUSBLib.h>


#include "kvaserLeaf.h"


Can4osxUsbDeviceHandleEntry can4osxUsbDeviceHandle[CAN4OSX_MAX_CHANNEL_COUNT];

static UInt32 can4osxMaxChannelCount = 0;


static Can4osxUsbDeviceEntry can4osxSupportedDevices[] =
{
    // Vendor Id, Product Id
    {0x0bfd, 0x0120}, //Kvaser Leaf Light v.2
    {0x0bfd, 0x0107}, //Kvaser Leaf Pro HS v.2
    {0x0bfd, 0x000E}, //Kvaser Leaf SemiPro HS
};


// Internal stuff
static IONotificationPortRef can4osxUsbNotificationPortRef = 0;
static io_iterator_t can4osxIoIterator[(sizeof(can4osxSupportedDevices)/sizeof(Can4osxUsbDeviceEntry))];
static dispatch_semaphore_t semaCan4osxStart = NULL;
static dispatch_queue_t queueCan4osx = NULL;


static void CAN4OSX_CanInitializeLibrary (void);
static void CAN4OSX_DeviceAdded(void *refCon, io_iterator_t iterator);
static IOReturn CAN4OSX_ConfigureDevice(IOUSBDeviceInterface **dev);
static IOReturn CAN4OSX_FindInterfaces(Can4osxUsbDeviceHandleEntry *handle);
static void CAN4OSX_DeviceNotification(void *refCon, io_service_t service, natural_t messageType, void *messageArgument);
static CanHandle CAN4OSX_CheckHandle(const CanHandle hnd);
static IOReturn CAN4OSX_CreateEndpointBuffer( const CanHandle hnd );
static IOReturn CAN4OSX_Dealloc(Can4osxUsbDeviceHandleEntry	*self);




//The offical kvaser API
/********************************************************************************/
/**
 * \brief canInitializeLibrary - intializing the driver
 *
 * This function might be called more than once, but at least once to init the
 * driver and internal structures.
 *
 * \return none
 *
 */
void canInitializeLibrary (void)
{
    if (queueCan4osx != NULL) {
        // If the queue already exist, the this function was already called
        return;
    }
    // Create a queue to run in background, so the driver has his own task
    queueCan4osx = dispatch_queue_create("can4osx", NULL);
    semaCan4osxStart = dispatch_semaphore_create(0);
    
    dispatch_set_target_queue(queueCan4osx, dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0));
    //Get a own thread where the usb stuff runs
    dispatch_async(queueCan4osx, ^(void) {
        CAN4OSX_CanInitializeLibrary();
    });
    // Wait here until the background usb task is done
    dispatch_semaphore_wait(semaCan4osxStart, DISPATCH_TIME_FOREVER);
    
    dispatch_release(semaCan4osxStart);
}


/********************************************************************************/
/**
 * \brief canBusOn - enables the CAn bus
 *
 * This function set the CAN bus online
 *
 * \return canStatus
 *
 */
canStatus canBusOn(const CanHandle hnd)
{
    if ( CAN4OSX_CheckHandle(hnd) == -1 ) {
        return canERR_INVHANDLE;
    } else {
        Can4osxUsbDeviceHandleEntry *self = &can4osxUsbDeviceHandle[hnd];
        return self->hwFunctions.can4osxhwCanBusOnRef(hnd);
    }
}


/********************************************************************************/
/**
 * \brief canBusOff - disables the CAn bus
 *
 * This function set the CAN bus offline
 *
 * \return canStatus
 *
 */
canStatus canBusOff(const CanHandle hnd)
{
    if ( CAN4OSX_CheckHandle(hnd) == -1 ) {
        return canERR_INVHANDLE;
    } else {
        Can4osxUsbDeviceHandleEntry *self = &can4osxUsbDeviceHandle[hnd];
        return self->hwFunctions.can4osxhwCanBusOffRef(hnd);
    }
}


/********************************************************************************/
/**
 * \brief canOpenChannel - opens a channel on the interface
 *
 * This function opens a channel on the interface.
 *
 * \return canStatus
 *
 */
CanHandle canOpenChannel(int channel, int flags)
{
    if ( CAN4OSX_CheckHandle(channel) == -1 ) {
        return canERR_NOCHANNELS;
    } else {
        return (CanHandle)channel;
    }
}


canStatus canSetNotify (const CanHandle hnd, CanNotificationType notifyStruct, unsigned int notifyFlags, void *tag)
{
    if ( CAN4OSX_CheckHandle(hnd) == -1 ) {
        return canERR_INVHANDLE;
    } else {
    
        Can4osxUsbDeviceHandleEntry *self = &can4osxUsbDeviceHandle[hnd];
    
        if ( notifyFlags ) {
            CFStringRef temp = self->canNotification.notificationString;
        
            self->canNotification.notifacionCenter = notifyStruct.notifacionCenter;
            self->canNotification.notificationString = CFStringCreateCopy(kCFAllocatorDefault, notifyStruct.notificationString);
        
            if ( temp ) {
                CFRelease(temp);
            }
        } else {
            self->canNotification.notifacionCenter = NULL;
            CFRelease( self->canNotification.notificationString );
        }
        return 0;
    }
}


canStatus canSetBusParams (const CanHandle hnd, SInt32 freq, UInt32 tseg1, UInt32 tseg2, UInt32 sjw, UInt32 noSamp, UInt32 syncmode)
{
    if ( CAN4OSX_CheckHandle(hnd) == -1 ) {
        return canERR_INVHANDLE;
    } else {
        Can4osxUsbDeviceHandleEntry *self = &can4osxUsbDeviceHandle[hnd];
        return self->hwFunctions.can4osxhwCanSetBusParamsRef(hnd, freq, tseg1, tseg2, sjw, noSamp, syncmode);
    }
}


canStatus canSetBusParamsFd(const CanHandle hnd, SInt64 freq_brs, UInt32 tseg1, UInt32 tseg2, UInt32 sjw)
{
    if ( CAN4OSX_CheckHandle(hnd) == -1 ) {
        return canERR_INVHANDLE;
    } else {
        Can4osxUsbDeviceHandleEntry *self = &can4osxUsbDeviceHandle[hnd];
        if (NULL != self->hwFunctions.can4osxhwCanSetBusParamsFdRef) {
            return self->hwFunctions.can4osxhwCanSetBusParamsFdRef(hnd, freq_brs, tseg1, tseg2, sjw);
        } else {
            return canERR_PARAM;
        }
    }
}


canStatus canRead (const CanHandle hnd, UInt32 *id, void *msg, UInt16 *dlc, UInt16 *flag, UInt32 *time)
{
    if ( CAN4OSX_CheckHandle(hnd) == -1 ) {
        return canERR_INVHANDLE;
    } else {
        Can4osxUsbDeviceHandleEntry *pSelf = &can4osxUsbDeviceHandle[hnd];
        return pSelf->hwFunctions.can4osxhwCanReadRef(hnd, id, msg, dlc, flag, time );
    }
}


canStatus canWrite (const CanHandle hnd,UInt32 id, void *msg, UInt16 dlc, UInt16 flag)
{
    if ( CAN4OSX_CheckHandle(hnd) == -1 ) {
        return canERR_INVHANDLE;
    } else {
        Can4osxUsbDeviceHandleEntry *pSelf = &can4osxUsbDeviceHandle[hnd];
        return pSelf->hwFunctions.can4osxhwCanWriteRef(hnd, id, msg, dlc, flag);
    }
}


canStatus canReadStatus	(const CanHandle hnd, UInt32 *const flags)
{
    if ( CAN4OSX_CheckHandle(hnd) == -1 ) {
        return canERR_INVHANDLE;
    } else {
        Can4osxUsbDeviceHandleEntry *self = &can4osxUsbDeviceHandle[hnd];
        
        *flags = 0;
        
        switch ( self->canState.canState ) {
            case CHIPSTAT_ERROR_ACTIVE:
                *flags = canSTAT_ERROR_ACTIVE;
                break;
            case CHIPSTAT_BUSOFF:
                *flags = canSTAT_BUS_OFF;
                break;
            case CHIPSTAT_ERROR_PASSIVE:
                *flags = canSTAT_ERROR_PASSIVE;
                break;
            default:
                break;
        }
        
        return canOK;
    }
}


canStatus canGetChannelData(const CanHandle hnd, SInt32 item, void* pBuffer, size_t bufsize)
{
    if ( CAN4OSX_CheckHandle(hnd) == -1 ) {
        return canERR_INVHANDLE;
    } else {
        Can4osxUsbDeviceHandleEntry *pSelf = &can4osxUsbDeviceHandle[hnd];
        
        if (NULL == pBuffer) {
            return canERR_NOMEM;
        }
        
        if (bufsize <= 0) {
            return canERR_NOMEM;
        }
        
        memset(pBuffer, 0, bufsize);
        
        return CAN4OSX_GetChannelData(pSelf, item, pBuffer, bufsize);
    }
}


// Internal

static void CAN4OSX_CanInitializeLibrary (void)
{
    UInt16 loopCount = 0;
    
    CFMutableDictionaryRef 	can4osxUsbMatchingDictRef;
    CFRunLoopSourceRef		can4osxRunLoopSourceRef;
    CFNumberRef				numberRef;
    
    
    CAN4OSX_DEBUG_PRINT(" Call: %s\n",__func__);
    
    //Set all channels inactive
    for (loopCount = 0; loopCount < CAN4OSX_MAX_CHANNEL_COUNT; loopCount++ ) {
        can4osxUsbDeviceHandle[loopCount].channelNumber = -1;
    }
    
    can4osxUsbNotificationPortRef = IONotificationPortCreate(kIOMasterPortDefault);
    can4osxRunLoopSourceRef = IONotificationPortGetRunLoopSource(can4osxUsbNotificationPortRef);
    
    CFRunLoopAddSource(CFRunLoopGetCurrent(), can4osxRunLoopSourceRef, kCFRunLoopDefaultMode);
    
    
    for ( loopCount = 0; loopCount < (sizeof(can4osxSupportedDevices)/sizeof(Can4osxUsbDeviceEntry)); loopCount++ ) {
        
        can4osxUsbMatchingDictRef = IOServiceMatching(kIOUSBDeviceClassName);
        
        // IOUSBDevice and its subclasses
        if (can4osxUsbMatchingDictRef == NULL) {
            CAN4OSX_DEBUG_PRINT("%s : IOServiceMatching returned NULL.\n",__func__);
            return;
        }
        
        // Create a CFNumber for the idVendor and set the value in the dictionary
        numberRef = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &can4osxSupportedDevices[loopCount].vendorId );
        CFDictionarySetValue(can4osxUsbMatchingDictRef, CFSTR(kUSBVendorID), numberRef);
        CFRelease(numberRef);
        
        // Create a CFNumber for the idProduct and set the value in the dictionary
        numberRef = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &can4osxSupportedDevices[loopCount].productId);
        CFDictionarySetValue(can4osxUsbMatchingDictRef, CFSTR(kUSBProductID), numberRef);
        CFRelease(numberRef);
        
        IOServiceAddMatchingNotification(can4osxUsbNotificationPortRef, kIOFirstMatchNotification, can4osxUsbMatchingDictRef, CAN4OSX_DeviceAdded, NULL, &can4osxIoIterator[loopCount]);
        
        
        numberRef = NULL;
        
        CAN4OSX_DeviceAdded(NULL, can4osxIoIterator[loopCount]);
        
    }
    
    dispatch_semaphore_signal(semaCan4osxStart);
    CFRunLoopRun();
    
    // if the runloop is stopped release
    for ( loopCount = 0; loopCount < (sizeof(can4osxSupportedDevices)/sizeof(Can4osxUsbDeviceEntry)); loopCount++ ) {
        IOObjectRelease(can4osxIoIterator[loopCount]);
    }
    
    IONotificationPortDestroy(can4osxUsbNotificationPortRef);
    
}


/********************************************************************************/
/**
 * \internal
 * \brief CAN4OSX_CheckHandle - checks if the handle is valid
 *
 * This function checks if the given handle is valid
 *
 * \return canStatus
 *
 */
static CanHandle CAN4OSX_CheckHandle(const CanHandle hnd)
{
    if (hnd >= CAN4OSX_MAX_CHANNEL_COUNT) {
        return -1;
    }
    
    if ( can4osxUsbDeviceHandle[hnd].channelNumber == -1 ) {
        return -1;
    }
    
    return hnd;
}


static void CAN4OSX_DeviceAdded(void *refCon, io_iterator_t iterator)
{
    kern_return_t          kernRetVal;
    SInt32                 score;
    HRESULT                result;
    
    io_service_t           can4osxUsbDevice;
    IOCFPlugInInterface  **can4osxPluginInterface = NULL;
    
    while ( (can4osxUsbDevice = IOIteratorNext(iterator) ) ) {

        CAN4OSX_DEBUG_PRINT("%s : Device added\n", __func__);
        
        if (can4osxMaxChannelCount >= CAN4OSX_MAX_CHANNEL_COUNT) {
            CAN4OSX_DEBUG_PRINT("%s : max Channel reached\n", __func__);
            return;
        }
        

        kernRetVal = IOCreatePlugInInterfaceForService(can4osxUsbDevice, kIOUSBDeviceUserClientTypeID, kIOCFPlugInInterfaceID,
                                               &can4osxPluginInterface, &score);
        
        if ((kIOReturnSuccess != kernRetVal) || !can4osxPluginInterface) {
            CAN4OSX_DEBUG_PRINT("%s : IOCreatePlugInInterfaceForService returned 0x%08x.\n",__func__ ,kernRetVal);
            IOObjectRelease(can4osxUsbDevice);
            continue;
        }
        
        // Use the plugin interface to retrieve the device interface.
        result = (*can4osxPluginInterface)->QueryInterface(can4osxPluginInterface, CFUUIDGetUUIDBytes(kIOUSBDeviceInterfaceID),
                                                 (LPVOID*) &can4osxUsbDeviceHandle[can4osxMaxChannelCount].can4osxDeviceInterface);
   
        // Now done with the plugin interface.
        (*can4osxPluginInterface)->Release(can4osxPluginInterface);
        
        if (result || (can4osxUsbDeviceHandle[can4osxMaxChannelCount].can4osxDeviceInterface == NULL) ) {
            CAN4OSX_DEBUG_PRINT("%s : Could not create interface\n", __func__);
            IODestroyPlugInInterface(can4osxPluginInterface);
            IOObjectRelease(can4osxUsbDevice);
            continue;
        }
        
        
        // Open the device to change its state
        kernRetVal = (*can4osxUsbDeviceHandle[can4osxMaxChannelCount].can4osxDeviceInterface)->USBDeviceOpen(can4osxUsbDeviceHandle[can4osxMaxChannelCount].can4osxDeviceInterface);
        if (kernRetVal != kIOReturnSuccess) {
            CAN4OSX_DEBUG_PRINT("%s : Unable to open device: %08x\n", __func__,kernRetVal);
            (void) (*can4osxUsbDeviceHandle[can4osxMaxChannelCount].can4osxDeviceInterface)->Release(can4osxUsbDeviceHandle[can4osxMaxChannelCount].can4osxDeviceInterface);
            IODestroyPlugInInterface(can4osxPluginInterface);
            IOObjectRelease(can4osxUsbDevice);
            continue;
        }
        
        //Configure device
        kernRetVal = CAN4OSX_ConfigureDevice(can4osxUsbDeviceHandle[can4osxMaxChannelCount].can4osxDeviceInterface);
        if (kernRetVal != kIOReturnSuccess) {
            CAN4OSX_DEBUG_PRINT("%s : Unable to configure device: %08x\n", __func__,kernRetVal);
            (void) (*can4osxUsbDeviceHandle[can4osxMaxChannelCount].can4osxDeviceInterface)->USBDeviceClose(can4osxUsbDeviceHandle[can4osxMaxChannelCount].can4osxDeviceInterface);
            (void) (*can4osxUsbDeviceHandle[can4osxMaxChannelCount].can4osxDeviceInterface)->Release(can4osxUsbDeviceHandle[can4osxMaxChannelCount].can4osxDeviceInterface);
            IODestroyPlugInInterface(can4osxPluginInterface);
            IOObjectRelease(can4osxUsbDevice);
            continue;
        }
        
        
        /*kernRetVal = */CAN4OSX_FindInterfaces(&can4osxUsbDeviceHandle[can4osxMaxChannelCount]);
        
        kernRetVal = IOServiceAddInterestNotification(can4osxUsbNotificationPortRef,            // notifyPort
											  can4osxUsbDevice,                                 // service
											  kIOGeneralInterest,                               // interestType
											  CAN4OSX_DeviceNotification,                       // callback
											  &can4osxUsbDeviceHandle[can4osxMaxChannelCount],	// refCon
											  &(can4osxUsbDeviceHandle[can4osxMaxChannelCount].can4osxNotification)	// notification
											  );
        
        if (KERN_SUCCESS != kernRetVal) {
            CAN4OSX_DEBUG_PRINT("%s : IOServiceAddInterestNotification returned 0x%08x.\n",__func__ ,kernRetVal);
        }
        
        can4osxUsbDeviceHandle[can4osxMaxChannelCount].channelNumber = can4osxMaxChannelCount;
        
        // Done with this USB device; release the reference added by IOIteratorNext
        (void)IOObjectRelease(can4osxUsbDevice);
 
        // Set up buffer for sending and receiving
        (void)CAN4OSX_CreateEndpointBuffer(can4osxMaxChannelCount);
        
        can4osxUsbDeviceHandle[can4osxMaxChannelCount].canEventMsgBuff = CAN4OSX_CreateCanEventBuffer(1000);
        
        can4osxUsbDeviceHandle[can4osxMaxChannelCount].endpoitBulkOutBusy = FALSE;
        
        // FIXME
        
        can4osxUsbDeviceHandle[can4osxMaxChannelCount].hwFunctions = leafHardwareFunctions;
        
        can4osxUsbDeviceHandle[can4osxMaxChannelCount].hwFunctions.can4osxhwInitRef(can4osxMaxChannelCount);
        
        can4osxUsbDeviceHandle[can4osxMaxChannelCount].hwFunctions.can4osxhwCanSetBusParamsRef(can4osxMaxChannelCount, canBITRATE_125K, 10, 5, 1, 1, 0);

        can4osxMaxChannelCount++;
        
    }

}


static IOReturn CAN4OSX_ConfigureDevice(IOUSBDeviceInterface **dev)
{
    UInt8 numConfig;
    IOReturn kr;
    IOUSBConfigurationDescriptorPtr configDesc;
 
    /*kr = */(*dev)->GetNumberOfConfigurations(dev, &numConfig);
    if (!numConfig) {
        return -1;
    }
    //Get the configuration descriptor for index 0
    kr = (*dev)->GetConfigurationDescriptorPtr(dev, 0, &configDesc);
    if (kr) {
        CAN4OSX_DEBUG_PRINT("%s : Could not get configuration descriptor for index %d (err = %08x)\n",__func__, 0, (unsigned int)kr);
        return -1;
    }
    //Set the device’s configuration.
    kr = (*dev)->SetConfiguration(dev, configDesc->bConfigurationValue);
    if (kr) {
        CAN4OSX_DEBUG_PRINT("%s : Could not set configuration to value %d (err = %08x)\n",__func__, 0, (unsigned int)kr);
        return -1;
    }
    return kIOReturnSuccess;
}


static IOReturn CAN4OSX_FindInterfaces(Can4osxUsbDeviceHandleEntry *handle)
{
    IOReturn ret, ret2;
    IOUSBFindInterfaceRequest request;
    io_iterator_t iterator;
    io_service_t usbInterface;
    IOCFPlugInInterface **plugInInterface = NULL;
    IOUSBInterfaceInterface **interface = NULL;
    HRESULT result;
    SInt32 score;
    UInt8 interfaceNumEndpoints;
    IOUSBDeviceInterface **device = handle->can4osxDeviceInterface;
    int loopCount = 1;
    
    CFRunLoopSourceRef runLoopSource;
    
    request.bInterfaceClass    = kIOUSBFindInterfaceDontCare;
    request.bInterfaceSubClass = kIOUSBFindInterfaceDontCare;
    request.bInterfaceProtocol = kIOUSBFindInterfaceDontCare;
    request.bAlternateSetting  = kIOUSBFindInterfaceDontCare;
    
    //Get an iterator for the interfaces on the device
    ret = (*device)->CreateInterfaceIterator(device, &request, &iterator);
    
    if ( ret != kIOReturnSuccess ) {
        CAN4OSX_DEBUG_PRINT("%s : Could not create InterfaceIterator\n",__func__);
        return ret;
    }
    
    while ((usbInterface = IOIteratorNext(iterator))) {
        //Create an intermediate plug-in
        ret = IOCreatePlugInInterfaceForService(usbInterface,
                                               kIOUSBInterfaceUserClientTypeID,
                                               kIOCFPlugInInterfaceID,
                                               &plugInInterface, &score);
        //Release the usbInterface object after getting the plug-in
        (void)IOObjectRelease(usbInterface);
        
        if ((ret != kIOReturnSuccess) || !plugInInterface) {
            CAN4OSX_DEBUG_PRINT("%s : Unable to create a plug-in\n", __func__);
            break;
        }
        
        //Now create the device interface for the interface
        result = (*plugInInterface)->QueryInterface(plugInInterface, CFUUIDGetUUIDBytes(kIOUSBInterfaceInterfaceID), (LPVOID *) &interface);
        //No longer need the intermediate plug-in
        (*plugInInterface)->Release(plugInInterface);
        if (result || !interface) {
            CAN4OSX_DEBUG_PRINT("%s : Could not create a device interface for the interface (%08x)\n", __func__,(int) result);
            break;
        }
        
        //Now open the interface. This will cause the pipes associated with
        //the endpoints in the interface descriptor to be instantiated
        ret = (*interface)->USBInterfaceOpen(interface);
        if (ret != kIOReturnSuccess) {
            CAN4OSX_DEBUG_PRINT("%s : Unable to open interface (%08x)\n", __func__,ret);
            (void) (*interface)->Release(interface);
            continue;
        }
        
        //Get the number of endpoints associated with this interface
        ret = (*interface)->GetNumEndpoints(interface, &interfaceNumEndpoints);
        if (ret != kIOReturnSuccess) {
            CAN4OSX_DEBUG_PRINT("%s : Unable to get number of endpoints (%08x)\n",__func__ ,ret);
            (void) (*interface)->USBInterfaceClose(interface);
            (void) (*interface)->Release(interface);
            continue;
        }
        
        CAN4OSX_DEBUG_PRINT("%s : Interface has %d endpoints\n",__func__, interfaceNumEndpoints);

        for (loopCount = 1; loopCount <= interfaceNumEndpoints; loopCount++ ) {
            UInt8 direction;
            UInt8 number;
            UInt8 transferType;
            UInt16 maxPacketSize;
            UInt8 interval;
            
            ret2 = (*interface)->GetPipeProperties(interface, loopCount, &direction, &number, &transferType, &maxPacketSize, &interval);
            
            if (ret2 != kIOReturnSuccess) {
                CAN4OSX_DEBUG_PRINT("%s : Unable to get properties of pipe %d (%08x)\n",__func__ ,loopCount, ret2);
            } else {
                if ( (direction == kUSBOut) && (transferType == kUSBBulk) ) {
                    CAN4OSX_DEBUG_PRINT("%s : Found BulkOut endpoint %d \n",__func__ ,loopCount);
                    handle->endpointNumberBulkOut = loopCount;
                    handle->endpointMaxSizeBulkOut = maxPacketSize;
                }
                
                if ( (direction == kUSBIn) && (transferType == kUSBBulk) ) {
                    CAN4OSX_DEBUG_PRINT("%s : Found BulkIn endpoint %d \n",__func__ ,loopCount);
                    handle->endpointNumberBulkIn = loopCount;
                    handle->endpointMaxSizeBulkIn = maxPacketSize;
                }
            }
        
        }
        
        ret = (*interface)->CreateInterfaceAsyncEventSource(interface, &runLoopSource);
        
        if (ret != kIOReturnSuccess) {
            CAN4OSX_DEBUG_PRINT("%s : Unable to create asynchronous event source (%08x)\n", __func__,ret);
            (void) (*interface)->USBInterfaceClose(interface);
            (void) (*interface)->Release(interface);
            continue;
        }
        CFRunLoopAddSource(CFRunLoopGetCurrent(), runLoopSource, kCFRunLoopDefaultMode);
        CAN4OSX_DEBUG_PRINT("%s : Asynchronous event source added to run loop\n", __func__);
        
        //Save the interface
        handle->can4osxInterfaceInterface = interface;
        
        //Right now only the first interface is supported
        break;
    }
    return ret;
}


static void CAN4OSX_DeviceNotification(void *refCon, io_service_t service, natural_t messageType, void *messageArgument)
{
    Can4osxUsbDeviceHandleEntry	*self = (Can4osxUsbDeviceHandleEntry *) refCon;
    
    if (messageType == kIOMessageServiceIsTerminated) {
        CAN4OSX_DEBUG_PRINT("%s : Device removed. Channel number %d\n",__func__, self->channelNumber);
        
        CAN4OSX_Dealloc(self);

        self->channelNumber = -1;

    }
}


static IOReturn CAN4OSX_CreateEndpointBuffer( const CanHandle hnd )
{
    Can4osxUsbDeviceHandleEntry *self = &can4osxUsbDeviceHandle[hnd];
    
    self->endpointBufferBulkInRef = calloc( 1 , self->endpointMaxSizeBulkIn);

    self->endpointBufferBulkOutRef = calloc( 1 , self->endpointMaxSizeBulkOut);
    
    return kIOReturnSuccess;
}


static IOReturn CAN4OSX_Dealloc(Can4osxUsbDeviceHandleEntry	*self)
{
    kern_return_t retval;
    
    // Release the usb stuff
    
    if (self->can4osxDeviceInterface) {
        /*retval = */(*self->can4osxDeviceInterface)->Release(self->can4osxDeviceInterface);
    }
    
    //if(self->can4osxInterfaceInterface) {
    //    (void)(*self->can4osxInterfaceInterface)->Release(self->can4osxInterfaceInterface);
    //}
    
    
    if(self->endpointBufferBulkInRef) {
        free(self->endpointBufferBulkInRef);
    }
    
    if(self->endpointBufferBulkOutRef) {
        free(self->endpointBufferBulkOutRef);
    }
    
    // Release the notification
    
    retval = IOObjectRelease(self->can4osxNotification);
    
    // FIXME test return value
    if (0) {
        return retval;
    }
    
    // Now release  the dive internal stuff
    
    // FIXME with the channelnumber
    
    self->hwFunctions.can4osxhwCanCloseRef(self->channelNumber);
    
    return retval;
    
}


