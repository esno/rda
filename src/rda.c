#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <rda.h>

#define RDA_BUFFER_SIZE 1024

typedef struct options options_t;
struct options {
  uint8_t extract;
  char *archive;
  char *workdir;
};

typedef struct {
  uint8_t version;
  uint64_t ptr;
  FILE *fd;
} rda_t;

typedef struct {
  uint64_t ptr;
  uint32_t flags;
  uint32_t files;
  uint64_t csize;
  uint64_t usize;
  uint64_t nxt;
  uint64_t self;
} rda_block_t;

typedef struct {
  char path[RDA_FILE_HEADER_PATH_LEN];
  uint64_t ptr;
  uint64_t csize;
  uint64_t usize;
  uint64_t mtime;
  uint64_t nxt;
  uint64_t self;
} rda_file_t;

void rda_close(rda_t *rda) {
  fclose(rda->fd);
  rda->fd = NULL;
}

int rda_extract_file(rda_t *rda, rda_file_t *f, char *workdir) {
  char *path = strdup(f->path);
  char *token = NULL;
  uint32_t w = strlen(workdir);
  uint32_t p = strlen(path);
  uint32_t t = w;

  char dest[w + p + 1];
  memset(&dest, 0, sizeof(dest));
  strcpy(dest, workdir);
  do {
    struct stat sb;
    if (stat(dest, &sb) == -1) {
      if (mkdir(dest, 0755) == -1) {
        fprintf(stderr, "[\e[0;31m!\e[0m] %s: mkdir(\e[0;34m%s\e[0m)\n", strerror(errno), dest);
        return -1;
      }
    }

    token = strsep(&path, "/");
    strcpy(&dest[t], "/");
    strcpy(&dest[t + 1], token);
    t = t + strlen(token) + 1;
  } while (t - w < p);

  unsigned char *buffer[RDA_BUFFER_SIZE];
  memset(buffer, 0, sizeof(char) * RDA_BUFFER_SIZE);
  FILE *fd = fopen(dest, "w");
  if (fd == NULL) {
    fprintf(stderr, "[\e[0;31m!\e[0m] %s: fopen(\e[0;34m%s\e[0m)\n", strerror(errno), dest);
    return -1;
  }

  size_t rcount, wcount, rsize = 0, bsize = 0;
  fseek(rda->fd, sizeof(char) * f->ptr, SEEK_SET);
  do {
    if (f->csize <= RDA_BUFFER_SIZE)
      bsize = f->csize;
    else
      bsize = (f->csize - rsize <= RDA_BUFFER_SIZE) ? f->csize - rsize : RDA_BUFFER_SIZE;

    rcount = fread(buffer, sizeof(char), bsize, rda->fd);
    wcount = fwrite(buffer, sizeof(char), rcount, fd);
    if (wcount != rcount) {
      fprintf(stderr, "[\e[0;31m!\e[0m] %s: fwrite(\e[0;34m%s\e[0m)\n", strerror(errno), dest);
      fclose(fd);
      return -1;
    }
    rsize += sizeof(char) * rcount;
  } while (rsize < f->csize);

  fclose(fd);
  return 0;
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
  blk->ptr = blk->self - blk->csize;

  if (blk->nxt == 0 && blk->csize == 0 && blk->usize == 0) {
    free(blk);
    return NULL;
  }

  return blk;
}

rda_file_t *rda_parse_file(rda_t *rda, rda_block_t *blk, uint64_t ptr) {
  rda_file_t *f = malloc(sizeof(rda_file_t));
  memset(f, 0, sizeof(rda_file_t));

  char path[RDA_FILE_HEADER_PATH_LEN];
  memset(&path, 0, sizeof(char) * RDA_FILE_HEADER_PATH_LEN);

  f->self = ptr;
  fseek(rda->fd, sizeof(char) * f->self, SEEK_SET);
  fread(&path, sizeof(char), RDA_FILE_HEADER_PATH_LEN, rda->fd);
  for (int i = 0, j = 0; i < RDA_FILE_HEADER_PATH_LEN; ++i) {
    if (path[i] != '\0') {
      f->path[j] = path[i];
      ++j;
    }
  }

  uint32_t osize = 0, csize = 0, usize = 0, hsize = 0, mtime = 0;
  switch (rda->version) {
    case RDA_VERSION_2_0:
      osize = RDA_FILE_HEADER_2_0_OSIZE;
      csize = RDA_FILE_HEADER_2_0_CSIZE;
      usize = RDA_FILE_HEADER_2_0_USIZE;
      mtime = RDA_FILE_HEADER_2_0_MTIME;
      hsize = RDA_FILE_HEADER_2_0_SIZE;
      break;
    case RDA_VERSION_2_2:
      osize = RDA_FILE_HEADER_2_2_OSIZE;
      csize = RDA_FILE_HEADER_2_2_CSIZE;
      usize = RDA_FILE_HEADER_2_2_USIZE;
      mtime = RDA_FILE_HEADER_2_2_MTIME;
      hsize = RDA_FILE_HEADER_2_2_SIZE;
      break;
  }

  fread(&f->ptr, sizeof(char), osize, rda->fd);
  fread(&f->csize, sizeof(char), csize, rda->fd);
  fread(&f->usize, sizeof(char), usize, rda->fd);
  fread(&f->mtime, sizeof(char), mtime, rda->fd);
  f->nxt = f->self + hsize;

  return f;
}

void rda_print_block(rda_block_t *blk) {
  int c = blk->flags & RDA_BLOCK_FLAGS_COMPRESSED;
  int e = (blk->flags & RDA_BLOCK_FLAGS_ENCRYPTED) >> 1;
  int r = (blk->flags & RDA_BLOCK_FLAGS_RESIDENT) >> 2;
  int d = (blk->flags & RDA_BLOCK_FLAGS_DELETED) >> 3;

  fprintf(stdout, "[\e[0;32m#\e[0m] block@\e[0;36m0x%08x\e[0m (c=%d;e=%d;r=%d;d=%d [n=%d] %u/%u) => \e[0;36m0x%08x\e[0m\n",
    blk->self, c, e, r, d, blk->files, blk->csize, blk->usize, blk->nxt);
}

void rda_print_file(rda_file_t *f) {
  fprintf(stdout, "[\e[0;32m+\e[0m] > \e[0;34m%s\e[0m@\e[0;36m0x%08x\e[0m (m=%d %u/%u) => \e[0;36m0x%08x\e[0m\n",
    f->path, f->ptr, f->mtime, f->csize, f->usize, f->nxt);
}

int main(int argc, char *argv[]) {
  int opt;
  options_t o;

  memset(&o, 0, sizeof(options_t));
  while ((opt = getopt(argc, argv, "xf:C:")) != -1) {
    switch (opt) {
      case 'C':
        o.workdir = optarg;
        break;
      case 'f':
        o.archive = optarg;
        break;
      case 'x':
        o.extract = 1;
        break;
      default:
        fprintf(stderr, "Usage: %s -xf file [-C ./workdir] \n", argv[0]);
        return EXIT_FAILURE;
    }
  }

  o.workdir = (o.workdir == NULL) ? "." : o.workdir;

  if (o.extract && o.archive) {
    rda_t rda;
    if (rda_open(&rda, o.archive) == -1)
      return EXIT_FAILURE;

    uint64_t ptr = rda.ptr;

    do {
      rda_block_t *blk = rda_parse_block(&rda, ptr);
      if (blk == NULL)
        break;

      rda_print_block(blk);
      rda_file_t *f = NULL;
      uint64_t fptr = blk->ptr;
      for (int i = 0; i < blk->files; ++i) {
        f = rda_parse_file(&rda, blk, fptr);
        rda_print_file(f);
        if (rda_extract_file(&rda, f, o.workdir) == -1)
          break;

        fptr = f->nxt;
        free(f);
      }

      ptr = blk->nxt;
      free(blk);
    } while (ptr != 0);

    rda_close(&rda);
  } else  {
    fprintf(stderr, "Usage: %s -xf file [-C ./workdir] \n", argv[0]);
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
