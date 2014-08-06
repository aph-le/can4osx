//
// main.c
// simpleSend
//
// Created by Alexander Philipp on 05.08.14.
// Copyright (c) 2014 Alexander Philipp. All rights reserved.
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
#include <signal.h>
#include <errno.h>
#include <unistd.h>

#include "can4osx.h"


int main(int argc, const char * argv[])
{
    CanHandle hdl;
    int channel;
    int bitrate = canBITRATE_125K;
    int repeat;
    
    extern void CAN4OSX_CanInitializeLibrary(void);
    canInitializeLibrary();
  
    // this sleep is needed to pass time to the driver thread
    sleep(1);
    
    errno = 0;
    
    if (argc < 2 || (channel = atoi(argv[1]), errno) != 0) {
        channel = 0;
    }
    
    if (argc < 3 || (repeat = atoi(argv[2]), errno) != 0) {
        repeat = 1;
    }
    
    
    
    printf("Sending a message on channel %d\n", channel);
    
    
    //Allow signals to interrupt syscalls(e.g in canReadBlock)
    siginterrupt(SIGINT, 1);
    
    hdl = canOpenChannel(channel, canOPEN_EXCLUSIVE | canOPEN_REQUIRE_EXTENDED);
    if (hdl < 0) {
        printf("canOpenChannel %d failed\n", channel);
        return -1;
    }
    
    //canBusOff(hdl);
    
    if (canOK != canSetBusParams(hdl, bitrate, 10, 5, 1, 1, 0)) {
        printf("canSetBusParams channel %d failed\n", channel);
        return -1;
    }
    
//    check("canSetBusOutputControl", canSetBusOutputControl(h, canDRIVER_NORMAL));
    
    if (canOK != canBusOn(hdl)) {
        printf("canBusOn channel %d failed\n", channel);
        return -1;
    }
    
    do {
        if (canOK != canWrite(hdl, 0x123, "can4osx", 8, 0)) {
            printf("canWrite channel %d failed\n", channel);
            return -1;
        }
        usleep(100);
        repeat--;
    }while(repeat);
    
    
    //check("canBusOff", canBusOff(h));
    
//    check("canClose", canClose(h));
    
    return 0;
}

