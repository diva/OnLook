/*
 @file ndofdev_external.h
 @brief libndofdev main header file for external use.
 
 Copyright (c) 2007, 3Dconnexion, Inc. - All rights reserved.
 
 Redistribution and use in source and binary forms, with or without 
 modification, are permitted provided that the following conditions are met:
 - Redistributions of source code must retain the above copyright notice, 
   this list of conditions and the following disclaimer.
 - Redistributions in binary form must reproduce the above copyright notice, 
   this list of conditions and the following disclaimer in the documentation 
   and/or other materials provided with the distribution.
 - Neither the name of the 3Dconnexion, Inc. nor the names of its contributors 
   may be used to endorse or promote products derived from this software 
   without specific prior written permission.
 
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR 
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
 (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON 
 ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __ndofdev_external_h__
#define __ndofdev_external_h__

#ifdef __cplusplus
extern "C" {
#endif

#define NDOF_MAX_AXES_COUNT         6
#define NDOF_MAX_BUTTONS_COUNT      32

typedef enum NDOF_HotPlugResult {
    NDOF_KEEP_HOTPLUGGED,
    NDOF_DISCARD_HOTPLUGGED
} NDOF_HotPlugResult;

/** Do NOT create NDOF_Device variables manually. Always use ndof_create. */
typedef struct NDOF_Device {
    long axes[NDOF_MAX_AXES_COUNT];           /* axes current values */
    long buttons[NDOF_MAX_BUTTONS_COUNT];     /* buttons current values */
    short axes_count;		/* actual # axes (self sensed on OS X) */
    short btn_count;		/* actual # buttons (self sensed on OS X) */
    long axes_max;			/* logical max */
    long axes_min;			/* logical min */
	unsigned char absolute;	/* if 0 calc delta. On OSX we always have deltas. */
	unsigned char valid;	/* if 0, clients should not access this device. */
	char manufacturer[256]; /* name of device manufacturer */
    char product[256];      /* name of the device */
    void *private_data;     /* ptr to platform specific/private data */
} NDOF_Device;

/** Callback type for new hot-plugged devices. 
 *  Parameters: dev - pointer to the newly added NDOF device. A new NDOF_Device
 *                    struct is allocated for the client. Clients can express
 *                    interest in the device by returning NDOF_KEEP_HOTPLUGGED,
 *                    otherwise the memory pointed by dev will be released. */
typedef NDOF_HotPlugResult (*NDOF_DeviceAddCallback)(NDOF_Device *dev);

/** Removal callback type. Parameter points to the removed device.  */
typedef void (*NDOF_DeviceRemovalCallback)(NDOF_Device *device);

/** Purpose:    Initializes the library. 
 *  Parameters: in_add_cb - Callback invoked when a new	device is hotplugged in.
 *                          Pass NULL if you don't care.
 *              in_removal_cb - Callback invoked when a device is removed.
 *                              Pass NULL if you don't care.
 *              platform_specific - On Windows, it can be a pointer to a already
 *                                  initialized LPDIRECTINPUT8 value; pass NULL 
 *                                  for full initialization. Unused on OS X.
 *  Notes:      The callbacks functionality is currently implemented on Mac OS X
 *              only.  The callbacks functions are currently ignored on Windows.
 *  Returns:    0 if ok. 
 */
extern int ndof_libinit(NDOF_DeviceAddCallback in_add_cb, 
                        NDOF_DeviceRemovalCallback in_removal_cb,
                        void *platform_specific);

/** Purpose:    Clean up.  Must be called before program termination. 
 */
extern void ndof_libcleanup();

/** Purpose:    Allocates memory for a new NDOF_Device struct and initializes 
 *              it with default values. 
 *  Notes:      Always use this function to allocate a new NDOF_Device. 
 *              Call ndof_libcleanup to free the memory. 
 *  Returns:    Pointer to a mallocated struct.
 */
extern NDOF_Device *ndof_create();

/** Purpose:    Get the _first_ NDOF device found in the USB bus. 
 *  Notes:      It will be attempted to use the max and min values of the 
 *              input struct as the range of returned values from the device. 
 *              This may not work for every device. In any case, when this 
 *              function returns, the axes_min, axes_max values will contain 
 *              the actual range.
 *  Parameters: in_out_dev - Must be allocated externally using ndof_create(). 
 *              param - On Windows, you may use this to pass in a 
 *                      LPDIRECTINPUTDEVICE8.  It will be attempted to use
 *                      that instead of creating a new one. In all other cases
 *                      just pass NULL.
 *  Returns:    0 if all is ok, -1 otherwise. 
 */
extern int ndof_init_first(NDOF_Device *in_out_dev, void *param);

/** Purpose:    Reads the current status of the input device. 
 *  Parameters: Must be initialized with ndof_create(). 
 */
extern void ndof_update(NDOF_Device *in_dev);

/** Purpose:    Dumps device info on stderr. */
extern void ndof_dump(NDOF_Device *dev);

/** Purpose:    Dumps list of NDOF devices currently in use on stderr. */
extern void ndof_dump_list();

#if TARGET_OS_MAC
/** Returns the number of connected NDOF devices. Implemented only on OS X. */
extern int ndof_devcount();
#endif

#ifdef __cplusplus
}
#endif
    
#endif /* __ndofdev_external_h__ */
