/** ***********************************************************************************
 * core_audio_test.cxx
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

#include <stdio.h>
#include "core_audio.h"

int main(int argc, const char * argv[])
{
    char device_name[AUDIO_DEVICE_NAME_LIMIT+1];

    AudioDeviceID record_id   = 0;
    AudioDeviceID playback_id = 0;
    Float64 sample_rate = 0.0;
    Float64 return_sample_rate = 0.0;
    int convert_flags = CA_CONV_HIGH|CA_CONV_NORMAL;

    CA_HANDLE ca_handle = (CA_HANDLE) 0;

    ca_handle = ca_get_handle(1.5);

    if(!ca_handle) {
        printf("Handle create fail.");
        return 0;
    }

    ca_print_device_list_to_console(ca_handle);

    memset(device_name, 0, sizeof(device_name));
    strncpy(device_name, "AK5370", AUDIO_DEVICE_NAME_LIMIT);

    record_id = ca_select_device_by_name(ca_handle, device_name, AD_RECORD);

    memset(device_name, 0, sizeof(device_name));
    strncpy(device_name, "Built-in Output", AUDIO_DEVICE_NAME_LIMIT);

    playback_id = ca_select_device_by_name(ca_handle, device_name, AD_PLAYBACK);

    //sample_rate = 48000.0;
    sample_rate = 8000.0;
    ca_set_sample_rate(ca_handle, AD_PLAYBACK, sample_rate);
    ca_set_sample_rate(ca_handle, AD_RECORD, sample_rate);

    ca_get_sample_rate(ca_handle, AD_PLAYBACK, &return_sample_rate);
    printf("Playback SR = %f\n", (float) return_sample_rate);

    ca_get_sample_rate(ca_handle, AD_RECORD, &return_sample_rate);
    printf("Record SR = %f\n", (float) return_sample_rate);


    struct st_device_list * dev_list = ca_get_device_list(ca_handle);

    if(dev_list)
        ca_free_device_list(ca_handle, dev_list);

    ca_open(ca_handle);
    
    ca_set_converter_quaility(ca_handle, AD_PLAYBACK, convert_flags);
    ca_set_converter_quaility(ca_handle, AD_RECORD,   convert_flags);

    ca_run(ca_handle);

    time_t current_time = time(0);
    time_t end_time     = current_time + 30;

    while(current_time < end_time) {
        ca_cross_connect(ca_handle);
        current_time = time(0);
    }

    ca_close(ca_handle);

    ca_release_handle(ca_handle);

    return 0;
}
