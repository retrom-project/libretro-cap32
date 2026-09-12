/* Retrom-owned regression; no external game or firmware input. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libretro.h>
#include "cap32.h"
#include "asic.h"
#include "cart.h"

extern t_CPC CPC;
extern t_GateArray GateArray;
extern t_CRTC CRTC;
extern uint8_t *pbRAM;
extern uint8_t *pbROMlo;
extern uint8_t *pbExpansionROM;
extern uint8_t *membank_read[4];
extern void ga_memory_manager(void);
static struct retro_variable options[128];
static char option_value[128];

static bool environment(unsigned command, void *data)
{
   if (command == RETRO_ENVIRONMENT_SET_VARIABLES) {
      const struct retro_variable *source = data;
      unsigned n = 0;
      do { assert(n < 128); options[n] = source[n]; } while (source[n++].key);
      return true;
   }
   if (command == RETRO_ENVIRONMENT_SET_PIXEL_FORMAT) return true;
   if (command == RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY ||
       command == RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY ||
       command == RETRO_ENVIRONMENT_GET_CONTENT_DIRECTORY) {
      *(const char **)data = "."; return true;
   }
   if (command == RETRO_ENVIRONMENT_GET_VARIABLE) {
      struct retro_variable *v = data;
      if (!strcmp(v->key, "cap32_model")) { v->value = "6128+ (experimental)"; return true; }
      if (!strcmp(v->key, "cap32_gfx_colors")) { v->value = "24bit"; return true; }
      for (const struct retro_variable *o = options; o && o->key; o++) {
         if (strcmp(o->key, v->key)) continue;
         const char *first = strchr(o->value, ';');
         assert(first); first += 2;
         size_t n = strcspn(first, "|"); assert(n < sizeof(option_value));
         memcpy(option_value, first, n); option_value[n] = 0;
         v->value = option_value; return true;
      }
   }
   return false;
}

int main(void)
{
   retro_set_environment(environment);
   retro_init();
   assert(retro_load_game(NULL));
   assert(CPC.model == CPC_MODEL_PLUS);
   asic.locked = false;
   asic.lock_seq_pos = ASIC_LOCK_SIZE;
   asic.rmr2 = 0xb9;
   asic.dma.ch[1].enabled = true;
   asic.dma.ch[1].source_address = 0x7654;
   asic.dma.ch[1].pause_ticks = 19;
   asic.sprites[2][3][4] = 13;
   asic.sprites_x[2] = -12;
   asic_register_page_write(0x6400, 0xaf);
   asic_register_page_write(0x6401, 7);
   asic_register_page_write(0x6804, 0xa5);
   GateArray.registerPageOn = true;
   GateArray.lower_ROM_bank = 0;
   /* Plus stores a decoded page; replaying page 5 as a ROM-select port byte maps page 1. */
   GateArray.upper_ROM = 5;
   pbExpansionROM = pbCartridgePages[5];
   pbCartridgePages[5][0] = 0x5a;
   GateArray.ROM_config &= ~8;
   ga_memory_manager();
   CRTC.split_addr = 0x1234;
   CRTC.sl_count = 87;
   pbRAM[0x4567] = 0xa3;
   uint32_t palette = GateArray.palette[0];
   size_t size = retro_serialize_size();
   uint8_t *state = malloc(size); assert(state);
   assert(retro_serialize(state, size));
   asic_reset(); pbRAM[0x4567] = 0;
   assert(retro_unserialize(state, size));
   assert(!asic.locked);
   assert(asic.lock_seq_pos == ASIC_LOCK_SIZE);
   assert(asic.rmr2 == 0xb9);
   assert(asic.dma.ch[1].enabled && asic.dma.ch[1].source_address == 0x7654);
   assert(asic.dma.ch[1].pause_ticks == 19);
   assert(asic.sprites[2][3][4] == 13 && asic.sprites_x[2] == -12);
   assert(asic.hscroll == 5 && asic.vscroll == 2 && asic.extend_border);
   assert(GateArray.registerPageOn && GateArray.palette[0] == palette);
   assert(CRTC.split_addr == 0x1234 && CRTC.sl_count == 87);
   assert(pbRAM[0x4567] == 0xa3);
   assert(pbROMlo == pbCartridgePages[1]);
   assert(GateArray.upper_ROM == 5 && pbExpansionROM == pbCartridgePages[5]);
   assert(membank_read[3] == pbCartridgePages[5] && membank_read[3][0] == 0x5a);
   /* Truncated, missing and unrecognised Plus extensions fail before reset. */
   pbRAM[0x4567] = 0x55;
   assert(!retro_unserialize(state, size - 1));
   assert(!asic.locked && pbRAM[0x4567] == 0x55);
   size_t ordinary = sizeof(t_SNA_header) + get_ram_size();
   assert(!retro_unserialize(state, ordinary));
   assert(!asic.locked && pbRAM[0x4567] == 0x55);
   state[ordinary] ^= 1;
   assert(!retro_unserialize(state, size));
   assert(!asic.locked && pbRAM[0x4567] == 0x55);
   state[ordinary] ^= 1;
   /* Sprite pixels index the 32-entry hardware palette directly. */
   size_t sprite_pixel = ordinary + 8 + 7 * 4;
   uint8_t original_pixel = state[sprite_pixel];
   state[sprite_pixel] = 255;
   assert(!retro_unserialize(state, size));
   assert(!asic.locked && pbRAM[0x4567] == 0x55);
   state[sprite_pixel] = original_pixel;
   assert(retro_unserialize(state, size));
   uint8_t *again = malloc(size); assert(again);
   assert(retro_serialize(again, size));
   assert(!memcmp(state + ordinary, again + ordinary, size - ordinary));
   free(again);
   /* Existing CPC SNA sizes and restoration remain unchanged. */
   CPC.model = CPC_MODEL_6128;
   assert(retro_serialize_size() == ordinary);
   assert(retro_serialize(state, ordinary));
   pbRAM[0x4567] = 0;
   assert(retro_unserialize(state, ordinary));
   assert(pbRAM[0x4567] == 0xa3);
   free(state); retro_deinit();
   puts("Plus snapshot round trip: PASS");
   return 0;
}
