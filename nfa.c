#include <stdio.h>
#include <assert.h>
#include <string.h>
char* reg2post(const char* re) {
static char buf[8000];
char* dst;
int i = 0;
  dst = &buf[0];
  for(;*re;re++) {
    if(i > 1) {
      *dst++ = '.';
    }
    *dst++ = *re;
    i++;
  }
  if(i > 1)
    *dst++ = '.';
  return buf;
}

void test(void);
int main(void) {
  test();
  return 0;
}
void test(void) {
  assert(!strcmp("a",reg2post("a")) && "a -> a");
  assert(!strcmp("ab.",reg2post("ab")) && "ab -> ab.");
  assert(!strcmp("ab.c.d.e.f.g.",reg2post("abcdefg")) && "abcdefg -> ab.c.d.e.f.g.");
}

