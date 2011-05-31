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
enum {
  Match = 256
};
struct State {
  int c;
  struct State* out;
};
const char* CheckState(struct State* s, FILE* fp);
struct State match_state = {Match, NULL};
struct State* state(int c, struct State* out) {
struct State* s = malloc(sizeof(struct State));
  s->c = c; s->out = out;
  return s;
}
void patch(struct State* s1, struct State* s2) {
  s1->out = s2;
}
struct Frag {
  struct State* start;
  struct State* out;
};
struct Frag frag(struct State* s1, struct State* s2) {
  struct Frag f = {s1, s2};
  return f;
}
struct State* post2nfa(const char* post) {
  struct Frag stack[800], *stackp, e, e1, e2;
  struct State* s;
  stackp = &stack[0];
  #define push(s) *stackp++ = s
  #define pop() *--stackp
  for(; *post; post++) {
    switch(*post) {
      default:
        s = state(*post, NULL);
        push(frag(s, s));
	break;
      case '.':
        e2 = pop();
        e1 = pop();
	patch(e1.out, e2.start);
	push(frag(e1.start, e2.out));
	break;
    }
  }
  e = pop();
  patch(e.out, &match_state);
  return e.start;
}
int match(struct State* s, const char* str) {
  int c;
  for(;*str != '\0';str++) {
    c = *str & 0xff;
    if(s->c != c)
      return 0;
    s = s->out;
  }
  return s == &match_state;
}
void test(void);
int main(int argc, char* argv[]) {
  if(argc == 2 && !strcmp("test", argv[1])) {
    test();
    exit(0);
  }
  if(argc != 3) {
    fprintf(stderr, "%s regexp string", argv[0]);
    exit(1);
  }
  char* postfix = reg2post(argv[1]);
  struct State* start = post2nfa(postfix);
  int i;
  if(match(start, argv[2]))
    printf("match\n"); 
  else
    printf("no match\n"); 
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
void test_patch(void) {
  struct State* s1 = state('a', NULL);
  struct State* s2 = state('b', NULL);
  patch(s1, s2);
  assert(!strcmp("c:a, out:NN\n",CheckState(s1, NULL)));
  assert(!strcmp("c:b, out:NULL\n",CheckState(s2, NULL)));
  free(s1);free(s2);
}
void test_fragment(void) {
  struct State* s1 = state('a', NULL);
  struct State* s2 = state('b', NULL);
  struct Frag f = frag(s1, s2);
  assert(!strcmp("c:a, out:NULL\n",CheckState(f.start, NULL)));
  assert(!strcmp("c:b, out:NULL\n",CheckState(f.out, NULL)));
  free(s1);free(s2);
}
void test_post2nfa1(void) {
  struct State* s = post2nfa("ab.");
  assert(!strcmp("c:a, out:NN\n",CheckState(s, NULL)));
  assert(!strcmp("c:b, out:NN\n",CheckState(s->out, NULL)));
  assert(&match_state == s->out->out);
  struct State* outer;
  for(;s != &match_state ;s = outer) {
    outer = s->out;
    free(s);
  }
}
void test_post2nfa2(void) {
  struct State* s = post2nfa("ab.c.");
  assert(!strcmp("c:a, out:NN\n",CheckState(s, NULL)));
  assert(!strcmp("c:b, out:NN\n",CheckState(s->out, NULL)));
  assert(!strcmp("c:c, out:NN\n",CheckState(s->out->out, NULL)));
  assert(&match_state == s->out->out->out);
  struct State* outer;
  for(;s != &match_state ;s = outer) {
    outer = s->out;
    free(s);
  }
}
void test_match(void) {
  assert(match(post2nfa("ab.c."), "abc"));
  assert(!match(post2nfa("ab.c."), "ab"));
  assert(!match(post2nfa("ab.c."), "abcd"));
}
void test(void) {
  test_reg2post();
  test_create_state();
  test_patch();
  test_fragment();
  test_post2nfa1();
  test_post2nfa2();
  test_match();
}
