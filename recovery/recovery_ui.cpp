/*
 * Copyright (C) 2011 The Android Open Source Project
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
#include <fcntl.h>
#include <linux/fb.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "common.h"
#include "device.h"
#include "ui.h"
#include "screen_ui.h"

static const char* HEADERS[] = { "Use hardware button to move cursor; long-press to select item.",
                                 "",
                                 NULL };

// these strings are never actually displayed
static const char* ITEMS[] =  {"reboot system now",
                               "apply update from ADB",
                               "wipe data/factory reset",
                               "wipe cache partition",
                               "view recovery logs",
                               NULL };

#define kFBDevice "/dev/graphics/fb0"
#define FBIO_PSB_SET_RGBX       _IOWR('F', 0x42, struct fb_var_screeninfo)
#define FBIO_PSB_SET_RMODE      _IOWR('F', 0x43, struct fb_var_screeninfo)

class FuguUI : public ScreenRecoveryUI {
public:
    FuguUI() :
        up_keys(0),
        next_key_pos(0),
        pending_select(false),
        long_press(false) {
        pthread_mutex_init(&long_mu, NULL);
        memset(last_keys, 0, kKeyBufferSize * sizeof(int));
    }

    void Init() override {
        SetupDisplayMode();
        ScreenRecoveryUI::Init();
    }

    void SetupDisplayMode() {
        int fb_dev = open(kFBDevice, O_RDWR);
        int res;
        uint32_t i;
        printf("opening fb %s\n", kFBDevice);
        if (fb_dev < 0) {
            fprintf(stderr, "FAIL: failed to open \"%s\" (errno = %d)\n", kFBDevice, errno);
            return;
        }

        struct fb_var_screeninfo current_mode;

        res = ioctl(fb_dev, FBIO_PSB_SET_RMODE, &current_mode);
        if (res) {
            fprintf(stderr,
                "FAIL: unable to set RGBX mode on display controller (errno = %d)\n",
                errno);
            return;
        }

        res = ioctl(fb_dev, FBIOGET_VSCREENINFO, &current_mode);
        if (res) {
            fprintf(stderr, "FAIL: unable to get mode, err %d\n", res);
            return;
        }

        res = ioctl(fb_dev, FBIOBLANK, FB_BLANK_POWERDOWN);
        if (res) {
            fprintf(stderr, "FAIL: unable to blank display, err %d\n", res);
            return;
        }

        current_mode.bits_per_pixel = 32;
        current_mode.red.offset = 0;
        current_mode.red.length = 8;
        current_mode.green.offset = 8;
        current_mode.green.length = 8;
        current_mode.blue.offset = 16;
        current_mode.blue.length = 8;

        res = ioctl(fb_dev, FBIOPUT_VSCREENINFO, &current_mode);
        if (res) {
            fprintf(stderr, "FAIL: unable to set mode, err %d\n", res);
            return;
        }

        /* set our display controller for RGBX */
        res = ioctl(fb_dev, FBIO_PSB_SET_RGBX, &current_mode);
        if (res) {
            fprintf(stderr,
                "FAIL: unable to set RGBX mode on display controller (errno = %d)\n",
                errno);
            return;
        }

        res = ioctl(fb_dev, FBIOBLANK, FB_BLANK_UNBLANK);
        if (res) {
            fprintf(stderr, "FAIL: unable to unblank display, err %d\n", res);
            return;
        }
    }

    void SetColor(UIElement e) override {
        switch (e) {
            case HEADER:
                gr_color(247, 0, 6, 255);
                break;
            case MENU:
                gr_color(0, 106, 157, 255);
                break;
            case MENU_SEL_BG:
                pthread_mutex_lock(&long_mu);
                if (pending_select) {
                    gr_color(0, 156, 100, 255);
                } else {
                    gr_color(0, 106, 157, 255);
                }
                pthread_mutex_unlock(&long_mu);
                break;
            case MENU_SEL_FG:
                gr_color(255, 255, 255, 255);
                break;
            case LOG:
                gr_color(249, 194, 0, 255);
                break;
            case TEXT_FILL:
                gr_color(0, 0, 0, 160);
                break;
            default:
                gr_color(255, 255, 255, 255);
                break;
        }
    }

    void NextCheckKeyIsLong(bool is_long_press) override {
        long_press = is_long_press;
    }

    void KeyLongPress(int key) override {
        pthread_mutex_lock(&long_mu);
        pending_select = true;
        pthread_mutex_unlock(&long_mu);

        Redraw();
    }

    KeyAction CheckKey(int key) override {
        pthread_mutex_lock(&long_mu);
        pending_select = false;
        pthread_mutex_unlock(&long_mu);

        if (key == KEY_F1) {
            return MOUNT_SYSTEM;
        }

        if (long_press) {
            if (IsTextVisible()) {
                EnqueueKey(KEY_ENTER);
                return IGNORE;
            } else {
                return TOGGLE;
            }
        } else {
            return IsTextVisible() ? ENQUEUE : IGNORE;
        }
    }

private:
    static const int kKeyBufferSize = 100;

    int up_keys;
    int next_key_pos;
    int last_keys[kKeyBufferSize];

    pthread_mutex_t long_mu;
    bool pending_select;

    bool long_press;
};

class FuguDevice : public Device {
  public:
    FuguDevice() : ui(new FuguUI) {
    }

    RecoveryUI* GetUI() override { return ui; }

    int HandleMenuKey(int key, int visible) override {
        static int running = 0;

        if (visible) {
            switch (key) {
                case KEY_ENTER:
                    return kInvokeItem;
                    break;

                case KEY_UP:
                    return kHighlightUp;
                    break;

                case KEY_DOWN:
                case KEY_CONNECT:   // the Fugu hardware button
                    return kHighlightDown;
                    break;
            }
        }

        return kNoAction;
    }

    BuiltinAction InvokeMenuItem(int menu_position) override {
        switch (menu_position) {
          case 0: return REBOOT;
          case 1: return APPLY_ADB_SIDELOAD;
          case 2: return WIPE_DATA;
          case 3: return WIPE_CACHE;
          case 4: return READ_RECOVERY_LASTLOG;
          default: return NO_ACTION;
        }
    }

    const char* const* GetMenuHeaders() override { return HEADERS; }
    const char* const* GetMenuItems() override { return ITEMS; }

  private:
    RecoveryUI* ui;
};

Device* make_device() {
    return new FuguDevice();
}
