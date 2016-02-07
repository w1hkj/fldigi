/** ***********************************************************************************
 * core_audio.cxx
 *
 * Copyright (C) 2015 Robert Stiles, KK5VD
 *
 * Core Audio coding requirements (in part) derived from RtAudio source code by
 * Gary P. Scavone
 * Original (non-GPL, but free to use) source can be found here.
 * RtAudio WWW site: http://www.music.mcgill.ca/~gary/rtaudio/
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
    (const char *) "Converter initialization failure",
    (const char *) "Playback Device:",
    (const char *) "  Record Device:",
    (const char *) "Close current device before selecting new one",
    (const char *) "Invalid/Unknown error number",
    (const char *) 0
};

// Don't call directly.
static int  ca_read_native_buffer(CA_HANDLE handle, int io_device, size_t amount_to_read, size_t *amount_read, void *buffer);
static int  ca_read_native_stereo(CA_HANDLE handle, int io_device, CORE_AUDIO_DATA_TYPE *left, CORE_AUDIO_DATA_TYPE *right);
static int  ca_write_native_buffer(CA_HANDLE handle, int io_device, size_t amount_to_write, size_t *amount_written, void *buffer);
static int  ca_write_native_stereo(CA_HANDLE handle, int io_device, CORE_AUDIO_DATA_TYPE left, CORE_AUDIO_DATA_TYPE right);
static void ca_free_handle_memory(CA_HANDLE handle);
static int ca_initialize_converter(CA_HANDLE handle);
static void ca_release_converter(CA_HANDLE handle);
static inline int AudioConvertRecord(CA_HANDLE handle);
static inline int AudioConvertPlayback(CA_HANDLE handle);
static inline int ca_test_rb_buffers(CA_HANDLE handle, struct st_ring_buffer *rb);
static char * string_32(UInt32 value, char *buffer, size_t size);

#if defined(MAC_OS_X_VERSION_10_4) && !defined(MAC_OS_X_VERSION_10_7)
static size_t strnlen(const char *str, size_t limit);
#endif
// End don't call directly

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

        pthread_mutex_lock(&ca_handle->record_mutex);

        size_t bufferLoop = inInputData->mNumberBuffers;
        AudioBuffer *data = 0;
        size_t  amt_moved = 0;
        int write_to = ca_handle->record_sample_rate_convert ? AD_RECORD_SRC : AD_RECORD;

        for(UInt32 index = 0; index < bufferLoop; index++) {
            data = (AudioBuffer *) &inInputData->mBuffers[index];
            ca_write_native_buffer(inClientData, write_to, data->mDataByteSize/sizeof(CORE_AUDIO_DATA_TYPE), &amt_moved, data->mData);

            if(ca_handle->close_flag) {
                data->mDataByteSize = 0;
                pthread_cond_signal(&ca_handle->playback_cond);
            }
        }

        pthread_mutex_unlock(&ca_handle->record_mutex);

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

        pthread_mutex_lock(&ca_handle->playback_mutex);

        if(!used) {
            pthread_cond_wait(&ca_handle->playback_cond, &ca_handle->playback_mutex);
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

        pthread_mutex_unlock(&ca_handle->playback_mutex);

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

    /** ***********************************************************************************
     * \brief Convert sample rate from hardware to request rate.
     **************************************************************************************/
    OSStatus PlaybackConvert(AudioConverterRef inAudioConverter,
                             UInt32 *ioNumberDataPackets, AudioBufferList *ioData,
                             AudioStreamPacketDescription  **outDataPacketDescription,
                             void *inUserData)
    {
        int state = noErr;

        if(ioData && inUserData) {
            struct st_core_audio_handle *ca_handle = (struct st_core_audio_handle  *) inUserData;
            size_t out_frames       = 0;
            size_t in_frames        = 0;
            size_t bytes_per_frame  = ca_handle->selected_playback_device->playback->bytes_per_frame;;
            char *buffer = 0;

            ca_frames(inUserData, AD_PLAYBACK_SRC, &in_frames, 0);
            if(in_frames) {
                buffer = (char *) calloc(in_frames, bytes_per_frame);

                if(buffer) {
                    ca_read_native_buffer(inUserData, AD_PLAYBACK_SRC, in_frames, &out_frames, buffer);

                    if(!out_frames) {
                        free(buffer);
                        buffer = (char *) 0;
                        out_frames = 0;
                    }

                    ioData->mBuffers[0].mData = buffer;
                    ioData->mBuffers[0].mDataByteSize = (UInt32) out_frames * (UInt32) bytes_per_frame;
                    *ioNumberDataPackets = (UInt32) out_frames;
                }
            } else {
                ioData->mBuffers[0].mData = (void *) 0;
                ioData->mBuffers[0].mDataByteSize = (UInt32) 0;
                *ioNumberDataPackets = (UInt32) 0;
                state = CA_KEEP_PROCESSING;
            }
        }

        return state;
    }

    /** ***********************************************************************************
     * \brief Convert sample rate from hardware to request rate.
     **************************************************************************************/
    OSStatus RecordConvert(AudioConverterRef inAudioConverter,
                           UInt32 *ioNumberDataPackets, AudioBufferList *ioData,
                           AudioStreamPacketDescription  **outDataPacketDescription,
                           void *inUserData)
    {
        int state = noErr;

        if(ioData && inUserData) {
            struct st_core_audio_handle *ca_handle = (struct st_core_audio_handle  *) inUserData;
            size_t out_frames       = 0;
            size_t in_frames        = 0;
            size_t bytes_per_frame  = 0;
            char *buffer = 0;

            ca_frames(inUserData, AD_RECORD_SRC, &in_frames, 0);

            if(in_frames) {
                bytes_per_frame = ca_handle->selected_record_device->record->bytes_per_frame;
                buffer = (char *) calloc(in_frames, bytes_per_frame);
                if(buffer) {
                    ca_read_native_buffer(inUserData, AD_RECORD_SRC, in_frames, &out_frames, buffer);

                    if(!out_frames) {
                        free(buffer);
                        buffer = (char *) 0;
                        out_frames = 0;
                    }

                    ioData->mBuffers[0].mData = buffer;
                    ioData->mBuffers[0].mDataByteSize = (UInt32) out_frames * (UInt32) bytes_per_frame;
                    *ioNumberDataPackets = (UInt32) out_frames;
                }
            } else {
                ioData->mBuffers[0].mData = (void *) 0;
                ioData->mBuffers[0].mDataByteSize = (UInt32) 0;
                *ioNumberDataPackets = (UInt32) 0;
                state = CA_KEEP_PROCESSING;
            }
        }

        return state;
    }

    /** ***********************************************************************************
     * \brief Process playback audio when signaled.
     * \param in Opaque pointer to a struct st_core_audio_handle.
     * \return void * unused
     **************************************************************************************/
    void * PlaybackConvertThread(void * in)
    {
        struct st_core_audio_handle *ca_handle = (struct st_core_audio_handle  *) in;

        if(in) {
            ca_handle->playback_convert_thread_running = 1;

            while(!ca_handle->convert_close_flag) {
                pthread_mutex_lock(&ca_handle->playback_convert_mutex);
                pthread_cond_wait(&ca_handle->playback_convert_cond, &ca_handle->playback_convert_mutex);
                if(!ca_handle->convert_close_flag)
                    AudioConvertPlayback(in);
                pthread_mutex_unlock(&ca_handle->playback_convert_mutex);
            }

            ca_handle->playback_convert_thread_running = 0;
        }
        return (void *) 0;
    }

    /** ***********************************************************************************
     * \brief Process record audio when signaled.
     * \param in Opaque pointer to a struct st_core_audio_handle.
     * \return void * unused
     **************************************************************************************/
    void *RecordConvertThread(void * in)
    {
        struct st_core_audio_handle *ca_handle = (struct st_core_audio_handle  *) in;
        if(in) {
            ca_handle->record_convert_thread_running = 1;

            while(!ca_handle->convert_close_flag) {
                pthread_mutex_lock(&ca_handle->record_convert_mutex);
                pthread_cond_wait(&ca_handle->record_convert_cond, &ca_handle->record_convert_mutex);
                if(!ca_handle->convert_close_flag)
                    AudioConvertRecord(in);
                pthread_mutex_unlock(&ca_handle->record_convert_mutex);
            }

            ca_handle->record_convert_thread_running = 0;
        }

        return (void *) 0;
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
        SET_ERROR(0, CA_INVALID_HANDLE, 0);
        return;
    }

    struct st_core_audio_handle *ca_handle = (struct st_core_audio_handle  *) handle;
    struct st_audio_device_list *device_list = ca_handle->device_list;
    size_t device_count = ca_handle->device_list_count;
    AudioObjectID default_record_id   = 0;
    AudioObjectID default_playback_id = 0;

    ca_get_default_devices(&default_record_id, &default_playback_id);

    printf("\n");
    for(size_t i = 0; i < device_count; i++) {
        if(device_list[i].device_name) {
            if(device_list[i].device_io & AD_RECORD) {
                printf("%u) Record %s ", (unsigned int) i + 1, device_list[i].device_name);
                if(device_list[i].device_id == default_record_id) printf("(Default)");
                printf("\n");
            }

            if(device_list[i].device_io & AD_PLAYBACK) {
                printf("%u) Play   %s ", (unsigned int) i + 1, device_list[i].device_name);
                if(device_list[i].device_id == default_playback_id) printf("(Default)");
                printf("\n");
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
        SET_ERROR(0, CA_INVALID_HANDLE, 0);
        return;
    }

    ca_close(handle);
    ca_free_handle_memory(handle);

    pthread_mutex_destroy(&ca_handle->playback_mutex);
    pthread_mutex_destroy(&ca_handle->record_mutex);

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
        SET_ERROR(0, CA_INVALID_HANDLE, 0);
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

    if(ca_handle->rb_playback)     ca_release_ringbuffer(ca_handle->rb_playback);
    if(ca_handle->rb_record)       ca_release_ringbuffer(ca_handle->rb_record);
    if(ca_handle->rb_playback_src) ca_release_ringbuffer(ca_handle->rb_playback_src);
    if(ca_handle->rb_record_src)   ca_release_ringbuffer(ca_handle->rb_record_src);

    return;
}

/** ***********************************************************************************
 * \brief Deallocate device information memory.
 * \return void
 **************************************************************************************/
void ca_free_device_attributes(struct st_audio_device_info *device)
{
    if(!device) {
        SET_ERROR(0, CA_INVALID_PARAMATER, 0);
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

    AudioDeviceID *devices            = (AudioDeviceID *)0;
    AudioDeviceID default_record_id   = (AudioDeviceID) 0;
    AudioDeviceID default_playback_id = (AudioDeviceID) 0;

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

    ca_handle = (struct st_core_audio_handle *) calloc(1, sizeof(struct st_core_audio_handle));
    if(!ca_handle) {
        SET_ERROR(0, CA_ALLOCATE_FAIL, noErr);
        return (void *)0;
    }

    if(ca_get_device_id_list(&devices, &device_count)) {
        error = SET_ERROR(0, CA_ALLOCATE_FAIL, noErr);
        goto _end;
    }

    if(device_count < 1) {
        error = SET_ERROR(0, CA_INVALID_PARAMATER, noErr);
        goto _end;
    }

    device_list = (struct st_audio_device_list *) calloc(device_count + 1, sizeof(struct st_audio_device_list));

    if(!device_list) {
        error = SET_ERROR(0, CA_ALLOCATE_FAIL, noErr);
        goto _end;
    }

    ca_handle->device_list = device_list;
    ca_handle->device_list_count = device_count;

    for(size_t i = 0; i < device_count; i++) {
        device_list[i].device_id = devices[i];
        device_list[i].device_index = i;
        device_list[i].device_name = ca_get_device_name((CA_HANDLE) ca_handle, devices[i]);
        ca_get_no_of_channels((CA_HANDLE) ca_handle, devices[i], &record_channels, &playback_channels);
        ca_get_device_attributes((CA_HANDLE) ca_handle, devices[i], &device_list[i], record_channels, playback_channels);

        // Keep track of the number of in/out devices.
        if(record_channels) {
            no_of_record_devices++;
        }

        if(playback_channels) {
            no_of_playback_devices++;
        }

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

    ca_handle->rb_record       = ca_init_ring_buffer((CA_HANDLE) ca_handle, rb_size);
    ca_handle->rb_playback     = ca_init_ring_buffer((CA_HANDLE) ca_handle, rb_size);
    ca_handle->rb_record_src   = ca_init_ring_buffer((CA_HANDLE) ca_handle, rb_size);
    ca_handle->rb_playback_src = ca_init_ring_buffer((CA_HANDLE) ca_handle, rb_size);

    if(!ca_handle->rb_record || !ca_handle->rb_playback) {
        error = SET_ERROR(0, CA_ALLOCATE_FAIL, noErr);
        goto _end;
    }

    pthread_mutex_init(&ca_handle->playback_mutex, NULL);
    pthread_cond_init(&ca_handle->playback_cond,   NULL);

    pthread_mutex_init(&ca_handle->record_mutex,   NULL);
    pthread_cond_init(&ca_handle->record_cond,     NULL);

    pthread_mutex_init(&ca_handle->flag_mutex,     NULL);
    pthread_cond_init(&ca_handle->flag_cond,       NULL);


    if(!ca_get_default_devices(&default_record_id, &default_playback_id)) {
        ca_select_device_by_id((CA_HANDLE) ca_handle, default_record_id,   AD_RECORD);
        ca_select_device_by_id((CA_HANDLE) ca_handle, default_playback_id, AD_PLAYBACK);
    }

_end:;

    if(error != CA_NO_ERROR) {
        ca_free_handle_memory((CA_HANDLE) ca_handle);
    } else {
        handle = (CA_HANDLE) ca_handle;
        ca_handle->okay_to_run  = 1;
        ca_handle->highest_sample_rate = max_sample_rate;
        ca_handle->sample_duration_in_seconds = rb_duration;
    }

    return handle;
}

/** ***********************************************************************************
 * \brief Initialize ring buffer atttributes.
 * \param handle Core Audio Handle.
 * \param rb Ring Buffer
 * \param no_of_channels Number of channels for the device.
 * \return 0 Okay, otherwise error.
 **************************************************************************************/
int ca_set_ring_buffer_attributes(CA_HANDLE handle, struct st_ring_buffer * rb, size_t no_of_channels)
{
    if(!handle)
        return SET_ERROR(0, CA_INVALID_HANDLE, noErr);

    if(!rb)
        return SET_ERROR(0, CA_INVALID_PARAMATER, noErr);

    rb->channels_per_frame  = no_of_channels;
    rb->frame_count         = 0;
    rb->frame_size          = rb->buffer_size / (no_of_channels * sizeof(CORE_AUDIO_DATA_TYPE));
    rb->unit_count          = 0;
    rb->unit_size           = rb->frame_size * no_of_channels;
    rb->write_pos           = 0;
    rb->read_pos            = 0;

    return CA_NO_ERROR;
}

/** ***********************************************************************************
 * \brief Open (start) playback and record audio stream.
 * \return 0 Okay, otherwise error.
 **************************************************************************************/
int ca_open(CA_HANDLE handle)
{
    OSStatus result = noErr;
    AudioDeviceID device_id = 0;
    size_t no_of_channels = 0;

    if(!handle) {
        return SET_ERROR(0, CA_INVALID_HANDLE, noErr);
    }

    struct st_core_audio_handle *ca_handle = (struct st_core_audio_handle *) handle;

    if(ca_handle->running_flag)
        ca_close(handle);

    if(ca_handle->selected_playback_device && (ca_handle->selected_playback_device->device_io & AD_PLAYBACK)) {

        // Initialize the ring buffer and ensure buffer is frame size aligned.
        no_of_channels = ca_handle->selected_playback_device->playback->no_of_channels;

        ca_set_ring_buffer_attributes(handle, ca_handle->rb_playback,     no_of_channels);
        ca_set_ring_buffer_attributes(handle, ca_handle->rb_playback_src, no_of_channels);

        if(no_of_channels < 2)
            ca_map_device_channels(handle, AD_PLAYBACK, CA_MONO_CHANNEL_INDEX, CA_MONO_CHANNEL_INDEX);
        else
            ca_map_device_channels(handle, AD_PLAYBACK, CA_LEFT_CHANNEL_INDEX, CA_RIGHT_CHANNEL_INDEX);

        device_id = ca_handle->selected_playback_device->device_id;

#if defined( AVAILABLE_MAC_OS_X_VERSION_10_6_AND_LATER )
        result = AudioDeviceCreateIOProcID(device_id, CADevicePlayback, handle,  &ca_handle->selected_playback_device->io_proc_id);
#else
        result = AudioDeviceAddIOProc(device_id, CADevicePlayback, (void *) handle);
#endif
        if(result != noErr) return 1;

        ca_handle->playback_callback = CADevicePlayback;
    }

    if(ca_handle->selected_record_device && (ca_handle->selected_record_device->device_io & AD_RECORD)) {

        // Initialize the ring buffer and ensure buffer is frame size aligned.
        no_of_channels = ca_handle->selected_record_device->record->no_of_channels;

        ca_set_ring_buffer_attributes(handle, ca_handle->rb_record,     no_of_channels);
        ca_set_ring_buffer_attributes(handle, ca_handle->rb_record_src, no_of_channels);

        if(ca_handle->selected_record_device->record->no_of_channels < 2)
            ca_map_device_channels(handle, AD_RECORD, CA_MONO_CHANNEL_INDEX, CA_MONO_CHANNEL_INDEX);
        else
            ca_map_device_channels(handle, AD_RECORD, CA_LEFT_CHANNEL_INDEX, CA_RIGHT_CHANNEL_INDEX);

        device_id = ca_handle->selected_record_device->device_id;


#if defined( AVAILABLE_MAC_OS_X_VERSION_10_6_AND_LATER )
        result = AudioDeviceCreateIOProcID(device_id, CADeviceRecord, handle,  &ca_handle->selected_record_device->io_proc_id);
#else
        result = AudioDeviceAddIOProc(device_id, CADeviceRecord, (void *) handle);
#endif
        if(result != noErr) return CA_OS_ERROR;

        ca_handle->record_callback = CADeviceRecord;
    }

    ca_initialize_converter(handle);

    ca_handle->closed_flag  = 0;

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

#if defined( AVAILABLE_MAC_OS_X_VERSION_10_6_AND_LATER )
        if(ca_handle->selected_record_device) {
            result |= AudioDeviceStart(ca_handle->selected_record_device->device_id, ca_handle->selected_record_device->io_proc_id);
        }

        if(ca_handle->selected_playback_device) {
            result |= AudioDeviceStart(ca_handle->selected_playback_device->device_id, ca_handle->selected_playback_device->io_proc_id);
        }
#else
        if(ca_handle->selected_record_device) {
            result |= AudioDeviceStart(ca_handle->selected_record_device->device_id, CADeviceRecord);
        }

        if(ca_handle->selected_playback_device) {
            result |= AudioDeviceStart(ca_handle->selected_playback_device->device_id, CADevicePlayback);
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
        return SET_ERROR(0, CA_INVALID_HANDLE, noErr);
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

    ca_release_converter(handle);

#if defined( AVAILABLE_MAC_OS_X_VERSION_10_6_AND_LATER )
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
#else
    if(ca_handle->playback_callback) {
        result |= AudioDeviceStop(ca_handle->selected_playback_device->device_id, CADevicePlayback);
        result |= AudioDeviceRemoveIOProc(ca_handle->selected_playback_device->device_id, CADevicePlayback);
        ca_handle->playback_callback = 0;
    }

    if(ca_handle->record_callback) {
        result |= AudioDeviceStop(ca_handle->selected_record_device->device_id, CADeviceRecord);
        result |= AudioDeviceRemoveIOProc(ca_handle->selected_record_device->device_id, CADeviceRecord);
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
 * \brief Select audio device by index.
 * \param handle Core Audio Handle.
 * \param index Audio device index, 1...device_count.
 * \param device_io Record or Playback device.
 * \return 0 Error, otherwise Device ID No.
 **************************************************************************************/
AudioDeviceID ca_select_device_by_index(CA_HANDLE handle, int index, int device_io)
{
    if(!handle) {
        SET_ERROR(0, CA_INVALID_HANDLE, noErr);
        return 0;
    }

    struct st_core_audio_handle *ca_handle = (struct st_core_audio_handle *) handle;
    AudioDeviceID device_id = 0;

    if(ca_handle->running_flag) {
        ca_print_msg(handle, CA_MUST_CLOSE_DEVICE_FIRST, noErr, (char *) &__func__[0]);
        return 0;
    }

    if(index > 0) index--;

    if(index < 0 || index >= ca_handle->device_list_count) {
        SET_ERROR(handle, CA_INDEX_OUT_OF_RANGE, noErr);
        return 0;
    }

    if((ca_handle->device_list[index].device_io & device_io) == 0) {
        ca_print_msg(handle, CA_INVALID_DIRECTION, noErr, ca_handle->device_list[index].device_name);
        return 0;
    }

    if(device_io & AD_PLAYBACK) {
        ca_handle->selected_playback_device = &ca_handle->device_list[index];
        device_id = ca_handle->selected_playback_device->device_id;
        ca_print_msg(handle, CA_PLAYBACK_DEVICE_SELECTED, noErr, ca_handle->selected_playback_device->device_name);

    } else if(device_io & AD_RECORD) {
        ca_handle->selected_record_device = &ca_handle->device_list[index];
        device_id = ca_handle->selected_record_device->device_id;
        ca_print_msg(handle, CA_RECORD_DEVICE_SELECTED, noErr, ca_handle->selected_record_device->device_name);
    }

    return device_id;
}

/** ***********************************************************************************
 * \brief Select audio device by name.
 * \param handle Core Audio Handle.
 * \param device_name Device name
 * \param device_io Record or Playback device.
 * \return 0 Error, otherwise Device ID No.
 **************************************************************************************/
AudioDeviceID ca_select_device_by_name(CA_HANDLE handle, char *device_name, int device_io)
{
    if(!handle) {
        SET_ERROR(0, CA_INVALID_HANDLE, noErr);
        return 0;
    }

    if(!device_name) {
        SET_ERROR(0, CA_INVALID_PARAMATER, noErr);
        return 0;
    }

    struct st_core_audio_handle *ca_handle = (struct st_core_audio_handle *) handle;
    struct st_audio_device_list * list = ca_handle->device_list;
    size_t count = ca_handle->device_list_count;
    int match = 0;
    int device_found = 0;
    AudioDeviceID device_id = 0;

    if(ca_handle->running_flag) {
        ca_print_msg(handle, CA_MUST_CLOSE_DEVICE_FIRST, noErr, (char *) &__func__[0]);
        return 0;
    }

    if(count && list) {
        for(size_t i = 0; i < count; i++) {
            match = ca_string_match((const char *) device_name, (const char *) list[i].device_name, (size_t) AUDIO_DEVICE_NAME_LIMIT);
            if(match == 0) {
                if(device_io & AD_PLAYBACK & list[i].device_io) {
                    ca_handle->selected_playback_device = &list[i];
                    device_found = 1;
                    device_id = ca_handle->selected_playback_device->device_id;
                    ca_print_msg(handle, CA_PLAYBACK_DEVICE_SELECTED, noErr, ca_handle->selected_playback_device->device_name);
                    break;
                }

                if(device_io & AD_RECORD & list[i].device_io) {
                    ca_handle->selected_record_device = &list[i];
                    device_found = 1;
                    device_id = ca_handle->selected_record_device->device_id;
                    ca_print_msg(handle, CA_RECORD_DEVICE_SELECTED, noErr, ca_handle->selected_record_device->device_name);
                    break;
                }
            }
        }
    } else {
        SET_ERROR(handle, CA_DEVICE_SELECT_ERROR, noErr);
        return 0;
    }

    if(!device_found) {
        ca_print_msg(handle, CA_DEVICE_NOT_FOUND, noErr, device_name);
        return 0;
    }

    return device_id;
}

/** ***********************************************************************************
 * \brief Select audio device by OS ID.
 * \param handle Core Audio Handle.
 * \param device_id Operating systen device ID.
 * \param device_io Record or Playback device.
 * \return 0 Okay, otherwise error
 **************************************************************************************/
int ca_select_device_by_id(CA_HANDLE handle, AudioDeviceID device_id, int device_io)
{
    if(!handle) {
        return SET_ERROR(0, CA_INVALID_HANDLE, noErr);
    }

    struct st_core_audio_handle *ca_handle = (struct st_core_audio_handle *) handle;
    struct st_audio_device_list *list_item = (struct st_audio_device_list *) 0;

    if(ca_handle->running_flag) {
        ca_print_msg(handle, CA_MUST_CLOSE_DEVICE_FIRST, noErr, (char *) &__func__[0]);
        return 0;
    }

    for(size_t i = 0; i < ca_handle->device_list_count; i++) {
        if(ca_handle->device_list[i].device_id == device_id) {
            list_item = &ca_handle->device_list[i];
            break;
        }
    }

    if(!list_item) {
        return ca_print_msg(handle, CA_DEVICE_NOT_FOUND, noErr, (char *) &__func__[0]);
    }

    switch(device_io) {
        case AD_PLAYBACK:
            if(!list_item->playback) {
                return ca_print_msg(handle, CA_INVALID_DIRECTION, noErr, (char *) "(Playback)");
            }

            ca_handle->selected_playback_device = list_item;
            break;

        case AD_RECORD:
            if(!list_item->record) {
                return ca_print_msg(handle, CA_INVALID_DIRECTION, noErr, (char *) "(Record)");
            }

            ca_handle->selected_record_device = list_item;
            break;

        default:
            return ca_print_msg(handle, CA_UNKNOWN_IO_DIRECTION, noErr, (char *) &__func__[0]);
    }

    return CA_NO_ERROR;
}


/** ***********************************************************************************
 * \brief Set the run state of record/playback audio streams.
 * \return 0 Okay, otherwise error.
 **************************************************************************************/
int ca_set_run_state(CA_HANDLE handle, enum run_pause flag)
{
    if(!handle) {
        return SET_ERROR(0, CA_INVALID_HANDLE, noErr);
    }

    struct st_core_audio_handle *ca_handle = (struct st_core_audio_handle *) handle;

    switch(flag) {
        case RS_RUN:
        case RS_PAUSE:
            break;
        default:
            return SET_ERROR(handle, CA_INVALID_PARAMATER, noErr);
    }

    ca_handle->pause_flag = flag;

    return CA_NO_ERROR;
}

/** ***********************************************************************************
 * \brief Start property listener for the specified message.
 * \return 0 Okay, otherwise error.
 **************************************************************************************/
int ca_start_listener(CA_HANDLE handle, struct ca_wait_data *wait_data)
{
    if(!handle) {
        return SET_ERROR(0, CA_INVALID_HANDLE, noErr);
    }

    if(!wait_data)
        return SET_ERROR(0, CA_INVALID_PARAMATER, noErr);

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
 * \brief Remove installed property listener for the specified message.
 * \return 0 Okay, otherwise error.
 **************************************************************************************/
int ca_remove_listener(CA_HANDLE handle, struct ca_wait_data *wait_data)
{
    if(!handle) {
        return SET_ERROR(0, CA_INVALID_HANDLE, noErr);
    }

    if(!wait_data)
        return SET_ERROR(0, CA_INVALID_PARAMATER, noErr);

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
        return SET_ERROR(0, CA_INVALID_PARAMATER, noErr);

    *record = *playback = 0;

    AudioObjectPropertyAddress property = {
        kAudioHardwarePropertyDefaultOutputDevice,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMaster
    };

    AudioDeviceID device_id = 0;
    UInt32 property_size = sizeof(AudioDeviceID);
    OSStatus os_error = noErr;

    os_error = AudioObjectGetPropertyData(kAudioObjectSystemObject, &property, 0, NULL, &property_size, &device_id);

    if(os_error == noErr)
        *playback = device_id;
    else
        return SET_ERROR(0, CA_GET_PROPERTY_DATA_ERROR, noErr);

    property.mSelector = kAudioHardwarePropertyDefaultInputDevice;

    os_error = AudioObjectGetPropertyData(kAudioObjectSystemObject, &property, 0, NULL, &property_size, &device_id);

    if(os_error == noErr)
        *record = device_id;
    else
        return SET_ERROR(0, CA_GET_PROPERTY_DATA_ERROR, noErr);

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
        SET_ERROR(0, CA_INVALID_HANDLE, noErr);
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
int ca_get_device_id_list(AudioDeviceID **array_of_ids, size_t *count)
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
        return  SET_ERROR(0, CA_INVALID_PARAMATER, noErr);

    *count = 0;
    *array_of_ids = 0;

    result = AudioObjectGetPropertyDataSize(kAudioObjectSystemObject, &property, 0, NULL, &property_size);

    if(result != noErr)
        return  SET_ERROR(0, CA_OS_ERROR, result);

    list_count  = property_size / sizeof(AudioDeviceID);
    device_list = (AudioDeviceID *) calloc(list_count + 1, sizeof(AudioDeviceID));

    if(!device_list)
        return SET_ERROR(0, CA_ALLOCATE_FAIL, noErr);

    result = AudioObjectGetPropertyData(kAudioObjectSystemObject, &property, 0, NULL, &property_size, device_list);

    if(result != noErr) {
        free(device_list);
        return  SET_ERROR(0, CA_OS_ERROR, result);
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
        return SET_ERROR(0, CA_INVALID_HANDLE, noErr);
    }

    if(!array || !no_of_channels)
        return SET_ERROR(handle, CA_INVALID_PARAMATER, noErr);

    switch(direction) {
        case AD_PLAYBACK:
        case AD_RECORD:
            break;
        default:
            return SET_ERROR(handle, CA_INVALID_PARAMATER, noErr);
    }

    char *name = (char *) 0;
    char **tmp = (char **) calloc(no_of_channels + 1, sizeof(char *));

    if(!tmp)
        return SET_ERROR(handle, CA_ALLOCATE_FAIL, noErr);

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
        return SET_ERROR(0, CA_INVALID_HANDLE, noErr);
    }

    struct st_audio_device_info *device_info = (struct st_audio_device_info  *) 0;

    if(no_of_playback_channels) {
        device_info = (struct st_audio_device_info *) calloc(1, sizeof(struct st_audio_device_info));

        if(!device_info)
            return SET_ERROR(handle, CA_ALLOCATE_FAIL, noErr);

        ca_get_sample_rates(handle, id, &device_info->sample_rates, &device_info->sample_rate_count);

        ca_get_device_channel_names(handle, id, (char **) &device_info->channel_desc, no_of_playback_channels, AD_PLAYBACK);

        ca_get_frame_range(handle, id, &device_info->max_frame_count, &device_info->min_frame_count, &device_info->allocate_frame_count);

        device_info->no_of_channels = no_of_playback_channels;
        list->playback = device_info;
        list->playback->bytes_per_frame  = no_of_playback_channels * sizeof(CORE_AUDIO_DATA_TYPE);
        list->playback->parent = list;
        list->device_io |= AD_PLAYBACK;
    }

    if(no_of_record_channels) {
        device_info = (struct st_audio_device_info *) calloc(1, sizeof(struct st_audio_device_info));

        if(!device_info)
            return SET_ERROR(handle, CA_ALLOCATE_FAIL, noErr);

        ca_get_sample_rates(handle, id, &device_info->sample_rates, &device_info->sample_rate_count);

        ca_get_device_channel_names(handle, id, (char **) &device_info->channel_desc, no_of_record_channels, AD_RECORD);

        ca_get_frame_range(handle, id, &device_info->max_frame_count, &device_info->min_frame_count, &device_info->allocate_frame_count);

        device_info->no_of_channels = no_of_record_channels;
        list->record = device_info;
        list->record->bytes_per_frame  = no_of_record_channels * sizeof(CORE_AUDIO_DATA_TYPE);
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
        SET_ERROR(0, CA_INVALID_HANDLE, noErr);
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
        SET_ERROR(handle, CA_OS_ERROR, result);
        return (char *)0;
    }

    // Allocate device name storage.
    tmp1 = (char *) calloc(AUDIO_DEVICE_NAME_LIMIT + 1, sizeof(char));

    if(!tmp1) {
        SET_ERROR(handle, CA_ALLOCATE_FAIL, noErr);
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
        SET_ERROR(0, CA_INVALID_HANDLE, noErr);
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
            SET_ERROR(handle, CA_UNKNOWN_IO_DIRECTION, noErr);
            return (char *)0;

    }

    if(max_channels < 1) {
        SET_ERROR(handle, CA_DEVICE_NO_CHANNELS, noErr);
        return (char *)0;
    }

    if((channel_index < 1) || (channel_index > max_channels)) {
        ca_print_msg(handle, CA_CHANNEL_INDEX_OUT_OF_RANGE, noErr, (char *) __func__);
        return (char *)0;
    }

    pa.mElement = (UInt32) channel_index;

    result = AudioObjectGetPropertyData(device_id, &pa, 0, (void *)0, &propSize, &cfs_channel_name);

    if(result != noErr) {
        SET_ERROR(handle, CA_OS_ERROR, result);
        return (char *)0;
    }

    channel_name = (char *) calloc(AUDIO_CHANNEL_NAME_LIMIT + 1, sizeof(char));

    if(!channel_name) {
        SET_ERROR(handle, CA_ALLOCATE_FAIL, noErr);
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
        return SET_ERROR(0, CA_INVALID_HANDLE, noErr);
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
                error = SET_ERROR(handle, CA_ALLOCATE_FAIL, noErr);
                goto _playback;
            }

            results = AudioObjectGetPropertyData(id, &properties, 0, NULL, &size, bufferList);

            if(results == noErr) {
                for(UInt32 index = 0; index < bufferList->mNumberBuffers; index++)
                    *record += bufferList->mBuffers[index].mNumberChannels;
            } else {
                error = SET_ERROR(handle, CA_OS_ERROR, results);
            }

            free(bufferList);

        }  else {
            error = SET_ERROR(handle, CA_OS_ERROR, results);
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
                error = SET_ERROR(handle, CA_ALLOCATE_FAIL, noErr);
                goto _end;
            }

            results = AudioObjectGetPropertyData(id, &properties, 0, NULL, &size, bufferList);

            if(results == noErr) {
                for(UInt32 index = 0; index < bufferList->mNumberBuffers; index++)
                    *playback += bufferList->mBuffers[index].mNumberChannels;
            } else {
                error = SET_ERROR(handle, CA_OS_ERROR, results);
            }

            free(bufferList);
        }  else {
            error = SET_ERROR(handle, CA_OS_ERROR, results);
        }
    }

_end:;

    return error;
}

/** ****************************************************************************
 * \brief Set the sample rate of the audio device in question and wait for
 * async response.
 * \param handle Core Audio handle.
 * \param io_device Playback/Record
 * \param sample_rate Sample rate value
 * \return 0 if okay, otherwise error.
 *******************************************************************************/
int ca_set_sample_rate(CA_HANDLE handle, int io_device, Float64 sample_rate)
{
    if(!handle) {
        return SET_ERROR(0, CA_INVALID_HANDLE, noErr);
    }

    OSStatus result = noErr;

    AudioObjectPropertyAddress properties = {
        kAudioDevicePropertyNominalSampleRate,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMaster
    };

    struct ca_wait_data *wait_data = (struct ca_wait_data *) calloc(1, sizeof(struct ca_wait_data));
    struct st_core_audio_handle *ca_handle = (struct st_core_audio_handle *) handle;
    struct st_audio_device_info *device_info = (struct st_audio_device_info *) 0;
    Float64 harmonic_sample_rate = 0.0;
    Float64 matching_sample_rate = 0.0;
    Float64 higher_sample_rate   = 0.0;
    Float64 sr_value = 0;
    size_t  tmp = 0;
    Float64 fraction = 0;
    Float64 whole    = 0;

    AudioDeviceID device_id = 0;

    if(!wait_data) return SET_ERROR(handle, CA_ALLOCATE_FAIL, noErr);

    switch(io_device) {
        case AD_PLAYBACK:
            if(!ca_handle->selected_playback_device)
                return SET_ERROR(handle, CA_DEVICE_SELECT_ERROR, noErr);
            device_id   = ca_handle->selected_playback_device->device_id;
            device_info = ca_handle->selected_playback_device->playback;
            ca_handle->playback_sample_rate_convert = 0;
            break;

        case AD_RECORD:
            if(!ca_handle->selected_record_device)
                return SET_ERROR(handle, CA_DEVICE_SELECT_ERROR, noErr);
            device_id = ca_handle->selected_record_device->device_id;
            device_info = ca_handle->selected_record_device->record;
            ca_handle->record_sample_rate_convert = 0;
            break;
    }

    // Test requested sample rate to the supported sample rates.
    // Select one best suited for the low quaility converter if
    // a match can not be found.

    for(size_t i = 0; i < device_info->sample_rate_count; i++) {
        sr_value = device_info->sample_rates[i];
        if(sample_rate == sr_value) {
            matching_sample_rate = sample_rate;
            break;
        }

        if(higher_sample_rate == 0.0 && sr_value > sample_rate)
            higher_sample_rate = sr_value;

        if(harmonic_sample_rate == 0.0 && sr_value > sample_rate) {
            whole = sr_value / sample_rate;
            tmp = whole;
            fraction = whole - tmp;
            if(fraction == 0.0)
                harmonic_sample_rate = sr_value;
        }
    }

    if(matching_sample_rate == sample_rate) {
        sr_value = matching_sample_rate;
    } else if(harmonic_sample_rate) {
        sr_value = harmonic_sample_rate;
    } else if(higher_sample_rate) {
        sr_value = higher_sample_rate;
    } else {
        // default to what ever it's set to.
        ca_get_sample_rate(handle, io_device, &sr_value);
    }

    memcpy((void *) &wait_data->properties, (void *) &properties, sizeof(AudioObjectPropertyAddress));
    wait_data->handle    = (struct st_core_audio_handle *) handle;
    wait_data->device_id = device_id;

    ca_start_listener(handle, wait_data);

    result = AudioObjectSetPropertyData(device_id, &properties, 0, NULL, sizeof(Float64), &sr_value);

    ca_wait_for_os(handle, 1.5);

    ca_remove_listener(handle, wait_data);

    free(wait_data);

    ca_get_sample_rate(handle, io_device, &sr_value);

    switch(io_device) {
        case AD_PLAYBACK:
            ca_handle->requested_playback_sample_rate = sample_rate;
            ca_handle->hardware_playback_sample_rate  = sr_value;
            break;

        case AD_RECORD:
            ca_handle->requested_record_sample_rate = sample_rate;
            ca_handle->hardware_record_sample_rate  = sr_value;
            break;
    }

    return result;
}

/** ****************************************************************************
 * \brief Return the samplerate of the audio device in question.
 * \param device_id Operating System device ID
 * \param sample_rate The sample rate curreently set fot he device.
 * \return true if okay, flase if failed.
 *******************************************************************************/
int ca_get_sample_rate(CA_HANDLE handle, int io_device, Float64 *sample_rate)
{
    if(!handle) {
        return SET_ERROR(0, CA_INVALID_HANDLE, noErr);
    }

    OSStatus result = noErr;
    UInt32 property_size = sizeof(Float64);
    struct st_core_audio_handle *ca_handle = (struct st_core_audio_handle *) handle;
    AudioDeviceID device_id = 0;

    AudioObjectPropertyAddress properties = {
        kAudioDevicePropertyActualSampleRate,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMaster
    };

    if(sample_rate) {
        *sample_rate = 0;

        switch(io_device) {
            case AD_PLAYBACK:
                if(ca_handle->selected_playback_device)
                    device_id = ca_handle->selected_playback_device->device_id;
                else
                    result = CA_INVALID_PARAMATER;
                break;

            case AD_RECORD:
                if(ca_handle->selected_record_device)
                    device_id = ca_handle->selected_record_device->device_id;
                else
                    result = CA_INVALID_PARAMATER;
                break;

            default:
                return SET_ERROR(handle, CA_UNKNOWN_IO_DIRECTION, noErr);

        }

        result = AudioObjectGetPropertyData(device_id, &properties, 0, NULL, &property_size, sample_rate);

    } else {
        result = SET_ERROR(handle, CA_OS_ERROR, result);
    }

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
        return SET_ERROR(0, CA_INVALID_HANDLE, noErr);
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
        return SET_ERROR(handle, CA_INVALID_PARAMATER, noErr);

    *sample_rates = (Float64 *) 0;

    result = AudioObjectGetPropertyDataSize(device_id, &properties, 0, NULL, &property_size);

    if(result != noErr)
        return SET_ERROR(handle, CA_OS_ERROR, result);

    _count = property_size / unit_size;

    if(!_count)
        return SET_ERROR(handle, CA_INVALID_PARAMATER, noErr);

    tmp = (Float64 *) calloc(_count + 1, unit_size);

    if(!tmp)
        return SET_ERROR(handle, CA_ALLOCATE_FAIL, noErr);

    result = AudioObjectGetPropertyData(device_id, &properties, 0, NULL, &property_size, tmp);

    if(result != noErr)
        return SET_ERROR(handle, CA_OS_ERROR, result);

    *count = _count;
    *sample_rates = tmp;

    return CA_NO_ERROR;
}

/** ***********************************************************************************
 * \brief Set the converter quaility. Must be called after ca_open() and 
 * before ca_run()
 * \param handle Core audio handle
 * \param io_device device to configure.
 * \param quaility quaility of converted data.
 * \return 0 okay, otherwise error.
 **************************************************************************************/
int ca_set_converter_quaility(CA_HANDLE handle, int io_device, int quaility)
{
    if(!handle) {
        return SET_ERROR(0, CA_INVALID_HANDLE, noErr);
    }

    struct st_core_audio_handle *ca_handle = (struct st_core_audio_handle *) handle;

    UInt32 property_id   = 0;
    UInt32 property_data = 0;
    UInt32 property_size = 0;
    int converter_in_use = 0;
    int selector = 0;
    int valid_data = 0;
    int status = CA_NO_ERROR;
    AudioConverterRef sr_converter = (AudioConverterRef) 0;
    OSStatus os_status = noErr;

    switch(io_device) {
        case AD_PLAYBACK_SRC:
        case AD_PLAYBACK:
            sr_converter = ca_handle->playback_sr_converter;
            converter_in_use = ca_handle->playback_sample_rate_convert;
            break;

        case AD_RECORD_SRC:
        case AD_RECORD:
            sr_converter = ca_handle->record_sr_converter;
            converter_in_use = ca_handle->record_sample_rate_convert;
           break;

        default:
            return SET_ERROR(handle, CA_INVALID_PARAMATER, noErr);
    }

    if(!converter_in_use)
        return noErr;

    if(!sr_converter) {
        return SET_ERROR(handle, CA_INVALID_PARAMATER, noErr);
    }

    selector = quaility & CA_CONV_QTY_MASK;

    if(selector) {
        valid_data = 1;

        switch(selector) {
            case CA_CONV_MAX:
                property_data = kAudioConverterQuality_Max;
                break;

            case CA_CONV_HIGH:
                property_data = kAudioConverterQuality_High;
                break;

            case CA_CONV_MED:
                property_data = kAudioConverterQuality_Medium;
                break;

            case CA_CONV_LOW:
                property_data = kAudioConverterQuality_Low;
                break;

            case CA_CONV_MIN:
                property_data = kAudioConverterQuality_Min;
                break;

            default:
                valid_data = 0;
                SET_ERROR(handle, CA_INVALID_PARAMATER, noErr);
        }

        if(valid_data) {
            property_id   = kAudioConverterSampleRateConverterQuality;
            property_size = (UInt32) sizeof(UInt32);

            os_status = AudioConverterSetProperty(sr_converter, property_id, property_size,
                                                  (const void *) &property_data);
            if(os_status)
                status = SET_ERROR(handle, CA_OS_ERROR, os_status);
        }
    }

#if defined( MAC_OS_X_VERSION_10_5 )
    selector = quaility & CA_CONV_CPLX_MASK;

    if(selector) {
        valid_data = 1;

        switch(selector) {
            case CA_CONV_LINEAR:
                property_data = kAudioConverterSampleRateConverterComplexity_Linear;
                break;

            case CA_CONV_NORMAL:
                property_data = kAudioConverterSampleRateConverterComplexity_Normal;
                break;

            case CA_CONV_MASTERING:
                property_data = kAudioConverterSampleRateConverterComplexity_Mastering;
                break;

            default:
                valid_data = 0;
                SET_ERROR(handle, CA_INVALID_PARAMATER, noErr);
        }

        if(valid_data) {
            property_id   = kAudioConverterSampleRateConverterComplexity;
            property_size = (UInt32) sizeof(UInt32);

            os_status = AudioConverterSetProperty(sr_converter, property_id, property_size,
                                                  (const void *) &property_data);
            if(os_status)
                status = SET_ERROR(handle, CA_OS_ERROR, os_status);
        }
    }
#endif // #if defined( AVAILABLE_MAC_OS_X_VERSION_10_5_AND_LATER )

    return status;
}

#if 0
/** ***********************************************************************************
 * \brief Get the input and outout converter quaility.
 * \param handle Core audio handle
 * \param quaility quaility of converted data.
 * \return 0 okay, otherwise error.
 **************************************************************************************/
int ca_get_converter_quaility(CA_HANDLE *handle, , enum ca_convert_quaility *record_quaility,  enum ca_convert_quaility *playback_quaility)
{
    if(!handle) {
        return SET_ERROR(0, CA_INVALID_HANDLE, noErr);
    }

    OSStatus os_status = noErr;

    extern OSStatus
    AudioConverterGetPropertyInfo(  AudioConverterRef           inAudioConverter,
                                  AudioConverterPropertyID    inPropertyID,
                                  UInt32*                     outSize,
                                  Boolean*                    outWritable)        __OSX_AVAILABLE_STARTING(__MAC_10_1,__IPHONE_2_0);

    AudioConverterGetProperty(  AudioConverterRef           inAudioConverter,
                              AudioConverterPropertyID    inPropertyID,
                              UInt32*                     ioPropertyDataSize,
                              void*                       outPropertyData)        __OSX_AVAILABLE_STARTING(__MAC_10_1,__IPHONE_2_0);

    return CA_NO_ERROR;
}
#endif // #if 0

/** ***********************************************************************************
 * \brief Wait for the operation system to do it's job.
 * \param selector Message type to wait for.
 * \param timeout Limit wait time.
 * \return 0 okay,  ETIMEDOUT/Other Error.
 **************************************************************************************/
int ca_wait_for_os(CA_HANDLE handle, float timeout)
{
    if(!handle) {
        return SET_ERROR(0, CA_INVALID_HANDLE, noErr);
    }

    struct timespec ts;
    struct timeval tp;
    unsigned long seconds = 0;
    unsigned long microseconds = 0;
    float tmp = 0.0;
    int return_state = 0;

    struct st_core_audio_handle *ca_handle = (struct st_core_audio_handle *) handle;

    if(ca_handle->exit_flag) return CA_NO_ERROR;

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
        return SET_ERROR(0, CA_INVALID_HANDLE, noErr);
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
int ca_set_error(CA_HANDLE handle, const char *fn, int line_number, int error_no, OSStatus os_error_no)
{
    struct st_core_audio_handle *ca_handle = (struct st_core_audio_handle *) handle;

    if(ca_handle) {
        ca_handle->last_error_code = ca_handle->error_code;
        ca_handle->error_code = error_no;
    }

    if(error_no < 0 || error_no > CA_INVALID_ERROR) error_no = CA_INVALID_ERROR;

#if BUILD_FLDIGI
    if ((debug::ERROR_LEVEL <= debug::level) && (log_source_ & debug::mask))
        debug::log(debug::ERROR_LEVEL, fn, __FILE__, line_number, "Core Audio: %s", ca_error_message(handle, error_no, os_error_no));
#else
    printf("Function: %s\nFile: %s\nLine No: %d\nCore Audio Error: %s\n", fn, __FILE__, line_number, ca_error_message(handle, error_no, os_error_no));
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
int ca_print_msg(CA_HANDLE handle, int error_no, OSStatus os_error_no, char *string)
{
    if(error_no < 0 || error_no > CA_INVALID_ERROR) error_no = CA_INVALID_ERROR;

    if(!string)
        string = (char *) "";
    int limit = 1024;
    char *message = (char *) calloc(limit + 1, sizeof(char));
    char os_string[8];
    char *tmp = (char *) 0;

    if(message) {
        snprintf(message, limit, "Core Audio: %s (%s)", msg_string[error_no], string);
        if(os_error_no) {
            memset(os_string, 0, sizeof(os_string));
            tmp = string_32(os_error_no, os_string, sizeof(os_string) - 1);
            if(tmp)
                snprintf(message, limit, "\nOS Error Code: \'%s\' (%ld)\n", tmp, (long int) os_error_no);
        }

#if BUILD_FLDIGI
        if ((debug::INFO_LEVEL <= debug::level) && (log_source_ & debug::mask))
            debug::log(debug::INFO_LEVEL, fn, __FILE__, line_number, "%s", message);
#else
        printf( "%s\n", message);
#endif
        free(message);
    }

    return error_no;
}


/** ***********************************************************************************
 * \brief Converison scale between internal and external samples.
 * \param scale Convert selector.
 **************************************************************************************/
void ca_format_scale(CA_HANDLE handle, enum host_data_types data_type)
{
    if(!handle) {
        SET_ERROR(0, CA_INVALID_HANDLE, noErr);
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
const char * ca_error_message(CA_HANDLE handle, int error_no, OSStatus os_error)
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
        return SET_ERROR(0, CA_INVALID_HANDLE, noErr);
    }

    int error = CA_NO_ERROR;

    struct st_ring_buffer * rb = ca_device_to_ring_buffer(handle, io_device, AD_SRC_NO);

    if(!rb)
        return SET_ERROR(handle, CA_INVALID_PARAMATER, noErr);

    pthread_mutex_lock(&rb->io_mutex);

    if(used_count) *used_count = 0;
    if(free_count) *free_count = 0;

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

    if(used_count) *used_count = rb->frame_count;
    if(free_count) *free_count = rb->frame_size - rb->frame_count;

    pthread_mutex_unlock(&rb->io_mutex);

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
    if(!rb) return SET_ERROR(handle, CA_INVALID_PARAMATER, noErr);

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
        return SET_ERROR(handle, CA_INVALID_PARAMATER, noErr);

    if((rb->unit_count >= rb->unit_size) || (rb->frame_count >= rb->frame_size))
        return SET_ERROR(handle, CA_BUFFER_STALLED, noErr);

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
        return SET_ERROR(0, CA_INVALID_HANDLE, noErr);
    }

    struct st_ring_buffer *rb = ca_device_to_ring_buffer(handle, io_device, AD_SRC_NO);
    struct st_core_audio_handle *ca_handle = (struct st_core_audio_handle *) handle;
    int error = CA_NO_ERROR;

    if(!ca_handle->okay_to_run)
        return SET_ERROR(handle, CA_INITIALIZE_FAIL, noErr);

    if(rb) {
        pthread_mutex_lock(&rb->io_mutex);

        rb->buffer[rb->write_pos + rb->left_channel_index]  = (CORE_AUDIO_DATA_TYPE) left;
        rb->buffer[rb->write_pos + rb->right_channel_index] = (CORE_AUDIO_DATA_TYPE) right;

        ca_inc_write_pos(handle, rb);

        ca_test_rb_buffers(handle, rb);

        pthread_mutex_unlock(&rb->io_mutex);
    } else {
        error = CA_INVALID_PARAMATER;
    }

    if(error)
        SET_ERROR(handle, error, noErr);

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
        return SET_ERROR(0, CA_INVALID_HANDLE, noErr);
    }

    long head_count = 0;
    long tail_count = 0;
    long free_count = 0;
    size_t amt = 0;
    CORE_AUDIO_DATA_TYPE *data = (CORE_AUDIO_DATA_TYPE *) buffer;

    struct st_ring_buffer *rb = ca_device_to_ring_buffer(handle, io_device, AD_SRC_NO);

    if(amount_written)
        *amount_written = 0;

    if(!rb || !data)
        return SET_ERROR(handle, CA_INVALID_PARAMATER, noErr);

    pthread_mutex_lock(&rb->io_mutex);

    if(rb->unit_count >= rb->unit_size) {
        pthread_mutex_unlock(&rb->io_mutex);
        return CA_BUFFER_FULL;
    }

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
        amt             += head_count;
        rb->unit_count  += head_count;
        rb->frame_count  = rb->unit_count / rb->channels_per_frame;
    }

    if(tail_count > 0) {
        if(rb->write_pos >= rb->unit_size) rb->write_pos = 0;
        memcpy((void *) &rb->buffer[rb->write_pos], (void *) &data[amt], tail_count * sizeof(CORE_AUDIO_DATA_TYPE));
        rb->write_pos   += tail_count;
        amt             += tail_count;
        rb->unit_count  += tail_count;
        rb->frame_count  = rb->unit_count / rb->channels_per_frame;
    }

    if(amount_written)
        *amount_written = amt;

    ca_test_rb_buffers(handle, rb);

    pthread_mutex_unlock(&rb->io_mutex);

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
        return SET_ERROR(0, CA_INVALID_HANDLE, noErr);
    }

    struct st_core_audio_handle *ca_handle = (struct st_core_audio_handle *) handle;
    int error = CA_NO_ERROR;

    if(!ca_handle->okay_to_run)
        return SET_ERROR(handle, CA_INITIALIZE_FAIL, noErr);


    struct st_ring_buffer * rb = ca_device_to_ring_buffer(handle, io_device, AD_SRC_MAYBE);

    if(rb) {
        pthread_mutex_lock(&rb->io_mutex);

        rb->buffer[rb->write_pos + rb->left_channel_index]  = (CORE_AUDIO_DATA_TYPE) ((left)  * ca_handle->inOSScale) - ca_handle->bias;
        rb->buffer[rb->write_pos + rb->right_channel_index] = (CORE_AUDIO_DATA_TYPE) ((right) * ca_handle->inOSScale) - ca_handle->bias;

        error = ca_inc_write_pos(handle, rb);

        if(error)
            SET_ERROR(handle, error, noErr);

        ca_test_rb_buffers(handle, rb);

        pthread_mutex_unlock(&rb->io_mutex);

    } else {
        error = SET_ERROR(handle, CA_INVALID_PARAMATER, noErr);
    }



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
        return SET_ERROR(0, CA_INVALID_HANDLE, noErr);
    }

    *amount_written = 0;

    struct st_core_audio_handle *ca_handle = (struct st_core_audio_handle *) handle;
    int error = CA_NO_ERROR;
    if(!ca_handle->okay_to_run)
        return SET_ERROR(handle, CA_INITIALIZE_FAIL, noErr);

    struct st_ring_buffer * rb = ca_device_to_ring_buffer(handle, io_device, AD_SRC_MAYBE);

    if(!amount_to_write) return CA_NO_ERROR;

    if(!rb) return SET_ERROR(handle, CA_INVALID_PARAMATER, noErr);

    pthread_mutex_lock(&rb->io_mutex);

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
        }
    }

    ca_test_rb_buffers(handle, rb);

    pthread_mutex_unlock(&rb->io_mutex);



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
        return SET_ERROR(0, CA_INVALID_HANDLE, noErr);
    }

    struct st_core_audio_handle *ca_handle = (struct st_core_audio_handle *) handle;
    int error = CA_NO_ERROR;

    struct st_ring_buffer * rb = ca_device_to_ring_buffer(handle, io_device, AD_SRC_NO);

    if(rb) {
        pthread_mutex_lock(&rb->io_mutex);

        *left  = (HOST_AUDIO_DATA_TYPE) ((rb->buffer[rb->read_pos + rb->left_channel_index]  + ca_handle->bias) * ca_handle->outOSScale);
        *right = (HOST_AUDIO_DATA_TYPE) ((rb->buffer[rb->read_pos + rb->right_channel_index] + ca_handle->bias) * ca_handle->outOSScale);

        error = ca_inc_read_pos(handle, rb);

        pthread_mutex_unlock(&rb->io_mutex);
    } else {
        error = CA_INVALID_PARAMATER;
    }

    if(error)
        SET_ERROR(handle, error, noErr);

    pthread_mutex_unlock(&rb->io_mutex);

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
        return SET_ERROR(0, CA_INVALID_HANDLE, noErr);
    }

    if(amount_read) *amount_read = 0;

    struct st_core_audio_handle *ca_handle = (struct st_core_audio_handle *) handle;

    int error = CA_NO_ERROR;
    size_t amt = 0;

    if(!ca_handle->okay_to_run)
        return SET_ERROR(handle, CA_INITIALIZE_FAIL, noErr);


    if(!amount_to_read) return CA_NO_ERROR;

    struct st_ring_buffer * rb = ca_device_to_ring_buffer(handle, io_device, AD_SRC_NO);


    if(!rb) return SET_ERROR(handle, CA_INVALID_PARAMATER, noErr);

    pthread_mutex_lock(&rb->io_mutex);

    size_t index = 0;
    size_t dst_index = 0;

    if(amount_to_read > rb->frame_count)
        amount_to_read = rb->frame_count;

    for(index = 0; index < amount_to_read; index++) {
        buffer[dst_index++] = (HOST_AUDIO_DATA_TYPE) (rb->buffer[rb->read_pos + rb->left_channel_index]  + ca_handle->bias) * ca_handle->outOSScale;
        buffer[dst_index++] = (HOST_AUDIO_DATA_TYPE) (rb->buffer[rb->read_pos + rb->right_channel_index] + ca_handle->bias) * ca_handle->outOSScale;
        amt++;

        if(ca_inc_read_pos(handle, rb))
            error = CA_BUFFER_EMPTY;
    }

    if(amount_read) *amount_read = amt;

    pthread_mutex_unlock(&rb->io_mutex);

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
        return SET_ERROR(0, CA_INVALID_HANDLE, noErr);
    }

    struct st_core_audio_handle *ca_handle = (struct st_core_audio_handle *) handle;

    if(!ca_handle->okay_to_run)
        return SET_ERROR(handle, CA_INITIALIZE_FAIL, noErr);

    struct st_ring_buffer * rb = ca_device_to_ring_buffer(handle, io_device, AD_SRC_NO);

    if(!rb)
        return SET_ERROR(handle, CA_INVALID_PARAMATER, noErr);

    pthread_mutex_lock(&rb->io_mutex);

    *left  = (CORE_AUDIO_DATA_TYPE) rb->buffer[rb->read_pos + rb->left_channel_index];
    *right = (CORE_AUDIO_DATA_TYPE) rb->buffer[rb->read_pos + rb->right_channel_index];

    int error = ca_inc_read_pos(handle, rb);
    pthread_mutex_unlock(&rb->io_mutex);

    if(error)
        SET_ERROR(handle, error, noErr);

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
        return SET_ERROR(0, CA_INVALID_HANDLE, noErr);
    }

    long head_count = 0;
    long tail_count = 0;
    long free_count = 0;
    size_t amt = 0;

    CORE_AUDIO_DATA_TYPE *data = (CORE_AUDIO_DATA_TYPE *) buffer;
    struct st_ring_buffer * rb = (struct st_ring_buffer *) 0;

    if(amount_read) *amount_read = 0;

    rb = ca_device_to_ring_buffer(handle, io_device, AD_SRC_NO);

    if(!rb || !data)
        return SET_ERROR(handle, CA_INVALID_PARAMATER, noErr);

    if(rb->unit_count < 1)
        return SET_ERROR(handle, CA_BUFFER_EMPTY, noErr);

    pthread_mutex_lock(&rb->io_mutex);

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
        amt             += head_count;
        rb->unit_count -= head_count;
        if(rb->read_pos >= rb->unit_size) rb->read_pos = 0;
        rb->frame_count = rb->unit_count / rb->channels_per_frame;
    }

    if(tail_count > 0) {
        memcpy((void *) &data[amt], (void *) &rb->buffer[rb->read_pos], (size_t) (tail_count * sizeof(CORE_AUDIO_DATA_TYPE)));
        rb->read_pos   += tail_count;
        amt   += tail_count;
        rb->unit_count -= tail_count;
        if(rb->read_pos >= rb->unit_size) rb->read_pos = 0;
        rb->frame_count = rb->unit_count / rb->channels_per_frame;
    }

    if(amount_read) *amount_read = amt;

    pthread_mutex_unlock(&rb->io_mutex);

    return CA_NO_ERROR;
}

/** ***********************************************************************************
 * \brief Clear the ring buffer for playback and record.
 * \return 0 Okay, otherwise error.
 **************************************************************************************/
int ca_flush_audio_stream(CA_HANDLE handle)
{
    if(!handle) {
        return SET_ERROR(0, CA_INVALID_HANDLE, noErr);
    }

    struct st_core_audio_handle *ca_handle = (struct st_core_audio_handle *) handle;

    int old_pause_flag = ca_handle->pause_flag;

    ca_handle->pause_flag = 1;

    if(ca_handle->rb_record) {
        pthread_mutex_unlock(&ca_handle->rb_record->io_mutex);
        ca_handle->rb_record->write_pos     = (size_t) 0;
        ca_handle->rb_record->read_pos      = (size_t) 0;
        ca_handle->rb_record->unit_count    = (size_t) 0;
        ca_handle->rb_record->frame_count   = (size_t) 0;
        pthread_mutex_unlock(&ca_handle->rb_record->io_mutex);
    }

    if(ca_handle->rb_playback) {
        pthread_mutex_unlock(&ca_handle->rb_playback->io_mutex);
        ca_handle->rb_playback->write_pos   = (size_t) 0;
        ca_handle->rb_playback->read_pos    = (size_t) 0;
        ca_handle->rb_playback->unit_count  = (size_t) 0;
        ca_handle->rb_playback->frame_count = (size_t) 0;
        pthread_mutex_unlock(&ca_handle->rb_playback->io_mutex);
    }

    /*
     extern OSStatus
     AudioConverterReset(    AudioConverterRef   inAudioConverter)                   __OSX_AVAILABLE_STARTING(__MAC_10_1,__IPHONE_2_0);
     */
    ca_handle->pause_flag = old_pause_flag;

    return CA_NO_ERROR;
}

/** ****************************************************************************
 * \brief Return ring buffer pointer from device io selector
 * \return struct st_ring_buffer *, null = error
 *******************************************************************************/
struct st_ring_buffer * ca_device_to_ring_buffer(CA_HANDLE handle, int io_device, int src_flag)
{
    if(!handle) {
        SET_ERROR(0, CA_INVALID_HANDLE, noErr);
        return (struct st_ring_buffer *)0;
    }

    struct st_ring_buffer * _rb = 0;
    struct st_core_audio_handle *ca_handle = (struct st_core_audio_handle *) handle;

    switch(io_device) {
        case AD_RECORD:
            if(src_flag)
                _rb = ca_handle->rb_record_src;
            else
                _rb = ca_handle->rb_record;

            break;

        case AD_PLAYBACK:
            if(src_flag)
                _rb = ca_handle->rb_playback_src;
            else
                _rb = ca_handle->rb_playback;

            break;

        case AD_RECORD_SRC:
            _rb = ca_handle->rb_record_src;
            break;

        case AD_PLAYBACK_SRC:
            _rb = ca_handle->rb_playback_src;
            break;
    }

    return _rb;
}

/** ***********************************************************************************
 * \brief Allocate and initialize Ring buffer.
 * \return RingBuffer * pointer or null on error.
 **************************************************************************************/
struct st_ring_buffer * ca_init_ring_buffer(CA_HANDLE handle, size_t allocate_element_count)
{
    if(!handle) {
        SET_ERROR(0, CA_INVALID_HANDLE, noErr);
        return (struct st_ring_buffer *)0;
    }

    size_t data_size = sizeof(CORE_AUDIO_DATA_TYPE);

    struct st_ring_buffer *tmp = (struct st_ring_buffer *) calloc(1, sizeof(struct st_ring_buffer));

    if(!tmp) {
        SET_ERROR(handle, CA_ALLOCATE_FAIL, noErr);
        return (struct st_ring_buffer *)0;
    }

    if(allocate_element_count < 1024)
        allocate_element_count = 1024;

    tmp->buffer = (CORE_AUDIO_DATA_TYPE *) calloc(allocate_element_count + 1, data_size);

    if(!tmp->buffer) {
        free(tmp);
        SET_ERROR(handle, CA_ALLOCATE_FAIL, noErr);
        return (struct st_ring_buffer *)0;
    }

    tmp->write_pos       = 0;
    tmp->read_pos        = 0;
    tmp->buffer_size     = allocate_element_count * data_size;
    tmp->unit_size       = allocate_element_count;

    pthread_mutex_init(&tmp->io_mutex, NULL);
    pthread_cond_init(&tmp->io_cond,   NULL);

    ca_format_scale(handle, ADF_FLOAT);

    return tmp;
}

/** ***********************************************************************************
 * \brief Signal repsonsible party when available data reaches the minimum threshold.
 **************************************************************************************/
static inline int ca_test_rb_buffers(CA_HANDLE handle, struct st_ring_buffer *rb)
{
    size_t frame_count = rb->frame_count;
    struct st_core_audio_handle *ca_handle = (struct st_core_audio_handle *) handle;

    if(frame_count > CA_MIN_FRAME_COUNT) {
        if(rb == ca_handle->rb_record)
            pthread_cond_signal(&ca_handle->record_cond);
        else if(rb == ca_handle->rb_playback)
            pthread_cond_signal(&ca_handle->playback_cond);
        else if(rb == ca_handle->rb_playback_src)
            pthread_cond_signal(&ca_handle->playback_convert_cond);
        else if(rb == ca_handle->rb_record_src)
            pthread_cond_signal(&ca_handle->record_convert_cond);
    }

    return CA_NO_ERROR;
}

/** ***********************************************************************************
 * \brief Deallocate Ring buffer.
 * \return void
 **************************************************************************************/
void ca_release_ringbuffer(struct st_ring_buffer * ring_buffer)
{
    if(!ring_buffer) {
        SET_ERROR(0, CA_INVALID_PARAMATER, noErr);
        return;
    }

    pthread_mutex_destroy(&ring_buffer->io_mutex);
    pthread_cond_destroy(&ring_buffer->io_cond);

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
        return SET_ERROR(0, CA_INVALID_HANDLE, noErr);
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
                return ca_print_msg(handle, CA_INVALID_PARAMATER, noErr, (char *) "Playback device not selected");

            if(!device->playback)
                return ca_print_msg(handle, CA_INVALID_PARAMATER, noErr, (char *) &__func__[0]);

            device_info = device->playback;
            no_of_channels = device_info->no_of_channels;

            if(!ca_handle->rb_playback)
                return ca_print_msg(handle, CA_INVALID_PARAMATER, noErr, (char *) "Playback Ring buffer not allocated");

            rb = ca_handle->rb_playback;

            break;

        case AD_RECORD:
            device = ca_handle->selected_record_device;
            if(!device)
                return ca_print_msg(handle, CA_INVALID_PARAMATER, noErr, (char *) "Record device not selected");

            if(!device->record)
                return ca_print_msg(handle, CA_INVALID_PARAMATER, noErr, (char *) &__func__[0]);

            device_info = device->record;
            no_of_channels = device_info->no_of_channels;

            if(!ca_handle->rb_record)
                return ca_print_msg(handle, CA_INVALID_PARAMATER, noErr, (char *) "Record Ring buffer not allocated");

            rb = ca_handle->rb_record;
            break;
    }

    if(left_index >= no_of_channels) {
        ca_print_msg(handle, CA_INDEX_OUT_OF_RANGE, noErr, (char *) "Left Channel");
        if(no_of_channels > 0)
            left_index = no_of_channels - 1;
    }

    if(right_index >= no_of_channels) {
        ca_print_msg(handle, CA_INDEX_OUT_OF_RANGE, noErr, (char *) "Right Channel");
        if(no_of_channels > 0)
            right_index = no_of_channels - 1;
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
 * \brief Returns a structure array of devices. Use ca_free_device_list() to free memory.
 * \param handle Core Audio Handle
 * \return struct st_device_list * pointer, or null on error.
 **************************************************************************************/
struct st_device_list * ca_get_device_list(CA_HANDLE handle)
{
    if(!handle) {
        SET_ERROR(0, CA_INVALID_HANDLE, noErr);
        return (struct st_device_list *) 0;
    }

    struct st_core_audio_handle *ca_handle       = (struct st_core_audio_handle *) handle;
    struct st_device_list * list                 = (struct st_device_list *) 0;
    struct st_audio_device_list **_record_list   = (struct st_audio_device_list **) 0;
    struct st_audio_device_list **_playback_list = (struct st_audio_device_list **) 0;
    struct st_audio_device_list *device_list     = (struct st_audio_device_list *)  0;

    size_t list_count             = 0;
    size_t no_of_record_devices   = 0;
    size_t no_of_playback_devices = 0;

    if(ca_handle->device_list) {
        device_list = ca_handle->device_list;
        list_count  = ca_handle->device_list_count;

        list = (struct st_device_list *) calloc(1, sizeof(struct st_device_list));
        if(!list)
            goto _end_in_fail;

        _record_list = (struct st_audio_device_list **) calloc(list_count, sizeof(struct st_audio_device_list *));
        if(!_record_list)
            goto _end_in_fail;

        _playback_list = (struct st_audio_device_list **) calloc(list_count, sizeof(struct st_audio_device_list *));
        if(!_playback_list)
            goto _end_in_fail;

        for(size_t i = 0; i < list_count; i++) {
            if(ca_handle->device_list[i].device_io & AD_PLAYBACK) {
                _playback_list[no_of_playback_devices] = &device_list[i];
                no_of_playback_devices++;
            }

            if(ca_handle->device_list[i].device_io & AD_RECORD) {
                _record_list[no_of_record_devices] = &device_list[i];
                no_of_record_devices++;
            }
        }

        if(no_of_playback_devices) {
            list->playback_count = no_of_playback_devices;

            list->playback = (struct st_device_info *) calloc(no_of_playback_devices + 1, sizeof(struct st_device_info));
            if(!list->playback)
                goto _end_in_fail;

            for(size_t i = 0; i < no_of_playback_devices; i++ ) {
                strncpy(list->playback[i].device_name, _playback_list[i]->device_name, AUDIO_DEVICE_NAME_LIMIT);
                list->playback[i].no_of_channels = 0;
                list->playback[i].device_index   = _playback_list[i]->device_index;
                list->playback[i].device_id      = _playback_list[i]->device_id;

                for(size_t j = 0; j < _playback_list[i]->playback->no_of_channels; j++) {
                    list->playback[i].channel_desc[j] = (char *) calloc(AUDIO_CHANNEL_NAME_LIMIT, sizeof(char));

                    if(!list->playback[i].channel_desc[j])
                        goto _end_in_fail;

                    strncpy(list->playback[i].channel_desc[j], _playback_list[i]->playback->channel_desc[j], AUDIO_CHANNEL_NAME_LIMIT);
                    list->playback[i].no_of_channels++;
                }

                list->playback[i].sample_rates = (double *) calloc(_playback_list[i]->playback->sample_rate_count + 1, sizeof(double));
                if(!list->playback[i].sample_rates)
                    goto _end_in_fail;

                for(size_t j = 0; j < _playback_list[i]->playback->sample_rate_count; j++)
                    list->playback[i].sample_rates[j] = (double) _playback_list[i]->playback->sample_rates[j];

            }
        }

        if(no_of_record_devices) {
            list->record_count = no_of_record_devices;

            list->record = (struct st_device_info *) calloc(no_of_record_devices + 1, sizeof(struct st_device_info));
            if(!list->record)
                goto _end_in_fail;

            for(size_t i = 0; i < no_of_playback_devices; i++ ) {
                strncpy(list->record[i].device_name, _record_list[i]->device_name, AUDIO_DEVICE_NAME_LIMIT);
                list->record[i].no_of_channels = 0;
                list->record[i].device_index   = _record_list[i]->device_index;
                list->record[i].device_id      = _record_list[i]->device_id;

                for(size_t j = 0; j < _record_list[i]->record->no_of_channels; j++) {
                    list->record[i].channel_desc[j] = (char *) calloc(AUDIO_CHANNEL_NAME_LIMIT, sizeof(char));

                    if(!list->record[i].channel_desc[j])
                        goto _end_in_fail;

                    strncpy(list->record[i].channel_desc[j], _record_list[i]->record->channel_desc[j], AUDIO_CHANNEL_NAME_LIMIT);
                    list->record[i].no_of_channels++;
                }

                list->record[i].sample_rates = (double *) calloc(_record_list[i]->record->sample_rate_count + 1, sizeof(double));
                if(!list->record[i].sample_rates)
                    goto _end_in_fail;

                for(size_t j = 0; j < _record_list[i]->record->sample_rate_count; j++)
                    list->record[i].sample_rates[j] = (double) _record_list[i]->record->sample_rates[j];

            }
        }
    }

    if(_record_list)   free(_record_list);
    if(_playback_list) free(_playback_list);

    return list;

_end_in_fail:;

    if(_record_list)   free(_record_list);
    if(_playback_list) free(_playback_list);

    if(list)
        ca_free_device_list(handle, list);

    SET_ERROR(handle, CA_ALLOCATE_FAIL, noErr);

    return (struct st_device_list *) 0;
}

/** ***********************************************************************************
 * \brief
 *
 * \return int
 **************************************************************************************/
int ca_free_device_list(CA_HANDLE handle, struct st_device_list * list)
{
    if(!handle) {
        SET_ERROR(0, CA_INVALID_HANDLE, noErr);
    }

    int error = CA_NO_ERROR;

    if(!list)
        return SET_ERROR(handle, CA_INVALID_PARAMATER, noErr);

    if(list->record) {
        for(size_t i = 0; i < list->record_count; i++) {
            for(size_t j = 0; j < list->record[i].no_of_channels; j++) {
                if(list->record[i].channel_desc[j])
                    free(list->record[i].channel_desc[j]);
            }
            
            if(list->record[i].sample_rates)
                free(list->record[i].sample_rates);
        }
        free(list->record);
    }
    
    if(list->playback) {
        for(size_t i = 0; i < list->playback_count; i++) {
            for(size_t j = 0; j < list->playback[i].no_of_channels; j++) {
                if(list->playback[i].channel_desc[j])
                    free(list->playback[i].channel_desc[j]);
            }
            
            if(list->playback[i].sample_rates)
                free(list->playback[i].sample_rates);
        }
        free(list->playback);
    }
    
    free(list);
    
    return error;
}

/** ***********************************************************************************
 * \brief Inline condensed call to AudioConverterFillComplexBuffer()
 * \param Core Audio handle.
 * \return 0 okay, otherwise error.
 **************************************************************************************/
static inline int AudioConvertRecord(CA_HANDLE handle)
{
    struct st_core_audio_handle *ca_handle = (struct st_core_audio_handle *) handle;
    OSStatus os_status = noErr;
    size_t bufferLoop = 0;
    AudioBuffer *data = 0;
    size_t amt_moved  = 0;
    size_t summed_amt = 0;
    
    if(ca_handle->record_sample_rate_convert) {
        ca_handle->record_convert_buffer.mBuffers[0].mDataByteSize = (UInt32) ca_handle->record_convert_buffer_count_reset;
        AudioBufferList *ablp   = &ca_handle->playback_convert_buffer;
        UInt32 bytes_per_packet = ca_handle->playback_out_description.mBytesPerPacket;
        
        os_status = AudioConverterFillComplexBuffer(ca_handle->record_sr_converter,
                                                    RecordConvert,
                                                    handle,
                                                    &bytes_per_packet,
                                                    ablp,
                                                    0);//&ca_handle->record_packet_desc );
        
        if(bytes_per_packet) {
            bufferLoop = ca_handle->record_convert_buffer.mNumberBuffers;
            
            for(size_t index = 0; index < bufferLoop; index++) {
                data = (AudioBuffer *) &ca_handle->record_convert_buffer.mBuffers[index];
                if(!data) break;
                ca_write_native_buffer(handle, AD_RECORD, (size_t) data->mDataByteSize/sizeof(CORE_AUDIO_DATA_TYPE), &amt_moved, data->mData);
                summed_amt += amt_moved;
            }
            
            if(summed_amt)
                pthread_cond_signal(&ca_handle->record_cond);
        }
    }
    
    if(os_status && (os_status != CA_KEEP_PROCESSING))
        return SET_ERROR(handle, CA_OS_ERROR, os_status);
    
    return CA_NO_ERROR;
}

/** ***********************************************************************************
 * \brief Inline condensed call to AudioConverterFillComplexBuffer()
 * \param Core Audio handle.
 * \return 0 okay, otherwise error.
 **************************************************************************************/
static inline int AudioConvertPlayback(CA_HANDLE handle)
{
    struct st_core_audio_handle *ca_handle = (struct st_core_audio_handle *) handle;
    OSStatus os_status = noErr;
    AudioBuffer *data = 0;
    size_t amt_moved  = 0;

    if(ca_handle->playback_sample_rate_convert) {
        ca_handle->playback_convert_buffer.mBuffers[0].mDataByteSize = (UInt32) ca_handle->playback_convert_buffer_count_reset;
        AudioBufferList *ablp   = &ca_handle->playback_convert_buffer;
        UInt32 bytes_per_packet = (UInt32) ca_handle->playback_convert_buffer_count_reset / (UInt32) ca_handle->selected_playback_device->playback->bytes_per_frame;

        os_status = AudioConverterFillComplexBuffer(ca_handle->playback_sr_converter,
                                                    PlaybackConvert,
                                                    handle,
                                                    &bytes_per_packet,
                                                    ablp,
                                                    0); // &ca_handle->playback_packet_desc );
        
        data = (AudioBuffer *) &ablp->mBuffers[0];
        
        if(data && data->mDataByteSize) {
            ca_write_native_buffer(handle, AD_PLAYBACK, (size_t) data->mDataByteSize/sizeof(CORE_AUDIO_DATA_TYPE), &amt_moved, data->mData);

            if(amt_moved)
                pthread_cond_signal(&ca_handle->playback_cond);
        }
        
        if(os_status && (os_status != CA_KEEP_PROCESSING))
            return SET_ERROR(handle, CA_OS_ERROR, os_status);
        
    }
    return CA_NO_ERROR;
}


/** ***********************************************************************************
 * \brief Initialize Audio Converter interface.
 * \param Core Audio handle.
 * \return 0 okay, otherwise error.
 **************************************************************************************/
static int ca_initialize_converter(CA_HANDLE handle)
{
    if(!handle) {
        return SET_ERROR(0, CA_INVALID_HANDLE, noErr);
    }
    
    struct st_core_audio_handle *ca_handle = (struct st_core_audio_handle *) handle;
    OSStatus os_status = noErr;
    
    if(ca_handle->converter_okay || ca_handle->record_sr_converter || ca_handle->playback_sr_converter)
        ca_release_converter(handle);
    
    if(!ca_handle->selected_record_device || !ca_handle->selected_playback_device)
        return CA_DEVICE_SELECT_ERROR;
    
    if(!ca_handle->selected_record_device->record || !ca_handle->selected_playback_device->playback)
        return SET_ERROR(handle, CA_DEVICE_SELECT_ERROR, noErr);
    
    // Might need this later on.
#if 0
    UInt32 size = (UInt32) sizeof(AudioStreamBasicDescription);
    
    if(AudioFormatGetProperty(kAudioFormatProperty_FormatInfo, 0, NULL, &size, &ca_handle->record_in_description))
        return SET_ERROR(handle, CA_OS_ERROR);
    
    if(AudioFormatGetProperty(kAudioFormatProperty_FormatInfo, 0, NULL, &size, &ca_handle->record_out_description))
        return SET_ERROR(handle, CA_OS_ERROR);
    
    if(AudioFormatGetProperty(kAudioFormatProperty_FormatInfo, 0, NULL, &size, &ca_handle->playback_in_description))
        return SET_ERROR(handle, CA_OS_ERROR);
    
    if(AudioFormatGetProperty(kAudioFormatProperty_FormatInfo, 0, NULL, &size, &ca_handle->playback_out_description))
        return SET_ERROR(handle, CA_OS_ERROR);
#endif // #if 0
    
    UInt32 number_of_channels  = 0;
    Float64 in_sample_rate     = 0.0;
    Float64 out_sample_rate    = 0.0;
    size_t frame_byte_count    = 0;
    size_t frame_allocate_size = 0;
    double convert_scale       = 1.0;
    
    // Configure for specific device
    
    number_of_channels = (UInt32)  ca_handle->selected_record_device->record->no_of_channels;
    in_sample_rate     = (Float64) ca_handle->hardware_record_sample_rate;
    out_sample_rate    = (Float64) ca_handle->requested_record_sample_rate;
    
    if(in_sample_rate != out_sample_rate)
        ca_handle->record_sample_rate_convert = 1;
    else
        ca_handle->record_sample_rate_convert = 0;
    
    ca_handle->record_in_description.mSampleRate        = in_sample_rate;
    ca_handle->record_in_description.mFormatID          = kAudioFormatLinearPCM;
    ca_handle->record_in_description.mFormatFlags       = kLinearPCMFormatFlagIsFloat;
    ca_handle->record_in_description.mBytesPerPacket    = number_of_channels * sizeof(CORE_AUDIO_DATA_TYPE) * AC_FRAMES_PER_PACKET;
    ca_handle->record_in_description.mFramesPerPacket   = AC_FRAMES_PER_PACKET;
    ca_handle->record_in_description.mBytesPerFrame     = number_of_channels * sizeof(CORE_AUDIO_DATA_TYPE);
    ca_handle->record_in_description.mChannelsPerFrame  = number_of_channels;
    ca_handle->record_in_description.mBitsPerChannel    = sizeof(CORE_AUDIO_DATA_TYPE) * 8;
    
    ca_handle->record_out_description.mSampleRate       = out_sample_rate;
    ca_handle->record_out_description.mFormatID         = kAudioFormatLinearPCM;
    ca_handle->record_out_description.mFormatFlags      = kLinearPCMFormatFlagIsFloat;
    ca_handle->record_out_description.mBytesPerPacket   = number_of_channels * sizeof(CORE_AUDIO_DATA_TYPE) * AC_FRAMES_PER_PACKET;
    ca_handle->record_out_description.mFramesPerPacket  = AC_FRAMES_PER_PACKET;
    ca_handle->record_out_description.mBytesPerFrame    = number_of_channels * sizeof(CORE_AUDIO_DATA_TYPE);
    ca_handle->record_out_description.mChannelsPerFrame = number_of_channels;
    ca_handle->record_out_description.mBitsPerChannel   = sizeof(CORE_AUDIO_DATA_TYPE) * 8;
    
    // Check for upscaling and adjust output buffer requirements to support it.
    
    convert_scale       = ((double) out_sample_rate) / ((double) in_sample_rate);
    frame_allocate_size = ca_handle->selected_record_device->record->allocate_frame_count;
    frame_byte_count    = ca_handle->selected_record_device->record->bytes_per_frame;
    
    if(convert_scale > 1.0) {
        frame_allocate_size *= convert_scale;
    }
    
    frame_allocate_size *= frame_byte_count;
    
    ca_handle->record_convert_buffer.mNumberBuffers = 1;
    ca_handle->record_convert_buffer.mBuffers[0].mNumberChannels = number_of_channels;
    ca_handle->record_convert_buffer.mBuffers[0].mDataByteSize = (UInt32) frame_allocate_size;
    ca_handle->record_convert_buffer.mBuffers[0].mData = calloc(1, frame_allocate_size + frame_byte_count);
    
    if (!ca_handle->record_convert_buffer.mBuffers[0].mData)
        return SET_ERROR(handle, CA_ALLOCATE_FAIL, noErr);
    
    number_of_channels = (UInt32)  ca_handle->selected_playback_device->playback->no_of_channels;
    in_sample_rate     = (Float64) ca_handle->requested_playback_sample_rate;
    out_sample_rate    = (Float64) ca_handle->hardware_playback_sample_rate;
    
    if(in_sample_rate != out_sample_rate)
        ca_handle->playback_sample_rate_convert = 1;
    else
        ca_handle->playback_sample_rate_convert = 0;
    
    ca_handle->playback_in_description.mSampleRate         = in_sample_rate;
    ca_handle->playback_in_description.mFormatID           = kAudioFormatLinearPCM;
    ca_handle->playback_in_description.mFormatFlags        = kLinearPCMFormatFlagIsFloat;
    ca_handle->playback_in_description.mBytesPerPacket     = (number_of_channels * sizeof(CORE_AUDIO_DATA_TYPE)) * AC_FRAMES_PER_PACKET;
    ca_handle->playback_in_description.mFramesPerPacket    = AC_FRAMES_PER_PACKET;
    ca_handle->playback_in_description.mBytesPerFrame      = number_of_channels * sizeof(CORE_AUDIO_DATA_TYPE);
    ca_handle->playback_in_description.mChannelsPerFrame   = number_of_channels;
    ca_handle->playback_in_description.mBitsPerChannel     = sizeof(CORE_AUDIO_DATA_TYPE) * 8;
    
    ca_handle->playback_out_description.mSampleRate        = out_sample_rate;
    ca_handle->playback_out_description.mFormatID          = kAudioFormatLinearPCM;
    ca_handle->playback_out_description.mFormatFlags       = kLinearPCMFormatFlagIsFloat;
    ca_handle->playback_out_description.mBytesPerPacket    = (number_of_channels * sizeof(CORE_AUDIO_DATA_TYPE)) * AC_FRAMES_PER_PACKET;
    ca_handle->playback_out_description.mFramesPerPacket   = AC_FRAMES_PER_PACKET;
    ca_handle->playback_out_description.mBytesPerFrame     = number_of_channels * sizeof(CORE_AUDIO_DATA_TYPE);
    ca_handle->playback_out_description.mChannelsPerFrame  = number_of_channels;
    ca_handle->playback_out_description.mBitsPerChannel    = sizeof(CORE_AUDIO_DATA_TYPE) * 8;
    
    // Check for upscaling and adjust output buffer requirements to support it.
    
    convert_scale       = ((double) out_sample_rate) / ((double) in_sample_rate);
    frame_allocate_size = ca_handle->selected_playback_device->playback->allocate_frame_count;
    frame_byte_count    = ca_handle->selected_playback_device->playback->bytes_per_frame;
    
    if(convert_scale > 1.0) {
        frame_allocate_size *= convert_scale;
    }
    
    frame_allocate_size *= frame_byte_count;
    
    ca_handle->playback_convert_buffer.mNumberBuffers = 1;
    ca_handle->playback_convert_buffer.mBuffers[0].mNumberChannels = number_of_channels;
    ca_handle->playback_convert_buffer.mBuffers[0].mDataByteSize = (UInt32) frame_allocate_size;
    ca_handle->playback_convert_buffer.mBuffers[0].mData = calloc(1, frame_allocate_size + frame_byte_count);
    
    if (!ca_handle->playback_convert_buffer.mBuffers[0].mData)
        return SET_ERROR(handle, CA_ALLOCATE_FAIL, noErr);
    
    os_status = AudioConverterNew(&ca_handle->record_in_description, &ca_handle->record_out_description, &ca_handle->record_sr_converter);
    if(os_status)
        return SET_ERROR(handle, CA_OS_ERROR, os_status);
    
    os_status = AudioConverterNew(&ca_handle->playback_in_description, &ca_handle->playback_out_description, &ca_handle->playback_sr_converter);
    if(os_status)
        return SET_ERROR(handle, CA_OS_ERROR, os_status);
    
    // Retain for later use.
    
    ca_handle->playback_convert_buffer_count_reset = (size_t) ca_handle->playback_convert_buffer.mBuffers[0].mDataByteSize; 
    ca_handle->record_convert_buffer_count_reset   = (size_t) ca_handle->record_convert_buffer.mBuffers[0].mDataByteSize;
    
    if(pthread_cond_init(&ca_handle->record_convert_cond, 0))
        return SET_ERROR(handle, CA_CONVERTER_INITIALIZE_FAIL, noErr);
    
    if(pthread_mutex_init(&ca_handle->record_convert_mutex, 0))
        return SET_ERROR(handle, CA_CONVERTER_INITIALIZE_FAIL, noErr);
    
    if(pthread_cond_init(&ca_handle->playback_convert_cond, 0))
        return SET_ERROR(handle, CA_CONVERTER_INITIALIZE_FAIL, noErr);
    
    if(pthread_mutex_init(&ca_handle->playback_convert_mutex, 0))
        return SET_ERROR(handle, CA_CONVERTER_INITIALIZE_FAIL, noErr);
    
    ca_handle->convert_close_flag = 0;
    ca_handle->record_convert_thread_running = 0;
    ca_handle->playback_convert_thread_running = 0;
    
    if(pthread_create(&ca_handle->playback_convert_thread, 0, PlaybackConvertThread, handle))
        return SET_ERROR(handle, CA_CONVERTER_INITIALIZE_FAIL, noErr);
    
    if(pthread_create(&ca_handle->record_convert_thread, 0, RecordConvertThread, handle))
        return SET_ERROR(handle, CA_CONVERTER_INITIALIZE_FAIL, noErr);
    
    ca_handle->converter_okay = 1;
    
    return CA_NO_ERROR;
}

/** ***********************************************************************************
 * \brief Free Audio Unit interface resources.
 * \param Core Audio handle.
 * \return void
 **************************************************************************************/
static void ca_release_converter(CA_HANDLE handle)
{
    if(!handle) {
        SET_ERROR(0, CA_INVALID_HANDLE, noErr);
        return;
    }
    
    struct st_core_audio_handle *ca_handle = (struct st_core_audio_handle *) handle;
    
    ca_handle->convert_close_flag = 1;
    
    if(ca_handle->record_convert_thread_running) {
        pthread_mutex_lock(&ca_handle->record_convert_mutex);
        pthread_cond_signal(&ca_handle->record_convert_cond);
        pthread_mutex_unlock(&ca_handle->record_convert_mutex);
    }
    
    pthread_join(ca_handle->record_convert_thread, 0);
    
    if(ca_handle->playback_convert_thread_running) {
        pthread_mutex_lock(&ca_handle->playback_convert_mutex);
        pthread_cond_signal(&ca_handle->playback_convert_cond);
        pthread_mutex_unlock(&ca_handle->playback_convert_mutex);
    }
    
    pthread_join(ca_handle->playback_convert_thread, 0);
    
    pthread_cond_destroy(&ca_handle->record_convert_cond);
    pthread_cond_destroy(&ca_handle->playback_convert_cond);
    
    pthread_mutex_destroy(&ca_handle->record_convert_mutex);
    pthread_mutex_destroy(&ca_handle->playback_convert_mutex);
    
    if(ca_handle->record_convert_buffer.mBuffers[0].mData)
        free(ca_handle->record_convert_buffer.mBuffers[0].mData);
    
    if(ca_handle->playback_convert_buffer.mBuffers[0].mData)
        free(ca_handle->playback_convert_buffer.mBuffers[0].mData);
    
    if(ca_handle->record_sr_converter)
        AudioConverterDispose(ca_handle->record_sr_converter);
    
    if(ca_handle->playback_sr_converter)
        AudioConverterDispose(ca_handle->playback_sr_converter);
    
    ca_handle->converter_okay = 0;
}

/** ***********************************************************************************
 * \brief Determine the min/max frame size for a given device. Calculate nearset 16 frames
 * aligned high value (vector unit friendly)
 * \param handle Core Audio handle
 * \param device_id Device in question.
 * \param value_range pointer to AudioValueRange structure.
 * \return 0 Okay, otherwise fail.
 **************************************************************************************/
int ca_get_frame_range(CA_HANDLE handle, AudioDeviceID device_id, size_t *high, size_t *low, size_t *frame_allocate_count)
{
    UInt32 prop_size = sizeof(AudioValueRange);
    OSStatus status = noErr;
    AudioValueRange range_value;
    size_t tmp = 0;
    
    AudioObjectPropertyAddress properties = {
        kAudioDevicePropertyBufferFrameSizeRange,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMaster
    };
    
    if(!frame_allocate_count)
        return SET_ERROR(handle, CA_INVALID_PARAMATER, noErr);
    
    memset(&range_value, 0, sizeof(range_value));
    
    status =  AudioObjectGetPropertyData(device_id, &properties, 0, 0, &prop_size,
                                         (void *) &range_value);
    
    if(high) *high = (size_t) range_value.mMaximum;
    if(low)  *low  = (size_t) range_value.mMinimum;
    
    tmp = ((size_t) range_value.mMaximum) % AC_FRAME_MODULUS;
    *frame_allocate_count = range_value.mMaximum - tmp;
    
    if(*frame_allocate_count < range_value.mMinimum)
        *frame_allocate_count = range_value.mMaximum;
    
    if(status)
        SET_ERROR(handle, CA_OS_ERROR, noErr);
    
    return status ? CA_OS_ERROR : CA_NO_ERROR;
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
    int io_in_device  = AD_RECORD;
    int io_out_device = AD_PLAYBACK;
    
    struct st_core_audio_handle *ca_handle = (struct st_core_audio_handle *) handle;
    
    pthread_mutex_lock(&ca_handle->record_mutex);
    pthread_cond_wait(&ca_handle->record_cond, &ca_handle->record_mutex);
    pthread_mutex_unlock(&ca_handle->record_mutex);
    
    ca_frames(handle, AD_RECORD, &used_count, &free_count);
    
    if(ca_handle->playback_sample_rate_convert)
        io_out_device = AD_PLAYBACK_SRC;
    
    if(used_count) {
        for(index = 0; index < used_count; index++) {
            ca_read_native_stereo(handle, io_in_device,  &left_audio_data, &right_audio_data);
            ca_write_native_stereo(handle, io_out_device, left_audio_data, right_audio_data);
        }
    }
}

#if defined(MAC_OS_X_VERSION_10_4) && !defined(MAC_OS_X_VERSION_10_7)
/** ***********************************************************************************
 * \brief Compute the length of a string with length limitation.
 * \param str string.
 * \param limit max count length
 * \return the number of bytes in the string up to limitaion.
 * Not present in some versions of MacOSX (< 10.7).
 **************************************************************************************/
static size_t strnlen(const char *str, size_t limit)
{
    if(!str || !limit) return 0;
    char *end = (char *) &str[limit];
    size_t count = 0;
    for(; str < end; str++) {
        if(*str == 0) break;
        count++;
    }
    return count;
}
#endif // #if defined(MAC_OS_X_VERSION_10_4) && !defined(MAC_OS_X_VERSION_10_7)

/** ***********************************************************************************
 * \brief Convert a 32 bit value to string data.
 * \param value Value to convert
 * \param buffer Store the results here
 * \param bufferSize Buffer size
 * \return storage pointer.
 **************************************************************************************/
char * string_32(UInt32 value, char *buffer, size_t size)
{
    int index = 0;
    int byte_count = 0;
    unsigned char aByte = 0;
    
    if(!buffer)
        return (char *) "null pointer";
    
    memset(buffer, 0, size);
    
    byte_count = sizeof(value);
    
    if(byte_count > size)
        return (char *) "Increase buffer size";
    
    for(index = byte_count; index > 0; index--) {
        aByte = value & 0xFF;
        value >>= 8;
        if(aByte < ' ') aByte = ' ';
        buffer[index - 1] = aByte;
    }
    
    return buffer;
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

