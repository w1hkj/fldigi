/** ***********************************************************************************
 * core_audio.cxx
 *
 * Copyright (C) 2015 Robert Stiles, KK5VD
 *
 * The source code contained in this file is free software: you can redistribute it
 * and/or modify it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This source code is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this source code.  If not, see <http://www.gnu.org/licenses/>.
 **************************************************************************************/

#include "core_audio.h"
#include <sys/time.h>

#if BUILD_FLDIGI
#include "debug.h"
#endif

// Standard Messages
static const char *msg_string[] = {
    (const char *) "No Error",
    (const char *) "Buffer stalled, possible data loss",
    (const char *) "Buffer empty",
    (const char *) "Buffer full",
    (const char *) "Memory Allocation Failure",
    (const char *) "Index out of Range",
    (const char *) "Device Selection Error",
    (const char *) "Parameter(s) Invalid",
    (const char *) "Invalid Handle",
    (const char *) "Internal allocated buffer less then supplied data",
    (const char *) "Initialization failed, internal buffer not available.",
    (const char *) "Device does not support requested I/O direction:",
    (const char *) "Channel index out of range.",
    (const char *) "Unknown IO direction.",
    (const char *) "Device not found:",
    (const char *) "Device contains no channels.",
    (const char *) "AudioObjectGetPropertyDataSize() Error",
    (const char *) "AudioObjectGetPropertyData() Error",
    (const char *) "AudioObjectSetPropertyData Error",
    (const char *) "Operating System function error",
    (const char *) "Playback Device:",
    (const char *) "  Record Device:",
    (const char *) "Invalid/Unknown error number",
    (const char *) 0
};

// Don't call directly.
static int  ca_read_native_buffer(CA_HANDLE handle, int io_device, size_t amount_to_read, size_t *amount_read, void *buffer);
static int  ca_read_native_stereo(CA_HANDLE handle, int io_device, CORE_AUDIO_DATA_TYPE *left, CORE_AUDIO_DATA_TYPE *right);
static int  ca_write_native_buffer(CA_HANDLE handle, int io_device, size_t amount_to_write, size_t *amount_written, void *buffer);
static int  ca_write_native_stereo(CA_HANDLE handle, int io_device, CORE_AUDIO_DATA_TYPE left, CORE_AUDIO_DATA_TYPE right);
static void ca_free_handle_memory(CA_HANDLE handle);

#ifdef __cplusplus
extern "C" {
#endif

    /** ***********************************************************************************
     * \brief Data IO messenger
     *
     * \param inDevice The AudioDevice doing the IO.
     *
     * \param inNow
     *        An AudioTimeStamp that indicates the IO cycle started. Note that this time
     *        includes any scheduling latency that may have been incurred waking the
     *        thread on which IO is being done.
     * \param inInputData
     *        An AudioBufferList containing the input data for the current IO
     *        cycle. For streams that are disabled, the AudioBuffer's mData field will be
     *		  NULL but the mDataByteSize  field will still say how much data would have
     *		  been there if it was enabled. Note that the contents of this structure should
     *		  never be modified.
     * \param inInputTime
     *        An AudioTimeStamp that indicates the time at which the first frame in the
     *        data was acquired from the hardware. If the device has no input streams, the
     *        time stamp will be zeroed out.
     * \param outOutputData
     *        An AudioBufferList in which the output data for the current IO cycle is to
     *        be placed. On entry, each AudioBuffer's mDataByteSize field indicates the
     *        maximum amount of data that can be placed in the buffer and the buffer's
     *        memory has been zeroed out. For formats where the number of bytes per packet
     *        can vary (as with AC-3, for example), the client has to fill out on exit
     *        each mDataByteSize field in each AudioBuffer with the amount of data that
     *        was put in the buffer. Otherwise, the mDataByteSize field should not be
     *        changed. For streams that are disabled, the AudioBuffer's mData field will
     *        be NULL but the mDataByteSize field will still say how much data would have
     *        been there if it was enabled. Except as noted above, the contents of this
     *        structure should not other wise be modified.
     * \param inOutputTime
     *        An AudioTimeStamp that indicates the time at which the first frame in the
     *        data will be passed to the hardware. If the device has no output streams,
     *        the time stamp will be zeroed out.
     * \param inClientData
     *        A pointer to client data established when the AudioDeviceIOProc was
     *        registered with the AudioDevice.
     * \return The return value is currently unused and should always be 0.
     **************************************************************************************/
    OSStatus CADeviceRecord(AudioObjectID inDevice, const AudioTimeStamp * inNow,
                            const AudioBufferList * inInputData, const AudioTimeStamp * inInputTime,
                            AudioBufferList * outOutputData, const AudioTimeStamp * inOutputTime,
                            void * inClientData)
    {
        struct st_core_audio_handle *ca_handle = (struct st_core_audio_handle  *) inClientData;

        if(!inInputData || !ca_handle || ca_handle->pause_flag) return kAudioHardwareNoError;

        pthread_mutex_lock(&ca_handle->io_mutex);

        size_t bufferLoop = inInputData->mNumberBuffers;
        AudioBuffer *data = 0;
        size_t  amt_moved = 0;

        for(UInt32 index = 0; index < bufferLoop; index++) {
            data = (AudioBuffer *) &inInputData->mBuffers[index];
            ca_write_native_buffer(inClientData, AD_RECORD, data->mDataByteSize/sizeof(CORE_AUDIO_DATA_TYPE), &amt_moved, data->mData);

            if(ca_handle->close_flag) {
                data->mDataByteSize = 0;
                pthread_cond_signal(&ca_handle->playback_cond);
            }
        }

        pthread_cond_signal(&ca_handle->record_cond);
        pthread_mutex_unlock(&ca_handle->io_mutex);

        return kAudioHardwareNoError;
    }

    /** ***********************************************************************************
     * \brief See AudioDeviceRecord() Description
     **************************************************************************************/
    OSStatus CADevicePlayback(AudioObjectID inDevice, const AudioTimeStamp * inNow,
                              const AudioBufferList * inInputData, const AudioTimeStamp * inInputTime,
                              AudioBufferList * outOutputData, const AudioTimeStamp * inOutputTime,
                              void * inClientData)
    {
        struct st_core_audio_handle *ca_handle = (struct st_core_audio_handle  *) inClientData;
        size_t used = 0, free = 0;

        if(!outOutputData || !ca_handle || ca_handle->pause_flag) return kAudioHardwareNoError;

        ca_frames(inClientData, AD_PLAYBACK, &used, &free);

        pthread_mutex_lock(&ca_handle->io_mutex);

        if(!used) {
            pthread_cond_wait(&ca_handle->playback_cond, &ca_handle->io_mutex);
        }

        size_t bufferLoop = outOutputData->mNumberBuffers;
        AudioBuffer *data = 0;
        size_t amt_moved  = 0;

        for(size_t index = 0; index < bufferLoop; index++) {
            data = (AudioBuffer *) &outOutputData->mBuffers[index];
            if(!data) break;
            ca_read_native_buffer(inClientData, AD_PLAYBACK, (size_t) data->mDataByteSize/sizeof(CORE_AUDIO_DATA_TYPE), &amt_moved, data->mData);

            data->mDataByteSize = (UInt32) (amt_moved * sizeof(CORE_AUDIO_DATA_TYPE));

            if(ca_handle->close_flag) {
                data->mDataByteSize = 0;
                pthread_cond_signal(&ca_handle->record_cond);
            }
        }

        pthread_mutex_unlock(&ca_handle->io_mutex);

        return kAudioHardwareNoError;
    }

    /** ***********************************************************************************
     * \brief Listen for OS completion states.
     * \param id Audio device ID
     * \param address_count Number of selectors to process.
     * \param paddress selector storage.
     * \param class_ptr used to gain access to the host class data/members.
     * \return OSStatus noErr=Okay, other error.
     **************************************************************************************/
    OSStatus CAPropertyListener(AudioObjectID id, UInt32 address_count,
                                const AudioObjectPropertyAddress paddress[], void *inClientData)
    {
        struct ca_wait_data * wait_data = (struct ca_wait_data  *) inClientData;
        UInt32 index = 0;
        int state = 0;

        if(!wait_data) return noErr;

        for(index = 0; index < address_count; index++) {
            int match = memcmp((void *) &paddress[index], (void *) &wait_data->properties, sizeof(AudioObjectPropertyAddress));
            if((match == 0) && (wait_data->device_id == id)) {
                state = pthread_cond_signal(&wait_data->handle->flag_cond);
                break;
            }
        }

        return noErr;
    }

#ifdef __cplusplus
} //  extern "C" {
#endif


/** ***********************************************************************************
 * \brief Print device list to the console.
 * \return void
 **************************************************************************************/
void ca_print_device_list_to_console(CA_HANDLE handle)
{
    if(!handle) {
        SET_ERROR(0, CA_INVALID_HANDLE);
        return;
    }

    struct st_core_audio_handle *ca_handle = (struct st_core_audio_handle  *) handle;
    struct st_audio_device_list *device_list = ca_handle->device_list;
    size_t device_count = ca_handle->device_list_count;

    printf("\n");
    for(size_t i = 0; i < device_count; i++) {
        if(device_list[i].device_name) {
            if(device_list[i].device_io & AD_RECORD) {
                printf("%u) Record %s\n", (unsigned int) i + 1, device_list[i].device_name);
            }

            if(device_list[i].device_io & AD_PLAYBACK) {
                printf("%u) Play   %s\n", (unsigned int) i + 1, device_list[i].device_name);
            }
        }
    }
    printf("\n");
}

/** ***********************************************************************************
 * \brief Release handle.
 * \return void
 **************************************************************************************/
void ca_release_handle(CA_HANDLE handle)
{
    struct st_core_audio_handle *ca_handle = (struct st_core_audio_handle *) handle;

    if(!handle) {
        SET_ERROR(0, CA_INVALID_HANDLE);
        return;
    }

    ca_close(handle);
    ca_free_handle_memory(handle);

    pthread_mutex_destroy(&ca_handle->io_mutex);
    pthread_cond_destroy(&ca_handle->playback_cond);
    pthread_cond_destroy(&ca_handle->record_cond);

    pthread_mutex_destroy(&ca_handle->flag_mutex);
    pthread_cond_destroy(&ca_handle->flag_cond);

    memset(ca_handle, 0, sizeof(struct st_core_audio_handle));
    free(ca_handle);
}

/** ***********************************************************************************
 * \brief Deallocate handle memory. Not called directly use ca_release_handle().
 * \return void
 **************************************************************************************/
static void ca_free_handle_memory(CA_HANDLE handle)
{
    if(!handle) {
        SET_ERROR(0, CA_INVALID_HANDLE);
        return;
    }

    struct st_core_audio_handle *ca_handle   = (struct st_core_audio_handle *) handle;
    struct st_audio_device_list *device_list = (struct st_audio_device_list *) ca_handle->device_list;
    size_t list_count = ca_handle->device_list_count;

    if(device_list) {
        for(size_t i = 0; i < list_count; i++) {
            if(device_list[i].device_name) free(device_list[i].device_name);
            if(device_list[i].playback)    ca_free_device_attributes(device_list[i].playback);
            if(device_list[i].record)      ca_free_device_attributes(device_list[i].record);
        }
    }

    if(ca_handle->rb_playback) ca_release_ringbuffer(ca_handle->rb_playback);
    if(ca_handle->rb_record)   ca_release_ringbuffer(ca_handle->rb_record);

    return;
}

/** ***********************************************************************************
 * \brief Deallocate device information memory.
 * \return void
 **************************************************************************************/
void ca_free_device_attributes(struct st_audio_device_info *device)
{
    if(!device) {
        SET_ERROR(0, CA_INVALID_PARAMATER);
        return;
    }

    if(device->sample_rates) free(device->sample_rates);

    if(device->channel_desc) {
        for(size_t i = 0; i < device->no_of_channels; i++) {
            if(device->channel_desc[i]) free(device->channel_desc[i]);
        }
        free(device->channel_desc);
    }

    memset(device, 0, sizeof(struct st_audio_device_info));
}


/** ***********************************************************************************
 * \brief Allocate memory, create device list and fill with attributes.
 * \param rb_duration Minimum number seconds of record/playback time for all available devices.
 * \return CA_HANDLE or null value indicating error.
 **************************************************************************************/
CA_HANDLE ca_get_handle(Float64 rb_duration)
{
    CA_HANDLE handle = (CA_HANDLE) 0;

    struct st_core_audio_handle *ca_handle   = (struct st_core_audio_handle *) 0;
    struct st_audio_device_list *device_list = (struct st_audio_device_list *) 0;

    AudioDeviceID *devices   = (AudioDeviceID *)0;
    size_t device_count      = 0;
    size_t record_channels   = 0;
    size_t playback_channels = 0;
    Float64 max_sample_rate  = 0;
    Float64 max_channels     = 0;
    Float64 tmp              = 0;
    size_t  rb_size          = 0;

    size_t no_of_record_devices   = 0;
    size_t no_of_playback_devices = 0;

    int error = CA_NO_ERROR;
    static int run_one_time = 1;
    OSStatus result = noErr;

    if(run_one_time) {
#if defined( AVAILABLE_MAC_OS_X_VERSION_10_6_AND_LATER )
        // This is a largely undocumented but absolutely necessary requirement starting with OSX 10.6.  If not called, queries and
        // updates to various audio device properties are not handled correctly.

        CFRunLoopRef theRunLoop = NULL;
        AudioObjectPropertyAddress property = { kAudioHardwarePropertyRunLoop, kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMaster };
        result = AudioObjectSetPropertyData( kAudioObjectSystemObject, &property, 0, NULL, sizeof(CFRunLoopRef), &theRunLoop);
#endif
        run_one_time = 0;
    }

    ca_handle = calloc(1, sizeof(struct st_core_audio_handle));
    if(!ca_handle) {
        SET_ERROR(0, CA_ALLOCATE_FAIL);
        return (void *)0;
    }

    if(ca_get_device_list(&devices, &device_count)) {
        error = SET_ERROR(0, CA_ALLOCATE_FAIL);
        goto _end;
    }

    if(device_count < 1) {
        error = SET_ERROR(0, CA_INVALID_PARAMATER);
        goto _end;
    }

    device_list = (struct st_audio_device_list *) calloc(device_count + 1, sizeof(struct st_audio_device_list));

    if(!device_list) {
        error = SET_ERROR(0, CA_ALLOCATE_FAIL);
        goto _end;
    }

    ca_handle->device_list = device_list;
    ca_handle->device_list_count = device_count;

    for(size_t i = 0; i < device_count; i++) {
        device_list[i].device_id = devices[i];
        device_list[i].list_index_pos = i;
        device_list[i].device_name = ca_get_device_name((CA_HANDLE) ca_handle, devices[i]);
        ca_get_no_of_channels((CA_HANDLE) ca_handle, devices[i], &record_channels, &playback_channels);
        ca_get_device_attributes((CA_HANDLE) ca_handle, devices[i], &device_list[i], record_channels, playback_channels);

        // Keep track of the number of in/out devices.
        if(record_channels)
            no_of_record_devices++;

        if(playback_channels)
            no_of_playback_devices++;

        // Determine ring buffer memory requirements.
        if(max_channels < record_channels)
            max_channels = record_channels;

        if(max_channels < playback_channels)
            max_channels = playback_channels;

        tmp = ca_get_max_sample_rate((CA_HANDLE) ca_handle, &device_list[i]);
        if(tmp > max_sample_rate)
            max_sample_rate = tmp;
    }

    // Ring buffer allocated once on a per handle basis.
    rb_size = (size_t) (max_channels * max_sample_rate * rb_duration);

    ca_handle->rb_record   = ca_init_ringbuffer((CA_HANDLE) ca_handle, rb_size);
    ca_handle->rb_playback = ca_init_ringbuffer((CA_HANDLE) ca_handle, rb_size);

    if(!ca_handle->rb_record || !ca_handle->rb_playback) {
        error = SET_ERROR(0, CA_ALLOCATE_FAIL);
        goto _end;
    }

    pthread_mutex_init(&ca_handle->io_mutex,     NULL);
    pthread_cond_init(&ca_handle->playback_cond, NULL);
    pthread_cond_init(&ca_handle->record_cond,   NULL);

    pthread_mutex_init(&ca_handle->flag_mutex,   NULL);
    pthread_cond_init(&ca_handle->flag_cond,     NULL);

_end:;

    if(error != CA_NO_ERROR) {
        ca_free_handle_memory((CA_HANDLE) ca_handle);
    } else {
        handle = (CA_HANDLE) ca_handle;
    }

    return handle;
}

/** ***********************************************************************************
 * \brief Open (start) playback and record audio stream.
 * \return 0 Okay, otherwise error.
 **************************************************************************************/
int ca_open(CA_HANDLE handle)
{
    OSStatus result = noErr;
    AudioDeviceID device_id = 0;

    if(!handle) {
        return SET_ERROR(0, CA_INVALID_HANDLE);
    }

    struct st_core_audio_handle *ca_handle = (struct st_core_audio_handle *) handle;

    if(ca_handle->running_flag)
        ca_close(handle);

    ca_handle->close_flag   = 0;
    ca_handle->pause_flag   = 0;
    ca_handle->running_flag = 0;

    if(ca_handle->selected_playback_device && (ca_handle->selected_playback_device->device_io & AD_PLAYBACK)) {

        // Initialize the ring buffer and ensure buffer is frame size aligned.
        ca_handle->rb_playback->channels_per_frame  = ca_handle->selected_playback_device->playback->no_of_channels;
        ca_handle->rb_playback->frame_count         = 0;
        ca_handle->rb_playback->frame_size          = ca_handle->rb_playback->buffer_size / (ca_handle->rb_playback->channels_per_frame * sizeof(CORE_AUDIO_DATA_TYPE));
        ca_handle->rb_playback->unit_count          = 0;
        ca_handle->rb_playback->unit_size           = ca_handle->rb_playback->frame_size * ca_handle->rb_playback->channels_per_frame;
        ca_handle->rb_playback->write_pos           = 0;
        ca_handle->rb_playback->read_pos            = 0;

        ca_map_device_channels(handle, AD_PLAYBACK, CA_LEFT_CHANNEL_INDEX, CA_RIGHT_CHANNEL_INDEX);

        device_id = ca_handle->selected_playback_device->device_id;

#if defined( MAC_OS_X_VERSION_10_5 ) && ( MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_5 )
        result = AudioDeviceCreateIOProcID(device_id, CADevicePlayback, handle,  &ca_handle->selected_playback_device->io_proc_id);
#endif
        if(result != noErr) return 1;

        ca_handle->playback_callback = CADevicePlayback;

    }

    if(ca_handle->selected_record_device && (ca_handle->selected_record_device->device_io & AD_RECORD)) {
        device_id = ca_handle->selected_record_device->device_id;

        // Initialize the ring buffer and ensure buffer is frame size aligned.
        ca_handle->rb_record->channels_per_frame  = ca_handle->selected_record_device->record->no_of_channels;
        ca_handle->rb_record->frame_count         = 0;
        ca_handle->rb_record->frame_size          = ca_handle->rb_record->buffer_size / (ca_handle->rb_record->channels_per_frame * sizeof(CORE_AUDIO_DATA_TYPE));
        ca_handle->rb_record->unit_count          = 0;
        ca_handle->rb_record->unit_size           = ca_handle->rb_record->frame_size * ca_handle->rb_record->channels_per_frame;
        ca_handle->rb_record->write_pos           = 0;
        ca_handle->rb_record->read_pos            = 0;

        ca_map_device_channels(handle, AD_RECORD, CA_LEFT_CHANNEL_INDEX, CA_RIGHT_CHANNEL_INDEX);

#if defined( MAC_OS_X_VERSION_10_5 ) && ( MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_5 )
        result = AudioDeviceCreateIOProcID(device_id, CADeviceRecord, handle,  &ca_handle->selected_record_device->io_proc_id);
#endif
        if(result != noErr) return 1;

        ca_handle->record_callback = CADeviceRecord;
    }

    ca_handle->closed_flag  = 0;
    ca_handle->okay_to_run = 1;

    return CA_NO_ERROR;
}

/** ***********************************************************************************
 * \brief Start audio device data transfer.
 * \param handle Core Audio Handle
 * \return 0 Okay, otherwise error
 **************************************************************************************/
int ca_run(CA_HANDLE handle)
{
    OSStatus result = noErr;

    if(handle) {
        struct st_core_audio_handle *ca_handle = (struct st_core_audio_handle *) handle;

#if defined( MAC_OS_X_VERSION_10_5 ) && ( MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_5 )
        if(ca_handle->selected_record_device) {
            result |= AudioDeviceStart(ca_handle->selected_record_device->device_id, ca_handle->selected_record_device->io_proc_id);
        }

        if(ca_handle->selected_playback_device) {
            result |= AudioDeviceStart(ca_handle->selected_playback_device->device_id, ca_handle->selected_playback_device->io_proc_id);
        }
#endif
        ca_handle->running_flag = 1;
    }



    return (result ? CA_OS_ERROR : CA_NO_ERROR);
}

/** ***********************************************************************************
 * \brief Close playback and record audio stream.
 * \return 0 Okay, otherwise error.
 **************************************************************************************/
int ca_close(CA_HANDLE handle)
{
    if(!handle) {
        return SET_ERROR(0, CA_INVALID_HANDLE);
    }

    struct st_core_audio_handle *ca_handle = (struct st_core_audio_handle *) handle;
    OSStatus result = noErr;


    ca_handle->pause_flag   = 0;
    ca_handle->close_flag   = 1;
    ca_handle->running_flag = 0;

    pthread_cond_signal(&ca_handle->record_cond);
    pthread_cond_signal(&ca_handle->playback_cond);
    pthread_cond_signal(&ca_handle->flag_cond);

    if(ca_handle->closed_flag)
        return CA_NO_ERROR;

#if defined( MAC_OS_X_VERSION_10_5 ) && ( MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_5 )
    if(ca_handle->playback_callback) {
        result |= AudioDeviceStop(ca_handle->selected_playback_device->device_id, ca_handle->selected_playback_device->io_proc_id);
        result |= AudioDeviceDestroyIOProcID(ca_handle->selected_playback_device->device_id, ca_handle->selected_playback_device->io_proc_id);
        ca_handle->playback_callback = 0;
    }

    if(ca_handle->record_callback) {
        result |= AudioDeviceStop(ca_handle->selected_record_device->device_id, ca_handle->selected_record_device->io_proc_id);
        result |= AudioDeviceDestroyIOProcID(ca_handle->selected_record_device->device_id, ca_handle->selected_record_device->io_proc_id);
        ca_handle->record_callback = 0;
    }
#endif

    ca_handle->pause_flag   = 0;
    ca_handle->close_flag   = 0;
    ca_handle->running_flag = 0;
    ca_handle->closed_flag  = 1;

    return (result ? CA_OS_ERROR : CA_NO_ERROR);
}

/** ***********************************************************************************
 * \brief Select an audio device by index.
 * \param handle Core Audio Handle.
 * \param index Audio device index, 1...device_count.
 * \param device_io Record or Playback device.
 * \return 0 Error, otherwise Device ID No.
 **************************************************************************************/
AudioDeviceID ca_select_device_by_index(CA_HANDLE handle, int index, int device_io)
{
    if(!handle) {
        SET_ERROR(0, CA_INVALID_HANDLE);
        return 0;
    }

    struct st_core_audio_handle *ca_handle = (struct st_core_audio_handle *) handle;
    AudioDeviceID device_id = 0;

    if(index > 0) index--;

    if(index < 0 || index >= ca_handle->device_list_count) {
        SET_ERROR(handle, CA_INDEX_OUT_OF_RANGE);
        return 0;
    }

    if((ca_handle->device_list[index].device_io & device_io) == 0) {
        ca_print_msg(handle, CA_INVALID_DIRECTION, ca_handle->device_list[index].device_name);
        return 0;
    }

    if(device_io & AD_PLAYBACK) {
        ca_handle->selected_playback_device = &ca_handle->device_list[index];
        device_id = ca_handle->selected_playback_device->device_id;
        ca_print_msg(handle, CA_PLAYBACK_DEVICE_SELECTED, ca_handle->selected_playback_device->device_name);

    } else if(device_io & AD_RECORD) {
        ca_handle->selected_record_device = &ca_handle->device_list[index];
        device_id = ca_handle->selected_record_device->device_id;
        ca_print_msg(handle, CA_RECORD_DEVICE_SELECTED, ca_handle->selected_record_device->device_name);
    }

    return device_id;
}

/** ***********************************************************************************
 * \brief Select an audio device by name.
 * \param handle Core Audio Handle.
 * \param device_name Device name (Format: device_name:device_id)
 * \param device_io Record or Playback device.
 * \return 0 Error, otherwise Device ID No.
 **************************************************************************************/
AudioDeviceID ca_select_device_by_name(CA_HANDLE handle, char *device_name, int device_io)
{
    if(!handle) {
        SET_ERROR(0, CA_INVALID_HANDLE);
        return 0;
    }

    if(!device_name) {
        SET_ERROR(0, CA_INVALID_PARAMATER);
        return 0;
    }

    struct st_core_audio_handle *ca_handle = (struct st_core_audio_handle *) handle;
    struct st_audio_device_list * list = ca_handle->device_list;
    size_t count = ca_handle->device_list_count;
    int match = 0;
    int device_found = 0;
    AudioDeviceID device_id = 0;

    if(count && list) {
        for(size_t i = 0; i < count; i++) {
            match = ca_string_match((const char *) device_name, (const char *) list[i].device_name, (size_t) AUDIO_DEVICE_NAME_LIMIT);
            if(match == 0) {
                if(device_io & AD_PLAYBACK & list[i].device_io) {
                    ca_handle->selected_playback_device = &list[i];
                    device_found = 1;
                    device_id = ca_handle->selected_playback_device->device_id;
                    ca_print_msg(handle, CA_PLAYBACK_DEVICE_SELECTED, ca_handle->selected_playback_device->device_name);
                    break;
                }

                if(device_io & AD_RECORD & list[i].device_io) {
                    ca_handle->selected_record_device = &list[i];
                    device_found = 1;
                    device_id = ca_handle->selected_record_device->device_id;
                    ca_print_msg(handle, CA_RECORD_DEVICE_SELECTED, ca_handle->selected_record_device->device_name);
                    break;
                }
            }
        }

    } else {
        SET_ERROR(handle, CA_DEVICE_SELECT_ERROR);
        return 0;
    }

    if(!device_found) {
        ca_print_msg(handle, CA_DEVICE_NOT_FOUND, device_name);
        return 0;
    }

    return device_id;
}


/** ***********************************************************************************
 * \brief Set the run state of record/playback audio streams.
 * \return 0 Okay, otherwise error.
 **************************************************************************************/
int ca_set_run_state(CA_HANDLE handle, enum run_pause flag)
{
    if(!handle) {
        return SET_ERROR(0, CA_INVALID_HANDLE);
    }

    struct st_core_audio_handle *ca_handle = (struct st_core_audio_handle *) handle;

    switch(flag) {
        case RS_RUN:
        case RS_PAUSE:
            break;
        default:
            return SET_ERROR(handle, CA_INVALID_PARAMATER);
    }

    ca_handle->pause_flag = flag;

    return CA_NO_ERROR;
}

/** ***********************************************************************************
 * \brief Install a property listener for the specified device.
 * \return 0 Okay, otherwise error.
 **************************************************************************************/
int ca_start_listener(CA_HANDLE handle, struct ca_wait_data *wait_data)
{
    if(!handle) {
        return SET_ERROR(0, CA_INVALID_HANDLE);
    }

    if(!wait_data)
        return SET_ERROR(0, CA_INVALID_PARAMATER);

    struct st_core_audio_handle *ca_handle = (struct st_core_audio_handle *) handle;
    OSStatus results = noErr;
    int error = CA_NO_ERROR;

    results = AudioObjectAddPropertyListener(wait_data->device_id, &wait_data->properties,
                                             CAPropertyListener, (void *) wait_data);
    if(results == noErr)
        ca_handle->property_listener = CAPropertyListener;
    else
        error = CA_OS_ERROR;

    return error;
}

/** ***********************************************************************************
 * \brief Remove installed property listener for the specified device.
 * \return 0 Okay, otherwise error.
 **************************************************************************************/
int ca_remove_listener(CA_HANDLE handle, struct ca_wait_data *wait_data)
{
    if(!handle) {
        return SET_ERROR(0, CA_INVALID_HANDLE);
    }

    if(!wait_data)
        return SET_ERROR(0, CA_INVALID_PARAMATER);

    struct st_core_audio_handle *ca_handle = (struct st_core_audio_handle *) handle;
    OSStatus results = noErr;
    int error = CA_NO_ERROR;

    if(ca_handle->property_listener) {
        results = AudioObjectRemovePropertyListener(wait_data->device_id, &wait_data->properties,
                                                    ca_handle->property_listener, (void *) wait_data);
        if(results != noErr)
            error = CA_OS_ERROR;

        ca_handle->property_listener = (AudioObjectPropertyListenerProc) 0;
    }

    return error;
}

/** ***********************************************************************************
 * \brief Return default devices
 * \return 0 Okay, otherwise error.
 **************************************************************************************/
int ca_get_default_devices(AudioDeviceID *record, AudioDeviceID *playback)
{
    if(!record || !playback)
        return SET_ERROR(0, CA_INVALID_PARAMATER);

    *record = *playback = 0;

    AudioObjectPropertyAddress property = {
        kAudioHardwarePropertyDefaultOutputDevice,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMaster
    };

    AudioDeviceID device_id = 0;
    UInt32 property_size = sizeof(AudioDeviceID);
    OSStatus os_error = noErr;

    os_error = AudioObjectGetPropertyData(device_id, &property, 0, NULL, &property_size, &device_id);

    if(os_error == noErr)
        *playback = device_id;
    else
        return SET_ERROR(0, CA_GET_PROPERTY_DATA_ERROR);

    property.mSelector = kAudioHardwarePropertyDefaultInputDevice;

    os_error = AudioObjectGetPropertyData(device_id, &property, 0, NULL, &property_size, &device_id);

    if(os_error == noErr)
        *record = device_id;
    else
        return SET_ERROR(0, CA_GET_PROPERTY_DATA_ERROR);

    return CA_NO_ERROR;
}

/** ***********************************************************************************
 * \brief Return highest samplerate for the given device.
 * \param handle Handle
 * \param list Device list entry.
 * \return Highest sample rate, 0.0 error.
 **************************************************************************************/
Float64 ca_get_max_sample_rate(CA_HANDLE handle, struct st_audio_device_list * list)
{
    if(!handle) {
        SET_ERROR(0, CA_INVALID_HANDLE);
        return 0.0;
    }

    Float64 max_sample_rate = 0.0;
    Float64 *sr_list = (Float64 *)0;
    size_t count = 0;
    struct st_audio_device_info *device = (struct st_audio_device_info *)0;

    if(!list) return 0.0;

    // PLAYBACK
    if(list->playback) {
        device = list->playback;
        if(device && device->sample_rates && device->sample_rate_count) {
            count = device->sample_rate_count;
            sr_list = device->sample_rates;

            for(size_t i = 0; i < count; i++) {
                if(max_sample_rate < sr_list[i])
                    max_sample_rate = sr_list[i];
            }
        }
    }

    // RECORD
    if(list->record) {
        device = list->record;
        if(device && device->sample_rates && device->sample_rate_count) {
            count = device->sample_rate_count;
            sr_list = device->sample_rates;

            for(size_t i = 0; i < count; i++) {
                if(max_sample_rate < sr_list[i])
                    max_sample_rate = sr_list[i];
            }
        }
    }

    return max_sample_rate;
}

/** ***********************************************************************************
 * \brief Get a list of device id's.
 * \param array_of_ids Storage for the list of id's (malloc)
 * \param count Number of entries in the array.
 * \return 0 Okay, otherwise error.
 **************************************************************************************/
int ca_get_device_list(AudioDeviceID **array_of_ids, size_t *count)
{
    UInt32 property_size = sizeof(AudioDeviceID);
    OSStatus result = noErr;
    AudioDeviceID *device_list = 0;
    size_t list_count = 0;

    AudioObjectPropertyAddress property = { kAudioHardwarePropertyDevices,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMaster
    };

    if(!array_of_ids || !count)
        return  SET_ERROR(0, CA_INVALID_PARAMATER);

    *count = 0;
    *array_of_ids = (AudioDeviceID) 0;

    result = AudioObjectGetPropertyDataSize(kAudioObjectSystemObject, &property, 0, NULL, &property_size);

    if(result != noErr)
        return  SET_ERROR(0, CA_GET_PROPERTY_DATA_SIZE_ERROR);

    list_count  = property_size / sizeof(AudioDeviceID);
    device_list = (AudioDeviceID *) calloc(list_count + 1, sizeof(AudioDeviceID));

    if(!device_list)
        return SET_ERROR(0, CA_ALLOCATE_FAIL);

    result = AudioObjectGetPropertyData(kAudioObjectSystemObject, &property, 0, NULL, &property_size, device_list);

    if(result != noErr) {
        free(device_list);
        return  SET_ERROR(0, CA_GET_PROPERTY_DATA_ERROR);
    }

    *array_of_ids = device_list;
    *count = list_count;

    return CA_NO_ERROR;
}

/** ***********************************************************************************
 * \brief Allocate memory and assign device channels names.
 * \param id Audio device ID
 * \param array A place to store data.
 * \param no_of_channels Number of channels.
 * \param direction Record/Playback.
 * \return 0 Okay, otherwise error.
 **************************************************************************************/
int ca_get_device_channel_names(CA_HANDLE handle, AudioDeviceID id, char ** array, size_t no_of_channels, size_t direction)
{
    if(!handle) {
        return SET_ERROR(0, CA_INVALID_HANDLE);
    }

    if(!array || !no_of_channels)
        return SET_ERROR(handle, CA_INVALID_PARAMATER);

    switch(direction) {
        case AD_PLAYBACK:
        case AD_RECORD:
            break;
        default:
            return SET_ERROR(handle, CA_INVALID_PARAMATER);
    }

    char *name = (char *) 0;
    char **tmp = (char **) calloc(no_of_channels + 1, sizeof(char *));

    if(!tmp)
        return SET_ERROR(handle, CA_ALLOCATE_FAIL);

    for(size_t i = 0; i < no_of_channels; i++) {
        name = ca_get_device_channel_name(handle, id, i+1, no_of_channels, direction);
        if(!name) break;
        tmp[i] = name;
    }

    *array = (char *) tmp;

    return CA_NO_ERROR;
}

/** ***********************************************************************************
 * \brief Allocate memory and fill with attributes.
 * \param id Audio device ID
 * \param list A place to store data.
 * \param no_of_playback_channels Number of playback channels.
 * \param no_of_record_channels Number of record channels.
 * \return 0 Okay, otherwise error.
 **************************************************************************************/
int ca_get_device_attributes(CA_HANDLE handle, AudioDeviceID id, struct st_audio_device_list *list, size_t no_of_record_channels, size_t no_of_playback_channels)
{
    if(!handle) {
        return SET_ERROR(0, CA_INVALID_HANDLE);
    }

    struct st_audio_device_info *device_info = (struct st_audio_device_info  *) 0;

    if(no_of_playback_channels) {
        device_info = (struct st_audio_device_info *) calloc(1, sizeof(struct st_audio_device_info));

        if(!device_info)
            return SET_ERROR(handle, CA_ALLOCATE_FAIL);

        ca_get_sample_rates(handle, id, &device_info->sample_rates, &device_info->sample_rate_count);

        ca_get_device_channel_names(handle, id, (char **) &device_info->channel_desc, no_of_playback_channels, AD_PLAYBACK);

        device_info->no_of_channels = no_of_playback_channels;
        list->playback = device_info;
        list->playback->parent = list;
        list->device_io |= AD_PLAYBACK;
    }

    if(no_of_record_channels) {
        device_info = (struct st_audio_device_info *) calloc(1, sizeof(struct st_audio_device_info));

        if(!device_info)
            return SET_ERROR(handle, CA_ALLOCATE_FAIL);

        ca_get_sample_rates(handle, id, &device_info->sample_rates, &device_info->sample_rate_count);

        ca_get_device_channel_names(handle, id, (char **) &device_info->channel_desc, no_of_record_channels, AD_RECORD);

        device_info->no_of_channels = no_of_record_channels;
        list->record = device_info;
        list->record->parent = list;
        list->device_io |= AD_RECORD;
    }

    return CA_NO_ERROR;
}

/** ***********************************************************************************
 * \brief Return the name of the specified device and append OS device ID number.
 * \param device_id OS ID of the device in question.
 * \return Allocated char string pointer for the device name. Use free() to deallocate.
 * Null pointer indicates error.
 **************************************************************************************/
char * ca_get_device_name(CA_HANDLE handle, AudioDeviceID device_id)
{
    if(!handle) {
        SET_ERROR(0, CA_INVALID_HANDLE);
        return (char *)0;
    }

    OSStatus  result = noErr;
    CFStringRef cfs_device_name;
    UInt32 prop_size = sizeof(CFStringRef);
    char *tmp1 = (char *)0;

    AudioObjectPropertyAddress pa = { kAudioObjectPropertyName,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMaster
    };

    result = AudioObjectGetPropertyData(device_id, &pa, 0, NULL, &prop_size, &cfs_device_name);

    if (result) {
        SET_ERROR(handle, CA_GET_PROPERTY_DATA_ERROR);
        return (char *)0;
    }

    // Allocate device name storage.
    tmp1 = (char *) calloc(AUDIO_DEVICE_NAME_LIMIT + 1, sizeof(char));

    if(!tmp1) {
        SET_ERROR(handle, CA_ALLOCATE_FAIL);
        return (char *)0;
    }

    CFStringGetCString(cfs_device_name, tmp1, AUDIO_DEVICE_NAME_LIMIT, kCFStringEncodingASCII);
    CFRelease(cfs_device_name);

    return tmp1;
}

/** ***********************************************************************************
 * \brief Return the name of the specified channel.
 * \param device struct st_audio_device_info *device internal device info.
 * \param channel_index Channel Index (left, right,...)
 * \param direction Record/Playback
 * \return Allocated char string pointer for the device channel. Use free() to deallocate.
 * Null pointer indicates error.
 **************************************************************************************/
char * ca_get_device_channel_name(CA_HANDLE handle, AudioDeviceID device_id, size_t channel_index, size_t max_channels, size_t direction)
{
    if(!handle) {
        SET_ERROR(0, CA_INVALID_HANDLE);
        return (char *)0;
    }

    OSStatus  result = noErr;
    CFStringRef cfs_channel_name;
    UInt32 propSize = sizeof(CFStringRef);
    char *channel_name = (char *)0;

    AudioObjectPropertyAddress pa = { 0, 0, kAudioObjectPropertyElementMaster };

    pa.mSelector = kAudioObjectPropertyElementName;

    switch(direction) {
        case AD_PLAYBACK:
            pa.mScope = kAudioDevicePropertyScopeOutput;
            break;

        case AD_RECORD:
            pa.mScope = kAudioDevicePropertyScopeInput;
            break;

        default:
            SET_ERROR(handle, CA_UNKNOWN_IO_DIRECTION);
            return (char *)0;

    }

    if(max_channels < 1) {
        SET_ERROR(handle, CA_DEVICE_NO_CHANNELS);
        return (char *)0;
    }

    if((channel_index < 1) || (channel_index > max_channels)) {
        ca_print_msg(handle, CA_CHANNEL_INDEX_OUT_OF_RANGE, (char *) __func__);
        return (char *)0;
    }

    pa.mElement = (UInt32) channel_index;

    result = AudioObjectGetPropertyData(device_id, &pa, 0, (void *)0, &propSize, &cfs_channel_name);

    if(result != noErr) {
        SET_ERROR(handle, CA_GET_PROPERTY_DATA_ERROR);
        return (char *)0;
    }

    channel_name = (char *) calloc(AUDIO_CHANNEL_NAME_LIMIT + 1, sizeof(char));

    if(!channel_name) {
        SET_ERROR(handle, CA_ALLOCATE_FAIL);
        return (char *)0;
    }

    CFStringGetCString(cfs_channel_name, channel_name, AUDIO_CHANNEL_NAME_LIMIT, kCFStringEncodingASCII);
    CFRelease(cfs_channel_name);

    // The OS doesn't assign channel decriptions to every device. Add them.
    size_t strcnt = strnlen(channel_name, (size_t) AUDIO_CHANNEL_NAME_LIMIT);
    bool data_flag = false;

    if(strcnt) {
        for(size_t g = 0; g < strcnt; g++) {
            if(channel_name[g] > ' ') {
                data_flag = true;
                break;
            }
        }
    }

    if(!data_flag) {
        if((max_channels == 1) && (channel_index == 1))
            strncpy(channel_name, "Mono", AUDIO_CHANNEL_NAME_LIMIT);
        else if(max_channels == 2) {
            if(channel_index == 1)
                strncpy(channel_name, "Left", AUDIO_CHANNEL_NAME_LIMIT);
            else if(channel_index == 2)
                strncpy(channel_name, "Right", AUDIO_CHANNEL_NAME_LIMIT);
        } else {
            // Somthing greater then two channels
            snprintf(channel_name, AUDIO_CHANNEL_NAME_LIMIT, "Channel %u", (unsigned int) channel_index);
        }
    }

    return channel_name;
}

/** ***********************************************************************************
 * \brief Return number of channels for given io device.
 * \param id Device ID
 * \param record Storage for the number of channels
 * \param playback Storage for the number of channels
 * \return 0 okay, error otherwise.
 **************************************************************************************/
int ca_get_no_of_channels(CA_HANDLE handle, AudioDeviceID id, size_t *record, size_t *playback)
{
    if(!handle) {
        return SET_ERROR(0, CA_INVALID_HANDLE);
    }

    AudioObjectPropertyAddress properties = {0, 0, 0};
    OSStatus results = noErr;
    UInt32 size = 0;
    AudioBufferList *bufferList = 0;
    int error = CA_NO_ERROR;

    if(record) {
        *record = 0;

        properties.mSelector = kAudioDevicePropertyStreamConfiguration;
        properties.mScope    = kAudioDevicePropertyScopeInput;
        properties.mElement  = kAudioObjectPropertyElementMaster;

        results = AudioObjectGetPropertyDataSize(id, &properties, 0, NULL, &size);
        if(results == noErr) {

            bufferList = (AudioBufferList *) calloc(size, sizeof(AudioBufferList));
            if(!bufferList) {
                error = SET_ERROR(handle, CA_ALLOCATE_FAIL);
                goto _playback;
            }

            results = AudioObjectGetPropertyData(id, &properties, 0, NULL, &size, bufferList);

            if(results == noErr) {
                for(UInt32 index = 0; index < bufferList->mNumberBuffers; index++)
                    *record += bufferList->mBuffers[index].mNumberChannels;
            } else {
                error = SET_ERROR(handle, CA_GET_PROPERTY_DATA_ERROR);
            }

            free(bufferList);

        }  else {
            error = SET_ERROR(handle, CA_GET_PROPERTY_DATA_SIZE_ERROR);
        }
    }

_playback:;

    if(playback) {
        *playback = 0;

        properties.mSelector = kAudioDevicePropertyStreamConfiguration;
        properties.mScope    = kAudioDevicePropertyScopeOutput;
        properties.mElement  = kAudioObjectPropertyElementMaster;

        results = AudioObjectGetPropertyDataSize(id, &properties, 0, NULL, &size);
        if(results == noErr) {

            bufferList = (AudioBufferList *) calloc(size, sizeof(AudioBufferList));
            if(!bufferList) {
                error = SET_ERROR(handle, CA_ALLOCATE_FAIL);
                goto _end;
            }

            results = AudioObjectGetPropertyData(id, &properties, 0, NULL, &size, bufferList);

            if(results == noErr) {
                for(UInt32 index = 0; index < bufferList->mNumberBuffers; index++)
                    *playback += bufferList->mBuffers[index].mNumberChannels;
            } else {
                error = SET_ERROR(handle, CA_GET_PROPERTY_DATA_ERROR);
            }

            free(bufferList);
        }  else {
            error = SET_ERROR(handle, CA_GET_PROPERTY_DATA_SIZE_ERROR);
        }
    }

_end:;

    return error;
}

/** ****************************************************************************
 * \brief Set the sample rate of the audio device in question and wait for
 * async response.
 * \param device_id Operating System device ID
 * \param sample_rate Sample rate value
 * \return 0 if okay, otherwise error.
 *******************************************************************************/
int ca_set_sample_rate(CA_HANDLE handle, AudioDeviceID device_id, Float64 sample_rate)
{
    if(!handle) {
        return SET_ERROR(0, CA_INVALID_HANDLE);
    }

    OSStatus result = noErr;

    AudioObjectPropertyAddress properties = {
        kAudioDevicePropertyNominalSampleRate,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMaster
    };

    struct ca_wait_data *wait_data = (struct ca_wait_data *) calloc(1, sizeof(struct ca_wait_data));

    if(!wait_data) return SET_ERROR(handle, CA_ALLOCATE_FAIL);

    memcpy((void *) &wait_data->properties, (void *) &properties, sizeof(AudioObjectPropertyAddress));
    wait_data->handle    = handle;
    wait_data->device_id = device_id;

    ca_start_listener(handle, wait_data);

    result = AudioObjectSetPropertyData(device_id, &properties, 0, NULL, sizeof(Float64), &sample_rate);

    ca_wait_for_os(handle, 1.5);

    ca_remove_listener(handle, wait_data);

    free(wait_data);

    return result;
}

/** ****************************************************************************
 * \brief Return the samplerate of the audio device in question.
 * \param device_id Operating System device ID
 * \param sample_rate The sample rate curreently set fot he device.
 * \return true if okay, flase if failed.
 *******************************************************************************/
int ca_get_sample_rate(CA_HANDLE handle, AudioDeviceID device_id, Float64 *sample_rate)
{
    if(!handle) {
        return SET_ERROR(0, CA_INVALID_HANDLE);
    }

    OSStatus result = noErr;
    UInt32 property_size = sizeof(Float64);

    AudioObjectPropertyAddress properties = {
        kAudioDevicePropertyActualSampleRate,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMaster
    };

    *sample_rate = 0;

    result = AudioObjectGetPropertyData(device_id, &properties, 0, NULL, &property_size, sample_rate);

    return result;
}

/** ****************************************************************************
 * \brief Return the available sample rates for the device in question.
 * \param device_id Operating system audio device ID
 * \param sample_rates Invalid pointer to an array of sample rates. Once assigned
 * use free() to deallocate.
 * \para count Number of entries in the sample_rates array.
 * \return 0 if okay, otherwise error.
 *******************************************************************************/
int ca_get_sample_rates(CA_HANDLE handle, AudioDeviceID device_id, Float64 **sample_rates, size_t *count)
{
    if(!handle) {
        return SET_ERROR(0, CA_INVALID_HANDLE);
    }

    OSStatus result = noErr;
    UInt32 property_size = 0;
    Float64 *tmp = (Float64 *)0;
    UInt32 _count = 0;
    UInt32 unit_size = sizeof(Float64);

    AudioObjectPropertyAddress properties = {
        kAudioDevicePropertyAvailableNominalSampleRates,
        kAudioDevicePropertyStreamConfiguration,
        kAudioObjectPropertyElementMaster
    };

    *count = 0;

    if(!sample_rates)
        return SET_ERROR(handle, CA_INVALID_PARAMATER);

    *sample_rates = (Float64 *) 0;

    result = AudioObjectGetPropertyDataSize(device_id, &properties, 0, NULL, &property_size);

    if(result != noErr)
        return SET_ERROR(handle, CA_GET_PROPERTY_DATA_SIZE_ERROR);

    _count = property_size / unit_size;

    if(!_count)
        return SET_ERROR(handle, CA_INVALID_PARAMATER);

    tmp = (Float64 *) calloc(_count + 1, unit_size);

    if(!tmp)
        return SET_ERROR(handle, CA_ALLOCATE_FAIL);

    result = AudioObjectGetPropertyData(device_id, &properties, 0, NULL, &property_size, tmp);

    if(result != noErr)
        return SET_ERROR(handle, CA_GET_PROPERTY_DATA_ERROR);

    *count = _count;
    *sample_rates = tmp;

    return CA_NO_ERROR;
}

/** ***********************************************************************************
 * \brief Wait for the operation system to do it's job.
 * \param selector Message type to wait for.
 * \param timeout Limit wait time.
 * \return 0 okay,  ETIMEDOUT/Other Error.
 **************************************************************************************/
int ca_wait_for_os(CA_HANDLE handle, float timeout)
{
    if(!handle) {
        return SET_ERROR(0, CA_INVALID_HANDLE);
    }

    struct timespec ts;
    struct timeval tp;
    unsigned long seconds = 0;
    unsigned long microseconds = 0;
    float tmp = 0.0;
    int loop_count = 0;
    int return_state = 0;

    struct st_core_audio_handle *ca_handle = (struct st_core_audio_handle *) handle;

    if(ca_handle->exit_flag) return CA_NO_ERROR;

    while(1) {
        tmp = timeout;
        if(tmp <= 0.0) tmp = 1.0;

        gettimeofday(&tp, NULL);

        seconds = (unsigned long) tmp;
        tmp -= (float) seconds;
        microseconds = (unsigned long) (tmp * 1000000.0);

        ts.tv_nsec = (tp.tv_usec * 1000) + microseconds;
        ts.tv_sec  = tp.tv_sec + seconds;

        pthread_mutex_lock(&ca_handle->flag_mutex);
        return_state = pthread_cond_timedwait(&ca_handle->flag_cond, &ca_handle->flag_mutex, &ts);
        pthread_mutex_unlock(&ca_handle->flag_mutex);

        if(!return_state) break;

        loop_count++;
        if(loop_count > 1) break;
    }

#if 0
    if(return_state == EINVAL)
        return_state = EINVAL;

    if(return_state == ETIMEDOUT)
        return_state = ETIMEDOUT;
#endif // #if 0

    return return_state;
}

/** ****************************************************************************
 * \brief Get the last set internal error code.
 * \param none
 * \return return error code
 *******************************************************************************/
int ca_get_error(CA_HANDLE handle)
{
    if(!handle) {
        return SET_ERROR(0, CA_INVALID_HANDLE);
    }

    struct st_core_audio_handle *ca_handle = (struct st_core_audio_handle *) handle;

    if(ca_handle->error_code < 0) {
        ca_handle->error_code = -ca_handle->error_code;
    }

    return ca_handle->error_code;
}

/** ****************************************************************************
 * \brief Set the internal error code and print a message.
 * \param handle Core Audio handle.
 * \param fn Function name of where the error occured.
 * \param line_number Line number of where the error occured.
 * \param error_no Error number.
 * \return error_no
 *******************************************************************************/
int ca_set_error(CA_HANDLE handle, const char *fn, int line_number, int error_no)
{
    struct st_core_audio_handle *ca_handle = (struct st_core_audio_handle *) handle;

    if(ca_handle) {
        ca_handle->last_error_code = ca_handle->error_code;
        ca_handle->error_code = error_no;
    }

    if(error_no < 0 || error_no > CA_INVALID_ERROR) error_no = CA_INVALID_ERROR;

#if BUILD_FLDIGI
    if ((debug::ERROR_LEVEL <= debug::level) && (log_source_ & debug::mask))
        debug::log(debug::ERROR_LEVEL, fn, __FILE__, line_number, "Core Audio: %s", ca_error_message(error_no));
#else
    printf("Function: %s\nFile: %s\nLine No: %d\nCore Audio Error: %s\n", fn, __FILE__, line_number, ca_error_message(handle, error_no));
#endif

    return error_no;
}

/** ***********************************************************************************
 * \brief Print a message to the console.
 * \param handle Core Audio handle.
 * \param error_no Message index.
 * \param string Additional Information.
 * \return error_no
 **************************************************************************************/
int ca_print_msg(CA_HANDLE handle, int error_no, char *string)
{
    if(error_no < 0 || error_no > CA_INVALID_ERROR) error_no = CA_INVALID_ERROR;

    if(!string)
        string = (char *) "";

#if BUILD_FLDIGI
    if ((debug::INFO_LEVEL <= debug::level) && (log_source_ & debug::mask))
        debug::log(debug::INFO_LEVEL, fn, __FILE__, line_number, "Core Audio: %s (%s)", msg_string[error_no], string);
#else
    printf("%s %s\n", msg_string[error_no], string);
#endif

    return error_no;
}

/** ***********************************************************************************
 * \brief Converison scale between internal and external samples.
 * \param scale Convert selector.
 **************************************************************************************/
void ca_format_scale(CA_HANDLE handle, enum host_data_types data_type)
{
    if(!handle) {
        SET_ERROR(0, CA_INVALID_HANDLE);
        return;
    }

    struct st_core_audio_handle *ca_handle = (struct st_core_audio_handle *) handle;

    switch (data_type) {
        case ADF_S8:
            ca_handle->outOSScale = INT8_MAX;
            ca_handle->inOSScale  = 1.0/INT8_MAX;
            ca_handle->bias       = 0.0;
            ca_handle->zero_ref   = 0;
            break;

        case ADF_S16:
            ca_handle->outOSScale = INT16_MAX;
            ca_handle->inOSScale  = 1.0/INT16_MAX;
            ca_handle->bias       = 0.0;
            ca_handle->zero_ref   = 0;
            break;

        case ADF_S32:
            ca_handle->outOSScale = INT32_MAX;
            ca_handle->inOSScale  = 1.0/INT32_MAX;
            ca_handle->bias       = 0.0;
            ca_handle->zero_ref   = 0;
            break;

        case ADF_S64:
            ca_handle->outOSScale = INT64_MAX;
            ca_handle->inOSScale  = 1.0/INT64_MAX;
            ca_handle->bias       = 0.0;
            ca_handle->zero_ref   = 0;
            break;

        case ADF_U8:
            ca_handle->outOSScale = INT8_MAX;
            ca_handle->inOSScale  = 1.0/INT8_MAX;
            ca_handle->bias       = 1.0;
            ca_handle->zero_ref   = ((UInt8) INT8_MAX) + 1;
            break;

        case ADF_U16:
            ca_handle->outOSScale = INT16_MAX;
            ca_handle->inOSScale  = 1.0/INT16_MAX;
            ca_handle->bias       = 1.0;
            ca_handle->zero_ref   = ((UInt16) INT16_MAX) + 1;
            break;

        case ADF_U32:
            ca_handle->outOSScale = INT32_MAX;
            ca_handle->inOSScale  = 1.0/INT32_MAX;
            ca_handle->bias       = 1.0;
            ca_handle->zero_ref   = ((UInt32) INT32_MAX) + 1;
            break;

        case ADF_U64:
            ca_handle->outOSScale = INT64_MAX;
            ca_handle->inOSScale  = 1.0/INT64_MAX;
            ca_handle->bias       = 1.0;
            ca_handle->zero_ref   = ((UInt64) INT64_MAX) + 1;
            break;

        case ADF_FLOAT:
        case ADF_DOUBLE:
        default:
            ca_handle->outOSScale = 1.0;
            ca_handle->inOSScale  = 1.0;
            ca_handle->bias       = 0.0;
            ca_handle->zero_ref   = 0.0;
            break;
    }
}

/** ****************************************************************************
 * \brief Return a string representaion of a error number.
 * \param error_no Error number
 * \return const char * return copy of error string.
 *******************************************************************************/
const char * ca_error_message(CA_HANDLE handle, int error_no)
{
    char *return_str = (char *)0;
    struct st_core_audio_handle *ca_handle = (struct st_core_audio_handle *) handle;

    if(error_no < 0)
        error_no = -error_no;

    if(error_no > CA_INVALID_ERROR)
        return_str = (char *) msg_string[CA_INVALID_ERROR];
    else
        return_str = (char *) msg_string[error_no];

    if(ca_handle) {
        memset(ca_handle->err_str, 0, sizeof(ca_handle->err_str));
        strncpy(ca_handle->err_str, return_str, sizeof(ca_handle->err_str));
        return (const char *) ca_handle->err_str;
    } else {
        static char _emsg[1024];
        memset(_emsg, 0, sizeof(_emsg));
        strncpy(_emsg, return_str, sizeof(_emsg) - 1);
        return (const char *) _emsg;
    }

    return "";
}

/** ****************************************************************************
 * \brief Return the number of frames availible for reading or writting.
 * \param io_device the device in question.
 * \param count Number of frames available.
 * \return Error Code, 0 okay, other error.
 *******************************************************************************/
int ca_frames(CA_HANDLE handle, int io_device, size_t *used_count, size_t *free_count)
{
    if(!handle) {
        return SET_ERROR(0, CA_INVALID_HANDLE);
    }

    int error = CA_NO_ERROR;

    struct st_ring_buffer * rb = ca_device_to_ring_buffer(handle, io_device);

    if(!rb || !used_count || !free_count )
        return SET_ERROR(handle, CA_INVALID_PARAMATER);

    *used_count = *free_count = 0;

    switch(io_device) {
        case AD_PLAYBACK:
            if(rb->frame_count < 1)
                error = CA_BUFFER_EMPTY;
            break;

        case AD_RECORD:
            if(rb->frame_count >= rb->frame_size)
                error = CA_BUFFER_FULL;
            break;
    }

    *used_count = rb->frame_count;
    *free_count = rb->frame_size - rb->frame_count;

    return error;
}

/** ****************************************************************************
 * \brief Test read/write pointers for stall condition.
 * \param elem element storeage.
 * \return Error Code, 0 okay, other error.
 *******************************************************************************/
int ca_empty(CA_HANDLE handle, struct st_ring_buffer *rb)
{
    if(!rb->frame_count) return CA_BUFFER_EMPTY;

    return CA_NO_ERROR;
}

/** ****************************************************************************
 * \brief Test read/write pointers for stall condition.
 * \param elem element storeage.
 * \return Error Code, 0 okay, other error.
 *******************************************************************************/
int ca_stalled(CA_HANDLE handle, struct st_ring_buffer *rb)
{
    if(rb->frame_count >= rb->frame_size) return CA_BUFFER_STALLED;

    return CA_NO_ERROR;
}


/** ****************************************************************************
 * \brief Increment the element pointer within the bounds of the storage array.
 * \param elem element storeage.
 * \return Error Code, 0 okay, other error.
 *******************************************************************************/
int ca_inc_read_pos(CA_HANDLE handle, struct st_ring_buffer *rb)
{
    if(!rb) return SET_ERROR(handle, CA_INVALID_PARAMATER);

    if(rb->frame_count < 1) {
        return CA_BUFFER_EMPTY;
    }

    size_t tmp = rb->read_pos + rb->channels_per_frame;

    if(tmp >= rb->unit_size)
        rb->read_pos = 0;
    else
        rb->read_pos = tmp;

    if(rb->frame_count > 0)
        rb->frame_count--;

    rb->unit_count = rb->frame_count * rb->channels_per_frame;

    return CA_NO_ERROR;
}

/** ****************************************************************************
 * \brief Increment the element pointer within the bounds of the storage array.
 * \param elem element storeage.
 * \return Error Code, 0 okay, other error.
 *******************************************************************************/
int ca_inc_write_pos(CA_HANDLE handle, struct st_ring_buffer *rb)
{
    if(!rb)
        return SET_ERROR(handle, CA_INVALID_PARAMATER);

    if((rb->unit_count >= rb->unit_size) || (rb->frame_count >= rb->frame_size))
        return SET_ERROR(handle, CA_BUFFER_STALLED);

    size_t tmp = rb->write_pos + rb->channels_per_frame;

    if(tmp >= rb->unit_size) {
        rb->write_pos = 0;
    } else
        rb->write_pos = tmp;

    if(rb->frame_count < rb->frame_size)
        rb->frame_count++;

    rb->unit_count = rb->frame_count * rb->channels_per_frame;

    return CA_NO_ERROR;
}

/** ****************************************************************************
 * \brief Write a single value from Ring buffer.
 * \param elem data to store.
 * \return Error Code, 0 okay, other error.
 *******************************************************************************/
static int ca_write_native_stereo(CA_HANDLE handle, int io_device, CORE_AUDIO_DATA_TYPE left, CORE_AUDIO_DATA_TYPE right)
{
    if(!handle) {
        return SET_ERROR(0, CA_INVALID_HANDLE);
    }

    struct st_ring_buffer *rb = ca_device_to_ring_buffer(handle, io_device);
    struct st_core_audio_handle *ca_handle = (struct st_core_audio_handle *) handle;

    if(!ca_handle->okay_to_run)
        return SET_ERROR(handle, CA_INITIALIZE_FAIL);

    if(!rb)
        return SET_ERROR(handle, CA_INVALID_PARAMATER);

    rb->buffer[rb->write_pos + rb->left_channel_index]  = (CORE_AUDIO_DATA_TYPE) left;
    rb->buffer[rb->write_pos + rb->right_channel_index] = (CORE_AUDIO_DATA_TYPE) right;

    int error = ca_inc_write_pos(handle, rb);

    if(error)
        SET_ERROR(handle, error);

    return error;
}

/** ****************************************************************************
 * \brief Write Data in the Native Operating System Format.
 * \param rp_device reference to which ring buffer to copy to.
 * \param amount_to_write Number of bytes in the buffer.
 * \param amount_written Number of bytes written to the ring buffer.
 * \param buffer Storage of the audio data source pointer.
 * \return int error_code 0 okay, other Error.
 *******************************************************************************/
static int ca_write_native_buffer(CA_HANDLE handle, int io_device, size_t amount_to_write, size_t *amount_written, void *buffer)
{
    if(!handle) {
        return SET_ERROR(0, CA_INVALID_HANDLE);
    }

    long head_count = 0;
    long tail_count = 0;
    long free_count = 0;
    CORE_AUDIO_DATA_TYPE *data = (CORE_AUDIO_DATA_TYPE *) buffer;

    struct st_ring_buffer *rb = ca_device_to_ring_buffer(handle, io_device);

    *amount_written = 0;

    if(!rb || !data)
        return SET_ERROR(handle, CA_INVALID_PARAMATER);

    if(rb->unit_count >= rb->unit_size)
        return CA_BUFFER_FULL;

    if(rb->unit_count < 1) {
        rb->write_pos = rb->read_pos = 0;
        if(amount_to_write > rb->unit_size)
            head_count = rb->unit_size;
        else
            head_count = amount_to_write;
    } else if(rb->write_pos >= rb->read_pos) {
        head_count = rb->unit_size - rb->write_pos;
        tail_count = rb->read_pos;
    } else {
        head_count = 0;
        tail_count = rb->write_pos - rb->read_pos;
    }

    free_count = head_count + tail_count;

    if(amount_to_write > free_count)
        amount_to_write = free_count;

    if(head_count) {
        if(head_count > amount_to_write) {
            head_count = amount_to_write;
            tail_count = 0;
            amount_to_write = 0;
        } else {
            amount_to_write -= head_count;
        }
    }

    tail_count = amount_to_write;

    if(head_count > 0) {
        if(rb->write_pos >= rb->unit_size) rb->write_pos = 0;
        memcpy((void *) &rb->buffer[rb->write_pos], (void *) data, (size_t) (head_count * sizeof(CORE_AUDIO_DATA_TYPE)));
        rb->write_pos   += head_count;
        *amount_written += head_count;
        rb->unit_count  += head_count;
        rb->frame_count  = rb->unit_count / rb->channels_per_frame;
    }

    if(tail_count > 0) {
        if(rb->write_pos >= rb->unit_size) rb->write_pos = 0;
        memcpy((void *) &rb->buffer[rb->write_pos], (void *) &data[*amount_written], tail_count * sizeof(CORE_AUDIO_DATA_TYPE));
        rb->write_pos   += tail_count;
        *amount_written += tail_count;
        rb->unit_count  += tail_count;
        rb->frame_count  = rb->unit_count / rb->channels_per_frame;
    }

    return CA_NO_ERROR;
}

/** ****************************************************************************
 * \brief Write a single value from Ring buffer.
 * \param elem data to store.
 * \return Error Code, 0 okay, other error.
 *******************************************************************************/
int ca_write_stereo(CA_HANDLE handle, int io_device, HOST_AUDIO_DATA_TYPE left, HOST_AUDIO_DATA_TYPE right)
{
    if(!handle) {
        return SET_ERROR(0, CA_INVALID_HANDLE);
    }

    struct st_core_audio_handle *ca_handle = (struct st_core_audio_handle *) handle;
    int retval = CA_NO_ERROR;

    if(!ca_handle->okay_to_run)
        return SET_ERROR(handle, CA_INITIALIZE_FAIL);

    pthread_mutex_lock(&ca_handle->io_mutex);

    struct st_ring_buffer * rb = ca_device_to_ring_buffer(handle, io_device);

    if(!rb) {
        retval = SET_ERROR(handle, CA_INVALID_PARAMATER);
        goto _end;
    }

    rb->buffer[rb->write_pos + rb->left_channel_index]  = (CORE_AUDIO_DATA_TYPE) ((left)  * ca_handle->inOSScale) - ca_handle->bias;
    rb->buffer[rb->write_pos + rb->right_channel_index] = (CORE_AUDIO_DATA_TYPE) ((right) * ca_handle->inOSScale) - ca_handle->bias;

    int error = ca_inc_write_pos(handle, rb);

    if(error)
        SET_ERROR(handle, error);

_end:;
    pthread_mutex_unlock(&ca_handle->io_mutex);

    return error;
}

/** ****************************************************************************
 * \brief Read Data in the Native Operating System Format.
 * \param amount_to_read Number of elements in the buffer.
 * \param amount_read The number of elements written into the ring buffer.
 * \param buffer Elements to be read in the ring buffer.
 * \return int error_code 0 okay, < 0 Error.
 *******************************************************************************/
int ca_write_buffer(CA_HANDLE handle, int io_device, size_t amount_to_write, size_t *amount_written, HOST_AUDIO_DATA_TYPE buffer[])
{
    if(!handle) {
        return SET_ERROR(0, CA_INVALID_HANDLE);
    }

    *amount_written = 0;

    struct st_core_audio_handle *ca_handle = (struct st_core_audio_handle *) handle;
    int error = CA_NO_ERROR;
    if(!ca_handle->okay_to_run)
        return SET_ERROR(handle, CA_INITIALIZE_FAIL);

    pthread_mutex_lock(&ca_handle->io_mutex);

    struct st_ring_buffer * rb = ca_device_to_ring_buffer(handle, io_device);

    if(!amount_to_write || !rb) {
        error = SET_ERROR(handle, CA_INVALID_PARAMATER);
        goto _end;
    }

    size_t index = 0;
    size_t dst_index = 0;
    size_t limit = (rb->frame_size - rb->frame_count);

    if(amount_to_write > limit)
        amount_to_write = limit;

    for(index = 0; index < amount_to_write; index++) {
        rb->buffer[rb->write_pos + rb->left_channel_index]  = (CORE_AUDIO_DATA_TYPE) ((buffer[dst_index++] * ca_handle->inOSScale) - ca_handle->bias);
        rb->buffer[rb->write_pos + rb->right_channel_index] = (CORE_AUDIO_DATA_TYPE) ((buffer[dst_index++] * ca_handle->inOSScale) - ca_handle->bias);
        (*amount_written)++;
        if(ca_inc_write_pos(handle, rb)) {
            error = CA_BUFFER_EMPTY;
            goto _end;
        }
    }

_end:;
    pthread_mutex_unlock(&ca_handle->io_mutex);

    return error;
}

/** ****************************************************************************
 * \brief Read a single value from Ring buffer.
 * \param elem element storeage.
 * \return Error Code, 0 okay, other error.
 *******************************************************************************/
int ca_read_stereo(CA_HANDLE handle, int io_device, HOST_AUDIO_DATA_TYPE *left, HOST_AUDIO_DATA_TYPE *right)
{
    if(!handle) {
        return SET_ERROR(0, CA_INVALID_HANDLE);
    }

    struct st_core_audio_handle *ca_handle = (struct st_core_audio_handle *) handle;

    if(!ca_handle->okay_to_run)
        return SET_ERROR(handle, CA_INITIALIZE_FAIL);

    pthread_mutex_lock(&ca_handle->io_mutex);

    struct st_ring_buffer * rb = ca_device_to_ring_buffer(handle, io_device);

    if(!rb) return SET_ERROR(handle, CA_INVALID_PARAMATER);

    *left  = (HOST_AUDIO_DATA_TYPE) ((rb->buffer[rb->read_pos + rb->left_channel_index]  + ca_handle->bias) * ca_handle->outOSScale);
    *right = (HOST_AUDIO_DATA_TYPE) ((rb->buffer[rb->read_pos + rb->right_channel_index] + ca_handle->bias) * ca_handle->outOSScale);

    int error = ca_inc_read_pos(handle, rb);

    if(error)
        SET_ERROR(handle, error);

    pthread_mutex_unlock(&ca_handle->io_mutex);

    return error;
}

/** ****************************************************************************
 * \brief Read Data in the Native Operating System Format.
 * \param amount_to_read Number of elements in the buffer.
 * \param amount_read The number of elements written into the ring buffer.
 * \param buffer Elements to be read in the ring buffer.
 * \return int error_code 0 okay, < 0 Error.
 *******************************************************************************/
int ca_read_buffer(CA_HANDLE handle, int io_device, size_t amount_to_read, size_t *amount_read, HOST_AUDIO_DATA_TYPE buffer[])
{
    if(!handle) {
        return SET_ERROR(0, CA_INVALID_HANDLE);
    }

    *amount_read = 0;
    struct st_core_audio_handle *ca_handle = (struct st_core_audio_handle *) handle;

    int error = CA_NO_ERROR;

    if(!ca_handle->okay_to_run)
        return SET_ERROR(handle, CA_INITIALIZE_FAIL);

    pthread_mutex_lock(&ca_handle->io_mutex);

    struct st_ring_buffer * rb = ca_device_to_ring_buffer(handle, io_device);

    if(!amount_to_read || !rb) {
        error = SET_ERROR(handle, CA_INVALID_PARAMATER);
        goto _end;
    }

    size_t index = 0;
    size_t dst_index = 0;

    if(amount_to_read > rb->frame_count)
        amount_to_read = rb->frame_count;

    for(index = 0; index < amount_to_read; index++) {
        buffer[dst_index++] = (HOST_AUDIO_DATA_TYPE) (rb->buffer[rb->read_pos + rb->left_channel_index]  + ca_handle->bias) * ca_handle->outOSScale;
        buffer[dst_index++] = (HOST_AUDIO_DATA_TYPE) (rb->buffer[rb->read_pos + rb->right_channel_index] + ca_handle->bias) * ca_handle->outOSScale;
        (*amount_read)++;
        if(ca_inc_read_pos(handle, rb)) {
            error = CA_BUFFER_EMPTY;
            goto _end;
        }
    }

_end:;
    pthread_mutex_unlock(&ca_handle->io_mutex);
    return error;
}

/** ****************************************************************************
 * \brief Write a single value from Ring buffer.
 * \param elem data to store.
 * \return Error Code, 0 okay, other error.
 *******************************************************************************/
static int ca_read_native_stereo(CA_HANDLE handle, int io_device, CORE_AUDIO_DATA_TYPE *left, CORE_AUDIO_DATA_TYPE *right)
{
    if(!handle) {
        return SET_ERROR(0, CA_INVALID_HANDLE);
    }

    struct st_core_audio_handle *ca_handle = (struct st_core_audio_handle *) handle;

    if(!ca_handle->okay_to_run)
        return SET_ERROR(handle, CA_INITIALIZE_FAIL);

    struct st_ring_buffer * rb = ca_device_to_ring_buffer(handle, io_device);

    if(!rb)
        return SET_ERROR(handle, CA_INVALID_PARAMATER);

    *left  = (CORE_AUDIO_DATA_TYPE) rb->buffer[rb->read_pos + rb->left_channel_index];
    *right = (CORE_AUDIO_DATA_TYPE) rb->buffer[rb->read_pos + rb->right_channel_index];
    
    int error = ca_inc_read_pos(handle, rb);
    
    if(error)
        SET_ERROR(handle, error);
    
    return error;
}

/** ****************************************************************************
 * \brief Read Data in the Native Operating System Format.
 * \param amount_to_read Number of elements in the buffer.
 * \param amount_read The number of elements written into the ring buffer.
 * \param buffer Elements to be read in the ring buffer.
 * \return int error_code 0 okay, < 0 Error.
 *******************************************************************************/
static int ca_read_native_buffer(CA_HANDLE handle, int io_device, size_t amount_to_read, size_t *amount_read, void *buffer)
{
    if(!handle) {
        return SET_ERROR(0, CA_INVALID_HANDLE);
    }
    
    long head_count = 0;
    long tail_count = 0;
    long free_count = 0;
    CORE_AUDIO_DATA_TYPE *data = (CORE_AUDIO_DATA_TYPE *) buffer;
    struct st_ring_buffer * rb = (struct st_ring_buffer *) 0;
    
    *amount_read = 0;
    rb = ca_device_to_ring_buffer(handle, io_device);
    
    if(!rb || !data)
        return SET_ERROR(handle, CA_INVALID_PARAMATER);
    
    if(rb->unit_count < 1)
        return SET_ERROR(handle, CA_BUFFER_EMPTY);
    
    if(rb->read_pos >= rb->write_pos) {
        head_count = rb->unit_size - rb->read_pos;
        tail_count = rb->write_pos;
    } else {
        head_count = rb->write_pos - rb->read_pos;
        tail_count = 0;
    }
    
    free_count = head_count + tail_count;
    
    if(amount_to_read > free_count)
        amount_to_read = free_count;
    
    if(head_count) {
        if(head_count > amount_to_read) {
            head_count = amount_to_read;
            tail_count = 0;
            amount_to_read = 0;
        } else {
            amount_to_read -= head_count;
        }
    }
    
    tail_count = amount_to_read;
    
    if(head_count > 0) {
        memcpy((void *) data, (void *)  &rb->buffer[rb->read_pos], (size_t) (head_count * sizeof(CORE_AUDIO_DATA_TYPE)));
        rb->read_pos   += head_count;
        *amount_read    += head_count;
        rb->unit_count -= head_count;
        if(rb->read_pos >= rb->unit_size) rb->read_pos = 0;
        rb->frame_count = rb->unit_count / rb->channels_per_frame;
    }
    
    if(tail_count > 0) {
        memcpy((void *) &data[*amount_read], (void *) &rb->buffer[rb->read_pos], (size_t) (tail_count * sizeof(CORE_AUDIO_DATA_TYPE)));
        rb->read_pos   += tail_count;
        *amount_read   += tail_count;
        rb->unit_count -= tail_count;
        if(rb->read_pos >= rb->unit_size) rb->read_pos = 0;
        rb->frame_count = rb->unit_count / rb->channels_per_frame;
    }
    
    return CA_NO_ERROR;
}

/** ***********************************************************************************
 * \brief Clear the ring buffer for playback and record.
 * \return 0 Okay, otherwise error.
 **************************************************************************************/
int ca_flush_audio_stream(CA_HANDLE handle)
{
    if(!handle) {
        return SET_ERROR(0, CA_INVALID_HANDLE);
    }
    
    struct st_core_audio_handle *ca_handle = (struct st_core_audio_handle *) handle;
    
    pthread_mutex_lock(&ca_handle->io_mutex);
    
    int old_pause_flag = ca_handle->pause_flag;
    
    ca_handle->pause_flag = 1;
    
    if(ca_handle->rb_record) {
        ca_handle->rb_record->write_pos     = (size_t) 0;
        ca_handle->rb_record->read_pos      = (size_t) 0;
        ca_handle->rb_record->unit_count    = (size_t) 0;
        ca_handle->rb_record->frame_count   = (size_t) 0;
    }
    
    if(ca_handle->rb_playback) {
        ca_handle->rb_playback->write_pos   = (size_t) 0;
        ca_handle->rb_playback->read_pos    = (size_t) 0;
        ca_handle->rb_playback->unit_count  = (size_t) 0;
        ca_handle->rb_playback->frame_count = (size_t) 0;
    }
    
    ca_handle->pause_flag = old_pause_flag;
    
    pthread_mutex_unlock(&ca_handle->io_mutex);
    
    return CA_NO_ERROR;
}

/** ****************************************************************************
 * \brief Return ring buffer pointer from device io selector
 * \return struct st_ring_buffer *, null = error
 *******************************************************************************/
struct st_ring_buffer * ca_device_to_ring_buffer(CA_HANDLE handle, int io_device)
{
    if(!handle) {
        SET_ERROR(0, CA_INVALID_HANDLE);
        return (struct st_ring_buffer *)0;
    }
    
    struct st_ring_buffer * _rb = 0;
    struct st_core_audio_handle *ca_handle = (struct st_core_audio_handle *) handle;
    
    switch(io_device) {
        case AD_RECORD:
            _rb = ca_handle->rb_record;
            break;
            
        case AD_PLAYBACK:
            _rb = ca_handle->rb_playback;
            break;
    }
    return _rb;
}

/** ***********************************************************************************
 * \brief Allocate and initialize Ring buffer.
 * \return RingBuffer * pointer or null on error.
 **************************************************************************************/
struct st_ring_buffer * ca_init_ringbuffer(CA_HANDLE handle, size_t allocate_element_count)
{
    if(!handle) {
        SET_ERROR(0, CA_INVALID_HANDLE);
        return (struct st_ring_buffer *)0;
    }
    
    size_t data_size = sizeof(CORE_AUDIO_DATA_TYPE);
    
    struct st_ring_buffer *tmp = (struct st_ring_buffer *) calloc(1, sizeof(struct st_ring_buffer));
    
    if(!tmp) {
        SET_ERROR(handle, CA_ALLOCATE_FAIL);
        return (struct st_ring_buffer *)0;
    }
    
    if(allocate_element_count < 1024)
        allocate_element_count = 1024;
    
    tmp->buffer = (CORE_AUDIO_DATA_TYPE *) calloc(allocate_element_count + 1, data_size);
    
    if(!tmp->buffer) {
        free(tmp);
        SET_ERROR(handle, CA_ALLOCATE_FAIL);
        return (struct st_ring_buffer *)0;
    }
    
    tmp->write_pos       = 0;
    tmp->read_pos        = 0;
    tmp->buffer_size     = allocate_element_count * data_size;
    tmp->unit_count      = allocate_element_count;
    
    ca_format_scale(handle, ADF_FLOAT);
    
    return tmp;
}

/** ***********************************************************************************
 * \brief Deallocate Ring buffer.
 * \return void
 **************************************************************************************/
void ca_release_ringbuffer(struct st_ring_buffer * ring_buffer)
{
    if(!ring_buffer) {
        SET_ERROR(0, CA_INVALID_PARAMATER);
        return;
    }
    
    if(ring_buffer->buffer) {
        memset(ring_buffer->buffer, 0, ring_buffer->buffer_size);
        free(ring_buffer->buffer);
    }
    
    memset(ring_buffer, 0, sizeof(struct st_ring_buffer));
    free(ring_buffer);
}

/** ***********************************************************************************
 * \brief A device may containd more then 2 input/output channels. Use this fuction to
 * map left/right channels from the availble channel(s) of a given device.
 * \param handle Core Audio Handle
 * \param left_index left channel mapping to left channel.
 * \param right_index right channel mapping to right channel.
 * \return void
 **************************************************************************************/
int ca_map_device_channels(CA_HANDLE handle, int io_device, size_t left_index, size_t right_index)
{
    if(!handle) {
        return SET_ERROR(0, CA_INVALID_HANDLE);
    }
    
    struct st_core_audio_handle *ca_handle   = (struct st_core_audio_handle *) handle;
    struct st_audio_device_list *device      = (struct st_audio_device_list *) 0;
    struct st_ring_buffer *rb                = (struct st_ring_buffer *) 0;
    struct st_audio_device_info *device_info = (struct st_audio_device_info *) 0;
    
    size_t no_of_channels = 0;
    int error = CA_NO_ERROR;
    
    switch(io_device) {
        case AD_PLAYBACK:
            device = ca_handle->selected_playback_device;
            if(!device)
                return ca_print_msg(handle, CA_INVALID_PARAMATER, "Playback device not selected");
            
            if(!device->playback)
                return ca_print_msg(handle, CA_INVALID_PARAMATER, "");
            
            device_info = device->playback;
            no_of_channels = device_info->no_of_channels;
            
            if(!ca_handle->rb_playback)
                return ca_print_msg(handle, CA_INVALID_PARAMATER, "Playback Ring buffer not allocated");
            
            rb = ca_handle->rb_playback;
            
            break;
            
        case AD_RECORD:
            device = ca_handle->selected_record_device;
            if(!device)
                return ca_print_msg(handle, CA_INVALID_PARAMATER, "Record device not selected");
            
            if(!device->record)
                return ca_print_msg(handle, CA_INVALID_PARAMATER, "");
            
            device_info = device->record;
            no_of_channels = device_info->no_of_channels;
            
            if(!ca_handle->rb_record)
                return ca_print_msg(handle, CA_INVALID_PARAMATER, "Record Ring buffer not allocated");
            
            rb = ca_handle->rb_record;
            break;
    }
    
    if((left_index >= no_of_channels) || (right_index >= no_of_channels)) {
        return ca_print_msg(handle, CA_INDEX_OUT_OF_RANGE, "");
    }
    
    if(no_of_channels < 2) {
        device_info->selected_left_channel_index  = 0;
        device_info->selected_right_channel_index = 0;
        rb->left_channel_index  = 0;
        rb->right_channel_index = 0;
    } else {
        device_info->selected_left_channel_index  = left_index;
        device_info->selected_right_channel_index = right_index;
        rb->left_channel_index  = left_index;
        rb->right_channel_index = right_index;
    }
    
    return error;
}

/** ***********************************************************************************
 * \brief Copy the content of source audio to destination. Copying occures until
 * the ring buffer is emptied. Must be called repeatedly to maintain cross connect. This
 * is primarily here for testing the audio (data) circuit. But it can be usefull for
 * other applications.
 * \param handle Core Audio Handle
 * \return void
 **************************************************************************************/
void ca_cross_connect(CA_HANDLE handle)
{
    CORE_AUDIO_DATA_TYPE left_audio_data;
    CORE_AUDIO_DATA_TYPE right_audio_data;
    size_t index = 0;
    size_t used_count = 0;
    size_t free_count = 0;
    
    struct st_core_audio_handle *ca_handle = (struct st_core_audio_handle *) handle;
    
    pthread_mutex_lock(&ca_handle->io_mutex);
    pthread_cond_wait(&ca_handle->record_cond, &ca_handle->io_mutex);
    
    ca_frames(handle, AD_RECORD, &used_count, &free_count);
    
    if(used_count) {
        for(index = 0; index < used_count; index++) {
            ca_read_native_stereo(handle, AD_RECORD,   &left_audio_data, &right_audio_data);
            ca_write_native_stereo(handle, AD_PLAYBACK, left_audio_data, right_audio_data);
        }
        pthread_cond_signal(&ca_handle->playback_cond);
    }
    
    pthread_mutex_unlock(&ca_handle->io_mutex);
}

/** ***********************************************************************************
 * \brief Compare one string to another.
 * \param str1 first string.
 * \param str2 second string
 * \param limit maximum string test length.
 * \return 0 = match, other non-match
 **************************************************************************************/
int ca_string_match(const char * str1, const char *str2, size_t limit)
{
    if(!str1 || !str2) return -1;
    int diff = 0;
    int char_count = 0;
    
    for(size_t i = 0; i < limit; i++) {
        if(str1[i] || str2[i]) char_count++;
        diff = tolower(str1[i]) - tolower(str2[i]);
        if((str1[i] == 0) || (str2[i] == 0)) break;
        if(diff != 0) break;
        
    }
    
    // Return non match if both strings contain no data.
    if(!char_count)
        diff = -1;
    
    return diff;
}

