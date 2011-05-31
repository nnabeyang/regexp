#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
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
struct State {
  int c;
  struct State* out;
};
struct State* state(int c, struct State* out) {
struct State* s = malloc(sizeof(struct State));
  s->c = c; s->out = out;
  return s;
}
void test(void);
int main(void) {
  test();
  return 0;
}
void test_reg2post(void) {
  assert(!strcmp("a",reg2post("a")) && "a -> a");
  assert(!strcmp("ab.",reg2post("ab")) && "ab -> ab.");
  assert(!strcmp("ab.c.d.e.f.g.",reg2post("abcdefg")) && "abcdefg -> ab.c.d.e.f.g.");
}
const char* CheckState(struct State* s, FILE* fp) {
static char buf[80];
  if(fp) 
    fprintf(fp, "c:%c, out:%s\n", (char)s->c, (s->out)? "NN": "NULL");   
  sprintf(buf, "c:%c, out:%s\n", (char)s->c, (s->out)? "NN": "NULL");
  return buf; 
}
void test_create_state(void) {
  struct State* s1 = state('a', NULL);
  struct State* s2 = state('b', s1);
  assert(!strcmp("c:a, out:NULL\n",CheckState(s1, NULL)));
  assert(!strcmp("c:b, out:NN\n",CheckState(s2, NULL)));
  free(s1);free(s2);
}
void test(void) {
  test_reg2post();
  test_create_state();
}
