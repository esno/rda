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

typedef struct {
  uint8_t version;
  uint32_t ptr;
  FILE *fd;
} rda_t;

void rda_close(rda_t *rda) {
  fclose(rda->fd);
  rda->fd = NULL;
}

int rda_open(rda_t *rda, char *archive) {
  memset(rda, 0, sizeof(rda_t));
  rda->fd = fopen(archive, "r");
  unsigned char magic[RDA_HEADER_MAGIC_LEN];

  if (rda->fd == NULL) {
    fprintf(stderr, "%s (%s)\n", strerror(errno), archive);
    return -1;
  }

  memset(magic, 0, sizeof(char) * RDA_HEADER_MAGIC_LEN);
  size_t l = fread(magic, sizeof(char), RDA_HEADER_2_2_MAGIC_LEN, rda->fd);
  if (l != RDA_HEADER_2_2_MAGIC_LEN) {
    fprintf(stderr, "Bad magic size (%d)\n", l);
    fclose(rda->fd);
    return -1;
  }

  if (strcmp(magic, RDA_HEADER_2_2_MAGIC) != 0) {
    fprintf(stderr, "Bad magic. Not a RDA 2.2 archive\n");
    fclose(rda->fd);
    return -1;
  }

  rda->version = RDA_VERSION_2_2;
  uint32_t ptr = 0;
  switch (rda->version) {
    case RDA_VERSION_2_0:
      fseek(rda->fd, sizeof(char) * RDA_HEADER_2_0_PTR, SEEK_SET);
      l = fread(&ptr, sizeof(char), RDA_HEADER_2_0_PTR_LEN, rda->fd);
      break;
    case RDA_VERSION_2_2:
      fseek(rda->fd, sizeof(char) * RDA_HEADER_2_2_PTR, SEEK_SET);
      l = fread(&ptr, sizeof(char), RDA_HEADER_2_2_PTR_LEN, rda->fd);
      break;
  }

  if (rda->version == RDA_VERSION_2_0 && l != RDA_HEADER_2_0_PTR_LEN ||
      rda->version == RDA_VERSION_2_2 && l != RDA_HEADER_2_2_PTR_LEN) {
    fprintf(stderr, "Corrupt block pointer\n");
    fclose(rda->fd);
    return -1;
  }

  rda->ptr = ptr;
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

  rda_t rda;
  if (rda_open(&rda, o.archive) == -1)
    return EXIT_FAILURE;

  rda_close(&rda);
  return EXIT_SUCCESS;
}