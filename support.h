#ifndef META_SUPPORT
#define META_SUPPORT

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>
#include <stdbool.h>

#define STACK_DEPTH 20000

static long user_token_stack[STACK_DEPTH];
static long utoken_stack_top=0;
static char *parse_stack[STACK_DEPTH];
static long parse_stack_top=0;

static long token_line=0, token_column=0,
            bytes_written=0,input_len=0,
            pos=0,pos_last_line=0,line=1;
static FILE *output;
static char *source=NULL, *token=NULL, *capture=NULL,
            *input_name=NULL, *output_name=NULL;
static bool test_flag=false, ignore_whitespace=false, mute=false;
static char *expecting=NULL, *unexpected=NULL, *reason=NULL;

static void make_token (int start_pos);
static void no_unused_fwa ();

static void start_line (void) {
  pos_last_line=pos+1;
  line++;
}
static char* concatstr (char *txt, const char *concat) {
  int txtlen = strlen(txt),
      conclen = strlen(concat);
  char *newbuf = realloc(txt,txtlen+conclen+1);
  strcpy(newbuf+txtlen,concat);
  return newbuf;
}
static void enter_parse_rule (char *parse_rule) {
  parse_stack[parse_stack_top++]=parse_rule;
}
static void exit_parse_rule () {
  parse_stack_top--;
}
static void start_user_token () {
  user_token_stack[utoken_stack_top++]=pos;
}
static void end_user_token () {
  make_token(user_token_stack[--utoken_stack_top]);
}
static bool is_space (char c) {
  return (c==' ' || c=='\t' || c=='\r' || c=='\n');
}
static void skip_whitespace (void) {
  test_flag=false;
  while (is_space(source[pos]) && pos<input_len) {
    test_flag=true;
    if (source[pos] == '\n') {
      start_line(); }
    pos++; }
}
static void make_token (int start_pos) {
  token_line = line;
  token_column = start_pos-pos_last_line+1;
  int length = pos-start_pos;
  if (token) free(token);
  token = malloc(length + 1);
  token[length] = '\0';
  memcpy(token, &source[start_pos], length);
}
static void capture_to_token () {
  if (!capture) return;
  if (token) free(token);
  token=capture;
  capture=NULL;
}
static void emit (const char *str) {
  if (mute) return;
  if (!capture) {
    fprintf(output, "%s", str); bytes_written += strlen(str); }
  else {
    capture = concatstr(capture,str); }
}
static void emit_char (char c) {
  if (mute) return;
  if (!capture) {
    fputc(c, output); bytes_written++; }
  else {
    capture = concatstr(capture,(char[2]){c,'\0'}); }
}
static void emit_token (int quote) {
  if (mute || !token) return;
  if (!quote && strlen(token) == 1) {
    emit_char(token[0]); return; }
  int i=1;
  if (token[0] == '\'' || token[0] == '"') {
    if (quote) emit_char('"');
    for (; token[i+1] != '\0'; i++) {
      switch (token[i]) {
      case '"':
        if (!quote) goto def_case;
        emit("\\\"");
      break;
      case '\n':
        if (!quote) goto def_case;
        emit("\\n");
      break;
      case '\r':
        if (!quote) goto def_case;
        emit("\\r");
      break;
      case '\\':
        if (!quote) {
          i++;
          switch(token[i]) {
            case 'n': emit_char('\n'); break;
            case 'r': emit_char('\r'); break;
            default: goto def_case; }
          break; }
      default:
        def_case:
        emit_char(token[i]); }}
    if (quote) emit_char('"');
    return; }
  emit(token);
}
static void read_literal (const char *literal) {
  if (!ignore_whitespace) skip_whitespace();
  test_flag=true;
  long new_last_line=pos_last_line, new_lines=0, i=0;
  for (; (pos+i)<input_len && literal[i]!='\0'; i++) {
    if (source[pos+i] != literal[i]) {
      test_flag=false;
      return; }
    if (source[pos+i] == '\n') {
      new_last_line=pos+i+1;
      new_lines++; }}
  pos_last_line = new_last_line; line += new_lines; pos += i;
  make_token(pos-i);
}
static void read_string () {
  if (!ignore_whitespace) skip_whitespace();
  test_flag=true;
  if (!(pos<input_len && (source[pos] == '\'' || source[pos] == '"'))) {
    test_flag=false;
    return; }
  long new_last_line=pos_last_line, new_lines=0, i=1;
  for (; (pos+i)<input_len && source[pos+i] != source[pos]; i++) {
    switch (source[pos+i]) {
      case '\n': new_last_line=pos+i+1; new_lines++; break;
      case '\\': i++; }}
  pos += ++i;
  pos_last_line = new_last_line; line += new_lines;
  make_token(pos-i);
}
static inline bool alpha_und (char c) {
  return ('A'<=c && c<='Z') || ('a'<=c && c<='z') || c=='_';
}
static inline bool numeric (char c) {
  return ('0'<=c && c<='9');
}
static void read_id () {
  if (!ignore_whitespace) skip_whitespace();
  test_flag=true;
  if (!alpha_und(source[pos])) {
    test_flag=false; return; }
  long i=1;
  for (; (pos+i)<input_len &&
      (alpha_und(source[pos+i]) || numeric(source[pos+i])); i++);;
  pos += i;
  make_token(pos-i);
}
static void read_number () {
  if (!ignore_whitespace) skip_whitespace();
  test_flag=true;
  if (!numeric(source[pos])) {
    test_flag=false;
    return; }
  long i=1;
  for (; (pos+i)<input_len && numeric(source[pos+i]); i++);;
  pos += i;
  make_token(pos-i);
}
static void read_any_between (char first, char last) {
  if (pos<input_len && source[pos] >= first && source[pos] <= last) {
    if (source[pos] == '\n') {
      start_line(); }
    pos++;
    test_flag=true;
    return; }
  test_flag=false;
}
static void read_any_but (char first, char last) {
  if (pos<input_len && (source[pos]<first || source[pos] > last)) {
    if (source[pos] == '\n') {
      start_line(); }
    pos++;
    test_flag=true;
    return; }
  test_flag=false;
}
static void read_char_eq (char which) {
  if (pos<input_len && source[pos] == which) {
    if (which == '\n') {
      start_line(); }
    pos++;
    test_flag=true;
    return; }
  test_flag=false;
}
static void read_char_neq (char which) {
  if (pos<input_len && source[pos] != which) {
    if (which == '\n') {
      start_line(); }
    pos++;
    test_flag=true;
    return; }
  test_flag=false;
}
static void initialize_parser () {
  if (output) {
    fclose(output);
    output=NULL; }
  if (token) {
    free(token);
    token=NULL; }
  if (output_name) {
    if ((char*)output_name != (char*)"stdout") free(output_name);
    output_name=NULL; }
  if (input_name) {
    if ((char*)input_name != (char*)"stdin") free(input_name);
    input_name=NULL; }
  if (source) {
    free(source);
    source=NULL; }
  if (capture) {
    free(capture);
    capture=NULL; }
  parse_stack_top=0; utoken_stack_top=0;
  pos=0; pos_last_line=0; line=1;
  test_flag=false; ignore_whitespace=false; mute=false;
  input_len = token_line = token_column = bytes_written = 0;
  expecting=NULL; unexpected=NULL;
}
static void finalize_parser () {
  fclose(output);
  if (output_name) remove(output_name);
  exit(1);
}
static void status_line () {
  if (input_name) {
    fprintf(stderr, "%s:", input_name); }
  fprintf(stderr, "%li:%li: Rule %s",
          line, pos-pos_last_line+1,
          parse_stack[parse_stack_top-1]);
  if (token && token[0] != '\0') {
    fprintf(stderr,"\nLast token was '%s', starting at %li:%li",
            token,token_line,token_column); }
}
static void warning (const char *msg) {
  status_line();
  fprintf(stderr,"\nWarning: %s\n", msg);
}
static void error (const char *msg) {
  status_line();
  fprintf(stderr,"\nError: %s\n", msg);
  finalize_parser();
}
static void error_if_false () {
  if (!test_flag) {
    status_line();
    if (reason) {
      fputc('\n', stderr);
      fprintf(stderr,reason); }
    else {
      fprintf(stderr,"\nParsing error"); }
    if (expecting) {
      fprintf(stderr,", expecting %s",expecting); }
    if (unexpected) {
      fprintf(stderr,", unexpected %s",unexpected); }
    fputc('\n', stderr);
    finalize_parser(); }
}
static int first_into_second(int argc, char *argv[], void(*func)()) {
  FILE *input;
  if (argc > 3) {
    fprintf(stderr, "Usage: meta [<input>] [<output>]\n");
    exit(1); }
  if (argc == 1) {
    input=stdin;
    input_name="stdin";
    source=malloc(4194304+1);
    input_len=read(0,source,4194304); }
  else {
    input=fopen(argv[1], "r");
    if (input == NULL) {
      fprintf(stderr, "Invalid input file\n");
      exit(1); }
    input_name = argv[1];
    fseek(input, 0, SEEK_END);
    input_len = ftell(input);
    fseek(input, 0, SEEK_SET);
    source = malloc(input_len + 1);
    fread(source, 1, input_len, input); }
  if (argc<3) {
    output=stdout;
    output_name="stdout"; }
  else {
    output_name=argv[2];
    output=fopen(output_name, "w");
    if (output == NULL) {
      fprintf(stderr, "Invalid output file\n");
      exit(1); }}
  source[input_len]='\0';
  func();
  fprintf(stderr, "%li %li %li\n", input_len, pos, bytes_written);
  if (!bytes_written) remove(output_name);
  return 0;
}
static void multiple_files (int argc, char *argv[], char* extension, void(*func)()) {
  for (int a=1; a<argc; a++) {
    fprintf(stderr,"Processing file #%d...\n",a);
    initialize_parser();
    int input_name_len = strlen(argv[a]);
    output_name = malloc(input_name_len + strlen(extension) + 2);
    input_name = malloc(input_name_len);
    strcpy(output_name,argv[a]);
    strcpy(input_name,argv[a]);
    strcat(&output_name[input_name_len],extension);
    FILE* input=fopen(argv[a], "r");
    if (input == NULL) {
      fprintf(stderr, "Input file '%s' invalid\n",argv[a]);
      exit(1); }
    output=fopen(output_name, "w");
    if (output == NULL) {
      fprintf(stderr, "Can't write to output file '%s'\n",output_name);
      exit(1); }
    fseek(input, 0, SEEK_END);
    input_len=ftell(input);
    fseek(input, 0, SEEK_SET);
    source=malloc(input_len + 1);
    fread(source, 1, input_len, input);
    source[input_len]='\0';
    func();
    fprintf(stderr, "%li %li %li\n", input_len, pos, bytes_written);
    if (!bytes_written) {
      remove(output_name); } }
}
static int test (int source_len, void(*func)()) {
  source=malloc(source_len+1);
  int chars=read(0,source,source_len);
  input_len=chars;
  source[chars]='\0';
  output=stdout;
  input_name="stdin";
  output_name="stdout";
  func();
  return 0;
}

// To mute "unused function" warnings
static void(*no_unused_fwa1[])(char,char) = { read_any_between,read_any_but };
static void(*no_unused_fwa2[])(char) = { read_char_eq,read_char_neq,emit_char };
static void(*no_unused_fwa3[])() = { no_unused_fwa,
  start_user_token,end_user_token,capture_to_token,
  emit,emit_token,
  read_literal,read_id,read_string,read_number,
  warning,error,error_if_false,
  multiple_files };
static int(*no_unused_fwa4[])() = { first_into_second,test };
static void no_unused_fwa () {
  (void)&no_unused_fwa1;
  (void)&no_unused_fwa2;
  (void)&no_unused_fwa3;
  (void)&no_unused_fwa4;
}

#endif
