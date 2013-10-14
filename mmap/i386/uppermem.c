/* Compute amount of lower and upper memory till the first hole. */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2009  Free Software Foundation, Inc.
 *
 *  GRUB is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  GRUB is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GRUB.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <grub/memory.h>
#include <grub/mm.h>
#include <grub/misc.h>

GRUB_EXPORT(grub_mmap_get_lower);
GRUB_EXPORT(grub_mmap_get_upper);

static int
grub_mmap_get_lower_hook (grub_uint64_t addr, grub_uint64_t size,
			  grub_uint32_t type, void *closure)
{
  grub_uint64_t *lower = closure;

  if (type != GRUB_MACHINE_MEMORY_AVAILABLE)
    return 0;
  if (addr == 0)
    *lower = size;
  return 0;
}

grub_uint64_t
grub_mmap_get_lower (void)
{
  grub_uint64_t lower = 0;

  grub_mmap_iterate (grub_mmap_get_lower_hook, &lower);
  if (lower > 0x100000)
    lower =  0x100000;
  return lower;
}

static int
grub_mmap_get_upper_hook (grub_uint64_t addr, grub_uint64_t size,
			  grub_uint32_t type, void *closure)
{
  grub_uint64_t *upper = closure;

  if (type != GRUB_MACHINE_MEMORY_AVAILABLE)
    return 0;
  if (addr <= 0x100000 && addr + size > 0x100000)
    *upper = addr + size - 0x100000;
  return 0;
}

grub_uint64_t
grub_mmap_get_upper (void)
{
  grub_uint64_t upper = 0;

  grub_mmap_iterate (grub_mmap_get_upper_hook, &upper);
  return upper;
}

static int
grub_mmap_get_post64_hook (grub_uint64_t addr, grub_uint64_t size,
			   grub_uint32_t type, void *closure)
{
  grub_uint64_t *post64 = closure;

  if (type != GRUB_MACHINE_MEMORY_AVAILABLE)
    return 0;
  if (addr <= 0x4000000 && addr + size > 0x4000000)
    *post64 = addr + size - 0x4000000;
  return 0;
}

/* Count the continuous bytes after 64 MiB. */
grub_uint64_t
grub_mmap_get_post64 (void)
{
  grub_uint64_t post64 = 0;

  grub_mmap_iterate (grub_mmap_get_post64_hook, &post64);
  return post64;
}
