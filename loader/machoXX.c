
#include <grub/file.h>
#include <grub/mm.h>
#include <grub/misc.h>

#define min(a,b) (((a) < (b)) ? (a) : (b))

int
SUFFIX (grub_macho_contains_macho) (grub_macho_t macho)
{
  return macho->offsetXX != -1;
}

void
SUFFIX (grub_macho_parse) (grub_macho_t macho)
{
  grub_macho_header_t head;

  /* Is there any candidate at all? */
  if (macho->offsetXX == -1)
    return;

  /* Read header and check magic*/
  if (grub_file_seek (macho->file, macho->offsetXX) == (grub_off_t) -1
      || grub_file_read (macho->file, &head, sizeof (head))
      != sizeof(head))
    {
      grub_error (GRUB_ERR_READ_ERROR, "cannot read Mach-O header");
      macho->offsetXX = -1;
      return;
    }
  if (head.magic != GRUB_MACHO_MAGIC)
    {
      grub_error (GRUB_ERR_BAD_OS, "invalid Mach-O " XX "-bit header");
      macho->offsetXX = -1;
      return;
    }

  /* Read commands. */
  macho->ncmdsXX = head.ncmds;
  macho->cmdsizeXX = head.sizeofcmds;
  macho->cmdsXX = grub_malloc(macho->cmdsizeXX);
  if (! macho->cmdsXX)
    {
      grub_error (GRUB_ERR_OUT_OF_MEMORY, "not enough memory to read commands");
      return;
    }
  if (grub_file_read (macho->file, macho->cmdsXX,
		      (grub_size_t) macho->cmdsizeXX)
      != (grub_ssize_t) macho->cmdsizeXX)
    {
      grub_error (GRUB_ERR_READ_ERROR, "cannot read Mach-O header");
      macho->offsetXX = -1;
    }
}

typedef int (*grub_macho_iter_hook_t)
(grub_macho_t , struct grub_macho_cmd *,
	       void *);

static grub_err_t
grub_macho_cmds_iterate (grub_macho_t macho,
			 grub_macho_iter_hook_t hook,
			 void *hook_arg)
{
  grub_uint8_t *hdrs = macho->cmdsXX;
  int i;
  if (! macho->cmdsXX)
    return grub_error (GRUB_ERR_BAD_OS, "couldn't find " XX "-bit Mach-O");
  for (i = 0; i < macho->ncmdsXX; i++)
    {
      struct grub_macho_cmd *hdr = (struct grub_macho_cmd *) hdrs;
      if (hook (macho, hdr, hook_arg))
	break;
      hdrs += hdr->cmdsize;
    }

  return grub_errno;
}

grub_size_t
SUFFIX (grub_macho_filesize) (grub_macho_t macho)
{
  if (SUFFIX (grub_macho_contains_macho) (macho))
    return macho->endXX - macho->offsetXX;
  return 0;
}

grub_err_t
SUFFIX (grub_macho_readfile) (grub_macho_t macho, void *dest)
{
  grub_ssize_t read;
  if (! SUFFIX (grub_macho_contains_macho) (macho))
    return grub_error (GRUB_ERR_BAD_OS,
		       "couldn't read architecture-specific part");

  if (grub_file_seek (macho->file, macho->offsetXX) == (grub_off_t) -1)
    {
      grub_error_push ();
      return grub_error (GRUB_ERR_BAD_OS,
			 "invalid offset in program header");
    }

  read = grub_file_read (macho->file, dest,
			 macho->endXX - macho->offsetXX);
  if (read != (grub_ssize_t) (macho->endXX - macho->offsetXX))
    {
      grub_error_push ();
      return grub_error (GRUB_ERR_BAD_OS,
			 "couldn't read architecture-specific part");
    }
  return GRUB_ERR_NONE;
}

struct grub_macho_size_closure
{
  grub_macho_addr_t *segments_start;
  grub_macho_addr_t *segments_end;
  int flags;
  int nr_phdrs;
};

/* Run through the program headers to calculate the total memory size we
   should claim.  */
static int
grub_macho_size_calcsize (grub_macho_t _macho __attribute__ ((unused)),
			  struct grub_macho_cmd *hdr0,
			  void *closure)
{
  struct grub_macho_size_closure *c = closure;
  grub_macho_segment_t *hdr = (grub_macho_segment_t *) hdr0;
  if (hdr->cmd != GRUB_MACHO_CMD_SEGMENT)
    return 0;

  if (! hdr->vmsize)
    return 0;

  if (! hdr->filesize && (c->flags & GRUB_MACHO_NOBSS))
    return 0;

  c->nr_phdrs++;
  if (hdr->vmaddr < *(c->segments_start))
    *(c->segments_start) = hdr->vmaddr;
  if (hdr->vmaddr + hdr->vmsize > *(c->segments_end))
    *(c->segments_end) = hdr->vmaddr + hdr->vmsize;
  return 0;
}

/* Calculate the amount of memory spanned by the segments. */
grub_err_t
SUFFIX (grub_macho_size) (grub_macho_t macho, grub_macho_addr_t *segments_start,
			  grub_macho_addr_t *segments_end, int flags)
{
  struct grub_macho_size_closure c;

  c.segments_start = segments_start;
  c.segments_end = segments_end;
  c.flags = flags;
  c.nr_phdrs = 0;

  *segments_start = (grub_macho_addr_t) -1;
  *segments_end = 0;

  grub_macho_cmds_iterate (macho, grub_macho_size_calcsize, &c);

  if (c.nr_phdrs == 0)
    return grub_error (GRUB_ERR_BAD_OS, "no program headers present");

  if (*segments_end < *segments_start)
    /* Very bad addresses.  */
    return grub_error (GRUB_ERR_BAD_OS, "bad program header load addresses");

  return GRUB_ERR_NONE;
}

struct grub_macho_load_closure
{
  char *offset;
  int flags;
  grub_err_t err;
};

static int
grub_macho_load_do_load (grub_macho_t _macho,
			 struct grub_macho_cmd *hdr0,
			 void *closure)
{
  struct grub_macho_load_closure *c = closure;
  grub_macho_segment_t *hdr = (grub_macho_segment_t *) hdr0;

  if (hdr->cmd != GRUB_MACHO_CMD_SEGMENT)
    return 0;

  if (! hdr->filesize && (c->flags & GRUB_MACHO_NOBSS))
    return 0;
  if (! hdr->vmsize)
    return 0;

  if (grub_file_seek (_macho->file, hdr->fileoff
		      + _macho->offsetXX) == (grub_off_t) -1)
    {
      grub_error_push ();
      grub_error (GRUB_ERR_BAD_OS,
		  "invalid offset in program header");
      return 1;
    }

  if (hdr->filesize)
    {
      grub_ssize_t read;
      read = grub_file_read (_macho->file, c->offset + hdr->vmaddr,
			     min (hdr->filesize, hdr->vmsize));
      if (read != (grub_ssize_t) min (hdr->filesize, hdr->vmsize))
	{
	  /* XXX How can we free memory from `load_hook'? */
	  grub_error_push ();
	  c->err = grub_error (GRUB_ERR_BAD_OS,
			       "couldn't read segment from file: "
			       "wanted 0x%lx bytes; read 0x%lx bytes",
			       hdr->filesize, read);
	  return 1;
	}
    }

  if (hdr->filesize < hdr->vmsize)
    grub_memset (c->offset + hdr->vmaddr + hdr->filesize,
		 0, hdr->vmsize - hdr->filesize);
  return 0;
}

/* Load every loadable segment into memory specified by `_load_hook'.  */
grub_err_t
SUFFIX (grub_macho_load) (grub_macho_t macho, char *offset, int flags)
{
  struct grub_macho_load_closure c;

  c.offset = offset;
  c.flags = flags;
  c.err = 0;
  grub_macho_cmds_iterate (macho, grub_macho_load_do_load, &c);

  return c.err;
}

static int
grub_macho_get_entry_point_hook (grub_macho_t _macho __attribute__ ((unused)),
				 struct grub_macho_cmd *hdr,
				 void *closure)
{
  grub_macho_addr_t *entry_point = closure;
  if (hdr->cmd == GRUB_MACHO_CMD_THREAD)
    *entry_point = ((grub_macho_thread_t *) hdr)->entry_point;
  return 0;
}

grub_macho_addr_t
SUFFIX (grub_macho_get_entry_point) (grub_macho_t macho)
{
  grub_macho_addr_t entry_point = 0;
  grub_macho_cmds_iterate (macho, grub_macho_get_entry_point_hook,
			   &entry_point);
  return entry_point;
}
