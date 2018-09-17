/* Angel Kids / Space Position hardware driver

 driver by David Haywood
 with some help from Steph (DSWs, Inputs, other
 bits here and there)

2 Board System, Uses Boards X090-PC-A & X090-PC-B

Both games appear to be joint Sega / Nasco efforts
(although all I see in Angel Kids is 'Exa Planning'
 but I think that has something to do with Nasco   )

Space Position is encrypted, the main processor is
D317-0005 (NEC Z80 Custom), see machine/segacrpt.c
for details on this encryption scheme

*/

/* started 23/01/2002 */

/* notes / todo:

Decrypt Space Position Somehow (not something I
can do)
Unknown Reads / Writes
Whats the Prom for? nothing important?
Clock Speeds etc.
Is the level order correct?
the progress sprite on the side of the screen re-appears at the bottom when you get
to the top, but the wrap-around is needed for other things, actual game bug?

*/

/* readme's

------------------------------------------------------------------------

Angel Kids
833-6599-01
Sega 1988

Nasco X090-PC-A  (Sega 837-6600)

 SW1   SW2


 8255

 8255

       11429 6116 Z80   YM2203 YM2203


 11424 11425 11426 11427  -  -  -  -  5M5165 11428  Z80
                                                         4MHz

                                                         6MHz


Nasco X090-PC-B

                                  2016-55
11437  11445    2016-55  2016-55             U5
11436  11444
11435  11443
11434  11442
11433  11441                  2016-55    2016-55
11432  11440
11431  11439    11446         2016-55

                                             11148
                                             11147
   2016-55 2016-55 2016-55

                                               18.432MHz

11430  11438

------------------------------------------------------------------------

Space Position (JPN Ver.)
(c)1986 Sega / Nasco
X090-PC-A 171-5383
X090-PC-B 171-5384


CPU :D317-0005 (NEC Z80 Custom)
Sound   :NEC D780C-1
    :YM2203C x 2
OSC :4.000MHz 6.000MHz
    :18.432MHz


EPR10120.C1 prg
EPR10121.C2  |
EPR10122.C3  |
EPR10123.C4  |
EPR10124.C5  |
EPR10125.C10    /

EPR10126.D4 snd

EPR10127.06
EPR10128.07
EPR10129.08
EPR10130.14
EPR10131.15
EPR10132.16

EPR10133.17

EPR10134.18
EPR10135.19

63S081N.U5


--- Team Japump!!! ---
http://www.rainemu.com/japump/
http://japump.i.am/
Dumped by Chackn
02/25/2000

------------------------------------------------------------------------

*/


#include "driver.h"
#include "cpu/z80/z80.h"
#include "machine/segacrpt.h"
#include "sound/2203intf.h"

static READ8_HANDLER( angelkds_main_sound_r );
static WRITE8_HANDLER( angelkds_main_sound_w );
static READ8_HANDLER( angelkds_sub_sound_r );
static WRITE8_HANDLER( angelkds_sub_sound_w );

extern UINT8 *angelkds_txvideoram, *angelkds_bgtopvideoram, *angelkds_bgbotvideoram;

WRITE8_HANDLER( angelkds_bgtopvideoram_w );
WRITE8_HANDLER( angelkds_bgbotvideoram_w );
WRITE8_HANDLER( angelkds_txvideoram_w );

WRITE8_HANDLER( angelkds_bgtopbank_write );
WRITE8_HANDLER( angelkds_bgtopscroll_write );
WRITE8_HANDLER( angelkds_bgbotbank_write );
WRITE8_HANDLER( angelkds_bgbotscroll_write );
WRITE8_HANDLER( angelkds_txbank_write );

WRITE8_HANDLER( angelkds_paletteram_w );
WRITE8_HANDLER( angelkds_layer_ctrl_write );

VIDEO_START( angelkds );
VIDEO_UPDATE( angelkds );

/*** CPU Banking

*/

static WRITE8_HANDLER ( angelkds_cpu_bank_write )
{
	int bankaddress;
	UINT8 *RAM = memory_region(REGION_USER1);

	bankaddress = data & 0x0f;
	memory_set_bankptr(1,&RAM[bankaddress*0x4000]);
}


/*** Fake Inputs

these make the game a bit easier for testing purposes

*/

#define FAKEINPUTS 0

#if FAKEINPUTS

static READ8_HANDLER( angelkds_input_r )
{
	int fake = readinputport(6+offset);

	return ((fake & 0x01) ? fake  : readinputport(4+offset));
}

#else

static READ8_HANDLER( angelkds_input_r )
{
	return readinputport(4+offset);
}

#endif

/*** Memory Structures

Angel Kids:
I would have expected f003 to be the scroll register for the bottom
part of the screen, in the attract mode this works fine, but in the
game it doesn't, so maybe it wasn't really hooked up and instead
only one of the register (f001) is used for both part?

* update, it is correct, the screen is meant to split in two when
 the kid goes what would be offscreen, just looked kinda odd

Interesting note, each Bank in the 0x8000 - 0xbfff appears to
contain a level.



*/

static ADDRESS_MAP_START( readmem_main, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x8000, 0xbfff) AM_READ(MRA8_BANK1)
	AM_RANGE(0xc000, 0xdfff) AM_READ(MRA8_RAM)
	AM_RANGE(0xe000, 0xefff) AM_READ(MRA8_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem_main, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0xc000, 0xdfff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0xe000, 0xe3ff) AM_WRITE(angelkds_bgtopvideoram_w) AM_BASE(&angelkds_bgtopvideoram) /* Top Half of Screen */
	AM_RANGE(0xe400, 0xe7ff) AM_WRITE(angelkds_bgbotvideoram_w) AM_BASE(&angelkds_bgbotvideoram) /* Bottom Half of Screen */
	AM_RANGE(0xe800, 0xebff) AM_WRITE(angelkds_txvideoram_w) AM_BASE(&angelkds_txvideoram)
	AM_RANGE(0xec00, 0xecff) AM_WRITE(MWA8_RAM) AM_BASE(&spriteram)
	AM_RANGE(0xed00, 0xeeff) AM_WRITE(angelkds_paletteram_w) AM_BASE(&paletteram)
	AM_RANGE(0xf000, 0xf000) AM_WRITE(angelkds_bgtopbank_write)
	AM_RANGE(0xf001, 0xf001) AM_WRITE(angelkds_bgtopscroll_write)
	AM_RANGE(0xf002, 0xf002) AM_WRITE(angelkds_bgbotbank_write)
	AM_RANGE(0xf003, 0xf003) AM_WRITE(angelkds_bgbotscroll_write)
	AM_RANGE(0xf004, 0xf004) AM_WRITE(angelkds_txbank_write)
	AM_RANGE(0xf005, 0xf005) AM_WRITE(angelkds_layer_ctrl_write)
ADDRESS_MAP_END

static ADDRESS_MAP_START( readport_main, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x40, 0x40) AM_READ(input_port_0_r)	/* "Coinage" Dip Switches */
	AM_RANGE(0x41, 0x41) AM_READ(input_port_1_r)	/* Other Dip Switches */
	AM_RANGE(0x42, 0x42) AM_READ(input_port_2_r)	/* Players inputs (not needed ?) */
	AM_RANGE(0x80, 0x80) AM_READ(input_port_3_r)	/* System inputs */
	AM_RANGE(0x81, 0x82) AM_READ(angelkds_input_r)	/* Players inputs */
	AM_RANGE(0xc0, 0xc3) AM_READ(angelkds_main_sound_r)  /* needed by spcpostn */
ADDRESS_MAP_END

static ADDRESS_MAP_START( writeport_main, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_WRITE(MWA8_NOP) // 00 on start-up, not again
	AM_RANGE(0x42, 0x42) AM_WRITE(angelkds_cpu_bank_write)
	AM_RANGE(0x43, 0x43) AM_WRITE(MWA8_NOP) // 9a on start-up, not again
	AM_RANGE(0x83, 0x83) AM_WRITE(MWA8_NOP) // 9b on start-up, not again
	AM_RANGE(0xc0, 0xc3) AM_WRITE(angelkds_main_sound_w) // 02 various points
ADDRESS_MAP_END

/* sub cpu */

static ADDRESS_MAP_START( readmem_sub, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x8000, 0x87ff) AM_READ(MRA8_RAM)
	AM_RANGE(0xaaa9, 0xaaa9) AM_READ(MRA8_NOP)
	AM_RANGE(0xaaab, 0xaaab) AM_READ(MRA8_NOP)
	AM_RANGE(0xaaac, 0xaaac) AM_READ(MRA8_NOP)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem_sub, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x8000, 0x87ff) AM_WRITE(MWA8_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( readport_sub, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_READ(YM2203_status_port_0_r)
	AM_RANGE(0x40, 0x40) AM_READ(YM2203_status_port_1_r)
	AM_RANGE(0x80, 0x83) AM_READ(angelkds_sub_sound_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writeport_sub, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_WRITE(YM2203_control_port_0_w)
	AM_RANGE(0x01, 0x01) AM_WRITE(YM2203_write_port_0_w)
	AM_RANGE(0x40, 0x40) AM_WRITE(YM2203_control_port_1_w)
	AM_RANGE(0x41, 0x41) AM_WRITE(YM2203_write_port_1_w)
	AM_RANGE(0x80, 0x83) AM_WRITE(angelkds_sub_sound_w) // spcpostn
ADDRESS_MAP_END


/* Input Ports */

/* Here is the way to access to the different parts of the "test mode" :

     - sound  : set "Coin A" Dip Switch to "Free Play" and "Coin B" Dip Switch to "Free Play"
     - paddle : set "Coin A" Dip Switch to "3C_1C" and "Coin B" Dip Switch to "Free Play"

If use different settings, you'll only see a black screen.

I haven't found how to exit the tests. The only way seems to reset the game.
*/

#define ANGELDSK_PLAYERS_INPUT( player ) \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_UP ) PORT_PLAYER(player) PORT_8WAY \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_DOWN ) PORT_PLAYER(player) PORT_8WAY \
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_LEFT ) PORT_PLAYER(player) PORT_8WAY \
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_RIGHT ) PORT_PLAYER(player) PORT_8WAY \
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_UP ) PORT_PLAYER(player) PORT_8WAY \
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_DOWN ) PORT_PLAYER(player) PORT_8WAY \
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_LEFT ) PORT_PLAYER(player) PORT_8WAY \
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_RIGHT ) PORT_PLAYER(player) PORT_8WAY

#define ANGELDSK_FAKE_PLAYERS_INPUT( player ) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(player)	/* To enter initials */ \
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )		/* Unused */ \
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(player) PORT_8WAY \
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(player) PORT_8WAY \
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(player) PORT_8WAY \
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(player) PORT_8WAY \
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(player)	/* To shorten the rope and */ \
										/* move right in hiscores table */


static INPUT_PORTS_START( angelkds )
	PORT_START_TAG("I40")		/* inport $40 */
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(	0x70, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(	0x80, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(	0x90, DEF_STR( 2C_1C ) )
//  PORT_DIPSETTING(    0x60, DEF_STR( 2C_1C ) )
//  PORT_DIPSETTING(    0x50, DEF_STR( 2C_1C ) )
//  PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0xf0, DEF_STR( 1C_1C ) )
//  PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
//  PORT_DIPSETTING(    0x20, DEF_STR( 1C_1C ) )
//  PORT_DIPSETTING(    0x10, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0xe0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	0xd0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(	0xc0, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(	0xb0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(	0xa0, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Free_Play ) )	// needed to enter "test mode" (see above)

	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(	0x07, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(	0x08, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(	0x09, DEF_STR( 2C_1C ) )
//  PORT_DIPSETTING(    0x06, DEF_STR( 2C_1C ) )
//  PORT_DIPSETTING(    0x05, DEF_STR( 2C_1C ) )
//  PORT_DIPSETTING(    0x04, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x0f, DEF_STR( 1C_1C ) )
//  PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
//  PORT_DIPSETTING(    0x02, DEF_STR( 1C_1C ) )
//  PORT_DIPSETTING(    0x01, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x0e, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	0x0d, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(	0x0c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(	0x0b, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(	0x0a, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Free_Play ) )

	PORT_START_TAG("I41")		/* inport $41 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x00, "Hi Score" )
	PORT_DIPSETTING(    0x00, "3 Characters" )
	PORT_DIPSETTING(    0x02, "10 Characters" )
	PORT_DIPNAME( 0x0c, 0x08, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x0c, "20k, 50k, 100k, 200k and 500k" )
	PORT_DIPSETTING(    0x08, "50k, 100k, 200k and 500k" )
	PORT_DIPSETTING(    0x04, "100k, 200k and 500k" )
	PORT_DIPSETTING(    0x00, DEF_STR( None ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPSETTING(    0x00, "99 (Cheat)" )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Difficulty ) )	/* Stored at 0xc023 */
	PORT_DIPSETTING(    0xc0, DEF_STR( Very_Easy ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Very_Hard ) )

	PORT_START_TAG("I42")		/* inport $42 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )	// duplicated IPT_JOYSTICK_LEFTRIGHT
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN ) PORT_8WAY	// duplicated IPT_JOYSTICK_LEFTRIGHT
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("I80")		/* inport $80 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START_TAG("I81")		/* inport $81 */
	ANGELDSK_PLAYERS_INPUT( 1 )

	PORT_START_TAG("I82")		/* inport $82 */
	ANGELDSK_PLAYERS_INPUT( 2 )

#if FAKEINPUTS

	/* Fake inputs to allow to play the game with 1 joystick instead of 2 */
	PORT_START_TAG("FAKE1")
	PORT_DIPNAME( 0x01, 0x00, "FAKE (for debug) Joysticks (Player 1)" )
	PORT_DIPSETTING(	0x01, "1" )
	PORT_DIPSETTING(	0x00, "2" )
	ANGELDSK_FAKE_PLAYERS_INPUT( 1 )

	PORT_START_TAG("FAKE2")
	PORT_DIPNAME( 0x01, 0x00, "FAKE (for debug) Joysticks (Player 2)" )
	PORT_DIPSETTING(	0x01, "1" )
	PORT_DIPSETTING(	0x00, "2" )
	ANGELDSK_FAKE_PLAYERS_INPUT( 2 )

#endif

INPUT_PORTS_END

static INPUT_PORTS_START( spcpostn )
	PORT_START_TAG("I40")		/* inport $40 */
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 1C_7C ) )

	PORT_START_TAG("I41")		/* inport $41 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR(Allow_Continue ) )
	PORT_DIPSETTING(    0x01, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x02, 0x02, "Obstruction Car" )
	PORT_DIPSETTING(    0x02, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hard ) )
	PORT_DIPNAME( 0x0c, 0x08, "Time Limit" )
	PORT_DIPSETTING(    0x00, "1:10" )
	PORT_DIPSETTING(    0x04, "1:20" )
	PORT_DIPSETTING(    0x08, "1:30" )
	PORT_DIPSETTING(    0x0c, "1:40" )
	PORT_DIPNAME( 0x30, 0x20, "Power Down" )
	PORT_DIPSETTING(    0x30, "Slow" )
	PORT_DIPSETTING(    0x20, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x10, "Fast" )
	PORT_DIPSETTING(    0x00, "Fastest" )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START_TAG("I42")		/* inport $42 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("I80")		/* inport $80 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START_TAG("I81")		/* inport $81 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )    PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )  PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )  PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED ) PORT_PLAYER(1) // probably unused
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED ) PORT_PLAYER(1) // probably unused

	PORT_START_TAG("I82")		/* inport $82 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )    PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )  PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )  PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED ) PORT_PLAYER(2) // probably unused
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED ) PORT_PLAYER(2) // probably unused

INPUT_PORTS_END

/*** Sound Hardware

todo: verify / correct things
seems a bit strange are all the addresses really
sound related ?

*/

static UINT8 angelkds_sound[4];
static UINT8 angelkds_sound2[4];

static WRITE8_HANDLER( angelkds_main_sound_w )
{
	angelkds_sound[offset]=data;
}

static READ8_HANDLER( angelkds_main_sound_r )
{
	return angelkds_sound2[offset];
}

static WRITE8_HANDLER( angelkds_sub_sound_w )
{
	angelkds_sound2[offset]=data;
}

static READ8_HANDLER( angelkds_sub_sound_r )
{
	return angelkds_sound[offset];
}


static void irqhandler(int irq)
{
	cpunum_set_input_line(1,0,irq ? ASSERT_LINE : CLEAR_LINE);
}

static const struct YM2203interface ym2203_interface =
{
	0,
	0,
	0,
	0,
	irqhandler
};

/*** Graphics Decoding

all the 8x8 tiles are in one format, the 16x16 sprites in another

*/

static const gfx_layout angelkds_charlayout =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ 0,1,2,3 },
	{ 0, 4, 8, 12, 16, 20, 24, 28 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	8*32
};


static const gfx_layout angelkds_spritelayout =
{
	16,16,
	RGN_FRAC(1,2),
	4,
	{ 0,4,	RGN_FRAC(1,2)+0,	RGN_FRAC(1,2)+4 },
	{ 0, 1, 2, 3, 8, 9, 10, 11, 16,17,18,19, 24,25,26,27 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32, 8*32, 9*32, 10*32, 11*32, 12*32, 13*32, 14*32, 15*32  },
	16*32
};

static GFXDECODE_START( angelkds )
	GFXDECODE_ENTRY( REGION_GFX1, 0, angelkds_charlayout,   0x30, 1  )
	GFXDECODE_ENTRY( REGION_GFX3, 0, angelkds_charlayout,   0, 16 )
	GFXDECODE_ENTRY( REGION_GFX4, 0, angelkds_charlayout,   0, 16 )
	GFXDECODE_ENTRY( REGION_GFX2, 0, angelkds_spritelayout, 0x20, 0x0d )
GFXDECODE_END

/*** Machine Driver

 2 x z80 (one for game, one for sound)
 2 x YM2203 (for sound)

 all fairly straightforward

*/

static MACHINE_DRIVER_START( angelkds )
	MDRV_CPU_ADD(Z80, 8000000) /* 8MHz? 6 seems too slow? */
	MDRV_CPU_PROGRAM_MAP(readmem_main,writemem_main)
	MDRV_CPU_IO_MAP(readport_main,writeport_main)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80, 4000000) /* 8 MHz? */
	/* audio CPU */
	MDRV_CPU_PROGRAM_MAP(readmem_sub,writemem_sub)
	MDRV_CPU_IO_MAP(readport_sub,writeport_sub)

	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_SCREEN_VISIBLE_AREA(0*8, 32*8-1, 1*8, 31*8-1)
	MDRV_GFXDECODE(angelkds)
	MDRV_PALETTE_LENGTH(0x100)

	MDRV_VIDEO_START(angelkds)
	MDRV_VIDEO_UPDATE(angelkds)

	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(YM2203, 4000000)
	MDRV_SOUND_CONFIG(ym2203_interface)
	MDRV_SOUND_ROUTE(0, "mono", 0.65)
	MDRV_SOUND_ROUTE(1, "mono", 0.65)
	MDRV_SOUND_ROUTE(2, "mono", 0.65)
	MDRV_SOUND_ROUTE(3, "mono", 0.45)

	MDRV_SOUND_ADD(YM2203, 4000000)
	MDRV_SOUND_ROUTE(0, "mono", 0.65)
	MDRV_SOUND_ROUTE(1, "mono", 0.65)
	MDRV_SOUND_ROUTE(2, "mono", 0.65)
	MDRV_SOUND_ROUTE(3, "mono", 0.45)
MACHINE_DRIVER_END

/*** Rom Loading

 REGION_CPU1 for the main code
 REGION_USER1 for the banked data
 REGION_CPU2 for the sound cpu code
 REGION_GFX1 for the 8x8 Txt Layer Tiles
 REGION_GFX2 for the 16x16 Sprites
 REGION_GFX3 for the 8x8 Bg Layer Tiles (top tilemap)
 REGION_GFX4 for the 8x8 Bg Layer Tiles (bottom tilemap)
 REGION_PROMS for the Prom (same between games)

*/

ROM_START( angelkds )
	/* Nasco X090-PC-A  (Sega 837-6600) */
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "11428.c10",    0x00000, 0x08000, CRC(90daacd2) SHA1(7e50ad1cbed0c1e6bad04ef1611cad25538c905f) )

	ROM_REGION( 0x20000, REGION_USER1, 0 ) /* Banked Code */
	ROM_LOAD( "11424.c1",     0x00000, 0x08000, CRC(b55997f6) SHA1(7ed746becac1851f39591f1fdbeff64aa97d6206) )
	ROM_LOAD( "11425.c2",     0x08000, 0x08000, CRC(299359de) SHA1(f531dd3bfe6f64e9e043cb4f85d5657455241dc7) )
	ROM_LOAD( "11426.c3",     0x10000, 0x08000, CRC(5fad8bd3) SHA1(4d865342eb10dcfb779eee4ac1e159bb9ec140cb) )
	ROM_LOAD( "11427.c4",     0x18000, 0x08000, CRC(ef920c74) SHA1(81c0fbe4ace5441e4cd99ba423e0190cc541da31) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "11429.d4",     0x00000, 0x08000, CRC(0ca50a66) SHA1(cccb081b447419138b1ebd309e7f291e392a44d5) )

	/* Nasco X090-PC-B */
	ROM_REGION( 0x08000, REGION_GFX1, 0 )
	ROM_LOAD( "11446",        0x00000, 0x08000, CRC(45052470) SHA1(c2312a9f814d6dbe42aa465147a04a2bd9b2aa1b) )

	ROM_REGION( 0x10000, REGION_GFX2, 0 )
	ROM_LOAD( "11447.f7",     0x08000, 0x08000, CRC(b3afc5b3) SHA1(376d527f60e9044f18d19a5535bca77606efbd4c) )
	ROM_LOAD( "11448.h7",     0x00000, 0x08000, CRC(05dab626) SHA1(73feaca6e23c673a7d8c9e972714b20bd8f2d51e) )

	/* both tilemaps on angelkds use the same gfx */
	ROM_REGION( 0x40000, REGION_GFX3, 0 )
	ROM_LOAD( "11437",        0x00000, 0x08000, CRC(a520b628) SHA1(2b51f59e760e740e5e6b06dad61bbc23fc84a72b) )
	ROM_LOAD( "11436",        0x08000, 0x08000, CRC(469ab216) SHA1(8223f072a6f9135ff84841c95410368bcea073d8) )
	ROM_LOAD( "11435",        0x10000, 0x08000, CRC(b0f8c245) SHA1(882e27eaceac46c397fdae8427a082caa7d6b7dc) )
	ROM_LOAD( "11434",        0x18000, 0x08000, CRC(cbde81f5) SHA1(5d5b8e709c9dd09a45dfced6f3d4a9c52500da6b) )
	ROM_LOAD( "11433",        0x20000, 0x08000, CRC(b63fa414) SHA1(25adcafd7e17ab0be0fed2ec44245124febd74b3) )
	ROM_LOAD( "11432",        0x28000, 0x08000, CRC(00dc747b) SHA1(041b73aa48b45162af33b5f416ccc0c0dbbd995b) )
	ROM_LOAD( "11431",        0x30000, 0x08000, CRC(ac2025af) SHA1(2aba145df3ccdb1a7f0fec524bd2de3f9aab4161) )
	ROM_LOAD( "11430",        0x38000, 0x08000, CRC(d640f89e) SHA1(38fb67bcb2a3d1ad614fc62e42f22a66bc757137) )

	ROM_REGION( 0x40000, REGION_GFX4, 0 )
	ROM_LOAD( "11445",        0x00000, 0x08000, CRC(a520b628) SHA1(2b51f59e760e740e5e6b06dad61bbc23fc84a72b) )
	ROM_LOAD( "11444",        0x08000, 0x08000, CRC(469ab216) SHA1(8223f072a6f9135ff84841c95410368bcea073d8) )
	ROM_LOAD( "11443",        0x10000, 0x08000, CRC(b0f8c245) SHA1(882e27eaceac46c397fdae8427a082caa7d6b7dc) )
	ROM_LOAD( "11442",        0x18000, 0x08000, CRC(cbde81f5) SHA1(5d5b8e709c9dd09a45dfced6f3d4a9c52500da6b) )
	ROM_LOAD( "11441",        0x20000, 0x08000, CRC(b63fa414) SHA1(25adcafd7e17ab0be0fed2ec44245124febd74b3) )
	ROM_LOAD( "11440",        0x28000, 0x08000, CRC(00dc747b) SHA1(041b73aa48b45162af33b5f416ccc0c0dbbd995b) )
	ROM_LOAD( "11439",        0x30000, 0x08000, CRC(ac2025af) SHA1(2aba145df3ccdb1a7f0fec524bd2de3f9aab4161) )
	ROM_LOAD( "11438",        0x38000, 0x08000, CRC(d640f89e) SHA1(38fb67bcb2a3d1ad614fc62e42f22a66bc757137) )

	ROM_REGION( 0x20, REGION_PROMS, 0 )
	ROM_LOAD( "63s081n.u5",	  0x00,    0x20,    CRC(36b98627) SHA1(d2d54d92d1d47e7cc85104989ee421ce5d80a42a) )
ROM_END

ROM_START( spcpostn )
	/* X090-PC-A 171-5383 */
	ROM_REGION( 2*0x10000, REGION_CPU1, 0 ) /* D317-0005 (NEC Z80 Custom) */
	ROM_LOAD( "epr10125.c10", 0x00000, 0x08000, CRC(bffd38c6) SHA1(af02907124343ddecd21439d25f1ebb81ef9f51a) ) /* encrypted */

	ROM_REGION( 0x28000, REGION_USER1, 0 ) /* Banked Code */
	ROM_LOAD( "epr10120.c1",  0x00000, 0x08000, CRC(d6399f99) SHA1(4c7d19a8798e5a10b688bf793ca74f5170fd9b51) )
	ROM_LOAD( "epr10121.c2",  0x08000, 0x08000, CRC(d4861560) SHA1(74d28c36a08880abbd3c398cc3e990e8986caccb) )
	ROM_LOAD( "epr10122.c3",  0x10000, 0x08000, CRC(7a1bff1b) SHA1(e1bda8430fd632c1813dd78e0f210a358e1b0d2f) )
	ROM_LOAD( "epr10123.c4",  0x18000, 0x08000, CRC(6aed2925) SHA1(75848c8086c460b72494da2367f592d7d5dcf9f1) )
	ROM_LOAD( "epr10124.c5",  0x20000, 0x08000, CRC(a1d7ae6b) SHA1(ec81fecf63e0515cae2077e2623262227adfdf37) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* NEC D780C-1 */
	ROM_LOAD( "epr10126.d4",  0x00000, 0x08000, CRC(ab17f852) SHA1(dc0db427ddb4df97bb40dfb6fc65cb9354a6b9ad) )

	/* X090-PC-B 171-5384 */
	ROM_REGION( 0x08000, REGION_GFX1, 0 )
	ROM_LOAD( "epr10133.17",  0x00000, 0x08000, CRC(642e6609) SHA1(2dfb4cc66f89543b55ed2a5b914e2c9304e821ca) )

	ROM_REGION( 0x10000, REGION_GFX2, 0 )
	ROM_LOAD( "epr10134.18",  0x08000, 0x08000, CRC(c674ff88) SHA1(9f240910a1ffb7c9e09d2326de280e6a5dd84565) )
	ROM_LOAD( "epr10135.19",  0x00000, 0x08000, CRC(0685c4fa) SHA1(6950d9ad9ec13236cf24e83e87adb62aa53af7bb) )

	ROM_REGION( 0x30000, REGION_GFX3, 0 )
	ROM_LOAD( "epr10130.14",  0x10000, 0x08000, CRC(b68fcb36) SHA1(3943dd550b13f2911d56d8dad675410da79196e6) )
	ROM_LOAD( "epr10131.15",  0x08000, 0x08000, CRC(de223817) SHA1(1860db0a19c926fcfaabe676cb57fff38c4df8e6) )
	ROM_LOAD( "epr10132.16",  0x00000, 0x08000, CRC(2df8b1bd) SHA1(cad8befa3f2c158d2aa74073066ccd2b54e68825) )

	ROM_REGION( 0x18000, REGION_GFX4, 0 )
	ROM_LOAD( "epr10127.06",  0x10000, 0x08000, CRC(b68fcb36) SHA1(3943dd550b13f2911d56d8dad675410da79196e6) )
	ROM_LOAD( "epr10128.07",  0x08000, 0x08000, CRC(de223817) SHA1(1860db0a19c926fcfaabe676cb57fff38c4df8e6) )
	ROM_LOAD( "epr10129.08",  0x00000, 0x08000, CRC(a6f21023) SHA1(8d573446a2d3d3428409707d0c59b118d1463131) )

	ROM_REGION( 0x20, REGION_PROMS, 0 )
	ROM_LOAD( "63s081n.u5",   0x00,    0x20,    CRC(36b98627) SHA1(d2d54d92d1d47e7cc85104989ee421ce5d80a42a) )
ROM_END


static DRIVER_INIT( spcpostn )	{ spcpostn_decode(); }


GAME( 1988, angelkds, 0, angelkds, angelkds,        0,  ROT90,  "Sega / Nasco?", "Angel Kids (Japan)" , 0) /* Nasco not displayed but 'Exa Planning' is */
GAME( 1986, spcpostn, 0, angelkds, spcpostn, spcpostn,  ROT90,  "Sega / Nasco", "Space Position (Japan)" , 0) /* encrypted */
