/* Retrom Caprice32 Plus state extension, GPL-2.0-or-later.
 * SNA v3 omits the ASIC. Save explicit little-endian fields, never pointers or
 * C structure padding. CPPLUS02 is a fixed-size, versioned libretro extension.
 */
#include "plus_snapshot.h"
#include "asic.h"
#include "cart.h"
#include "crtc.h"

extern t_GateArray GateArray;
extern t_CRTC CRTC;
extern uint8_t asic_ram[16384];
extern double asic_colours[32][3];
extern uint8_t *pbROMlo;
extern uint8_t *pbExpansionROM;
extern void ga_memory_manager(void);

struct plus_state {
   t_asic chip;
   uint8_t ram[16384], registers[16384];
   uint32_t palette[34];
   uint32_t lower_bank, register_page, split_addr, split_sl, sl_count, interrupt_sl, upper_page;
};

static uint32_t transfer_word(uint8_t **cursor, uint32_t value, bool writing)
{
   uint8_t *p = *cursor;
   if (writing) {
      for (unsigned n = 0; n < 4; n++) p[n] = (uint8_t)(value >> (n * 8));
   } else {
      value = (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
              ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
   }
   *cursor += 4;
   return value;
}

static void transfer_bytes(uint8_t **cursor, void *value, size_t size, bool writing)
{
   if (writing) memcpy(*cursor, value, size);
   else memcpy(value, *cursor, size);
   *cursor += size;
}

static void transfer_state(struct plus_state *s, uint8_t *p, bool writing)
{
#define WORD(field) (field) = transfer_word(&p, (uint32_t)(field), writing)
#define BYTES(field) transfer_bytes(&p, (field), sizeof(field), writing)
   WORD(s->chip.locked); WORD(s->chip.lock_seq_pos); WORD(s->chip.lock_prev_data);
   WORD(s->chip.rmr2); WORD(s->chip.extend_border);
   WORD(s->chip.hscroll); WORD(s->chip.vscroll);
   BYTES(s->chip.sprites);
   for (unsigned n = 0; n < ASIC_SPRITES; n++) {
      WORD(s->chip.sprites_x[n]); WORD(s->chip.sprites_y[n]);
      WORD(s->chip.sprites_mag_x[n]); WORD(s->chip.sprites_mag_y[n]);
   }
   for (unsigned n = 0; n < NB_DMA_CHANNELS; n++) {
      t_DMA_channel *c = &s->chip.dma.ch[n];
      WORD(c->source_address); WORD(c->loop_address); WORD(c->prescaler);
      WORD(c->enabled); WORD(c->interrupt); WORD(c->pause_ticks);
      WORD(c->tick_cycles); WORD(c->loops);
   }
   WORD(s->chip.dma.dcsr); WORD(s->chip.dma.clear);
   WORD(s->chip.raster_interrupt); WORD(s->chip.interrupt_vector);
   WORD(s->chip.irq_cause); WORD(s->chip.irq_vector);
   BYTES(s->ram); BYTES(s->registers);
   for (unsigned n = 0; n < 34; n++) WORD(s->palette[n]);
   WORD(s->lower_bank); WORD(s->register_page); WORD(s->split_addr);
   WORD(s->split_sl); WORD(s->sl_count); WORD(s->interrupt_sl); WORD(s->upper_page);
#undef WORD
#undef BYTES
}

static bool decode(struct plus_state *s, const uint8_t *data, size_t size)
{
   if (!data || size != PLUS_SNAPSHOT_SIZE || memcmp(data, "CPPLUS02", 8)) return false;
   memset(s, 0, sizeof(*s));
   transfer_state(s, (uint8_t *)data + 8, false);
   if (s->chip.lock_seq_pos < 0 || s->chip.lock_seq_pos > ASIC_LOCK_SIZE ||
       s->chip.hscroll < 0 || s->chip.hscroll > 15 ||
       s->chip.vscroll < 0 || s->chip.vscroll > 7 ||
       s->lower_bank > 2 || s->register_page > 1 || s->upper_page > 31) return false;
   for (unsigned n = 0; n < ASIC_SPRITES; n++) {
      if (s->chip.sprites_mag_x[n] < 0 || s->chip.sprites_mag_x[n] > 4 ||
          s->chip.sprites_mag_y[n] < 0 || s->chip.sprites_mag_y[n] > 4) return false;
   }
   for (unsigned n = 0; n < NB_DMA_CHANNELS; n++) {
      if (s->chip.dma.ch[n].source_address > 0xffff ||
          s->chip.dma.ch[n].loop_address > 0xffff) return false;
   }
   return true;
}

bool plus_snapshot_save(uint8_t *data, size_t size)
{
   struct plus_state s;
   if (!data || size != PLUS_SNAPSHOT_SIZE || !pbRegisterPage) return false;
   memset(&s, 0, sizeof(s));
   s.chip = asic;
   memcpy(s.ram, asic_ram, sizeof(s.ram));
   memcpy(s.registers, pbRegisterPage, sizeof(s.registers));
   memcpy(s.palette, GateArray.palette, sizeof(s.palette));
   s.upper_page = GateArray.upper_ROM;
   s.lower_bank = GateArray.lower_ROM_bank; s.register_page = GateArray.registerPageOn;
   s.split_addr = CRTC.split_addr; s.split_sl = CRTC.split_sl;
   s.sl_count = CRTC.sl_count; s.interrupt_sl = CRTC.interrupt_sl;
   memset(data, 0, size);
   memcpy(data, "CPPLUS02", 8);
   transfer_state(&s, data + 8, true);
   return true;
}

bool plus_snapshot_valid(const uint8_t *data, size_t size)
{
   struct plus_state s;
   return decode(&s, data, size);
}

bool plus_snapshot_load(const uint8_t *data, size_t size)
{
   struct plus_state s;
   if (!decode(&s, data, size) || !pbRegisterPage) return false;
   asic = s.chip;
   memcpy(asic_ram, s.ram, sizeof(s.ram));
   memcpy(pbRegisterPage, s.registers, sizeof(s.registers));
   memcpy(GateArray.palette, s.palette, sizeof(s.palette));
   for (unsigned n = 0; n < 32; n++) {
      uint8_t rb = asic_ram[0x2400 + n * 2], g = asic_ram[0x2401 + n * 2];
      asic_colours[n][0] = (double)(rb >> 4) / 16;
      asic_colours[n][1] = (double)(g & 15) / 16;
      asic_colours[n][2] = (double)(rb & 15) / 16;
   }
   GateArray.lower_ROM_bank = s.lower_bank; GateArray.registerPageOn = s.register_page;
   CRTC.split_addr = s.split_addr; CRTC.split_sl = s.split_sl;
   CRTC.sl_count = s.sl_count; CRTC.interrupt_sl = s.interrupt_sl;
   pbROMlo = pbCartridgePages[asic.rmr2 & 7];
   GateArray.upper_ROM = s.upper_page;
   pbExpansionROM = pbCartridgePages[s.upper_page];
   ga_memory_manager();
   update_skew();
   return true;
}
