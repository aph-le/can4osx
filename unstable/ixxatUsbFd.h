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
} __attribute__ ((packed)) ixxUsbFdMsgRespHead_t;

typedef struct {
    IXXUSBFDMSGREQHEAD_T header;
    UInt8  mode;
    UInt8  padding1;
    UInt16 padding2;
} __attribute__ ((packed)) ixxUsbFdDevPowerReq_t;

typedef struct {
	ixxUsbFdMsgRespHead_t header;
} __attribute__ ((packed)) ixxUsbFdDevPowerResp_t;


typedef struct  {
	UInt16 chanCount;
	UInt16 chanTypes[32];
} __attribute__ ((packed)) IXXUSBFDDEVICECAPS_T;

typedef struct  {
	IXXUSBFDMSGREQHEAD_T header;
} __attribute__ ((packed)) IXXUSBFDDEVICECAPSREQ_T;

typedef struct  {
    ixxUsbFdMsgRespHead_t header;
    IXXUSBFDDEVICECAPS_T caps;
} __attribute__ ((packed)) IXXUSBFDDEVICECAPSRESP_T;

typedef union {
	IXXUSBFDMSGREQHEAD_T reqHeader;
} __attribute__ ((packed)) IXXUSBFD_CMD_T;


#endif /* can4osx_ixxatUsbFd_h */