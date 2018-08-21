/*##########################################################################

  macblittersPB.S

  Fast blitters for various occasions. Originally written by Aaron Giles,
  mods by Brad Oliver and Raphael Nabet.

  Adapted to MacOS X c preprocessor + assembler by Raphael Nabet

  This file should be preprocessed with "cc -E", not cpp.  cpp (even with the
  -ansi option set!) is a traditional preprocessor, which does not conform to
  ANSI.

##########################################################################*/

#ifndef BLITTER_RECURSIVE_INCLUDE

#define TARGET_RT_MAC_MACHO 1


#define concat7(a, b, c, d, e, f, g) a##b##c##d##e##f##g

#define EXPAND_PIXEL_L_NAME(horizontal_expand, src_bits_per_pixel, dst_bits_per_pixel)	concat7(EXPAND_PIXEL_, horizontal_expand, x_, src_bits_per_pixel, TO, dst_bits_per_pixel, L)
#define EXPAND_PIXELS_L_NAME(horizontal_expand, src_bits_per_pixel, dst_bits_per_pixel)	concat7(EXPAND_PIXELS_, horizontal_expand, x_, src_bits_per_pixel, TO, dst_bits_per_pixel, L)

#define EXPAND_PIXEL_L_VEC_NAME(horizontal_expand, src_bits_per_pixel, dst_bits_per_pixel)	concat7(EXPAND_PIXEL_, horizontal_expand, x_, src_bits_per_pixel, TO, dst_bits_per_pixel, L_VEC)
#define EXPAND_PIXELS_L_VEC_NAME(horizontal_expand, src_bits_per_pixel, dst_bits_per_pixel)	concat7(EXPAND_PIXELS_, horizontal_expand, x_, src_bits_per_pixel, TO, dst_bits_per_pixel, L_VEC)

#define EXPAND_PIXEL_D_NAME(horizontal_expand, src_bits_per_pixel, dst_bits_per_pixel)	concat7(EXPAND_PIXEL_, horizontal_expand, x_, src_bits_per_pixel, TO, dst_bits_per_pixel, D)
#define EXPAND_PIXELS_D_NAME(horizontal_expand, src_bits_per_pixel, dst_bits_per_pixel)	concat7(EXPAND_PIXELS_, horizontal_expand, x_, src_bits_per_pixel, TO, dst_bits_per_pixel, D)

#define EXPAND_PIXEL_D_VEC_NAME(horizontal_expand, src_bits_per_pixel, dst_bits_per_pixel)	concat7(EXPAND_PIXEL_, horizontal_expand, x_, src_bits_per_pixel, TO, dst_bits_per_pixel, D_VEC)
#define EXPAND_PIXELS_D_VEC_NAME(horizontal_expand, src_bits_per_pixel, dst_bits_per_pixel)	concat7(EXPAND_PIXELS_, horizontal_expand, x_, src_bits_per_pixel, TO, dst_bits_per_pixel, D_VEC)

#define EXPAND_PIXEL_L_SWAPXY_NAME(horizontal_expand, src_bits_per_pixel, dst_bits_per_pixel)	concat7(EXPAND_PIXEL_, horizontal_expand, x_, src_bits_per_pixel, TO, dst_bits_per_pixel, L_SWAPXY)
#define EXPAND_PIXELS_L_SWAPXY_NAME(horizontal_expand, src_bits_per_pixel, dst_bits_per_pixel)	concat7(EXPAND_PIXELS_, horizontal_expand, x_, src_bits_per_pixel, TO, dst_bits_per_pixel, L_SWAPXY)

#define EXPAND_PIXEL_L_SWAPXY_VEC_NAME(horizontal_expand, src_bits_per_pixel, dst_bits_per_pixel)	concat7(EXPAND_PIXEL_, horizontal_expand, x_, src_bits_per_pixel, TO, dst_bits_per_pixel, L_SWAPXY_VEC)
#define EXPAND_PIXELS_L_SWAPXY_VEC_NAME(horizontal_expand, src_bits_per_pixel, dst_bits_per_pixel)	concat7(EXPAND_PIXELS_, horizontal_expand, x_, src_bits_per_pixel, TO, dst_bits_per_pixel, L_SWAPXY_VEC)

#define EXPAND_PIXEL_D_SWAPXY_NAME(horizontal_expand, src_bits_per_pixel, dst_bits_per_pixel)	concat7(EXPAND_PIXEL_, horizontal_expand, x_, src_bits_per_pixel, TO, dst_bits_per_pixel, D_SWAPXY)
#define EXPAND_PIXELS_D_SWAPXY_NAME(horizontal_expand, src_bits_per_pixel, dst_bits_per_pixel)	concat7(EXPAND_PIXELS_, horizontal_expand, x_, src_bits_per_pixel, TO, dst_bits_per_pixel, D_SWAPXY)

#define EXPAND_PIXEL_D_SWAPXY_VEC_NAME(horizontal_expand, src_bits_per_pixel, dst_bits_per_pixel)	concat7(EXPAND_PIXEL_, horizontal_expand, x_, src_bits_per_pixel, TO, dst_bits_per_pixel, D_SWAPXY_VEC)
#define EXPAND_PIXELS_D_SWAPXY_VEC_NAME(horizontal_expand, src_bits_per_pixel, dst_bits_per_pixel)	concat7(EXPAND_PIXELS_, horizontal_expand, x_, src_bits_per_pixel, TO, dst_bits_per_pixel, D_SWAPXY_VEC)



#pragma altivec_model on

#define VRSAVE_VALUE_1x_16TO16L 0xfbfc0000
#define VRSAVE_VALUE_2x_16TO16L 0xfbfc0000
#define VRSAVE_VALUE_3x_16TO16L 0xfffc0000

#define VRSAVE_VALUE_1x_16TO32L 0xfbff0000
#define VRSAVE_VALUE_2x_16TO32L 0xfbff0000
#define VRSAVE_VALUE_3x_16TO32L 0xffff0000

#define VRSAVE_VALUE_1x 0xe0000000
#define VRSAVE_VALUE_2x 0xe0000000
#define VRSAVE_VALUE_3x 0xfc000000

#define VRSAVE_VALUE_1x_FLIPX 0xe2000000
#define VRSAVE_VALUE_2x_FLIPX 0xf8000000
#define VRSAVE_VALUE_3x_FLIPX 0xfc000000


#pragma mark Constants

.const

.align 4

/*
	Tables for doing the table lookup with the vector unit and gain a few
	precious cycles each iteration.
*/
offsetPerm_16to16:
	.byte	0x01,	0x01,	0x03,	0x03,	0x05,	0x05,	0x07,	0x07
	.byte	0x09,	0x09,	0x0B,	0x0B,	0x0D,	0x0D,	0x0F,	0x0F

offsetPerm_16to16_fx:
	.byte	0x0F,	0x0F,	0x0D,	0x0D,	0x0B,	0x0B,	0x09,	0x09
	.byte	0x07,	0x07,	0x05,	0x05,	0x03,	0x03,	0x01,	0x01

offsetPerm_16to32:
	.byte	0x01,	0x01,	0x01,	0x01,	0x03,	0x03,	0x03,	0x03
	.byte	0x05,	0x05,	0x05,	0x05,	0x07,	0x07,	0x07,	0x07
	.byte	0x09,	0x09,	0x09,	0x09,	0x0B,	0x0B,	0x0B,	0x0B
	.byte	0x0D,	0x0D,	0x0D,	0x0D,	0x0F,	0x0F,	0x0F,	0x0F

offsetPerm_16to32_fx:
	.byte	0x0F,	0x0F,	0x0F,	0x0F,	0x0D,	0x0D,	0x0D,	0x0D
	.byte	0x0B,	0x0B,	0x0B,	0x0B,	0x09,	0x09,	0x09,	0x09
	.byte	0x07,	0x07,	0x07,	0x07,	0x05,	0x05,	0x05,	0x05
	.byte	0x03,	0x03,	0x03,	0x03,	0x01,	0x01,	0x01,	0x01

/*offsetPerm_32to32:
	.byte	0x03,	0x03,	0x03,	0x03,	0x07,	0x07,	0x07,	0x07
	.byte	0x0B,	0x0B,	0x0B,	0x0B,	0x0F,	0x0F,	0x0F,	0x0F

offsetPerm_32to32_fx:
	.byte	0x0F,	0x0F,	0x0F,	0x0F,	0x0B,	0x0B,	0x0B,	0x0B
	.byte	0x07,	0x07,	0x07,	0x07,	0x03,	0x03,	0x03,	0x03*/

offsetOffset_dst16:
	.byte	0x00,	0x01,	0x10,	0x11,	0x00,	0x01,	0x10,	0x11
	.byte	0x00,	0x01,	0x10,	0x11,	0x00,	0x01,	0x10,	0x11

offsetOffset_dst32:
	.byte	0x00,	0x01,	0x02,	0x03,	0x10,	0x11,	0x12,	0x13
	.byte	0x00,	0x01,	0x02,	0x03,	0x10,	0x11,	0x12,	0x13

pixelMerge1:
	.short	0x0000,	0x0000,	0xFFFF,	0xFFFF,	0x0000,	0x0000,	0xFFFF,	0xFFFF

pixelMerge2:
	.short	0x0000,	0x0000,	0x0000,	0x0000,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF

/*
	Tables to flip a whole vector of pixels
*/

vectorFlip_16:
	.short	0x0E0F,	0x0C0D,	0x0A0B,	0x0809,	0x0607,	0x0405,	0x0203,	0x0001

vectorFlip_32:
	.long	0x0C0D0E0F,	0x08090A0B,	0x04050607,	0x00010203

/*
	Tables for zooming a whole vector of pixel with a few calls to vperm
*/
pixel_zoom_table_2x_16bit:
	.short	0x0001,	0x0001,	0x0203,	0x0203,	0x1415,	0x1415,	0x1617,	0x1617
	.short	0x0809,	0x0809,	0x0a0b,	0x0a0b,	0x1c1d,	0x1c1d,	0x1e1f,	0x1e1f

pixel_zoom_table_2x_16bit_flip:
	.short	0x0e0f,	0x0e0f,	0x0c0d,	0x0c0d,	0x0a0b,	0x0a0b,	0x0809,	0x0809
	.short	0x0607,	0x0607,	0x0405,	0x0405,	0x0203,	0x0203,	0x0001,	0x0001

pixel_zoom_table_2x_32bit:
	.long	0x00010203,	0x00010203,	0x14151617,	0x14151617
	.long	0x08090a0b,	0x08090a0b,	0x1c1d1e1f,	0x1c1d1e1f

pixel_zoom_table_2x_32bit_flip:
	.long	0x0c0d0e0f,	0x0c0d0e0f,	0x08090a0b,	0x08090a0b
	.long	0x04050607,	0x04050607,	0x00010203,	0x00010203

pixel_zoom_table_3x_16bit:
	.short	0x0001,	0x0001,	0x0001,	0x0203,	0x0203,	0x0203,	0x1415,	0x1415
	.short	0x0405,	0x0607,	0x0607,	0x0607,	0x1819,	0x1819,	0x1819,	0x1a1b
	.short	0x0a0b,	0x0a0b,	0x1c1d,	0x1c1d,	0x1c1d,	0x1e1f,	0x1e1f,	0x1e1f

pixel_zoom_table_3x_16bit_flip:
	.short	0x0e0f,	0x0e0f,	0x0e0f,	0x0c0d,	0x0c0d,	0x0c0d,	0x0a0b,	0x0a0b
	.short	0x0a0b,	0x0809,	0x0809,	0x0809,	0x0607,	0x0607,	0x0607,	0x0405
	.short	0x0405,	0x0405,	0x0203,	0x0203,	0x0203,	0x0001,	0x0001,	0x0001

pixel_zoom_table_3x_32bit:
	.long	0x00010203,	0x00010203,	0x00010203,	0x04050607
	.long	0x04050607,	0x04050607,	0x18191a1b,	0x18191a1b
	.long	0x18191a1b,	0x1c1d1e1f,	0x1c1d1e1f,	0x1c1d1e1f

.text

pixel_zoom_table_3x_32bit_flip:
	.long	0x0c0d0e0f,	0x0c0d0e0f,	0x0c0d0e0f,	0x08090a0b
	.long	0x08090a0b,	0x08090a0b,	0x04050607,	0x04050607
	.long	0x04050607,	0x00010203,	0x00010203,	0x00010203

#pragma mark Expand pixel macros

//===================================================
#pragma mark Expand 15 (direct)

#define EXPAND_PIXEL_1x_16TO16D(outLabel)			\
	lhz		rPixel1,X_OFFSET(0,2)(rSource);			\
	LOAD_FROM_LOOKASIDE(lhz,rLookaside1,0);			\
	addi	rSource,rSource,X_INCREMENT(2);			\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	STORE_TO_LOOKASIDE(sth,rPixel1,0);				\
	SKIP_IF_NOT_DIRTY(cr1,outLabel);				\
	ADD_TO_LOOKASIDE(2);							\
	ADD_TO_DEST1(2);								\
	ADD_TO_DEST2(2);								\
	ADD_TO_DEST3(2);								\
	STORE_TO_DEST1(sth,rPixel1,-2);					\
	STORE_TO_DEST2(sth,rPixel1,-2);					\
	STORE_TO_DEST3(sth,rPixel1,-2);					\

#define EXPAND_PIXELS_1x_16TO16D					\
	lwz		rPixel1,X_OFFSET(0,4)(rSource);			\
	lwz		rPixel3,X_OFFSET(4,4)(rSource);			\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside1,0);			\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside2,4);			\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	COMPARE_TO_LOOKASIDE(cr5,rPixel3,rLookaside2);	\
	STORE_TO_LOOKASIDE(stw,rPixel1,0);				\
	COMBINE_COMPARES(cr6*4+eq,cr1*4+eq,cr5*4+eq);	\
	STORE_TO_LOOKASIDE(stw,rPixel3,4);				\
	SKIP_IF_NOT_DIRTY(cr6,DontDraw1);				\
	FLIP_PIXELS_16(rPixel1);						\
	FLIP_PIXELS_16(rPixel3);						\
	stw		rPixel1,-8(sp);							\
	stw		rPixel3,-4(sp);							\
	lfd		fpTemp1,-8(sp);							\
	STORE_TO_DEST1(stfd,fpTemp1,0);					\
	STORE_TO_DEST2(stfd,fpTemp1,0);					\
	STORE_TO_DEST3(stfd,fpTemp1,0);					\
DontDraw1:;											\
	lwz		rPixel1,X_OFFSET(8,4)(rSource);			\
	lwz		rPixel3,X_OFFSET(12,4)(rSource);		\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside1,8);			\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside2,12);		\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	COMPARE_TO_LOOKASIDE(cr5,rPixel3,rLookaside2);	\
	STORE_TO_LOOKASIDE(stw,rPixel1,8);				\
	COMBINE_COMPARES(cr6*4+eq,cr1*4+eq,cr5*4+eq);	\
	STORE_TO_LOOKASIDE(stw,rPixel3,12);				\
	SKIP_IF_NOT_DIRTY(cr6,DontDrawMulti);			\
	FLIP_PIXELS_16(rPixel1);						\
	FLIP_PIXELS_16(rPixel3);						\
	stw		rPixel1,-8(sp);							\
	stw		rPixel3,-4(sp);							\
	addi	rSource,rSource,X_INCREMENT(16);		\
	ADD_TO_LOOKASIDE(16);							\
	ADD_TO_DEST1(16);								\
	ADD_TO_DEST2(16);								\
	ADD_TO_DEST3(16);								\
	lfd		fpTemp1,-8(sp);							\
	STORE_TO_DEST1(stfd,fpTemp1,-8);				\
	STORE_TO_DEST2(stfd,fpTemp1,-8);				\
	STORE_TO_DEST3(stfd,fpTemp1,-8);				\


#define EXPAND_PIXEL_1x_16TO16D_SWAPXY(outLabel)	\
	lhz		rPixel1,0(rSource);						\
	LOAD_FROM_LOOKASIDE(lhz,rLookaside1,0);			\
	INCREMENT_SOURCE;								\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	STORE_TO_LOOKASIDE(sth,rPixel1,0);				\
	SKIP_IF_NOT_DIRTY(cr1,outLabel);				\
	ADD_TO_LOOKASIDE(2);							\
	ADD_TO_DEST1(2);								\
	ADD_TO_DEST2(2);								\
	ADD_TO_DEST3(2);								\
	STORE_TO_DEST1(sth,rPixel1,-2);					\
	STORE_TO_DEST2(sth,rPixel1,-2);					\
	STORE_TO_DEST3(sth,rPixel1,-2);					\

#define EXPAND_PIXELS_1x_16TO16D_SWAPXY				\
	lhz		rPixel2,0(rSource);						\
	INCREMENT_SOURCE;								\
	lhz		rPixel1,0(rSource);						\
	INCREMENT_SOURCE;								\
	lhz		rPixel4,0(rSource);						\
	INCREMENT_SOURCE;								\
	lhz		rPixel3,0(rSource);						\
	INCREMENT_SOURCE;								\
	rlwimi	rPixel1,rPixel2,16,0,15;				\
	rlwimi	rPixel3,rPixel4,16,0,15;				\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside1,0);			\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside2,4);			\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	COMPARE_TO_LOOKASIDE(cr5,rPixel3,rLookaside2);	\
	STORE_TO_LOOKASIDE(stw,rPixel1,0);				\
	COMBINE_COMPARES(cr6*4+eq,cr1*4+eq,cr5*4+eq);	\
	STORE_TO_LOOKASIDE(stw,rPixel3,4);				\
	SKIP_IF_NOT_DIRTY(cr6,DontDraw1);				\
	stw		rPixel1,-8(sp);							\
	stw		rPixel3,-4(sp);							\
	lfd		fpTemp1,-8(sp);							\
	STORE_TO_DEST1(stfd,fpTemp1,0);					\
	STORE_TO_DEST2(stfd,fpTemp1,0);					\
	STORE_TO_DEST3(stfd,fpTemp1,0);					\
DontDraw1:;											\
	lhz		rPixel2,0(rSource);						\
	INCREMENT_SOURCE;								\
	lhz		rPixel1,0(rSource);						\
	INCREMENT_SOURCE;								\
	lhz		rPixel4,0(rSource);						\
	INCREMENT_SOURCE;								\
	lhz		rPixel3,0(rSource);						\
	INCREMENT_SOURCE;								\
	rlwimi	rPixel1,rPixel2,16,0,15;				\
	rlwimi	rPixel3,rPixel4,16,0,15;				\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside1,8);			\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside2,12);		\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	COMPARE_TO_LOOKASIDE(cr5,rPixel3,rLookaside2);	\
	STORE_TO_LOOKASIDE(stw,rPixel1,8);				\
	COMBINE_COMPARES(cr6*4+eq,cr1*4+eq,cr5*4+eq);	\
	STORE_TO_LOOKASIDE(stw,rPixel3,12);				\
	SKIP_IF_NOT_DIRTY(cr6,DontDrawMulti);			\
	stw		rPixel1,-8(sp);							\
	stw		rPixel3,-4(sp);							\
	ADD_TO_LOOKASIDE(16);							\
	ADD_TO_DEST1(16);								\
	ADD_TO_DEST2(16);								\
	ADD_TO_DEST3(16);								\
	lfd		fpTemp1,-8(sp);							\
	STORE_TO_DEST1(stfd,fpTemp1,-8);				\
	STORE_TO_DEST2(stfd,fpTemp1,-8);				\
	STORE_TO_DEST3(stfd,fpTemp1,-8);				\


//===================================================

#define EXPAND_PIXEL_2x_16TO16D(outLabel)			\
	lhz		rPixel1,X_OFFSET(0,2)(rSource);			\
	LOAD_FROM_LOOKASIDE(lhz,rLookaside1,0);			\
	addi	rSource,rSource,X_INCREMENT(2);			\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	STORE_TO_LOOKASIDE(sth,rPixel1,0);				\
	SKIP_IF_NOT_DIRTY(cr1,outLabel);				\
	rlwimi	rPixel1,rPixel1,16,0,15;				\
	ADD_TO_LOOKASIDE(2);							\
	ADD_TO_DEST1(4);								\
	ADD_TO_DEST2(4);								\
	ADD_TO_DEST3(4);								\
	STORE_TO_DEST1(stw,rPixel1,-4);					\
	STORE_TO_DEST2(stw,rPixel1,-4);					\
	STORE_TO_DEST3(stw,rPixel1,-4);					\

#define EXPAND_PIXELS_2x_16TO16D					\
	lwz		rPixel1,X_OFFSET(0,4)(rSource);			\
	lwz		rPixel3,X_OFFSET(4,4)(rSource);			\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside1,0);			\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside2,4);			\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	COMPARE_TO_LOOKASIDE(cr5,rPixel3,rLookaside2);	\
	STORE_TO_LOOKASIDE(stw,rPixel1,0);				\
	COMBINE_COMPARES(cr6*4+eq,cr1*4+eq,cr5*4+eq);	\
	STORE_TO_LOOKASIDE(stw,rPixel3,4);				\
	SKIP_IF_NOT_DIRTY(cr6,DontDraw1);				\
	mr		rPixel2,rPixel1;						\
	mr		rPixel4,rPixel3;						\
	DUPLICATE_PIXEL0_16(rPixel2,rPixel1);			\
	DUPLICATE_PIXEL0_16(rPixel4,rPixel3);			\
	DUPLICATE_PIXEL1_16(rPixel1,rPixel1);			\
	DUPLICATE_PIXEL1_16(rPixel3,rPixel3);			\
	stw		rPixel2,-16(sp);						\
	stw		rPixel1,-12(sp);						\
	stw		rPixel4,-8(sp);							\
	stw		rPixel3,-4(sp);							\
	lfd		fpTemp1,-16(sp);						\
	lfd		fpTemp2,-8(sp);							\
	SKIP_IF_NOT_DIRTY(cr1,skip1);					\
	STORE_TO_DEST1(stfd,fpTemp1,0);					\
	STORE_TO_DEST2(stfd,fpTemp1,0);					\
	STORE_TO_DEST3(stfd,fpTemp1,0);					\
	SKIP_IF_NOT_DIRTY(cr5,skip2);					\
skip1:;												\
	STORE_TO_DEST1(stfd,fpTemp2,8);					\
	STORE_TO_DEST2(stfd,fpTemp2,8);					\
	STORE_TO_DEST3(stfd,fpTemp2,8);					\
skip2:;												\
DontDraw1:;											\
	lwz		rPixel1,X_OFFSET(8,4)(rSource);			\
	lwz		rPixel3,X_OFFSET(12,4)(rSource);		\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside1,8);			\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside2,12);		\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	COMPARE_TO_LOOKASIDE(cr5,rPixel3,rLookaside2);	\
	STORE_TO_LOOKASIDE(stw,rPixel1,8);				\
	COMBINE_COMPARES(cr6*4+eq,cr1*4+eq,cr5*4+eq);	\
	STORE_TO_LOOKASIDE(stw,rPixel3,12);				\
	SKIP_IF_NOT_DIRTY(cr6,DontDrawMulti);			\
	mr		rPixel2,rPixel1;						\
	mr		rPixel4,rPixel3;						\
	DUPLICATE_PIXEL0_16(rPixel2,rPixel1);			\
	DUPLICATE_PIXEL0_16(rPixel4,rPixel3);			\
	DUPLICATE_PIXEL1_16(rPixel1,rPixel1);			\
	DUPLICATE_PIXEL1_16(rPixel3,rPixel3);			\
	stw		rPixel2,-16(sp);						\
	stw		rPixel1,-12(sp);						\
	stw		rPixel4,-8(sp);							\
	stw		rPixel3,-4(sp);							\
	addi	rSource,rSource,X_INCREMENT(16);		\
	ADD_TO_LOOKASIDE(16);							\
	ADD_TO_DEST1(32);								\
	ADD_TO_DEST2(32);								\
	ADD_TO_DEST3(32);								\
	lfd		fpTemp1,-16(sp);						\
	lfd		fpTemp2,-8(sp);							\
	SKIP_IF_NOT_DIRTY(cr1,skip3);					\
	STORE_TO_DEST1(stfd,fpTemp1,-16);				\
	STORE_TO_DEST2(stfd,fpTemp1,-16);				\
	STORE_TO_DEST3(stfd,fpTemp1,-16);				\
	SKIP_IF_NOT_DIRTY(cr5,skip4);					\
skip3:;												\
	STORE_TO_DEST1(stfd,fpTemp2,-8);				\
	STORE_TO_DEST2(stfd,fpTemp2,-8);				\
	STORE_TO_DEST3(stfd,fpTemp2,-8);				\
skip4:;												\


#define EXPAND_PIXEL_2x_16TO16D_SWAPXY(outLabel)	\
	lhz		rPixel1,X_OFFSET(0,2)(rSource);			\
	LOAD_FROM_LOOKASIDE(lhz,rLookaside1,0);			\
	INCREMENT_SOURCE;								\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	STORE_TO_LOOKASIDE(sth,rPixel1,0);				\
	SKIP_IF_NOT_DIRTY(cr1,outLabel);				\
	rlwimi	rPixel1,rPixel1,16,0,15;				\
	ADD_TO_LOOKASIDE(2);							\
	ADD_TO_DEST1(4);								\
	ADD_TO_DEST2(4);								\
	ADD_TO_DEST3(4);								\
	STORE_TO_DEST1(stw,rPixel1,-4);					\
	STORE_TO_DEST2(stw,rPixel1,-4);					\
	STORE_TO_DEST3(stw,rPixel1,-4);					\

#define EXPAND_PIXELS_2x_16TO16D_SWAPXY				\
	lhz		rPixel2,0(rSource);						\
	INCREMENT_SOURCE;								\
	lhz		rPixel1,0(rSource);						\
	INCREMENT_SOURCE;								\
	lhz		rPixel4,0(rSource);						\
	INCREMENT_SOURCE;								\
	lhz		rPixel3,0(rSource);						\
	INCREMENT_SOURCE;								\
	rlwimi	rPixel1,rPixel2,16,0,15;				\
	rlwimi	rPixel3,rPixel4,16,0,15;				\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside1,0);			\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside2,4);			\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	COMPARE_TO_LOOKASIDE(cr5,rPixel3,rLookaside2);	\
	STORE_TO_LOOKASIDE(stw,rPixel1,0);				\
	COMBINE_COMPARES(cr6*4+eq,cr1*4+eq,cr5*4+eq);	\
	STORE_TO_LOOKASIDE(stw,rPixel3,4);				\
	SKIP_IF_NOT_DIRTY(cr6,DontDraw1);				\
	mr		rPixel2,rPixel1;						\
	mr		rPixel4,rPixel3;						\
	rlwimi	rPixel2,rPixel1,16,16,31;				\
	rlwimi	rPixel4,rPixel3,16,16,31;				\
	rlwimi	rPixel1,rPixel1,16,0,15;				\
	rlwimi	rPixel3,rPixel3,16,0,15;				\
	stw		rPixel2,-16(sp);						\
	stw		rPixel1,-12(sp);						\
	stw		rPixel4,-8(sp);							\
	stw		rPixel3,-4(sp);							\
	lfd		fpTemp1,-16(sp);						\
	lfd		fpTemp2,-8(sp);							\
	SKIP_IF_NOT_DIRTY(cr1,skip1);					\
	STORE_TO_DEST1(stfd,fpTemp1,0);					\
	STORE_TO_DEST2(stfd,fpTemp1,0);					\
	STORE_TO_DEST3(stfd,fpTemp1,0);					\
	SKIP_IF_NOT_DIRTY(cr5,skip2);					\
skip1:;												\
	STORE_TO_DEST1(stfd,fpTemp2,8);					\
	STORE_TO_DEST2(stfd,fpTemp2,8);					\
	STORE_TO_DEST3(stfd,fpTemp2,8);					\
skip2:;												\
DontDraw1:;											\
	lhz		rPixel2,0(rSource);						\
	INCREMENT_SOURCE;								\
	lhz		rPixel1,0(rSource);						\
	INCREMENT_SOURCE;								\
	lhz		rPixel4,0(rSource);						\
	INCREMENT_SOURCE;								\
	lhz		rPixel3,0(rSource);						\
	INCREMENT_SOURCE;								\
	rlwimi	rPixel1,rPixel2,16,0,15;				\
	rlwimi	rPixel3,rPixel4,16,0,15;				\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside1,8);			\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside2,12);		\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	COMPARE_TO_LOOKASIDE(cr5,rPixel3,rLookaside2);	\
	STORE_TO_LOOKASIDE(stw,rPixel1,8);				\
	COMBINE_COMPARES(cr6*4+eq,cr1*4+eq,cr5*4+eq);	\
	STORE_TO_LOOKASIDE(stw,rPixel3,12);				\
	SKIP_IF_NOT_DIRTY(cr6,DontDrawMulti);			\
	mr		rPixel2,rPixel1;						\
	mr		rPixel4,rPixel3;						\
	rlwimi	rPixel2,rPixel1,16,16,31;				\
	rlwimi	rPixel4,rPixel3,16,16,31;				\
	rlwimi	rPixel1,rPixel1,16,0,15;				\
	rlwimi	rPixel3,rPixel3,16,0,15;				\
	stw		rPixel2,-16(sp);						\
	stw		rPixel1,-12(sp);						\
	stw		rPixel4,-8(sp);							\
	stw		rPixel3,-4(sp);							\
	ADD_TO_LOOKASIDE(16);							\
	ADD_TO_DEST1(32);								\
	ADD_TO_DEST2(32);								\
	ADD_TO_DEST3(32);								\
	lfd		fpTemp1,-16(sp);						\
	lfd		fpTemp2,-8(sp);							\
	SKIP_IF_NOT_DIRTY(cr1,skip3);					\
	STORE_TO_DEST1(stfd,fpTemp1,-16);				\
	STORE_TO_DEST2(stfd,fpTemp1,-16);				\
	STORE_TO_DEST3(stfd,fpTemp1,-16);				\
	SKIP_IF_NOT_DIRTY(cr5,skip4);					\
skip3:;												\
	STORE_TO_DEST1(stfd,fpTemp2,-8);				\
	STORE_TO_DEST2(stfd,fpTemp2,-8);				\
	STORE_TO_DEST3(stfd,fpTemp2,-8);				\
skip4:;												\


//===================================================

#define EXPAND_PIXEL_3x_16TO16D(outLabel)			\
	lhz		rPixel1,X_OFFSET(0,2)(rSource);			\
	LOAD_FROM_LOOKASIDE(lhz,rLookaside1,0);			\
	addi	rSource,rSource,X_INCREMENT(2);			\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	STORE_TO_LOOKASIDE(sth,rPixel1,0);				\
	rlwimi	rPixel1,rPixel1,16,0,15;				\
	SKIP_IF_NOT_DIRTY(cr1,outLabel);				\
	ADD_TO_LOOKASIDE(2);							\
	ADD_TO_DEST1(6);								\
	ADD_TO_DEST2(6);								\
	ADD_TO_DEST3(6);								\
	STORE_TO_DEST1(stw,rPixel1,-6);					\
	STORE_TO_DEST1(sth,rPixel1,-2);					\
	STORE_TO_DEST2(stw,rPixel1,-6);					\
	STORE_TO_DEST2(sth,rPixel1,-2);					\
	STORE_TO_DEST3(stw,rPixel1,-6);					\
	STORE_TO_DEST3(sth,rPixel1,-2);					\

#define EXPAND_PIXELS_3x_16TO16D					\
	lwz		rPixel1,X_OFFSET(0,4)(rSource);			\
	lwz		rPixel4,X_OFFSET(4,4)(rSource);			\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside1,0);			\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside2,4);			\
	MOVE_FLIP_PIXELS_16(rPixel2,rPixel1);	/*AB*/	\
	MOVE_FLIP_PIXELS_16(rPixel5,rPixel4);	/*CD*/	\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	COMPARE_TO_LOOKASIDE(cr5,rPixel4,rLookaside2);	\
	STORE_TO_LOOKASIDE(stw,rPixel1,0);				\
	COMBINE_COMPARES(cr6*4+eq,cr1*4+eq,cr5*4+eq);	\
	STORE_TO_LOOKASIDE(stw,rPixel4,4);				\
	SKIP_IF_NOT_DIRTY(cr6,DontDraw1);				\
	mr		rPixel3,rPixel1;						\
	mr		rPixel6,rPixel4;						\
	DUPLICATE_PIXEL0_16(rPixel1,rPixel1);	/*AA*/	\
	DUPLICATE_PIXEL1_16(rPixel3,rPixel3);	/*BB*/	\
	DUPLICATE_PIXEL0_16(rPixel4,rPixel4);	/*CC*/	\
	DUPLICATE_PIXEL1_16(rPixel6,rPixel6);	/*DD*/	\
	stw		rPixel1,-24(sp);						\
	stw		rPixel2,-20(sp);						\
	stw		rPixel3,-16(sp);						\
	stw		rPixel4,-12(sp);						\
	stw		rPixel5,-8(sp);							\
	stw		rPixel6,-4(sp);							\
	lfd		fpTemp1,-24(sp);						\
	lfd		fpTemp2,-16(sp);						\
	lfd		fpTemp3,-8(sp);							\
	SKIP_IF_NOT_DIRTY(cr1,skip1);					\
	STORE_TO_DEST1(stfd,fpTemp1,0);					\
	STORE_TO_DEST2(stfd,fpTemp1,0);					\
	STORE_TO_DEST3(stfd,fpTemp1,0);					\
skip1:;												\
	STORE_TO_DEST1(stfd,fpTemp2,8);					\
	STORE_TO_DEST2(stfd,fpTemp2,8);					\
	STORE_TO_DEST3(stfd,fpTemp2,8);					\
	SKIP_IF_NOT_DIRTY(cr5,skip2);					\
	STORE_TO_DEST1(stfd,fpTemp3,16);				\
	STORE_TO_DEST2(stfd,fpTemp3,16);				\
	STORE_TO_DEST3(stfd,fpTemp3,16);				\
skip2:;												\
DontDraw1:;											\
	lwz		rPixel1,X_OFFSET(8,4)(rSource);			\
	lwz		rPixel4,X_OFFSET(12,4)(rSource);		\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside1,8);			\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside2,12);		\
	MOVE_FLIP_PIXELS_16(rPixel2,rPixel1);	/*AB*/	\
	MOVE_FLIP_PIXELS_16(rPixel5,rPixel4);	/*CD*/	\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	COMPARE_TO_LOOKASIDE(cr5,rPixel4,rLookaside2);	\
	STORE_TO_LOOKASIDE(stw,rPixel1,8);				\
	COMBINE_COMPARES(cr6*4+eq,cr1*4+eq,cr5*4+eq);	\
	STORE_TO_LOOKASIDE(stw,rPixel4,12);				\
	SKIP_IF_NOT_DIRTY(cr6,DontDrawMulti);			\
	mr		rPixel3,rPixel1;						\
	mr		rPixel6,rPixel4;						\
	DUPLICATE_PIXEL0_16(rPixel1,rPixel1);	/*AA*/	\
	DUPLICATE_PIXEL1_16(rPixel3,rPixel3);	/*BB*/	\
	DUPLICATE_PIXEL0_16(rPixel4,rPixel4);	/*CC*/	\
	DUPLICATE_PIXEL1_16(rPixel6,rPixel6);	/*DD*/	\
	stw		rPixel1,-24(sp);						\
	stw		rPixel2,-20(sp);						\
	stw		rPixel3,-16(sp);						\
	stw		rPixel4,-12(sp);						\
	stw		rPixel5,-8(sp);							\
	stw		rPixel6,-4(sp);							\
	addi	rSource,rSource,X_INCREMENT(16);		\
	ADD_TO_LOOKASIDE(16);							\
	ADD_TO_DEST1(48);								\
	ADD_TO_DEST2(48);								\
	ADD_TO_DEST3(48);								\
	lfd		fpTemp1,-24(sp);						\
	lfd		fpTemp2,-16(sp);						\
	lfd		fpTemp3,-8(sp);							\
	SKIP_IF_NOT_DIRTY(cr1,skip3);					\
	STORE_TO_DEST1(stfd,fpTemp1,-24);				\
	STORE_TO_DEST2(stfd,fpTemp1,-24);				\
	STORE_TO_DEST3(stfd,fpTemp1,-24);				\
skip3:;												\
	STORE_TO_DEST1(stfd,fpTemp2,-16);				\
	STORE_TO_DEST2(stfd,fpTemp2,-16);				\
	STORE_TO_DEST3(stfd,fpTemp2,-16);				\
	SKIP_IF_NOT_DIRTY(cr5,skip4);					\
	STORE_TO_DEST1(stfd,fpTemp3,-8);				\
	STORE_TO_DEST2(stfd,fpTemp3,-8);				\
	STORE_TO_DEST3(stfd,fpTemp3,-8);				\
skip4:;												\


#define EXPAND_PIXEL_3x_16TO16D_SWAPXY(outLabel)	\
	lhz		rPixel1,0(rSource);						\
	LOAD_FROM_LOOKASIDE(lhz,rLookaside1,0);			\
	INCREMENT_SOURCE;								\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	STORE_TO_LOOKASIDE(sth,rPixel1,0);				\
	rlwimi	rPixel1,rPixel1,16,0,15;				\
	SKIP_IF_NOT_DIRTY(cr1,outLabel);				\
	ADD_TO_LOOKASIDE(2);							\
	ADD_TO_DEST1(6);								\
	ADD_TO_DEST2(6);								\
	ADD_TO_DEST3(6);								\
	STORE_TO_DEST1(stw,rPixel1,-6);					\
	STORE_TO_DEST1(sth,rPixel1,-2);					\
	STORE_TO_DEST2(stw,rPixel1,-6);					\
	STORE_TO_DEST2(sth,rPixel1,-2);					\
	STORE_TO_DEST3(stw,rPixel1,-6);					\
	STORE_TO_DEST3(sth,rPixel1,-2);					\

#define EXPAND_PIXELS_3x_16TO16D_SWAPXY				\
	lhz		rPixel2,0(rSource);						\
	INCREMENT_SOURCE;								\
	lhz		rPixel1,0(rSource);						\
	INCREMENT_SOURCE;								\
	lhz		rPixel5,0(rSource);						\
	INCREMENT_SOURCE;								\
	lhz		rPixel4,0(rSource);						\
	INCREMENT_SOURCE;								\
	rlwimi	rPixel1,rPixel2,16,0,15;				\
	rlwimi	rPixel4,rPixel5,16,0,15;				\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside1,0);			\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside2,4);			\
	mr		rPixel2,rPixel1;				/*AB*/	\
	mr		rPixel5,rPixel4;				/*CD*/	\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	COMPARE_TO_LOOKASIDE(cr5,rPixel4,rLookaside2);	\
	STORE_TO_LOOKASIDE(stw,rPixel1,0);				\
	COMBINE_COMPARES(cr6*4+eq,cr1*4+eq,cr5*4+eq);	\
	STORE_TO_LOOKASIDE(stw,rPixel4,4);				\
	SKIP_IF_NOT_DIRTY(cr6,DontDraw1);				\
	mr		rPixel3,rPixel1;						\
	mr		rPixel6,rPixel4;						\
	rlwimi	rPixel1,rPixel1,16,16,31;		/*AA*/	\
	rlwimi	rPixel3,rPixel3,16,0,15;		/*BB*/	\
	rlwimi	rPixel4,rPixel4,16,16,31;		/*CC*/	\
	rlwimi	rPixel6,rPixel6,16,0,15;		/*DD*/	\
	stw		rPixel1,-24(sp);						\
	stw		rPixel2,-20(sp);						\
	stw		rPixel3,-16(sp);						\
	stw		rPixel4,-12(sp);						\
	stw		rPixel5,-8(sp);							\
	stw		rPixel6,-4(sp);							\
	lfd		fpTemp1,-24(sp);						\
	lfd		fpTemp2,-16(sp);						\
	lfd		fpTemp3,-8(sp);							\
	SKIP_IF_NOT_DIRTY(cr1,skip1);					\
	STORE_TO_DEST1(stfd,fpTemp1,0);					\
	STORE_TO_DEST2(stfd,fpTemp1,0);					\
	STORE_TO_DEST3(stfd,fpTemp1,0);					\
skip1:;												\
	STORE_TO_DEST1(stfd,fpTemp2,8);					\
	STORE_TO_DEST2(stfd,fpTemp2,8);					\
	STORE_TO_DEST3(stfd,fpTemp2,8);					\
	SKIP_IF_NOT_DIRTY(cr5,skip2);					\
	STORE_TO_DEST1(stfd,fpTemp3,16);				\
	STORE_TO_DEST2(stfd,fpTemp3,16);				\
	STORE_TO_DEST3(stfd,fpTemp3,16);				\
skip2:;												\
DontDraw1:;											\
	lhz		rPixel2,0(rSource);						\
	INCREMENT_SOURCE;								\
	lhz		rPixel1,0(rSource);						\
	INCREMENT_SOURCE;								\
	lhz		rPixel5,0(rSource);						\
	INCREMENT_SOURCE;								\
	lhz		rPixel4,0(rSource);						\
	INCREMENT_SOURCE;								\
	rlwimi	rPixel1,rPixel2,16,0,15;				\
	rlwimi	rPixel4,rPixel5,16,0,15;				\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside1,8);			\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside2,12);		\
	mr		rPixel2,rPixel1;				/*AB*/	\
	mr		rPixel5,rPixel4;				/*CD*/	\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	COMPARE_TO_LOOKASIDE(cr5,rPixel4,rLookaside2);	\
	STORE_TO_LOOKASIDE(stw,rPixel1,8);				\
	COMBINE_COMPARES(cr6*4+eq,cr1*4+eq,cr5*4+eq);	\
	STORE_TO_LOOKASIDE(stw,rPixel4,12);				\
	SKIP_IF_NOT_DIRTY(cr6,DontDrawMulti);			\
	mr		rPixel3,rPixel1;						\
	mr		rPixel6,rPixel4;						\
	rlwimi	rPixel1,rPixel1,16,16,31;		/*AA*/	\
	rlwimi	rPixel3,rPixel3,16,0,15;		/*BB*/	\
	rlwimi	rPixel4,rPixel4,16,16,31;		/*CC*/	\
	rlwimi	rPixel6,rPixel6,16,0,15;		/*DD*/	\
	stw		rPixel1,-24(sp);						\
	stw		rPixel2,-20(sp);						\
	stw		rPixel3,-16(sp);						\
	stw		rPixel4,-12(sp);						\
	stw		rPixel5,-8(sp);							\
	stw		rPixel6,-4(sp);							\
	ADD_TO_LOOKASIDE(16);							\
	ADD_TO_DEST1(48);								\
	ADD_TO_DEST2(48);								\
	ADD_TO_DEST3(48);								\
	lfd		fpTemp1,-24(sp);						\
	lfd		fpTemp2,-16(sp);						\
	lfd		fpTemp3,-8(sp);							\
	SKIP_IF_NOT_DIRTY(cr1,skip3);					\
	STORE_TO_DEST1(stfd,fpTemp1,-24);				\
	STORE_TO_DEST2(stfd,fpTemp1,-24);				\
	STORE_TO_DEST3(stfd,fpTemp1,-24);				\
skip3:;												\
	STORE_TO_DEST1(stfd,fpTemp2,-16);				\
	STORE_TO_DEST2(stfd,fpTemp2,-16);				\
	STORE_TO_DEST3(stfd,fpTemp2,-16);				\
	SKIP_IF_NOT_DIRTY(cr5,skip4);					\
	STORE_TO_DEST1(stfd,fpTemp3,-8);				\
	STORE_TO_DEST2(stfd,fpTemp3,-8);				\
	STORE_TO_DEST3(stfd,fpTemp3,-8);				\
skip4:;												\


//===================================================

#define EXPAND_PIXEL_1x_16TO16D_VEC(outLabel)		\
	lhz		rPixel1,X_OFFSET(0,2)(rSource);			\
	LOAD_FROM_LOOKASIDE(lhz,rLookaside1,0);			\
	addi	rSource,rSource,X_INCREMENT(2);			\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	STORE_TO_LOOKASIDE(sth,rPixel1,0);				\
	SKIP_IF_NOT_DIRTY(cr1,outLabel);				\
	ADD_TO_LOOKASIDE(2);							\
	STORE_TO_DEST1(sth,rPixel1,0);					\
	STORE_TO_DEST2(sthx,rPixel1);					\
	STORE_TO_DEST3(sthx,rPixel1);					\
	ADD_TO_DEST(2);									\

#define EXPAND_PIXELS_1x_16TO16D_VEC										\
	lvx		vrTemp2,0,rSource;				/* load source */				\
	addi	rSource,rSource,X_INCREMENT(16);								\
	LOAD_FROM_LOOKASIDE_VEC(vrTemp1);		/* load 8 pixels of lookaside */\
	COMPARE_TO_LOOKASIDE_VEC(vrTemp1,vrTemp1,vrTemp2);	/* compare all 8 pixels */	\
	SKIP_IF_NOT_DIRTY_VEC(DontDrawMulti);	/* skip if it is clean */		\
	STORE_TO_LOOKASIDE_VEC(vrTemp2);		/* save current source to lookaside */	\
	FLIP_PIXELS_VEC(vrTemp2);												\
																			\
	ADD_TO_LOOKASIDE(16);													\
	STORE_TO_DEST1_VEC(vrTemp2);			/* ABCDEFGH to line 1 */		\
	STORE_TO_DEST2_VEC(vrTemp2);			/* ABCDEFGH to line 2 */		\
	STORE_TO_DEST3_VEC(vrTemp2);			/* ABCDEFGH to line 3 */		\
	ADD_TO_DEST(16);														\


#define EXPAND_PIXEL_2x_16TO16D_VEC(outLabel)		\
	lhz		rPixel1,X_OFFSET(0,2)(rSource);			\
	LOAD_FROM_LOOKASIDE(lhz,rLookaside1,0);			\
	addi	rSource,rSource,X_INCREMENT(2);			\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	STORE_TO_LOOKASIDE(sth,rPixel1,0);				\
	SKIP_IF_NOT_DIRTY(cr1,outLabel);				\
	rlwimi	rPixel1,rPixel1,16,0,15;				\
	ADD_TO_LOOKASIDE(2);							\
	STORE_TO_DEST1(stw,rPixel1,0);					\
	STORE_TO_DEST2(stwx,rPixel1);					\
	STORE_TO_DEST3(stwx,rPixel1);					\
	ADD_TO_DEST(4);									\

#define EXPAND_PIXELS_2x_16TO16D_VEC										\
	lvx		vrTemp2,0,rSource;				/* load source */				\
	addi	rSource,rSource,X_INCREMENT(16);								\
	LOAD_FROM_LOOKASIDE_VEC(vrTemp1);		/* load 8 pixels of lookaside */\
	COMPARE_TO_LOOKASIDE_VEC(vrTemp1,vrTemp1,vrTemp2);	/* compare all 8 pixels */	\
	SKIP_IF_NOT_DIRTY_VEC(DontDrawMulti);	/* skip if it is clean */		\
	STORE_TO_LOOKASIDE_VEC(vrTemp2);		/* save current source to lookaside */	\
																			\
	ADD_TO_LOOKASIDE(16);													\
	ZOOM_FLIP_PIXELS_2x_HIGH_VEC(vrTemp0,vrTemp2);	/* AABBCCDD */			\
	ZOOM_FLIP_PIXELS_2x_LOW_VEC(vrTemp1,vrTemp2);	/* EEFFGGHH */			\
	STORE_TO_DEST1_VEC(vrTemp0);			/* AABBCCDD to line 1 */		\
	STORE_TO_DEST2_VEC(vrTemp0);			/* AABBCCDD to line 2 */		\
	STORE_TO_DEST3_VEC(vrTemp0);			/* AABBCCDD to line 3 */		\
	ADD_TO_DEST(16);														\
	STORE_TO_DEST1_VEC(vrTemp1);			/* EEFFGGHH to line 1 */		\
	STORE_TO_DEST2_VEC(vrTemp1);			/* EEFFGGHH to line 2 */		\
	STORE_TO_DEST3_VEC(vrTemp1);			/* EEFFGGHH to line 3 */		\
	ADD_TO_DEST(16);														\


#define EXPAND_PIXEL_3x_16TO16D_VEC(outLabel)		\
	lhz		rPixel1,X_OFFSET(0,2)(rSource);			\
	LOAD_FROM_LOOKASIDE(lhz,rLookaside1,0);			\
	addi	rSource,rSource,X_INCREMENT(2);			\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	STORE_TO_LOOKASIDE(sth,rPixel1,0);				\
	rlwimi	rPixel1,rPixel1,16,0,15;				\
	SKIP_IF_NOT_DIRTY(cr1,outLabel);				\
	ADD_TO_LOOKASIDE(2);							\
	STORE_TO_DEST1(stw,rPixel1,0);					\
	STORE_TO_DEST2(stwx,rPixel1);					\
	STORE_TO_DEST3(stwx,rPixel1);					\
	STORE_TO_DEST1(sthu,rPixel1,4);					\
	STORE_TO_DEST2(sthx,rPixel1);					\
	STORE_TO_DEST3(sthx,rPixel1);					\
	ADD_TO_DEST(2);									\

#define EXPAND_PIXELS_3x_16TO16D_VEC										\
	lvx		vrTemp2,0,rSource;				/* load source */				\
	addi	rSource,rSource,X_INCREMENT(16);								\
	LOAD_FROM_LOOKASIDE_VEC(vrTemp1);		/* load 8 pixels of lookaside */\
	COMPARE_TO_LOOKASIDE_VEC(vrTemp1,vrTemp1,vrTemp2);	/* compare all 8 pixels */	\
	SKIP_IF_NOT_DIRTY_VEC(DontDrawMulti);	/* skip if it is clean */		\
	STORE_TO_LOOKASIDE_VEC(vrTemp2);		/* save current source to lookaside */	\
																			\
	ADD_TO_LOOKASIDE(16);													\
	vperm	vrTemp0,vrTemp2,vrTemp2,vrPixelZoom0;	/* AAABBBCC */			\
	vperm	vrTemp1,vrTemp2,vrTemp2,vrPixelZoom1;	/* CDDDEEEF */			\
	STORE_TO_DEST1_VEC(vrTemp0);			/* AAABBBCC to line 1 */		\
	STORE_TO_DEST2_VEC(vrTemp0);			/* AAABBBCC to line 2 */		\
	STORE_TO_DEST3_VEC(vrTemp0);			/* AAABBBCC to line 3 */		\
	ADD_TO_DEST(16);														\
	vperm	vrTemp2,vrTemp2,vrTemp2,vrPixelZoom2;	/* FFGGGHHH */			\
	STORE_TO_DEST1_VEC(vrTemp1);			/* CDDDEEEF to line 1 */		\
	STORE_TO_DEST2_VEC(vrTemp1);			/* CDDDEEEF to line 2 */		\
	STORE_TO_DEST3_VEC(vrTemp1);			/* CDDDEEEF to line 3 */		\
	ADD_TO_DEST(16);														\
	STORE_TO_DEST1_VEC(vrTemp2);			/* FFGGGHHH to line 1 */		\
	STORE_TO_DEST2_VEC(vrTemp2);			/* FFGGGHHH to line 2 */		\
	STORE_TO_DEST3_VEC(vrTemp2);			/* FFGGGHHH to line 3 */		\
	ADD_TO_DEST(16);														\


//===================================================
#pragma mark Expand 16

#define EXPAND_PIXEL_1x_16TO16L(outLabel)			\
	lhz		rPixel1,X_OFFSET(0,2)(rSource);			\
	LOAD_FROM_LOOKASIDE(lhz,rLookaside1,0);			\
	addi	rSource,rSource,X_INCREMENT(2);			\
	rlwinm	rPixel2,rPixel1,2,14,29;				\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	STORE_TO_LOOKASIDE(sth,rPixel1,0);				\
	SKIP_IF_NOT_DIRTY(cr1,outLabel);				\
	lwzx	rPixel2,rLookup,rPixel2;				\
	ADD_TO_LOOKASIDE(2);							\
	ADD_TO_DEST1(2);								\
	ADD_TO_DEST2(2);								\
	ADD_TO_DEST3(2);								\
	STORE_TO_DEST1(sth,rPixel2,-2);					\
	STORE_TO_DEST2(sth,rPixel2,-2);					\
	STORE_TO_DEST3(sth,rPixel2,-2);					\

#define EXPAND_PIXELS_1x_16TO16L					\
	lwz		rPixel1,X_OFFSET(0,4)(rSource);			\
	lwz		rPixel3,X_OFFSET(4,4)(rSource);			\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside1,0);			\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside2,4);			\
	EXTRACT_PIXEL1_16(rPixel2,rPixel1,2);			\
	EXTRACT_PIXEL1_16(rPixel4,rPixel3,2);			\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	COMPARE_TO_LOOKASIDE(cr5,rPixel3,rLookaside2);	\
	STORE_TO_LOOKASIDE(stw,rPixel1,0);				\
	COMBINE_COMPARES(cr6*4+eq,cr1*4+eq,cr5*4+eq);	\
	STORE_TO_LOOKASIDE(stw,rPixel3,4);				\
	SKIP_IF_NOT_DIRTY(cr6,DontDraw1);				\
	EXTRACT_PIXEL0_16(rPixel1,rPixel1,2);			\
	EXTRACT_PIXEL0_16(rPixel3,rPixel3,2);			\
	lwzx	rPixel2,rLookup,rPixel2;				\
	lwzx	rPixel4,rLookup,rPixel4;				\
	lwzx	rPixel1,rLookup,rPixel1;				\
	lwzx	rPixel3,rLookup,rPixel3;				\
	rlwimi	rPixel2,rPixel1,16,0,15;		/*AB*/	\
	rlwimi	rPixel4,rPixel3,16,0,15;		/*CD*/	\
	stw		rPixel2,-8(sp);							\
	stw		rPixel4,-4(sp);							\
	lfd		fpTemp1,-8(sp);							\
	STORE_TO_DEST1(stfd,fpTemp1,0);					\
	STORE_TO_DEST2(stfd,fpTemp1,0);					\
	STORE_TO_DEST3(stfd,fpTemp1,0);					\
DontDraw1:;											\
	lwz		rPixel1,X_OFFSET(8,4)(rSource);			\
	lwz		rPixel3,X_OFFSET(12,4)(rSource);		\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside1,8);			\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside2,12);		\
	EXTRACT_PIXEL1_16(rPixel2,rPixel1,2);			\
	EXTRACT_PIXEL1_16(rPixel4,rPixel3,2);			\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	COMPARE_TO_LOOKASIDE(cr5,rPixel3,rLookaside2);	\
	STORE_TO_LOOKASIDE(stw,rPixel1,8);				\
	COMBINE_COMPARES(cr6*4+eq,cr1*4+eq,cr5*4+eq);	\
	STORE_TO_LOOKASIDE(stw,rPixel3,12);				\
	SKIP_IF_NOT_DIRTY(cr6,DontDrawMulti);			\
	EXTRACT_PIXEL0_16(rPixel1,rPixel1,2);			\
	EXTRACT_PIXEL0_16(rPixel3,rPixel3,2);			\
	lwzx	rPixel2,rLookup,rPixel2;				\
	lwzx	rPixel4,rLookup,rPixel4;				\
	lwzx	rPixel1,rLookup,rPixel1;				\
	lwzx	rPixel3,rLookup,rPixel3;				\
	rlwimi	rPixel2,rPixel1,16,0,15;		/*EF*/	\
	rlwimi	rPixel4,rPixel3,16,0,15;		/*GH*/	\
	stw		rPixel2,-8(sp);							\
	stw		rPixel4,-4(sp);							\
	addi	rSource,rSource,X_INCREMENT(16);		\
	ADD_TO_LOOKASIDE(16);							\
	ADD_TO_DEST1(16);								\
	ADD_TO_DEST2(16);								\
	ADD_TO_DEST3(16);								\
	lfd		fpTemp1,-8(sp);							\
	STORE_TO_DEST1(stfd,fpTemp1,-8);				\
	STORE_TO_DEST2(stfd,fpTemp1,-8);				\
	STORE_TO_DEST3(stfd,fpTemp1,-8);				\


#define EXPAND_PIXEL_1x_16TO16L_SWAPXY(outLabel)	\
	lhz		rPixel1,0(rSource);						\
	LOAD_FROM_LOOKASIDE(lhz,rLookaside1,0);			\
	INCREMENT_SOURCE;								\
	rlwinm	rPixel2,rPixel1,2,14,29;				\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	STORE_TO_LOOKASIDE(sth,rPixel1,0);				\
	SKIP_IF_NOT_DIRTY(cr1,outLabel);				\
	lwzx	rPixel2,rLookup,rPixel2;				\
	ADD_TO_LOOKASIDE(2);							\
	ADD_TO_DEST1(2);								\
	ADD_TO_DEST2(2);								\
	ADD_TO_DEST3(2);								\
	STORE_TO_DEST1(sth,rPixel2,-2);					\
	STORE_TO_DEST2(sth,rPixel2,-2);					\
	STORE_TO_DEST3(sth,rPixel2,-2);					\

#define EXPAND_PIXELS_1x_16TO16L_SWAPXY				\
	lhz		rPixel2,0(rSource);						\
	INCREMENT_SOURCE;								\
	lhz		rPixel1,0(rSource);						\
	INCREMENT_SOURCE;								\
	lhz		rPixel4,0(rSource);						\
	INCREMENT_SOURCE;								\
	lhz		rPixel3,0(rSource);						\
	INCREMENT_SOURCE;								\
	rlwimi	rPixel1,rPixel2,16,0,15;				\
	rlwimi	rPixel3,rPixel4,16,0,15;				\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside1,0);			\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside2,4);			\
	rlwinm	rPixel2,rPixel1,2,14,29;				\
	rlwinm	rPixel4,rPixel3,2,14,29;				\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	COMPARE_TO_LOOKASIDE(cr5,rPixel3,rLookaside2);	\
	STORE_TO_LOOKASIDE(stw,rPixel1,0);				\
	COMBINE_COMPARES(cr6*4+eq,cr1*4+eq,cr5*4+eq);	\
	STORE_TO_LOOKASIDE(stw,rPixel3,4);				\
	SKIP_IF_NOT_DIRTY(cr6,DontDraw1);				\
	rlwinm	rPixel1,rPixel1,18,14,29;				\
	rlwinm	rPixel3,rPixel3,18,14,29;				\
	lwzx	rPixel2,rLookup,rPixel2;				\
	lwzx	rPixel4,rLookup,rPixel4;				\
	lwzx	rPixel1,rLookup,rPixel1;				\
	lwzx	rPixel3,rLookup,rPixel3;				\
	rlwimi	rPixel2,rPixel1,16,0,15;		/*AB*/	\
	rlwimi	rPixel4,rPixel3,16,0,15;		/*CD*/	\
	stw		rPixel2,-8(sp);							\
	stw		rPixel4,-4(sp);							\
	lfd		fpTemp1,-8(sp);							\
	STORE_TO_DEST1(stfd,fpTemp1,0);					\
	STORE_TO_DEST2(stfd,fpTemp1,0);					\
	STORE_TO_DEST3(stfd,fpTemp1,0);					\
DontDraw1:;											\
	lhz		rPixel2,0(rSource);						\
	INCREMENT_SOURCE;								\
	lhz		rPixel1,0(rSource);						\
	INCREMENT_SOURCE;								\
	lhz		rPixel4,0(rSource);						\
	INCREMENT_SOURCE;								\
	lhz		rPixel3,0(rSource);						\
	INCREMENT_SOURCE;								\
	rlwimi	rPixel1,rPixel2,16,0,15;				\
	rlwimi	rPixel3,rPixel4,16,0,15;				\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside1,8);			\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside2,12);		\
	rlwinm	rPixel2,rPixel1,2,14,29;				\
	rlwinm	rPixel4,rPixel3,2,14,29;				\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	COMPARE_TO_LOOKASIDE(cr5,rPixel3,rLookaside2);	\
	STORE_TO_LOOKASIDE(stw,rPixel1,8);				\
	COMBINE_COMPARES(cr6*4+eq,cr1*4+eq,cr5*4+eq);	\
	STORE_TO_LOOKASIDE(stw,rPixel3,12);				\
	SKIP_IF_NOT_DIRTY(cr6,DontDrawMulti);			\
	rlwinm	rPixel1,rPixel1,18,14,29;				\
	rlwinm	rPixel3,rPixel3,18,14,29;				\
	lwzx	rPixel2,rLookup,rPixel2;				\
	lwzx	rPixel4,rLookup,rPixel4;				\
	lwzx	rPixel1,rLookup,rPixel1;				\
	lwzx	rPixel3,rLookup,rPixel3;				\
	rlwimi	rPixel2,rPixel1,16,0,15;		/*EF*/	\
	rlwimi	rPixel4,rPixel3,16,0,15;		/*GH*/	\
	stw		rPixel2,-8(sp);							\
	stw		rPixel4,-4(sp);							\
	ADD_TO_LOOKASIDE(16);							\
	ADD_TO_DEST1(16);								\
	ADD_TO_DEST2(16);								\
	ADD_TO_DEST3(16);								\
	lfd		fpTemp1,-8(sp);							\
	STORE_TO_DEST1(stfd,fpTemp1,-8);				\
	STORE_TO_DEST2(stfd,fpTemp1,-8);				\
	STORE_TO_DEST3(stfd,fpTemp1,-8);				\


//===================================================

#define EXPAND_PIXEL_2x_16TO16L(outLabel)			\
	lhz		rPixel1,X_OFFSET(0,2)(rSource);			\
	LOAD_FROM_LOOKASIDE(lhz,rLookaside1,0);			\
	addi	rSource,rSource,X_INCREMENT(2);			\
	rlwinm	rPixel2,rPixel1,2,14,29;				\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	STORE_TO_LOOKASIDE(sth,rPixel1,0);				\
	SKIP_IF_NOT_DIRTY(cr1,outLabel);				\
	lwzx	rPixel2,rLookup,rPixel2;				\
	ADD_TO_LOOKASIDE(2);							\
	ADD_TO_DEST1(4);								\
	ADD_TO_DEST2(4);								\
	ADD_TO_DEST3(4);								\
	STORE_TO_DEST1(stw,rPixel2,-4);					\
	STORE_TO_DEST2(stw,rPixel2,-4);					\
	STORE_TO_DEST3(stw,rPixel2,-4);					\

#define EXPAND_PIXELS_2x_16TO16L					\
	lwz		rPixel1,X_OFFSET(0,4)(rSource);			\
	lwz		rPixel3,X_OFFSET(4,4)(rSource);			\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside1,0);			\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside2,4);			\
	EXTRACT_PIXEL1_16(rPixel2,rPixel1,2);			\
	EXTRACT_PIXEL1_16(rPixel4,rPixel3,2);			\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	COMPARE_TO_LOOKASIDE(cr5,rPixel3,rLookaside2);	\
	STORE_TO_LOOKASIDE(stw,rPixel1,0);				\
	COMBINE_COMPARES(cr6*4+eq,cr1*4+eq,cr5*4+eq);	\
	STORE_TO_LOOKASIDE(stw,rPixel3,4);				\
	SKIP_IF_NOT_DIRTY(cr6,DontDraw1);				\
	EXTRACT_PIXEL0_16(rPixel1,rPixel1,2);			\
	EXTRACT_PIXEL0_16(rPixel3,rPixel3,2);			\
	lwzx	rPixel2,rLookup,rPixel2;		/*AA*/	\
	lwzx	rPixel4,rLookup,rPixel4;		/*BB*/	\
	lwzx	rPixel1,rLookup,rPixel1;		/*CC*/	\
	lwzx	rPixel3,rLookup,rPixel3;		/*DD*/	\
	stw		rPixel1,-16(sp);						\
	stw		rPixel2,-12(sp);						\
	stw		rPixel3,-8(sp);							\
	stw		rPixel4,-4(sp);							\
	lfd		fpTemp1,-16(sp);						\
	lfd		fpTemp2,-8(sp);							\
	SKIP_IF_NOT_DIRTY(cr1,skip1);					\
	STORE_TO_DEST1(stfd,fpTemp1,0);					\
	STORE_TO_DEST2(stfd,fpTemp1,0);					\
	STORE_TO_DEST3(stfd,fpTemp1,0);					\
	SKIP_IF_NOT_DIRTY(cr5,skip2);					\
skip1:;												\
	STORE_TO_DEST1(stfd,fpTemp2,8);					\
	STORE_TO_DEST2(stfd,fpTemp2,8);					\
	STORE_TO_DEST3(stfd,fpTemp2,8);					\
skip2:;												\
DontDraw1:;											\
	lwz		rPixel1,X_OFFSET(8,4)(rSource);			\
	lwz		rPixel3,X_OFFSET(12,4)(rSource);		\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside1,8);			\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside2,12);		\
	EXTRACT_PIXEL1_16(rPixel2,rPixel1,2);			\
	EXTRACT_PIXEL1_16(rPixel4,rPixel3,2);			\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	COMPARE_TO_LOOKASIDE(cr5,rPixel3,rLookaside2);	\
	STORE_TO_LOOKASIDE(stw,rPixel1,8);				\
	COMBINE_COMPARES(cr6*4+eq,cr1*4+eq,cr5*4+eq);	\
	STORE_TO_LOOKASIDE(stw,rPixel3,12);				\
	SKIP_IF_NOT_DIRTY(cr6,DontDrawMulti);			\
	EXTRACT_PIXEL0_16(rPixel1,rPixel1,2);			\
	EXTRACT_PIXEL0_16(rPixel3,rPixel3,2);			\
	lwzx	rPixel2,rLookup,rPixel2;		/*EE*/	\
	lwzx	rPixel4,rLookup,rPixel4;		/*FF*/	\
	lwzx	rPixel1,rLookup,rPixel1;		/*GG*/	\
	lwzx	rPixel3,rLookup,rPixel3;		/*HH*/	\
	stw		rPixel1,-16(sp);						\
	stw		rPixel2,-12(sp);						\
	stw		rPixel3,-8(sp);							\
	stw		rPixel4,-4(sp);							\
	addi	rSource,rSource,X_INCREMENT(16);		\
	ADD_TO_LOOKASIDE(16);							\
	ADD_TO_DEST1(32);								\
	ADD_TO_DEST2(32);								\
	ADD_TO_DEST3(32);								\
	lfd		fpTemp1,-16(sp);						\
	lfd		fpTemp2,-8(sp);							\
	SKIP_IF_NOT_DIRTY(cr1,skip3);					\
	STORE_TO_DEST1(stfd,fpTemp1,-16);				\
	STORE_TO_DEST2(stfd,fpTemp1,-16);				\
	STORE_TO_DEST3(stfd,fpTemp1,-16);				\
	SKIP_IF_NOT_DIRTY(cr5,skip4);					\
skip3:;												\
	STORE_TO_DEST1(stfd,fpTemp2,-8);				\
	STORE_TO_DEST2(stfd,fpTemp2,-8);				\
	STORE_TO_DEST3(stfd,fpTemp2,-8);				\
skip4:;												\


#define EXPAND_PIXEL_2x_16TO16L_SWAPXY(outLabel)	\
	lhz		rPixel1,0(rSource);						\
	LOAD_FROM_LOOKASIDE(lhz,rLookaside1,0);			\
	INCREMENT_SOURCE;								\
	rlwinm	rPixel2,rPixel1,2,14,29;				\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	STORE_TO_LOOKASIDE(sth,rPixel1,0);				\
	SKIP_IF_NOT_DIRTY(cr1,outLabel);				\
	lwzx	rPixel2,rLookup,rPixel2;				\
	ADD_TO_LOOKASIDE(2);							\
	ADD_TO_DEST1(4);								\
	ADD_TO_DEST2(4);								\
	ADD_TO_DEST3(4);								\
	STORE_TO_DEST1(stw,rPixel2,-4);					\
	STORE_TO_DEST2(stw,rPixel2,-4);					\
	STORE_TO_DEST3(stw,rPixel2,-4);					\

#define EXPAND_PIXELS_2x_16TO16L_SWAPXY				\
	lhz		rPixel2,0(rSource);						\
	INCREMENT_SOURCE;								\
	lhz		rPixel1,0(rSource);						\
	INCREMENT_SOURCE;								\
	lhz		rPixel4,0(rSource);						\
	INCREMENT_SOURCE;								\
	lhz		rPixel3,0(rSource);						\
	INCREMENT_SOURCE;								\
	rlwimi	rPixel1,rPixel2,16,0,15;				\
	rlwimi	rPixel3,rPixel4,16,0,15;				\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside1,0);			\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside2,4);			\
	rlwinm	rPixel2,rPixel1,2,14,29;				\
	rlwinm	rPixel4,rPixel3,2,14,29;				\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	COMPARE_TO_LOOKASIDE(cr5,rPixel3,rLookaside2);	\
	STORE_TO_LOOKASIDE(stw,rPixel1,0);				\
	COMBINE_COMPARES(cr6*4+eq,cr1*4+eq,cr5*4+eq);	\
	STORE_TO_LOOKASIDE(stw,rPixel3,4);				\
	SKIP_IF_NOT_DIRTY(cr6,DontDraw1);				\
	rlwinm	rPixel1,rPixel1,18,14,29;				\
	rlwinm	rPixel3,rPixel3,18,14,29;				\
	lwzx	rPixel2,rLookup,rPixel2;		/*AA*/	\
	lwzx	rPixel4,rLookup,rPixel4;		/*BB*/	\
	lwzx	rPixel1,rLookup,rPixel1;		/*CC*/	\
	lwzx	rPixel3,rLookup,rPixel3;		/*DD*/	\
	stw		rPixel1,-16(sp);						\
	stw		rPixel2,-12(sp);						\
	stw		rPixel3,-8(sp);							\
	stw		rPixel4,-4(sp);							\
	lfd		fpTemp1,-16(sp);						\
	lfd		fpTemp2,-8(sp);							\
	SKIP_IF_NOT_DIRTY(cr1,skip1);					\
	STORE_TO_DEST1(stfd,fpTemp1,0);					\
	STORE_TO_DEST2(stfd,fpTemp1,0);					\
	STORE_TO_DEST3(stfd,fpTemp1,0);					\
	SKIP_IF_NOT_DIRTY(cr5,skip2);					\
skip1:;												\
	STORE_TO_DEST1(stfd,fpTemp2,8);					\
	STORE_TO_DEST2(stfd,fpTemp2,8);					\
	STORE_TO_DEST3(stfd,fpTemp2,8);					\
skip2:;												\
DontDraw1:;											\
	lhz		rPixel2,0(rSource);						\
	INCREMENT_SOURCE;								\
	lhz		rPixel1,0(rSource);						\
	INCREMENT_SOURCE;								\
	lhz		rPixel4,0(rSource);						\
	INCREMENT_SOURCE;								\
	lhz		rPixel3,0(rSource);						\
	INCREMENT_SOURCE;								\
	rlwimi	rPixel1,rPixel2,16,0,15;				\
	rlwimi	rPixel3,rPixel4,16,0,15;				\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside1,8);			\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside2,12);		\
	rlwinm	rPixel2,rPixel1,2,14,29;				\
	rlwinm	rPixel4,rPixel3,2,14,29;				\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	COMPARE_TO_LOOKASIDE(cr5,rPixel3,rLookaside2);	\
	STORE_TO_LOOKASIDE(stw,rPixel1,8);				\
	COMBINE_COMPARES(cr6*4+eq,cr1*4+eq,cr5*4+eq);	\
	STORE_TO_LOOKASIDE(stw,rPixel3,12);				\
	SKIP_IF_NOT_DIRTY(cr6,DontDrawMulti);			\
	rlwinm	rPixel1,rPixel1,18,14,29;				\
	rlwinm	rPixel3,rPixel3,18,14,29;				\
	lwzx	rPixel2,rLookup,rPixel2;		/*EE*/	\
	lwzx	rPixel4,rLookup,rPixel4;		/*FF*/	\
	lwzx	rPixel1,rLookup,rPixel1;		/*GG*/	\
	lwzx	rPixel3,rLookup,rPixel3;		/*HH*/	\
	stw		rPixel1,-16(sp);						\
	stw		rPixel2,-12(sp);						\
	stw		rPixel3,-8(sp);							\
	stw		rPixel4,-4(sp);							\
	ADD_TO_LOOKASIDE(16);							\
	ADD_TO_DEST1(32);								\
	ADD_TO_DEST2(32);								\
	ADD_TO_DEST3(32);								\
	lfd		fpTemp1,-16(sp);						\
	lfd		fpTemp2,-8(sp);							\
	SKIP_IF_NOT_DIRTY(cr1,skip3);					\
	STORE_TO_DEST1(stfd,fpTemp1,-16);				\
	STORE_TO_DEST2(stfd,fpTemp1,-16);				\
	STORE_TO_DEST3(stfd,fpTemp1,-16);				\
	SKIP_IF_NOT_DIRTY(cr5,skip4);					\
skip3:;												\
	STORE_TO_DEST1(stfd,fpTemp2,-8);				\
	STORE_TO_DEST2(stfd,fpTemp2,-8);				\
	STORE_TO_DEST3(stfd,fpTemp2,-8);				\
skip4:;												\


//===================================================

#define EXPAND_PIXEL_3x_16TO16L(outLabel)			\
	lhz		rPixel1,X_OFFSET(0,2)(rSource);			\
	LOAD_FROM_LOOKASIDE(lhz,rLookaside1,0);			\
	addi	rSource,rSource,X_INCREMENT(2);			\
	rlwinm	rPixel2,rPixel1,2,14,29;				\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	STORE_TO_LOOKASIDE(sth,rPixel1,0);				\
	SKIP_IF_NOT_DIRTY(cr1,outLabel);				\
	lwzx	rPixel2,rLookup,rPixel2;				\
	ADD_TO_LOOKASIDE(2);							\
	ADD_TO_DEST1(6);								\
	ADD_TO_DEST2(6);								\
	ADD_TO_DEST3(6);								\
	STORE_TO_DEST1(stw,rPixel2,-6);					\
	STORE_TO_DEST1(sth,rPixel2,-2);					\
	STORE_TO_DEST2(stw,rPixel2,-6);					\
	STORE_TO_DEST2(sth,rPixel2,-2);					\
	STORE_TO_DEST3(stw,rPixel2,-6);					\
	STORE_TO_DEST3(sth,rPixel2,-2);					\

#define EXPAND_PIXELS_3x_16TO16L					\
	lwz		rPixel1,X_OFFSET(0,4)(rSource);			\
	lwz		rPixel4,X_OFFSET(4,4)(rSource);			\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside1,0);			\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside2,4);			\
	EXTRACT_PIXEL1_16(rPixel2,rPixel1,2);			\
	EXTRACT_PIXEL1_16(rPixel5,rPixel4,2);			\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	COMPARE_TO_LOOKASIDE(cr5,rPixel4,rLookaside2);	\
	STORE_TO_LOOKASIDE(stw,rPixel1,0);				\
	COMBINE_COMPARES(cr6*4+eq,cr1*4+eq,cr5*4+eq);	\
	STORE_TO_LOOKASIDE(stw,rPixel4,4);				\
	SKIP_IF_NOT_DIRTY(cr6,DontDraw1);				\
	EXTRACT_PIXEL0_16(rPixel1,rPixel1,2);			\
	EXTRACT_PIXEL0_16(rPixel4,rPixel4,2);			\
	lwzx	rPixel2,rLookup,rPixel2;		/*BB*/	\
	lwzx	rPixel5,rLookup,rPixel5;		/*DD*/	\
	lwzx	rPixel1,rLookup,rPixel1;		/*AA*/	\
	lwzx	rPixel4,rLookup,rPixel4;		/*CC*/	\
	mr		rPixel3,rPixel2;				/*BB*/	\
	mr		rPixel6,rPixel5;				/*DD*/	\
	rlwimi	rPixel2,rPixel1,16,0,15;		/*AB*/	\
	rlwimi	rPixel5,rPixel4,16,0,15;		/*CD*/	\
	stw		rPixel1,-24(sp);						\
	stw		rPixel2,-20(sp);						\
	stw		rPixel3,-16(sp);						\
	stw		rPixel4,-12(sp);						\
	stw		rPixel5,-8(sp);							\
	stw		rPixel6,-4(sp);							\
	lfd		fpTemp1,-24(sp);						\
	lfd		fpTemp2,-16(sp);						\
	lfd		fpTemp3,-8(sp);							\
	SKIP_IF_NOT_DIRTY(cr1,skip1);					\
	STORE_TO_DEST1(stfd,fpTemp1,0);					\
	STORE_TO_DEST2(stfd,fpTemp1,0);					\
	STORE_TO_DEST3(stfd,fpTemp1,0);					\
skip1:;												\
	STORE_TO_DEST1(stfd,fpTemp2,8);					\
	STORE_TO_DEST2(stfd,fpTemp2,8);					\
	STORE_TO_DEST3(stfd,fpTemp2,8);					\
	SKIP_IF_NOT_DIRTY(cr5,skip2);					\
	STORE_TO_DEST1(stfd,fpTemp3,16);				\
	STORE_TO_DEST2(stfd,fpTemp3,16);				\
	STORE_TO_DEST3(stfd,fpTemp3,16);				\
skip2:;												\
DontDraw1:;											\
	lwz		rPixel1,X_OFFSET(8,4)(rSource);			\
	lwz		rPixel4,X_OFFSET(12,4)(rSource);		\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside1,8);			\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside2,12);		\
	EXTRACT_PIXEL1_16(rPixel2,rPixel1,2);			\
	EXTRACT_PIXEL1_16(rPixel5,rPixel4,2);			\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	COMPARE_TO_LOOKASIDE(cr5,rPixel4,rLookaside2);	\
	STORE_TO_LOOKASIDE(stw,rPixel1,8);				\
	COMBINE_COMPARES(cr6*4+eq,cr1*4+eq,cr5*4+eq);	\
	STORE_TO_LOOKASIDE(stw,rPixel4,12);				\
	SKIP_IF_NOT_DIRTY(cr6,DontDrawMulti);			\
	EXTRACT_PIXEL0_16(rPixel1,rPixel1,2);			\
	EXTRACT_PIXEL0_16(rPixel4,rPixel4,2);			\
	lwzx	rPixel2,rLookup,rPixel2;		/*FF*/	\
	lwzx	rPixel5,rLookup,rPixel5;		/*HH*/	\
	lwzx	rPixel1,rLookup,rPixel1;		/*EE*/	\
	lwzx	rPixel4,rLookup,rPixel4;		/*GG*/	\
	mr		rPixel3,rPixel2;				/*FF*/	\
	mr		rPixel6,rPixel5;				/*HH*/	\
	rlwimi	rPixel2,rPixel1,16,0,15;		/*EF*/	\
	rlwimi	rPixel5,rPixel4,16,0,15;		/*GH*/	\
	stw		rPixel1,-24(sp);						\
	stw		rPixel2,-20(sp);						\
	stw		rPixel3,-16(sp);						\
	stw		rPixel4,-12(sp);						\
	stw		rPixel5,-8(sp);							\
	stw		rPixel6,-4(sp);							\
	addi	rSource,rSource,X_INCREMENT(16);		\
	ADD_TO_LOOKASIDE(16);							\
	ADD_TO_DEST1(48);								\
	ADD_TO_DEST2(48);								\
	ADD_TO_DEST3(48);								\
	lfd		fpTemp1,-24(sp);						\
	lfd		fpTemp2,-16(sp);						\
	lfd		fpTemp3,-8(sp);							\
	SKIP_IF_NOT_DIRTY(cr1,skip3);					\
	STORE_TO_DEST1(stfd,fpTemp1,-24);				\
	STORE_TO_DEST2(stfd,fpTemp1,-24);				\
	STORE_TO_DEST3(stfd,fpTemp1,-24);				\
skip3:;												\
	STORE_TO_DEST1(stfd,fpTemp2,-16);				\
	STORE_TO_DEST2(stfd,fpTemp2,-16);				\
	STORE_TO_DEST3(stfd,fpTemp2,-16);				\
	SKIP_IF_NOT_DIRTY(cr5,skip4);					\
	STORE_TO_DEST1(stfd,fpTemp3,-8);				\
	STORE_TO_DEST2(stfd,fpTemp3,-8);				\
	STORE_TO_DEST3(stfd,fpTemp3,-8);				\
skip4:;												\


#define EXPAND_PIXEL_3x_16TO16L_SWAPXY(outLabel)	\
	lhz		rPixel1,0(rSource);						\
	LOAD_FROM_LOOKASIDE(lhz,rLookaside1,0);			\
	INCREMENT_SOURCE;								\
	rlwinm	rPixel2,rPixel1,2,14,29;				\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	STORE_TO_LOOKASIDE(sth,rPixel1,0);				\
	SKIP_IF_NOT_DIRTY(cr1,outLabel);				\
	lwzx	rPixel2,rLookup,rPixel2;				\
	ADD_TO_LOOKASIDE(2);							\
	ADD_TO_DEST1(6);								\
	ADD_TO_DEST2(6);								\
	ADD_TO_DEST3(6);								\
	STORE_TO_DEST1(stw,rPixel2,-6);					\
	STORE_TO_DEST1(sth,rPixel2,-2);					\
	STORE_TO_DEST2(stw,rPixel2,-6);					\
	STORE_TO_DEST2(sth,rPixel2,-2);					\
	STORE_TO_DEST3(stw,rPixel2,-6);					\
	STORE_TO_DEST3(sth,rPixel2,-2);					\

#define EXPAND_PIXELS_3x_16TO16L_SWAPXY				\
	lhz		rPixel2,0(rSource);						\
	INCREMENT_SOURCE;								\
	lhz		rPixel1,0(rSource);						\
	INCREMENT_SOURCE;								\
	lhz		rPixel5,0(rSource);						\
	INCREMENT_SOURCE;								\
	lhz		rPixel4,0(rSource);						\
	INCREMENT_SOURCE;								\
	rlwimi	rPixel1,rPixel2,16,0,15;				\
	rlwimi	rPixel4,rPixel5,16,0,15;				\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside1,0);			\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside2,4);			\
	rlwinm	rPixel2,rPixel1,2,14,29;				\
	rlwinm	rPixel5,rPixel4,2,14,29;				\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	COMPARE_TO_LOOKASIDE(cr5,rPixel4,rLookaside2);	\
	STORE_TO_LOOKASIDE(stw,rPixel1,0);				\
	COMBINE_COMPARES(cr6*4+eq,cr1*4+eq,cr5*4+eq);	\
	STORE_TO_LOOKASIDE(stw,rPixel4,4);				\
	SKIP_IF_NOT_DIRTY(cr6,DontDraw1);				\
	rlwinm	rPixel1,rPixel1,18,14,29;				\
	rlwinm	rPixel4,rPixel4,18,14,29;				\
	lwzx	rPixel2,rLookup,rPixel2;		/*BB*/	\
	lwzx	rPixel5,rLookup,rPixel5;		/*DD*/	\
	lwzx	rPixel1,rLookup,rPixel1;		/*AA*/	\
	lwzx	rPixel4,rLookup,rPixel4;		/*CC*/	\
	mr		rPixel3,rPixel2;				/*BB*/	\
	mr		rPixel6,rPixel5;				/*DD*/	\
	rlwimi	rPixel2,rPixel1,16,0,15;		/*AB*/	\
	rlwimi	rPixel5,rPixel4,16,0,15;		/*CD*/	\
	stw		rPixel1,-24(sp);						\
	stw		rPixel2,-20(sp);						\
	stw		rPixel3,-16(sp);						\
	stw		rPixel4,-12(sp);						\
	stw		rPixel5,-8(sp);							\
	stw		rPixel6,-4(sp);							\
	lfd		fpTemp1,-24(sp);						\
	lfd		fpTemp2,-16(sp);						\
	lfd		fpTemp3,-8(sp);							\
	SKIP_IF_NOT_DIRTY(cr1,skip1);					\
	STORE_TO_DEST1(stfd,fpTemp1,0);					\
	STORE_TO_DEST2(stfd,fpTemp1,0);					\
	STORE_TO_DEST3(stfd,fpTemp1,0);					\
skip1:;												\
	STORE_TO_DEST1(stfd,fpTemp2,8);					\
	STORE_TO_DEST2(stfd,fpTemp2,8);					\
	STORE_TO_DEST3(stfd,fpTemp2,8);					\
	SKIP_IF_NOT_DIRTY(cr5,skip2);					\
	STORE_TO_DEST1(stfd,fpTemp3,16);				\
	STORE_TO_DEST2(stfd,fpTemp3,16);				\
	STORE_TO_DEST3(stfd,fpTemp3,16);				\
skip2:;												\
DontDraw1:;											\
	lhz		rPixel2,0(rSource);						\
	INCREMENT_SOURCE;								\
	lhz		rPixel1,0(rSource);						\
	INCREMENT_SOURCE;								\
	lhz		rPixel5,0(rSource);						\
	INCREMENT_SOURCE;								\
	lhz		rPixel4,0(rSource);						\
	INCREMENT_SOURCE;								\
	rlwimi	rPixel1,rPixel2,16,0,15;				\
	rlwimi	rPixel4,rPixel5,16,0,15;				\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside1,8);			\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside2,12);		\
	rlwinm	rPixel2,rPixel1,2,14,29;				\
	rlwinm	rPixel5,rPixel4,2,14,29;				\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	COMPARE_TO_LOOKASIDE(cr5,rPixel4,rLookaside2);	\
	STORE_TO_LOOKASIDE(stw,rPixel1,8);				\
	COMBINE_COMPARES(cr6*4+eq,cr1*4+eq,cr5*4+eq);	\
	STORE_TO_LOOKASIDE(stw,rPixel4,12);				\
	SKIP_IF_NOT_DIRTY(cr6,DontDrawMulti);			\
	rlwinm	rPixel1,rPixel1,18,14,29;				\
	rlwinm	rPixel4,rPixel4,18,14,29;				\
	lwzx	rPixel2,rLookup,rPixel2;		/*FF*/	\
	lwzx	rPixel5,rLookup,rPixel5;		/*HH*/	\
	lwzx	rPixel1,rLookup,rPixel1;		/*EE*/	\
	lwzx	rPixel4,rLookup,rPixel4;		/*GG*/	\
	mr		rPixel3,rPixel2;				/*FF*/	\
	mr		rPixel6,rPixel5;				/*HH*/	\
	rlwimi	rPixel2,rPixel1,16,0,15;		/*EF*/	\
	rlwimi	rPixel5,rPixel4,16,0,15;		/*GH*/	\
	stw		rPixel1,-24(sp);						\
	stw		rPixel2,-20(sp);						\
	stw		rPixel3,-16(sp);						\
	stw		rPixel4,-12(sp);						\
	stw		rPixel5,-8(sp);							\
	stw		rPixel6,-4(sp);							\
	ADD_TO_LOOKASIDE(16);							\
	ADD_TO_DEST1(48);								\
	ADD_TO_DEST2(48);								\
	ADD_TO_DEST3(48);								\
	lfd		fpTemp1,-24(sp);						\
	lfd		fpTemp2,-16(sp);						\
	lfd		fpTemp3,-8(sp);							\
	SKIP_IF_NOT_DIRTY(cr1,skip3);					\
	STORE_TO_DEST1(stfd,fpTemp1,-24);				\
	STORE_TO_DEST2(stfd,fpTemp1,-24);				\
	STORE_TO_DEST3(stfd,fpTemp1,-24);				\
skip3:;												\
	STORE_TO_DEST1(stfd,fpTemp2,-16);				\
	STORE_TO_DEST2(stfd,fpTemp2,-16);				\
	STORE_TO_DEST3(stfd,fpTemp2,-16);				\
	SKIP_IF_NOT_DIRTY(cr5,skip4);					\
	STORE_TO_DEST1(stfd,fpTemp3,-8);				\
	STORE_TO_DEST2(stfd,fpTemp3,-8);				\
	STORE_TO_DEST3(stfd,fpTemp3,-8);				\
skip4:;												\


//===================================================

#define EXPAND_PIXEL_1x_16TO16L_VEC(outLabel)		\
	lhz		rPixel1,X_OFFSET(0,2)(rSource);			\
	LOAD_FROM_LOOKASIDE(lhz,rLookaside1,0);			\
	addi	rSource,rSource,X_INCREMENT(2);			\
	rlwinm	rPixel2,rPixel1,2,14,29;				\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	STORE_TO_LOOKASIDE(sth,rPixel1,0);				\
	SKIP_IF_NOT_DIRTY(cr1,outLabel);				\
	lwzx	rPixel2,rLookup,rPixel2;				\
	ADD_TO_LOOKASIDE(2);							\
	STORE_TO_DEST1(sth,rPixel2,0);					\
	STORE_TO_DEST2(sthx,rPixel2);					\
	STORE_TO_DEST3(sthx,rPixel2);					\
	ADD_TO_DEST(2);									\

#define EXPAND_PIXELS_1x_16TO16L_VEC										\
	lvx		vrTemp2,0,rSource;				/* load source */				\
	addi	rSource,rSource,X_INCREMENT(16);								\
	LOAD_FROM_LOOKASIDE_VEC(vrTemp1);		/* load 8 pixels of lookaside */\
	COMPARE_TO_LOOKASIDE_VEC(vrTemp1,vrTemp1,vrTemp2);	/* compare all 8 pixels */	\
	SKIP_IF_NOT_DIRTY_VEC(DontDrawMulti);	/* skip if it is clean */		\
	STORE_TO_LOOKASIDE_VEC(vrTemp2);		/* save current source to lookaside */	\
													\
	vand	vrTemp2,vrTemp2,vrOffsetMask;			\
	lwz		rPixel1,X_OFFSET(0,4)-X_INCREMENT(16)(rSource);					\
	vslh	vrTemp2,vrTemp2,vrOffsetShift;			\
	lwz		rPixel3,X_OFFSET(4,4)-X_INCREMENT(16)(rSource);					\
	vperm	vrTemp2,vrTemp2,vrTemp2,vrOffsetPerm;	\
	lwz		rPixel5,X_OFFSET(8,4)-X_INCREMENT(16)(rSource);					\
	EXTRACT_PIXEL1_16(rPixel2,rPixel1,2);			\
	lwz		rPixel7,X_OFFSET(12,4)-X_INCREMENT(16)(rSource);				\
	EXTRACT_PIXEL0_16(rPixel1,rPixel1,2);			\
	vaddubm	vrTemp2,vrTemp2,vrOffsetOffset;			\
	EXTRACT_PIXEL1_16(rPixel4,rPixel3,2);			\
	lvehx	vrPixel2,rLookup,rPixel2;				\
	EXTRACT_PIXEL0_16(rPixel3,rPixel3,2);			\
	lvehx	vrPixel1,rLookup,rPixel1;				\
	EXTRACT_PIXEL1_16(rPixel6,rPixel5,2);			\
	lvehx	vrPixel4,rLookup,rPixel4;				\
	EXTRACT_PIXEL0_16(rPixel5,rPixel5,2);			\
	lvehx	vrPixel3,rLookup,rPixel3;				\
	EXTRACT_PIXEL1_16(rPixel8,rPixel7,2);			\
	lvehx	vrPixel6,rLookup,rPixel6;				\
	vperm	vrPixel1,vrPixel1,vrPixel2,vrTemp2;		\
	lvehx	vrPixel5,rLookup,rPixel5;				\
	EXTRACT_PIXEL0_16(rPixel7,rPixel7,2);			\
	lvehx	vrPixel8,rLookup,rPixel8;				\
	vperm	vrPixel3,vrPixel3,vrPixel4,vrTemp2;		\
	lvehx	vrPixel7,rLookup,rPixel7;				\
	vperm	vrPixel5,vrPixel5,vrPixel6,vrTemp2;		\
	vsel	vrPixel1,vrPixel1,vrPixel3,vrPixelMerge1;\
	vperm	vrPixel7,vrPixel7,vrPixel8,vrTemp2;		\
	vsel	vrPixel5,vrPixel5,vrPixel7,vrPixelMerge1;\
													\
	ADD_TO_LOOKASIDE(16);							\
	vsel	vrPixel1,vrPixel1,vrPixel5,vrPixelMerge2;\
	STORE_TO_DEST1_VEC(vrPixel1);			/* ABCDEFGH to line 1 */		\
	STORE_TO_DEST2_VEC(vrPixel1);			/* ABCDEFGH to line 2 */		\
	STORE_TO_DEST3_VEC(vrPixel1);			/* ABCDEFGH to line 3 */		\
	ADD_TO_DEST(16);														\


#define EXPAND_PIXEL_2x_16TO16L_VEC(outLabel)		\
	lhz		rPixel1,X_OFFSET(0,2)(rSource);			\
	LOAD_FROM_LOOKASIDE(lhz,rLookaside1,0);			\
	addi	rSource,rSource,X_INCREMENT(2);			\
	rlwinm	rPixel2,rPixel1,2,14,29;				\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	STORE_TO_LOOKASIDE(sth,rPixel1,0);				\
	SKIP_IF_NOT_DIRTY(cr1,outLabel);				\
	lwzx	rPixel2,rLookup,rPixel2;				\
	ADD_TO_LOOKASIDE(2);							\
	STORE_TO_DEST1(stw,rPixel2,0);					\
	STORE_TO_DEST2(stwx,rPixel2);					\
	STORE_TO_DEST3(stwx,rPixel2);					\
	ADD_TO_DEST(4);									\

#define EXPAND_PIXELS_2x_16TO16L_VEC										\
	lvx		vrTemp2,0,rSource;				/* load source */				\
	addi	rSource,rSource,X_INCREMENT(16);								\
	LOAD_FROM_LOOKASIDE_VEC(vrTemp1);		/* load 8 pixels of lookaside */\
	COMPARE_TO_LOOKASIDE_VEC(vrTemp1,vrTemp1,vrTemp2);	/* compare all 8 pixels */	\
	SKIP_IF_NOT_DIRTY_VEC(DontDrawMulti);	/* skip if it is clean */		\
	STORE_TO_LOOKASIDE_VEC(vrTemp2);		/* save current source to lookaside */	\
													\
	vand	vrTemp2,vrTemp2,vrOffsetMask;			\
	lwz		rPixel1,X_OFFSET(0,4)-X_INCREMENT(16)(rSource);					\
	vslh	vrTemp2,vrTemp2,vrOffsetShift;			\
	lwz		rPixel3,X_OFFSET(4,4)-X_INCREMENT(16)(rSource);					\
	vperm	vrTemp2,vrTemp2,vrTemp2,vrOffsetPerm;	\
	lwz		rPixel5,X_OFFSET(8,4)-X_INCREMENT(16)(rSource);					\
	EXTRACT_PIXEL1_16(rPixel2,rPixel1,2);			\
	lwz		rPixel7,X_OFFSET(12,4)-X_INCREMENT(16)(rSource);				\
	EXTRACT_PIXEL0_16(rPixel1,rPixel1,2);			\
	vaddubm	vrTemp2,vrTemp2,vrOffsetOffset;			\
	EXTRACT_PIXEL1_16(rPixel4,rPixel3,2);			\
	lvehx	vrPixel2,rLookup,rPixel2;				\
	EXTRACT_PIXEL0_16(rPixel3,rPixel3,2);			\
	lvehx	vrPixel1,rLookup,rPixel1;				\
	EXTRACT_PIXEL1_16(rPixel6,rPixel5,2);			\
	lvehx	vrPixel4,rLookup,rPixel4;				\
	EXTRACT_PIXEL0_16(rPixel5,rPixel5,2);			\
	lvehx	vrPixel3,rLookup,rPixel3;				\
	EXTRACT_PIXEL1_16(rPixel8,rPixel7,2);			\
	lvehx	vrPixel6,rLookup,rPixel6;				\
	vperm	vrPixel1,vrPixel1,vrPixel2,vrTemp2;		\
	lvehx	vrPixel5,rLookup,rPixel5;				\
	EXTRACT_PIXEL0_16(rPixel7,rPixel7,2);			\
	lvehx	vrPixel8,rLookup,rPixel8;				\
	vperm	vrPixel3,vrPixel3,vrPixel4,vrTemp2;		\
	lvehx	vrPixel7,rLookup,rPixel7;				\
	vperm	vrPixel5,vrPixel5,vrPixel6,vrTemp2;		\
	vperm	vrPixel7,vrPixel7,vrPixel8,vrTemp2;		\
													\
	ADD_TO_LOOKASIDE(16);							\
	vperm	vrTemp0,vrPixel1,vrPixel3,vrPixelZoom0;/* AABBCCDD */			\
	vperm	vrTemp1,vrPixel5,vrPixel7,vrPixelZoom1;/* EEFFGGHH */			\
	STORE_TO_DEST1_VEC(vrTemp0);			/* AABBCCDD to line 1 */		\
	STORE_TO_DEST2_VEC(vrTemp0);			/* AABBCCDD to line 2 */		\
	STORE_TO_DEST3_VEC(vrTemp0);			/* AABBCCDD to line 3 */		\
	ADD_TO_DEST(16);														\
	STORE_TO_DEST1_VEC(vrTemp1);			/* EEFFGGHH to line 1 */		\
	STORE_TO_DEST2_VEC(vrTemp1);			/* EEFFGGHH to line 2 */		\
	STORE_TO_DEST3_VEC(vrTemp1);			/* EEFFGGHH to line 3 */		\
	ADD_TO_DEST(16);														\


#define EXPAND_PIXEL_3x_16TO16L_VEC(outLabel)		\
	lhz		rPixel1,X_OFFSET(0,2)(rSource);			\
	LOAD_FROM_LOOKASIDE(lhz,rLookaside1,0);			\
	addi	rSource,rSource,X_INCREMENT(2);			\
	rlwinm	rPixel2,rPixel1,2,14,29;				\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	STORE_TO_LOOKASIDE(sth,rPixel1,0);				\
	SKIP_IF_NOT_DIRTY(cr1,outLabel);				\
	lwzx	rPixel2,rLookup,rPixel2;				\
	ADD_TO_LOOKASIDE(2);							\
	STORE_TO_DEST1(stw,rPixel2,0);					\
	STORE_TO_DEST2(stwx,rPixel2);					\
	STORE_TO_DEST3(stwx,rPixel2);					\
	STORE_TO_DEST1(sthu,rPixel2,4);					\
	STORE_TO_DEST2(sthx,rPixel2);					\
	STORE_TO_DEST3(sthx,rPixel2);					\
	ADD_TO_DEST(2);									\

#define EXPAND_PIXELS_3x_16TO16L_VEC										\
	lvx		vrTemp2,0,rSource;				/* load source */				\
	addi	rSource,rSource,X_INCREMENT(16);								\
	LOAD_FROM_LOOKASIDE_VEC(vrTemp1);		/* load 8 pixels of lookaside */\
	COMPARE_TO_LOOKASIDE_VEC(vrTemp1,vrTemp1,vrTemp2);	/* compare all 8 pixels */	\
	SKIP_IF_NOT_DIRTY_VEC(DontDrawMulti);	/* skip if it is clean */		\
	STORE_TO_LOOKASIDE_VEC(vrTemp2);		/* save current source to lookaside */	\
													\
	vand	vrTemp2,vrTemp2,vrOffsetMask;			\
	lwz		rPixel1,X_OFFSET(0,4)-X_INCREMENT(16)(rSource);					\
	vslh	vrTemp2,vrTemp2,vrOffsetShift;			\
	lwz		rPixel3,X_OFFSET(4,4)-X_INCREMENT(16)(rSource);					\
	vperm	vrTemp2,vrTemp2,vrTemp2,vrOffsetPerm;	\
	lwz		rPixel5,X_OFFSET(8,4)-X_INCREMENT(16)(rSource);					\
	EXTRACT_PIXEL1_16(rPixel2,rPixel1,2);			\
	lwz		rPixel7,X_OFFSET(12,4)-X_INCREMENT(16)(rSource);				\
	EXTRACT_PIXEL0_16(rPixel1,rPixel1,2);			\
	vaddubm	vrTemp2,vrTemp2,vrOffsetOffset;			\
	EXTRACT_PIXEL1_16(rPixel4,rPixel3,2);			\
	lvehx	vrPixel2,rLookup,rPixel2;				\
	EXTRACT_PIXEL0_16(rPixel3,rPixel3,2);			\
	lvehx	vrPixel1,rLookup,rPixel1;				\
	EXTRACT_PIXEL1_16(rPixel6,rPixel5,2);			\
	lvehx	vrPixel4,rLookup,rPixel4;				\
	EXTRACT_PIXEL0_16(rPixel5,rPixel5,2);			\
	lvehx	vrPixel3,rLookup,rPixel3;				\
	EXTRACT_PIXEL1_16(rPixel8,rPixel7,2);			\
	lvehx	vrPixel6,rLookup,rPixel6;				\
	vperm	vrPixel1,vrPixel1,vrPixel2,vrTemp2;		\
	lvehx	vrPixel5,rLookup,rPixel5;				\
	EXTRACT_PIXEL0_16(rPixel7,rPixel7,2);			\
	lvehx	vrPixel8,rLookup,rPixel8;				\
	vperm	vrPixel3,vrPixel3,vrPixel4,vrTemp2;		\
	lvehx	vrPixel7,rLookup,rPixel7;				\
	vperm	vrPixel5,vrPixel5,vrPixel6,vrTemp2;		\
	vperm	vrPixel7,vrPixel7,vrPixel8,vrTemp2;		\
													\
	ADD_TO_LOOKASIDE(16);							\
	vperm	vrTemp0,vrPixel1,vrPixel3,vrPixelZoom0;	/* AAABBBCC */			\
	vperm	vrTemp1,vrPixel3,vrPixel5,vrPixelZoom1;	/* CDDDEEEF */			\
	STORE_TO_DEST1_VEC(vrTemp0);			/* AAABBBCC to line 1 */		\
	STORE_TO_DEST2_VEC(vrTemp0);			/* AAABBBCC to line 2 */		\
	STORE_TO_DEST3_VEC(vrTemp0);			/* AAABBBCC to line 3 */		\
	ADD_TO_DEST(16);														\
	vperm	vrTemp2,vrPixel5,vrPixel7,vrPixelZoom2;	/* FFGGGHHH */			\
	STORE_TO_DEST1_VEC(vrTemp1);			/* CDDDEEEF to line 1 */		\
	STORE_TO_DEST2_VEC(vrTemp1);			/* CDDDEEEF to line 2 */		\
	STORE_TO_DEST3_VEC(vrTemp1);			/* CDDDEEEF to line 3 */		\
	ADD_TO_DEST(16);														\
	STORE_TO_DEST1_VEC(vrTemp2);			/* FFGGGHHH to line 1 */		\
	STORE_TO_DEST2_VEC(vrTemp2);			/* FFGGGHHH to line 2 */		\
	STORE_TO_DEST3_VEC(vrTemp2);			/* FFGGGHHH to line 3 */		\
	ADD_TO_DEST(16);														\


//===================================================
#pragma mark Expand 16 to 32

#define EXPAND_PIXEL_1x_16TO32L(outLabel)			\
	lhz		rPixel1,X_OFFSET(0,2)(rSource);			\
	LOAD_FROM_LOOKASIDE(lhz,rLookaside1,0);			\
	addi	rSource,rSource,X_INCREMENT(2);			\
	rlwinm	rPixel2,rPixel1,3,13,28;				\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	STORE_TO_LOOKASIDE(sth,rPixel1,0);				\
	SKIP_IF_NOT_DIRTY(cr1,outLabel);				\
	lwzx	rPixel2,rLookup,rPixel2;				\
	ADD_TO_LOOKASIDE(2);							\
	ADD_TO_DEST1(4);								\
	ADD_TO_DEST2(4);								\
	ADD_TO_DEST3(4);								\
	STORE_TO_DEST1(stw,rPixel2,-4);					\
	STORE_TO_DEST2(stw,rPixel2,-4);					\
	STORE_TO_DEST3(stw,rPixel2,-4);					\

#define EXPAND_PIXELS_1x_16TO32L					\
	lwz		rPixel1,X_OFFSET(0,4)(rSource);			\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside1,0);			\
	EXTRACT_PIXEL1_16(rPixel2,rPixel1,3);			\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	STORE_TO_LOOKASIDE(stw,rPixel1,0);				\
	SKIP_IF_NOT_DIRTY(cr1,DontDraw1);				\
	EXTRACT_PIXEL0_16(rPixel1,rPixel1,3);			\
	lwzx	rPixel2,rLookup,rPixel2;				\
	lwzx	rPixel1,rLookup,rPixel1;				\
	stw		rPixel2,-4(sp);							\
	stw		rPixel1,-8(sp);							\
	lfd		fpTemp1,-8(sp);							\
	STORE_TO_DEST1(stfd,fpTemp1,0);					\
	STORE_TO_DEST2(stfd,fpTemp1,0);					\
	STORE_TO_DEST3(stfd,fpTemp1,0);					\
DontDraw1:;											\
	lwz		rPixel1,X_OFFSET(4,4)(rSource);			\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside1,4);			\
	EXTRACT_PIXEL1_16(rPixel2,rPixel1,3);			\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	STORE_TO_LOOKASIDE(stw,rPixel1,4);				\
	SKIP_IF_NOT_DIRTY(cr1,DontDraw2);				\
	EXTRACT_PIXEL0_16(rPixel1,rPixel1,3);			\
	lwzx	rPixel2,rLookup,rPixel2;				\
	lwzx	rPixel1,rLookup,rPixel1;				\
	stw		rPixel2,-4(sp);							\
	stw		rPixel1,-8(sp);							\
	lfd		fpTemp1,-8(sp);							\
	STORE_TO_DEST1(stfd,fpTemp1,8);					\
	STORE_TO_DEST2(stfd,fpTemp1,8);					\
	STORE_TO_DEST3(stfd,fpTemp1,8);					\
DontDraw2:;											\
	lwz		rPixel1,X_OFFSET(8,4)(rSource);			\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside1,8);			\
	EXTRACT_PIXEL1_16(rPixel2,rPixel1,3);			\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	STORE_TO_LOOKASIDE(stw,rPixel1,8);				\
	SKIP_IF_NOT_DIRTY(cr1,DontDraw3);				\
	EXTRACT_PIXEL0_16(rPixel1,rPixel1,3);			\
	lwzx	rPixel2,rLookup,rPixel2;				\
	lwzx	rPixel1,rLookup,rPixel1;				\
	stw		rPixel2,-4(sp);							\
	stw		rPixel1,-8(sp);							\
	lfd		fpTemp1,-8(sp);							\
	STORE_TO_DEST1(stfd,fpTemp1,16);				\
	STORE_TO_DEST2(stfd,fpTemp1,16);				\
	STORE_TO_DEST3(stfd,fpTemp1,16);				\
DontDraw3:;											\
	lwz		rPixel1,X_OFFSET(12,4)(rSource);		\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside1,12);		\
	EXTRACT_PIXEL1_16(rPixel2,rPixel1,3);			\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	STORE_TO_LOOKASIDE(stw,rPixel1,12);				\
	SKIP_IF_NOT_DIRTY(cr1,DontDrawMulti);			\
	EXTRACT_PIXEL0_16(rPixel1,rPixel1,3);			\
	lwzx	rPixel2,rLookup,rPixel2;				\
	lwzx	rPixel1,rLookup,rPixel1;				\
	stw		rPixel2,-4(sp);							\
	stw		rPixel1,-8(sp);							\
	addi	rSource,rSource,X_INCREMENT(16);		\
	ADD_TO_LOOKASIDE(16);							\
	ADD_TO_DEST1(32);								\
	ADD_TO_DEST2(32);								\
	ADD_TO_DEST3(32);								\
	lfd		fpTemp1,-8(sp);							\
	STORE_TO_DEST1(stfd,fpTemp1,-8);				\
	STORE_TO_DEST2(stfd,fpTemp1,-8);				\
	STORE_TO_DEST3(stfd,fpTemp1,-8);				\


#define EXPAND_PIXEL_1x_16TO32L_SWAPXY(outLabel)	\
	lhz		rPixel1,0(rSource);						\
	LOAD_FROM_LOOKASIDE(lhz,rLookaside1,0);			\
	INCREMENT_SOURCE;								\
	rlwinm	rPixel2,rPixel1,3,13,28;				\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	STORE_TO_LOOKASIDE(sth,rPixel1,0);				\
	SKIP_IF_NOT_DIRTY(cr1,outLabel);				\
	lwzx	rPixel2,rLookup,rPixel2;				\
	ADD_TO_LOOKASIDE(2);							\
	ADD_TO_DEST1(4);								\
	ADD_TO_DEST2(4);								\
	ADD_TO_DEST3(4);								\
	STORE_TO_DEST1(stw,rPixel2,-4);					\
	STORE_TO_DEST2(stw,rPixel2,-4);					\
	STORE_TO_DEST3(stw,rPixel2,-4);					\

#define EXPAND_PIXELS_1x_16TO32L_SWAPXY				\
	lhz		rPixel2,0(rSource);						\
	INCREMENT_SOURCE;								\
	lhz		rPixel1,0(rSource);						\
	INCREMENT_SOURCE;								\
	rlwimi	rPixel1,rPixel2,16,0,15;				\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside1,0);			\
	rlwinm	rPixel2,rPixel1,3,13,28;				\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	STORE_TO_LOOKASIDE(stw,rPixel1,0);				\
	SKIP_IF_NOT_DIRTY(cr1,DontDraw1);				\
	rlwinm	rPixel1,rPixel1,19,13,28;				\
	lwzx	rPixel2,rLookup,rPixel2;				\
	lwzx	rPixel1,rLookup,rPixel1;				\
	stw		rPixel2,-4(sp);							\
	stw		rPixel1,-8(sp);							\
	lfd		fpTemp1,-8(sp);							\
	STORE_TO_DEST1(stfd,fpTemp1,0);					\
	STORE_TO_DEST2(stfd,fpTemp1,0);					\
	STORE_TO_DEST3(stfd,fpTemp1,0);					\
DontDraw1:;											\
	lhz		rPixel2,0(rSource);						\
	INCREMENT_SOURCE;								\
	lhz		rPixel1,0(rSource);						\
	INCREMENT_SOURCE;								\
	rlwimi	rPixel1,rPixel2,16,0,15;				\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside1,4);			\
	rlwinm	rPixel2,rPixel1,3,13,28;				\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	STORE_TO_LOOKASIDE(stw,rPixel1,4);				\
	SKIP_IF_NOT_DIRTY(cr1,DontDraw2);				\
	rlwinm	rPixel1,rPixel1,19,13,28;				\
	lwzx	rPixel2,rLookup,rPixel2;				\
	lwzx	rPixel1,rLookup,rPixel1;				\
	stw		rPixel2,-4(sp);							\
	stw		rPixel1,-8(sp);							\
	lfd		fpTemp1,-8(sp);							\
	STORE_TO_DEST1(stfd,fpTemp1,8);					\
	STORE_TO_DEST2(stfd,fpTemp1,8);					\
	STORE_TO_DEST3(stfd,fpTemp1,8);					\
DontDraw2:;											\
	lhz		rPixel2,0(rSource);						\
	INCREMENT_SOURCE;								\
	lhz		rPixel1,0(rSource);						\
	INCREMENT_SOURCE;								\
	rlwimi	rPixel1,rPixel2,16,0,15;				\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside1,8);			\
	rlwinm	rPixel2,rPixel1,3,13,28;				\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	STORE_TO_LOOKASIDE(stw,rPixel1,8);				\
	SKIP_IF_NOT_DIRTY(cr1,DontDraw3);				\
	rlwinm	rPixel1,rPixel1,19,13,28;				\
	lwzx	rPixel2,rLookup,rPixel2;				\
	lwzx	rPixel1,rLookup,rPixel1;				\
	stw		rPixel2,-4(sp);							\
	stw		rPixel1,-8(sp);							\
	lfd		fpTemp1,-8(sp);							\
	STORE_TO_DEST1(stfd,fpTemp1,16);				\
	STORE_TO_DEST2(stfd,fpTemp1,16);				\
	STORE_TO_DEST3(stfd,fpTemp1,16);				\
DontDraw3:;											\
	lhz		rPixel2,0(rSource);						\
	INCREMENT_SOURCE;								\
	lhz		rPixel1,0(rSource);						\
	INCREMENT_SOURCE;								\
	rlwimi	rPixel1,rPixel2,16,0,15;				\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside1,12);		\
	rlwinm	rPixel2,rPixel1,3,13,28;				\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	STORE_TO_LOOKASIDE(stw,rPixel1,12);				\
	SKIP_IF_NOT_DIRTY(cr1,DontDrawMulti);			\
	rlwinm	rPixel1,rPixel1,19,13,28;				\
	lwzx	rPixel2,rLookup,rPixel2;				\
	lwzx	rPixel1,rLookup,rPixel1;				\
	stw		rPixel2,-4(sp);							\
	stw		rPixel1,-8(sp);							\
	ADD_TO_LOOKASIDE(16);							\
	ADD_TO_DEST1(32);								\
	ADD_TO_DEST2(32);								\
	ADD_TO_DEST3(32);								\
	lfd		fpTemp1,-8(sp);							\
	STORE_TO_DEST1(stfd,fpTemp1,-8);				\
	STORE_TO_DEST2(stfd,fpTemp1,-8);				\
	STORE_TO_DEST3(stfd,fpTemp1,-8);				\


//===================================================

#define EXPAND_PIXEL_2x_16TO32L(outLabel)			\
	lhz		rPixel1,X_OFFSET(0,2)(rSource);			\
	LOAD_FROM_LOOKASIDE(lhz,rLookaside1,0);			\
	addi	rSource,rSource,X_INCREMENT(2);			\
	rlwinm	rPixel2,rPixel1,3,13,28;				\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	STORE_TO_LOOKASIDE(sth,rPixel1,0);				\
	SKIP_IF_NOT_DIRTY(cr1,outLabel);				\
	lfdx	fpTemp1,rLookup,rPixel2;				\
	ADD_TO_LOOKASIDE(2);							\
	ADD_TO_DEST1(8);								\
	ADD_TO_DEST2(8);								\
	ADD_TO_DEST3(8);								\
	STORE_TO_DEST1(stfd,fpTemp1,-8);				\
	STORE_TO_DEST2(stfd,fpTemp1,-8);				\
	STORE_TO_DEST3(stfd,fpTemp1,-8);				\

#define EXPAND_PIXELS_2x_16TO32L					\
	lwz		rPixel1,X_OFFSET(0,4)(rSource);			\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside1,0);			\
	EXTRACT_PIXEL1_16(rPixel2,rPixel1,3);			\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	STORE_TO_LOOKASIDE(stw,rPixel1,0);				\
	SKIP_IF_NOT_DIRTY(cr1,DontDraw1);				\
	EXTRACT_PIXEL0_16(rPixel1,rPixel1,3);			\
	lfdx	fpTemp1,rLookup,rPixel1;				\
	lfdx	fpTemp2,rLookup,rPixel2;				\
	STORE_TO_DEST1(stfd,fpTemp1,0);					\
	STORE_TO_DEST1(stfd,fpTemp2,8);					\
	STORE_TO_DEST2(stfd,fpTemp1,0);					\
	STORE_TO_DEST2(stfd,fpTemp2,8);					\
	STORE_TO_DEST3(stfd,fpTemp1,0);					\
	STORE_TO_DEST3(stfd,fpTemp2,8);					\
DontDraw1:;											\
	lwz		rPixel1,X_OFFSET(4,4)(rSource);			\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside1,4);			\
	EXTRACT_PIXEL1_16(rPixel2,rPixel1,3);			\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	STORE_TO_LOOKASIDE(stw,rPixel1,4);				\
	SKIP_IF_NOT_DIRTY(cr1,DontDraw2);				\
	EXTRACT_PIXEL0_16(rPixel1,rPixel1,3);			\
	lfdx	fpTemp1,rLookup,rPixel1;				\
	lfdx	fpTemp2,rLookup,rPixel2;				\
	STORE_TO_DEST1(stfd,fpTemp1,16);				\
	STORE_TO_DEST1(stfd,fpTemp2,24);				\
	STORE_TO_DEST2(stfd,fpTemp1,16);				\
	STORE_TO_DEST2(stfd,fpTemp2,24);				\
	STORE_TO_DEST3(stfd,fpTemp1,16);				\
	STORE_TO_DEST3(stfd,fpTemp2,24);				\
DontDraw2:;											\
	lwz		rPixel1,X_OFFSET(8,4)(rSource);			\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside1,8);			\
	EXTRACT_PIXEL1_16(rPixel2,rPixel1,3);			\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	STORE_TO_LOOKASIDE(stw,rPixel1,8);				\
	SKIP_IF_NOT_DIRTY(cr1,DontDraw3);				\
	EXTRACT_PIXEL0_16(rPixel1,rPixel1,3);			\
	lfdx	fpTemp1,rLookup,rPixel1;				\
	lfdx	fpTemp2,rLookup,rPixel2;				\
	STORE_TO_DEST1(stfd,fpTemp1,32);				\
	STORE_TO_DEST1(stfd,fpTemp2,40);				\
	STORE_TO_DEST2(stfd,fpTemp1,32);				\
	STORE_TO_DEST2(stfd,fpTemp2,40);				\
	STORE_TO_DEST3(stfd,fpTemp1,32);				\
	STORE_TO_DEST3(stfd,fpTemp2,40);				\
DontDraw3:;											\
	lwz		rPixel1,X_OFFSET(12,4)(rSource);		\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside1,12);		\
	EXTRACT_PIXEL1_16(rPixel2,rPixel1,3);			\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	STORE_TO_LOOKASIDE(stw,rPixel1,12);				\
	SKIP_IF_NOT_DIRTY(cr1,DontDrawMulti);			\
	EXTRACT_PIXEL0_16(rPixel1,rPixel1,3);			\
	lfdx	fpTemp1,rLookup,rPixel1;				\
	lfdx	fpTemp2,rLookup,rPixel2;				\
	addi	rSource,rSource,X_INCREMENT(16);		\
	ADD_TO_LOOKASIDE(16);							\
	ADD_TO_DEST1(64);								\
	ADD_TO_DEST2(64);								\
	ADD_TO_DEST3(64);								\
	STORE_TO_DEST1(stfd,fpTemp1,-16);				\
	STORE_TO_DEST1(stfd,fpTemp2,-8);				\
	STORE_TO_DEST2(stfd,fpTemp1,-16);				\
	STORE_TO_DEST2(stfd,fpTemp2,-8);				\
	STORE_TO_DEST3(stfd,fpTemp1,-16);				\
	STORE_TO_DEST3(stfd,fpTemp2,-8);				\


#define EXPAND_PIXEL_2x_16TO32L_SWAPXY(outLabel)	\
	lhz		rPixel1,0(rSource);						\
	LOAD_FROM_LOOKASIDE(lhz,rLookaside1,0);			\
	INCREMENT_SOURCE;								\
	rlwinm	rPixel2,rPixel1,3,13,28;				\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	STORE_TO_LOOKASIDE(sth,rPixel1,0);				\
	SKIP_IF_NOT_DIRTY(cr1,outLabel);				\
	lfdx	fpTemp1,rLookup,rPixel2;				\
	ADD_TO_LOOKASIDE(2);							\
	ADD_TO_DEST1(8);								\
	ADD_TO_DEST2(8);								\
	ADD_TO_DEST3(8);								\
	STORE_TO_DEST1(stfd,fpTemp1,-8);				\
	STORE_TO_DEST2(stfd,fpTemp1,-8);				\
	STORE_TO_DEST3(stfd,fpTemp1,-8);				\

#define EXPAND_PIXELS_2x_16TO32L_SWAPXY				\
	lhz		rPixel2,0(rSource);						\
	INCREMENT_SOURCE;								\
	lhz		rPixel1,0(rSource);						\
	INCREMENT_SOURCE;								\
	rlwimi	rPixel1,rPixel2,16,0,15;				\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside1,0);			\
	rlwinm	rPixel2,rPixel1,3,13,28;				\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	STORE_TO_LOOKASIDE(stw,rPixel1,0);				\
	SKIP_IF_NOT_DIRTY(cr1,DontDraw1);				\
	rlwinm	rPixel1,rPixel1,19,13,28;				\
	lfdx	fpTemp1,rLookup,rPixel1;				\
	lfdx	fpTemp2,rLookup,rPixel2;				\
	STORE_TO_DEST1(stfd,fpTemp1,0);					\
	STORE_TO_DEST1(stfd,fpTemp2,8);					\
	STORE_TO_DEST2(stfd,fpTemp1,0);					\
	STORE_TO_DEST2(stfd,fpTemp2,8);					\
	STORE_TO_DEST3(stfd,fpTemp1,0);					\
	STORE_TO_DEST3(stfd,fpTemp2,8);					\
DontDraw1:;											\
	lhz		rPixel2,0(rSource);						\
	INCREMENT_SOURCE;								\
	lhz		rPixel1,0(rSource);						\
	INCREMENT_SOURCE;								\
	rlwimi	rPixel1,rPixel2,16,0,15;				\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside1,4);			\
	rlwinm	rPixel2,rPixel1,3,13,28;				\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	STORE_TO_LOOKASIDE(stw,rPixel1,4);				\
	SKIP_IF_NOT_DIRTY(cr1,DontDraw2);				\
	rlwinm	rPixel1,rPixel1,19,13,28;				\
	lfdx	fpTemp1,rLookup,rPixel1;				\
	lfdx	fpTemp2,rLookup,rPixel2;				\
	STORE_TO_DEST1(stfd,fpTemp1,16);				\
	STORE_TO_DEST1(stfd,fpTemp2,24);				\
	STORE_TO_DEST2(stfd,fpTemp1,16);				\
	STORE_TO_DEST2(stfd,fpTemp2,24);				\
	STORE_TO_DEST3(stfd,fpTemp1,16);				\
	STORE_TO_DEST3(stfd,fpTemp2,24);				\
DontDraw2:;											\
	lhz		rPixel2,0(rSource);						\
	INCREMENT_SOURCE;								\
	lhz		rPixel1,0(rSource);						\
	INCREMENT_SOURCE;								\
	rlwimi	rPixel1,rPixel2,16,0,15;				\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside1,8);			\
	rlwinm	rPixel2,rPixel1,3,13,28;				\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	STORE_TO_LOOKASIDE(stw,rPixel1,8);				\
	SKIP_IF_NOT_DIRTY(cr1,DontDraw3);				\
	rlwinm	rPixel1,rPixel1,19,13,28;				\
	lfdx	fpTemp1,rLookup,rPixel1;				\
	lfdx	fpTemp2,rLookup,rPixel2;				\
	STORE_TO_DEST1(stfd,fpTemp1,32);				\
	STORE_TO_DEST1(stfd,fpTemp2,40);				\
	STORE_TO_DEST2(stfd,fpTemp1,32);				\
	STORE_TO_DEST2(stfd,fpTemp2,40);				\
	STORE_TO_DEST3(stfd,fpTemp1,32);				\
	STORE_TO_DEST3(stfd,fpTemp2,40);				\
DontDraw3:;											\
	lhz		rPixel2,0(rSource);						\
	INCREMENT_SOURCE;								\
	lhz		rPixel1,0(rSource);						\
	INCREMENT_SOURCE;								\
	rlwimi	rPixel1,rPixel2,16,0,15;				\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside1,12);		\
	rlwinm	rPixel2,rPixel1,3,13,28;				\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	STORE_TO_LOOKASIDE(stw,rPixel1,12);				\
	SKIP_IF_NOT_DIRTY(cr1,DontDrawMulti);			\
	rlwinm	rPixel1,rPixel1,19,13,28;				\
	lfdx	fpTemp1,rLookup,rPixel1;				\
	lfdx	fpTemp2,rLookup,rPixel2;				\
	ADD_TO_LOOKASIDE(16);							\
	ADD_TO_DEST1(64);								\
	ADD_TO_DEST2(64);								\
	ADD_TO_DEST3(64);								\
	STORE_TO_DEST1(stfd,fpTemp1,-16);				\
	STORE_TO_DEST1(stfd,fpTemp2,-8);				\
	STORE_TO_DEST2(stfd,fpTemp1,-16);				\
	STORE_TO_DEST2(stfd,fpTemp2,-8);				\
	STORE_TO_DEST3(stfd,fpTemp1,-16);				\
	STORE_TO_DEST3(stfd,fpTemp2,-8);				\


//===================================================

#define EXPAND_PIXEL_3x_16TO32L(outLabel)			\
	lhz		rPixel1,X_OFFSET(0,2)(rSource);			\
	LOAD_FROM_LOOKASIDE(lhz,rLookaside1,0);			\
	addi	rSource,rSource,X_INCREMENT(2);			\
	rlwinm	rPixel2,rPixel1,3,13,28;				\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	STORE_TO_LOOKASIDE(sth,rPixel1,0);				\
	SKIP_IF_NOT_DIRTY(cr1,outLabel);				\
	lfdx	fpTemp1,rLookup,rPixel2;				\
	lwzx	rPixel2,rLookup,rPixel2;				\
	ADD_TO_LOOKASIDE(2);							\
	ADD_TO_DEST1(12);								\
	ADD_TO_DEST2(12);								\
	ADD_TO_DEST3(12);								\
	STORE_TO_DEST1(stfd,fpTemp1,-12);				\
	STORE_TO_DEST1(stw,rPixel2,-4);					\
	STORE_TO_DEST2(stfd,fpTemp1,-12);				\
	STORE_TO_DEST2(stw,rPixel2,-4);					\
	STORE_TO_DEST3(stfd,fpTemp1,-12);				\
	STORE_TO_DEST3(stw,rPixel2,-4);					\

#define EXPAND_PIXELS_3x_16TO32L					\
	lwz		rPixel1,X_OFFSET(0,4)(rSource);			\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside1,0);			\
	EXTRACT_PIXEL1_16(rPixel2,rPixel1,3);			\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	STORE_TO_LOOKASIDE(stw,rPixel1,0);				\
	SKIP_IF_NOT_DIRTY(cr1,DontDraw1);				\
	EXTRACT_PIXEL0_16(rPixel1,rPixel1,3);			\
	lwzx	rPixel4,rLookup,rPixel2;				\
	lwzx	rPixel3,rLookup,rPixel1;				\
	stw		rPixel4,-4(sp);							\
	stw		rPixel3,-8(sp);							\
	lfdx	fpTemp1,rLookup,rPixel1;				\
	lfdx	fpTemp3,rLookup,rPixel2;				\
	lfd		fpTemp2,-8(sp);							\
	STORE_TO_DEST1(stfd,fpTemp1,0);					\
	STORE_TO_DEST1(stfd,fpTemp2,8);					\
	STORE_TO_DEST1(stfd,fpTemp3,16);				\
	STORE_TO_DEST2(stfd,fpTemp1,0);					\
	STORE_TO_DEST2(stfd,fpTemp2,8);					\
	STORE_TO_DEST2(stfd,fpTemp3,16);				\
	STORE_TO_DEST3(stfd,fpTemp1,0);					\
	STORE_TO_DEST3(stfd,fpTemp2,8);					\
	STORE_TO_DEST3(stfd,fpTemp3,16);				\
DontDraw1:;											\
	lwz		rPixel1,X_OFFSET(4,4)(rSource);			\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside1,4);			\
	EXTRACT_PIXEL1_16(rPixel2,rPixel1,3);			\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	STORE_TO_LOOKASIDE(stw,rPixel1,4);				\
	SKIP_IF_NOT_DIRTY(cr1,DontDraw2);				\
	EXTRACT_PIXEL0_16(rPixel1,rPixel1,3);			\
	lwzx	rPixel4,rLookup,rPixel2;				\
	lwzx	rPixel3,rLookup,rPixel1;				\
	stw		rPixel4,-4(sp);							\
	stw		rPixel3,-8(sp);							\
	lfdx	fpTemp1,rLookup,rPixel1;				\
	lfdx	fpTemp3,rLookup,rPixel2;				\
	lfd		fpTemp2,-8(sp);							\
	STORE_TO_DEST1(stfd,fpTemp1,24);				\
	STORE_TO_DEST1(stfd,fpTemp2,32);				\
	STORE_TO_DEST1(stfd,fpTemp3,40);				\
	STORE_TO_DEST2(stfd,fpTemp1,24);				\
	STORE_TO_DEST2(stfd,fpTemp2,32);				\
	STORE_TO_DEST2(stfd,fpTemp3,40);				\
	STORE_TO_DEST3(stfd,fpTemp1,24);				\
	STORE_TO_DEST3(stfd,fpTemp2,32);				\
	STORE_TO_DEST3(stfd,fpTemp3,40);				\
DontDraw2:;											\
	lwz		rPixel1,X_OFFSET(8,4)(rSource);			\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside1,8);			\
	EXTRACT_PIXEL1_16(rPixel2,rPixel1,3);			\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	STORE_TO_LOOKASIDE(stw,rPixel1,8);				\
	SKIP_IF_NOT_DIRTY(cr1,DontDraw3);				\
	EXTRACT_PIXEL0_16(rPixel1,rPixel1,3);			\
	lwzx	rPixel4,rLookup,rPixel2;				\
	lwzx	rPixel3,rLookup,rPixel1;				\
	stw		rPixel4,-4(sp);							\
	stw		rPixel3,-8(sp);							\
	lfdx	fpTemp1,rLookup,rPixel1;				\
	lfdx	fpTemp3,rLookup,rPixel2;				\
	lfd		fpTemp2,-8(sp);							\
	STORE_TO_DEST1(stfd,fpTemp1,48);				\
	STORE_TO_DEST1(stfd,fpTemp2,56);				\
	STORE_TO_DEST1(stfd,fpTemp3,64);				\
	STORE_TO_DEST2(stfd,fpTemp1,48);				\
	STORE_TO_DEST2(stfd,fpTemp2,56);				\
	STORE_TO_DEST2(stfd,fpTemp3,64);				\
	STORE_TO_DEST3(stfd,fpTemp1,48);				\
	STORE_TO_DEST3(stfd,fpTemp2,56);				\
	STORE_TO_DEST3(stfd,fpTemp3,64);				\
DontDraw3:;											\
	lwz		rPixel1,X_OFFSET(12,4)(rSource);		\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside1,12);		\
	EXTRACT_PIXEL1_16(rPixel2,rPixel1,3);			\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	STORE_TO_LOOKASIDE(stw,rPixel1,12);				\
	SKIP_IF_NOT_DIRTY(cr1,DontDrawMulti);			\
	EXTRACT_PIXEL0_16(rPixel1,rPixel1,3);			\
	lwzx	rPixel4,rLookup,rPixel2;				\
	lwzx	rPixel3,rLookup,rPixel1;				\
	stw		rPixel4,-4(sp);							\
	stw		rPixel3,-8(sp);							\
	lfdx	fpTemp1,rLookup,rPixel1;				\
	lfdx	fpTemp3,rLookup,rPixel2;				\
	lfd		fpTemp2,-8(sp);							\
	addi	rSource,rSource,X_INCREMENT(16);		\
	ADD_TO_LOOKASIDE(16);							\
	ADD_TO_DEST1(96);								\
	ADD_TO_DEST2(96);								\
	ADD_TO_DEST3(96);								\
	STORE_TO_DEST1(stfd,fpTemp1,-24);				\
	STORE_TO_DEST1(stfd,fpTemp2,-16);				\
	STORE_TO_DEST1(stfd,fpTemp3,-8);				\
	STORE_TO_DEST2(stfd,fpTemp1,-24);				\
	STORE_TO_DEST2(stfd,fpTemp2,-16);				\
	STORE_TO_DEST2(stfd,fpTemp3,-8);				\
	STORE_TO_DEST3(stfd,fpTemp1,-24);				\
	STORE_TO_DEST3(stfd,fpTemp2,-16);				\
	STORE_TO_DEST3(stfd,fpTemp3,-8);				\


#define EXPAND_PIXEL_3x_16TO32L_SWAPXY(outLabel)	\
	lhz		rPixel1,0(rSource);						\
	LOAD_FROM_LOOKASIDE(lhz,rLookaside1,0);			\
	INCREMENT_SOURCE;								\
	rlwinm	rPixel2,rPixel1,3,13,28;				\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	STORE_TO_LOOKASIDE(sth,rPixel1,0);				\
	SKIP_IF_NOT_DIRTY(cr1,outLabel);				\
	lfdx	fpTemp1,rLookup,rPixel2;				\
	lwzx	rPixel2,rLookup,rPixel2;				\
	ADD_TO_LOOKASIDE(2);							\
	ADD_TO_DEST1(12);								\
	ADD_TO_DEST2(12);								\
	ADD_TO_DEST3(12);								\
	STORE_TO_DEST1(stfd,fpTemp1,-12);				\
	STORE_TO_DEST1(stw,rPixel2,-4);					\
	STORE_TO_DEST2(stfd,fpTemp1,-12);				\
	STORE_TO_DEST2(stw,rPixel2,-4);					\
	STORE_TO_DEST3(stfd,fpTemp1,-12);				\
	STORE_TO_DEST3(stw,rPixel2,-4);					\

#define EXPAND_PIXELS_3x_16TO32L_SWAPXY				\
	lhz		rPixel2,0(rSource);						\
	INCREMENT_SOURCE;								\
	lhz		rPixel1,0(rSource);						\
	INCREMENT_SOURCE;								\
	rlwimi	rPixel1,rPixel2,16,0,15;				\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside1,0);			\
	rlwinm	rPixel2,rPixel1,3,13,28;				\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	STORE_TO_LOOKASIDE(stw,rPixel1,0);				\
	SKIP_IF_NOT_DIRTY(cr1,DontDraw1);				\
	rlwinm	rPixel1,rPixel1,19,13,28;				\
	lwzx	rPixel4,rLookup,rPixel2;				\
	lwzx	rPixel3,rLookup,rPixel1;				\
	stw		rPixel4,-4(sp);							\
	stw		rPixel3,-8(sp);							\
	lfdx	fpTemp1,rLookup,rPixel1;				\
	lfdx	fpTemp3,rLookup,rPixel2;				\
	lfd		fpTemp2,-8(sp);							\
	STORE_TO_DEST1(stfd,fpTemp1,0);					\
	STORE_TO_DEST1(stfd,fpTemp2,8);					\
	STORE_TO_DEST1(stfd,fpTemp3,16);				\
	STORE_TO_DEST2(stfd,fpTemp1,0);					\
	STORE_TO_DEST2(stfd,fpTemp2,8);					\
	STORE_TO_DEST2(stfd,fpTemp3,16);				\
	STORE_TO_DEST3(stfd,fpTemp1,0);					\
	STORE_TO_DEST3(stfd,fpTemp2,8);					\
	STORE_TO_DEST3(stfd,fpTemp3,16);				\
DontDraw1:;											\
	lhz		rPixel2,0(rSource);						\
	INCREMENT_SOURCE;								\
	lhz		rPixel1,0(rSource);						\
	INCREMENT_SOURCE;								\
	rlwimi	rPixel1,rPixel2,16,0,15;				\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside1,4);			\
	rlwinm	rPixel2,rPixel1,3,13,28;				\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	STORE_TO_LOOKASIDE(stw,rPixel1,4);				\
	SKIP_IF_NOT_DIRTY(cr1,DontDraw2);				\
	rlwinm	rPixel1,rPixel1,19,13,28;				\
	lwzx	rPixel4,rLookup,rPixel2;				\
	lwzx	rPixel3,rLookup,rPixel1;				\
	stw		rPixel4,-4(sp);							\
	stw		rPixel3,-8(sp);							\
	lfdx	fpTemp1,rLookup,rPixel1;				\
	lfdx	fpTemp3,rLookup,rPixel2;				\
	lfd		fpTemp2,-8(sp);							\
	STORE_TO_DEST1(stfd,fpTemp1,24);				\
	STORE_TO_DEST1(stfd,fpTemp2,32);				\
	STORE_TO_DEST1(stfd,fpTemp3,40);				\
	STORE_TO_DEST2(stfd,fpTemp1,24);				\
	STORE_TO_DEST2(stfd,fpTemp2,32);				\
	STORE_TO_DEST2(stfd,fpTemp3,40);				\
	STORE_TO_DEST3(stfd,fpTemp1,24);				\
	STORE_TO_DEST3(stfd,fpTemp2,32);				\
	STORE_TO_DEST3(stfd,fpTemp3,40);				\
DontDraw2:;											\
	lhz		rPixel2,0(rSource);						\
	INCREMENT_SOURCE;								\
	lhz		rPixel1,0(rSource);						\
	INCREMENT_SOURCE;								\
	rlwimi	rPixel1,rPixel2,16,0,15;				\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside1,8);			\
	rlwinm	rPixel2,rPixel1,3,13,28;				\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	STORE_TO_LOOKASIDE(stw,rPixel1,8);				\
	SKIP_IF_NOT_DIRTY(cr1,DontDraw3);				\
	rlwinm	rPixel1,rPixel1,19,13,28;				\
	lwzx	rPixel4,rLookup,rPixel2;				\
	lwzx	rPixel3,rLookup,rPixel1;				\
	stw		rPixel4,-4(sp);							\
	stw		rPixel3,-8(sp);							\
	lfdx	fpTemp1,rLookup,rPixel1;				\
	lfdx	fpTemp3,rLookup,rPixel2;				\
	lfd		fpTemp2,-8(sp);							\
	STORE_TO_DEST1(stfd,fpTemp1,48);				\
	STORE_TO_DEST1(stfd,fpTemp2,56);				\
	STORE_TO_DEST1(stfd,fpTemp3,64);				\
	STORE_TO_DEST2(stfd,fpTemp1,48);				\
	STORE_TO_DEST2(stfd,fpTemp2,56);				\
	STORE_TO_DEST2(stfd,fpTemp3,64);				\
	STORE_TO_DEST3(stfd,fpTemp1,48);				\
	STORE_TO_DEST3(stfd,fpTemp2,56);				\
	STORE_TO_DEST3(stfd,fpTemp3,64);				\
DontDraw3:;											\
	lhz		rPixel2,0(rSource);						\
	INCREMENT_SOURCE;								\
	lhz		rPixel1,0(rSource);						\
	INCREMENT_SOURCE;								\
	rlwimi	rPixel1,rPixel2,16,0,15;				\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside1,12);		\
	rlwinm	rPixel2,rPixel1,3,13,28;				\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	STORE_TO_LOOKASIDE(stw,rPixel1,12);				\
	SKIP_IF_NOT_DIRTY(cr1,DontDrawMulti);			\
	rlwinm	rPixel1,rPixel1,19,13,28;				\
	lwzx	rPixel4,rLookup,rPixel2;				\
	lwzx	rPixel3,rLookup,rPixel1;				\
	stw		rPixel4,-4(sp);							\
	stw		rPixel3,-8(sp);							\
	lfdx	fpTemp1,rLookup,rPixel1;				\
	lfdx	fpTemp3,rLookup,rPixel2;				\
	lfd		fpTemp2,-8(sp);							\
	ADD_TO_LOOKASIDE(16);							\
	ADD_TO_DEST1(96);								\
	ADD_TO_DEST2(96);								\
	ADD_TO_DEST3(96);								\
	STORE_TO_DEST1(stfd,fpTemp1,-24);				\
	STORE_TO_DEST1(stfd,fpTemp2,-16);				\
	STORE_TO_DEST1(stfd,fpTemp3,-8);				\
	STORE_TO_DEST2(stfd,fpTemp1,-24);				\
	STORE_TO_DEST2(stfd,fpTemp2,-16);				\
	STORE_TO_DEST2(stfd,fpTemp3,-8);				\
	STORE_TO_DEST3(stfd,fpTemp1,-24);				\
	STORE_TO_DEST3(stfd,fpTemp2,-16);				\
	STORE_TO_DEST3(stfd,fpTemp3,-8);				\


//===================================================

#define EXPAND_PIXEL_1x_16TO32L_VEC(outLabel)		\
	lhz		rPixel1,X_OFFSET(0,2)(rSource);			\
	LOAD_FROM_LOOKASIDE(lhz,rLookaside1,0);			\
	addi	rSource,rSource,X_INCREMENT(2);			\
	rlwinm	rPixel2,rPixel1,3,13,28;				\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	STORE_TO_LOOKASIDE(sth,rPixel1,0);				\
	SKIP_IF_NOT_DIRTY(cr1,outLabel);				\
	lwzx	rPixel2,rLookup,rPixel2;				\
	ADD_TO_LOOKASIDE(2);							\
	STORE_TO_DEST1(stw,rPixel2,0);					\
	STORE_TO_DEST2(stwx,rPixel2);					\
	STORE_TO_DEST3(stwx,rPixel2);					\
	ADD_TO_DEST(4);									\

#define EXPAND_PIXELS_1x_16TO32L_VEC										\
	lvx		vrTemp2,0,rSource;				/* load source */				\
	addi	rSource,rSource,X_INCREMENT(16);								\
	LOAD_FROM_LOOKASIDE_VEC(vrTemp1);		/* load 8 pixels of lookaside */\
	COMPARE_TO_LOOKASIDE_VEC(vrTemp1,vrTemp1,vrTemp2);	/* compare all 8 pixels */	\
	SKIP_IF_NOT_DIRTY_VEC(DontDrawMulti);	/* skip if it is clean */		\
	STORE_TO_LOOKASIDE_VEC(vrTemp2);		/* save current source to lookaside */	\
													\
	vand	vrTemp2,vrTemp2,vrOffsetMask;			\
	lwz		rPixel1,X_OFFSET(0,4)-X_INCREMENT(16)(rSource);					\
	vslh	vrTemp2,vrTemp2,vrOffsetShift;			\
	lwz		rPixel3,X_OFFSET(4,4)-X_INCREMENT(16)(rSource);					\
	vperm	vrTemp3,vrTemp2,vrTemp2,vrOffsetPerm;	\
	lwz		rPixel5,X_OFFSET(8,4)-X_INCREMENT(16)(rSource);					\
	EXTRACT_PIXEL1_16(rPixel2,rPixel1,3);			\
	lwz		rPixel7,X_OFFSET(12,4)-X_INCREMENT(16)(rSource);				\
	EXTRACT_PIXEL0_16(rPixel1,rPixel1,3);			\
	vperm	vrTemp2,vrTemp2,vrTemp2,vrOffsetPerm2;	\
	vaddubm	vrTemp3,vrTemp3,vrOffsetOffset;			\
	EXTRACT_PIXEL1_16(rPixel4,rPixel3,3);			\
	lvewx	vrPixel2,rLookup,rPixel2;				\
	EXTRACT_PIXEL0_16(rPixel3,rPixel3,3);			\
	lvewx	vrPixel1,rLookup,rPixel1;				\
	vaddubm	vrTemp2,vrTemp2,vrOffsetOffset;			\
	EXTRACT_PIXEL1_16(rPixel6,rPixel5,3);			\
	lvewx	vrPixel4,rLookup,rPixel4;				\
	EXTRACT_PIXEL0_16(rPixel5,rPixel5,3);			\
	lvewx	vrPixel3,rLookup,rPixel3;				\
	EXTRACT_PIXEL1_16(rPixel8,rPixel7,3);			\
	lvewx	vrPixel6,rLookup,rPixel6;				\
	vperm	vrPixel1,vrPixel1,vrPixel2,vrTemp3;		\
	lvewx	vrPixel5,rLookup,rPixel5;				\
	EXTRACT_PIXEL0_16(rPixel7,rPixel7,3);			\
	lvewx	vrPixel8,rLookup,rPixel8;				\
	vperm	vrPixel3,vrPixel3,vrPixel4,vrTemp3;		\
	lvewx	vrPixel7,rLookup,rPixel7;				\
	vperm	vrPixel5,vrPixel5,vrPixel6,vrTemp2;		\
	vsel	vrPixel1,vrPixel1,vrPixel3,vrPixelMerge2;\
	vperm	vrPixel7,vrPixel7,vrPixel8,vrTemp2;		\
	vsel	vrPixel5,vrPixel5,vrPixel7,vrPixelMerge2;\
													\
	ADD_TO_LOOKASIDE(16);							\
	STORE_TO_DEST1_VEC(vrPixel1);			/* ABCD to line 1 */			\
	STORE_TO_DEST2_VEC(vrPixel1);			/* ABCD to line 2 */			\
	STORE_TO_DEST3_VEC(vrPixel1);			/* ABCD to line 3 */			\
	ADD_TO_DEST(16);														\
													\
	STORE_TO_DEST1_VEC(vrPixel5);			/* EFGH to line 1 */			\
	STORE_TO_DEST2_VEC(vrPixel5);			/* EFGH to line 2 */			\
	STORE_TO_DEST3_VEC(vrPixel5);			/* EFGH to line 3 */			\
	ADD_TO_DEST(16);														\


#define EXPAND_PIXEL_2x_16TO32L_VEC(outLabel)		\
	lhz		rPixel1,X_OFFSET(0,2)(rSource);			\
	LOAD_FROM_LOOKASIDE(lhz,rLookaside1,0);			\
	addi	rSource,rSource,X_INCREMENT(2);			\
	rlwinm	rPixel2,rPixel1,3,13,28;				\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	STORE_TO_LOOKASIDE(sth,rPixel1,0);				\
	SKIP_IF_NOT_DIRTY(cr1,outLabel);				\
	lfdx	fpTemp1,rLookup,rPixel2;				\
	ADD_TO_LOOKASIDE(2);							\
	STORE_TO_DEST1(stfd,fpTemp1,0);					\
	STORE_TO_DEST2(stfdx,fpTemp1);					\
	STORE_TO_DEST3(stfdx,fpTemp1);					\
	ADD_TO_DEST(8);									\

#define EXPAND_PIXELS_2x_16TO32L_VEC										\
	lvx		vrTemp2,0,rSource;				/* load source */				\
	addi	rSource,rSource,X_INCREMENT(16);								\
	LOAD_FROM_LOOKASIDE_VEC(vrTemp1);		/* load 8 pixels of lookaside */\
	COMPARE_TO_LOOKASIDE_VEC(vrTemp1,vrTemp1,vrTemp2);	/* compare all 8 pixels */	\
	SKIP_IF_NOT_DIRTY_VEC(DontDrawMulti);	/* skip if it is clean */		\
	STORE_TO_LOOKASIDE_VEC(vrTemp2);		/* save current source to lookaside */	\
													\
	vand	vrTemp2,vrTemp2,vrOffsetMask;			\
	lwz		rPixel1,X_OFFSET(0,4)-X_INCREMENT(16)(rSource);					\
	vslh	vrTemp2,vrTemp2,vrOffsetShift;			\
	lwz		rPixel3,X_OFFSET(4,4)-X_INCREMENT(16)(rSource);					\
	vperm	vrTemp3,vrTemp2,vrTemp2,vrOffsetPerm;	\
	lwz		rPixel5,X_OFFSET(8,4)-X_INCREMENT(16)(rSource);					\
	EXTRACT_PIXEL1_16(rPixel2,rPixel1,3);			\
	lwz		rPixel7,X_OFFSET(12,4)-X_INCREMENT(16)(rSource);				\
	EXTRACT_PIXEL0_16(rPixel1,rPixel1,3);			\
	vperm	vrTemp2,vrTemp2,vrTemp2,vrOffsetPerm2;	\
	vaddubm	vrTemp3,vrTemp3,vrOffsetOffset;			\
	EXTRACT_PIXEL1_16(rPixel4,rPixel3,3);			\
	lvewx	vrPixel2,rLookup,rPixel2;				\
	EXTRACT_PIXEL0_16(rPixel3,rPixel3,3);			\
	lvewx	vrPixel1,rLookup,rPixel1;				\
	vaddubm	vrTemp2,vrTemp2,vrOffsetOffset;			\
	EXTRACT_PIXEL1_16(rPixel6,rPixel5,3);			\
	lvewx	vrPixel4,rLookup,rPixel4;				\
	EXTRACT_PIXEL0_16(rPixel5,rPixel5,3);			\
	lvewx	vrPixel3,rLookup,rPixel3;				\
	EXTRACT_PIXEL1_16(rPixel8,rPixel7,3);			\
	lvewx	vrPixel6,rLookup,rPixel6;				\
	vperm	vrPixel1,vrPixel1,vrPixel2,vrTemp3;		\
	lvewx	vrPixel5,rLookup,rPixel5;				\
	EXTRACT_PIXEL0_16(rPixel7,rPixel7,3);			\
	lvewx	vrPixel8,rLookup,rPixel8;				\
	vperm	vrPixel3,vrPixel3,vrPixel4,vrTemp3;		\
	lvewx	vrPixel7,rLookup,rPixel7;				\
	vperm	vrPixel5,vrPixel5,vrPixel6,vrTemp2;		\
	vperm	vrPixel7,vrPixel7,vrPixel8,vrTemp2;		\
													\
	ADD_TO_LOOKASIDE(16);							\
	vmrghw	vrTemp0,vrPixel1,vrPixel1;		/* AABB */						\
	vmrglw	vrTemp1,vrPixel3,vrPixel3;		/* CCDD */						\
	STORE_TO_DEST1_VEC(vrTemp0);			/* AABB to line 1 */			\
	STORE_TO_DEST2_VEC(vrTemp0);			/* AABB to line 2 */			\
	STORE_TO_DEST3_VEC(vrTemp0);			/* AABB to line 3 */			\
	ADD_TO_DEST(16);														\
	STORE_TO_DEST1_VEC(vrTemp1);			/* CCDD to line 1 */			\
	STORE_TO_DEST2_VEC(vrTemp1);			/* CCDD to line 2 */			\
	STORE_TO_DEST3_VEC(vrTemp1);			/* CCDD to line 3 */			\
	ADD_TO_DEST(16);														\
													\
	vmrghw	vrTemp0,vrPixel5,vrPixel5;		/* EEFF */						\
	vmrglw	vrTemp1,vrPixel7,vrPixel7;		/* GGHH */						\
	STORE_TO_DEST1_VEC(vrTemp0);			/* EEFF to line 1 */			\
	STORE_TO_DEST2_VEC(vrTemp0);			/* EEFF to line 2 */			\
	STORE_TO_DEST3_VEC(vrTemp0);			/* EEFF to line 3 */			\
	ADD_TO_DEST(16);														\
	STORE_TO_DEST1_VEC(vrTemp1);			/* GGHH to line 1 */			\
	STORE_TO_DEST2_VEC(vrTemp1);			/* GGHH to line 2 */			\
	STORE_TO_DEST3_VEC(vrTemp1);			/* GGHH to line 3 */			\
	ADD_TO_DEST(16);														\


#define EXPAND_PIXEL_3x_16TO32L_VEC(outLabel)		\
	lhz		rPixel1,X_OFFSET(0,2)(rSource);			\
	LOAD_FROM_LOOKASIDE(lhz,rLookaside1,0);			\
	addi	rSource,rSource,X_INCREMENT(2);			\
	rlwinm	rPixel2,rPixel1,3,13,28;				\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	STORE_TO_LOOKASIDE(sth,rPixel1,0);				\
	SKIP_IF_NOT_DIRTY(cr1,outLabel);				\
	lfdx	fpTemp1,rLookup,rPixel2;				\
	lwzx	rPixel2,rLookup,rPixel2;				\
	ADD_TO_LOOKASIDE(2);							\
	STORE_TO_DEST1(stfd,fpTemp1,0);					\
	STORE_TO_DEST2(stfdx,fpTemp1);					\
	STORE_TO_DEST3(stfdx,fpTemp1);					\
	STORE_TO_DEST1(stwu,rPixel2,8);					\
	STORE_TO_DEST2(stwx,rPixel2);					\
	STORE_TO_DEST3(stwx,rPixel2);					\
	ADD_TO_DEST(4);									\

#define EXPAND_PIXELS_3x_16TO32L_VEC										\
	lvx		vrTemp2,0,rSource;				/* load source */				\
	addi	rSource,rSource,X_INCREMENT(16);								\
	LOAD_FROM_LOOKASIDE_VEC(vrTemp1);		/* load 8 pixels of lookaside */\
	COMPARE_TO_LOOKASIDE_VEC(vrTemp1,vrTemp1,vrTemp2);	/* compare all 8 pixels */	\
	SKIP_IF_NOT_DIRTY_VEC(DontDrawMulti);	/* skip if it is clean */		\
	STORE_TO_LOOKASIDE_VEC(vrTemp2);		/* save current source to lookaside */	\
													\
	vand	vrTemp2,vrTemp2,vrOffsetMask;			\
	lwz		rPixel1,X_OFFSET(0,4)-X_INCREMENT(16)(rSource);					\
	vslh	vrTemp2,vrTemp2,vrOffsetShift;			\
	lwz		rPixel3,X_OFFSET(4,4)-X_INCREMENT(16)(rSource);					\
	vperm	vrTemp3,vrTemp2,vrTemp2,vrOffsetPerm;	\
	lwz		rPixel5,X_OFFSET(8,4)-X_INCREMENT(16)(rSource);					\
	EXTRACT_PIXEL1_16(rPixel2,rPixel1,3);			\
	lwz		rPixel7,X_OFFSET(12,4)-X_INCREMENT(16)(rSource);				\
	EXTRACT_PIXEL0_16(rPixel1,rPixel1,3);			\
	vperm	vrTemp2,vrTemp2,vrTemp2,vrOffsetPerm2;	\
	vaddubm	vrTemp3,vrTemp3,vrOffsetOffset;			\
	EXTRACT_PIXEL1_16(rPixel4,rPixel3,3);			\
	lvewx	vrPixel2,rLookup,rPixel2;				\
	EXTRACT_PIXEL0_16(rPixel3,rPixel3,3);			\
	lvewx	vrPixel1,rLookup,rPixel1;				\
	vaddubm	vrTemp2,vrTemp2,vrOffsetOffset;			\
	EXTRACT_PIXEL1_16(rPixel6,rPixel5,3);			\
	lvewx	vrPixel4,rLookup,rPixel4;				\
	EXTRACT_PIXEL0_16(rPixel5,rPixel5,3);			\
	lvewx	vrPixel3,rLookup,rPixel3;				\
	EXTRACT_PIXEL1_16(rPixel8,rPixel7,3);			\
	lvewx	vrPixel6,rLookup,rPixel6;				\
	vperm	vrPixel1,vrPixel1,vrPixel2,vrTemp3;		\
	lvewx	vrPixel5,rLookup,rPixel5;				\
	EXTRACT_PIXEL0_16(rPixel7,rPixel7,3);			\
	lvewx	vrPixel8,rLookup,rPixel8;				\
	vperm	vrPixel3,vrPixel3,vrPixel4,vrTemp3;		\
	lvewx	vrPixel7,rLookup,rPixel7;				\
	vperm	vrPixel5,vrPixel5,vrPixel6,vrTemp2;		\
	vperm	vrPixel7,vrPixel7,vrPixel8,vrTemp2;		\
													\
	ADD_TO_LOOKASIDE(16);							\
	vperm	vrTemp0,vrPixel1,vrPixel1,vrPixelZoom0;	/* AAAB */				\
	vperm	vrTemp1,vrPixel1,vrPixel3,vrPixelZoom1;	/* BBCC */				\
	STORE_TO_DEST1_VEC(vrTemp0);			/* AAAB to line 1 */			\
	STORE_TO_DEST2_VEC(vrTemp0);			/* AAAB to line 2 */			\
	STORE_TO_DEST3_VEC(vrTemp0);			/* AAAB to line 3 */			\
	ADD_TO_DEST(16);														\
	vperm	vrTemp2,vrPixel3,vrPixel3,vrPixelZoom2;	/* CDDD */				\
	STORE_TO_DEST1_VEC(vrTemp1);			/* BBCC to line 1 */			\
	STORE_TO_DEST2_VEC(vrTemp1);			/* BBCC to line 2 */			\
	STORE_TO_DEST3_VEC(vrTemp1);			/* BBCC to line 3 */			\
	ADD_TO_DEST(16);														\
	STORE_TO_DEST1_VEC(vrTemp2);			/* CDDD to line 1 */			\
	STORE_TO_DEST2_VEC(vrTemp2);			/* CDDD to line 2 */			\
	STORE_TO_DEST3_VEC(vrTemp2);			/* CDDD to line 3 */			\
	ADD_TO_DEST(16);														\
													\
	vperm	vrTemp0,vrPixel5,vrPixel5,vrPixelZoom0;	/* EEEF */				\
	vperm	vrTemp1,vrPixel5,vrPixel7,vrPixelZoom1;	/* FFGG */				\
	STORE_TO_DEST1_VEC(vrTemp0);			/* EEEF to line 1 */			\
	STORE_TO_DEST2_VEC(vrTemp0);			/* EEEF to line 2 */			\
	STORE_TO_DEST3_VEC(vrTemp0);			/* EEEF to line 3 */			\
	ADD_TO_DEST(16);														\
	vperm	vrTemp2,vrPixel7,vrPixel7,vrPixelZoom2;	/* GHHH */				\
	STORE_TO_DEST1_VEC(vrTemp1);			/* FFGG to line 1 */			\
	STORE_TO_DEST2_VEC(vrTemp1);			/* FFGG to line 2 */			\
	STORE_TO_DEST3_VEC(vrTemp1);			/* FFGG to line 3 */			\
	ADD_TO_DEST(16);														\
	STORE_TO_DEST1_VEC(vrTemp2);			/* GHHH to line 1 */			\
	STORE_TO_DEST2_VEC(vrTemp2);			/* GHHH to line 2 */			\
	STORE_TO_DEST3_VEC(vrTemp2);			/* GHHH to line 3 */			\
	ADD_TO_DEST(16);														\


//===================================================
#pragma mark Expand 32 (direct)

#define EXPAND_PIXEL_1x_32TO32D(outLabel)			\
	lwz		rPixel1,X_OFFSET(0,4)(rSource);			\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside1,0);			\
	addi	rSource,rSource,X_INCREMENT(4);			\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	STORE_TO_LOOKASIDE(stw,rPixel1,0);				\
	SKIP_IF_NOT_DIRTY(cr1,outLabel);				\
	ADD_TO_LOOKASIDE(4);							\
	ADD_TO_DEST1(4);								\
	ADD_TO_DEST2(4);								\
	ADD_TO_DEST3(4);								\
	STORE_TO_DEST1(stw,rPixel1,-4);					\
	STORE_TO_DEST2(stw,rPixel1,-4);					\
	STORE_TO_DEST3(stw,rPixel1,-4);					\

#define EXPAND_PIXELS_1x_32TO32D					\
	lwz		rPixel1,X_OFFSET(0,4)(rSource);			\
	lwz		rPixel2,X_OFFSET(4,4)(rSource);			\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside1,0);			\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside2,4);			\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	COMPARE_TO_LOOKASIDE(cr5,rPixel2,rLookaside2);	\
	STORE_TO_LOOKASIDE(stw,rPixel1,0);				\
	COMBINE_COMPARES(cr6*4+eq,cr1*4+eq,cr5*4+eq);	\
	STORE_TO_LOOKASIDE(stw,rPixel2,4);				\
	SKIP_IF_NOT_DIRTY(cr6,DontDraw1);				\
	stw		rPixel1,-8(sp);							\
	stw		rPixel2,-4(sp);							\
	lfd		fpTemp1,-8(sp);							\
	STORE_TO_DEST1(stfd,fpTemp1,0);					\
	STORE_TO_DEST2(stfd,fpTemp1,0);					\
	STORE_TO_DEST3(stfd,fpTemp1,0);					\
DontDraw1:;											\
	lwz		rPixel1,X_OFFSET(8,4)(rSource);			\
	lwz		rPixel2,X_OFFSET(12,4)(rSource);		\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside1,8);			\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside2,12);		\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	COMPARE_TO_LOOKASIDE(cr5,rPixel2,rLookaside2);	\
	STORE_TO_LOOKASIDE(stw,rPixel1,8);				\
	COMBINE_COMPARES(cr6*4+eq,cr1*4+eq,cr5*4+eq);	\
	STORE_TO_LOOKASIDE(stw,rPixel2,12);				\
	SKIP_IF_NOT_DIRTY(cr6,DontDraw2);				\
	stw		rPixel1,-8(sp);							\
	stw		rPixel2,-4(sp);							\
	lfd		fpTemp1,-8(sp);							\
	STORE_TO_DEST1(stfd,fpTemp1,8);					\
	STORE_TO_DEST2(stfd,fpTemp1,8);					\
	STORE_TO_DEST3(stfd,fpTemp1,8);					\
DontDraw2:;											\
	lwz		rPixel1,X_OFFSET(16,4)(rSource);		\
	lwz		rPixel2,X_OFFSET(20,4)(rSource);		\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside1,16);		\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside2,20);		\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	COMPARE_TO_LOOKASIDE(cr5,rPixel2,rLookaside2);	\
	STORE_TO_LOOKASIDE(stw,rPixel1,16);				\
	COMBINE_COMPARES(cr6*4+eq,cr1*4+eq,cr5*4+eq);	\
	STORE_TO_LOOKASIDE(stw,rPixel2,20);				\
	SKIP_IF_NOT_DIRTY(cr6,DontDraw3);				\
	stw		rPixel1,-8(sp);							\
	stw		rPixel2,-4(sp);							\
	lfd		fpTemp1,-8(sp);							\
	STORE_TO_DEST1(stfd,fpTemp1,16);				\
	STORE_TO_DEST2(stfd,fpTemp1,16);				\
	STORE_TO_DEST3(stfd,fpTemp1,16);				\
DontDraw3:;											\
	lwz		rPixel1,X_OFFSET(24,4)(rSource);		\
	lwz		rPixel2,X_OFFSET(28,4)(rSource);		\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside1,24);		\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside2,28);		\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	COMPARE_TO_LOOKASIDE(cr5,rPixel2,rLookaside2);	\
	STORE_TO_LOOKASIDE(stw,rPixel1,24);				\
	COMBINE_COMPARES(cr6*4+eq,cr1*4+eq,cr5*4+eq);	\
	STORE_TO_LOOKASIDE(stw,rPixel2,28);				\
	SKIP_IF_NOT_DIRTY(cr6,DontDrawMulti);			\
	stw		rPixel1,-8(sp);							\
	stw		rPixel2,-4(sp);							\
	addi	rSource,rSource,X_INCREMENT(32);		\
	ADD_TO_LOOKASIDE(32);							\
	ADD_TO_DEST1(32);								\
	ADD_TO_DEST2(32);								\
	ADD_TO_DEST3(32);								\
	lfd		fpTemp1,-8(sp);							\
	STORE_TO_DEST1(stfd,fpTemp1,-8);				\
	STORE_TO_DEST2(stfd,fpTemp1,-8);				\
	STORE_TO_DEST3(stfd,fpTemp1,-8);				\


#define EXPAND_PIXEL_1x_32TO32D_SWAPXY(outLabel)	\
	lwz		rPixel1,0(rSource);						\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside1,0);			\
	INCREMENT_SOURCE;								\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	STORE_TO_LOOKASIDE(stw,rPixel1,0);				\
	SKIP_IF_NOT_DIRTY(cr1,outLabel);				\
	ADD_TO_LOOKASIDE(4);							\
	ADD_TO_DEST1(4);								\
	ADD_TO_DEST2(4);								\
	ADD_TO_DEST3(4);								\
	STORE_TO_DEST1(stw,rPixel1,-4);					\
	STORE_TO_DEST2(stw,rPixel1,-4);					\
	STORE_TO_DEST3(stw,rPixel1,-4);					\

#define EXPAND_PIXELS_1x_32TO32D_SWAPXY				\
	lwz		rPixel1,0(rSource);						\
	INCREMENT_SOURCE;								\
	lwz		rPixel2,0(rSource);						\
	INCREMENT_SOURCE;								\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside1,0);			\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside2,4);			\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	COMPARE_TO_LOOKASIDE(cr5,rPixel2,rLookaside2);	\
	STORE_TO_LOOKASIDE(stw,rPixel1,0);				\
	COMBINE_COMPARES(cr6*4+eq,cr1*4+eq,cr5*4+eq);	\
	STORE_TO_LOOKASIDE(stw,rPixel2,4);				\
	SKIP_IF_NOT_DIRTY(cr6,DontDraw1);				\
	stw		rPixel1,-8(sp);							\
	stw		rPixel2,-4(sp);							\
	lfd		fpTemp1,-8(sp);							\
	STORE_TO_DEST1(stfd,fpTemp1,0);					\
	STORE_TO_DEST2(stfd,fpTemp1,0);					\
	STORE_TO_DEST3(stfd,fpTemp1,0);					\
DontDraw1:;											\
	lwz		rPixel1,0(rSource);						\
	INCREMENT_SOURCE;								\
	lwz		rPixel2,0(rSource);						\
	INCREMENT_SOURCE;								\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside1,8);			\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside2,12);		\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	COMPARE_TO_LOOKASIDE(cr5,rPixel2,rLookaside2);	\
	STORE_TO_LOOKASIDE(stw,rPixel1,8);				\
	COMBINE_COMPARES(cr6*4+eq,cr1*4+eq,cr5*4+eq);	\
	STORE_TO_LOOKASIDE(stw,rPixel2,12);				\
	SKIP_IF_NOT_DIRTY(cr6,DontDraw2);				\
	stw		rPixel1,-8(sp);							\
	stw		rPixel2,-4(sp);							\
	lfd		fpTemp1,-8(sp);							\
	STORE_TO_DEST1(stfd,fpTemp1,8);					\
	STORE_TO_DEST2(stfd,fpTemp1,8);					\
	STORE_TO_DEST3(stfd,fpTemp1,8);					\
DontDraw2:;											\
	lwz		rPixel1,0(rSource);						\
	INCREMENT_SOURCE;								\
	lwz		rPixel2,0(rSource);						\
	INCREMENT_SOURCE;								\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside1,16);		\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside2,20);		\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	COMPARE_TO_LOOKASIDE(cr5,rPixel2,rLookaside2);	\
	STORE_TO_LOOKASIDE(stw,rPixel1,16);				\
	COMBINE_COMPARES(cr6*4+eq,cr1*4+eq,cr5*4+eq);	\
	STORE_TO_LOOKASIDE(stw,rPixel2,20);				\
	SKIP_IF_NOT_DIRTY(cr6,DontDraw3);				\
	stw		rPixel1,-8(sp);							\
	stw		rPixel2,-4(sp);							\
	lfd		fpTemp1,-8(sp);							\
	STORE_TO_DEST1(stfd,fpTemp1,16);				\
	STORE_TO_DEST2(stfd,fpTemp1,16);				\
	STORE_TO_DEST3(stfd,fpTemp1,16);				\
DontDraw3:;											\
	lwz		rPixel1,0(rSource);						\
	INCREMENT_SOURCE;								\
	lwz		rPixel2,0(rSource);						\
	INCREMENT_SOURCE;								\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside1,24);		\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside2,28);		\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	COMPARE_TO_LOOKASIDE(cr5,rPixel2,rLookaside2);	\
	STORE_TO_LOOKASIDE(stw,rPixel1,24);				\
	COMBINE_COMPARES(cr6*4+eq,cr1*4+eq,cr5*4+eq);	\
	STORE_TO_LOOKASIDE(stw,rPixel2,28);				\
	SKIP_IF_NOT_DIRTY(cr6,DontDrawMulti);			\
	stw		rPixel1,-8(sp);							\
	stw		rPixel2,-4(sp);							\
	ADD_TO_LOOKASIDE(32);							\
	ADD_TO_DEST1(32);								\
	ADD_TO_DEST2(32);								\
	ADD_TO_DEST3(32);								\
	lfd		fpTemp1,-8(sp);							\
	STORE_TO_DEST1(stfd,fpTemp1,-8);				\
	STORE_TO_DEST2(stfd,fpTemp1,-8);				\
	STORE_TO_DEST3(stfd,fpTemp1,-8);				\


//===================================================

#define EXPAND_PIXEL_2x_32TO32D(outLabel)			\
	lwz		rPixel1,X_OFFSET(0,4)(rSource);			\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside1,0);			\
	addi	rSource,rSource,X_INCREMENT(4);			\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	STORE_TO_LOOKASIDE(stw,rPixel1,0);				\
	SKIP_IF_NOT_DIRTY(cr1,outLabel);				\
	stw		rPixel1,-8(sp);							\
	stw		rPixel1,-4(sp);							\
	lfd		fpTemp1,-8(sp);							\
	ADD_TO_LOOKASIDE(4);							\
	ADD_TO_DEST1(8);								\
	ADD_TO_DEST2(8);								\
	ADD_TO_DEST3(8);								\
	STORE_TO_DEST1(stfd,fpTemp1,-8);				\
	STORE_TO_DEST2(stfd,fpTemp1,-8);				\
	STORE_TO_DEST3(stfd,fpTemp1,-8);				\

#define EXPAND_PIXELS_2x_32TO32D					\
	lwz		rPixel1,X_OFFSET(0,4)(rSource);			\
	lwz		rPixel2,X_OFFSET(4,4)(rSource);			\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside1,0);			\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside2,4);			\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	COMPARE_TO_LOOKASIDE(cr5,rPixel2,rLookaside2);	\
	STORE_TO_LOOKASIDE(stw,rPixel1,0);				\
	COMBINE_COMPARES(cr6*4+eq,cr1*4+eq,cr5*4+eq);	\
	STORE_TO_LOOKASIDE(stw,rPixel2,4);				\
	SKIP_IF_NOT_DIRTY(cr6,DontDraw1);				\
	stw		rPixel1,-16(sp);						\
	stw		rPixel1,-12(sp);						\
	stw		rPixel2,-8(sp);							\
	stw		rPixel2,-4(sp);							\
	lfd		fpTemp1,-16(sp);						\
	lfd		fpTemp2,-8(sp);							\
	STORE_TO_DEST1(stfd,fpTemp1,0);					\
	STORE_TO_DEST1(stfd,fpTemp2,8);					\
	STORE_TO_DEST2(stfd,fpTemp1,0);					\
	STORE_TO_DEST2(stfd,fpTemp2,8);					\
	STORE_TO_DEST3(stfd,fpTemp1,0);					\
	STORE_TO_DEST3(stfd,fpTemp2,8);					\
DontDraw1:;											\
	lwz		rPixel1,X_OFFSET(8,4)(rSource);			\
	lwz		rPixel2,X_OFFSET(12,4)(rSource);		\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside1,8);			\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside2,12);		\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	COMPARE_TO_LOOKASIDE(cr5,rPixel2,rLookaside2);	\
	STORE_TO_LOOKASIDE(stw,rPixel1,8);				\
	COMBINE_COMPARES(cr6*4+eq,cr1*4+eq,cr5*4+eq);	\
	STORE_TO_LOOKASIDE(stw,rPixel2,12);				\
	SKIP_IF_NOT_DIRTY(cr6,DontDraw2);				\
	stw		rPixel1,-16(sp);						\
	stw		rPixel1,-12(sp);						\
	stw		rPixel2,-8(sp);							\
	stw		rPixel2,-4(sp);							\
	lfd		fpTemp1,-16(sp);						\
	lfd		fpTemp2,-8(sp);							\
	STORE_TO_DEST1(stfd,fpTemp1,16);				\
	STORE_TO_DEST1(stfd,fpTemp2,24);				\
	STORE_TO_DEST2(stfd,fpTemp1,16);				\
	STORE_TO_DEST2(stfd,fpTemp2,24);				\
	STORE_TO_DEST3(stfd,fpTemp1,16);				\
	STORE_TO_DEST3(stfd,fpTemp2,24);				\
DontDraw2:;											\
	lwz		rPixel1,X_OFFSET(16,4)(rSource);		\
	lwz		rPixel2,X_OFFSET(20,4)(rSource);		\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside1,16);		\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside2,20);		\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	COMPARE_TO_LOOKASIDE(cr5,rPixel2,rLookaside2);	\
	STORE_TO_LOOKASIDE(stw,rPixel1,16);				\
	COMBINE_COMPARES(cr6*4+eq,cr1*4+eq,cr5*4+eq);	\
	STORE_TO_LOOKASIDE(stw,rPixel2,20);				\
	SKIP_IF_NOT_DIRTY(cr6,DontDraw3);				\
	stw		rPixel1,-16(sp);						\
	stw		rPixel1,-12(sp);						\
	stw		rPixel2,-8(sp);							\
	stw		rPixel2,-4(sp);							\
	lfd		fpTemp1,-16(sp);						\
	lfd		fpTemp2,-8(sp);							\
	STORE_TO_DEST1(stfd,fpTemp1,32);				\
	STORE_TO_DEST1(stfd,fpTemp2,40);				\
	STORE_TO_DEST2(stfd,fpTemp1,32);				\
	STORE_TO_DEST2(stfd,fpTemp2,40);				\
	STORE_TO_DEST3(stfd,fpTemp1,32);				\
	STORE_TO_DEST3(stfd,fpTemp2,40);				\
DontDraw3:;											\
	lwz		rPixel1,X_OFFSET(24,4)(rSource);		\
	lwz		rPixel2,X_OFFSET(28,4)(rSource);		\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside1,24);		\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside2,28);		\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	COMPARE_TO_LOOKASIDE(cr5,rPixel2,rLookaside2);	\
	STORE_TO_LOOKASIDE(stw,rPixel1,24);				\
	COMBINE_COMPARES(cr6*4+eq,cr1*4+eq,cr5*4+eq);	\
	STORE_TO_LOOKASIDE(stw,rPixel2,28);				\
	SKIP_IF_NOT_DIRTY(cr6,DontDrawMulti);			\
	stw		rPixel1,-16(sp);						\
	stw		rPixel1,-12(sp);						\
	stw		rPixel2,-8(sp);							\
	stw		rPixel2,-4(sp);							\
	lfd		fpTemp1,-16(sp);						\
	lfd		fpTemp2,-8(sp);							\
	addi	rSource,rSource,X_INCREMENT(32);		\
	ADD_TO_LOOKASIDE(32);							\
	ADD_TO_DEST1(64);								\
	ADD_TO_DEST2(64);								\
	ADD_TO_DEST3(64);								\
	STORE_TO_DEST1(stfd,fpTemp1,-16);				\
	STORE_TO_DEST1(stfd,fpTemp2,-8);				\
	STORE_TO_DEST2(stfd,fpTemp1,-16);				\
	STORE_TO_DEST2(stfd,fpTemp2,-8);				\
	STORE_TO_DEST3(stfd,fpTemp1,-16);				\
	STORE_TO_DEST3(stfd,fpTemp2,-8);				\


#define EXPAND_PIXEL_2x_32TO32D_SWAPXY(outLabel)	\
	lwz		rPixel1,0(rSource);						\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside1,0);			\
	INCREMENT_SOURCE;								\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	STORE_TO_LOOKASIDE(stw,rPixel1,0);				\
	SKIP_IF_NOT_DIRTY(cr1,outLabel);				\
	stw		rPixel1,-8(sp);							\
	stw		rPixel1,-4(sp);							\
	lfd		fpTemp1,-8(sp);							\
	ADD_TO_LOOKASIDE(4);							\
	ADD_TO_DEST1(8);								\
	ADD_TO_DEST2(8);								\
	ADD_TO_DEST3(8);								\
	STORE_TO_DEST1(stfd,fpTemp1,-8);				\
	STORE_TO_DEST2(stfd,fpTemp1,-8);				\
	STORE_TO_DEST3(stfd,fpTemp1,-8);				\

#define EXPAND_PIXELS_2x_32TO32D_SWAPXY				\
	lwz		rPixel1,0(rSource);						\
	INCREMENT_SOURCE;								\
	lwz		rPixel2,0(rSource);						\
	INCREMENT_SOURCE;								\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside1,0);			\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside2,4);			\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	COMPARE_TO_LOOKASIDE(cr5,rPixel2,rLookaside2);	\
	STORE_TO_LOOKASIDE(stw,rPixel1,0);				\
	COMBINE_COMPARES(cr6*4+eq,cr1*4+eq,cr5*4+eq);	\
	STORE_TO_LOOKASIDE(stw,rPixel2,4);				\
	SKIP_IF_NOT_DIRTY(cr6,DontDraw1);				\
	stw		rPixel1,-16(sp);						\
	stw		rPixel1,-12(sp);						\
	stw		rPixel2,-8(sp);							\
	stw		rPixel2,-4(sp);							\
	lfd		fpTemp1,-16(sp);						\
	lfd		fpTemp2,-8(sp);							\
	STORE_TO_DEST1(stfd,fpTemp1,0);					\
	STORE_TO_DEST1(stfd,fpTemp2,8);					\
	STORE_TO_DEST2(stfd,fpTemp1,0);					\
	STORE_TO_DEST2(stfd,fpTemp2,8);					\
	STORE_TO_DEST3(stfd,fpTemp1,0);					\
	STORE_TO_DEST3(stfd,fpTemp2,8);					\
DontDraw1:;											\
	lwz		rPixel1,0(rSource);						\
	INCREMENT_SOURCE;								\
	lwz		rPixel2,0(rSource);						\
	INCREMENT_SOURCE;								\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside1,8);			\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside2,12);		\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	COMPARE_TO_LOOKASIDE(cr5,rPixel2,rLookaside2);	\
	STORE_TO_LOOKASIDE(stw,rPixel1,8);				\
	COMBINE_COMPARES(cr6*4+eq,cr1*4+eq,cr5*4+eq);	\
	STORE_TO_LOOKASIDE(stw,rPixel2,12);				\
	SKIP_IF_NOT_DIRTY(cr6,DontDraw2);				\
	stw		rPixel1,-16(sp);						\
	stw		rPixel1,-12(sp);						\
	stw		rPixel2,-8(sp);							\
	stw		rPixel2,-4(sp);							\
	lfd		fpTemp1,-16(sp);						\
	lfd		fpTemp2,-8(sp);							\
	STORE_TO_DEST1(stfd,fpTemp1,16);				\
	STORE_TO_DEST1(stfd,fpTemp2,24);				\
	STORE_TO_DEST2(stfd,fpTemp1,16);				\
	STORE_TO_DEST2(stfd,fpTemp2,24);				\
	STORE_TO_DEST3(stfd,fpTemp1,16);				\
	STORE_TO_DEST3(stfd,fpTemp2,24);				\
DontDraw2:;											\
	lwz		rPixel1,0(rSource);						\
	INCREMENT_SOURCE;								\
	lwz		rPixel2,0(rSource);						\
	INCREMENT_SOURCE;								\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside1,16);		\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside2,20);		\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	COMPARE_TO_LOOKASIDE(cr5,rPixel2,rLookaside2);	\
	STORE_TO_LOOKASIDE(stw,rPixel1,16);				\
	COMBINE_COMPARES(cr6*4+eq,cr1*4+eq,cr5*4+eq);	\
	STORE_TO_LOOKASIDE(stw,rPixel2,20);				\
	SKIP_IF_NOT_DIRTY(cr6,DontDraw3);				\
	stw		rPixel1,-16(sp);						\
	stw		rPixel1,-12(sp);						\
	stw		rPixel2,-8(sp);							\
	stw		rPixel2,-4(sp);							\
	lfd		fpTemp1,-16(sp);						\
	lfd		fpTemp2,-8(sp);							\
	STORE_TO_DEST1(stfd,fpTemp1,32);				\
	STORE_TO_DEST1(stfd,fpTemp2,40);				\
	STORE_TO_DEST2(stfd,fpTemp1,32);				\
	STORE_TO_DEST2(stfd,fpTemp2,40);				\
	STORE_TO_DEST3(stfd,fpTemp1,32);				\
	STORE_TO_DEST3(stfd,fpTemp2,40);				\
DontDraw3:;											\
	lwz		rPixel1,0(rSource);						\
	INCREMENT_SOURCE;								\
	lwz		rPixel2,0(rSource);						\
	INCREMENT_SOURCE;								\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside1,24);		\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside2,28);		\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	COMPARE_TO_LOOKASIDE(cr5,rPixel2,rLookaside2);	\
	STORE_TO_LOOKASIDE(stw,rPixel1,24);				\
	COMBINE_COMPARES(cr6*4+eq,cr1*4+eq,cr5*4+eq);	\
	STORE_TO_LOOKASIDE(stw,rPixel2,28);				\
	SKIP_IF_NOT_DIRTY(cr6,DontDrawMulti);			\
	stw		rPixel1,-16(sp);						\
	stw		rPixel1,-12(sp);						\
	stw		rPixel2,-8(sp);							\
	stw		rPixel2,-4(sp);							\
	lfd		fpTemp1,-16(sp);						\
	lfd		fpTemp2,-8(sp);							\
	ADD_TO_LOOKASIDE(32);							\
	ADD_TO_DEST1(64);								\
	ADD_TO_DEST2(64);								\
	ADD_TO_DEST3(64);								\
	STORE_TO_DEST1(stfd,fpTemp1,-16);				\
	STORE_TO_DEST1(stfd,fpTemp2,-8);				\
	STORE_TO_DEST2(stfd,fpTemp1,-16);				\
	STORE_TO_DEST2(stfd,fpTemp2,-8);				\
	STORE_TO_DEST3(stfd,fpTemp1,-16);				\
	STORE_TO_DEST3(stfd,fpTemp2,-8);				\


//===================================================

#define EXPAND_PIXEL_3x_32TO32D(outLabel)			\
	lwz		rPixel1,X_OFFSET(0,4)(rSource);			\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside1,0);			\
	addi	rSource,rSource,X_INCREMENT(4);			\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	STORE_TO_LOOKASIDE(stw,rPixel1,0);				\
	SKIP_IF_NOT_DIRTY(cr1,outLabel);				\
	stw		rPixel1,-8(sp);							\
	stw		rPixel1,-4(sp);							\
	lfd		fpTemp1,-8(sp);							\
	ADD_TO_LOOKASIDE(4);							\
	ADD_TO_DEST1(12);								\
	ADD_TO_DEST2(12);								\
	ADD_TO_DEST3(12);								\
	STORE_TO_DEST1(stfd,fpTemp1,-12);				\
	STORE_TO_DEST1(stw,rPixel1,-4);					\
	STORE_TO_DEST2(stfd,fpTemp1,-12);				\
	STORE_TO_DEST2(stw,rPixel1,-4);					\
	STORE_TO_DEST3(stfd,fpTemp1,-12);				\
	STORE_TO_DEST3(stw,rPixel1,-4);					\

#define EXPAND_PIXELS_3x_32TO32D					\
	lwz		rPixel1,X_OFFSET(0,4)(rSource);			\
	lwz		rPixel2,X_OFFSET(4,4)(rSource);			\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside1,0);			\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside2,4);			\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	COMPARE_TO_LOOKASIDE(cr1,rPixel2,rLookaside2);	\
	STORE_TO_LOOKASIDE(stw,rPixel1,0);				\
	COMBINE_COMPARES(cr6*4+eq,cr1*4+eq,cr5*4+eq);	\
	STORE_TO_LOOKASIDE(stw,rPixel2,4);				\
	SKIP_IF_NOT_DIRTY(cr6,DontDraw1);				\
	stw		rPixel1,-24(sp);						\
	stw		rPixel1,-20(sp);						\
	stw		rPixel1,-16(sp);						\
	stw		rPixel2,-12(sp);						\
	stw		rPixel2,-8(sp);							\
	stw		rPixel2,-4(sp);							\
	lfd		fpTemp1,-24(sp);						\
	lfd		fpTemp2,-16(sp);						\
	lfd		fpTemp3,-8(sp);							\
	STORE_TO_DEST1(stfd,fpTemp1,0);					\
	STORE_TO_DEST1(stfd,fpTemp2,8);					\
	STORE_TO_DEST1(stfd,fpTemp3,16);				\
	STORE_TO_DEST2(stfd,fpTemp1,0);					\
	STORE_TO_DEST2(stfd,fpTemp2,8);					\
	STORE_TO_DEST2(stfd,fpTemp3,16);				\
	STORE_TO_DEST3(stfd,fpTemp1,0);					\
	STORE_TO_DEST3(stfd,fpTemp2,8);					\
	STORE_TO_DEST3(stfd,fpTemp3,16);				\
DontDraw1:;											\
	lwz		rPixel1,X_OFFSET(8,4)(rSource);			\
	lwz		rPixel2,X_OFFSET(12,4)(rSource);		\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside1,8);			\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside2,12);		\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	COMPARE_TO_LOOKASIDE(cr1,rPixel2,rLookaside2);	\
	STORE_TO_LOOKASIDE(stw,rPixel1,8);				\
	COMBINE_COMPARES(cr6*4+eq,cr1*4+eq,cr5*4+eq);	\
	STORE_TO_LOOKASIDE(stw,rPixel2,12);				\
	SKIP_IF_NOT_DIRTY(cr6,DontDraw2);				\
	stw		rPixel1,-24(sp);						\
	stw		rPixel1,-20(sp);						\
	stw		rPixel1,-16(sp);						\
	stw		rPixel2,-12(sp);						\
	stw		rPixel2,-8(sp);							\
	stw		rPixel2,-4(sp);							\
	lfd		fpTemp1,-24(sp);						\
	lfd		fpTemp2,-16(sp);						\
	lfd		fpTemp3,-8(sp);							\
	STORE_TO_DEST1(stfd,fpTemp1,24);				\
	STORE_TO_DEST1(stfd,fpTemp2,32);				\
	STORE_TO_DEST1(stfd,fpTemp3,40);				\
	STORE_TO_DEST2(stfd,fpTemp1,24);				\
	STORE_TO_DEST2(stfd,fpTemp2,32);				\
	STORE_TO_DEST2(stfd,fpTemp3,40);				\
	STORE_TO_DEST3(stfd,fpTemp1,24);				\
	STORE_TO_DEST3(stfd,fpTemp2,32);				\
	STORE_TO_DEST3(stfd,fpTemp3,40);				\
DontDraw2:;											\
	lwz		rPixel1,X_OFFSET(16,4)(rSource);		\
	lwz		rPixel2,X_OFFSET(20,4)(rSource);		\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside1,16);		\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside2,20);		\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	COMPARE_TO_LOOKASIDE(cr1,rPixel2,rLookaside2);	\
	STORE_TO_LOOKASIDE(stw,rPixel1,16);				\
	COMBINE_COMPARES(cr6*4+eq,cr1*4+eq,cr5*4+eq);	\
	STORE_TO_LOOKASIDE(stw,rPixel2,20);				\
	SKIP_IF_NOT_DIRTY(cr6,DontDraw3);				\
	stw		rPixel1,-24(sp);						\
	stw		rPixel1,-20(sp);						\
	stw		rPixel1,-16(sp);						\
	stw		rPixel2,-12(sp);						\
	stw		rPixel2,-8(sp);							\
	stw		rPixel2,-4(sp);							\
	lfd		fpTemp1,-24(sp);						\
	lfd		fpTemp2,-16(sp);						\
	lfd		fpTemp3,-8(sp);							\
	STORE_TO_DEST1(stfd,fpTemp1,48);				\
	STORE_TO_DEST1(stfd,fpTemp2,56);				\
	STORE_TO_DEST1(stfd,fpTemp3,64);				\
	STORE_TO_DEST2(stfd,fpTemp1,48);				\
	STORE_TO_DEST2(stfd,fpTemp2,56);				\
	STORE_TO_DEST2(stfd,fpTemp3,64);				\
	STORE_TO_DEST3(stfd,fpTemp1,48);				\
	STORE_TO_DEST3(stfd,fpTemp2,56);				\
	STORE_TO_DEST3(stfd,fpTemp3,64);				\
DontDraw3:;											\
	lwz		rPixel1,X_OFFSET(24,4)(rSource);		\
	lwz		rPixel2,X_OFFSET(28,4)(rSource);		\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside1,24);		\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside2,28);		\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	COMPARE_TO_LOOKASIDE(cr1,rPixel2,rLookaside2);	\
	STORE_TO_LOOKASIDE(stw,rPixel1,24);				\
	COMBINE_COMPARES(cr6*4+eq,cr1*4+eq,cr5*4+eq);	\
	STORE_TO_LOOKASIDE(stw,rPixel2,28);				\
	SKIP_IF_NOT_DIRTY(cr6,DontDrawMulti);			\
	stw		rPixel1,-24(sp);						\
	stw		rPixel1,-20(sp);						\
	stw		rPixel1,-16(sp);						\
	stw		rPixel2,-12(sp);						\
	stw		rPixel2,-8(sp);							\
	stw		rPixel2,-4(sp);							\
	lfd		fpTemp1,-24(sp);						\
	lfd		fpTemp2,-16(sp);						\
	lfd		fpTemp3,-8(sp);							\
	addi	rSource,rSource,X_INCREMENT(32);		\
	ADD_TO_LOOKASIDE(32);							\
	ADD_TO_DEST1(96);								\
	ADD_TO_DEST2(96);								\
	ADD_TO_DEST3(96);								\
	STORE_TO_DEST1(stfd,fpTemp1,-24);				\
	STORE_TO_DEST1(stfd,fpTemp2,-16);				\
	STORE_TO_DEST1(stfd,fpTemp3,-8);				\
	STORE_TO_DEST2(stfd,fpTemp1,-24);				\
	STORE_TO_DEST2(stfd,fpTemp2,-16);				\
	STORE_TO_DEST2(stfd,fpTemp3,-8);				\
	STORE_TO_DEST3(stfd,fpTemp1,-24);				\
	STORE_TO_DEST3(stfd,fpTemp2,-16);				\
	STORE_TO_DEST3(stfd,fpTemp3,-8);				\


#define EXPAND_PIXEL_3x_32TO32D_SWAPXY(outLabel)	\
	lwz		rPixel1,0(rSource);						\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside1,0);			\
	INCREMENT_SOURCE;								\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	STORE_TO_LOOKASIDE(stw,rPixel1,0);				\
	SKIP_IF_NOT_DIRTY(cr1,outLabel);				\
	stw		rPixel1,-8(sp);							\
	stw		rPixel1,-4(sp);							\
	lfd		fpTemp1,-8(sp);							\
	ADD_TO_LOOKASIDE(4);							\
	ADD_TO_DEST1(12);								\
	ADD_TO_DEST2(12);								\
	ADD_TO_DEST3(12);								\
	STORE_TO_DEST1(stfd,fpTemp1,-12);				\
	STORE_TO_DEST1(stw,rPixel1,-4);					\
	STORE_TO_DEST2(stfd,fpTemp1,-12);				\
	STORE_TO_DEST2(stw,rPixel1,-4);					\
	STORE_TO_DEST3(stfd,fpTemp1,-12);				\
	STORE_TO_DEST3(stw,rPixel1,-4);					\

#define EXPAND_PIXELS_3x_32TO32D_SWAPXY				\
	lwz		rPixel1,0(rSource);						\
	INCREMENT_SOURCE;								\
	lwz		rPixel2,0(rSource);						\
	INCREMENT_SOURCE;								\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside1,0);			\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside2,4);			\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	COMPARE_TO_LOOKASIDE(cr1,rPixel2,rLookaside2);	\
	STORE_TO_LOOKASIDE(stw,rPixel1,0);				\
	COMBINE_COMPARES(cr6*4+eq,cr1*4+eq,cr5*4+eq);	\
	STORE_TO_LOOKASIDE(stw,rPixel2,4);				\
	SKIP_IF_NOT_DIRTY(cr6,DontDraw1);				\
	stw		rPixel1,-24(sp);						\
	stw		rPixel1,-20(sp);						\
	stw		rPixel1,-16(sp);						\
	stw		rPixel2,-12(sp);						\
	stw		rPixel2,-8(sp);							\
	stw		rPixel2,-4(sp);							\
	lfd		fpTemp1,-24(sp);						\
	lfd		fpTemp2,-16(sp);						\
	lfd		fpTemp3,-8(sp);							\
	STORE_TO_DEST1(stfd,fpTemp1,0);					\
	STORE_TO_DEST1(stfd,fpTemp2,8);					\
	STORE_TO_DEST1(stfd,fpTemp3,16);				\
	STORE_TO_DEST2(stfd,fpTemp1,0);					\
	STORE_TO_DEST2(stfd,fpTemp2,8);					\
	STORE_TO_DEST2(stfd,fpTemp3,16);				\
	STORE_TO_DEST3(stfd,fpTemp1,0);					\
	STORE_TO_DEST3(stfd,fpTemp2,8);					\
	STORE_TO_DEST3(stfd,fpTemp3,16);				\
DontDraw1:;											\
	lwz		rPixel1,0(rSource);						\
	INCREMENT_SOURCE;								\
	lwz		rPixel2,0(rSource);						\
	INCREMENT_SOURCE;								\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside1,8);			\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside2,12);		\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	COMPARE_TO_LOOKASIDE(cr1,rPixel2,rLookaside2);	\
	STORE_TO_LOOKASIDE(stw,rPixel1,8);				\
	COMBINE_COMPARES(cr6*4+eq,cr1*4+eq,cr5*4+eq);	\
	STORE_TO_LOOKASIDE(stw,rPixel2,12);				\
	SKIP_IF_NOT_DIRTY(cr6,DontDraw2);				\
	stw		rPixel1,-24(sp);						\
	stw		rPixel1,-20(sp);						\
	stw		rPixel1,-16(sp);						\
	stw		rPixel2,-12(sp);						\
	stw		rPixel2,-8(sp);							\
	stw		rPixel2,-4(sp);							\
	lfd		fpTemp1,-24(sp);						\
	lfd		fpTemp2,-16(sp);						\
	lfd		fpTemp3,-8(sp);							\
	STORE_TO_DEST1(stfd,fpTemp1,24);				\
	STORE_TO_DEST1(stfd,fpTemp2,32);				\
	STORE_TO_DEST1(stfd,fpTemp3,40);				\
	STORE_TO_DEST2(stfd,fpTemp1,24);				\
	STORE_TO_DEST2(stfd,fpTemp2,32);				\
	STORE_TO_DEST2(stfd,fpTemp3,40);				\
	STORE_TO_DEST3(stfd,fpTemp1,24);				\
	STORE_TO_DEST3(stfd,fpTemp2,32);				\
	STORE_TO_DEST3(stfd,fpTemp3,40);				\
DontDraw2:;											\
	lwz		rPixel1,0(rSource);						\
	INCREMENT_SOURCE;								\
	lwz		rPixel2,0(rSource);						\
	INCREMENT_SOURCE;								\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside1,16);		\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside2,20);		\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	COMPARE_TO_LOOKASIDE(cr1,rPixel2,rLookaside2);	\
	STORE_TO_LOOKASIDE(stw,rPixel1,16);				\
	COMBINE_COMPARES(cr6*4+eq,cr1*4+eq,cr5*4+eq);	\
	STORE_TO_LOOKASIDE(stw,rPixel2,20);				\
	SKIP_IF_NOT_DIRTY(cr6,DontDraw3);				\
	stw		rPixel1,-24(sp);						\
	stw		rPixel1,-20(sp);						\
	stw		rPixel1,-16(sp);						\
	stw		rPixel2,-12(sp);						\
	stw		rPixel2,-8(sp);							\
	stw		rPixel2,-4(sp);							\
	lfd		fpTemp1,-24(sp);						\
	lfd		fpTemp2,-16(sp);						\
	lfd		fpTemp3,-8(sp);							\
	STORE_TO_DEST1(stfd,fpTemp1,48);				\
	STORE_TO_DEST1(stfd,fpTemp2,56);				\
	STORE_TO_DEST1(stfd,fpTemp3,64);				\
	STORE_TO_DEST2(stfd,fpTemp1,48);				\
	STORE_TO_DEST2(stfd,fpTemp2,56);				\
	STORE_TO_DEST2(stfd,fpTemp3,64);				\
	STORE_TO_DEST3(stfd,fpTemp1,48);				\
	STORE_TO_DEST3(stfd,fpTemp2,56);				\
	STORE_TO_DEST3(stfd,fpTemp3,64);				\
DontDraw3:;											\
	lwz		rPixel1,0(rSource);						\
	INCREMENT_SOURCE;								\
	lwz		rPixel2,0(rSource);						\
	INCREMENT_SOURCE;								\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside1,24);		\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside2,28);		\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	COMPARE_TO_LOOKASIDE(cr1,rPixel2,rLookaside2);	\
	STORE_TO_LOOKASIDE(stw,rPixel1,24);				\
	COMBINE_COMPARES(cr6*4+eq,cr1*4+eq,cr5*4+eq);	\
	STORE_TO_LOOKASIDE(stw,rPixel2,28);				\
	SKIP_IF_NOT_DIRTY(cr6,DontDrawMulti);			\
	stw		rPixel1,-24(sp);						\
	stw		rPixel1,-20(sp);						\
	stw		rPixel1,-16(sp);						\
	stw		rPixel2,-12(sp);						\
	stw		rPixel2,-8(sp);							\
	stw		rPixel2,-4(sp);							\
	lfd		fpTemp1,-24(sp);						\
	lfd		fpTemp2,-16(sp);						\
	lfd		fpTemp3,-8(sp);							\
	ADD_TO_LOOKASIDE(32);							\
	ADD_TO_DEST1(96);								\
	ADD_TO_DEST2(96);								\
	ADD_TO_DEST3(96);								\
	STORE_TO_DEST1(stfd,fpTemp1,-24);				\
	STORE_TO_DEST1(stfd,fpTemp2,-16);				\
	STORE_TO_DEST1(stfd,fpTemp3,-8);				\
	STORE_TO_DEST2(stfd,fpTemp1,-24);				\
	STORE_TO_DEST2(stfd,fpTemp2,-16);				\
	STORE_TO_DEST2(stfd,fpTemp3,-8);				\
	STORE_TO_DEST3(stfd,fpTemp1,-24);				\
	STORE_TO_DEST3(stfd,fpTemp2,-16);				\
	STORE_TO_DEST3(stfd,fpTemp3,-8);				\


//===================================================

#define EXPAND_PIXEL_1x_32TO32D_VEC(outLabel)		\
	lwz		rPixel1,X_OFFSET(0,4)(rSource);			\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside1,0);			\
	addi	rSource,rSource,X_INCREMENT(4);			\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	STORE_TO_LOOKASIDE(stw,rPixel1,0);				\
	SKIP_IF_NOT_DIRTY(cr1,outLabel);				\
	ADD_TO_LOOKASIDE(4);							\
	STORE_TO_DEST1(stw,rPixel1,0);					\
	STORE_TO_DEST2(stwx,rPixel1);					\
	STORE_TO_DEST3(stwx,rPixel1);					\
	ADD_TO_DEST(4);									\

#define EXPAND_PIXELS_1x_32TO32D_VEC										\
	lvx		vrTemp2,0,rSource;				/* load source */				\
	addi	rSource,rSource,X_INCREMENT(16);								\
	LOAD_FROM_LOOKASIDE_VEC(vrTemp0);		/* load 4 pixels of lookaside */\
	COMPARE_TO_LOOKASIDE_VEC(vrTemp0,vrTemp0,vrTemp2);	/* compare 4 pixels */	\
	SKIP_IF_NOT_DIRTY_VEC(DontDraw1);		/* skip if it is clean */		\
	STORE_TO_LOOKASIDE_VEC(vrTemp2);		/* save current source to lookaside */	\
	FLIP_PIXELS_VEC(vrTemp2);												\
																			\
	ADD_TO_LOOKASIDE(16);													\
	STORE_TO_DEST1_VEC(vrTemp2);			/* ABCD to line 1 */			\
	STORE_TO_DEST2_VEC(vrTemp2);			/* ABCD to line 2 */			\
	STORE_TO_DEST3_VEC(vrTemp2);			/* ABCD to line 3 */			\
	ADD_TO_DEST(16);														\
	b		Skip1;															\
																			\
DontDraw1:;																	\
	ADD_TO_LOOKASIDE(16);													\
	ADD_TO_DEST(16);														\
Skip1:;																		\
	lvx		vrTemp2,0,rSource;				/* load source */				\
	addi	rSource,rSource,X_INCREMENT(16);								\
	LOAD_FROM_LOOKASIDE_VEC(vrTemp0);		/* load 4 pixels of lookaside */\
	COMPARE_TO_LOOKASIDE_VEC(vrTemp0,vrTemp0,vrTemp2);	/* compare all 8 pixels */	\
	SKIP_IF_NOT_DIRTY_VEC(DontDrawMulti);	/* skip if it is clean */		\
	STORE_TO_LOOKASIDE_VEC(vrTemp2);		/* save current source to lookaside */	\
	FLIP_PIXELS_VEC(vrTemp2);												\
																			\
	ADD_TO_LOOKASIDE(16);													\
	STORE_TO_DEST1_VEC(vrTemp2);			/* EFGH to line 1 */			\
	STORE_TO_DEST2_VEC(vrTemp2);			/* EFGH to line 2 */			\
	STORE_TO_DEST3_VEC(vrTemp2);			/* EFGH to line 3 */			\
	ADD_TO_DEST(16);														\


#define EXPAND_PIXEL_2x_32TO32D_VEC(outLabel)		\
	lwz		rPixel1,X_OFFSET(0,4)(rSource);			\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside1,0);			\
	addi	rSource,rSource,X_INCREMENT(4);			\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	STORE_TO_LOOKASIDE(stw,rPixel1,0);				\
	SKIP_IF_NOT_DIRTY(cr1,outLabel);				\
	stw		rPixel1,-8(sp);							\
	stw		rPixel1,-4(sp);							\
	lfd		fpTemp1,-8(sp);							\
	ADD_TO_LOOKASIDE(4);							\
	STORE_TO_DEST1(stfd,fpTemp1,0);					\
	STORE_TO_DEST2(stfdx,fpTemp1);					\
	STORE_TO_DEST3(stfdx,fpTemp1);					\
	ADD_TO_DEST(8);									\

#define EXPAND_PIXELS_2x_32TO32D_VEC										\
	lvx		vrTemp2,0,rSource;				/* load source */				\
	addi	rSource,rSource,X_INCREMENT(16);								\
	LOAD_FROM_LOOKASIDE_VEC(vrTemp0);		/* load 4 pixels of lookaside */\
	COMPARE_TO_LOOKASIDE_VEC(vrTemp0,vrTemp0,vrTemp2);	/* compare 4 pixels */	\
	SKIP_IF_NOT_DIRTY_VEC(DontDraw1);		/* skip if it is clean */		\
	STORE_TO_LOOKASIDE_VEC(vrTemp2);		/* save current source to lookaside */	\
																			\
	ADD_TO_LOOKASIDE(16);													\
	ZOOM_FLIP_PIXELS_2x_HIGH_VEC(vrTemp0,vrTemp2);	/* AABB */				\
	ZOOM_FLIP_PIXELS_2x_LOW_VEC(vrTemp1,vrTemp2);	/* CCDD */				\
	STORE_TO_DEST1_VEC(vrTemp0);			/* AABB to line 1 */			\
	STORE_TO_DEST2_VEC(vrTemp0);			/* AABB to line 2 */			\
	STORE_TO_DEST3_VEC(vrTemp0);			/* AABB to line 3 */			\
	ADD_TO_DEST(16);														\
	STORE_TO_DEST1_VEC(vrTemp1);			/* CCDD to line 1 */			\
	STORE_TO_DEST2_VEC(vrTemp1);			/* CCDD to line 2 */			\
	STORE_TO_DEST3_VEC(vrTemp1);			/* CCDD to line 3 */			\
	ADD_TO_DEST(16);														\
	b		Skip1;															\
																			\
DontDraw1:;																	\
	ADD_TO_LOOKASIDE(16);													\
	ADD_TO_DEST(32);														\
Skip1:;																		\
	lvx		vrTemp2,0,rSource;				/* load source */				\
	addi	rSource,rSource,X_INCREMENT(16);								\
	LOAD_FROM_LOOKASIDE_VEC(vrTemp0);		/* load 4 pixels of lookaside */\
	COMPARE_TO_LOOKASIDE_VEC(vrTemp0,vrTemp0,vrTemp2);	/* compare all 8 pixels */	\
	SKIP_IF_NOT_DIRTY_VEC(DontDrawMulti);	/* skip if it is clean */		\
	STORE_TO_LOOKASIDE_VEC(vrTemp2);		/* save current source to lookaside */	\
																			\
	ADD_TO_LOOKASIDE(16);													\
	ZOOM_FLIP_PIXELS_2x_HIGH_VEC(vrTemp0,vrTemp2);	/* EEFF */				\
	ZOOM_FLIP_PIXELS_2x_LOW_VEC(vrTemp1,vrTemp2);	/* GGHH */				\
	STORE_TO_DEST1_VEC(vrTemp0);			/* EEFF to line 1 */			\
	STORE_TO_DEST2_VEC(vrTemp0);			/* EEFF to line 2 */			\
	STORE_TO_DEST3_VEC(vrTemp0);			/* EEFF to line 3 */			\
	ADD_TO_DEST(16);														\
	STORE_TO_DEST1_VEC(vrTemp1);			/* GGHH to line 1 */			\
	STORE_TO_DEST2_VEC(vrTemp1);			/* GGHH to line 2 */			\
	STORE_TO_DEST3_VEC(vrTemp1);			/* GGHH to line 3 */			\
	ADD_TO_DEST(16);														\


#define EXPAND_PIXEL_3x_32TO32D_VEC(outLabel)		\
	lwz		rPixel1,X_OFFSET(0,4)(rSource);			\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside1,0);			\
	addi	rSource,rSource,X_INCREMENT(4);			\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	STORE_TO_LOOKASIDE(stw,rPixel1,0);				\
	SKIP_IF_NOT_DIRTY(cr1,outLabel);				\
	stw		rPixel1,-8(sp);							\
	stw		rPixel1,-4(sp);							\
	lfd		fpTemp1,-8(sp);							\
	ADD_TO_LOOKASIDE(4);							\
	STORE_TO_DEST1(stfd,fpTemp1,0);					\
	STORE_TO_DEST2(stfdx,fpTemp1);					\
	STORE_TO_DEST3(stfdx,fpTemp1);					\
	STORE_TO_DEST1(stwu,rPixel1,8);					\
	STORE_TO_DEST2(stwx,rPixel1);					\
	STORE_TO_DEST3(stwx,rPixel1);					\
	ADD_TO_DEST(4);									\

#define EXPAND_PIXELS_3x_32TO32D_VEC										\
	lvx		vrTemp2,0,rSource;				/* load source */				\
	addi	rSource,rSource,X_INCREMENT(16);								\
	LOAD_FROM_LOOKASIDE_VEC(vrTemp0);		/* load 4 pixels of lookaside */\
	COMPARE_TO_LOOKASIDE_VEC(vrTemp0,vrTemp0,vrTemp2);	/* compare 4 pixels */	\
	SKIP_IF_NOT_DIRTY_VEC(DontDraw1);		/* skip if it is clean */		\
	STORE_TO_LOOKASIDE_VEC(vrTemp2);		/* save current source to lookaside */	\
																			\
	ADD_TO_LOOKASIDE(16);													\
	vperm	vrTemp0,vrTemp2,vrTemp2,vrPixelZoom0;	/* AAAB */				\
	vperm	vrTemp1,vrTemp2,vrTemp2,vrPixelZoom1;	/* BBCC */				\
	STORE_TO_DEST1_VEC(vrTemp0);			/* AAAB to line 1 */			\
	STORE_TO_DEST2_VEC(vrTemp0);			/* AAAB to line 2 */			\
	STORE_TO_DEST3_VEC(vrTemp0);			/* AAAB to line 3 */			\
	ADD_TO_DEST(16);														\
	vperm	vrTemp2,vrTemp2,vrTemp2,vrPixelZoom2;	/* CDDD */				\
	STORE_TO_DEST1_VEC(vrTemp1);			/* BBCC to line 1 */			\
	STORE_TO_DEST2_VEC(vrTemp1);			/* BBCC to line 2 */			\
	STORE_TO_DEST3_VEC(vrTemp1);			/* BBCC to line 3 */			\
	ADD_TO_DEST(16);														\
	STORE_TO_DEST1_VEC(vrTemp2);			/* CDDD to line 1 */			\
	STORE_TO_DEST2_VEC(vrTemp2);			/* CDDD to line 2 */			\
	STORE_TO_DEST3_VEC(vrTemp2);			/* CDDD to line 3 */			\
	ADD_TO_DEST(16);														\
	b		Skip1;															\
																			\
DontDraw1:;																	\
	ADD_TO_LOOKASIDE(16);													\
	ADD_TO_DEST(48);														\
Skip1:;																		\
	lvx		vrTemp2,0,rSource;				/* load source */				\
	addi	rSource,rSource,X_INCREMENT(16);								\
	LOAD_FROM_LOOKASIDE_VEC(vrTemp0);		/* load 4 pixels of lookaside */\
	COMPARE_TO_LOOKASIDE_VEC(vrTemp0,vrTemp0,vrTemp2);	/* compare all 8 pixels */	\
	SKIP_IF_NOT_DIRTY_VEC(DontDrawMulti);	/* skip if it is clean */		\
	STORE_TO_LOOKASIDE_VEC(vrTemp2);		/* save current source to lookaside */	\
																			\
	ADD_TO_LOOKASIDE(16);													\
	vperm	vrTemp0,vrTemp2,vrTemp2,vrPixelZoom0;	/* EEEF */				\
	vperm	vrTemp1,vrTemp2,vrTemp2,vrPixelZoom1;	/* FFGG */				\
	STORE_TO_DEST1_VEC(vrTemp0);			/* EEEF to line 1 */			\
	STORE_TO_DEST2_VEC(vrTemp0);			/* EEEF to line 2 */			\
	STORE_TO_DEST3_VEC(vrTemp0);			/* EEEF to line 3 */			\
	ADD_TO_DEST(16);														\
	vperm	vrTemp2,vrTemp2,vrTemp2,vrPixelZoom2;	/* GHHH */				\
	STORE_TO_DEST1_VEC(vrTemp1);			/* FFGG to line 1 */			\
	STORE_TO_DEST2_VEC(vrTemp1);			/* FFGG to line 2 */			\
	STORE_TO_DEST3_VEC(vrTemp1);			/* FFGG to line 3 */			\
	ADD_TO_DEST(16);														\
	STORE_TO_DEST1_VEC(vrTemp2);			/* GHHH to line 1 */			\
	STORE_TO_DEST2_VEC(vrTemp2);			/* GHHH to line 2 */			\
	STORE_TO_DEST3_VEC(vrTemp2);			/* GHHH to line 3 */			\
	ADD_TO_DEST(16);														\


//===================================================
#pragma mark Expand 32

#define EXPAND_PIXEL_1x_32TO32L(outLabel)			\
	lwz		rPixel1,X_OFFSET(0,4)(rSource);			\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside1,0);			\
	addi	rSource,rSource,X_INCREMENT(4);			\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	STORE_TO_LOOKASIDE(stw,rPixel1,0);				\
	SKIP_IF_NOT_DIRTY(cr1,outLabel);				\
	rlwinm	rPixel1,rPixel1,3,13,28;				\
	lwzx	rPixel1,rLookup,rPixel1;				\
	ADD_TO_LOOKASIDE(4);							\
	ADD_TO_DEST1(4);								\
	ADD_TO_DEST2(4);								\
	ADD_TO_DEST3(4);								\
	STORE_TO_DEST1(stw,rPixel1,-4);					\
	STORE_TO_DEST2(stw,rPixel1,-4);					\
	STORE_TO_DEST3(stw,rPixel1,-4);					\

#define EXPAND_PIXELS_1x_32TO32L					\
	lwz		rPixel1,X_OFFSET(0,4)(rSource);			\
	lwz		rPixel2,X_OFFSET(4,4)(rSource);			\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside1,0);			\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside2,4);			\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	COMPARE_TO_LOOKASIDE(cr5,rPixel2,rLookaside2);	\
	STORE_TO_LOOKASIDE(stw,rPixel1,0);				\
	COMBINE_COMPARES(cr6*4+eq,cr1*4+eq,cr5*4+eq);	\
	STORE_TO_LOOKASIDE(stw,rPixel2,4);				\
	SKIP_IF_NOT_DIRTY(cr6,DontDraw1);				\
	rlwinm	rPixel1,rPixel1,3,13,28;				\
	rlwinm	rPixel2,rPixel2,3,13,28;				\
	lwzx	rPixel1,rLookup,rPixel1;				\
	lwzx	rPixel2,rLookup,rPixel2;				\
	stw		rPixel1,-8(sp);							\
	stw		rPixel2,-4(sp);							\
	lfd		fpTemp1,-8(sp);							\
	STORE_TO_DEST1(stfd,fpTemp1,0);					\
	STORE_TO_DEST2(stfd,fpTemp1,0);					\
	STORE_TO_DEST3(stfd,fpTemp1,0);					\
DontDraw1:;											\
	lwz		rPixel1,X_OFFSET(8,4)(rSource);			\
	lwz		rPixel2,X_OFFSET(12,4)(rSource);		\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside1,8);			\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside2,12);		\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	COMPARE_TO_LOOKASIDE(cr5,rPixel2,rLookaside2);	\
	STORE_TO_LOOKASIDE(stw,rPixel1,8);				\
	COMBINE_COMPARES(cr6*4+eq,cr1*4+eq,cr5*4+eq);	\
	STORE_TO_LOOKASIDE(stw,rPixel2,12);				\
	SKIP_IF_NOT_DIRTY(cr6,DontDraw2);				\
	rlwinm	rPixel1,rPixel1,3,13,28;				\
	rlwinm	rPixel2,rPixel2,3,13,28;				\
	lwzx	rPixel1,rLookup,rPixel1;				\
	lwzx	rPixel2,rLookup,rPixel2;				\
	stw		rPixel1,-8(sp);							\
	stw		rPixel2,-4(sp);							\
	lfd		fpTemp1,-8(sp);							\
	STORE_TO_DEST1(stfd,fpTemp1,8);					\
	STORE_TO_DEST2(stfd,fpTemp1,8);					\
	STORE_TO_DEST3(stfd,fpTemp1,8);					\
DontDraw2:;											\
	lwz		rPixel1,X_OFFSET(16,4)(rSource);		\
	lwz		rPixel2,X_OFFSET(20,4)(rSource);		\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside1,16);		\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside2,20);		\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	COMPARE_TO_LOOKASIDE(cr5,rPixel2,rLookaside2);	\
	STORE_TO_LOOKASIDE(stw,rPixel1,16);				\
	COMBINE_COMPARES(cr6*4+eq,cr1*4+eq,cr5*4+eq);	\
	STORE_TO_LOOKASIDE(stw,rPixel2,20);				\
	SKIP_IF_NOT_DIRTY(cr6,DontDraw3);				\
	rlwinm	rPixel1,rPixel1,3,13,28;				\
	rlwinm	rPixel2,rPixel2,3,13,28;				\
	lwzx	rPixel1,rLookup,rPixel1;				\
	lwzx	rPixel2,rLookup,rPixel2;				\
	stw		rPixel1,-8(sp);							\
	stw		rPixel2,-4(sp);							\
	lfd		fpTemp1,-8(sp);							\
	STORE_TO_DEST1(stfd,fpTemp1,16);				\
	STORE_TO_DEST2(stfd,fpTemp1,16);				\
	STORE_TO_DEST3(stfd,fpTemp1,16);				\
DontDraw3:;											\
	lwz		rPixel1,X_OFFSET(24,4)(rSource);		\
	lwz		rPixel2,X_OFFSET(28,4)(rSource);		\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside1,24);		\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside2,28);		\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	COMPARE_TO_LOOKASIDE(cr5,rPixel2,rLookaside2);	\
	STORE_TO_LOOKASIDE(stw,rPixel1,24);				\
	COMBINE_COMPARES(cr6*4+eq,cr1*4+eq,cr5*4+eq);	\
	STORE_TO_LOOKASIDE(stw,rPixel2,28);				\
	SKIP_IF_NOT_DIRTY(cr6,DontDrawMulti);			\
	rlwinm	rPixel1,rPixel1,3,13,28;				\
	rlwinm	rPixel2,rPixel2,3,13,28;				\
	lwzx	rPixel1,rLookup,rPixel1;				\
	lwzx	rPixel2,rLookup,rPixel2;				\
	stw		rPixel1,-8(sp);							\
	stw		rPixel2,-4(sp);							\
	addi	rSource,rSource,X_INCREMENT(32);		\
	ADD_TO_LOOKASIDE(32);							\
	ADD_TO_DEST1(32);								\
	ADD_TO_DEST2(32);								\
	ADD_TO_DEST3(32);								\
	lfd		fpTemp1,-8(sp);							\
	STORE_TO_DEST1(stfd,fpTemp1,-8);				\
	STORE_TO_DEST2(stfd,fpTemp1,-8);				\
	STORE_TO_DEST3(stfd,fpTemp1,-8);				\


#if 0

#define EXPAND_PIXEL_1x_32TO32L_VEC(outLabel)		\
	lwz		rPixel1,X_OFFSET(0,4)(rSource);			\
	LOAD_FROM_LOOKASIDE(lwz,rLookaside1,0);			\
	addi	rSource,rSource,X_INCREMENT(4);			\
	COMPARE_TO_LOOKASIDE(cr1,rPixel1,rLookaside1);	\
	STORE_TO_LOOKASIDE(stw,rPixel1,0);				\
	SKIP_IF_NOT_DIRTY(cr1,outLabel);				\
	rlwinm	rPixel1,rPixel1,3,13,28;				\
	lwzx	rPixel1,rLookup,rPixel1;				\
	ADD_TO_LOOKASIDE(4);							\
	STORE_TO_DEST1(stw,rPixel1,0);					\
	STORE_TO_DEST2(stwx,rPixel1);					\
	STORE_TO_DEST3(stwx,rPixel1);					\
	ADD_TO_DEST(4);									\

#if 0

#define EXPAND_PIXELS_1x_32TO32L_VEC										\
	LOAD_FROM_SOURCE_FOR_DELTA_VEC(vrTemp2);/* load source */				\
	addi	rSource,rSource,X_INCREMENT(16);								\
	LOAD_FROM_LOOKASIDE_VEC(vrTemp0);		/* load 4 pixels of lookaside */\
	COMPARE_TO_LOOKASIDE_VEC(vrTemp0,vrTemp0,vrTemp2);	/* compare 4 pixels */	\
	SKIP_IF_NOT_DIRTY_VEC(DontDraw1);		/* skip if it is clean */		\
	STORE_TO_LOOKASIDE_VEC(vrTemp2);		/* save current source to lookaside */	\
																			\
	lwz		rPixel1,X_OFFSET(0,4)-X_INCREMENT(16)(rSource);					\
	lwz		rPixel2,X_OFFSET(4,4)-X_INCREMENT(16)(rSource);					\
	lwz		rPixel3,X_OFFSET(8,4)-X_INCREMENT(16)(rSource);					\
	lwz		rPixel4,X_OFFSET(12,4)-X_INCREMENT(16)(rSource);				\
	rlwinm	rPixel1,rPixel1,3,13,28;				\
	rlwinm	rPixel2,rPixel2,3,13,28;				\
	rlwinm	rPixel3,rPixel3,3,13,28;				\
	rlwinm	rPixel4,rPixel4,3,13,28;				\
	lwzx	rPixel1,rLookup,rPixel1;				\
	lwzx	rPixel2,rLookup,rPixel2;				\
	lwzx	rPixel3,rLookup,rPixel3;				\
	lwzx	rPixel4,rLookup,rPixel4;				\
	stw		rPixel1,0(rPTmpVecSave);		/*A*/	\
	stw		rPixel2,4(rPTmpVecSave);		/*B*/	\
	stw		rPixel3,8(rPTmpVecSave);		/*C*/	\
	stw		rPixel4,12(rPTmpVecSave);		/*D*/	\
																			\
	ADD_TO_LOOKASIDE(16);													\
	lvx		vrTemp2,0,rPTmpVecSave;			/* load 4 pixels */				\
	STORE_TO_DEST1_VEC(vrTemp2);			/* ABCD to line 1 */			\
	STORE_TO_DEST2_VEC(vrTemp2);			/* ABCD to line 2 */			\
	STORE_TO_DEST3_VEC(vrTemp2);			/* ABCD to line 3 */			\
	ADD_TO_DEST(16);														\
	b		Skip1;															\
																			\
DontDraw1:;																	\
	ADD_TO_LOOKASIDE(16);													\
	ADD_TO_DEST(16);														\
Skip1:;																		\
	LOAD_FROM_SOURCE_FOR_DELTA_VEC(vrTemp2);/* load source */				\
	addi	rSource,rSource,X_INCREMENT(16);								\
	LOAD_FROM_LOOKASIDE_VEC(vrTemp0);		/* load 4 pixels of lookaside */\
	COMPARE_TO_LOOKASIDE_VEC(vrTemp0,vrTemp0,vrTemp2);	/* compare all 8 pixels */	\
	SKIP_IF_NOT_DIRTY_VEC(DontDrawMulti);	/* skip if it is clean */		\
	STORE_TO_LOOKASIDE_VEC(vrTemp2);		/* save current source to lookaside */	\
																			\
	lwz		rPixel1,X_OFFSET(0,4)-X_INCREMENT(16)(rSource);					\
	lwz		rPixel2,X_OFFSET(4,4)-X_INCREMENT(16)(rSource);					\
	lwz		rPixel3,X_OFFSET(8,4)-X_INCREMENT(16)(rSource);					\
	lwz		rPixel4,X_OFFSET(12,4)-X_INCREMENT(16)(rSource);				\
	rlwinm	rPixel1,rPixel1,3,13,28;				\
	rlwinm	rPixel2,rPixel2,3,13,28;				\
	rlwinm	rPixel3,rPixel3,3,13,28;				\
	rlwinm	rPixel4,rPixel4,3,13,28;				\
	lwzx	rPixel1,rLookup,rPixel1;				\
	lwzx	rPixel2,rLookup,rPixel2;				\
	lwzx	rPixel3,rLookup,rPixel3;				\
	lwzx	rPixel4,rLookup,rPixel4;				\
	stw		rPixel1,0(rPTmpVecSave);		/*A*/	\
	stw		rPixel2,4(rPTmpVecSave);		/*B*/	\
	stw		rPixel3,8(rPTmpVecSave);		/*C*/	\
	stw		rPixel4,12(rPTmpVecSave);		/*D*/	\
																			\
	ADD_TO_LOOKASIDE(16);													\
	lvx		vrTemp2,0,rPTmpVecSave;			/* load 4 pixels */				\
	STORE_TO_DEST1_VEC(vrTemp2);			/* EFGH to line 1 */			\
	STORE_TO_DEST2_VEC(vrTemp2);			/* EFGH to line 2 */			\
	STORE_TO_DEST3_VEC(vrTemp2);			/* EFGH to line 3 */			\
	ADD_TO_DEST(16);														\

#else

#define EXPAND_PIXELS_1x_32TO32L_VEC										\
	LOAD_FROM_SOURCE_FOR_DELTA_VEC(vrTemp2);/* load source */				\
	addi	rSource,rSource,X_INCREMENT(16);								\
	LOAD_FROM_LOOKASIDE_VEC(vrTemp0);		/* load 4 pixels of lookaside */\
	COMPARE_TO_LOOKASIDE_VEC(vrTemp0,vrTemp0,vrTemp2);	/* compare 4 pixels */	\
	SKIP_IF_NOT_DIRTY_VEC(DontDraw1);		/* skip if it is clean */		\
	STORE_TO_LOOKASIDE_VEC(vrTemp2);		/* save current source to lookaside */	\
																			\
	vand	vrTemp2,vrTemp2,vrOffsetMask;									\
	lwz		rPixel1,X_OFFSET(0,4)-X_INCREMENT(16)(rSource);					\
	vslh	vrTemp2,vrTemp2,vrOffsetShift;									\
	lwz		rPixel2,X_OFFSET(4,4)-X_INCREMENT(16)(rSource);					\
	vperm	vrTemp2,vrTemp2,vrTemp2,vrOffsetPerm;							\
	lwz		rPixel3,X_OFFSET(8,4)-X_INCREMENT(16)(rSource);					\
	rlwinm	rPixel1,rPixel1,3,13,28;										\
	lwz		rPixel4,X_OFFSET(12,4)-X_INCREMENT(16)(rSource);				\
	rlwinm	rPixel2,rPixel2,3,13,28;										\
	lvewx	vrPixel1,rLookup,rPixel1;										\
	rlwinm	rPixel3,rPixel3,3,13,28;										\
	lvewx	vrPixel2,rLookup,rPixel2;										\
	rlwinm	rPixel4,rPixel4,3,13,28;										\
	lvewx	vrPixel3,rLookup,rPixel3;										\
	vaddubm	vrTemp2,vrTemp2,vrOffsetOffset;									\
	lvewx	vrPixel4,rLookup,rPixel4;										\
	vperm	vrPixel1,vrPixel1,vrPixel2,vrTemp2;								\
	vperm	vrPixel3,vrPixel3,vrPixel4,vrTemp2;								\
	vsel	vrPixel1,vrPixel1,vrPixel3,vrPixelMerge2;						\
																			\
	ADD_TO_LOOKASIDE(16);													\
	STORE_TO_DEST1_VEC(vrTemp2);			/* ABCD to line 1 */			\
	STORE_TO_DEST2_VEC(vrTemp2);			/* ABCD to line 2 */			\
	STORE_TO_DEST3_VEC(vrTemp2);			/* ABCD to line 3 */			\
	ADD_TO_DEST(16);														\
	b		Skip1;															\
																			\
DontDraw1:;																	\
	ADD_TO_LOOKASIDE(16);													\
	ADD_TO_DEST(16);														\
Skip1:;																		\
	LOAD_FROM_SOURCE_FOR_DELTA_VEC(vrTemp2);/* load source */				\
	addi	rSource,rSource,X_INCREMENT(16);								\
	LOAD_FROM_LOOKASIDE_VEC(vrTemp0);		/* load 4 pixels of lookaside */\
	COMPARE_TO_LOOKASIDE_VEC(vrTemp0,vrTemp0,vrTemp2);	/* compare all 8 pixels */	\
	SKIP_IF_NOT_DIRTY_VEC(DontDrawMulti);	/* skip if it is clean */		\
	STORE_TO_LOOKASIDE_VEC(vrTemp2);		/* save current source to lookaside */	\
																			\
	vand	vrTemp2,vrTemp2,vrOffsetMask;									\
	lwz		rPixel1,X_OFFSET(0,4)-X_INCREMENT(16)(rSource);					\
	vslh	vrTemp2,vrTemp2,vrOffsetShift;									\
	lwz		rPixel2,X_OFFSET(4,4)-X_INCREMENT(16)(rSource);					\
	vperm	vrTemp2,vrTemp2,vrTemp2,vrOffsetPerm;							\
	lwz		rPixel3,X_OFFSET(8,4)-X_INCREMENT(16)(rSource);					\
	rlwinm	rPixel1,rPixel1,3,13,28;										\
	lwz		rPixel4,X_OFFSET(12,4)-X_INCREMENT(16)(rSource);				\
	rlwinm	rPixel2,rPixel2,3,13,28;										\
	lvewx	vrPixel1,rLookup,rPixel1;										\
	rlwinm	rPixel3,rPixel3,3,13,28;										\
	lvewx	vrPixel2,rLookup,rPixel2;										\
	rlwinm	rPixel4,rPixel4,3,13,28;										\
	lvewx	vrPixel3,rLookup,rPixel3;										\
	vaddubm	vrTemp2,vrTemp2,vrOffsetOffset;									\
	lvewx	vrPixel4,rLookup,rPixel4;										\
	vperm	vrPixel1,vrPixel1,vrPixel2,vrTemp2;								\
	vperm	vrPixel3,vrPixel3,vrPixel4,vrTemp2;								\
	vsel	vrPixel1,vrPixel1,vrPixel3,vrPixelMerge2;						\
																			\
	ADD_TO_LOOKASIDE(16);													\
	STORE_TO_DEST1_VEC(vrTemp2);			/* ABCD to line 1 */			\
	STORE_TO_DEST2_VEC(vrTemp2);			/* ABCD to line 2 */			\
	STORE_TO_DEST3_VEC(vrTemp2);			/* ABCD to line 3 */			\
	ADD_TO_DEST(16);														\

#endif

#endif



#define BLITTER_RECURSIVE_INCLUDE

// DisplayParameters struct
// these parameters are fixed, and valid at allocate time:
/*width=0				// full bitmap width (UInt32)
height=4			// full bitmap height (UInt32)
depth=8				// supported depth (UInt32)
aspect=12			// aspect ratio = width / height (double) ** note that offset may be either 12 or 16 according to compiler settings (yes this structure sucks)

visxoffset=20		// x offset of the visible area (UInt32)
visyoffset=24		// y offset of the visible area (UInt32)
viswidth=28			// width of the visible area (UInt32)
visheight=32		// height of the visible area (UInt32)

vector=36			// true if a vector game (Boolean)

// these parameters may change frame-to-frame, and are not valid at allocate time:
device=40			// device we're drawing to (GDHandle)
window=44			// window we're drawing to (WindowRef)

fullscreen=48		// full screen? (Boolean)
scale=52			// scale factor (UInt32)
interlace=56		// interlace? (Boolean)

colortable=60*/		// handle to the adjusted color table (8bpp only; NULL for 16bpp) (CTabHandle)
DisplayParameters.lookup16=64			// adjusted 16bpp lookup table (16bpp only; NULL for 8bpp) (UInt32 *)
//paletteDirty=68		// true if the palette is dirty (Boolean)

//bits=72				// pointer to the screen bits (void *)
DisplayParameters.deltabits=76		// pointer to extra screen bits for delta blits (void *)
//rowbytes=80			// rowbytes of the screen bits (UInt32)

//full=84				// true to do a full update regardless of dirty (Boolean)
DisplayParameters.curdirty=88			// pointer to the current dirty bits (UInt8 *)
DisplayParameters.prevdirty=92		// pointer to the previous dirty bits (UInt8 *)
DisplayParameters.dirtyrowshift=96	// log2 of the rowbytes of the dirty maps (UInt32)

//
//	This was added to the 1.2 API, and may change frame-to-frame
//
//throttle=100		// is rendering speed throttled? (Boolean)

//
// Added for 1.4, shouldn't affect earlier versions
//
DisplayParameters.lookup32=104		// adjusted 32bpp lookup table (32bpp only; NULL for 8bpp) (UInt32 *)
//grafPort=108		// grafPort we're drawing into (CGrafPtr)

// Register definitions
// Standard stack pointer
#define sp r1
// Function parameters
#define inSource				r3
#define inDest					r4
#define inSourceRowBytes		r5
#define inDestRowBytes			r6
#define inWidth					r7
#define inHeight				r8
#define inParams				r9
// Local variables
#define rSource					r10
#define rDest1					r11
#define rPixel1					r12
#define rPixel2					r31
#define rPixel3					r30
#define rPixel4					r29
#define rPixel5					r28
#define rPixel6					r27
#define rLookup					r26
#define rDirtyMap				r25
#define rLookaside1				r24
#define rDirty					r23
#define rLookaside2				r22
#define rDirtyRowBytes			r21
#define rDirtyMask				r20
#define rDest2					r19
#define rDest3					r18
#define rPixel7					r17 /* altivec */
#define rLeftEdge				r16 /* altivec */
#define vrsave_save				r15 /* altivec */
#define rLastNonVolatile		r15

// Altivec only locals
//#define rPTmpVecSave			r19
#define rPixel8					/*r14*/rPixel1
#define doubleDestRowBytes		r18


// fp registers (non-altivec code mostly)
#define fpTemp1					f0
#define fpTemp2					f1
#define fpTemp3					f2

// Altivec registers
#define vrTemp0					v0
#define vrTemp1					v1
#define vrTemp2					v2
#define vrTemp3					v15

#define vrPixelZoom0			v3
#define vrPixelZoom1			v4
#define vrPixelZoom2			v5

#define vrOffsetMask			v6
#define vrVectorFlip			v6
#define vrOffsetShift			v7
#define vrOffsetPerm			v8
#define vrOffsetPerm2			v14
#define	vrOffsetOffset			v9

#define vrPixel1				v10
#define vrPixel2				v11
#define vrPixel3				v12
#define vrPixel4				v13
#define vrPixel5				vrPixel2
#define vrPixel6				vrTemp0
#define vrPixel7				vrPixel4
#define vrPixel8				vrTemp1

#define vrPixelMerge1			vrPixelZoom0
#define vrPixelMerge2			vrPixelZoom1

#define vrsave					256
#define cr1						1
#define cr5						5
#define cr6						6
#define eq						2


// types of dirty/delta updating
#define NO_DIRTY				0
#define SINGLE_BUFFER_DIRTY		2
#define PIXEL_DELTA				3
#define PIXEL_DELTA_FULL		4


#pragma mark Blitter function generation

//===============================================================================
//	15bpp Blitters
//===============================================================================

#define SRC_BITS_PER_PIXEL	16
#define DST_BITS_PER_PIXEL	16
#define HAS_LOOKUP			0

// 1x1 15-bit blitters

#define VERTICAL_EXPAND		1
#define VERTICAL_EXPAND_EX	0
#define HORIZONTAL_EXPAND	1

#define DIRTY_OPTION		NO_DIRTY

#define SWAP_XY				0

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#define SWAP_XY				1

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#undef DIRTY_OPTION

#define DIRTY_OPTION		SINGLE_BUFFER_DIRTY

#define SWAP_XY				0

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#define SWAP_XY				1

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#undef DIRTY_OPTION

#undef HORIZONTAL_EXPAND
#undef VERTICAL_EXPAND_EX
#undef VERTICAL_EXPAND



// 1x2 15-bit blitters

#define VERTICAL_EXPAND		2
#define VERTICAL_EXPAND_EX	0
#define HORIZONTAL_EXPAND	1

#define DIRTY_OPTION		NO_DIRTY

#define SWAP_XY				0

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#define SWAP_XY				1

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#undef DIRTY_OPTION

#define DIRTY_OPTION		PIXEL_DELTA

#define SWAP_XY				0

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#define SWAP_XY				1

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#undef DIRTY_OPTION

#define DIRTY_OPTION		PIXEL_DELTA_FULL

#define SWAP_XY				0

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#define SWAP_XY				1

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#undef DIRTY_OPTION

#undef HORIZONTAL_EXPAND
#undef VERTICAL_EXPAND_EX
#undef VERTICAL_EXPAND



// 1x2 interlaced 15-bit blitters

#define VERTICAL_EXPAND		1
#define VERTICAL_EXPAND_EX	1
#define HORIZONTAL_EXPAND	1

#define DIRTY_OPTION		NO_DIRTY

#define SWAP_XY				0

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#define SWAP_XY				1

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#undef DIRTY_OPTION

#define DIRTY_OPTION		PIXEL_DELTA

#define SWAP_XY				0

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#define SWAP_XY				1

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#undef DIRTY_OPTION

#define DIRTY_OPTION		PIXEL_DELTA_FULL

#define SWAP_XY				0

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#define SWAP_XY				1

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#undef DIRTY_OPTION

#undef HORIZONTAL_EXPAND
#undef VERTICAL_EXPAND_EX
#undef VERTICAL_EXPAND



// 2x2 15-bit blitters

#define VERTICAL_EXPAND		2
#define VERTICAL_EXPAND_EX	0
#define HORIZONTAL_EXPAND	2

#define DIRTY_OPTION		NO_DIRTY

#define SWAP_XY				0

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#define SWAP_XY				1

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#undef DIRTY_OPTION

#define DIRTY_OPTION		PIXEL_DELTA

#define SWAP_XY				0

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#define SWAP_XY				1

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#undef DIRTY_OPTION

#define DIRTY_OPTION		PIXEL_DELTA_FULL

#define SWAP_XY				0

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#define SWAP_XY				1

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#undef DIRTY_OPTION

#undef HORIZONTAL_EXPAND
#undef VERTICAL_EXPAND_EX
#undef VERTICAL_EXPAND



// 2x2 interlaced 15-bit blitters

#define VERTICAL_EXPAND		1
#define VERTICAL_EXPAND_EX	1
#define HORIZONTAL_EXPAND	2

#define DIRTY_OPTION		NO_DIRTY

#define SWAP_XY				0

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#define SWAP_XY				1

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#undef DIRTY_OPTION

#define DIRTY_OPTION		PIXEL_DELTA

#define SWAP_XY				0

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#define SWAP_XY				1

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#undef DIRTY_OPTION

#define DIRTY_OPTION		PIXEL_DELTA_FULL

#define SWAP_XY				0

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#define SWAP_XY				1

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#undef DIRTY_OPTION

#undef HORIZONTAL_EXPAND
#undef VERTICAL_EXPAND_EX
#undef VERTICAL_EXPAND



// 3x3 15-bit blitters

#define VERTICAL_EXPAND		3
#define VERTICAL_EXPAND_EX	0
#define HORIZONTAL_EXPAND	3

#define DIRTY_OPTION		NO_DIRTY

#define SWAP_XY				0

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#define SWAP_XY				1

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#undef DIRTY_OPTION

#define DIRTY_OPTION		PIXEL_DELTA

#define SWAP_XY				0

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#define SWAP_XY				1

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#undef DIRTY_OPTION

#define DIRTY_OPTION		PIXEL_DELTA_FULL

#define SWAP_XY				0

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#define SWAP_XY				1

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#undef DIRTY_OPTION

#undef HORIZONTAL_EXPAND
#undef VERTICAL_EXPAND_EX
#undef VERTICAL_EXPAND



// 3x3 interlaced 15-bit blitters

#define VERTICAL_EXPAND		2
#define VERTICAL_EXPAND_EX	1
#define HORIZONTAL_EXPAND	3

#define DIRTY_OPTION		NO_DIRTY

#define SWAP_XY				0

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#define SWAP_XY				1

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#undef DIRTY_OPTION

#define DIRTY_OPTION		PIXEL_DELTA

#define SWAP_XY				0

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#define SWAP_XY				1

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#undef DIRTY_OPTION

#define DIRTY_OPTION		PIXEL_DELTA_FULL

#define SWAP_XY				0

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#define SWAP_XY				1

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#undef DIRTY_OPTION

#undef HORIZONTAL_EXPAND
#undef VERTICAL_EXPAND_EX
#undef VERTICAL_EXPAND

#undef HAS_LOOKUP
#undef DST_BITS_PER_PIXEL
#undef SRC_BITS_PER_PIXEL


//===============================================================================
//	16bpp Blitters
//===============================================================================

#define SRC_BITS_PER_PIXEL	16
#define DST_BITS_PER_PIXEL	16
#define HAS_LOOKUP			1

// 1x1 16-bit blitters

#define VERTICAL_EXPAND		1
#define VERTICAL_EXPAND_EX	0
#define HORIZONTAL_EXPAND	1

#define DIRTY_OPTION		NO_DIRTY

#define SWAP_XY				0

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#define SWAP_XY				1

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#undef DIRTY_OPTION

#define DIRTY_OPTION		SINGLE_BUFFER_DIRTY

#define SWAP_XY				0

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#define SWAP_XY				1

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#undef DIRTY_OPTION

#undef HORIZONTAL_EXPAND
#undef VERTICAL_EXPAND_EX
#undef VERTICAL_EXPAND



// 1x2 16-bit blitters

#define VERTICAL_EXPAND		2
#define VERTICAL_EXPAND_EX	0
#define HORIZONTAL_EXPAND	1

#define DIRTY_OPTION		NO_DIRTY

#define SWAP_XY				0

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#define SWAP_XY				1

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#undef DIRTY_OPTION

#define DIRTY_OPTION		PIXEL_DELTA

#define SWAP_XY				0

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#define SWAP_XY				1

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#undef DIRTY_OPTION

#define DIRTY_OPTION		PIXEL_DELTA_FULL

#define SWAP_XY				0

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#define SWAP_XY				1

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#undef DIRTY_OPTION

#undef HORIZONTAL_EXPAND
#undef VERTICAL_EXPAND_EX
#undef VERTICAL_EXPAND



// 1x2 interlaced 16-bit blitters

#define VERTICAL_EXPAND		1
#define VERTICAL_EXPAND_EX	1
#define HORIZONTAL_EXPAND	1

#define DIRTY_OPTION		NO_DIRTY

#define SWAP_XY				0

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#define SWAP_XY				1

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#undef DIRTY_OPTION

#define DIRTY_OPTION		PIXEL_DELTA

#define SWAP_XY				0

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#define SWAP_XY				1

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#undef DIRTY_OPTION

#define DIRTY_OPTION		PIXEL_DELTA_FULL

#define SWAP_XY				0

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#define SWAP_XY				1

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#undef DIRTY_OPTION

#undef HORIZONTAL_EXPAND
#undef VERTICAL_EXPAND_EX
#undef VERTICAL_EXPAND



// 2x2 16-bit blitters

#define VERTICAL_EXPAND		2
#define VERTICAL_EXPAND_EX	0
#define HORIZONTAL_EXPAND	2

#define DIRTY_OPTION		NO_DIRTY

#define SWAP_XY				0

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#define SWAP_XY				1

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#undef DIRTY_OPTION

#define DIRTY_OPTION		PIXEL_DELTA

#define SWAP_XY				0

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#define SWAP_XY				1

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#undef DIRTY_OPTION

#define DIRTY_OPTION		PIXEL_DELTA_FULL

#define SWAP_XY				0

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#define SWAP_XY				1

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#undef DIRTY_OPTION

#undef HORIZONTAL_EXPAND
#undef VERTICAL_EXPAND_EX
#undef VERTICAL_EXPAND



// 2x2 interlaced 16-bit blitters

#define VERTICAL_EXPAND		1
#define VERTICAL_EXPAND_EX	1
#define HORIZONTAL_EXPAND	2

#define DIRTY_OPTION		NO_DIRTY

#define SWAP_XY				0

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#define SWAP_XY				1

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#undef DIRTY_OPTION

#define DIRTY_OPTION		PIXEL_DELTA

#define SWAP_XY				0

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#define SWAP_XY				1

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#undef DIRTY_OPTION

#define DIRTY_OPTION		PIXEL_DELTA_FULL

#define SWAP_XY				0

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#define SWAP_XY				1

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#undef DIRTY_OPTION

#undef HORIZONTAL_EXPAND
#undef VERTICAL_EXPAND_EX
#undef VERTICAL_EXPAND



// 3x3 16-bit blitters

#define VERTICAL_EXPAND		3
#define VERTICAL_EXPAND_EX	0
#define HORIZONTAL_EXPAND	3

#define DIRTY_OPTION		NO_DIRTY

#define SWAP_XY				0

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#define SWAP_XY				1

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#undef DIRTY_OPTION

#define DIRTY_OPTION		PIXEL_DELTA

#define SWAP_XY				0

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#define SWAP_XY				1

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#undef DIRTY_OPTION

#define DIRTY_OPTION		PIXEL_DELTA_FULL

#define SWAP_XY				0

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#define SWAP_XY				1

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#undef DIRTY_OPTION

#undef HORIZONTAL_EXPAND
#undef VERTICAL_EXPAND_EX
#undef VERTICAL_EXPAND



// 3x3 interlaced 16-bit blitters

#define VERTICAL_EXPAND		2
#define VERTICAL_EXPAND_EX	1
#define HORIZONTAL_EXPAND	3

#define DIRTY_OPTION		NO_DIRTY

#define SWAP_XY				0

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#define SWAP_XY				1

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#undef DIRTY_OPTION

#define DIRTY_OPTION		PIXEL_DELTA

#define SWAP_XY				0

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#define SWAP_XY				1

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#undef DIRTY_OPTION

#define DIRTY_OPTION		PIXEL_DELTA_FULL

#define SWAP_XY				0

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#define SWAP_XY				1

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#undef DIRTY_OPTION

#undef HORIZONTAL_EXPAND
#undef VERTICAL_EXPAND_EX
#undef VERTICAL_EXPAND

#undef HAS_LOOKUP
#undef DST_BITS_PER_PIXEL
#undef SRC_BITS_PER_PIXEL


//===============================================================================
//	32bpp Blitters
//===============================================================================

#define SRC_BITS_PER_PIXEL	16
#define DST_BITS_PER_PIXEL	32
#define HAS_LOOKUP			1

// 1x1 32-bit blitters

#define VERTICAL_EXPAND		1
#define VERTICAL_EXPAND_EX	0
#define HORIZONTAL_EXPAND	1

#define DIRTY_OPTION		NO_DIRTY

#define SWAP_XY				0

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#define SWAP_XY				1

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#undef DIRTY_OPTION

#define DIRTY_OPTION		SINGLE_BUFFER_DIRTY

#define SWAP_XY				0

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#define SWAP_XY				1

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#undef DIRTY_OPTION

#undef HORIZONTAL_EXPAND
#undef VERTICAL_EXPAND_EX
#undef VERTICAL_EXPAND



// 1x2 32-bit blitters

#define VERTICAL_EXPAND		2
#define VERTICAL_EXPAND_EX	0
#define HORIZONTAL_EXPAND	1

#define DIRTY_OPTION		NO_DIRTY

#define SWAP_XY				0

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#define SWAP_XY				1

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#undef DIRTY_OPTION

#define DIRTY_OPTION		PIXEL_DELTA

#define SWAP_XY				0

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#define SWAP_XY				1

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#undef DIRTY_OPTION

#define DIRTY_OPTION		PIXEL_DELTA_FULL

#define SWAP_XY				0

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#define SWAP_XY				1

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#undef DIRTY_OPTION

#undef HORIZONTAL_EXPAND
#undef VERTICAL_EXPAND_EX
#undef VERTICAL_EXPAND



// 1x2 interlaced 32-bit blitters

#define VERTICAL_EXPAND		1
#define VERTICAL_EXPAND_EX	1
#define HORIZONTAL_EXPAND	1

#define DIRTY_OPTION		NO_DIRTY

#define SWAP_XY				0

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#define SWAP_XY				1

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#undef DIRTY_OPTION

#define DIRTY_OPTION		PIXEL_DELTA

#define SWAP_XY				0

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#define SWAP_XY				1

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#undef DIRTY_OPTION

#define DIRTY_OPTION		PIXEL_DELTA_FULL

#define SWAP_XY				0

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#define SWAP_XY				1

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#undef DIRTY_OPTION

#undef HORIZONTAL_EXPAND
#undef VERTICAL_EXPAND_EX
#undef VERTICAL_EXPAND



// 2x2 32-bit blitters

#define VERTICAL_EXPAND		2
#define VERTICAL_EXPAND_EX	0
#define HORIZONTAL_EXPAND	2

#define DIRTY_OPTION		NO_DIRTY

#define SWAP_XY				0

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#define SWAP_XY				1

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#undef DIRTY_OPTION

#define DIRTY_OPTION		PIXEL_DELTA

#define SWAP_XY				0

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#define SWAP_XY				1

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#undef DIRTY_OPTION

#define DIRTY_OPTION		PIXEL_DELTA_FULL

#define SWAP_XY				0

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#define SWAP_XY				1

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#undef DIRTY_OPTION

#undef HORIZONTAL_EXPAND
#undef VERTICAL_EXPAND_EX
#undef VERTICAL_EXPAND



// 2x2 interlaced 32-bit blitters

#define VERTICAL_EXPAND		1
#define VERTICAL_EXPAND_EX	1
#define HORIZONTAL_EXPAND	2

#define DIRTY_OPTION		NO_DIRTY

#define SWAP_XY				0

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#define SWAP_XY				1

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#undef DIRTY_OPTION

#define DIRTY_OPTION		PIXEL_DELTA

#define SWAP_XY				0

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#define SWAP_XY				1

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#undef DIRTY_OPTION

#define DIRTY_OPTION		PIXEL_DELTA_FULL

#define SWAP_XY				0

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#define SWAP_XY				1

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#undef DIRTY_OPTION

#undef HORIZONTAL_EXPAND
#undef VERTICAL_EXPAND_EX
#undef VERTICAL_EXPAND



// 3x3 32-bit blitters

#define VERTICAL_EXPAND		3
#define VERTICAL_EXPAND_EX	0
#define HORIZONTAL_EXPAND	3

#define DIRTY_OPTION		NO_DIRTY

#define SWAP_XY				0

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#define SWAP_XY				1

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#undef DIRTY_OPTION

#define DIRTY_OPTION		PIXEL_DELTA

#define SWAP_XY				0

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#define SWAP_XY				1

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#undef DIRTY_OPTION

#define DIRTY_OPTION		PIXEL_DELTA_FULL

#define SWAP_XY				0

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#define SWAP_XY				1

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#undef DIRTY_OPTION

#undef HORIZONTAL_EXPAND
#undef VERTICAL_EXPAND_EX
#undef VERTICAL_EXPAND



// 3x3 interlaced 32-bit blitters

#define VERTICAL_EXPAND		2
#define VERTICAL_EXPAND_EX	1
#define HORIZONTAL_EXPAND	3

#define DIRTY_OPTION		NO_DIRTY

#define SWAP_XY				0

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#define SWAP_XY				1

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#undef DIRTY_OPTION

#define DIRTY_OPTION		PIXEL_DELTA

#define SWAP_XY				0

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#define SWAP_XY				1

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#undef DIRTY_OPTION

#define DIRTY_OPTION		PIXEL_DELTA_FULL

#define SWAP_XY				0

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#define SWAP_XY				1

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#undef DIRTY_OPTION

#undef HORIZONTAL_EXPAND
#undef VERTICAL_EXPAND_EX
#undef VERTICAL_EXPAND

#undef HAS_LOOKUP
#undef DST_BITS_PER_PIXEL
#undef SRC_BITS_PER_PIXEL


//===============================================================================
//	32bpp Direct Blitters
//===============================================================================

#define SRC_BITS_PER_PIXEL	32
#define DST_BITS_PER_PIXEL	32
#define HAS_LOOKUP			0

// 1x1 32-bit blitters

#define VERTICAL_EXPAND		1
#define VERTICAL_EXPAND_EX	0
#define HORIZONTAL_EXPAND	1

#define DIRTY_OPTION		NO_DIRTY

#define SWAP_XY				0

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#define SWAP_XY				1

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#undef DIRTY_OPTION

#define DIRTY_OPTION		SINGLE_BUFFER_DIRTY

#define SWAP_XY				0

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#define SWAP_XY				1

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#undef DIRTY_OPTION

#undef HORIZONTAL_EXPAND
#undef VERTICAL_EXPAND_EX
#undef VERTICAL_EXPAND



// 1x2 32-bit blitters

#define VERTICAL_EXPAND		2
#define VERTICAL_EXPAND_EX	0
#define HORIZONTAL_EXPAND	1

#define DIRTY_OPTION		NO_DIRTY

#define SWAP_XY				0

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#define SWAP_XY				1

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#undef DIRTY_OPTION

#define DIRTY_OPTION		PIXEL_DELTA

#define SWAP_XY				0

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#define SWAP_XY				1

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#undef DIRTY_OPTION

#define DIRTY_OPTION		PIXEL_DELTA_FULL

#define SWAP_XY				0

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#define SWAP_XY				1

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#undef DIRTY_OPTION

#undef HORIZONTAL_EXPAND
#undef VERTICAL_EXPAND_EX
#undef VERTICAL_EXPAND



// 1x2 interlaced 32-bit blitters

#define VERTICAL_EXPAND		1
#define VERTICAL_EXPAND_EX	1
#define HORIZONTAL_EXPAND	1

#define DIRTY_OPTION		NO_DIRTY

#define SWAP_XY				0

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#define SWAP_XY				1

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#undef DIRTY_OPTION

#define DIRTY_OPTION		PIXEL_DELTA

#define SWAP_XY				0

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#define SWAP_XY				1

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#undef DIRTY_OPTION

#define DIRTY_OPTION		PIXEL_DELTA_FULL

#define SWAP_XY				0

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#define SWAP_XY				1

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#undef DIRTY_OPTION

#undef HORIZONTAL_EXPAND
#undef VERTICAL_EXPAND_EX
#undef VERTICAL_EXPAND



// 2x2 32-bit blitters

#define VERTICAL_EXPAND		2
#define VERTICAL_EXPAND_EX	0
#define HORIZONTAL_EXPAND	2

#define DIRTY_OPTION		NO_DIRTY

#define SWAP_XY				0

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#define SWAP_XY				1

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#undef DIRTY_OPTION

#define DIRTY_OPTION		PIXEL_DELTA

#define SWAP_XY				0

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#define SWAP_XY				1

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#undef DIRTY_OPTION

#define DIRTY_OPTION		PIXEL_DELTA_FULL

#define SWAP_XY				0

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#define SWAP_XY				1

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#undef DIRTY_OPTION

#undef HORIZONTAL_EXPAND
#undef VERTICAL_EXPAND_EX
#undef VERTICAL_EXPAND



// 2x2 interlaced 32-bit blitters

#define VERTICAL_EXPAND		1
#define VERTICAL_EXPAND_EX	1
#define HORIZONTAL_EXPAND	2

#define DIRTY_OPTION		NO_DIRTY

#define SWAP_XY				0

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#define SWAP_XY				1

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#undef DIRTY_OPTION

#define DIRTY_OPTION		PIXEL_DELTA

#define SWAP_XY				0

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#define SWAP_XY				1

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#undef DIRTY_OPTION

#define DIRTY_OPTION		PIXEL_DELTA_FULL

#define SWAP_XY				0

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#define SWAP_XY				1

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#undef DIRTY_OPTION

#undef HORIZONTAL_EXPAND
#undef VERTICAL_EXPAND_EX
#undef VERTICAL_EXPAND



// 3x3 32-bit blitters

#define VERTICAL_EXPAND		3
#define VERTICAL_EXPAND_EX	0
#define HORIZONTAL_EXPAND	3

#define DIRTY_OPTION		NO_DIRTY

#define SWAP_XY				0

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#define SWAP_XY				1

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#undef DIRTY_OPTION

#define DIRTY_OPTION		PIXEL_DELTA

#define SWAP_XY				0

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#define SWAP_XY				1

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#undef DIRTY_OPTION

#define DIRTY_OPTION		PIXEL_DELTA_FULL

#define SWAP_XY				0

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#define SWAP_XY				1

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#undef DIRTY_OPTION

#undef HORIZONTAL_EXPAND
#undef VERTICAL_EXPAND_EX
#undef VERTICAL_EXPAND



// 3x3 interlaced 32-bit blitters

#define VERTICAL_EXPAND		2
#define VERTICAL_EXPAND_EX	1
#define HORIZONTAL_EXPAND	3

#define DIRTY_OPTION		NO_DIRTY

#define SWAP_XY				0

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#define SWAP_XY				1

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#undef DIRTY_OPTION

#define DIRTY_OPTION		PIXEL_DELTA

#define SWAP_XY				0

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#define SWAP_XY				1

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#undef DIRTY_OPTION

#define DIRTY_OPTION		PIXEL_DELTA_FULL

#define SWAP_XY				0

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#define SWAP_XY				1

#define FLIP_X				0

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#define FLIP_X				1

#define FLIP_Y				0

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#define FLIP_Y				1

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#undef DIRTY_OPTION

#undef HORIZONTAL_EXPAND
#undef VERTICAL_EXPAND_EX
#undef VERTICAL_EXPAND

#undef HAS_LOOKUP
#undef DST_BITS_PER_PIXEL
#undef SRC_BITS_PER_PIXEL


//===============================================================================
//	32bpp Blitters
//===============================================================================

#define SRC_BITS_PER_PIXEL	32
#define DST_BITS_PER_PIXEL	32
#define HAS_LOOKUP			1

#define SWAP_XY				0

#define FLIP_X 0

#define FLIP_Y 0

// 1x1 32-bit blitters

#define VERTICAL_EXPAND		1
#define VERTICAL_EXPAND_EX	0
#define HORIZONTAL_EXPAND	1

#define DIRTY_OPTION		NO_DIRTY

#define USE_ALTIVEC			0

#include __FILE__

#undef USE_ALTIVEC

/*#define USE_ALTIVEC			1

#include __FILE__

#undef USE_ALTIVEC*/

#undef DIRTY_OPTION

#undef HORIZONTAL_EXPAND
#undef VERTICAL_EXPAND_EX
#undef VERTICAL_EXPAND

#undef FLIP_Y

#undef FLIP_X

#undef SWAP_XY

#undef HAS_LOOKUP
#undef DST_BITS_PER_PIXEL
#undef SRC_BITS_PER_PIXEL


#else


//===============================================================================
//	Core blitter loop
//===============================================================================

#if SWAP_XY

	#if USE_ALTIVEC
		#if HAS_LOOKUP
			#define EXPAND_PIXEL	EXPAND_PIXEL_L_SWAPXY_VEC_NAME(HORIZONTAL_EXPAND, SRC_BITS_PER_PIXEL, DST_BITS_PER_PIXEL)
			#define EXPAND_PIXELS	EXPAND_PIXELS_L_SWAPXY_VEC_NAME(HORIZONTAL_EXPAND, SRC_BITS_PER_PIXEL, DST_BITS_PER_PIXEL)
		#else
			#define EXPAND_PIXEL	EXPAND_PIXEL_D_SWAPXY_VEC_NAME(HORIZONTAL_EXPAND, SRC_BITS_PER_PIXEL, DST_BITS_PER_PIXEL)
			#define EXPAND_PIXELS	EXPAND_PIXELS_D_SWAPXY_VEC_NAME(HORIZONTAL_EXPAND, SRC_BITS_PER_PIXEL, DST_BITS_PER_PIXEL)
		#endif
	#else
		#if HAS_LOOKUP
			#define EXPAND_PIXEL	EXPAND_PIXEL_L_SWAPXY_NAME(HORIZONTAL_EXPAND, SRC_BITS_PER_PIXEL, DST_BITS_PER_PIXEL)
			#define EXPAND_PIXELS	EXPAND_PIXELS_L_SWAPXY_NAME(HORIZONTAL_EXPAND, SRC_BITS_PER_PIXEL, DST_BITS_PER_PIXEL)
		#else
			#define EXPAND_PIXEL	EXPAND_PIXEL_D_SWAPXY_NAME(HORIZONTAL_EXPAND, SRC_BITS_PER_PIXEL, DST_BITS_PER_PIXEL)
			#define EXPAND_PIXELS	EXPAND_PIXELS_D_SWAPXY_NAME(HORIZONTAL_EXPAND, SRC_BITS_PER_PIXEL, DST_BITS_PER_PIXEL)
		#endif
	#endif


#else

	#if USE_ALTIVEC
		#if HAS_LOOKUP
			#define EXPAND_PIXEL	EXPAND_PIXEL_L_VEC_NAME(HORIZONTAL_EXPAND, SRC_BITS_PER_PIXEL, DST_BITS_PER_PIXEL)
			#define EXPAND_PIXELS	EXPAND_PIXELS_L_VEC_NAME(HORIZONTAL_EXPAND, SRC_BITS_PER_PIXEL, DST_BITS_PER_PIXEL)
		#else
			#define EXPAND_PIXEL	EXPAND_PIXEL_D_VEC_NAME(HORIZONTAL_EXPAND, SRC_BITS_PER_PIXEL, DST_BITS_PER_PIXEL)
			#define EXPAND_PIXELS	EXPAND_PIXELS_D_VEC_NAME(HORIZONTAL_EXPAND, SRC_BITS_PER_PIXEL, DST_BITS_PER_PIXEL)
		#endif
	#else
		#if HAS_LOOKUP
			#define EXPAND_PIXEL	EXPAND_PIXEL_L_NAME(HORIZONTAL_EXPAND, SRC_BITS_PER_PIXEL, DST_BITS_PER_PIXEL)
			#define EXPAND_PIXELS	EXPAND_PIXELS_L_NAME(HORIZONTAL_EXPAND, SRC_BITS_PER_PIXEL, DST_BITS_PER_PIXEL)
		#else
			#define EXPAND_PIXEL	EXPAND_PIXEL_D_NAME(HORIZONTAL_EXPAND, SRC_BITS_PER_PIXEL, DST_BITS_PER_PIXEL)
			#define EXPAND_PIXELS	EXPAND_PIXELS_D_NAME(HORIZONTAL_EXPAND, SRC_BITS_PER_PIXEL, DST_BITS_PER_PIXEL)
		#endif
	#endif

#endif

// based on the DIRTY_OPTION value, redefine macros to handle delta modes
#if (DIRTY_OPTION == PIXEL_DELTA)
	#define LOAD_FROM_LOOKASIDE(op,reg,offs)	op		reg,offs(rDirty)
	#define STORE_TO_LOOKASIDE(op,reg,offs)		op		reg,offs(rDirty)
	#define ADD_TO_LOOKASIDE(count)				addi	rDirty,rDirty,count
	#define COMPARE_TO_LOOKASIDE(cr,reg1,reg2)	cmp		cr,reg1,reg2
	#define SKIP_IF_NOT_DIRTY(cr,label)			beq		cr,label
	#define COMBINE_COMPARES(c3,c1,c2)			crand	c3,c1,c2
#elif (DIRTY_OPTION == PIXEL_DELTA_FULL)
	#define LOAD_FROM_LOOKASIDE(op,reg,offs)
	#define STORE_TO_LOOKASIDE(op,reg,offs)		op		reg,offs(rDirty)
	#define ADD_TO_LOOKASIDE(count)				addi	rDirty,rDirty,count
	#define COMPARE_TO_LOOKASIDE(cr,reg1,reg2)
	#define SKIP_IF_NOT_DIRTY(cr,num)
	#define COMBINE_COMPARES(c3,c1,c2)
#else
	#define LOAD_FROM_LOOKASIDE(op,reg,offs)
	#define STORE_TO_LOOKASIDE(op,reg,offs)
	#define ADD_TO_LOOKASIDE(count)
	#define COMPARE_TO_LOOKASIDE(cr,reg1,reg2)
	#define SKIP_IF_NOT_DIRTY(cr,num)
	#define COMBINE_COMPARES(c3,c1,c2)
#endif

#if !USE_ALTIVEC

	// base on the VERTICAL_EXPAND value, redefine macros to store 1, 2 or 3 scanlines at a time
	#if (VERTICAL_EXPAND == 1)
		#define ADD_TO_DEST1(offs)			addi	rDest1,rDest1,offs
		#define ADD_TO_DEST2(offs)
		#define ADD_TO_DEST3(offs)
		#define STORE_TO_DEST1(op,reg,offs)	op		reg,offs(rDest1)
		#define STORE_TO_DEST2(op,reg,offs)
		#define STORE_TO_DEST3(op,reg,offs)
	#elif (VERTICAL_EXPAND == 2)
		#define ADD_TO_DEST1(offs)			addi	rDest1,rDest1,offs
		#define ADD_TO_DEST2(offs)			addi	rDest2,rDest2,offs
		#define ADD_TO_DEST3(offs)
		#define STORE_TO_DEST1(op,reg,offs)	op		reg,offs(rDest1)
		#define STORE_TO_DEST2(op,reg,offs)	op		reg,offs(rDest2)
		#define STORE_TO_DEST3(op,reg,offs)
	#elif (VERTICAL_EXPAND == 3)
		#define ADD_TO_DEST1(offs)			addi	rDest1,rDest1,offs
		#define ADD_TO_DEST2(offs)			addi	rDest2,rDest2,offs
		#define ADD_TO_DEST3(offs)			addi	rDest3,rDest3,offs
		#define STORE_TO_DEST1(op,reg,offs)	op		reg,offs(rDest1)
		#define STORE_TO_DEST2(op,reg,offs)	op		reg,offs(rDest2)
		#define STORE_TO_DEST3(op,reg,offs)	op		reg,offs(rDest3)
	#else
		#error Invalid VERTICAL_EXPAND value!
	#endif

#else

	// base on the VERTICAL_EXPAND value, redefine macros to store 1, 2 or 3 scanlines at a time
	#define ADD_TO_DEST(offs)						addi	rDest1,rDest1,offs
	#if (VERTICAL_EXPAND == 1)
		#define STORE_TO_DEST1_VEC(reg)				stvx	reg,0,rDest1
		#define STORE_TO_DEST2_VEC(reg)
		#define STORE_TO_DEST3_VEC(reg)
		#define STORE_TO_DEST1(op,reg,offs)			op		reg,offs(rDest1)
		#define STORE_TO_DEST2(op,reg)
		#define STORE_TO_DEST3(op,reg)
	#elif (VERTICAL_EXPAND == 2)
		#define STORE_TO_DEST1_VEC(reg)				stvx	reg,0,rDest1
		#define STORE_TO_DEST2_VEC(reg)				stvx	reg,inDestRowBytes,rDest1
		#define STORE_TO_DEST3_VEC(reg)
		#define STORE_TO_DEST1(op,reg,offs)			op		reg,offs(rDest1)
		#define STORE_TO_DEST2(op,reg)				op		reg,inDestRowBytes,rDest1
		#define STORE_TO_DEST3(op,reg)
	#elif (VERTICAL_EXPAND == 3)
		#define STORE_TO_DEST1_VEC(reg)				stvx	reg,0,rDest1
		#define STORE_TO_DEST2_VEC(reg)				stvx	reg,inDestRowBytes,rDest1
		#define STORE_TO_DEST3_VEC(reg)				stvx	reg,doubleDestRowBytes,rDest1
		#define STORE_TO_DEST1(op,reg,offs)			op		reg,offs(rDest1)
		#define STORE_TO_DEST2(op,reg)				op		reg,inDestRowBytes,rDest1
		#define STORE_TO_DEST3(op,reg)				op		reg,doubleDestRowBytes,rDest1
	#else
		#error Invalid VERTICAL_EXPAND value!
	#endif


	// based on the DIRTY_OPTION value, redefine macros to handle delta modes
	#if (DIRTY_OPTION == PIXEL_DELTA)
		#define LOAD_FROM_SOURCE_FOR_DELTA_VEC(reg)		lvx		reg,0,rSource
		#define LOAD_FROM_LOOKASIDE_VEC(reg)		lvx		reg,0,rDirty
		#define STORE_TO_LOOKASIDE_VEC(reg)			stvx	reg,0,rDirty
		#define COMPARE_TO_LOOKASIDE_VEC(reg3,reg1,reg2)	vcmpequh.	reg3,reg1,reg2
		#define SKIP_IF_NOT_DIRTY_VEC(label)		bt	cr6*4+0,label
	#elif (DIRTY_OPTION == PIXEL_DELTA_FULL)
		#define LOAD_FROM_SOURCE_FOR_DELTA_VEC(reg)		lvx		reg,0,rSource
		#define LOAD_FROM_LOOKASIDE_VEC(reg)
		#define STORE_TO_LOOKASIDE_VEC(reg)			stvx	reg,0,rDirty
		#define COMPARE_TO_LOOKASIDE_VEC(reg3,reg1,reg2)
		#define SKIP_IF_NOT_DIRTY_VEC(label)
	#else
		#define LOAD_FROM_SOURCE_FOR_DELTA_VEC(reg)
		#define LOAD_FROM_LOOKASIDE_VEC(reg)
		#define STORE_TO_LOOKASIDE_VEC(reg)
		#define COMPARE_TO_LOOKASIDE_VEC(reg3,reg1,reg2)
		#define SKIP_IF_NOT_DIRTY_VEC(label)
	#endif


	// value for VRSAVE
	#if HAS_LOOKUP
		#if (SRC_BITS_PER_PIXEL == 16) && (DST_BITS_PER_PIXEL == 16)
			#if HORIZONTAL_EXPAND == 1
				#define VRSAVE_VALUE VRSAVE_VALUE_1x_16TO16L
			#elif HORIZONTAL_EXPAND == 2
				#define VRSAVE_VALUE VRSAVE_VALUE_2x_16TO16L
			#elif HORIZONTAL_EXPAND == 3
				#define VRSAVE_VALUE VRSAVE_VALUE_3x_16TO16L
			#endif
		#else
			#if HORIZONTAL_EXPAND == 1
				#define VRSAVE_VALUE VRSAVE_VALUE_1x_16TO32L
			#elif HORIZONTAL_EXPAND == 2
				#define VRSAVE_VALUE VRSAVE_VALUE_2x_16TO32L
			#elif HORIZONTAL_EXPAND == 3
				#define VRSAVE_VALUE VRSAVE_VALUE_3x_16TO32L
			#endif
		#endif
	#else
		#if (! SWAP_XY) && FLIP_X
			#if HORIZONTAL_EXPAND == 1
				#define VRSAVE_VALUE VRSAVE_VALUE_1x_FLIPX
			#elif HORIZONTAL_EXPAND == 2
				#define VRSAVE_VALUE VRSAVE_VALUE_2x_FLIPX
			#elif HORIZONTAL_EXPAND == 3
				#define VRSAVE_VALUE VRSAVE_VALUE_3x_FLIPX
			#endif
		#else
			#if HORIZONTAL_EXPAND == 1
				#define VRSAVE_VALUE VRSAVE_VALUE_1x
			#elif HORIZONTAL_EXPAND == 2
				#define VRSAVE_VALUE VRSAVE_VALUE_2x
			#elif HORIZONTAL_EXPAND == 3
				#define VRSAVE_VALUE VRSAVE_VALUE_3x
			#endif
		#endif
	#endif


	// expand table to load
	#if HORIZONTAL_EXPAND == 3
		#if (! HAS_LOOKUP) && (! SWAP_XY) && FLIP_X
			#if DST_BITS_PER_PIXEL == 16
				#define ZOOM_TABLE pixel_zoom_table_3x_16bit_flip
			#elif DST_BITS_PER_PIXEL == 32
				#define ZOOM_TABLE pixel_zoom_table_3x_32bit_flip
			#endif
		#else
			#if DST_BITS_PER_PIXEL == 16
				#define ZOOM_TABLE pixel_zoom_table_3x_16bit
			#elif DST_BITS_PER_PIXEL == 32
					#define ZOOM_TABLE pixel_zoom_table_3x_32bit
			#endif
		#endif
	#elif HORIZONTAL_EXPAND == 2
		#if (! HAS_LOOKUP) && (! SWAP_XY) && FLIP_X
			#if DST_BITS_PER_PIXEL == 16
				#define ZOOM_TABLE pixel_zoom_table_2x_16bit_flip
			#elif DST_BITS_PER_PIXEL == 32
				#define ZOOM_TABLE pixel_zoom_table_2x_32bit_flip
			#endif
		#elif HAS_LOOKUP
			#if DST_BITS_PER_PIXEL == 16
				#define ZOOM_TABLE pixel_zoom_table_2x_16bit
			#elif DST_BITS_PER_PIXEL == 32
				#define ZOOM_TABLE pixel_zoom_table_2x_32bit
			#endif
		#endif
	#endif

	#if HAS_LOOKUP
		#if (! SWAP_XY) && FLIP_X
			#if SRC_BITS_PER_PIXEL == 16
				#if DST_BITS_PER_PIXEL == 16
					#define OFFSET_PERM offsetPerm_16to16_fx
				#elif DST_BITS_PER_PIXEL == 32
					#define OFFSET_PERM offsetPerm_16to32_fx
				#endif
			#elif SRC_BITS_PER_PIXEL == 32
				#define OFFSET_PERM offsetPerm_32to32_fx
			#endif
		#else
			#if SRC_BITS_PER_PIXEL == 16
				#if DST_BITS_PER_PIXEL == 16
					#define OFFSET_PERM offsetPerm_16to16
				#elif DST_BITS_PER_PIXEL == 32
					#define OFFSET_PERM offsetPerm_16to32
				#endif
			#elif SRC_BITS_PER_PIXEL == 32
				#define OFFSET_PERM offsetPerm_32to32
			#endif
		#endif

		#if DST_BITS_PER_PIXEL == 16
			#define OFFSET_OFFSET offsetOffset_dst16
		#elif DST_BITS_PER_PIXEL == 32
			#define OFFSET_OFFSET offsetOffset_dst32
		#endif
	#endif

	#if (! HAS_LOOKUP) && (! SWAP_XY) && FLIP_X
		#if DST_BITS_PER_PIXEL == 16
			#define VECTOR_FLIP vectorFlip_16
		#elif DST_BITS_PER_PIXEL == 32
			#define VECTOR_FLIP vectorFlip_32
		#endif
	#endif

#endif

#if SWAP_XY ? FLIP_Y : FLIP_X

	#define X_INCREMENT(value)						(-value)
	#if USE_ALTIVEC
		#define X_OFFSET(item_offset,item_width)	(16-item_width-item_offset)
	#else
		#define X_OFFSET(item_offset,item_width)	(-item_width-item_offset)
	#endif
	#define FLIP_PIXELS_16(reg)						rlwinm	reg,reg,16,0,31
	#define MOVE_FLIP_PIXELS_16(dreg,sreg)			rlwinm	dreg,sreg,16,0,31
	#define EXTRACT_PIXEL0_16(dreg,sreg,shift)		rlwinm	dreg,sreg,shift,16-shift,31-shift
	#define EXTRACT_PIXEL1_16(dreg,sreg,shift)		rlwinm	dreg,sreg,16+shift,16-shift,31-shift
	#define DUPLICATE_PIXEL0_16(dreg,sreg)			rlwimi	dreg,sreg,16,0,15
	#define DUPLICATE_PIXEL1_16(dreg,sreg)			rlwimi	dreg,sreg,16,16,31

#else

	#define X_INCREMENT(value)						value
	#define X_OFFSET(item_offset,item_width)		(item_offset)
	#define FLIP_PIXELS_16(reg)
	#define MOVE_FLIP_PIXELS_16(dreg,sreg)			mr		dreg,sreg
	#define EXTRACT_PIXEL0_16(dreg,sreg,shift)		rlwinm	dreg,sreg,16+shift,16-shift,31-shift
	#define EXTRACT_PIXEL1_16(dreg,sreg,shift)		rlwinm	dreg,sreg,shift,16-shift,31-shift
	#define DUPLICATE_PIXEL0_16(dreg,sreg)			rlwimi	dreg,sreg,16,16,31
	#define DUPLICATE_PIXEL1_16(dreg,sreg)			rlwimi	dreg,sreg,16,0,15

#endif

#if USE_ALTIVEC && (! HAS_LOOKUP) && (HORIZONTAL_EXPAND == 1)
	#if (! SWAP_XY) && FLIP_X
		#define FLIP_PIXELS_VEC(reg)				vperm	reg,reg,reg,vrVectorFlip
	#else
		#define FLIP_PIXELS_VEC(reg)
	#endif
#endif

#if USE_ALTIVEC && (! HAS_LOOKUP) && (HORIZONTAL_EXPAND == 2)
	#if (! SWAP_XY) && FLIP_X
		#define ZOOM_FLIP_PIXELS_2x_HIGH_VEC(dreg,sreg)		vperm	dreg,sreg,sreg,vrPixelZoom0
		#define ZOOM_FLIP_PIXELS_2x_LOW_VEC(dreg,sreg)		vperm	dreg,sreg,sreg,vrPixelZoom1
	#else
		#if DST_BITS_PER_PIXEL == 16
			#define ZOOM_FLIP_PIXELS_2x_HIGH_VEC(dreg,sreg)	vmrghh	dreg,sreg,sreg
			#define ZOOM_FLIP_PIXELS_2x_LOW_VEC(dreg,sreg)	vmrglh	dreg,sreg,sreg
		#elif DST_BITS_PER_PIXEL == 32
			#define ZOOM_FLIP_PIXELS_2x_HIGH_VEC(dreg,sreg)	vmrghw	dreg,sreg,sreg
			#define ZOOM_FLIP_PIXELS_2x_LOW_VEC(dreg,sreg)	vmrglw	dreg,sreg,sreg
		#endif
	#endif
#endif


#if SWAP_XY
	#if FLIP_X
		#define INCREMENT_SOURCE	sub	rSource,rSource,inSourceRowBytes
	#else
		#define INCREMENT_SOURCE	add	rSource,rSource,inSourceRowBytes
	#endif
#endif

#pragma mark Blitter function body

// Now generate function name
#define concat_13(a,b,c,d,e,f,g,h,i,j,k,l,m) a##b##c##d##e##f##g##h##i##j##k##l##m
#define BLIT_FUNC_NAME(horizontal_expand,vertical_expand,dirty_option,src_bits_per_pixel,dst_bits_per_pixel,has_lookup,swap_xy,flip_x,flip_y,use_altivec)	concat_13(blit_,horizontal_expand,x,vertical_expand,dirty_option,src_bits_per_pixel,to,dst_bits_per_pixel,has_lookup,swap_xy,flip_x,flip_y,use_altivec)

#if VERTICAL_EXPAND_EX
	#if VERTICAL_EXPAND == 1
		#define VERTICAL_EXPAND_INFIX 2i
	#elif VERTICAL_EXPAND == 2
		#define VERTICAL_EXPAND_INFIX 3i
	#else
		#error Invalid VERTICAL_EXPAND value!
	#endif
#else
	#define VERTICAL_EXPAND_INFIX VERTICAL_EXPAND
#endif

#if DIRTY_OPTION == NO_DIRTY
	#define DIRTY_OPTION_INFIX _full_
#elif DIRTY_OPTION == SINGLE_BUFFER_DIRTY
	#define DIRTY_OPTION_INFIX _dirty_vector_
#elif DIRTY_OPTION == PIXEL_DELTA
	#define DIRTY_OPTION_INFIX _delta_
#elif DIRTY_OPTION == PIXEL_DELTA_FULL
	#define DIRTY_OPTION_INFIX _delta_full_
#else
	#error Invalid DIRTY_OPTION value!
#endif

#if HAS_LOOKUP
	#define HAS_LOOKUP_SUFFIX l
#else
	#define HAS_LOOKUP_SUFFIX d
#endif

#if SWAP_XY
	#define SWAP_XY_SUFFIX _sxy
#else
	#define SWAP_XY_SUFFIX
#endif

#if FLIP_X
	#define FLIP_X_SUFFIX _fx
#else
	#define FLIP_X_SUFFIX
#endif

#if FLIP_Y
	#define FLIP_Y_SUFFIX _fy
#else
	#define FLIP_Y_SUFFIX
#endif

#if USE_ALTIVEC
	#define USE_ALTIVEC_SUFFIX _vec
#else
	#define USE_ALTIVEC_SUFFIX
#endif

// black magic
#define concat2(a, b) a##b
#define global(symbol) concat2(_, symbol)

#define concat3(a, b, c) a##b##c
#define concat3b(a, b, c) concat3(a, b, c)
#define local(symbol) concat3b(BLIT_FUNC_NAME(HORIZONTAL_EXPAND,VERTICAL_EXPAND_INFIX,DIRTY_OPTION_INFIX,SRC_BITS_PER_PIXEL,DST_BITS_PER_PIXEL,HAS_LOOKUP_SUFFIX,SWAP_XY_SUFFIX,FLIP_X_SUFFIX,FLIP_Y_SUFFIX,USE_ALTIVEC_SUFFIX), _, symbol)


#define yLoop local(yLoop)
#define DontClip local(DontClip)
#define SkipLeftEdgeCase local(SkipLeftEdgeCase)
#define LeftEdgeLoop local(LeftEdgeLoop)
#define DontDrawSingleLeft local(DontDrawSingleLeft)
#define SkipXLoop local(SkipXLoop)
#define xLoop local(xLoop)
#define DontClear local(DontClear)
#define DontDrawMulti local(DontDrawMulti)
#define DontDrawMultiDirty local(DontDrawMultiDirty)
#define SkipEdgeCase local(SkipEdgeCase)
#define DontClear2 local(DontClear2)
#define EdgeLoop local(EdgeLoop)
#define DontDrawSingle local(DontDrawSingle)
#define DontResetDirty local(DontResetDirty)
#define DontDraw1 local(DontDraw1)
#define DontDraw2 local(DontDraw2)
#define DontDraw3 local(DontDraw3)
#define skip1 local(skip1)
#define skip2 local(skip2)
#define skip3 local(skip3)
#define skip4 local(skip4)
#define Skip1 local(Skip1)

.globl global(BLIT_FUNC_NAME(HORIZONTAL_EXPAND,VERTICAL_EXPAND_INFIX,DIRTY_OPTION_INFIX,SRC_BITS_PER_PIXEL,DST_BITS_PER_PIXEL,HAS_LOOKUP_SUFFIX,SWAP_XY_SUFFIX,FLIP_X_SUFFIX,FLIP_Y_SUFFIX,USE_ALTIVEC_SUFFIX))

global(BLIT_FUNC_NAME(HORIZONTAL_EXPAND,VERTICAL_EXPAND_INFIX,DIRTY_OPTION_INFIX,SRC_BITS_PER_PIXEL,DST_BITS_PER_PIXEL,HAS_LOOKUP_SUFFIX,SWAP_XY_SUFFIX,FLIP_X_SUFFIX,FLIP_Y_SUFFIX,USE_ALTIVEC_SUFFIX)):
	//=================================================================================
	// save registers

	stmw	rLastNonVolatile,-100(sp)

	//=================================================================================
	// setup

	// fix alignment
	#if SWAP_XY
		#if USE_ALTIVEC
			// not implemented yet
		#endif
	#else
		#if USE_ALTIVEC
			// compute and save offset from inSource to first 16-byte boundary, in pixels
			#if FLIP_X
				#if (SRC_BITS_PER_PIXEL == 16)
					rlwinm	rLeftEdge,inSource,31,29,31
				#elif (SRC_BITS_PER_PIXEL == 32)
					rlwinm	rLeftEdge,inSource,30,30,31
				#endif
			#else
				neg		r0,inSource
				#if (SRC_BITS_PER_PIXEL == 16)
					rlwinm	rLeftEdge,r0,31,29,31
				#elif (SRC_BITS_PER_PIXEL == 32)
					rlwinm	rLeftEdge,r0,30,30,31
				#endif
			#endif
			// apply zoom to get desired dest offset in pixels
			#if HORIZONTAL_EXPAND == 1
				mr		r0,rLeftEdge
			#endif
			#if HORIZONTAL_EXPAND > 1
				add		r0,rLeftEdge,rLeftEdge
			#endif
			#if HORIZONTAL_EXPAND > 2
				add		r0,r0,rLeftEdge
			#endif
			// convert from pixels to byte offset, modulo 16
			#if (DST_BITS_PER_PIXEL == 16)
				rlwinm	r0,r0,1,28,31
			#elif (DST_BITS_PER_PIXEL == 32)
				rlwinm	r0,r0,2,28,31
			#endif
			// ensure that inDest is 16-byte aligned, minding the desired offset
			add		inDest,inDest,r0
			rlwinm	inDest,inDest,0,0,27
			sub		inDest,inDest,r0

			// fix rLeftEdge if larger than inWidth
			cmpw	cr0,inWidth,rLeftEdge
			bge		DontClip
			mr		rLeftEdge,inWidth
		DontClip:
		#else
			// ensure that inDest is 8-byte aligned
			rlwinm	inDest,inDest,0,0,28
		#endif
	#endif

	#if USE_ALTIVEC
		// save vrsave and load it with our value
		lis		r0,VRSAVE_VALUE>>16
		mfspr	vrsave_save,vrsave
		mtspr	vrsave,r0

		// set pointer to vector buffer on stack
		//addi	rPTmpVecSave,sp,-16

		// load expand table
		#if (HORIZONTAL_EXPAND == 3) || ((HORIZONTAL_EXPAND == 2) && (HAS_LOOKUP || ((! SWAP_XY) && FLIP_X)))
			#if TARGET_RT_MAC_MACHO
				// The Mach-O ABI is incompatible with other PPC ABIs.
				lis		rPixel1,hi16(ZOOM_TABLE)
				ori		rPixel1,rPixel1,lo16(ZOOM_TABLE) 
			#else
				// This must be the traditional PPC ABI.
				lwz		rPixel1,ZOOM_TABLE(rtoc)
			#endif
			lvx		vrPixelZoom0,0,rPixel1
			addi	rPixel1,rPixel1,16
			lvx		vrPixelZoom1,0,rPixel1
			#if (HORIZONTAL_EXPAND == 3)
				addi	rPixel1,rPixel1,16
				lvx		vrPixelZoom2,0,rPixel1
			#endif
		#endif

		#if (VERTICAL_EXPAND > 2)
			// set up offset 2*inDestRowBytes
			add		doubleDestRowBytes,inDestRowBytes,inDestRowBytes
		#endif

		#if HAS_LOOKUP
			#if (DST_BITS_PER_PIXEL == 16)
				vspltish	vrOffsetMask,3
				vspltish	vrOffsetShift,2
			#elif (DST_BITS_PER_PIXEL == 32)
				vspltish	vrOffsetMask,1
				vspltish	vrOffsetShift,3
			#endif
			#if TARGET_RT_MAC_MACHO
				// The Mach-O ABI is incompatible with other PPC ABIs.
				lis		rPixel1,hi16(OFFSET_PERM)
				ori		rPixel1,rPixel1,lo16(OFFSET_PERM) 
			#else
				// This must be the traditional PPC ABI.
				lwz		rPixel1,OFFSET_PERM(rtoc)
			#endif
			lvx		vrOffsetPerm,0,rPixel1
			#if (SRC_BITS_PER_PIXEL == 16) && (DST_BITS_PER_PIXEL == 32)
				addi	rPixel1,rPixel1,16
				lvx		vrOffsetPerm2,0,rPixel1
			#endif
			#if TARGET_RT_MAC_MACHO
				// The Mach-O ABI is incompatible with other PPC ABIs.
				lis		rPixel1,hi16(OFFSET_OFFSET)
				ori		rPixel1,rPixel1,lo16(OFFSET_OFFSET) 
			#else
				// This must be the traditional PPC ABI.
				lwz		rPixel1,OFFSET_OFFSET(rtoc)
			#endif
			lvx		vrOffsetOffset,0,rPixel1
			#if (HORIZONTAL_EXPAND == 1)
				#if (DST_BITS_PER_PIXEL == 16)
					#if TARGET_RT_MAC_MACHO
						// The Mach-O ABI is incompatible with other PPC ABIs.
						lis		rPixel1,hi16(pixelMerge1)
						ori		rPixel1,rPixel1,lo16(pixelMerge1) 
					#else
						// This must be the traditional PPC ABI.
						lwz		rPixel1,pixelMerge1(rtoc)
					#endif
					lvx		vrPixelMerge1,0,rPixel1
				#endif
				#if TARGET_RT_MAC_MACHO
					// The Mach-O ABI is incompatible with other PPC ABIs.
					lis		rPixel1,hi16(pixelMerge2)
					ori		rPixel1,rPixel1,lo16(pixelMerge2) 
				#else
					// This must be the traditional PPC ABI.
					lwz		rPixel1,pixelMerge2(rtoc)
				#endif
				lvx		vrPixelMerge2,0,rPixel1
			#endif
		#endif

		#if (! HAS_LOOKUP) && (! SWAP_XY) && FLIP_X
			#if TARGET_RT_MAC_MACHO
				// The Mach-O ABI is incompatible with other PPC ABIs.
				lis		rPixel1,hi16(VECTOR_FLIP)
				ori		rPixel1,rPixel1,lo16(VECTOR_FLIP) 
			#else
				// This must be the traditional PPC ABI.
				lwz		rPixel1,VECTOR_FLIP(rtoc)
			#endif
			lvx		vrVectorFlip,0,rPixel1
		#endif

		#if (! SWAP_XY) && FLIP_X
			addi	inSource,inSource,-16
		#endif
	#endif

	// load the dirty parameters for single/double buffer dirty options
	#if (DIRTY_OPTION == SINGLE_BUFFER_DIRTY)
		lwz		r0,DisplayParameters.dirtyrowshift(inParams)
		li		rDirtyMask,1
		lwz		rDirtyMap,DisplayParameters.curdirty(inParams)
		slw		rDirtyRowBytes,rDirtyMask,r0
	#endif
	
	// load the delta pointers for delta options
	#if (DIRTY_OPTION == PIXEL_DELTA || DIRTY_OPTION == PIXEL_DELTA_FULL)
		lwz		rDirtyMap,DisplayParameters.deltabits(inParams)
		#if (! SWAP_XY)
			rlwinm	r0,inSource,0,28,31
			add		rDirtyMap,rDirtyMap,r0
		#endif
		#if (SWAP_XY)
			#if (SRC_BITS_PER_PIXEL == 16)
				rlwinm	rDirtyRowBytes,inWidth,1,0,30
			#elif (SRC_BITS_PER_PIXEL == 32)
				rlwinm	rDirtyRowBytes,inWidth,2,0,29
			#else
				#error Invalid SRC_BITS_PER_PIXEL value!
			#endif
			// align on 16-byte boundary
			addi	rDirtyRowBytes,rDirtyRowBytes,15
			rlwinm	rDirtyRowBytes,rDirtyRowBytes,0,0,27
		#endif
	#endif

	// load the lookup table base for palettized modes
	#if HAS_LOOKUP
	#if /*1*/(DST_BITS_PER_PIXEL == 16)
		lwz		rLookup,DisplayParameters.lookup16(inParams)
	#elif (DST_BITS_PER_PIXEL == 32)
		lwz		rLookup,DisplayParameters.lookup32(inParams)
	#endif
	#endif

	//=================================================================================
	// Y loop top
yLoop:

	// copy the base registers to their temporary conterparts
	mr		rSource,inSource
	mr		rDest1,inDest

	// advance the source and destination values by the appropriate rowbytes
	#if SWAP_XY
		#if FLIP_Y
			addi	inSource,inSource,-(SRC_BITS_PER_PIXEL / 8)
		#else
			addi	inSource,inSource,(SRC_BITS_PER_PIXEL / 8)
		#endif
	#else
		#if FLIP_Y
			sub		inSource,inSource,inSourceRowBytes
		#else
			add		inSource,inSource,inSourceRowBytes
		#endif
	#endif
	#if (VERTICAL_EXPAND > 1)
		#if (! USE_ALTIVEC)
			add		rDest2,inDest,inDestRowBytes
		#endif
		add		inDest,inDest,inDestRowBytes
	#endif
	#if (VERTICAL_EXPAND > 2)
		#if (! USE_ALTIVEC)
			add		rDest3,rDest2,inDestRowBytes
		#endif
		add		inDest,inDest,inDestRowBytes
	#endif
	add		inDest,inDest,inDestRowBytes
	
	// if we're leaving an extra blank line between vertical lines,
	// add an additional rowbytes here
	#if (VERTICAL_EXPAND_EX != 0)
		add		inDest,inDest,inDestRowBytes
	#endif

	// copy the dirty base registers to their temporary counterparts
	#if (DIRTY_OPTION != NO_DIRTY)
		mr		rDirty,rDirtyMap
	#endif

	// advance the dirty map if we're doing deltas
	#if (DIRTY_OPTION == PIXEL_DELTA || DIRTY_OPTION == PIXEL_DELTA_FULL)
		#if SWAP_XY
			add		rDirtyMap,rDirtyMap,rDirtyRowBytes
		#else
			add		rDirtyMap,rDirtyMap,inSourceRowBytes
		#endif
	#endif

	#if USE_ALTIVEC

		cmpwi	cr0,rLeftEdge,0
		mtctr	rLeftEdge
		beq		SkipLeftEdgeCase

		//=================================================================================
		// edge case loop top
	LeftEdgeLoop:

		// expand and copy 1 pixel
		// this looks simple, but it's actually just a big nasty macro
		EXPAND_PIXEL(DontDrawSingleLeft)
		
		// loop until all remaining pixels are done
		bdnz	LeftEdgeLoop

		//=================================================================================
		// edge case loop bottom

		// for the dirty cases, provide the skipping case
		#if (DIRTY_OPTION != NO_DIRTY) && (DIRTY_OPTION != PIXEL_DELTA_FULL)
			b		SkipLeftEdgeCase
		DontDrawSingleLeft:
			ADD_TO_LOOKASIDE(1 * (SRC_BITS_PER_PIXEL / 8))
			#if USE_ALTIVEC
				ADD_TO_DEST(1 * HORIZONTAL_EXPAND * (DST_BITS_PER_PIXEL / 8))
			#else
				ADD_TO_DEST1(1 * HORIZONTAL_EXPAND * (DST_BITS_PER_PIXEL / 8))
				ADD_TO_DEST2(1 * HORIZONTAL_EXPAND * (DST_BITS_PER_PIXEL / 8))
				ADD_TO_DEST3(1 * HORIZONTAL_EXPAND * (DST_BITS_PER_PIXEL / 8))
			#endif
			bdnz	LeftEdgeLoop
		#endif

		// update the dirty checking mask
	SkipLeftEdgeCase:

	#endif

	// copy the width/8 into the counter register
	#if USE_ALTIVEC
		sub		r0,inWidth,rLeftEdge
		rlwinm.	r0,r0,32-3,3,31
	#else
		rlwinm.	r0,inWidth,32-3,3,31
	#endif
	mtctr	r0

	// skip the core loop if we have less than 8 pixels to draw
	beq		SkipXLoop
	
	//=================================================================================
	// X loop top
xLoop:

	// single buffer dirty case: skip if not dirty, otherwise clear the dirty flag
	#if (DIRTY_OPTION == SINGLE_BUFFER_DIRTY)
		lbz		r0,0(rDirty)
		addi	rDirty,rDirty,1
		cmpwi	cr7,rDirtyMask,0x80
		and.	r0,r0,rDirtyMask
		bne		cr7,DontClear
		li		r0,0
		stb		r0,-1(rDirty)
	DontClear:
		beq		DontDrawMultiDirty
	#endif

	// expand and copy 8 pixels
	// this looks simple, but it's actually just a big nasty macro
	EXPAND_PIXELS
	// loop until all groups of 8 are done
	bdnz	xLoop

	//=================================================================================
	// X loop bottom

	// for the dirty cases, provide the skipping case
	#if (DIRTY_OPTION != NO_DIRTY) && (DIRTY_OPTION != PIXEL_DELTA_FULL)
		b		SkipXLoop
		#if (DIRTY_OPTION == SINGLE_BUFFER_DIRTY)
			DontDrawMultiDirty:
				#if USE_ALTIVEC
					addi	rSource,rSource,X_INCREMENT(8 * (SRC_BITS_PER_PIXEL / 8))
					ADD_TO_LOOKASIDE(8 * (SRC_BITS_PER_PIXEL / 8))
					ADD_TO_DEST(8 * HORIZONTAL_EXPAND * (DST_BITS_PER_PIXEL / 8))
					bdnz	xLoop
					b		SkipXLoop
				#endif
		#endif
	DontDrawMulti:
		#if USE_ALTIVEC
			ADD_TO_LOOKASIDE(16)
			ADD_TO_DEST(8 * HORIZONTAL_EXPAND * (DST_BITS_PER_PIXEL / 8) / (SRC_BITS_PER_PIXEL / 16))
		#else
			#if !SWAP_XY
				addi	rSource,rSource,X_INCREMENT(8 * (SRC_BITS_PER_PIXEL / 8))
			#endif
			ADD_TO_LOOKASIDE(8 * (SRC_BITS_PER_PIXEL / 8))
			ADD_TO_DEST1(8 * HORIZONTAL_EXPAND * (DST_BITS_PER_PIXEL / 8))
			ADD_TO_DEST2(8 * HORIZONTAL_EXPAND * (DST_BITS_PER_PIXEL / 8))
			ADD_TO_DEST3(8 * HORIZONTAL_EXPAND * (DST_BITS_PER_PIXEL / 8))
		#endif
		bdnz	xLoop
	#endif
	
	// handle edge cases
SkipXLoop:
	#if USE_ALTIVEC
		sub		r0,inWidth,rLeftEdge
		rlwinm.	r0,r0,0,29,31
	#else
		rlwinm.	r0,inWidth,0,29,31
	#endif
	mtctr	r0
	beq		SkipEdgeCase

	// single buffer dirty case: skip if not dirty, otherwise clear the dirty flag
	#if (DIRTY_OPTION == SINGLE_BUFFER_DIRTY)
		lbz		r0,0(rDirty)
		addi	rDirty,rDirty,1
		cmpwi	cr7,rDirtyMask,0x80
		and.	r0,r0,rDirtyMask
		bne		cr7,DontClear2
		li		r0,0
		stb		r0,-1(rDirty)
	DontClear2:
		beq		SkipEdgeCase
	#endif

	//=================================================================================
	// edge case loop top
EdgeLoop:

	// expand and copy 1 pixel
	// this looks simple, but it's actually just a big nasty macro
	EXPAND_PIXEL(DontDrawSingle)
	
	// loop until all remaining pixels are done
	bdnz	EdgeLoop

	//=================================================================================
	// edge case loop bottom

	// for the dirty cases, provide the skipping case
	#if (DIRTY_OPTION != NO_DIRTY) && (DIRTY_OPTION != PIXEL_DELTA_FULL)
		b		SkipEdgeCase
	DontDrawSingle:
		ADD_TO_LOOKASIDE(1 * (SRC_BITS_PER_PIXEL / 8))
		#if USE_ALTIVEC
			ADD_TO_DEST(1 * HORIZONTAL_EXPAND * (DST_BITS_PER_PIXEL / 8))
		#else
			ADD_TO_DEST1(1 * HORIZONTAL_EXPAND * (DST_BITS_PER_PIXEL / 8))
			ADD_TO_DEST2(1 * HORIZONTAL_EXPAND * (DST_BITS_PER_PIXEL / 8))
			ADD_TO_DEST3(1 * HORIZONTAL_EXPAND * (DST_BITS_PER_PIXEL / 8))
		#endif
		bdnz	EdgeLoop
	#endif

	// update the dirty checking mask
SkipEdgeCase:
	#if (DIRTY_OPTION == SINGLE_BUFFER_DIRTY)
		rlwinm.	rDirtyMask,rDirtyMask,1,24,31
		bne		DontResetDirty
		li		rDirtyMask,1
		add		rDirtyMap,rDirtyMap,rDirtyRowBytes
	DontResetDirty:
	#endif

	//=================================================================================
	// Y loop bottom

	// count down in height
	subic.	inHeight,inHeight,1
	bne		yLoop

	// restore registers and exit	
	lmw		rLastNonVolatile,-100(sp)
#if USE_ALTIVEC
	// restore vrsave
	mtspr	vrsave,vrsave_save;
#endif
	blr

#undef yLoop
#undef DontClip
#undef SkipLeftEdgeCase
#undef LeftEdgeLoop
#undef DontDrawSingleLeft
#undef SkipXLoop
#undef xLoop
#undef DontClear
#undef DontDrawMulti
#undef DontDrawMultiDirty
#undef SkipEdgeCase
#undef DontClear2
#undef EdgeLoop
#undef DontDrawSingle
#undef DontResetDirty
#undef DontDraw1
#undef DontDraw2
#undef DontDraw3
#undef skip1
#undef skip2
#undef skip3
#undef skip4
#undef Skip1

#undef VERTICAL_EXPAND_INFIX
#undef DIRTY_OPTION_INFIX
#undef HAS_LOOKUP_SUFFIX
#undef SWAP_XY_SUFFIX
#undef FLIP_X_SUFFIX
#undef FLIP_Y_SUFFIX
#undef USE_ALTIVEC_SUFFIX

#undef EXPAND_PIXEL
#undef EXPAND_PIXELS

#undef ADD_TO_DEST1
#undef ADD_TO_DEST2
#undef ADD_TO_DEST3
#undef STORE_TO_DEST1
#undef STORE_TO_DEST2
#undef STORE_TO_DEST3

#undef LOAD_FROM_LOOKASIDE
#undef STORE_TO_LOOKASIDE
#undef ADD_TO_LOOKASIDE
#undef COMPARE_TO_LOOKASIDE
#undef SKIP_IF_NOT_DIRTY
#undef COMBINE_COMPARES

#undef ADD_TO_DEST
#undef STORE_TO_DEST1_VEC
#undef STORE_TO_DEST2_VEC
#undef STORE_TO_DEST3_VEC

#undef LOAD_FROM_SOURCE_FOR_DELTA_VEC
#undef LOAD_FROM_LOOKASIDE_VEC
#undef STORE_TO_LOOKASIDE_VEC
#undef COMPARE_TO_LOOKASIDE_VEC
#undef SKIP_IF_NOT_DIRTY_VEC

#undef VRSAVE_VALUE
#undef ZOOM_TABLE
#undef OFFSET_PERM
#undef OFFSET_OFFSET
#undef VECTOR_FLIP

#undef X_INCREMENT
#undef X_OFFSET
#undef FLIP_PIXELS_16
#undef MOVE_FLIP_PIXELS_16
#undef EXTRACT_PIXEL0_16
#undef EXTRACT_PIXEL1_16
#undef DUPLICATE_PIXEL0_16
#undef DUPLICATE_PIXEL1_16
#undef FLIP_PIXELS_VEC
#undef ZOOM_FLIP_PIXELS_2x_HIGH_VEC
#undef ZOOM_FLIP_PIXELS_2x_LOW_VEC
#undef INCREMENT_SOURCE

#endif
