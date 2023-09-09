/*
 * MIT License
 *
 * Copyright (c) 2023 touchgadgetdev@gmail.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef _REPORT_DESC_H_
#define _REPORT_DESC_H_
#define DEBUG_HID_MAIN 0
#define USB_HID_DEBUG 0

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef struct {
  uint32_t buttons;
  int32_t x;
  int32_t y;
  int32_t wheel;
  int32_t pan;
  uint8_t report_id;
} mouse_values_t;

/*
 * Parse a HID report descriptor. This only saves information about
 * HID mouse devices.
 * report_desc points to the descriptor bytes.
 * report_id if false, ignore the report ID field. Used for ESP32.
 * desc_len is the number of descriptor bytes.
 */
void parse_hid_report_descriptor(const uint8_t *report_desc, size_t desc_len,
    bool report_id);

/*
 * Extract mouse parameters from a HID report.
 *
 * A HID report contains mouse parameters such a X and Y movement and button
 * presses/releases. This function uses information from parsing the HID
 * report descriptor to return mouse paramters.
 *
 * report is the HID report sent via an interrupt endpoint
 * mouse_values are the extract mouse parameter values.
 */
void extract_mouse_values(const uint8_t *report, mouse_values_t *mouse_values);

#endif  /* _REPORT_DESC_H_ */
