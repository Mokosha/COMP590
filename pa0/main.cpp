#include <stdio.h>
#include <string.h>
#include <algorithm>

static void reverse(char *str) {
  const int l = strlen(str);
  const int hl = l >> 1;
  for (int i = 0; i < hl; i++) {
    std::swap(str[i], str[l - i - 1]);
  }
}

int main(int argc, char* argv[]) {
  if (2 == argc && !strcmp(argv[1], "--help")) {
    printf("Reverses the arguments and their order but maintains the whitespace\n");
    printf("    (i.e. it is the identity function for palindromes)\n");
    return 0;
  }

  int forward_arg = 1;
  int backwards_arg = argc - 1;
  int total_len = 0;
  int next_space = 0;
  while (forward_arg < argc) {
    next_space += strlen(argv[forward_arg]);
    while (total_len < next_space) {
      char *arg = argv[backwards_arg];
      reverse(arg);
      const int l = strlen(arg);

      if (total_len + l <= next_space) {
	total_len += l;
	printf("%s%s", arg, total_len == next_space? " " : "");
      } else {
	const int end_pos = total_len + l;
	int chars_left = l;
	while (end_pos > next_space) {
	  int after_space = end_pos - next_space;
	  int before_space = chars_left - after_space;

	  next_space += strlen(argv[++forward_arg]);

	  printf("%.*s ", before_space, arg + l - chars_left);
	  total_len += before_space;
	  chars_left -= before_space;

	  if (end_pos <= next_space) {
	    printf("%s%s", arg + l - chars_left, end_pos == next_space? " " : "");
	  }
	} 
	total_len += chars_left;
      }
      backwards_arg--;
    }
    forward_arg++;
  }
  printf("\n");
  return 0;
}

