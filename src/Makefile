ROTT ?= rott

# Regular build flags and rules

CC ?= gcc

CFLAGS ?= -g -O2
CFLAGS += -Wall -Wno-unused
CFLAGS += $(EXTRACFLAGS)

CPPFLAGS += -DPLATFORM_WIN32=1
CPPFLAGS += $(EXTRACPPFLAGS)

LDFLAGS += $(EXTRALDFLAGS)

LDLIBS += -lmingw32 -lSDL2main -lSDL2 -lSDL2_mixer -lSDL_net
LDLIBS += $(EXTRALDLIBS)

OBJS :=
OBJS += au_dsl.o
OBJS += au_fxman.o
OBJS += au_llman.o
OBJS += au_music.o
OBJS += au_mvmix.o
OBJS += au_mvoc.o
OBJS += au_mvrvb.o
OBJS += au_ndpmi.o
OBJS += au_pitch.o
OBJS += au_usrhk.o
OBJS += byteordr.o
OBJS += cin_actr.o
OBJS += cin_efct.o
OBJS += cin_evnt.o
OBJS += cin_glob.o
OBJS += cin_main.o
OBJS += cin_util.o
OBJS += dosutil.o
OBJS += engine.o
OBJS += isr.o
OBJS += modexlib.o
OBJS += rt_actor.o
OBJS += rt_battl.o
OBJS += rt_build.o
OBJS += rt_cfg.o
OBJS += rt_com.o
OBJS += rt_crc.o
OBJS += rt_debug.o
OBJS += rt_dmand.o
OBJS += rt_door.o
OBJS += rt_draw.o
OBJS += rt_floor.o
OBJS += rt_game.o
OBJS += rt_in.o
OBJS += rt_main.o
OBJS += rt_map.o
OBJS += rt_menu.o
OBJS += rt_msg.o
OBJS += rt_net.o
OBJS += rt_playr.o
OBJS += rt_rand.o
OBJS += rt_scale.o
OBJS += rt_sound.o
OBJS += rt_sqrt.o
OBJS += rt_stat.o
OBJS += rt_state.o
OBJS += rt_str.o
OBJS += rt_ted.o
OBJS += rt_util.o
OBJS += rt_vid.o
OBJS += rt_view.o
OBJS += scriplib.o
OBJS += w_wad.o
OBJS += watcom.o
OBJS += z_zone.o

all: $(ROTT)

$(ROTT): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@

clean:
	$(RM) $(ROTT) $(ROTT).exe
