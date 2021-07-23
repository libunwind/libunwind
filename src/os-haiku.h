#ifndef os_haiku_h
#define os_haiku_h

#include <kernel/image.h>
#include <kernel/OS.h>
#include <runtime_loader.h>

#define RFLAG_RW 0x0010

struct map_iterator
{
	runtime_loader_debug_area *debugArea;
	image_t *currentImage;
	uint32 numRegions;
	uint32 currentRegion;
};

static inline int
maps_init(struct map_iterator *mi)
{
  ssize_t cookie = 0;

  team_info teamInfo;
  area_info areaInfo;

  if (get_team_info(B_CURRENT_TEAM, &teamInfo) != B_OK)
  	return -1;

  while (get_next_area_info(teamInfo.team, &cookie, &areaInfo) == B_OK) {
  	if (strcmp(areaInfo.name, RUNTIME_LOADER_DEBUG_AREA_NAME) == 0) {
  		mi->debugArea = (runtime_loader_debug_area*)areaInfo.address;
		mi->currentImage = NULL;
		mi->numRegions = 0;
		mi->currentRegion = 0;

  		return 0;
  	}
  }

  return -1;
}

static inline int
maps_next(struct map_iterator *mi,
		unsigned long *low, unsigned long *high, unsigned long *offset,
		unsigned long *flags)
{
	while (mi->currentImage != mi->debugArea->loaded_images->tail)
	{
		if (mi->currentImage == NULL) {
			mi->currentImage = mi->debugArea->loaded_images->head;
			mi->numRegions = 0;
		}

		if (mi->numRegions == 0) {
			mi->numRegions = mi->currentImage->num_regions;
			mi->currentRegion = 0;
		}

		if (mi->currentRegion < mi->numRegions)
		{
			elf_region_t region = mi->currentImage->regions[mi->currentRegion];
			*low = region.vmstart;
			*high = region.vmstart + region.vmsize;
			*offset = region.fdstart;
			*flags = PROT_READ;
			if ((region.flags & RFLAG_RW) == RFLAG_RW)
				*flags |= PROT_WRITE;
			// currently there isn't a way to identify an executable mapping
			// via elf_region_t

			mi->currentRegion++;

			return 1;
		} else {
			mi->currentImage = mi->currentImage->next;
			mi->numRegions = 0;
		}
	}

	return 0;
}

static inline void
maps_close(struct map_iterator *mi)
{
	mi->currentImage = NULL;
	mi->numRegions = 0;
	mi->currentRegion = 0;
}

#endif
