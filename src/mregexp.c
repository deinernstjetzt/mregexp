#include "mregexp.h"

#include <stdint.h>

/**
 * @brief convert a utf8 string into a string of unicode codepoints
 * @param n length of the utf8 string
 * @param string utf8 encoded string with a length of at least n bytes
 * @param buf_len maximum length of the output buffer
 * @param buf output buffer where codepoints are stored
 * @param out_len destination to write the amount of generated unicode codepoints to
 * @return amount of bytes consumed during the conversion
 */
size_t mregexp_parse_utf8(size_t n, const char* string, size_t buf_len, uint32_t* buf,
                          size_t* out_len) {
  const uint8_t* ustring = (const uint8_t*)string;
  size_t pos = 0;
  size_t out_pos = 0;

  while (pos < n && out_pos < buf_len) {
    int width = -1;
    uint32_t res = 0;

    if (ustring[pos] < 0x7F) {
      width = 1;
      res = (uint32_t)ustring[pos];
    } else if ((ustring[pos] & 0xE0) == 0xC0) {
      width = 2;
      res = (uint32_t)ustring[pos] & 0x1F;
    } else if ((ustring[pos] & 0xF0) == 0xE0) {
      width = 3;
      res = (uint32_t)ustring[pos] & 0x0F;
    } else if ((ustring[pos] & 0xF8) == 0xF0) {
      width = 4;
      res = (uint32_t)ustring[pos] & 0x07;
    } else {
      width = 1;
      res = 0xFFFD;
    }

    if (pos + width <= n) {
      for (int i = 1; i < width; ++i) {
        if ((ustring[pos + i] & 0xC0) != 0x80) {
          res = 0xFFFD;
          break;
        }

        res <<= 6;
        res |= (uint32_t)ustring[pos + 1] & 0x3F;
      }
    } else {
      res = 0xFFFD;
    }

    buf[out_pos] = res;

    pos += width;
    out_pos += 1;
  }

  if (out_len) {
    *out_len = out_pos;
  }

  return pos;
}

mregexp* mregexp_compile(size_t n, const char* regex, mregexp_error* out_error) { return NULL; }

bool mregexp_first_match(const mregexp* regex, size_t n, const char* haystack,
                         mregexp_match* out_match) {
  return NULL;
}
