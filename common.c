/*
 * Public domain / CC0. Use freely for any purpose. RoyR 2026
 * common.c  —  Compiler for the "Common" language
 * Outputs NASM x86_32 assembly (cdecl, ELF32)
 *
 * Build:   gcc -o common common.c
 * Usage:   ./common source.cm > output.asm
 *          nasm -f elf32 output.asm -o output.o
 *          gcc -m32 output.o -o output
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

/* ══════════════════════════════════════════════════════════
   TOKENS
   ══════════════════════════════════════════════════════════ */
typedef enum {
  /* literals */
  TK_NUM, TK_STR, TK_ID,
  /* types */
  TK_UINT8, TK_UINT16, TK_UINT32, TK_UINT64, TK_VOID,
  TK_INT8,  TK_INT16,  TK_INT32, TK_INT64,
  /* keywords */
  TK_IF, TK_ELSE, TK_WHILE, TK_FOR,
  TK_SWITCH, TK_CASE, TK_DEFAULT,
  TK_BREAK, TK_CONTINUE, TK_RETURN,
  /* operators (multi-char first) */
  TK_EQ, TK_NEQ, TK_LEQ, TK_GEQ, TK_AND, TK_OR,
  TK_SHL, TK_SHR, TK_INC, TK_DEC,
  TK_ADDEQ, TK_SUBEQ, TK_MULEQ, TK_DIVEQ, TK_MODEQ,
  TK_ANDEQ, TK_OREQ,  TK_XOREQ, TK_SHLEQ, TK_SHREQ,
  /* single-char operators / punctuation */
  TK_PLUS, TK_MINUS, TK_STAR, TK_SLASH, TK_MOD,
  TK_AMP, TK_PIPE, TK_CARET, TK_TILDE, TK_BANG,
  TK_LT, TK_GT, TK_ASSIGN,
  TK_LPAREN, TK_RPAREN, TK_LBRACE, TK_RBRACE,
  TK_LBRACK, TK_RBRACK,
  TK_SEMI, TK_COMMA, TK_COLON, TK_QUESTION,
  TK_EOF
} TKind;

typedef struct {
  TKind   kind;
  char    str[512];   /* identifier / string value — 512 to match str_val */
  int     slen;       /* explicit byte count for str, excl. terminator */
  long    num;        /* numeric value */
  int     line;       /* source line where token starts */
} Token;

/* ══════════════════════════════════════════════════════════
   LEXER
   ══════════════════════════════════════════════════════════ */
static const char *src;
static int      src_pos;
static int      src_line = 1;  /* current line number (1-based) */
static Token    tok;           /* current token */

static void die(const char *fmt, ...) {
  fprintf(stderr, "line %d: ", src_line);
  va_list ap; va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fputc('\n', stderr);
  exit(1);
}

static void skip_ws(void) {
  for (;;) {
    while (isspace((unsigned char)src[src_pos])) {
      if (src[src_pos] == '\n') src_line++;
      src_pos++;
    }
    if (src[src_pos]=='/' && src[src_pos+1]=='/') {
      while (src[src_pos] && src[src_pos]!='\n') src_pos++;
    } else if (src[src_pos]=='/' && src[src_pos+1]=='*') {
      src_pos += 2;
      while (src[src_pos] && !(src[src_pos-1]=='*' && src[src_pos]=='/')) {
	if (src[src_pos] == '\n') src_line++;
	src_pos++;
      }
      if (src[src_pos]) src_pos++;
    } else break;
  }
}

static struct { const char *kw; TKind tk; } kw_table[] = {
  {"if",TK_IF},{"else",TK_ELSE},{"while",TK_WHILE},{"for",TK_FOR},
  {"switch",TK_SWITCH},{"case",TK_CASE},{"default",TK_DEFAULT},
  {"break",TK_BREAK},{"continue",TK_CONTINUE},{"return",TK_RETURN},
  {"void",TK_VOID},{"uint8",TK_UINT8},{"uint16",TK_UINT16},
  {"uint32",TK_UINT32},{"uint64",TK_UINT64},
  {"int8",TK_INT8},{"int16",TK_INT16},{"int32",TK_INT32},
  {"int64", TK_INT64},{NULL,TK_EOF}
};

static void next(void) {
  skip_ws();
  tok.line = src_line;
  char c = src[src_pos];
  if (!c) { tok.kind = TK_EOF; return; }

  /* number */
  if (isdigit((unsigned char)c)) {
    char *end;
    tok.num  = (long)strtoul(src+src_pos, &end, 0);
    src_pos  = (int)(end - src);
    tok.kind = TK_NUM; return;
  }

  /* string literal */
  if (c == '"') {
    src_pos++; int i=0;
    while (src[src_pos] && src[src_pos]!='"') {
      /* bounds check before writing into tok.str */
      if (i >= 511) die("string literal too long");
      if (src[src_pos]=='\\') {
	src_pos++;
	switch(src[src_pos]) {
	case 'n': tok.str[i++]='\n'; break;
	case 't': tok.str[i++]='\t'; break;
	case 'r': tok.str[i++]='\r'; break;
	case '0': tok.str[i++]='\0'; break;
	default:  tok.str[i++]=src[src_pos]; break;
	}
      } else tok.str[i++]=src[src_pos];
      src_pos++;
    }
    if (src[src_pos]=='"') src_pos++;
    tok.str[i]=0;
    tok.slen=i;     /* record true byte count */
    tok.kind=TK_STR;
    return;
  }

  /* identifier / keyword */
  if (isalpha((unsigned char)c) || c=='_') {
    int i=0;
    while (isalnum((unsigned char)src[src_pos]) || src[src_pos]=='_') {
      /* bounds check for identifiers too */
      if (i >= 511) die("identifier too long");
      tok.str[i++]=src[src_pos++];
    }
    tok.str[i]=0;
    tok.slen=i;
    tok.kind=TK_ID;
    for (int k=0; kw_table[k].kw; k++)
      if (!strcmp(tok.str, kw_table[k].kw)) { tok.kind=kw_table[k].tk; return; }
    return;
  }

  /* multi/single char operators */
  src_pos++;
#define PEEK src[src_pos]
#define EAT  src_pos++
  switch(c) {
  case '+': if(PEEK=='+'){ EAT; tok.kind=TK_INC; }
    else if(PEEK=='='){ EAT; tok.kind=TK_ADDEQ; }
    else tok.kind=TK_PLUS; break;
  case '-': if(PEEK=='-'){ EAT; tok.kind=TK_DEC; }
    else if(PEEK=='='){ EAT; tok.kind=TK_SUBEQ; }
    else tok.kind=TK_MINUS; break;
  case '*': if(PEEK=='='){ EAT; tok.kind=TK_MULEQ; } else tok.kind=TK_STAR;  break;
  case '/': if(PEEK=='='){ EAT; tok.kind=TK_DIVEQ; } else tok.kind=TK_SLASH; break;
  case '%': if(PEEK=='='){ EAT; tok.kind=TK_MODEQ; } else tok.kind=TK_MOD;   break;
  case '&': if(PEEK=='&'){ EAT; tok.kind=TK_AND; }
    else if(PEEK=='='){ EAT; tok.kind=TK_ANDEQ; }
    else tok.kind=TK_AMP; break;
  case '|': if(PEEK=='|'){ EAT; tok.kind=TK_OR; }
    else if(PEEK=='='){ EAT; tok.kind=TK_OREQ; }
    else tok.kind=TK_PIPE; break;
  case '^': if(PEEK=='='){ EAT; tok.kind=TK_XOREQ; } else tok.kind=TK_CARET; break;
  case '<': if(PEEK=='<'){ EAT; if(PEEK=='='){ EAT; tok.kind=TK_SHLEQ; } else tok.kind=TK_SHL; }
    else if(PEEK=='='){ EAT; tok.kind=TK_LEQ; } else tok.kind=TK_LT; break;
  case '>': if(PEEK=='>'){ EAT; if(PEEK=='='){ EAT; tok.kind=TK_SHREQ; } else tok.kind=TK_SHR; }
    else if(PEEK=='='){ EAT; tok.kind=TK_GEQ; } else tok.kind=TK_GT; break;
  case '=': if(PEEK=='='){ EAT; tok.kind=TK_EQ;  } else tok.kind=TK_ASSIGN; break;
  case '!': if(PEEK=='='){ EAT; tok.kind=TK_NEQ; } else tok.kind=TK_BANG;   break;
  case '~': tok.kind=TK_TILDE;   break;
  case '(': tok.kind=TK_LPAREN;  break;
  case ')': tok.kind=TK_RPAREN;  break;
  case '{': tok.kind=TK_LBRACE;  break;
  case '}': tok.kind=TK_RBRACE;  break;
  case '[': tok.kind=TK_LBRACK;  break;
  case ']': tok.kind=TK_RBRACK;  break;
  case ';': tok.kind=TK_SEMI;    break;
  case ',': tok.kind=TK_COMMA;   break;
  case ':': tok.kind=TK_COLON;   break;
  case '?': tok.kind=TK_QUESTION;break;
  default:  die("Unknown char '%c'", c);
  }
#undef PEEK
#undef EAT
}

static void expect(TKind k) {
  if (tok.kind != k) {
    fprintf(stderr, "line %d: syntax error near '%s'\n", tok.line, tok.str);
    exit(1);
  }
  next();
}
static int accept(TKind k) {
  if (tok.kind == k) { next(); return 1; }
  return 0;
}
static int is_type(void) {
  return tok.kind==TK_UINT8||tok.kind==TK_UINT16||
    tok.kind==TK_UINT32||tok.kind==TK_UINT64||tok.kind==TK_VOID||
    tok.kind==TK_INT8||tok.kind==TK_INT16||tok.kind==TK_INT32||
    tok.kind==TK_INT64;
}
static int is_signed(TKind t) {
  return t==TK_INT8||t==TK_INT16||t==TK_INT32||t==TK_INT64;
}
static int is_64bit(TKind t) {
  return t==TK_UINT64||t==TK_INT64;
}

/* ══════════════════════════════════════════════════════════
   AST NODES
   ══════════════════════════════════════════════════════════ */
typedef enum {
  N_PROG, N_FUNC, N_FDECL, N_GVAR, N_GARR,
  N_BLK, N_LVAR, N_LARR, N_ES, N_RET, N_BRK, N_CONT,
  N_IF, N_WHILE, N_FOR, N_SW, N_CASE, N_DEF,
  N_NUM, N_STR, N_ID,
  N_BOPN, N_UOPN, N_ASGN, N_TERN,
  N_CALL, N_IDX, N_ADDR, N_DEREF, N_CAST,
  N_POSTINC, N_POSTDEC, N_PREINC, N_PREDEC,
  N_PARAM
} NKind;

typedef struct Node Node;
struct Node {
  NKind    kind;
  TKind    op;           /* operator token */
  long     num;
  char     str[512];     /* FIX(bug3): was 256, now 512 to match str_val */
  int      slen;         /* FIX(bug2): explicit byte count for str literals */
  /* type */
  TKind    tbase;        /* TK_UINT8 … TK_VOID */
  int      ptrs;         /* pointer depth */
  /* children */
  Node    *ch[4];        /* left/right/body/else */
  /* lists */
  Node   **list;         /* params, args, stmts, cases */
  int      nlist;
  int      cap;
};

static Node *alloc_node(NKind k) {
  Node *n = calloc(1, sizeof(Node));
  n->kind = k; return n;
}
static void list_push(Node *n, Node *child) {
  if (n->nlist == n->cap) {
    n->cap = n->cap ? n->cap*2 : 4;
    n->list = realloc(n->list, n->cap * sizeof(Node*));
  }
  n->list[n->nlist++] = child;
}

/* ══════════════════════════════════════════════════════════
   PARSER
   ══════════════════════════════════════════════════════════ */
static void parse_type(TKind *base, int *ptrs) {
  *base = tok.kind; next();
  *ptrs = 0;
  while (tok.kind==TK_STAR) { (*ptrs)++; next(); }
}

static Node *expr(void);

/* forward decls */
static Node *stmt(void);
static Node *block(void);

static Node *primary(void) {
  Node *n;
  if (tok.kind==TK_NUM) {
    n=alloc_node(N_NUM); n->num=tok.num; next(); return n;
  }
  if (tok.kind==TK_STR) {
    n=alloc_node(N_STR);
    /* FIX(bug2): use memcpy + slen instead of strcpy so embedded nulls survive */
    memcpy(n->str, tok.str, tok.slen+1);
    n->slen=tok.slen;
    next(); return n;
  }
  if (tok.kind==TK_ID) {
    n=alloc_node(N_ID); strcpy(n->str, tok.str); next(); return n;
  }
  if (tok.kind==TK_LPAREN) {
    next(); n=expr(); expect(TK_RPAREN); return n;
  }
  die("expected expression");
  return NULL;
}

static Node *postfix(void) {
  Node *e = primary(), *t;
  for(;;) {
    if (tok.kind==TK_LPAREN) {
      next(); t=alloc_node(N_CALL); t->ch[0]=e;
      if (tok.kind!=TK_RPAREN)
	for(;;) {
	  list_push(t, expr());
	  if (!accept(TK_COMMA)) break;
	}
      expect(TK_RPAREN); e=t;
    } else if (tok.kind==TK_LBRACK) {
      next(); t=alloc_node(N_IDX); t->ch[0]=e; t->ch[1]=expr();
      expect(TK_RBRACK); e=t;
    } else if (tok.kind==TK_INC) {
      next(); t=alloc_node(N_POSTINC); t->ch[0]=e; e=t;
    } else if (tok.kind==TK_DEC) {
      next(); t=alloc_node(N_POSTDEC); t->ch[0]=e; e=t;
    } else break;
  }
  return e;
}

static Node *unary(void) {
  Node *t;
  if (tok.kind==TK_MINUS||tok.kind==TK_BANG||tok.kind==TK_TILDE) {
    TKind op=tok.kind; next();
    t=alloc_node(N_UOPN); t->op=op; t->ch[0]=unary(); return t;
  }
  if (tok.kind==TK_AMP) {
    next(); t=alloc_node(N_ADDR); t->ch[0]=unary(); return t;
  }
  if (tok.kind==TK_STAR) {
    next(); t=alloc_node(N_DEREF); t->ch[0]=unary(); return t;
  }
  if (tok.kind==TK_INC) {
    next(); t=alloc_node(N_PREINC); t->ch[0]=unary(); return t;
  }
  if (tok.kind==TK_DEC) {
    next(); t=alloc_node(N_PREDEC); t->ch[0]=unary(); return t;
  }
  /* cast: (type) expr */
  if (tok.kind==TK_LPAREN && (src[src_pos]==' '||1)) {
    int saved=src_pos; Token saved_tok=tok; int saved_line=src_line;
    next();
    if (is_type()) {
      TKind tb; int pt;
      parse_type(&tb,&pt);
      if (tok.kind==TK_RPAREN) {
	next(); t=alloc_node(N_CAST);
	t->tbase=tb; t->ptrs=pt; t->ch[0]=unary(); return t;
      }
    }
    src_pos=saved; tok=saved_tok; src_line=saved_line;
  }
  return postfix();
}

#define BINOP(name, next_fn, ...)					\
  static Node *name(void) {						\
    Node *l=next_fn(), *t; TKind ops[]={__VA_ARGS__, TK_EOF};		\
    for(;;) {								\
      int found=0;							\
      for(int i=0;ops[i]!=TK_EOF;i++) if(tok.kind==ops[i]){found=1;break;} \
      if(!found) break;							\
      TKind op=tok.kind; next();					\
      t=alloc_node(N_BOPN); t->op=op; t->ch[0]=l; t->ch[1]=next_fn(); l=t; \
    } return l;								\
  }

BINOP(mul_expr,  unary,   TK_STAR, TK_SLASH, TK_MOD)
  BINOP(add_expr,  mul_expr, TK_PLUS, TK_MINUS)
  BINOP(shf_expr,  add_expr, TK_SHL, TK_SHR)
  BINOP(cmp_expr,  shf_expr, TK_LT, TK_LEQ, TK_GT, TK_GEQ)
  BINOP(eq_expr,   cmp_expr, TK_EQ, TK_NEQ)
  BINOP(band_expr, eq_expr,  TK_AMP)
  BINOP(bxor_expr, band_expr,TK_CARET)
  BINOP(bor_expr,  bxor_expr,TK_PIPE)

     static Node *land_expr(void) {
  Node *l=bor_expr(), *t;
  while (tok.kind==TK_AND) {
    next(); t=alloc_node(N_BOPN); t->op=TK_AND;
    t->ch[0]=l; t->ch[1]=bor_expr(); l=t;
  } return l;
}
static Node *lor_expr(void) {
  Node *l=land_expr(), *t;
  while (tok.kind==TK_OR) {
    next(); t=alloc_node(N_BOPN); t->op=TK_OR;
    t->ch[0]=l; t->ch[1]=land_expr(); l=t;
  } return l;
}

static Node *ternary(void);
static Node *ternary(void) {
  Node *c=lor_expr(), *t;
  if (tok.kind==TK_QUESTION) {
    next(); t=alloc_node(N_TERN); t->ch[0]=c;
    t->ch[1]=expr(); expect(TK_COLON); t->ch[2]=ternary(); return t;
  }
  return c;
}

static TKind asgn_ops[] = {
  TK_ASSIGN, TK_ADDEQ, TK_SUBEQ, TK_MULEQ, TK_DIVEQ, TK_MODEQ,
  TK_ANDEQ,  TK_OREQ,  TK_XOREQ, TK_SHLEQ, TK_SHREQ, TK_EOF
};
static Node *expr(void) {
  Node *l=ternary(), *t;
  for(int i=0; asgn_ops[i]!=TK_EOF; i++)
    if (tok.kind==asgn_ops[i]) {
      TKind op=tok.kind; next();
      t=alloc_node(N_ASGN); t->op=op; t->ch[0]=l; t->ch[1]=expr();
      return t;
    }
  return l;
}

static Node *decl_local(void) {
  TKind tb; int pt;
  parse_type(&tb,&pt);
  char nm[512]; strcpy(nm, tok.str); expect(TK_ID);
  Node *n;
  if (tok.kind==TK_LBRACK) {
    next(); n=alloc_node(N_LARR);
    n->tbase=tb; n->ptrs=pt; strcpy(n->str,nm);
    n->ch[0]=expr(); expect(TK_RBRACK);
    if (accept(TK_ASSIGN)) {
      expect(TK_LBRACE);
      while (tok.kind!=TK_RBRACE) {
	list_push(n, expr()); accept(TK_COMMA);
      }
      expect(TK_RBRACE);
    }
  } else {
    n=alloc_node(N_LVAR);
    n->tbase=tb; n->ptrs=pt; strcpy(n->str,nm);
    if (accept(TK_ASSIGN)) n->ch[0]=expr();
  }
  expect(TK_SEMI); return n;
}

static Node *stmt(void) {
  Node *n, *t;
  if (is_type()) return decl_local();
  switch(tok.kind) {
  case TK_LBRACE: return block();
  case TK_IF:
    n=alloc_node(N_IF); next();
    expect(TK_LPAREN); n->ch[0]=expr(); expect(TK_RPAREN);
    n->ch[1]=stmt();
    if (tok.kind==TK_ELSE) {
      next(); n->ch[2]=stmt();
    }
    return n;
  case TK_WHILE:
    n=alloc_node(N_WHILE); next();
    expect(TK_LPAREN); n->ch[0]=expr(); expect(TK_RPAREN);
    n->ch[1]=stmt(); return n;
  case TK_FOR:
    n=alloc_node(N_FOR); next(); expect(TK_LPAREN);
    if (is_type()) n->ch[0]=decl_local();   /* consumes ; */
    else if (tok.kind!=TK_SEMI) { Node *es=alloc_node(N_ES); es->ch[0]=expr(); expect(TK_SEMI); n->ch[0]=es; }
    else { next(); }
    if (tok.kind!=TK_SEMI) n->ch[1]=expr();
    expect(TK_SEMI);
    if (tok.kind!=TK_RPAREN) n->ch[2]=expr();
    expect(TK_RPAREN); n->ch[3]=stmt(); return n;
  case TK_SWITCH:
    n=alloc_node(N_SW); next();
    expect(TK_LPAREN); n->ch[0]=expr(); expect(TK_RPAREN);
    expect(TK_LBRACE);
    while (tok.kind!=TK_RBRACE) {
      if (tok.kind==TK_CASE) {
	next(); t=alloc_node(N_CASE); t->ch[0]=expr(); expect(TK_COLON);
	while(tok.kind!=TK_CASE&&tok.kind!=TK_DEFAULT&&tok.kind!=TK_RBRACE)
	  list_push(t, stmt());
	list_push(n, t);
      } else if (tok.kind==TK_DEFAULT) {
	next(); expect(TK_COLON);
	t=alloc_node(N_DEF);
	while(tok.kind!=TK_CASE&&tok.kind!=TK_DEFAULT&&tok.kind!=TK_RBRACE)
	  list_push(t, stmt());
	list_push(n, t);
      } else break;
    }
    expect(TK_RBRACE); return n;
  case TK_RETURN:
    n=alloc_node(N_RET); next();
    if (tok.kind!=TK_SEMI) n->ch[0]=expr();
    expect(TK_SEMI); return n;
  case TK_BREAK:
    next(); expect(TK_SEMI); return alloc_node(N_BRK);
  case TK_CONTINUE:
    next(); expect(TK_SEMI); return alloc_node(N_CONT);
  default:
    n=alloc_node(N_ES); n->ch[0]=expr(); expect(TK_SEMI); return n;
  }
}

static Node *block(void) {
  Node *n=alloc_node(N_BLK); expect(TK_LBRACE);
  while (tok.kind!=TK_RBRACE) list_push(n, stmt());
  expect(TK_RBRACE); return n;
}

static Node *parse_prog(void) {
  Node *prog=alloc_node(N_PROG);
  next();
  while (tok.kind!=TK_EOF) {
    TKind tb; int pt;
    parse_type(&tb,&pt);
    char nm[512]; strcpy(nm,tok.str); expect(TK_ID);
    Node *d;
    if (tok.kind==TK_LPAREN) {
      next(); d=alloc_node(N_FUNC);
      d->tbase=tb; d->ptrs=pt; strcpy(d->str,nm);
      /* params */
      if (tok.kind!=TK_RPAREN)
	for(;;) {
	  if (tok.kind==TK_VOID) { next(); break; } /* accept (void) as empty param list */
	  TKind ptb; int ppt;
	  parse_type(&ptb,&ppt);
	  Node *p=alloc_node(N_PARAM);
	  p->tbase=ptb; p->ptrs=ppt; strcpy(p->str,tok.str);
	  expect(TK_ID); list_push(d, p);
	  if (!accept(TK_COMMA)) break;
	}
      expect(TK_RPAREN);
      if (tok.kind==TK_SEMI) { next(); d->kind=N_FDECL; }
      else d->ch[0]=block();
    } else if (tok.kind==TK_LBRACK) {
      /* global array:  type name[size];
	 or  type name[size] = { v, … }; */
      next();
      d=alloc_node(N_GARR);
      d->tbase=tb; d->ptrs=pt; strcpy(d->str,nm);
      d->ch[0]=expr();          /* size expression */
      expect(TK_RBRACK);
      if (accept(TK_ASSIGN)) {
	expect(TK_LBRACE);
	while (tok.kind!=TK_RBRACE) {
	  list_push(d, expr()); accept(TK_COMMA);
	}
	expect(TK_RBRACE);
      }
      expect(TK_SEMI);
    } else {
      d=alloc_node(N_GVAR);
      d->tbase=tb; d->ptrs=pt; strcpy(d->str,nm);
      if (accept(TK_ASSIGN)) d->ch[0]=expr();
      expect(TK_SEMI);
    }
    list_push(prog, d);
  }
  return prog;
}

/* ══════════════════════════════════════════════════════════
   CODE GENERATOR  (NASM x86_32, cdecl)
   ══════════════════════════════════════════════════════════ */

/* String literal pool */
#define MAX_STRS 512
static char str_val[MAX_STRS][512];
static int  str_val_len[MAX_STRS];  /* FIX(bug2): track explicit byte lengths */
static int  str_cnt = 0;

/* FIX(bug2): length-aware string interning using memcmp instead of strcmp */
static int intern_str_n(const char *s, int len) {
  for (int i=0; i<str_cnt; i++)
    if (str_val_len[i]==len && memcmp(str_val[i], s, len)==0) return i;
  if (str_cnt==MAX_STRS) die("too many strings");
  memcpy(str_val[str_cnt], s, len+1);
  str_val_len[str_cnt]=len;
  return str_cnt++;
}

/* ══════════════════════════════════════════════════════════
   SYMBOL TABLES  (with type info for pointer arithmetic)
   ══════════════════════════════════════════════════════════ */

/* Local variable table */
#define MAX_LOCALS 256
static struct {
  char  name[64];
  int   off;
  TKind tbase;
  int   ptrs;
  int   is_array; /* 1 = array on stack (name decays to address via lea) */
} locals[MAX_LOCALS];
static int nlocals=0, frame_size=0;

static int find_local(const char *nm) {
  for(int i=0;i<nlocals;i++) if(!strcmp(locals[i].name,nm)) return locals[i].off;
  return 0x7fffffff; /* not found */
}
static int local_is_array(const char *nm) {
  for(int i=0;i<nlocals;i++) if(!strcmp(locals[i].name,nm)) return locals[i].is_array;
  return 0;
}
static void def_local(const char *nm, int off, TKind tbase, int ptrs) {
  if (nlocals==MAX_LOCALS) die("too many locals");
  strncpy(locals[nlocals].name, nm, 63);
  locals[nlocals].off      = off;
  locals[nlocals].tbase    = tbase;
  locals[nlocals].ptrs     = ptrs;
  locals[nlocals].is_array = 0;
  nlocals++;
}
static void def_local_array(const char *nm, int off, TKind tbase, int ptrs) {
  def_local(nm, off, tbase, ptrs);
  locals[nlocals-1].is_array = 1;
}

/* Global variable table */
#define MAX_GLOBALS 256
static struct {
  char  name[64];
  TKind tbase;
  int   ptrs;
  int   is_array;
} gvars[MAX_GLOBALS];
static int ngvars=0;

static int global_is_array(const char *nm) {
  for(int i=0;i<ngvars;i++) if(!strcmp(gvars[i].name,nm)) return gvars[i].is_array;
  return 0;
}
static void def_global(const char *nm, TKind tbase, int ptrs) {
  if (ngvars==MAX_GLOBALS) die("too many globals");
  strncpy(gvars[ngvars].name, nm, 63);
  gvars[ngvars].tbase    = tbase;
  gvars[ngvars].ptrs     = ptrs;
  gvars[ngvars].is_array = 0;
  ngvars++;
}
static void def_global_array(const char *nm, TKind tbase, int ptrs) {
  def_global(nm, tbase, ptrs);
  gvars[ngvars-1].is_array = 1;
}

/* Label counter */
static int lbl_cnt=0;
static int new_lbl(void) { return ++lbl_cnt; }

/* break/continue/return label stacks */
static int brk_stk[64], cont_stk[64], stk_top=0;
static int ret_lbl=0;

/* Output helpers */
static FILE *out;
#define E(...)  fprintf(out, __VA_ARGS__)
#define EL(...) do { fprintf(out,"  "); fprintf(out,__VA_ARGS__); } while(0)

static void emit_ref(const char *nm) {
  int off = find_local(nm);
  if (off == 0x7fffffff) E("[%s]", nm);
  else if (off > 0) E("[ebp+%d]", off);
  else E("[ebp%d]", off);
}

/* Emit address of a local/global (for array decay: lea not mov) */
static void emit_addr(const char *nm) {
  int off = find_local(nm);
  if (off == 0x7fffffff) { E("  mov eax, %s\n", nm); return; } /* global: label IS address */
  if (off > 0) EL("lea eax, [ebp+%d]\n", off);
  else         EL("lea eax, [ebp%d]\n", off);
}

static void load_nm(const char *nm) {
  /* Arrays decay to a pointer: yield address, not the value stored there */
  if (local_is_array(nm)) { emit_addr(nm); return; }
  if (global_is_array(nm)) { E("  mov eax, %s\n", nm); return; }
  E("  mov eax, "); emit_ref(nm); E("\n");
}

/* ══════════════════════════════════════════════════════════
   TYPE SYSTEM  (for pointer arithmetic)
   ══════════════════════════════════════════════════════════ */
typedef struct { TKind tbase; int ptrs; } Type;
static const Type T_INT = { TK_UINT32, 0 };

/* Size in bytes of a base type (non-pointer) */
static int base_size(TKind tbase) {
  switch (tbase) {
  case TK_UINT8:  case TK_INT8:  return 1;
  case TK_UINT16: case TK_INT16: return 2;
  case TK_UINT64: case TK_INT64: return 8;
  default:        return 4;  /* uint32, int32, void, pointer */
  }
}

/* Stride for pointer arithmetic: the size of what the pointer points at.
   pointer-to-pointer always has stride 4 (one address). */
static int pointee_size(TKind tbase, int ptrs) {
  if (ptrs > 1) return 4;
  return base_size(tbase);
}

/* forward */
static void gen_expr(Node *n);
static void gen_stmt(Node *n);
static Type fun_ret_type(const char *nm); /* defined in codegen section */

/* Infer the type of an expression node.
   Walks the AST without emitting any code. */
static Type get_type(Node *n) {
  if (!n) return T_INT;
  switch (n->kind) {
  case N_NUM:
    return T_INT;
  case N_STR:
    return (Type){ TK_UINT8, 1 };  /* char* */
  case N_ID: {
    /* Check locals first, then globals */
    for (int i=0; i<nlocals; i++)
      if (!strcmp(locals[i].name, n->str))
	return (Type){ locals[i].tbase, locals[i].ptrs };
    for (int i=0; i<ngvars; i++)
      if (!strcmp(gvars[i].name, n->str))
	return (Type){ gvars[i].tbase, gvars[i].ptrs };
    return T_INT;
  }
  case N_ADDR: {
    Type inner = get_type(n->ch[0]);
    return (Type){ inner.tbase, inner.ptrs + 1 };
  }
  case N_DEREF: {
    Type inner = get_type(n->ch[0]);
    if (inner.ptrs > 0) return (Type){ inner.tbase, inner.ptrs - 1 };
    return T_INT;
  }
  case N_IDX: {
    Type arr = get_type(n->ch[0]);
    if (arr.ptrs > 0) return (Type){ arr.tbase, arr.ptrs - 1 };
    return T_INT;
  }
  case N_CAST:
    return (Type){ n->tbase, n->ptrs };
  case N_CALL:
    if (n->ch[0] && n->ch[0]->kind == N_ID)
      return fun_ret_type(n->ch[0]->str);
    return T_INT;
  case N_BOPN:
  case N_ASGN: {
    /* Arithmetic preserves a pointer type if either operand is a pointer */
    Type l = get_type(n->ch[0]);
    Type r = get_type(n->ch[1]);
    if (l.ptrs > 0) return l;
    if (r.ptrs > 0) return r;
    return T_INT;
  }
  case N_POSTINC: case N_POSTDEC:
  case N_PREINC:  case N_PREDEC:
    return get_type(n->ch[0]);
  case N_UOPN:
    /* Negation/bitwise-not propagate the child type; the result is signed */
    if (n->op == TK_MINUS || n->op == TK_TILDE)
      return (Type){ TK_INT32, 0 };
    return T_INT;
  default:
    return T_INT;
  }
}

/* Emit an integer scale instruction sequence.
   On entry eax holds the integer to scale; on exit eax = eax * scale. */
static void emit_scale(int scale) {
  if      (scale == 1) { /* nothing */ }
  else if (scale == 2) EL("shl eax, 1\n");
  else if (scale == 4) EL("shl eax, 2\n");
  else if (scale == 8) EL("shl eax, 3\n");
  else                 EL("imul eax, %d\n", scale);
}

/* ══════════════════════════════════════════════════════════
   LVALUE / STORE HELPERS
   ══════════════════════════════════════════════════════════ */
static void gen_lval_addr(Node *n) {
  if (n->kind==N_ID) {
    int off=find_local(n->str);
    if (off==0x7fffffff) EL("mov eax, %s\n", n->str);
    else if (off>0) EL("lea eax, [ebp+%d]\n", off);
    else EL("lea eax, [ebp%d]\n", off);
  } else if (n->kind==N_IDX) {
    /* addr of arr[ix]: base + ix * element_size.
       FIX: use gen_expr on the base, not gen_lval_addr.
       gen_lval_addr on a pointer N_ID yields the stack slot address (lea),
       but we need the pointer *value* (mov).  gen_expr already handles both:
       array identifiers decay to their base address (lea), pointer identifiers
       load their value (mov). */
    Type arr = get_type(n->ch[0]);
    int scale = pointee_size(arr.tbase, arr.ptrs);
    gen_expr(n->ch[0]); EL("push eax\n");
    gen_expr(n->ch[1]);
    emit_scale(scale);
    EL("pop ecx\n"); EL("add eax, ecx\n");
  } else if (n->kind==N_DEREF) {
    gen_expr(n->ch[0]);
  }
}

static void store_lval(Node *n) {
  if (n->kind==N_ID) {
    E("  mov "); emit_ref(n->str); E(", eax\n");
  } else if (n->kind==N_IDX || n->kind==N_DEREF) {
    EL("push eax\n");
    gen_lval_addr(n);
    EL("pop ecx\n");
    /* Store only as many bytes as the element type requires */
    Type t = get_type(n);
    int sz = (t.ptrs > 0) ? 4 : base_size(t.tbase);
    switch (sz) {
    case 1:  EL("mov byte [eax], cl\n");   break;
    case 2:  EL("mov word [eax], cx\n");   break;
    default: EL("mov dword [eax], ecx\n"); break;
    }
  }
}

/* ══════════════════════════════════════════════════════════
   ARITHMETIC HELPERS
   ══════════════════════════════════════════════════════════ */
static void arith(TKind op, int sgn) {
  /* ecx=left, eax=right → eax=result; sgn=1 for signed operands */
  switch(op) {
  case TK_PLUS:  EL("add eax, ecx\n"); break;
  case TK_MINUS: EL("sub ecx, eax\n"); EL("mov eax, ecx\n"); break;
  case TK_STAR:  EL("imul eax, ecx\n"); break;
  case TK_SLASH:
    EL("xchg eax, ecx\n");
    if (sgn) { EL("cdq\n"); EL("idiv ecx\n"); }
    else     { EL("xor edx, edx\n"); EL("div ecx\n"); }
    break;
  case TK_MOD:
    EL("xchg eax, ecx\n");
    if (sgn) { EL("cdq\n"); EL("idiv ecx\n"); }
    else     { EL("xor edx, edx\n"); EL("div ecx\n"); }
    EL("mov eax, edx\n");
    break;
  case TK_AMP:   EL("and eax, ecx\n"); break;
  case TK_PIPE:  EL("or  eax, ecx\n"); break;
  case TK_CARET: EL("xor eax, ecx\n"); break;
  case TK_SHL:   EL("xchg eax, ecx\n"); EL("shl eax, cl\n"); break;
  case TK_SHR:
    EL("xchg eax, ecx\n");
    EL(sgn ? "sar eax, cl\n" : "shr eax, cl\n");
    break;
  default: break;
  }
}

/* Pointer-aware add/subtract.
   On entry: ecx = left operand, eax = right operand.
   On exit:  eax = result. */
static void ptr_arith(TKind op, Type lt, Type rt) {
  if (op == TK_PLUS) {
    if (lt.ptrs > 0) {
      /* ptr + int: scale the integer (eax) by pointee size */
      int scale = pointee_size(lt.tbase, lt.ptrs);
      emit_scale(scale);
      EL("add eax, ecx\n");
    } else if (rt.ptrs > 0) {
      /* int + ptr: scale the integer (ecx) by pointee size */
      int scale = pointee_size(rt.tbase, rt.ptrs);
      if      (scale == 1) { /* nothing */ }
      else if (scale == 2) EL("shl ecx, 1\n");
      else if (scale == 4) EL("shl ecx, 2\n");
      else if (scale == 8) EL("shl ecx, 3\n");
      else                 EL("imul ecx, %d\n", scale);
      EL("add eax, ecx\n");
    } else {
      EL("add eax, ecx\n");
    }
  } else if (op == TK_MINUS) {
    if (lt.ptrs > 0 && rt.ptrs == 0) {
      /* ptr - int: scale the integer (eax) by pointee size */
      int scale = pointee_size(lt.tbase, lt.ptrs);
      emit_scale(scale);
      EL("sub ecx, eax\n"); EL("mov eax, ecx\n");
    } else if (lt.ptrs > 0 && rt.ptrs > 0) {
      /* ptr - ptr: raw byte difference divided by pointee size */
      EL("sub ecx, eax\n"); EL("mov eax, ecx\n");
      int scale = pointee_size(lt.tbase, lt.ptrs);
      if (scale > 1) {
	EL("xor edx, edx\n");
	EL("mov ecx, %d\n", scale);
	EL("div ecx\n");
      }
    } else {
      EL("sub ecx, eax\n"); EL("mov eax, ecx\n");
    }
  }
}

/* ══════════════════════════════════════════════════════════
   EXPRESSION CODE GENERATOR
   ══════════════════════════════════════════════════════════ */
static void gen_expr(Node *n) {
  int la, lb;
  switch(n->kind) {
  case N_NUM:
    EL("mov eax, %ld\n", n->num); break;
  case N_STR: {
    /* FIX(bug2): use length-aware intern */
    int id=intern_str_n(n->str, n->slen);
    EL("mov eax, _s%d\n", id); break; }
  case N_ID:
    load_nm(n->str); break;
  case N_ADDR:
    gen_lval_addr(n->ch[0]); break;

  case N_DEREF: {
    /* Load from pointer; respect the pointee width and signedness */
    gen_expr(n->ch[0]);
    Type inner = get_type(n->ch[0]);
    int psz = (inner.ptrs > 1) ? 4 : base_size(inner.tbase);
    int sgn = (inner.ptrs == 1) && is_signed(inner.tbase);
    switch (psz) {
    case 1: EL(sgn ? "movsx eax, byte [eax]\n" : "movzx eax, byte [eax]\n"); break;
    case 2: EL(sgn ? "movsx eax, word [eax]\n" : "movzx eax, word [eax]\n"); break;
    case 8: EL("mov eax, [eax]\n"); break; /* truncate 64→32; full 64-bit NYI */
    default: EL("mov eax, [eax]\n"); break;
    }
    break;
  }

  case N_CAST:
    gen_expr(n->ch[0]);
    if (n->ptrs == 0) {
      switch (n->tbase) {
      case TK_INT8:   EL("movsx eax, al\n");      break; /* sign-extend low byte */
      case TK_INT16:  EL("movsx eax, ax\n");      break; /* sign-extend low word */
      case TK_UINT8:  EL("and eax, 0xFF\n");      break;
      case TK_UINT16: EL("and eax, 0xFFFF\n");    break;
      default: break; /* int32/uint32/int64/uint64: no truncation needed */
      }
    }
    break;

  case N_POSTINC: {
    Type t = get_type(n->ch[0]);
    int stride = (t.ptrs > 0) ? pointee_size(t.tbase, t.ptrs) : 1;
    gen_expr(n->ch[0]); EL("push eax\n");
    EL("add eax, %d\n", stride); store_lval(n->ch[0]); EL("pop eax\n");
    break;
  }
  case N_POSTDEC: {
    Type t = get_type(n->ch[0]);
    int stride = (t.ptrs > 0) ? pointee_size(t.tbase, t.ptrs) : 1;
    gen_expr(n->ch[0]); EL("push eax\n");
    EL("sub eax, %d\n", stride); store_lval(n->ch[0]); EL("pop eax\n");
    break;
  }
  case N_PREINC: {
    Type t = get_type(n->ch[0]);
    int stride = (t.ptrs > 0) ? pointee_size(t.tbase, t.ptrs) : 1;
    gen_expr(n->ch[0]); EL("add eax, %d\n", stride); store_lval(n->ch[0]);
    break;
  }
  case N_PREDEC: {
    Type t = get_type(n->ch[0]);
    int stride = (t.ptrs > 0) ? pointee_size(t.tbase, t.ptrs) : 1;
    gen_expr(n->ch[0]); EL("sub eax, %d\n", stride); store_lval(n->ch[0]);
    break;
  }

  case N_UOPN:
    gen_expr(n->ch[0]);
    if (n->op==TK_MINUS) EL("neg eax\n");
    else if (n->op==TK_TILDE) EL("not eax\n");
    else { EL("test eax, eax\n"); EL("setz al\n"); EL("movzx eax, al\n"); }
    break;

  case N_BOPN:
    if (n->op==TK_OR) {
      la=new_lbl(); lb=new_lbl();
      gen_expr(n->ch[0]); EL("test eax, eax\n"); EL("jnz .L%d\n",la);
      gen_expr(n->ch[1]); EL("test eax, eax\n"); EL("jnz .L%d\n",la);
      EL("xor eax, eax\n"); EL("jmp .L%d\n",lb);
      E(".L%d:\n",la); EL("mov eax, 1\n"); E(".L%d:\n",lb);
    } else if (n->op==TK_AND) {
      la=new_lbl(); lb=new_lbl();
      gen_expr(n->ch[0]); EL("test eax, eax\n"); EL("jz .L%d\n",la);
      gen_expr(n->ch[1]); EL("test eax, eax\n"); EL("jz .L%d\n",la);
      EL("mov eax, 1\n"); EL("jmp .L%d\n",lb);
      E(".L%d:\n",la); EL("xor eax, eax\n"); E(".L%d:\n",lb);
    } else {
      /* Evaluate both sides; ecx=left, eax=right */
      gen_expr(n->ch[0]); EL("push eax\n");
      gen_expr(n->ch[1]); EL("pop ecx\n");
      {
	int sgn = is_signed(get_type(n->ch[0]).tbase) ||
	  is_signed(get_type(n->ch[1]).tbase);
	switch(n->op) {
	case TK_EQ:  EL("cmp ecx, eax\n"); EL("sete al\n");                          EL("movzx eax, al\n"); break;
	case TK_NEQ: EL("cmp ecx, eax\n"); EL("setne al\n");                         EL("movzx eax, al\n"); break;
	case TK_LT:  EL("cmp ecx, eax\n"); EL(sgn?"setl al\n" :"setb al\n");         EL("movzx eax, al\n"); break;
	case TK_LEQ: EL("cmp ecx, eax\n"); EL(sgn?"setle al\n":"setbe al\n");        EL("movzx eax, al\n"); break;
	case TK_GT:  EL("cmp ecx, eax\n"); EL(sgn?"setg al\n" :"seta al\n");         EL("movzx eax, al\n"); break;
	case TK_GEQ: EL("cmp ecx, eax\n"); EL(sgn?"setge al\n":"setae al\n");        EL("movzx eax, al\n"); break;
	case TK_PLUS:
	case TK_MINUS: {
	  Type lt = get_type(n->ch[0]);
	  Type rt = get_type(n->ch[1]);
	  if (lt.ptrs > 0 || rt.ptrs > 0)
	    ptr_arith(n->op, lt, rt);
	  else
	    arith(n->op, sgn);
	  break;
	}
	default: arith(n->op, sgn); break;
	}
      }
    }
    break;

  case N_ASGN:
    if (n->op==TK_ASSIGN) {
      gen_expr(n->ch[1]); store_lval(n->ch[0]);
    } else {
      /* Compound assignment */
      TKind base;
      switch(n->op){
      case TK_ADDEQ: base=TK_PLUS;  break; case TK_SUBEQ: base=TK_MINUS; break;
      case TK_MULEQ: base=TK_STAR;  break; case TK_DIVEQ: base=TK_SLASH; break;
      case TK_MODEQ: base=TK_MOD;   break; case TK_ANDEQ: base=TK_AMP;   break;
      case TK_OREQ:  base=TK_PIPE;  break; case TK_XOREQ: base=TK_CARET; break;
      case TK_SHLEQ: base=TK_SHL;   break; case TK_SHREQ: base=TK_SHR;   break;
      default: base=TK_EOF;
      }
      gen_expr(n->ch[0]); EL("push eax\n");
      gen_expr(n->ch[1]); EL("pop ecx\n");
      {
	int sgn = is_signed(get_type(n->ch[0]).tbase) ||
	  is_signed(get_type(n->ch[1]).tbase);
	if (base==TK_PLUS || base==TK_MINUS) {
	  Type lt = get_type(n->ch[0]);
	  Type rt = get_type(n->ch[1]);
	  if (lt.ptrs > 0 || rt.ptrs > 0)
	    ptr_arith(base, lt, rt);
	  else
	    arith(base, sgn);
	} else {
	  arith(base, sgn);
	}
      }
      store_lval(n->ch[0]);
    }
    break;

  case N_TERN:
    la=new_lbl(); lb=new_lbl();
    gen_expr(n->ch[0]); EL("test eax, eax\n"); EL("jz .L%d\n",la);
    gen_expr(n->ch[1]); EL("jmp .L%d\n",lb);
    E(".L%d:\n",la); gen_expr(n->ch[2]); E(".L%d:\n",lb);
    break;

  case N_CALL: {
    int argc=n->nlist;
    for (int i=argc-1;i>=0;i--) { gen_expr(n->list[i]); EL("push eax\n"); }
    if (n->ch[0]->kind==N_ID) EL("call %s\n", n->ch[0]->str);
    else { gen_expr(n->ch[0]); EL("call eax\n"); }
    if (argc) EL("add esp, %d\n", argc*4);
    break; }

  case N_IDX: {
    /* arr[i] — address then dereference with correct width and signedness */
    Type arr = get_type(n->ch[0]);
    int psz = (arr.ptrs > 1) ? 4 : base_size(arr.tbase);
    int sgn = (arr.ptrs == 1) && is_signed(arr.tbase);
    gen_lval_addr(n);
    switch (psz) {
    case 1: EL(sgn ? "movsx eax, byte [eax]\n" : "movzx eax, byte [eax]\n"); break;
    case 2: EL(sgn ? "movsx eax, word [eax]\n" : "movzx eax, word [eax]\n"); break;
    default: EL("mov eax, [eax]\n"); break;
    }
    break;
  }

  default: break;
  }
}

/* ══════════════════════════════════════════════════════════
   STATEMENT CODE GENERATOR
   ══════════════════════════════════════════════════════════ */
static void gen_stmt(Node *n) {
  int la, lb, lc;
  switch(n->kind) {
  case N_BLK:
    for(int i=0;i<n->nlist;i++) gen_stmt(n->list[i]); break;
  case N_ES:
    gen_expr(n->ch[0]); break;
  case N_LVAR:
    if (n->ch[0]) { gen_expr(n->ch[0]); E("  mov "); emit_ref(n->str); E(", eax\n"); }
    break;
  case N_LARR:
    if (n->nlist) {
      int base=find_local(n->str);
      /* element size for initialiser stride */
      int esz = (n->ptrs > 0) ? 4 : base_size(n->tbase);
      for(int i=0;i<n->nlist;i++) {
	gen_expr(n->list[i]);
	int off=base+i*esz;
	switch(esz) {
	case 1:
	  if(off>0) EL("mov byte [ebp+%d], al\n",off);
	  else      EL("mov byte [ebp%d], al\n",off);
	  break;
	case 2:
	  if(off>0) EL("mov word [ebp+%d], ax\n",off);
	  else      EL("mov word [ebp%d], ax\n",off);
	  break;
	default:
	  if(off>0) EL("mov dword [ebp+%d], eax\n",off);
	  else      EL("mov dword [ebp%d], eax\n",off);
	  break;
	}
      }
    }
    break;
  case N_RET:
    if (n->ch[0]) gen_expr(n->ch[0]);
    EL("jmp .Lret%d\n", ret_lbl); break;
  case N_BRK:  EL("jmp .Lbrk%d\n",  brk_stk[stk_top-1]);  break;
  case N_CONT: EL("jmp .Lcont%d\n", cont_stk[stk_top-1]); break;
  case N_IF:
    la=new_lbl(); lb=new_lbl();
    gen_expr(n->ch[0]); EL("test eax, eax\n"); EL("jz .L%d\n",la);
    gen_stmt(n->ch[1]); EL("jmp .L%d\n",lb);
    E(".L%d:\n",la);
    if (n->ch[2]) gen_stmt(n->ch[2]);
    E(".L%d:\n",lb); break;
  case N_WHILE:
    la=new_lbl(); lb=new_lbl();
    brk_stk[stk_top]=lb; cont_stk[stk_top]=la; stk_top++;
    E(".Lcont%d:\n",la);
    gen_expr(n->ch[0]); EL("test eax, eax\n"); EL("jz .Lbrk%d\n",lb);
    gen_stmt(n->ch[1]); EL("jmp .Lcont%d\n",la);
    E(".Lbrk%d:\n",lb); stk_top--; break;
  case N_FOR:
    la=new_lbl(); lb=new_lbl(); lc=new_lbl();
    brk_stk[stk_top]=lb; cont_stk[stk_top]=lc; stk_top++;
    if (n->ch[0]) gen_stmt(n->ch[0]);
    E(".L%d:\n",la);
    if (n->ch[1]) { gen_expr(n->ch[1]); EL("test eax, eax\n"); EL("jz .Lbrk%d\n",lb); }
    gen_stmt(n->ch[3]);
    E(".Lcont%d:\n",lc);
    if (n->ch[2]) gen_expr(n->ch[2]);
    EL("jmp .L%d\n",la); E(".Lbrk%d:\n",lb); stk_top--; break;
  case N_SW: {
    lb=new_lbl();
    int ncases=n->nlist;
    int *clbls=malloc(ncases*sizeof(int));
    for(int i=0;i<ncases;i++) clbls[i]=new_lbl();
    brk_stk[stk_top]=lb; stk_top++;
    gen_expr(n->ch[0]); EL("push eax\n");
    for(int i=0;i<ncases;i++) {
      Node *c=n->list[i];
      if (c->kind==N_CASE) {
	EL("mov eax, [esp]\n");
	EL("cmp eax, %ld\n", c->ch[0]->num);
	EL("je .L%d\n", clbls[i]);
      } else {
	EL("jmp .L%d\n", clbls[i]);
      }
    }
    EL("jmp .Lbrk%d\n",lb);
    for(int i=0;i<ncases;i++) {
      E(".L%d:\n",clbls[i]);
      Node *c=n->list[i];
      int ns=(c->kind==N_CASE||c->kind==N_DEF)?c->nlist:0;
      for(int j=0;j<ns;j++) gen_stmt(c->list[j]);
    }
    E(".Lbrk%d:\n",lb); EL("add esp, 4\n");
    stk_top--; free(clbls); break; }
  default: break;
  }
}

/* ══════════════════════════════════════════════════════════
   LOCAL VARIABLE PRE-SCAN
   ══════════════════════════════════════════════════════════ */
static void scan_locals(Node *n) {
  if (!n) return;
  if (n->kind==N_LVAR) {
    frame_size+=4;
    def_local(n->str, -frame_size, n->tbase, n->ptrs);
  } else if (n->kind==N_LARR) {
    long cnt = n->ch[0]->num;
    /* size per element */
    int esz = (n->ptrs > 0) ? 4 : base_size(n->tbase);
    frame_size += esz * (int)cnt;
    /* Array name decays to a pointer to its element type */
    def_local_array(n->str, -frame_size, n->tbase, n->ptrs + 1);
  } else {
    for(int i=0;i<4;i++) scan_locals(n->ch[i]);
    for(int i=0;i<n->nlist;i++) scan_locals(n->list[i]);
  }
}

/* ══════════════════════════════════════════════════════════
   FUNCTION GENERATOR
   ══════════════════════════════════════════════════════════ */
static char called[256][64];
static int  ncalled=0;
static void collect_calls(Node *n) {
  if (!n) return;
  if (n->kind==N_CALL && n->ch[0]->kind==N_ID) {
    char *nm=n->ch[0]->str;
    int found=0;
    for(int i=0;i<ncalled;i++) if(!strcmp(called[i],nm)){found=1;break;}
    if(!found && ncalled<256) strcpy(called[ncalled++],nm);
  }
  for(int i=0;i<4;i++) collect_calls(n->ch[i]);
  for(int i=0;i<n->nlist;i++) collect_calls(n->list[i]);
}

static void gen_func(Node *fn) {
  nlocals=0; frame_size=0;
  scan_locals(fn->ch[0]);
  int fsize=(frame_size+15)&~15;

  ret_lbl=new_lbl();
  E("\n%s:\n", fn->str);
  EL("push ebp\n"); EL("mov ebp, esp\n");
  if (fsize) EL("sub esp, %d\n", fsize);

  /* Bind params: [ebp+8], [ebp+12], … with their declared types */
  int poff=8;
  for(int i=0;i<fn->nlist;i++) {
    def_local(fn->list[i]->str, poff,
	      fn->list[i]->tbase, fn->list[i]->ptrs);
    poff+=4;
  }

  gen_stmt(fn->ch[0]);

  E(".Lret%d:\n", ret_lbl);
  EL("mov esp, ebp\n"); EL("pop ebp\n"); EL("ret\n");
}

/* ══════════════════════════════════════════════════════════
   STRING DATA EMITTER
   ══════════════════════════════════════════════════════════ */
/* FIX(bug2): takes explicit length; walks to p<=end to include null terminator */
static void emit_str_data(const char *s, int len) {
  E("db ");
  int first=1;
  const char *end = s + len;  /* points at the null terminator */
  for(const char *p=s; p<=end; p++) {
    unsigned char c=(unsigned char)*p;
    if (c>=32 && c<127 && c!='"' && c!='\\') {
      if (!first) E(",");
      E("\"");
      while (p<=end && (unsigned char)*p>=32 && (unsigned char)*p<127
	     && *p!='"' && *p!='\\') { fputc(*p,out); p++; }
      p--;
      E("\"");
    } else {
      if (!first) E(",");
      E("%d", c);
    }
    first=0;
  }
  E("\n");
}

/* ══════════════════════════════════════════════════════════
   TOP-LEVEL CODE GENERATOR
   ══════════════════════════════════════════════════════════ */

/* Function return-type table — populated before codegen so get_type(N_CALL) works */
#define MAX_FUNS 256
static struct { char name[64]; TKind tbase; int ptrs; } funs[MAX_FUNS];
static int nfuns=0;
static void def_fun(const char *nm, TKind tbase, int ptrs) {
  if (nfuns==MAX_FUNS) return;
  strncpy(funs[nfuns].name, nm, 63);
  funs[nfuns].tbase = tbase;
  funs[nfuns].ptrs  = ptrs;
  nfuns++;
}
static Type fun_ret_type(const char *nm) {
  for (int i=0;i<nfuns;i++)
    if (!strcmp(funs[i].name, nm)) return (Type){funs[i].tbase, funs[i].ptrs};
  return T_INT;
}

static void codegen(Node *prog) {
  /* Register all global variable types before codegen so get_type() works */
  for(int i=0;i<prog->nlist;i++) {
    Node *d=prog->list[i];
    if(d->kind==N_GVAR)
      def_global(d->str, d->tbase, d->ptrs);
    else if(d->kind==N_GARR)
      def_global_array(d->str, d->tbase, d->ptrs + 1); /* array decays to pointer */
    else if(d->kind==N_FUNC || d->kind==N_FDECL)
      def_fun(d->str, d->tbase, d->ptrs); /* register return type */
  }

  /* Collect defined function names */
  char defined[256][64]; int ndef=0;
  for(int i=0;i<prog->nlist;i++)
    if(prog->list[i]->kind==N_FUNC)
      strcpy(defined[ndef++], prog->list[i]->str);

  collect_calls(prog);

  E("BITS 32\n");
  E("section .text\n");
  for(int i=0;i<ncalled;i++) {
    int found=0;
    for(int j=0;j<ndef;j++) if(!strcmp(called[i],defined[j])){found=1;break;}
    if(!found) E("extern %s\n", called[i]);
  }
  for(int i=0;i<ndef;i++) E("global %s\n", defined[i]);

  for(int i=0;i<prog->nlist;i++)
    if(prog->list[i]->kind==N_FUNC)
      gen_func(prog->list[i]);

  /* ── .data section: string literals + explicitly initialised globals ── */
  int has_data = (str_cnt > 0);
  for(int i=0;i<prog->nlist;i++) {
    Node *d=prog->list[i];
    /* FIX(bug1): guard with kind==N_NUM before checking num, so N_STR/N_ID
       initializers don't accidentally satisfy num==0 and leak into .bss too */
    if (d->kind==N_GVAR && d->ch[0] != NULL &&
	!(d->ch[0]->kind==N_NUM && d->ch[0]->num==0)) has_data=1;
    if (d->kind==N_GARR && d->nlist > 0) has_data=1;
  }
  if (has_data) {
    E("\nsection .data\n");
    for(int i=0;i<prog->nlist;i++) {
      Node *d=prog->list[i];
      /* scalar global with non-zero initialiser */
      if(d->kind==N_GVAR && d->ch[0] != NULL &&
	 !(d->ch[0]->kind==N_NUM && d->ch[0]->num==0)) {
	const char *dw = d->ptrs ? "dd" :
	  (base_size(d->tbase)==1?"db":base_size(d->tbase)==2?"dw":
	   base_size(d->tbase)==8?"dq":"dd");
	if (d->ch[0]->kind == N_NUM) {
	  long v = d->ch[0]->num;
	  E("%s: %s %ld\n", d->str, dw, v);
	} else if (d->ch[0]->kind == N_STR) {
	  /* FIX(bug2): use length-aware intern for global string inits */
	  int id = intern_str_n(d->ch[0]->str, d->ch[0]->slen);
	  E("%s: %s _s%d\n", d->str, dw, id);
	} else if (d->ch[0]->kind == N_ID) {
	  E("%s: %s %s\n", d->str, dw, d->ch[0]->str);
	}
      }
      /* global array with explicit initialiser */
      if(d->kind==N_GARR && d->nlist > 0) {
	long cnt = d->ch[0]->num;
	const char *dw = d->ptrs ? "dd" :
	  (base_size(d->tbase)==1?"db":base_size(d->tbase)==2?"dw":
	   base_size(d->tbase)==8?"dq":"dd");
	E("%s: %s", d->str, dw);
	for(int j=0; j<d->nlist; j++)
	  E("%s%ld", j ? "," : " ", d->list[j]->num);
	for(long j=d->nlist; j<cnt; j++)
	  E("%s0", (j > 0 || d->nlist > 0) ? "," : " ");
	E("\n");
      }
    }
    /* FIX(bug2): pass length to emit_str_data */
    for(int i=0;i<str_cnt;i++) {
      E("_s%d: ", i);
      emit_str_data(str_val[i], str_val_len[i]);
    }
  }

  /* ── .bss section: zero-init scalars and uninitialised arrays ── */
  int has_bss = 0;
  for(int i=0;i<prog->nlist;i++) {
    Node *d=prog->list[i];
    /* FIX(bug1): check kind==N_NUM before num==0, so string/id inits
       don't produce a duplicate symbol in both .data and .bss */
    if (d->kind==N_GVAR && (!d->ch[0] || (d->ch[0]->kind==N_NUM && d->ch[0]->num==0))) has_bss=1;
    if (d->kind==N_GARR && d->nlist == 0) has_bss=1;
  }
  if (has_bss) {
    E("\nsection .bss\n");
    for(int i=0;i<prog->nlist;i++) {
      Node *d=prog->list[i];
      /* FIX(bug1): same guard as has_bss check above */
      if(d->kind==N_GVAR && (!d->ch[0] || (d->ch[0]->kind==N_NUM && d->ch[0]->num==0))) {
	const char *rs = d->ptrs ? "resd" :
	  (base_size(d->tbase)==1?"resb":base_size(d->tbase)==2?"resw":
	   base_size(d->tbase)==8?"resq":"resd");
	E("%s: %s 1\n", d->str, rs);
      }
      if(d->kind==N_GARR && d->nlist == 0) {
	long cnt = d->ch[0]->num;
	const char *rs = d->ptrs ? "resd" :
	  (base_size(d->tbase)==1?"resb":base_size(d->tbase)==2?"resw":
	   base_size(d->tbase)==8?"resq":"resd");
	E("%s: %s %ld\n", d->str, rs, cnt);
      }
    }
  }
}

/* ══════════════════════════════════════════════════════════
   MAIN
   ══════════════════════════════════════════════════════════ */
static char *read_file(const char *path) {
  FILE *f=fopen(path,"r");
  if(!f) die("cannot open: %s", path);
  fseek(f,0,SEEK_END); long sz=ftell(f); rewind(f);
  char *buf=malloc(sz+1);
  fread(buf,1,sz,f); buf[sz]=0; fclose(f);
  return buf;
}

int main(int argc, char **argv) {
  if (argc<2) { fprintf(stderr,"usage: %s <source.cm> [out.asm]\n",argv[0]); return 1; }
  char *source = read_file(argv[1]);
  src=source; src_pos=0;
  Node *prog = parse_prog();
  out = (argc>=3) ? fopen(argv[2],"w") : stdout;
  if (!out) die("cannot open output: %s", argv[2]);
  codegen(prog);
  if (argc>=3) { fclose(out); fprintf(stderr,"wrote %s\n",argv[2]); }
  return 0;
}
