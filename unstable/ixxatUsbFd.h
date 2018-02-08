//
//  ixxatUsbFd.h
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
//
#ifndef can4osx_ixxatUsbFd_h
#define can4osx_ixxatUsbFd_h

extern Can4osxHwFunctions ixxUsbFdHardwareFunctions;

#define IXXUSBFD_CMD_BUFFER_SIZE	256

/* reception of 11-bit id messages */
#define IXXATUSBFD_OPMODE_STANDARD         0x01
/* reception of 29-bit id messages */
#define IXXATUSBFD_OPMODE_EXTENDED         0x02
/* enable reception of error frames */
#define IXXATUSBFD_OPMODE_ERRFRAME         0x04
/* listen only mode (TX passive) */
#define IXXATUSBFD_OPMODE_LISTONLY         0x08

/* no extended operation */
#define IXXATUSBFD_EXMODE_DISABLED         0x00
/* extended data length */
#define IXXATUSBFD_EXMODE_EXTDATA          0x01
/* fast data bit rate */
#define IXXATUSBFD_EXMODE_FASTDATA         0x02
/* ISO conform CAN-FD frame */
#define IXXATUSBFD_EXMODE_ISOFD            0x04

#define IXXUSBFD_MSG_FLAG_TYPE   0x000000FF
#define IXXUSBFD_MSG_FLAG_SSM    0x00000100
#define IXXUSBFD_MSG_FLAG_HPM    0x00000200
#define IXXUSBFD_MSG_FLAG_EDL    0x00000400
#define IXXUSBFD_MSG_FLAG_FDR    0x00000800
#define IXXUSBFD_MSG_FLAG_ESI    0x00001000
#define IXXUSBFD_MSG_FLAG_RES    0x0000E000
#define IXXUSBFD_MSG_FLAG_DLC    0x000F0000
#define IXXUSBFD_MSG_FLAG_OVR    0x00100000
#define IXXUSBFD_MSG_FLAG_SRR    0x00200000
#define IXXUSBFD_MSG_FLAG_RTR    0x00400000
#define IXXUSBFD_MSG_FLAG_EXT    0x00800000
#define IXXUSBFD_MSG_FLAG_AFC    0xFF000000

typedef struct {
    UInt32 reqSize;
    UInt16 reqPort;
    UInt16 reqSocket;
    UInt32 reqCode;
} __attribute__ ((packed)) IXXUSBFDMSGREQHEAD_T;

typedef struct {
    UInt32 respSize;
    UInt32 retSize;
    UInt32 retCode;
} __attribute__ ((packed)) IXXUSBFDMSGRESPHEAD_T;


typedef struct {
    IXXUSBFDMSGREQHEAD_T header;
    UInt8  mode;
    UInt8  padding1;
    UInt16 padding2;
} __attribute__ ((packed)) ixxUsbFdDevPowerReq_t;

typedef struct {
	IXXUSBFDMSGRESPHEAD_T header;
} __attribute__ ((packed)) ixxUsbFdDevPowerResp_t;


typedef struct  {
	UInt16 chanCount;
	UInt16 chanTypes[32];
} __attribute__ ((packed)) IXXUSBFDDEVICECAPS_T;

typedef struct  {
	IXXUSBFDMSGREQHEAD_T header;
} __attribute__ ((packed)) IXXUSBFDDEVICECAPSREQ_T;

typedef struct  {
    IXXUSBFDMSGRESPHEAD_T header;
    IXXUSBFDDEVICECAPS_T caps;
} __attribute__ ((packed)) IXXUSBFDDEVICECAPSRESP_T;


typedef struct  {
	UInt32 mode;
	UInt32 bps;
	UInt16 tseg1;
	UInt16 tseg2;
	UInt16 sjw;
	UInt16 tdo;
} __attribute__ ((packed)) IXXUSBFDDCANBTP_T;

typedef struct  {
	IXXUSBFDMSGREQHEAD_T header;
	UInt8 opMode;
	UInt8 exMode;
	IXXUSBFDDCANBTP_T stdBitrate;
    IXXUSBFDDCANBTP_T fdBitrate;
	UInt16 padding;
} __attribute__ ((packed)) IXXUSBFDCANINITREQ_T;

typedef struct  {
    IXXUSBFDMSGRESPHEAD_T header;
} __attribute__ ((packed)) IXXUSBFDCANINITRESP_T;


typedef struct  {
	IXXUSBFDMSGREQHEAD_T header;
} __attribute__ ((packed)) IXXUSBFDCANSTARTREQ_T;

typedef struct  {
    IXXUSBFDMSGRESPHEAD_T header;
    UInt32 startTime;
} __attribute__ ((packed)) IXXUSBFDCANSTARTRESP_T;


typedef struct  {
    IXXUSBFDMSGREQHEAD_T header;
	UInt32 action;
} __attribute__ ((packed)) IXXUSBFDCANSTOPREQ_T;

typedef struct  {
    IXXUSBFDMSGRESPHEAD_T header;
} __attribute__ ((packed)) IXXUSBFDCANSTOPRESP_T;


typedef union {
	IXXUSBFDMSGREQHEAD_T 	reqHeader;
	IXXUSBFDDEVICECAPSREQ_T	respHeader;
} __attribute__ ((packed)) IXXUSBFD_CMD_T;

typedef struct  {
	UInt8 size;
	UInt32 time;
	UInt32 canId;
	UInt32 flags;
	UInt32 clientId;
	UInt8 data[64];
} __attribute__ ((packed)) IXXUSBFDCANMSG_T;



#endif /* can4osx_ixxatUsbFd_h */
