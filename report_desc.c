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

#if defined(USB_HID_DEBUG) && USB_HID_DEBUG
#if defined(ARDUINO)
#define printf(...)   Serial.printf(__VA_ARGS__)
#endif
#else
#define LOG_ON  0
#if defined(LOG_ON) && !LOG_ON
#define printf(...)
#endif
#endif

#define UINT16(p) (*p|(*(p + 1) << 8))
#define UINT32(p) (*p|(*(p + 1) << 8)|(*(p + 2) << 16)|(*(p + 3) << 24))

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "./report_desc.h"

typedef struct {
  uint8_t bSize:2;
  uint8_t bType:2;
  uint8_t bTag:4;
} HID_Item_Prefix_t;

enum {
  BTYPE_MAIN, BTYPE_GLOBAL, BTYPE_LOCAL
};

enum {
  MAIN_INPUT = 8,
  MAIN_OUTPUT = 9,
  MAIN_COLLECTION = 10,
  MAIN_FEATURE = 11,
  MAIN_END_COLLECTION = 12
};

enum {
  GLOBAL_USAGE_PAGE = 0,
  GLOBAL_LOGICAL_MINIMUM,
  GLOBAL_LOGICAL_MAXIMUM,
  GLOBAL_PHYSICAL_MINIMUM,
  GLOBAL_PHYSICAL_MAXIMUM,
  GLOBAL_UNIT_EXPONENT,
  GLOBAL_UNIT,
  GLOBAL_REPORT_SIZE,
  GLOBAL_REPORT_ID,
  GLOBAL_REPORT_COUNT,
  GLOBAL_PUSH,
  GLOBAL_POP
};

enum {
  LOCAL_USAGE = 0,
  LOCAL_USAGE_MINIMUM,
  LOCAL_USAGE_MAXIMUM,
  LOCAL_DESIGNATOR_INDEX,
  LOCAL_DESIGNATOR_MIN,
  LOCAL_DESIGNATOR_MAX,
  LOCAL_STRING_INDEX = 7,
  LOCAL_STRING_MIN,
  LOCAL_STRING_MAX,
  LOCAL_DELIMITER,
};

static const char *GLOBAL_ITEM_NAMES[] = {
  /*  0 */ "Usage Page",
  /*  1 */ "Logical Minimum",
  /*  2 */ "Logical Maximum",
  /*  3 */ "Physical Minimum",
  /*  4 */ "Physical Maximum",
  /*  5 */ "Unit Exponent",
  /*  6 */ "Unit",
  /*  7 */ "Report Size",
  /*  8 */ "Report ID",
  /*  9 */ "Report Count",
  /* 10 */ "Push",
  /* 11 */ "Pop",
  /* 12 */ NULL,
  /* 13 */ NULL,
  /* 14 */ NULL,
  /* 15 */ NULL,
};

static const char *LOCAL_ITEM_NAMES[] = {
  /*  0 */ "Usage",
  /*  1 */ "Usage Minimum",
  /*  2 */ "Usage Maximum",
  /*  3 */ "Designator Index",
  /*  4 */ "Designator Min",
  /*  5 */ "Designator Max",
  /*  6 */ NULL,
  /*  7 */ "String Index",
  /*  8 */ "String Min",
  /*  9 */ "String Max",
  /* 10 */ "Delimiter",
  /* 11 */ NULL,
  /* 12 */ NULL,
  /* 13 */ NULL,
  /* 14 */ NULL,
  /* 15 */ NULL,
};

typedef union {
  uint8_t u8;
  int8_t i8;
  uint16_t u16;
  int16_t i16;
  uint32_t u32;
  int32_t i32;
} ITEM_VALUE_t;

typedef struct {
  ITEM_VALUE_t value;
  int8_t size;
} ITEM_VALUE_TYPE_t;

static ITEM_VALUE_TYPE_t Global_Item_Values[16] = {0};
static ITEM_VALUE_TYPE_t Local_Item_Values[16] = {0};

static void print_items(const char *item_name[], ITEM_VALUE_TYPE_t *item_values) {
  for (size_t i = 0; i < 12; i++) {
    if (item_name[i] != NULL) {
      if (item_values[i].size < 0) {
        printf("%s (%"PRIi32")\n",
            item_name[i], item_values[i].value.i32);
      } else {
        printf("%s (%"PRIu32")\n",
            item_name[i], item_values[i].value.u32);
      }
    }
  }
}

static uint16_t Usage_Items[8];
static uint32_t Usage_Item_Count = 0;

#if LOG_ON
static const char *MAIN_ITEM_NAMES[] = {
  /*  0 */ NULL,
  /*  1 */ NULL,
  /*  2 */ NULL,
  /*  3 */ NULL,
  /*  4 */ NULL,
  /*  5 */ NULL,
  /*  6 */ NULL,
  /*  7 */ NULL,
  /*  8 */ "Input",
  /*  9 */ "Ouput",
  /* 10 */ "Collection",
  /* 11 */ "Feature",
  /* 12 */ "End Collection",
  /* 13 */ NULL,
  /* 14 */ NULL,
  /* 15 */ NULL,
};
#endif

enum {
  GENERIC_DESKTOP_PAGE = 1,
  SIMULATION_PAGE,
  VR_PAGE,
  SPORT_PAGE,
  GAME_PAGE,
  GENERIC_DEVICE_PAGE,
  KEYBOARD_PAGE,
  LED_PAGE,
  BUTTON_PAGE,
  ORDINAL_PAGE,
  TELEPHONY_DEVICE_PAGE,
  CONSUMER_PAGE,
};

enum {
  GENERIC_DESKTOP_X = 0x30,
  GENERIC_DESKTOP_Y = 0x31,
  GENERIC_DESKTOP_WHEEL = 0x38,
};

typedef struct {
  uint32_t usage;
  uint8_t offset_byte;
  uint8_t offset_bit;
  uint8_t len_in_bits;
  uint8_t filler;
} field_t;

#if 0
typedef struct {
  field_t bit_fields[8];
  uint8_t is_report_id:1;
  uint8_t is_buttons:1;
  uint8_t is_x:1;
  uint8_t is_y:1;
  uint8_t is_wheel:1;
  uint8_t is_pan:1;
  uint8_t is_signed:1;
  uint8_t is_reserved2:1;
  uint8_t report_id;
} hid_mouse_report_t;
#endif

#define MOUSE_FIELDS_MAX  (32)
static field_t mouse_fields[MOUSE_FIELDS_MAX];
static uint32_t Mouse_Field_Count = 0;

enum {
  USAGE_BUTTON = 0x00090000UL,
  USAGE_X = 0x00010030UL,
  USAGE_Y = 0x00010031UL,
  USAGE_WHEEL = 0x00010038UL,
  USAGE_REPORT_ID = 0x00000085UL,
  USAGE_FILLER = 0x0001FFFFUL,
};

static uint32_t total_offset_bit = 0;

static void parse_main_item(size_t bTag, const uint8_t *report_desc, size_t bSize) {
#if LOG_ON
  const char *tag_name;

  if (MAIN_ITEM_NAMES[bTag] == NULL) {
    tag_name = "Reserved";
  } else {
    tag_name = MAIN_ITEM_NAMES[bTag];
  }
#endif
  switch (bSize) {
    case 0:
      printf("%s\n", tag_name);
      break;
    case 1:
      printf("%s (%02x)\n", tag_name, *report_desc);
      if (bTag == MAIN_INPUT) {
        print_items(GLOBAL_ITEM_NAMES, Global_Item_Values);
        print_items(LOCAL_ITEM_NAMES, Local_Item_Values);
        uint32_t usage_page = Global_Item_Values[GLOBAL_USAGE_PAGE].value.u32;
        if (usage_page == BUTTON_PAGE) {
          printf("buttons size %"PRIu32" count %"PRIu32" total_offset_bit %"PRIu32"\n",
              Global_Item_Values[GLOBAL_REPORT_SIZE].value.u32,
              Global_Item_Values[GLOBAL_REPORT_COUNT].value.u32,
              total_offset_bit);
          if (Usage_Item_Count == 0) {
            if ((Local_Item_Values[LOCAL_USAGE_MINIMUM].value.u32 == 0) &&
                (Local_Item_Values[LOCAL_USAGE_MAXIMUM].value.u32 == 0)) {
              mouse_fields[Mouse_Field_Count].usage = USAGE_FILLER;
            } else {
              mouse_fields[Mouse_Field_Count].usage = USAGE_BUTTON;
            }
          }
          mouse_fields[Mouse_Field_Count].len_in_bits =
            Global_Item_Values[GLOBAL_REPORT_SIZE].value.u32 *
            Global_Item_Values[GLOBAL_REPORT_COUNT].value.u32;
          printf("len_in_bits %d\n", mouse_fields[Mouse_Field_Count].len_in_bits);
          mouse_fields[Mouse_Field_Count].offset_byte = total_offset_bit / 8;
          mouse_fields[Mouse_Field_Count].offset_bit = total_offset_bit % 8;
          total_offset_bit += mouse_fields[Mouse_Field_Count].len_in_bits;
          Mouse_Field_Count++;
          printf("total_offset_bit %"PRIu32" Mouse_Field_Count %"PRIu32"\n",
              total_offset_bit, Mouse_Field_Count);
        } else if (usage_page == GENERIC_DESKTOP_PAGE) {
          printf("Generic Desktop | Usage_Item_Count %"PRIu32", size %"PRIu32", count %"PRIu32", total_offset_bit %"PRIu32"\n",
              Usage_Item_Count,
              Global_Item_Values[GLOBAL_REPORT_SIZE].value.u32,
              Global_Item_Values[GLOBAL_REPORT_COUNT].value.u32,
              total_offset_bit);
          for (size_t i = 0; i < Usage_Item_Count; i++) {
            printf("Usage (0x%02X)", Usage_Items[i]);
            switch (Usage_Items[i]) {
              case GENERIC_DESKTOP_X:
                printf(" X control\n");
                mouse_fields[Mouse_Field_Count].usage = USAGE_X;
                mouse_fields[Mouse_Field_Count].len_in_bits =
                  Global_Item_Values[GLOBAL_REPORT_SIZE].value.u32;
                break;
              case GENERIC_DESKTOP_Y:
                printf(" Y control\n");
                mouse_fields[Mouse_Field_Count].usage = USAGE_Y;
                mouse_fields[Mouse_Field_Count].len_in_bits =
                  Global_Item_Values[GLOBAL_REPORT_SIZE].value.u32;
                break;
              case GENERIC_DESKTOP_WHEEL:
                printf(" wheel control\n");
                mouse_fields[Mouse_Field_Count].usage = USAGE_WHEEL;
                mouse_fields[Mouse_Field_Count].len_in_bits =
                  Global_Item_Values[GLOBAL_REPORT_SIZE].value.u32;
                break;
              default:
                printf("\n");
                mouse_fields[Mouse_Field_Count].usage = USAGE_FILLER;
                if (Usage_Item_Count == 0) {
                  mouse_fields[Mouse_Field_Count].len_in_bits =
                    Global_Item_Values[GLOBAL_REPORT_SIZE].value.u32 *
                    Global_Item_Values[GLOBAL_REPORT_COUNT].value.u32;
                } else {
                  mouse_fields[Mouse_Field_Count].len_in_bits =
                    Global_Item_Values[GLOBAL_REPORT_SIZE].value.u32;
                }
                break;
            }
            mouse_fields[Mouse_Field_Count].offset_byte = total_offset_bit / 8;
            mouse_fields[Mouse_Field_Count].offset_bit = total_offset_bit % 8;
            total_offset_bit += mouse_fields[Mouse_Field_Count].len_in_bits;
            if (Mouse_Field_Count < (MOUSE_FIELDS_MAX - 1)) {
              Mouse_Field_Count++;
              printf("Mouse_Field_Count %"PRIu32"\n", Mouse_Field_Count);
            }
          }
        } else {
          total_offset_bit += Global_Item_Values[GLOBAL_REPORT_SIZE].value.u32 *
            Global_Item_Values[GLOBAL_REPORT_COUNT].value.u32;
        }
      } else if ((bTag == MAIN_COLLECTION) && (*report_desc == 0x01)) {
        total_offset_bit = 0;
      }
      break;
    case 2:
      printf("%s (%04x)\n", tag_name, UINT16(report_desc));
      break;
    case 3:
      printf("%s (%08x)\n", tag_name, UINT32(report_desc));
      break;
  }
}


static void main_item(const uint8_t *report_desc, size_t bTag, size_t bSize) {
  parse_main_item(bTag, report_desc, bSize);
  Usage_Item_Count = 0;
  memset((uint8_t *)Local_Item_Values, 0, sizeof(Local_Item_Values));
}

static void parse_global_item(int bTag, const uint8_t *report_desc,
    size_t bSize, bool report_id) {
#if LOG_ON
  const char *tag_name;

  if (GLOBAL_ITEM_NAMES[bTag] == NULL) {
    tag_name = "Reserved";
  } else {
    tag_name = GLOBAL_ITEM_NAMES[bTag];
  }
#endif

  switch (bSize) {
    case 0:
      printf("%s\n", tag_name);
      break;
    case 1:
      if (*report_desc & 0x80) {
        Global_Item_Values[bTag].size = -1;
        Global_Item_Values[bTag].value.i32 = *(int8_t *)report_desc;
        printf("%s (%"PRIi32")\n", tag_name, Global_Item_Values[bTag].value.i32);
      } else {
        Global_Item_Values[bTag].size = 1;
        Global_Item_Values[bTag].value.u32 = *report_desc;
        printf("%s (%"PRIu32")\n", tag_name, Global_Item_Values[bTag].value.u32);
      }
      break;
    case 2:
      {
        uint16_t u16 = UINT16(report_desc);
        if (u16 & 0x8000) {
          Global_Item_Values[bTag].size = -2;
          Global_Item_Values[bTag].value.i32 = (int16_t)u16;
          printf("%s (%"PRIi32")\n", tag_name, Global_Item_Values[bTag].value.i32);
        } else {
          Global_Item_Values[bTag].size = 2;
          Global_Item_Values[bTag].value.u32 = u16;
          printf("%s (%"PRIu32")\n", tag_name, Global_Item_Values[bTag].value.i32);
        }
      }
      break;
    case 3:
      {
        uint32_t u32 = UINT32(report_desc);
        if (u32 & 0x80000000) {
          Global_Item_Values[bTag].size = -4;
          Global_Item_Values[bTag].value.i32 = (int32_t)u32;
          printf("%s (%"PRIi32")\n", tag_name, Global_Item_Values[bTag].value.i32);
        } else {
          Global_Item_Values[bTag].size = 4;
          Global_Item_Values[bTag].value.u32 = u32;
          printf("%s (%"PRIu32")\n", tag_name, u32);
        }
      }
      break;
  }
  if (report_id && (bTag == GLOBAL_REPORT_ID)) {
    if (Global_Item_Values[GLOBAL_REPORT_ID].value.u8) {
      mouse_fields[Mouse_Field_Count].usage = USAGE_REPORT_ID;
      mouse_fields[Mouse_Field_Count].len_in_bits = 8;
      total_offset_bit += mouse_fields[Mouse_Field_Count].len_in_bits;
      Mouse_Field_Count++;
      printf("total_offset_bit %"PRIu32" Mouse_Field_Count %"PRIu32"\n",
          total_offset_bit, Mouse_Field_Count);
    }
  }
}

static void global_item(const uint8_t *report_desc, size_t bTag, size_t bSize,
    bool report_id) {
  parse_global_item(bTag, report_desc, bSize, report_id);
}

static void parse_local_item(int bTag, const uint8_t *report_desc, size_t bSize) {
#if LOG_ON
  const char *tag_name;

  if (LOCAL_ITEM_NAMES[bTag] == NULL) {
    tag_name = "Reserved";
  } else {
    tag_name = LOCAL_ITEM_NAMES[bTag];
  }
#endif
  switch (bSize) {
    case 0:
      printf("%s\n", tag_name);
      break;
    case 1:
      if (*report_desc & 0x80) {
        Local_Item_Values[bTag].size = -1;
        Local_Item_Values[bTag].value.i32 = *(int8_t *)report_desc;
        printf("%s (%"PRIi32")\n", tag_name, Local_Item_Values[bTag].value.i32);
      } else {
        Local_Item_Values[bTag].size = 1;
        Local_Item_Values[bTag].value.u32 = *report_desc;
        printf("%s (%"PRIu32")\n", tag_name, Local_Item_Values[bTag].value.u32);
      }
      if (bTag == LOCAL_USAGE) {
        if (Usage_Item_Count > 7) {
          printf("Too many usage items\n");
        } else {
          Usage_Items[Usage_Item_Count++] = *report_desc;
        }
      }
      printf("%s (0x%02X)\n", tag_name, *report_desc);
      break;
    case 2:
      {
        uint16_t u16 = UINT16(report_desc);
        if (u16 & 0x8000) {
          Local_Item_Values[bTag].size = -2;
          Local_Item_Values[bTag].value.i32 = (int16_t)u16;
          printf("%s (%"PRIi32")\n", tag_name, Local_Item_Values[bTag].value.i32);
        } else {
          Local_Item_Values[bTag].size = 2;
          Local_Item_Values[bTag].value.u32 = u16;
          printf("%s (%"PRIu32")\n", tag_name, Local_Item_Values[bTag].value.i32);
        }
      }
      if (bTag == LOCAL_USAGE) {
        if (Usage_Item_Count > 7) {
          printf("Too many usage items\n");
        } else {
          Usage_Items[Usage_Item_Count++] = UINT16(report_desc);
        }
      }
      printf("%s (0x%04X)\n", tag_name, UINT16(report_desc));
      break;
    case 3:
      {
        uint32_t u32 = UINT32(report_desc);
        if (u32 & 0x80000000) {
          Local_Item_Values[bTag].size = -4;
          Local_Item_Values[bTag].value.i32 = (int32_t)u32;
          printf("%s (%"PRIi32")\n", tag_name, Local_Item_Values[bTag].value.i32);
        } else {
          Local_Item_Values[bTag].size = 4;
          Local_Item_Values[bTag].value.u32 = u32;
          printf("%s (%"PRIu32")\n", tag_name, u32);
        }
      }
      if (bTag == LOCAL_USAGE) {
        printf("Usage 16 bits or less");
      }
      printf("%s (0x%08X)\n", tag_name, UINT32(report_desc));
      break;
  }
}

static void local_item(const uint8_t *report_desc, size_t bTag, size_t bSize) {
  parse_local_item(bTag, report_desc, bSize);
}

void parse_hid_report_descriptor(const uint8_t *report_desc, size_t desc_len,
    bool report_id) {
  if ((report_desc == NULL) || (desc_len == 0)) return;
  Mouse_Field_Count = 0;
  memset((void *)Global_Item_Values, 0, sizeof(Global_Item_Values));
  memset((void *)Local_Item_Values, 0, sizeof(Local_Item_Values));
  while (desc_len > 0) {
    HID_Item_Prefix_t *prefix = (HID_Item_Prefix_t *)report_desc;
    report_desc++;
    desc_len--;
    switch (prefix->bType) {
      case BTYPE_MAIN:    // Main items
        printf("M: ");
        main_item(report_desc, prefix->bTag, prefix->bSize);
        break;
      case BTYPE_GLOBAL:  // Global items
        printf("G: ");
        global_item(report_desc, prefix->bTag, prefix->bSize, report_id);
        break;
      case BTYPE_LOCAL:   // Local items
        printf("L: ");
        local_item(report_desc, prefix->bTag, prefix->bSize);
        break;
      default:
        printf("Invalid bType = %d\n", prefix->bType);
        break;
    }
    size_t skip_bytes = (prefix->bSize == 3) ? 4 : prefix->bSize;
    report_desc += skip_bytes;
    desc_len -= skip_bytes;
  }
}

void extract_mouse_values(const uint8_t *report, mouse_values_t *mouse_values) {
  mouse_values->report_id = 0;
  mouse_values->x = 0;
  mouse_values->y = 0;
  mouse_values->wheel = 0;
  mouse_values->pan = 0;
  mouse_values->buttons = 0;
  printf("Mouse_Field_Count %"PRIu32"\n", Mouse_Field_Count);
  for (size_t i = 0; i < Mouse_Field_Count; i++) {
    printf("offset_byte %u", mouse_fields[i].offset_byte);
    printf(" offset_bit %u", mouse_fields[i].offset_bit);
    printf(" len_in_bits %u", mouse_fields[i].len_in_bits);
    printf(" usage %"PRIu32"\n", mouse_fields[i].usage);
    printf("%x, %x, %x, %x\n", report[0], report[1], report[2], report[3]);
    uint32_t u32 = UINT32(&report[mouse_fields[i].offset_byte]);
    printf("u32 %"PRIu32"\n", u32);
    uint8_t offset_bit =  mouse_fields[i].offset_bit;
    u32 >>= offset_bit;
    printf("u32 %"PRIu32"\n", u32);
    uint8_t bit_len = mouse_fields[i].len_in_bits;
    uint32_t mask = (1 << bit_len) - 1;
    printf("mask %"PRIu32"\n", mask);
    u32 = (u32 & mask);
    int32_t i32;
    if (u32 & (1 << (bit_len - 1))) {
      i32 = (int32_t)(~mask | u32);
    } else {
      i32 = (int32_t)u32;
    }
    printf("u32 %"PRIu32", i32 %"PRIi32"\n", u32, i32);
    switch (mouse_fields[i].usage) {
      case USAGE_X:
        mouse_values->x = i32;
        break;
      case USAGE_Y:
        mouse_values->y = i32;
        break;
      case USAGE_WHEEL:
        mouse_values->wheel = i32;
        break;
      case USAGE_BUTTON:
        mouse_values->buttons = u32;
        break;
      case USAGE_REPORT_ID:
        mouse_values->report_id = u32;
        break;
    }
  }
}

#if DEBUG_HID_MAIN
#include "./hid_desc.h"
#include "./perixx.h"
#include "./black_trackpad.h"
#include "./jellycomb.h"
#include "./microsoft_ble_mouse.h"
#include "./trackball_ble.h"
#include "./clicker_1_button.h"
#include "./clicker_6_button.h"

int main(void) {
  printf("## HID_Descriptor\n");
  parse_hid_report_descriptor(HID_Descriptor, sizeof(HID_Descriptor), true);
  printf("\n## Perixx 0\n");
  parse_hid_report_descriptor(HID_Descriptor_Perixx_0,
      sizeof(HID_Descriptor_Perixx_0), true);
  printf("\n## Perixx 1\n");
  parse_hid_report_descriptor(HID_Descriptor_Perixx_1,
      sizeof(HID_Descriptor_Perixx_1), true);
  printf("\n## Black Trackpad 1\n");
  parse_hid_report_descriptor(black_trackpad_1, sizeof(black_trackpad_1), true);
  printf("\n## Black Trackpad 2\n");
  parse_hid_report_descriptor(black_trackpad_2, sizeof(black_trackpad_2), true);
  printf("\n## Jelly Comb\n");
  parse_hid_report_descriptor(jellycomb_ble, sizeof(jellycomb_ble), true);
  printf("\n## Microsoft BLE mouse\n");
  parse_hid_report_descriptor(microsoft_ble_mouse, sizeof(microsoft_ble_mouse), true);
  printf("\n## Clicker 1 button\n");
  parse_hid_report_descriptor(clicker_1_button, sizeof(clicker_1_button), true);
  printf("\n## Clicker 6 button\n");
  parse_hid_report_descriptor(clicker_6_button, sizeof(clicker_6_button), true);
  printf("\n## Trackball BLE\n");
  parse_hid_report_descriptor(trackball_ble, sizeof(trackball_ble), false);
  mouse_values_t mouse_values;
  uint8_t report[] = {0x1F, 0xFF, 0x01, 0x01, 0x80};
  extract_mouse_values(report, &mouse_values);
  printf("id %d, buttons %x, x %d, y %d\n", mouse_values.report_id,
      mouse_values.buttons, mouse_values.x, mouse_values.y);
  return 0;
}
#endif
