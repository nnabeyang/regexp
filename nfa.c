#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
char* reg2post(const char* re) {
static char buf[8000];
char* dst;
int i = 0;
int j = 0;
  dst = &buf[0];
  for(;*re;re++) {
    switch(*re) {
      default:
        if(i > 1) {
          *dst++ = '.';
	  i--;
        }
        *dst++ = *re;
        i++;
        break;
      case '|':
        assert(i > 0);
	i--;
        if(i == 1) {
          *dst++ = '.';
	  i--;
	}
        j++;
	break;
    }
  }
  if(i > 1)
    *dst++ = '.';
  for(;j > 0;j--)
    *dst++ = '|';

  *dst = '\0';
  return buf;
}
enum {
  Match = 256,
  Split = 257
};
struct State {
  int c;
  struct State* out;
  struct State* out1;
};
union PtrList {
  union PtrList* next;
  struct State* s;
};
union PtrList* list(struct State** s) {
union PtrList* l = (union PtrList*)s;
  l->next = NULL;
  return l;
}
union PtrList* append(union PtrList* l1, union PtrList* l2) {
union PtrList* head = l1;
  while(l1->next)
    l1 = l1->next;
  l1->next = l2;
  return head;
}
const char* CheckState(struct State* s, FILE* fp);
struct State match_state = {Match, NULL, NULL};
static int nstate = 0;
struct State* state(int c, struct State* out, struct State* out1) {
  nstate++;
struct State* s = malloc(sizeof(struct State));
  s->c = c; s->out = out; s->out1 = out1;
  return s;
}
void patch(union PtrList* l, struct State* s) {
  union PtrList* next;
  for(; l; l = next) {
    next = l->next;
    l->s = s;
  }
}
struct Frag {
  struct State* start;
  union PtrList* out;
};
struct Frag frag(struct State* s, union PtrList* l) {
  struct Frag f = {s, l};
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
        s = state(*post, NULL, NULL);
        push(frag(s, list(&s->out)));
	break;
      case '.':
        e2 = pop();
        e1 = pop();
	patch(e1.out, e2.start);
	push(frag(e1.start, e2.out));
	break;
      case '|':
        e2 = pop();
	e1 = pop();
	s = state(Split, e1.start, e2.start);
	push(frag(s, append(e1.out, e2.out)));
	break;
    }
  }
  e = pop();
  patch(e.out, &match_state);
  return e.start;
}
struct StateList {
  struct State** s;
  int n;
};
struct StateList l1, l2;
void addstate(struct StateList* l, struct State* s);
void startlist(struct StateList* l, struct State* s) {
  l->n = 0;
  addstate(l, s);
}
void addstate(struct StateList* l, struct State* s) {
  if(s->c == Split) {
    l->s[l->n++] = s->out;
    l->s[l->n++] = s->out1;
    return;
  }
  l->s[l->n++] = s;
}
void step(struct StateList* l1, int c, struct StateList* l2) {
  struct State* s;
  int i;
  for(i = 0; i < l1->n; i++) {
    s = l1->s[i];
    if(s->out == NULL)
      return;
    if(s->c == c)
      addstate(l2, s->out);
  }
}
int ismatch(struct StateList* l) {
  int i;
  for(i = 0; i < l->n; i++) {
   if(l->s[i] == &match_state)
     return 1;
  }
  return 0;
}
int match(struct State* s, const char* str) {
  l1.n = 0; l2.n = 0;
  struct StateList *clist, *nlist, *tlist;
  clist = &l1;
  nlist = &l2;
  startlist(clist, s);
  int c;
  for(;*str != '\0';str++) {
    c = *str & 0xff;
    step(clist, c, nlist);
    tlist = nlist; nlist = clist; clist = tlist;
    // nlistを以前のものにすることによって、更新されないときmatchしないように
    // する。
  }
  return ismatch(clist);
}
void test(void);
int main(int argc, char* argv[]) {

  if(argc == 2 && !strcmp("test", argv[1])) {
    test();
    exit(0);
  }
  if(argc < 3) {
    fprintf(stderr, "%s regexp string...\n", argv[0]);
    exit(1);
  }
  char* postfix = reg2post(argv[1]);
  struct State* start = post2nfa(postfix);
  nstate = 0;
  l1.s = malloc(nstate* sizeof(l1.s[0]));
  l2.s = malloc(nstate* sizeof(l2.s[0])); 
  int i;
  for(i = 2; i < argc; i++) {
    if(match(start, argv[i]))
      printf("%s\n", argv[i]); 
  
  }
  return 0;
}
void test_reg2post(void) {
  assert(!strcmp("a",reg2post("a")) && "a -> a");
  assert(!strcmp("ab.",reg2post("ab")) && "ab -> ab.");
  assert(!strcmp("ab.c.d.e.f.g.",reg2post("abcdefg")) && "abcdefg -> ab.c.d.e.f.g.");
  assert(!strcmp("ab|", reg2post("a|b")));
  assert(!strcmp("abc||", reg2post("a|b|c")));
  assert(!strcmp("ab.cd.|", reg2post("ab|cd")));
  assert(!strcmp("ab.c.de.f.gh.i.||", reg2post("abc|def|ghi")));
}
const char* CheckState(struct State* s, FILE* fp) {
static char buf[80];
  if(s->c == Match) {
    if(fp) 
      fprintf(fp, "c:Match, out:%s, out1:%s\n", (s->out)? "NN": "NULL",(s->out1)?"NN": "NULL");   
    sprintf(buf, "c:Match, out:%s, out1:%s\n", (s->out)? "NN": "NULL", (s->out1)?"NN": "NULL");
  } else if(s->c == Split) {
    if(fp) 
      fprintf(fp, "c:Split, out:%s, out1:%s\n", (s->out)? "NN": "NULL",(s->out1)?"NN": "NULL");   
    sprintf(buf, "c:Split, out:%s, out1:%s\n", (s->out)? "NN": "NULL", (s->out1)?"NN": "NULL");
  } else {
    if(fp) 
      fprintf(fp, "c:%c, out:%s, out1:%s\n",(char)s->c, (s->out)? "NN": "NULL", (s->out1)? "NN": "NULL");   
    sprintf(buf, "c:%c, out:%s, out1:%s\n",(char)s->c, (s->out)? "NN": "NULL", (s->out1)? "NN": "NULL"); 
  }
  return buf; 
}
void test_create_state(void) {
  struct State* s1 = state('a', NULL, NULL);
  struct State* s2 = state('b', s1, NULL);
  assert(!strcmp("c:a, out:NULL, out1:NULL\n",CheckState(s1, NULL)));
  assert(!strcmp("c:b, out:NN, out1:NULL\n",CheckState(s2, NULL)));
  free(s1);free(s2);
}

void test_patch(void) {
  struct State* s1 = state('a', NULL, NULL);
  struct State* s2 = state('b', NULL, NULL);
  union PtrList* l1 = list(&s1->out);
  patch(l1, s2);
  assert(!strcmp("c:a, out:NN, out1:NULL\n",CheckState(s1, NULL)));
  assert(!strcmp("c:b, out:NULL, out1:NULL\n",CheckState(s2, NULL)));
  free(s1);free(s2);
}
void test_append(void) {
  struct State* s1 = state('a', NULL, NULL);
  struct State* s2 = state('b', NULL, NULL);
  struct State* s3 = state('c', NULL, NULL);
  union PtrList* l1 = list(&s1->out);
  union PtrList* l2 = list(&s2->out);
  l1 = append(l1, l2);
  union PtrList* next;
  for(; l1; l1 = next) {
    next = l1->next;
    l1->s = s3;
  }
  assert(!strcmp("c:a, out:NN, out1:NULL\n",CheckState(s1, NULL)));
  assert(!strcmp("c:b, out:NN, out1:NULL\n",CheckState(s2, NULL)));
  free(s1);free(s2);free(s3);
}

void test_fragment(void) {
  struct State* s1 = state('a', NULL, NULL);
  struct State* s2 = state('b', NULL, NULL);
  struct Frag f = frag(s1, list(&s1->out));
  assert(f.out->s == NULL);
  patch(f.out, s2);
  assert(!strcmp("c:a, out:NN, out1:NULL\n",CheckState(f.start, NULL)));
  assert(!strcmp("c:b, out:NULL, out1:NULL\n",CheckState(f.out->s, NULL)));
  free(s1);free(s2);
}

void test_post2nfa1(void) {
  struct State* s = post2nfa("ab.");
  assert(!strcmp("c:a, out:NN, out1:NULL\n",CheckState(s, NULL)));
  assert(!strcmp("c:b, out:NN, out1:NULL\n",CheckState(s->out, NULL)));
  assert(&match_state == s->out->out);
  struct State* outer;
  for(;s != &match_state ;s = outer) {
    outer = s->out;
    free(s);
  }
}

void test_post2nfa2(void) {
  struct State* s = post2nfa("ab.c.");
  assert(!strcmp("c:a, out:NN, out1:NULL\n",CheckState(s, NULL)));
  assert(!strcmp("c:b, out:NN, out1:NULL\n",CheckState(s->out, NULL)));
  assert(!strcmp("c:c, out:NN, out1:NULL\n",CheckState(s->out->out, NULL)));
  assert(&match_state == s->out->out->out);
  struct State* outer;
  for(;s != &match_state ;s = outer) {
    outer = s->out;
    free(s);
  }
}

void test_post2nfa_alt(void) {
  struct State* s = post2nfa("ab|");
  assert(!strcmp("c:Split, out:NN, out1:NN\n",CheckState(s, NULL)));
  assert(!strcmp("c:a, out:NN, out1:NULL\n",CheckState(s->out, NULL)));
  assert(!strcmp("c:b, out:NN, out1:NULL\n",CheckState(s->out1, NULL)));
  assert(&match_state == s->out->out);
  assert(&match_state == s->out1->out);
  free(s); free(s->out); free(s->out1);
}
void test_startlist(void) {
  nstate = 0;
  struct State* s = post2nfa("ab.");
  l1.s = malloc(nstate* sizeof(l1.s[0]));
  startlist(&l1, s);
  assert(!strcmp("c:a, out:NN, out1:NULL\n",CheckState(l1.s[0], NULL)));
  assert(1 == l1.n);
  free(l1.s);
}

void test_startlist_alt(void) {
  nstate = 0;
  struct State* s = post2nfa("ab|");
  l1.s = malloc(nstate* sizeof(l1.s[0]));
  startlist(&l1, s);
  assert(2 == l1.n);
  assert(!strcmp("c:a, out:NN, out1:NULL\n",CheckState(l1.s[0], NULL)));
  assert(!strcmp("c:b, out:NN, out1:NULL\n",CheckState(l1.s[1], NULL)));
  free(l1.s);
}

void test_match(void) {
  nstate = 0;
  l1.s = malloc(nstate* sizeof(l1.s[0]));
  l2.s = malloc(nstate* sizeof(l2.s[0]));
  struct State* s = post2nfa("ab.c.");
  assert(match(s, "abc"));
  assert(!match(s, "ab"));
  assert(!match(s, "abcd"));
  free(l1.s); free(l2.s);
}

void test_match_alt(void) {
  assert(match(post2nfa("ab|"), "a"));
  assert(match(post2nfa("ab|"), "b"));
  assert(!match(post2nfa("ab|"), "c"));
  assert(!match(post2nfa("ab|"), "ab"));
}

void test(void) {
  test_reg2post();
  test_create_state();
  test_patch();
  test_append();
  test_fragment();
  test_post2nfa1();
  test_post2nfa2();
  test_post2nfa_alt();
  test_startlist();
  test_match();
  test_match_alt();
}
