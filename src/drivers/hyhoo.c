/******************************************************************************

	Game Driver for Nichibutsu Mahjong series.

	Taisen Quiz HYHOO
	(c)1987 Nihon Bussan Co.,Ltd.

	Taisen Quiz HYHOO 2
	(c)1987 Nihon Bussan Co.,Ltd.

	Driver by Takahiro Nogi <nogi@kt.rim.or.jp> 2000/01/28 -

******************************************************************************/
/******************************************************************************
Memo:

- Some games display "GFXROM BANK OVER!!" or "GFXROM ADDRESS OVER!!"
  in Debug build.

- Screen flip is not perfect.

******************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "vidhrdw/generic.h"
#include "nb1413m3.h"


#define	SIGNED_DAC	0		// 0:unsigned DAC, 1:signed DAC


PALETTE_INIT( hyhoo );
VIDEO_UPDATE( hyhoo );
VIDEO_START( hyhoo );

WRITE_HANDLER( hyhoo_palette_w );
void hyhoo_radrx_w(int data);
void hyhoo_radry_w(int data);
void hyhoo_sizex_w(int data);
void hyhoo_sizey_w(int data);
void hyhoo_gfxflag1_w(int data);
void hyhoo_gfxflag2_w(int data);
void hyhoo_drawx_w(int data);
void hyhoo_drawy_w(int data);
void hyhoo_romsel_w(int data);


static DRIVER_INIT( hyhoo )
{
	nb1413m3_type = NB1413M3_HYHOO;
}

static DRIVER_INIT( hyhoo2 )
{
	nb1413m3_type = NB1413M3_HYHOO2;
}


static MEMORY_READ_START( readmem_hyhoo )
	{ 0x0000, 0xefff, MRA_ROM },
	{ 0xf000, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( writemem_hyhoo )
	{ 0x0000, 0xefff, MWA_ROM },
	{ 0xf000, 0xffff, MWA_RAM, &nb1413m3_nvram, &nb1413m3_nvram_size },
MEMORY_END


static READ_HANDLER( io_hyhoo_r )
{
	offset = (((offset & 0xff00) >> 8) | ((offset & 0x00ff) << 8));

	if (offset < 0x8000) return nb1413m3_sndrom_r(offset);

	switch (offset & 0xff00)
	{
		case	0x8100:	return AY8910_read_port_0_r(0);
		case	0x9000:	return nb1413m3_inputport0_r();
		case	0xa000:	return nb1413m3_inputport1_r();
		case	0xb000:	return nb1413m3_inputport2_r();
		case	0xf000:	return nb1413m3_dipsw1_r();
		case	0xf100:	return nb1413m3_dipsw2_r();
		case	0xe000:	return nb1413m3_gfxrom_r((offset & 0x0100) >> 8);
		case	0xe100:	return nb1413m3_gfxrom_r((offset & 0x0100) >> 8);
		default:	return 0xff;
	}
}

static PORT_READ_START( readport_hyhoo )
	{ 0x0000, 0xffff, io_hyhoo_r },
PORT_END

static WRITE_HANDLER( io_hyhoo_w )
{
	offset = (((offset & 0xff00) >> 8) | ((offset & 0x00ff) << 8));

	if ((0xc000 <= offset) && (0xd000 > offset))
	{
		hyhoo_palette_w(((offset & 0x0f00) >> 8), data);
		return;
	}

	switch (offset & 0xff00)
	{
		case	0x0000:	break;
		case	0x8200:	AY8910_write_port_0_w(0, data); break;
		case	0x8300:	AY8910_control_port_0_w(0, data); break;
		case	0x9000:	hyhoo_radrx_w(data);
				nb1413m3_gfxradr_l_w(data); break;
		case	0x9100:	hyhoo_radry_w(data);
				nb1413m3_gfxradr_h_w(data); break;
		case	0x9200:	hyhoo_drawx_w(data); break;
		case	0x9300:	hyhoo_drawy_w(data); break;
		case	0x9400:	hyhoo_sizex_w(data); break;
		case	0x9500:	hyhoo_sizey_w(data); break;
		case	0x9600:	hyhoo_gfxflag1_w(data); break;
		case	0x9700:	break;
		case	0xa000:	nb1413m3_inputportsel_w(data); break;
		case	0xb000:	nb1413m3_sndrombank1_w(data); break;
#if SIGNED_DAC
		case	0xd000:	DAC_0_signed_data_w(0, data); break;
#else
		case	0xd000:	DAC_0_data_w(0, data); break;
#endif
		case	0xe000:	hyhoo_romsel_w(data);
				hyhoo_gfxflag2_w(data);
				nb1413m3_gfxrombank_w(data);
				break;
		case	0xf000:	break;
	}
}

static PORT_WRITE_START( writeport_hyhoo )
	{ 0x0000, 0xffff, io_hyhoo_w },
PORT_END


INPUT_PORTS_START( hyhoo )
	PORT_START	/* (0) DIPSW-A */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, "4 (Easy)" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x03, "1 (Hard)" )
	PORT_DIPNAME( 0x0C, 0x00, "Quiz Count" )
	PORT_DIPSETTING(    0x0C, "12" )
	PORT_DIPSETTING(    0x08, "16" )
	PORT_DIPSETTING(    0x04, "18" )
	PORT_DIPSETTING(    0x00, "20" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_5C ) )
	PORT_DIPNAME( 0x40, 0x00, "Game Sounds" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* (1) DIPSW-B */
	PORT_DIPNAME( 0x01, 0x01, "Bonus Game" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Sexy Quiz" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "Picture Quiz" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Max Players" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x20, 0x20, "Commemoration Medal Payout" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0xC0, 0xC0, "Medal Allotment Rate" )
	PORT_DIPSETTING(    0xC0, "80%" )
	PORT_DIPSETTING(    0x80, "85%" )
	PORT_DIPSETTING(    0x40, "90%" )
	PORT_DIPSETTING(    0x00, "95%" )

	PORT_START	/* (2) PORT 0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )		// DRAW BUSY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )		//
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )		// SERVICE
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE3 )		// MEMORY RESET
	PORT_SERVICE( 0x10, IP_ACTIVE_LOW )			// TEST
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )		// COIN1
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START3 )		// NOT USED
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )		// COIN2

	PORT_START	/* (3) PORT 1-0 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P1-A", KEYCODE_Z, IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P1-B", KEYCODE_X, IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P1-C", KEYCODE_C, IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P2-A", KEYCODE_V, IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P2-B", KEYCODE_B, IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, 0, "P2-C", KEYCODE_N, IP_JOY_NONE )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* (4) PORT 1-1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* (5) PORT 2-0 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P3-A", KEYCODE_A, IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P3-B", KEYCODE_S, IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P3-C", KEYCODE_D, IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P4-A", KEYCODE_F, IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P4-B", KEYCODE_G, IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, 0, "P4-C", KEYCODE_H, IP_JOY_NONE )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( hyhoo2 )
	PORT_START	/* (0) DIPSW-A */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, "4 (Easy)" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x03, "1 (Hard)" )
	PORT_DIPNAME( 0x0C, 0x0C, "Quiz Count" )
	PORT_DIPSETTING(    0x0C, "8" )
	PORT_DIPSETTING(    0x08, "10" )
	PORT_DIPSETTING(    0x04, "12" )
	PORT_DIPSETTING(    0x00, "14" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_5C ) )
	PORT_DIPNAME( 0x40, 0x00, "Game Sounds" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Sexy Quiz" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START	/* (1) DIPSW-B */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* (2) PORT 0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )		// DRAW BUSY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )		//
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )		// SERVICE
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE3 )		// MEMORY RESET
	PORT_SERVICE( 0x10, IP_ACTIVE_LOW )			// TEST
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )		// COIN1
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START3 )		// NOT USED
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )		// COIN2

	PORT_START	/* (3) PORT 1-0 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P1-A", KEYCODE_Z, IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P1-B", KEYCODE_X, IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P1-C", KEYCODE_C, IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P2-A", KEYCODE_V, IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P2-B", KEYCODE_B, IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, 0, "P2-C", KEYCODE_N, IP_JOY_NONE )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* (4) PORT 1-1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* (5) PORT 2-0 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P3-A", KEYCODE_A, IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P3-B", KEYCODE_S, IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P3-C", KEYCODE_D, IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P4-A", KEYCODE_F, IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P4-B", KEYCODE_G, IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, 0, "P4-C", KEYCODE_H, IP_JOY_NONE )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END


static struct AY8910interface ay8910_interface =
{
	1,				/* 1 chip */
	1250000,			/* 1.25 MHz ?? */
	{ 35 },
	{ input_port_0_r },		// DIPSW-A read
	{ input_port_1_r },		// DIPSW-B read
	{ 0 },
	{ 0 }
};

static struct DACinterface dac_interface =
{
	1,				/* 1 channels */
	{ 50 }
};


static MACHINE_DRIVER_START( hyhoo )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 5000000/1)	/* 2.50 MHz */
	MDRV_CPU_FLAGS(CPU_16BIT_PORT)
	MDRV_CPU_MEMORY(readmem_hyhoo,writemem_hyhoo)
	MDRV_CPU_PORTS(readport_hyhoo,writeport_hyhoo)
	MDRV_CPU_VBLANK_INT(nb1413m3_interrupt,128)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(nb1413m3)
	MDRV_NVRAM_HANDLER(nb1413m3)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_PIXEL_ASPECT_RATIO_1_2 | VIDEO_NEEDS_6BITS_PER_GUN)
	MDRV_SCREEN_SIZE(512, 256)
	MDRV_VISIBLE_AREA(0, 512-1, 15, 239-1)
	MDRV_PALETTE_LENGTH(65536)
	MDRV_COLORTABLE_LENGTH(65536)

	MDRV_PALETTE_INIT(hyhoo)
	MDRV_VIDEO_START(hyhoo)
	MDRV_VIDEO_UPDATE(hyhoo)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, ay8910_interface)
	MDRV_SOUND_ADD(DAC, dac_interface)
MACHINE_DRIVER_END


ROM_START( hyhoo )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* program */
	ROM_LOAD( "hyhoo.1",  0x00000, 0x08000, 0xc2852861 )

	ROM_REGION( 0x10000, REGION_SOUND1, 0 ) /* voice */
	ROM_LOAD( "hyhoo.2",  0x00000, 0x10000, 0x1fffcc84 )

	ROM_REGION( 0x380000, REGION_GFX1, 0 ) /* gfx */
	ROM_LOAD( "hy1506-1.1i", 0x000000, 0x80000, 0x42c9fa34 )
	ROM_LOAD( "hy1506-1.2i", 0x080000, 0x80000, 0x4c14972f )
	ROM_LOAD( "hy1506-1.3i", 0x100000, 0x80000, 0x4a18c783 )
	ROM_LOAD( "hy1506-1.4i", 0x180000, 0x80000, 0xdf26de46 )
	ROM_LOAD( "hyhoo.3",     0x280000, 0x10000, 0xb641c5a6 )
ROM_END

ROM_START( hyhoo2 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* program */
	ROM_LOAD( "hyhoo2.2",  0x00000, 0x08000, 0xd8733cdc )
	ROM_LOAD( "hyhoo2.1",  0x08000, 0x08000, 0x4a1d9493 )

	ROM_REGION( 0x10000, REGION_SOUND1, 0 ) /* voice */
	ROM_LOAD( "hyhoo2.3",  0x00000, 0x10000, 0xd7e82b23 )

	ROM_REGION( 0x380000, REGION_GFX1, 0 ) /* gfx */
	ROM_LOAD( "hy1506-1.1i", 0x000000, 0x80000, 0x42c9fa34 )
	ROM_LOAD( "hy1506-1.2i", 0x080000, 0x80000, 0x4c14972f )
	ROM_LOAD( "hy1506-1.3i", 0x100000, 0x80000, 0x4a18c783 )
	ROM_LOAD( "hy1506-1.4i", 0x180000, 0x80000, 0xdf26de46 )
	ROM_LOAD( "hyhoo2.s01",  0x200000, 0x10000, 0x20f93ff0 )
	ROM_LOAD( "hyhoo2.s02",  0x210000, 0x10000, 0x82a2b590 )
	ROM_LOAD( "hyhoo2.s03",  0x220000, 0x10000, 0xa921b5ba )
	ROM_LOAD( "hyhoo2.s04",  0x230000, 0x10000, 0xea389c82 )
	ROM_LOAD( "hyhoo2.s05",  0x240000, 0x10000, 0x89ca44fa )
	ROM_LOAD( "hyhoo2.s06",  0x250000, 0x10000, 0xf9bebf40 )
	ROM_LOAD( "hyhoo2.s07",  0x260000, 0x10000, 0x3a219376 )
	ROM_LOAD( "hyhoo2.s08",  0x270000, 0x10000, 0xac008d3f )
	ROM_LOAD( "hyhoo2.s09",  0x280000, 0x10000, 0x5b364a79 )
	ROM_LOAD( "hyhoo2.s10",  0x290000, 0x10000, 0x944b01bb )
	ROM_LOAD( "hyhoo2.s11",  0x2a0000, 0x10000, 0x5f4e455b )
	ROM_LOAD( "hyhoo2.s12",  0x2b0000, 0x10000, 0x92a07b8a )
ROM_END


GAME( 1987, hyhoo,  0, hyhoo,  hyhoo,  hyhoo,  ROT90, "Nichibutsu", "Taisen Quiz HYHOO (Japan)" )
GAME( 1987, hyhoo2, 0, hyhoo,  hyhoo2, hyhoo2, ROT90, "Nichibutsu", "Taisen Quiz HYHOO 2 (Japan)" )
