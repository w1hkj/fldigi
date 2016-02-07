/** ***********************************************************************************
 * core_audio.h
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

#ifndef __core_audio_h
#define __core_audio_h

#include <CoreAudio/CoreAudio.h>
#include <AudioToolbox/AudioToolbox.h>
#include <CoreFoundation/CFString.h>
#include <pthread.h>

#define CORE_AUDIO_DATA_TYPE Float32
#define HOST_AUDIO_DATA_TYPE double

#define AD_PLAYBACK     0x1
#define AD_RECORD       0x2
#define AD_PLAYBACK_SRC 0x4
#define AD_RECORD_SRC   0x8

#define AD_SRC_NO       0x0
#define AD_SRC_MAYBE    0x1

#define CA_KEEP_PROCESSING 100

#define CA_MIN_FRAME_COUNT 512

#define AUDIO_DEVICE_NAME_LIMIT  80
#define AUDIO_CHANNEL_NAME_LIMIT 80

#define AC_FRAMES_PER_PACKET 1
#define AC_INPUT_FRAMES_PER_PACKET 64

#define AC_FRAME_MODULUS 16

enum ca_convert_quaility {
    CA_CONV_MAX  = 0x01,
    CA_CONV_HIGH = 0x02,
    CA_CONV_MED  = 0x03,
    CA_CONV_LOW  = 0x04,
    CA_CONV_MIN  = 0x05,
    // bitwise or'd with above
    CA_CONV_LINEAR    = 0x10,
    CA_CONV_NORMAL    = 0x20,
    CA_CONV_MASTERING = 0x30,
    // Internal use.
    CA_CONV_QTY_MASK  = 0x0F,
    CA_CONV_CPLX_MASK = 0xF0
};

enum ca_channel_mapping {
    CA_MONO_CHANNEL_INDEX = 0,
    CA_LEFT_CHANNEL_INDEX = 0,
    CA_RIGHT_CHANNEL_INDEX,
    CA_CHANNEL_3_INDEX,
    CA_CHANNEL_4_INDEX,
    CA_CHANNEL_5_INDEX,
    CA_CHANNEL_6_INDEX,
    CA_CHANNEL_7_INDEX,
    CA_CHANNEL_8_INDEX,
    CA_CHANNEL_9_INDEX,
    CA_CHANNEL_10_INDEX,
    CA_CHANNEL_11_INDEX,
    CA_CHANNEL_12_INDEX,
    CA_CHANNEL_13_INDEX,
    CA_CHANNEL_14_INDEX,
    CA_CHANNEL_15_INDEX,
    CA_CHANNEL_16_INDEX,
    CA_CHANNEL_MAXIMUM
};

enum run_pause {
    RS_RUN = 0,
    RS_PAUSE
};

enum host_data_types {
    ADF_S8 = 1,
    ADF_S16,
    ADF_S32,
    ADF_S64,
    ADF_U8,
    ADF_U16,
    ADF_U32,
    ADF_U64,
    ADF_FLOAT,
    ADF_DOUBLE
};

enum ca_error_number_list {
    CA_NO_ERROR = 0,
    CA_BUFFER_STALLED,
    CA_BUFFER_EMPTY,
    CA_BUFFER_FULL,
    CA_ALLOCATE_FAIL,
    CA_INDEX_OUT_OF_RANGE,
    CA_DEVICE_SELECT_ERROR,
    CA_INVALID_PARAMATER,
    CA_INVALID_HANDLE,
    CA_BUFFER_TO_SMALL,
    CA_INITIALIZE_FAIL,
    CA_INVALID_DIRECTION,
    CA_CHANNEL_INDEX_OUT_OF_RANGE,
    CA_UNKNOWN_IO_DIRECTION,
    CA_DEVICE_NOT_FOUND,
    CA_DEVICE_NO_CHANNELS,
    CA_GET_PROPERTY_DATA_SIZE_ERROR,
    CA_GET_PROPERTY_DATA_ERROR,
    CA_SET_PROPERTY_DATA_ERROR,
    CA_OS_ERROR,
    CA_CONVERTER_INITIALIZE_FAIL,
    CA_PLAYBACK_DEVICE_SELECTED,
    CA_RECORD_DEVICE_SELECTED,
    CA_MUST_CLOSE_DEVICE_FIRST,
    CA_INVALID_ERROR // Must be last entry in enum list
};

struct st_device_info {
    char device_name[AUDIO_DEVICE_NAME_LIMIT];
    char *channel_desc[CA_CHANNEL_MAXIMUM];
    size_t no_of_channels;
    size_t device_index;
    size_t device_id;
    double *sample_rates;
    size_t sample_rate_count;
};

struct st_device_list {
    struct st_device_info *record;
    size_t record_count;
    struct st_device_info *playback;
    size_t playback_count;
};

struct st_ring_buffer {
    CORE_AUDIO_DATA_TYPE *buffer;
    size_t write_pos;
    size_t read_pos;

    size_t buffer_size;
    size_t unit_count;
    size_t unit_size;
    size_t frame_count;
    size_t frame_size;
    size_t channels_per_frame;
    size_t left_channel_index;
    size_t right_channel_index;

    pthread_mutex_t io_mutex;
    pthread_cond_t  io_cond;
};

struct st_audio_device_list;

struct st_audio_device_info {
    size_t          no_of_channels;
    size_t          bytes_per_frame;
    size_t          min_frame_count;
    size_t          max_frame_count;
    size_t          allocate_frame_count;
    Float64       * sample_rates;
    size_t          sample_rate_count;
    char         ** channel_desc;
    size_t          selected_left_channel_index;
    size_t          selected_right_channel_index;
    struct st_audio_device_list * parent;
};

struct st_audio_device_list {
    size_t          device_index;
    char          * device_name;
    int             device_io;
    AudioDeviceID   device_id;
#if defined( MAC_OS_X_VERSION_10_5 ) && ( MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_5 )
    AudioDeviceIOProcID io_proc_id;
#endif
    struct st_audio_device_info *playback;
    struct st_audio_device_info *record;
};

// Opaque pointer
#define CA_HANDLE void *

struct st_core_audio_handle {

    struct st_audio_device_list *selected_record_device;
    struct st_audio_device_list *selected_playback_device;

    AudioObjectPropertyListenerProc property_listener;

    AudioDeviceIOProc record_callback;
    AudioDeviceIOProc playback_callback;

    AudioConverterRef playback_sr_converter;
    AudioConverterRef record_sr_converter;

    int record_sample_rate_convert;
    int playback_sample_rate_convert;

    AudioStreamBasicDescription playback_in_description;
    AudioStreamBasicDescription playback_out_description;
    AudioStreamBasicDescription record_in_description;
    AudioStreamBasicDescription record_out_description;

    AudioStreamPacketDescription record_packet_desc;
    AudioStreamPacketDescription playback_packet_desc;

    AudioBufferList record_convert_buffer;
    AudioBufferList playback_convert_buffer;

    size_t record_convert_buffer_count_reset;
    size_t playback_convert_buffer_count_reset;

    int convert_close_flag;
    int record_convert_thread_running;
    int playback_convert_thread_running;

    pthread_t       record_convert_thread;
    pthread_mutex_t record_convert_mutex;
    pthread_cond_t  record_convert_cond;

    pthread_t       playback_convert_thread;
    pthread_mutex_t playback_convert_mutex;
    pthread_cond_t  playback_convert_cond;

    pthread_mutex_t record_mutex;
    pthread_cond_t  record_cond;

    pthread_mutex_t playback_mutex;
    pthread_cond_t  playback_cond;

    pthread_mutex_t flag_mutex;
    pthread_cond_t  flag_cond;

    struct st_ring_buffer *rb_record;
    struct st_ring_buffer *rb_playback;

    struct st_ring_buffer *rb_record_src;
    struct st_ring_buffer *rb_playback_src;

    struct st_audio_device_list *device_list;
    size_t device_list_count;

    Float64 hardware_record_sample_rate;
    Float64 requested_record_sample_rate;
    Float64 hardware_playback_sample_rate;
    Float64 requested_playback_sample_rate;
    Float64 highest_sample_rate;
    double sample_duration_in_seconds;


    int converter_okay;
    int wait_state;
    int okay_to_run;

    double outOSScale;
    double inOSScale;
    double bias;
    double zero_ref;

    int pause_flag;
    int close_flag;
    int running_flag;
    int exit_flag;
    int closed_flag;

    int last_error_code;
    int error_code;

    char err_str[1024];
};

struct ca_wait_data {
    AudioObjectPropertyAddress properties;
    AudioDeviceID device_id;
    struct st_core_audio_handle *handle;
};

#define SET_ERROR(a, b, c) ca_set_error(a, __func__, __LINE__, b, c)

void ca_print_device_list_to_console(CA_HANDLE handle);

CA_HANDLE ca_get_handle(Float64 rb_duration);
void ca_release_handle(CA_HANDLE handle);
void ca_free_device_attributes(struct st_audio_device_info *device);

int ca_wait_for_os(CA_HANDLE handle, float timeout);
int ca_set_set_converter_quaility(CA_HANDLE *handle, int io_device, int quaility);

int ca_set_converter_quaility(CA_HANDLE handle, int io_device, int quaility);
int ca_get_frame_range(CA_HANDLE handle, AudioDeviceID device_id, size_t *high, size_t *low, size_t *frame_allocate_count);
int ca_get_device_channel_names(CA_HANDLE handle, AudioDeviceID id, char ** array, size_t no_of_channels, size_t direction);
int ca_set_sample_rate(CA_HANDLE handle, int io_device, Float64 sample_rate);
int ca_get_sample_rate(CA_HANDLE handle, int io_device, Float64 *sample_rate);
int ca_get_sample_rates(CA_HANDLE handle, AudioDeviceID device_id, Float64 **sample_rates, size_t *count);
int ca_get_device_attributes(CA_HANDLE handle, AudioDeviceID id, struct st_audio_device_list *list, size_t no_of_record_channels, size_t no_of_playback_channels);
int ca_get_no_of_channels(CA_HANDLE handle, AudioDeviceID id, size_t *record, size_t *playback);
char * ca_get_device_name(CA_HANDLE handle, AudioDeviceID id);
char * ca_get_device_channel_name(CA_HANDLE handle, AudioDeviceID device_id, size_t channel_index, size_t max_channels, size_t direction);
Float64 ca_get_max_sample_rate(CA_HANDLE handle, struct st_audio_device_list * list);
int ca_get_default_devices(AudioDeviceID *record, AudioDeviceID *playback);
int ca_get_device_id_list(AudioDeviceID **array_of_ids, size_t *count);

struct st_device_list * ca_get_device_list(CA_HANDLE handle);
int ca_free_device_list(CA_HANDLE handle, struct st_device_list * list);

int ca_map_device_channels(CA_HANDLE handle, int io_device, size_t left_index, size_t right_index);

int ca_open(CA_HANDLE handle);
int ca_close(CA_HANDLE handle);
int ca_run(CA_HANDLE handle);

AudioDeviceID ca_select_device_by_index(CA_HANDLE handle, int index, int device_io);
AudioDeviceID ca_select_device_by_name(CA_HANDLE handle, char *device_name, int device_io);
int ca_select_device_by_id(CA_HANDLE handle, AudioDeviceID device_id, int device_io);

int ca_set_error(CA_HANDLE handle, const char *fn, int line_number, int error, OSStatus os_error_no);
int ca_get_error(CA_HANDLE handle);
const char *ca_error_message(CA_HANDLE handle, int error_no, OSStatus os_error_no);

int ca_start_listener(CA_HANDLE handle, struct ca_wait_data *wait_data);
int ca_flush_audio_stream(CA_HANDLE handle);

int ca_start_listener(CA_HANDLE handle, struct ca_wait_data *wait_data);
int ca_remove_listener(CA_HANDLE handle, struct ca_wait_data *wd);

int ca_inc_read_pos(CA_HANDLE handle, struct st_ring_buffer *rb);
int ca_inc_write_pos(CA_HANDLE handle, struct st_ring_buffer *rb);
int ca_stalled(CA_HANDLE handle, struct st_ring_buffer *rb);
int ca_empty(CA_HANDLE handle, struct st_ring_buffer *rb);

struct st_ring_buffer * ca_init_ring_buffer(CA_HANDLE handle, size_t allocate_element_count);
struct st_ring_buffer * ca_device_to_ring_buffer(CA_HANDLE handle, int io_device, int src_flag);
int ca_set_ring_buffer_attributes(CA_HANDLE handle, struct st_ring_buffer * rb, size_t no_of_channels);
void ca_release_ringbuffer(struct st_ring_buffer * ring_buffer);


int ca_frames(CA_HANDLE handle, int io_device, size_t *used_count, size_t *free_count);
void ca_cross_connect(CA_HANDLE handle);

int ca_read_stereo(CA_HANDLE handle, int io_device, HOST_AUDIO_DATA_TYPE *left, HOST_AUDIO_DATA_TYPE *right);
int ca_read_buffer(CA_HANDLE handle, int io_device, size_t amount_to_read,  size_t *amount_read, HOST_AUDIO_DATA_TYPE buffer[]);

int ca_write_stereo(CA_HANDLE handle, int io_device, HOST_AUDIO_DATA_TYPE  left, HOST_AUDIO_DATA_TYPE  right);
int ca_write_buffer(CA_HANDLE handle, int io_device, size_t amount_to_write, size_t *amount_written, HOST_AUDIO_DATA_TYPE buffer[]);

void ca_format_scale(CA_HANDLE handle, enum host_data_types data_type);

int ca_string_match(const char * str1, const char *str2, size_t limit);

int ca_print_msg(CA_HANDLE handle, int error_no, OSStatus os_error_no, char *string);

#ifdef __cplusplus
extern "C" {
#endif

    // Operating system callbacks

    OSStatus CADeviceRecord(AudioObjectID inDevice, const AudioTimeStamp * inNow,
                            const AudioBufferList * inInputData, const AudioTimeStamp * inInputTime,
                            AudioBufferList * outOutputData, const AudioTimeStamp * inOutputTime,
                            void * inClientData);

    OSStatus CADevicePlayback(AudioObjectID inDevice, const AudioTimeStamp * inNow,
                              const AudioBufferList * inInputData, const AudioTimeStamp * inInputTime,
                              AudioBufferList * outOutputData, const AudioTimeStamp * inOutputTime,
                              void * inClientData);
    
    OSStatus CAPropertyListener(AudioObjectID id, UInt32 address_count,
                                const AudioObjectPropertyAddress paddress[], void *inClientData);
    
    
    OSStatus PlaybackConvert(AudioConverterRef inAudioConverter,
                             UInt32 * ioNumberDataPackets,
                             AudioBufferList * ioData,
                             AudioStreamPacketDescription ** outDataPacketDescription,
                             void * inUserData);
    
    OSStatus RecordConvert(AudioConverterRef inAudioConverter,
                           UInt32 * ioNumberDataPackets,
                           AudioBufferList * ioData,
                           AudioStreamPacketDescription ** outDataPacketDescription,
                           void * inUserData);
    
    void * PlaybackConvertThread(void * in);
    void * RecordConvertThread(void * in);

#ifdef __cplusplus
} //  extern "C" {
#endif

#endif /* __core_audio_h */
