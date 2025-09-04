#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include <rda.h>

typedef struct options options_t;
struct options {
  uint8_t extract;
  char *archive;
};

int is_rda(char *archive) {
  FILE *fd = fopen(archive, "r");
  unsigned char magic[RDA_HEADER_MAGIC_LEN];

  if (fd == NULL) {
    fprintf(stderr, "%s (%s)\n", strerror(errno), archive);
    return -1;
  }

  memset(magic, 0, sizeof(char) * RDA_HEADER_MAGIC_LEN);
  size_t magic_l = fread(magic, sizeof(char), RDA_HEADER_2_2_MAGIC_LEN, fd);
  if (magic_l != RDA_HEADER_2_2_MAGIC_LEN) {
    fprintf(stderr, "Bad magic size (%d)\n", magic_l);
    fclose(fd);
    return -1;
  }

  if (strcmp(magic, RDA_HEADER_2_2_MAGIC) != 0) {
    fprintf(stderr, "Bad magic. Not a RDA 2.2 archive\n");
    fclose(fd);
    return -1;
  }

  fclose(fd);
  return 0;
}

int main(int argc, char *argv[]) {
  int opt;
  options_t o;

  memset(&o, 0, sizeof(options_t));
  while (opt = (getopt(argc, argv, "+x") != -1)) {
    switch (opt) {
      case 'x':
        o.extract = 1;
        break;
      default:
        fprintf(stderr, "Usage: %s [-x] file\n", argv[0]);
        return EXIT_FAILURE;
    }
  }

  if (optind >= argc) {
    fprintf(stderr, "Usage: %s [-x] file\n", argv[0]);
    return EXIT_FAILURE;
  }

  o.archive = argv[optind];

  if (is_rda(o.archive) == -1)
    return EXIT_FAILURE;

  return EXIT_SUCCESS;
}