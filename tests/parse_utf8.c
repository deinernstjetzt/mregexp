#include <assert.h>
#include <stdint.h>
#include <string.h>

size_t mregexp_parse_utf8(size_t n, const char* string, size_t buf_len, uint32_t* buf,
                          size_t* out_len);

int main(void) {
  const uint32_t target[] = {'H', 228, 'l', 'l', 0xF6, ' ', 'W', 0xF6, 'r', 'l', 'd'};
  const char* test_string = "Hällö Wörld";
  size_t length = strlen(test_string);
  uint32_t buf[11];

  size_t buf_len;
  size_t res_len = mregexp_parse_utf8(length, test_string, 11, buf, &buf_len);

  assert(res_len == length);
  assert(buf_len == 11);

  for (size_t i = 0; i < 11; ++i) {
    assert(buf[i] == target[i]);
  }

  return 0;
}
