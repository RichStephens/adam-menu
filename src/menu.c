/* Lists the .ddp files beside the menu's own image and boots the one
   picked from D4. A program returns here by exiting through EOS_START
   ($FC30), which reboots D1. */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <conio.h>
#include <eos.h>
#include <video/tms99x8.h>
#include <fujinet-fuji.h>

#define NUM_SLOTS     4
#define MENU_SLOT     0
#define RUN_SLOT      3
#define RUN_DEV       (RUN_SLOT + 4)
#define MODE_READ     1
#define INVALID_HOST  0xFF

#define MAX_ENTRIES   60
#define NAME_LEN      32
#define LIST_ROW      3
#define PAGE_LEN      18
#define HINT_ROW      (LIST_ROW + PAGE_LEN + 1)
#define MSG_ROW       (HINT_ROW + 1)
#define LINE_LEN      31

#define KEY_UP        0xA0
#define KEY_DOWN      0xA2
#define KEY_RETURN    '\n'
// Not 31: at that length the Adam firmware prefixes two type glyphs
#define ENTRY_LEN     64

// Where EOS loads a boot block, entered with B = device
#define BOOT_BLOCK    ((void *) 0xC800)

extern void chain_boot(uint8_t dev) __z88dk_fastcall;

static DeviceSlot slots[NUM_SLOTS];
static char dir[MAX_FILENAME_LEN];
static char path[MAX_FILENAME_LEN];
static char entry[ENTRY_LEN];
static char names[MAX_ENTRIES][NAME_LEN];
static char self[MAX_FILENAME_LEN];
static char line[LINE_LEN + 1];
static uint8_t host;
static uint8_t count;

static bool is_ddp(const char *name, size_t len)
{
  const char *ext;

  if (len < 5)
    return false;
  ext = name + len - 4;
  return ext[0] == '.'
    && (ext[1] | 0x20) == 'd' && (ext[2] | 0x20) == 'd' && (ext[3] | 0x20) == 'p';
}

static bool find_self(void)
{
  char *slash;

  if (!fuji_get_device_slots(slots, NUM_SLOTS))
    return false;
  host = slots[MENU_SLOT].hostSlot;
  if (host == INVALID_HOST)
    return false;

  if (!fuji_get_device_filename(MENU_SLOT, dir))
    return false;
  slash = strrchr(dir, '/');
  if (slash) {
    strcpy(self, slash + 1);
    slash[1] = 0;
  }
  else {
    strcpy(self, dir);
    strcpy(dir, "/");
  }

  // All of dir is sent; bytes after the path are read as a filter
  slash = dir + strlen(dir);
  memset(slash, 0, dir + sizeof(dir) - slash);
  return true;
}

static bool read_dir(void)
{
  size_t len;

  count = 0;
  if (!fuji_mount_host_slot(host) || !fuji_open_directory_filter(host, dir, ""))
    return false;

  while (count < MAX_ENTRIES) {
    memset(entry, 0, sizeof(entry));
    if (!fuji_read_directory(ENTRY_LEN, 0, entry))
      break;
    if (entry[0] == 0x7F && entry[1] == 0x7F)
      break;
    len = strlen(entry);
    if (len < NAME_LEN && is_ddp(entry, len) && strcmp(entry, self))
      strcpy(names[count++], entry);
  }

  fuji_close_directory();
  return true;
}

static bool launch(const char *name)
{
  fuji_unmount_disk_image(RUN_SLOT);

  // The Adam SET DEVICE FULLPATH keeps the slot's old host and mode
  slots[RUN_SLOT].hostSlot = host;
  slots[RUN_SLOT].mode = MODE_READ;
  memset(slots[RUN_SLOT].file, 0, FILE_MAXLEN);
  strncpy((char *) slots[RUN_SLOT].file, name, FILE_MAXLEN - 1);
  if (!fuji_put_device_slots(slots, NUM_SLOTS))
    return false;

  strcpy(path, dir);
  strcat(path, name);
  if (!fuji_set_device_filename(MODE_READ, host, RUN_SLOT, path)
      || !fuji_mount_disk_image(RUN_SLOT, MODE_READ))
    return false;

  // ReadBlock leaves the DCB status ($80) in A on success
  if (!eos_find_dcb(RUN_DEV)
      || eos_read_block(RUN_DEV, 0, BOOT_BLOCK) == EOS_ERR_DEVICE_DEPD)
    return false;

  // The loader overwrites the NMI vector at $0066
  vdp_set_reg(1, 0x80);
  chain_boot(RUN_DEV);
  return false;
}

static void show_line(uint8_t index, bool selected)
{
  // Padded so the highlight spans the row
  memset(line, ' ', LINE_LEN);
  memcpy(line + 1, names[index], strlen(names[index]));
  line[LINE_LEN] = 0;

  if (selected) {
    textcolor(BLACK);
    textbackground(WHITE);
  }
  gotoxy(0, LIST_ROW + index % PAGE_LEN);
  printf("%s", line);
  if (selected) {
    textcolor(WHITE);
    textbackground(BLACK);
  }
}

static void show_page(uint8_t top, uint8_t selected)
{
  uint8_t i;

  clrscr();
  gotoxy(0, 0);
  printf("Select a program");
  gotoxy(0, 1);
  memcpy(line, dir, LINE_LEN);
  line[LINE_LEN] = 0;
  printf("%s", line);
  for (i = top; i < top + PAGE_LEN && i < count; i++)
    show_line(i, i == selected);
  gotoxy(0, HINT_ROW);
  printf("Up/Down select, RETURN boots");
}

static void wait_key(const char *msg)
{
  printf("\n%s\nPress any key to retry\n", msg);
  fgetc_cons();
}

static uint8_t choose(void)
{
  uint8_t selected = 0, top = 0, next, key;

  show_page(top, selected);
  for (;;) {
    key = fgetc_cons();
    if (key == KEY_RETURN || key == '\r')
      return selected;
    if (key == KEY_UP)
      next = selected ? selected - 1 : count - 1;
    else if (key == KEY_DOWN)
      next = selected + 1 < count ? selected + 1 : 0;
    else
      continue;

    if (next / PAGE_LEN != top / PAGE_LEN) {
      top = next - next % PAGE_LEN;
      selected = next;
      show_page(top, selected);
    }
    else {
      show_line(selected, false);
      selected = next;
      show_line(selected, true);
    }
  }
}

int main(void)
{
  uint8_t pick;

  for (;;) {
    clrscr();
    if (!find_self()) {
      wait_key("Menu is not in D1 on a host");
      continue;
    }
    printf("Reading %s\n", dir);
    if (!read_dir()) {
      wait_key("Could not read directory");
      continue;
    }
    if (!count) {
      wait_key("No .ddp files found");
      continue;
    }

    pick = choose();
    gotoxy(0, MSG_ROW);
    printf("Booting %s", names[pick]);
    if (!launch(names[pick]))
      wait_key("Could not boot it");
  }
}
