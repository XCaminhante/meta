#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

char *output_name = NULL,
    *token = NULL,
    *source = NULL;
FILE *output = NULL;
int pos = 0,
    pos_last_line = 0,
    line = 1,
    test_flag = 0,
    ignore_whitespace = 0,
    input_len = 0,
    user_token=0,
    defining_utoken = 0;

void skip_whitespace (void) {
  while ((source[pos] == '\x20' || source[pos] == '\t' ||
  source[pos] == '\r' || source[pos] == '\n') && pos < input_len) {
    if (source[pos] == '\n') {
      pos_last_line = pos;
      line++;
    }
    pos++;
  }
}

void make_token (int start_pos) {
  if (defining_utoken) return;
  int length = pos - start_pos;
  free(token);
  token = malloc(length + 1);
  token[length] = '\0';
  memcpy(token, &source[start_pos], length);
}

void emit_token (int quote) {
  int i;
  if (token[0] == '\'') {
    if (quote) fprintf(output, "\"");
    for (i = 1; token[i+1] != '\0'; i++) {
      switch (token[i]) {
      case '\"':
        if (!quote) goto def_case;
        fprintf(output, "\\\"");
        break;
      case '\n':
        if (!quote) goto def_case;
        fprintf(output, "\\n");
        break;
      default:
        def_case:
        fprintf(output, "%c", token[i]);
        break;
      }
    }
    if (quote) fprintf(output, "\"");
    return;
  }
  fprintf(output, "%s", token);
}

void emit (const char *str) {
  fprintf(output, "%s", str);
}

void read_literal (const char *literal) {
  int entry_pos;
  int i;
  if (!ignore_whitespace) skip_whitespace();
  entry_pos = pos;
  i = 0;
  while (pos < input_len && literal[i] != '\0' && source[pos] == literal[i]) {
    pos++;
    i++;
  }
  if (literal[i] == '\0') {
    test_flag = 1;
    make_token(entry_pos);
  } else {
    pos = entry_pos;
    test_flag = 0;
  }
}

void read_any_between (char first, char last) {
  if (pos < input_len && source[pos] >= first && source[pos] <= last) {
    pos++;
    test_flag=1;
    make_token(pos-1);
    return;
  }
  test_flag=0;
}

void read_any_but (char first, char last) {
  if (pos < input_len && (source[pos] < first || source[pos] > last)) {
    pos++;
    test_flag=1;
    make_token(pos-1);
    return;
  }
  test_flag=0;
}

void read_char_eq (char which) {
  if (pos < input_len && source[pos] == which) {
    pos++;
    test_flag=1;
    make_token(pos-1);
    return;
  }
  test_flag=0;
}

void read_char_neq (char which) {
  if (pos < input_len && source[pos] != which) {
    pos++;
    test_flag=1;
    make_token(pos-1);
    return;
  }
  test_flag=0;
}

void read_id (void) {
  int entry_pos;
  if (!ignore_whitespace) skip_whitespace();
  entry_pos = pos;
  if (pos < input_len &&
  (('A' <= source[pos] && source[pos] <= 'Z') ||
  ('a' <= source[pos] && source[pos] <= 'z'))) {
    pos++;
    test_flag = 1;
  } else {
    test_flag = 0;
    return;
  }
  while (pos < input_len &&
  (('A' <= source[pos] && source[pos] <= 'Z') ||
  ('a' <= source[pos] && source[pos] <= 'z') ||
  ('0' <= source[pos] && source[pos] <= '9') || source[pos] == '_')) {
    pos++;
  }
  make_token(entry_pos);
}

void read_number (void) {
  int entry_pos;
  if (!ignore_whitespace) skip_whitespace();
  entry_pos = pos;
  if (pos < input_len && source[pos] == '-') {
    pos++;
  }
  if (pos < input_len && '0' <= source[pos] && source[pos] <= '9') {
    pos++;
    test_flag = 1;
  } else {
    test_flag = 0;
    return;
  }
  while (pos < input_len && '0' <= source[pos] && source[pos] <= '9') {
    pos++;
  }
  make_token(entry_pos);
}

void read_string (void) {
  int entry_pos;
  if (!ignore_whitespace) skip_whitespace();
  entry_pos = pos;
  if (pos < input_len && source[pos] == '\'') {
    pos++;
  } else {
    test_flag = 0;
    return;
  }
  while (pos < input_len && source[pos] != '\'') {
    switch (source[pos]) {
    case '\n':
      pos_last_line = pos;
      line++;
      break;
    case '\\':
      pos++;
    }
    pos++;
  }
  if (source[pos] == '\'') {
    pos++;
    test_flag = 1;
    make_token(entry_pos);
  } else if (pos < input_len) {
    pos = entry_pos;
    test_flag = 0;
  }
}

void error_if_false (void) {
  if (!test_flag) {
    fprintf(stderr, "error in line:column %i:%i at token %s\n", line, pos-pos_last_line, token);
    fclose(output);
    remove(output_name);
    free(source);
    free(token);
    exit(1);
  }
}

void meta_program (void);
void meta_exp1 (void);

//~ int main (int argc, char *argv[]) {
  //~ source = malloc(65535+1);
  //~ int chars = read(0,source,65535);
  //~ source[chars] = '\0';
  //~ input_len = chars;
  //~ output = stdout;
  //~ token = malloc(1);
  //~ token[0] = '\0';
//~ }

//~ int main(int argc, char *argv[]) {
  //~ FILE *input;
  //~ int length;
//~
  //~ if (argc != 3) {
    //~ fprintf(stderr, "usage: meta <input> <output>\n");
    //~ exit(1);
  //~ }
//~
  //~ // open input and output
  //~ input = fopen(argv[1], "r");
  //~ if (input == NULL) {
    //~ fprintf(stderr, "invalid input file\n");
    //~ exit(1);
  //~ }
  //~ output_name = argv[2];
  //~ output = fopen(output_name, "w");
  //~ if (output == NULL) {
    //~ fprintf(stderr, "invalid output file\n");
    //~ exit(1);
  //~ }
  //~ // read entire input into source
  //~ fseek(input, 0, SEEK_END);
  //~ length = (int)ftell(input);
  //~ fseek(input, 0, SEEK_SET);
  //~ source = malloc(length + 1);
  //~ fread(source, 1, length, input);
  //~ source[length] = '\0';
  //~ fclose(input);
//~
  //~ // initially we have empty token; token is never NULL
  //~ token = malloc(1);
  //~ token[0] = '\0';
//~
  //~ // run meta
  //~ meta_program();
//~
  //~ fprintf(stderr, "%d %d\n", pos, strlen(source));
//~
  //~ fclose(output);
  //~ free(source);
  //~ free(token);
  //~ return 0;
//~ }
