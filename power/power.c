/*
 * Copyright (C) 2012 The Android Open Source Project
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
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define LOG_TAG "Grouper PowerHAL"
#include <utils/Log.h>

#include <hardware/hardware.h>
#include <hardware/power.h>

#define BOOSTPULSE_PATH "/sys/devices/system/cpu/cpufreq/interactive/boostpulse"
#define SCALING_MAX_BUF_SZ  10
static char scaling_max_freq[SCALING_MAX_BUF_SZ]   = "1300000";
static char screenoff_max_freq[SCALING_MAX_BUF_SZ] = "640000";

struct grouper_power_module {
    struct power_module base;
    pthread_mutex_t lock;
    int boostpulse_fd;
    int boostpulse_warned;
};

static void sysfs_write(char *path, char *s)
{
    char buf[80];
    int len;
    int fd = open(path, O_WRONLY);

    if (fd < 0) {
        strerror_r(errno, buf, sizeof(buf));
        ALOGE("Error opening %s: %s\n", path, buf);
        return;
    }

    len = write(fd, s, strlen(s));
    if (len < 0) {
        strerror_r(errno, buf, sizeof(buf));
        ALOGE("Error writing to %s: %s\n", path, buf);
    }

    close(fd);
}

static int sysfs_read(const char *path, char *s, size_t l)
{
    char buf[80];
    int ret = -1;
    int count;
    int fd = open(path, O_RDONLY);

    if (fd < 0) {
        strerror_r(errno, buf, sizeof(buf));
        ALOGE("Error opening %s: %s\n", path, buf);
        return ret;
    }

    do {
        count = read(fd, s, l - 1);
    } while (count < 0 && errno == EINTR); /* Retry if interrupted */

    if (count < 0) {
        strerror_r(errno, buf, sizeof(buf));
        ALOGE("Error reading from %s: %s\n", path, buf);
    } else {
        s[count] = '\0';
        ret = count;
    }

    close(fd);
    return ret;
}

static int boostpulse_open(struct grouper_power_module *grouper)
{
    char buf[80];

    pthread_mutex_lock(&grouper->lock);

    if (grouper->boostpulse_fd < 0) {
        grouper->boostpulse_fd = open(BOOSTPULSE_PATH, O_WRONLY);

        if (grouper->boostpulse_fd < 0) {
            if (!grouper->boostpulse_warned) {
                strerror_r(errno, buf, sizeof(buf));
                ALOGE("Error opening %s: %s\n", BOOSTPULSE_PATH, buf);
                grouper->boostpulse_warned = 1;
            }
        }
    }

    pthread_mutex_unlock(&grouper->lock);
    return grouper->boostpulse_fd;
}

static void grouper_power_init(struct power_module *module)
{
    /*
     * cpufreq interactive governor: timer 20ms, min sample 30ms,
     * hispeed 1GHz
     */

    sysfs_write("/sys/devices/system/cpu/cpufreq/interactive/timer_rate",
                "20000");
    sysfs_write("/sys/devices/system/cpu/cpufreq/interactive/min_sample_time",
                "30000");
    sysfs_write("/sys/devices/system/cpu/cpufreq/interactive/go_hispeed_load",
                "85");
    sysfs_write("/sys/devices/system/cpu/cpufreq/interactive/hispeed_freq",
                "1000000");
}

static void grouper_power_set_interactive(struct power_module *module, int on)
{
    int len;
    char buf[SCALING_MAX_BUF_SZ];

    /*
     * Lower maximum frequency when screen is off.  CPU 0 and 1 share a
     * cpufreq policy.
     */

    if (!on) {
        /* read the current scaling max freq and save it before updating */
        len = sysfs_read("/sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq",
                         buf, sizeof(buf));
        if (len > 0 && strncmp(buf, screenoff_max_freq,
                                strlen(screenoff_max_freq)) != 0) {
                strcpy(scaling_max_freq, buf);
        }
    }

    sysfs_write("/sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq",
                on ? scaling_max_freq : screenoff_max_freq);

}

static void grouper_power_hint(struct power_module *module, power_hint_t hint,
                            void *data)
{
    struct grouper_power_module *grouper = (struct grouper_power_module *) module;
    char buf[80];
    int len;
    int duration = 1;

    switch (hint) {
    case POWER_HINT_INTERACTION:
        if (boostpulse_open(grouper) >= 0) {
            if (data != NULL)
                duration = (int) data;
            snprintf(buf, sizeof(buf), "%d", duration);
            len = write(grouper->boostpulse_fd, buf, strlen(buf));
            if (len < 0) {
                strerror_r(errno, buf, sizeof(buf));
                ALOGE("Error writing to %s: %s\n", BOOSTPULSE_PATH, buf);
                pthread_mutex_lock(&grouper->lock);
                close(grouper->boostpulse_fd);
                grouper->boostpulse_fd = -1;
                grouper->boostpulse_warned = 0;
                pthread_mutex_unlock(&grouper->lock);
            }
        }
        break;

    case POWER_HINT_VSYNC:
        break;

    default:
        break;
    }
}

static struct hw_module_methods_t power_module_methods = {
    .open = NULL,
};

struct grouper_power_module HAL_MODULE_INFO_SYM = {
    base: {
        common: {
            tag: HARDWARE_MODULE_TAG,
            module_api_version: POWER_MODULE_API_VERSION_0_2,
            hal_api_version: HARDWARE_HAL_API_VERSION,
            id: POWER_HARDWARE_MODULE_ID,
            name: "Grouper Power HAL",
            author: "The Android Open Source Project",
            methods: &power_module_methods,
        },

        init: grouper_power_init,
        setInteractive: grouper_power_set_interactive,
        powerHint: grouper_power_hint,
    },

    lock: PTHREAD_MUTEX_INITIALIZER,
    boostpulse_fd: -1,
    boostpulse_warned: 0,
};
