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
  uint64_t ptr;
  FILE *fd;
} rda_t;

typedef struct {
  uint32_t flags;
  uint32_t files;
  uint64_t csize;
  uint64_t usize;
  uint64_t nxt;
  uint64_t self;
} rda_block_t;

typedef struct {
  char path[RDA_FILE_PATH_LEN];
  uint64_t *ptr;
  uint64_t csize;
  uint64_t usize;
  uint64_t timestamp;
  struct {
    uint64_t csize;
    uint64_t usize;
  } resident;
} rda_file_t;

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
  uint64_t ptr = 0;
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

rda_block_t *rda_parse_block(rda_t *rda, uint64_t ptr) {
  rda_block_t *blk = malloc(sizeof(rda_block_t));
  memset(blk, 0, sizeof(rda_block_t));

  fseek(rda->fd, sizeof(char) * ptr, SEEK_SET);
  fread(&blk->flags, sizeof(char), RDA_BLOCK_HEADER_FLAGS_LEN, rda->fd);
  fread(&blk->files, sizeof(char), RDA_BLOCK_HEADER_FILES_LEN, rda->fd);

  uint32_t csize = 0, usize = 0, osize = 0;
  switch (rda->version) {
    case RDA_VERSION_2_0:
      csize = RDA_BLOCK_HEADER_2_0_CSIZE;
      usize = RDA_BLOCK_HEADER_2_0_USIZE;
      osize = RDA_BLOCK_HEADER_2_0_OSIZE;
      break;
    case RDA_VERSION_2_2:
      csize = RDA_BLOCK_HEADER_2_2_CSIZE;
      usize = RDA_BLOCK_HEADER_2_2_USIZE;
      osize = RDA_BLOCK_HEADER_2_2_OSIZE;
      break;
  }

  fread(&blk->csize, sizeof(char), csize, rda->fd);
  fread(&blk->usize, sizeof(char), usize, rda->fd);
  fread(&blk->nxt, sizeof(char), osize, rda->fd);

  blk->self = ptr;
  return blk;
}

rda_file_t *rda_parse_file(rda_t *rda, rda_block_t *blk) {
  rda_file_t *f = malloc(sizeof(rda_file_t));
  memset(f, 0, sizeof(rda_file_t));

  int r = (blk->flags & RDA_BLOCK_FLAGS_RESIDENT) >> 2;
  if (r == 1) {
    fseek(rda->fd, sizeof(char) * blk->self - 16, SEEK_SET);
    fread(&f->resident.csize, sizeof(char), 8, rda->fd);
    fread(&f->resident.usize, sizeof(char), 8, rda->fd);
    return NULL;
  }

  fseek(rda->fd, sizeof(char) * blk->self - blk->csize, SEEK_SET);
  char path[RDA_FILE_PATH_LEN];
  memset(&path, 0, sizeof(char) * RDA_FILE_PATH_LEN);
  memset(&f->path, 0, sizeof(char) * RDA_FILE_PATH_LEN);
  fread(&path, sizeof(char), RDA_FILE_PATH_LEN, rda->fd);
  for (int i = 0, j = 0; i < RDA_FILE_PATH_LEN; ++i) {
    if (path[i] != '\0') {
      f->path[j] = path[i];
      ++j;
    }
  }
  return f;
}

void rda_print_block(rda_block_t *blk) {
  int c = blk->flags & RDA_BLOCK_FLAGS_COMPRESSED;
  int e = (blk->flags & RDA_BLOCK_FLAGS_ENCRYPTED) >> 1;
  int r = (blk->flags & RDA_BLOCK_FLAGS_RESIDENT) >> 2;
  int d = (blk->flags & RDA_BLOCK_FLAGS_DELETED) >> 3;

  fprintf(stdout, "[**] block 0x%08x (c=%d;e=%d;r=%d;d=%d %u/%u)\n",
    blk->self, c, e, r, d, blk->csize, blk->usize);
}

void rda_print_file(rda_file_t *f) {
  fprintf(stdout, "[--] > %s\n", f->path);
}

int main(int argc, char *argv[]) {
  int opt;
  options_t o;

  memset(&o, 0, sizeof(options_t));
  while ((opt = getopt(argc, argv, "xf:")) != -1) {
    switch (opt) {
      case 'x':
        o.extract = 1;
        break;
      case 'f':
        o.archive = optarg;
        break;
      default:
        fprintf(stderr, "Usage: %s [-x] file\n", argv[0]);
        return EXIT_FAILURE;
    }
  }

  rda_t rda;
  if (rda_open(&rda, o.archive) == -1)
    return EXIT_FAILURE;

  if (o.extract) {
    uint64_t ptr = rda.ptr;

    do {
      rda_block_t *blk = rda_parse_block(&rda, ptr);
      rda_print_block(blk);
      rda_file_t *f = rda_parse_file(&rda, blk);
      rda_print_file(f);
      ptr = blk->nxt;
      free(blk);
    } while (ptr != 0);
  }

  rda_close(&rda);
  return EXIT_SUCCESS;
}
