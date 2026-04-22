// VM_addr_map.c
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#define MAXSTR 1000

int main(int argc, char *argv[])
{
  char line[MAXSTR];
  int *page_table, *mem_map;
  unsigned int log_size, phy_size, page_size, d;
  unsigned int num_pages, num_frames;
  unsigned int offset, logical_addr, physical_addr, page_num, frame_num;

  /* Get the memory characteristics from the input file */
  fgets(line, MAXSTR, stdin);
  if((sscanf(line, "Logical address space size: %d^%d", &d, &log_size)) != 2){
    fprintf(stderr, "Unexpected line 1. Abort.\n");
    exit(-1);
  }
  fgets(line, MAXSTR, stdin);
  if((sscanf(line, "Physical address space size: %d^%d", &d, &phy_size)) != 2){
    fprintf(stderr, "Unexpected line 2. Abort.\n");
    exit(-1);
  }
  fgets(line, MAXSTR, stdin);
  if((sscanf(line, "Page size: %d^%d", &d, &page_size)) != 2){
    fprintf(stderr, "Unexpected line 3. Abort.\n");
    exit(-1);
  }

  // EDIT BELOW
  /* Calculate number of pages and frames
     num_pages = 2^log_size / 2^page_size = 2^(log_size - page_size)
     num_frames = 2^phy_size / 2^page_size = 2^(phy_size - page_size) */
  num_pages = 1u << (log_size - page_size);
  num_frames = 1u << (phy_size - page_size);
  fprintf(stdout, "Number of Pages: %d, Number of Frames: %d\n\n", num_pages, num_frames);
  // EDIT ABOVE

  /* Allocate arrays to hold the page table and memory frames map */
  // EDIT BELOW
  page_table = (int *)malloc(num_pages * sizeof(int));
  mem_map    = (int *)malloc(num_frames * sizeof(int));
  // EDIT ABOVE

  /* Initialize page table to indicate that no pages are currently mapped to
     physical memory */
  // EDIT BELOW
  for (unsigned int i = 0; i < num_pages; i++) {
    page_table[i] = -1;
  }
  // EDIT ABOVE

  /* Initialize memory map table to indicate no valid frames */
  // EDIT BELOW
  for (unsigned int i = 0; i < num_frames; i++) {
    mem_map[i] = 0;
  }
  // EDIT ABOVE

  /* Read each accessed address from input file. Map the logical address to
     corresponding physical address */
  fgets(line, MAXSTR, stdin);
  while(!(feof(stdin))){
    sscanf(line, "0x%x", &logical_addr);
    fprintf(stdout, "Logical Address: 0x%x\n", logical_addr);
    
    /* Calculate page number and offset from the logical address */
    // EDIT BELOW
    unsigned int offset_mask = (1u << page_size) - 1u;
    offset   = logical_addr & offset_mask;
    page_num = logical_addr >> page_size;
    fprintf(stdout, "Page Number: %d\n", page_num);

    /* Check for page fault (page not yet mapped to a frame) */
    if (page_table[page_num] == -1) {
      fprintf(stdout, "Page Fault!\n");

      /* Find the first free frame in mem_map */
      for (frame_num = 0; frame_num < num_frames; frame_num++) {
        if (mem_map[frame_num] == 0) break;
      }

      /* Map the page to the free frame */
      page_table[page_num] = frame_num;
      mem_map[frame_num]   = 1;
    } else {
      /* Look up frame number from page table */
      frame_num = page_table[page_num];
    }
    fprintf(stdout, "Frame Number: %d\n", frame_num);
    // EDIT ABOVE

    /* Form corresponding physical address */
    // EDIT BELOW
    physical_addr = (frame_num << page_size) | offset;
    fprintf(stdout, "Physical Address: 0x%x\n\n", physical_addr);
    // EDIT ABOVE

    /* Read next line */
    fgets(line, MAXSTR, stdin);    
  }

  // EDIT BELOW
  free(page_table);
  free(mem_map);
  // EDIT ABOVE

  return 0;
}