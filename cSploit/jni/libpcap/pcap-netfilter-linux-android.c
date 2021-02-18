/*
 * Copyright (C) 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "pcap-netfilter-linux.c"

int
android_nflog_send_config_cmd(int fd, u_int16_t group_id, u_int8_t cmd, u_int8_t family)
{
    pcap_t handle;
    handle.fd = fd;
    return nflog_send_config_cmd(&handle, group_id, cmd, family);
}
