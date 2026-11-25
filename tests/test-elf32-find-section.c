#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "unw_test.h"

#include <string.h>

#include "elf32.h"

/* A section header's sh_offset and sh_size come from the file being
   unwound, so find_section() has to confirm that a section really lies
   inside the mapped image before handing its header back.  The image here
   is placed after a 64-byte guard area holding a decoy string table, so a
   section offset that wraps resolves to readable memory outside the image
   and the checks below stay deterministic.  */

#define GUARD_LEN     64
#define SHSTRTAB_OFF  64
#define SHSTRTAB_LEN  24
#define FRAME_OFF     88
#define FRAME_LEN     16
#define SHTAB_OFF     104
#define NUM_SECTIONS  3
#define IMAGE_LEN     (SHTAB_OFF + NUM_SECTIONS * sizeof (Elf32_Shdr))

static union
  {
    char bytes[GUARD_LEN + IMAGE_LEN];
    Elf32_Ehdr align;
  }
backing;

#define IMAGE (backing.bytes + GUARD_LEN)

/* Build a well-formed image with a .shstrtab and a .debug_frame section and
   return its section header table.  */
static Elf32_Shdr *
build_image (struct elf_image *ei)
{
  Elf32_Ehdr *ehdr = (Elf32_Ehdr *) IMAGE;
  Elf32_Shdr *shdr = (Elf32_Shdr *) (IMAGE + SHTAB_OFF);

  memset (&backing, 0, sizeof (backing));

  /* The decoy string table in the guard area names .debug_frame too.  */
  memcpy (backing.bytes + 11, ".debug_frame", 12);

  memcpy (ehdr->e_ident, ELFMAG, SELFMAG);
  ehdr->e_ident[EI_CLASS] = ELFCLASS32;
  ehdr->e_ident[EI_DATA] = ELFDATA2LSB;
  ehdr->e_ident[EI_VERSION] = EV_CURRENT;
  ehdr->e_type = ET_DYN;
  ehdr->e_version = EV_CURRENT;
  ehdr->e_shoff = SHTAB_OFF;
  ehdr->e_shentsize = sizeof (Elf32_Shdr);
  ehdr->e_shnum = NUM_SECTIONS;
  ehdr->e_shstrndx = 1;

  /* "\0.shstrtab\0.debug_frame\0" */
  memcpy (IMAGE + SHSTRTAB_OFF + 1, ".shstrtab", 9);
  memcpy (IMAGE + SHSTRTAB_OFF + 11, ".debug_frame", 12);
  memcpy (IMAGE + FRAME_OFF, "DEBUG_FRAME_DATA", FRAME_LEN);

  shdr[1].sh_name = 1;
  shdr[1].sh_type = SHT_STRTAB;
  shdr[1].sh_offset = SHSTRTAB_OFF;
  shdr[1].sh_size = SHSTRTAB_LEN;

  shdr[2].sh_name = 11;
  shdr[2].sh_type = SHT_PROGBITS;
  shdr[2].sh_offset = FRAME_OFF;
  shdr[2].sh_size = FRAME_LEN;

  ei->image = IMAGE;
  ei->size = IMAGE_LEN;
  return shdr;
}

int
main (void)
{
  struct elf_image ei;
  Elf32_Shdr *shdr;
  Elf32_Shdr *found;

  shdr = build_image (&ei);
  found = _Uelf32_find_section (&ei, ".debug_frame");
  UNW_TEST_ASSERT (found != NULL, "%s",
		   ".debug_frame not found in a well-formed image");
  UNW_TEST_ASSERT (found->sh_offset == FRAME_OFF,
		   "expected .debug_frame at 0x%x, got 0x%lx", FRAME_OFF,
		   (unsigned long) found->sh_offset);

  /* .debug_frame outside the image, with sh_offset + sh_size wrapping to a
     value that is inside it.  */
  shdr = build_image (&ei);
  shdr[2].sh_offset = (Elf32_Off) -GUARD_LEN;
  shdr[2].sh_size = GUARD_LEN;
  found = _Uelf32_find_section (&ei, ".debug_frame");
  UNW_TEST_ASSERT (found == NULL, "%s",
		   ".debug_frame outside the image was accepted");

  /* Same for the section name string table, which decides which section
     find_section() returns in the first place.  */
  shdr = build_image (&ei);
  shdr[1].sh_offset = (Elf32_Off) -GUARD_LEN;
  shdr[1].sh_size = GUARD_LEN;
  found = _Uelf32_find_section (&ei, ".debug_frame");
  UNW_TEST_ASSERT (found == NULL, "%s",
		   "string table outside the image was accepted");

  /* A string table index with no section header behind it.  */
  shdr = build_image (&ei);
  ((Elf32_Ehdr *) IMAGE)->e_shstrndx = NUM_SECTIONS;
  found = _Uelf32_find_section (&ei, ".debug_frame");
  UNW_TEST_ASSERT (found == NULL, "%s",
		   "string table index past the section table was accepted");

  return UNW_TEST_EXIT_PASS;
}
