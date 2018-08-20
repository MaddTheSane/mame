/*##########################################################################

  macblitters.c

  Fast blitters for various occasions. Originally written by Aaron Giles,
  mods by Brad Oliver and Raphael Nabet.

##########################################################################*/


#ifndef BLITTER_RECURSIVE_INCLUDE

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

#else

#endif


#include "macblitters.h"

void blit_1x1_dirty_vector_16to16d( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_dirty_vector_16to16d_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_dirty_vector_16to16d_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_dirty_vector_16to16d_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_dirty_vector_16to16d_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_dirty_vector_16to16d_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_dirty_vector_16to16d_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_dirty_vector_16to16d_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_dirty_vector_16to16d_sxy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_dirty_vector_16to16d_sxy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_dirty_vector_16to16d_sxy_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_dirty_vector_16to16d_sxy_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_dirty_vector_16to16d_sxy_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_dirty_vector_16to16d_sxy_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_dirty_vector_16to16d_sxy_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_dirty_vector_16to16d_sxy_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_dirty_vector_16to16l( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_dirty_vector_16to16l_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_dirty_vector_16to16l_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_dirty_vector_16to16l_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_dirty_vector_16to16l_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_dirty_vector_16to16l_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_dirty_vector_16to16l_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_dirty_vector_16to16l_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_dirty_vector_16to16l_sxy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_dirty_vector_16to16l_sxy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_dirty_vector_16to16l_sxy_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_dirty_vector_16to16l_sxy_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_dirty_vector_16to16l_sxy_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_dirty_vector_16to16l_sxy_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_dirty_vector_16to16l_sxy_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_dirty_vector_16to16l_sxy_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_dirty_vector_16to32l( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_dirty_vector_16to32l_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_dirty_vector_16to32l_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_dirty_vector_16to32l_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_dirty_vector_16to32l_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_dirty_vector_16to32l_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_dirty_vector_16to32l_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_dirty_vector_16to32l_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_dirty_vector_16to32l_sxy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_dirty_vector_16to32l_sxy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_dirty_vector_16to32l_sxy_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_dirty_vector_16to32l_sxy_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_dirty_vector_16to32l_sxy_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_dirty_vector_16to32l_sxy_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_dirty_vector_16to32l_sxy_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_dirty_vector_16to32l_sxy_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_dirty_vector_32to32d( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_dirty_vector_32to32d_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_dirty_vector_32to32d_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_dirty_vector_32to32d_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_dirty_vector_32to32d_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_dirty_vector_32to32d_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_dirty_vector_32to32d_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_dirty_vector_32to32d_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_dirty_vector_32to32d_sxy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_dirty_vector_32to32d_sxy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_dirty_vector_32to32d_sxy_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_dirty_vector_32to32d_sxy_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_dirty_vector_32to32d_sxy_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_dirty_vector_32to32d_sxy_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_dirty_vector_32to32d_sxy_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_dirty_vector_32to32d_sxy_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}


void blit_1x1_full_16to16d( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_full_16to16d_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_full_16to16d_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_full_16to16d_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_full_16to16d_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_full_16to16d_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_full_16to16d_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_full_16to16d_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_full_16to16d_sxy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_full_16to16d_sxy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_full_16to16d_sxy_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_full_16to16d_sxy_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_full_16to16d_sxy_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_full_16to16d_sxy_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_full_16to16d_sxy_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_full_16to16d_sxy_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_full_16to16d( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_full_16to16d_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_full_16to16d_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_full_16to16d_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_full_16to16d_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_full_16to16d_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_full_16to16d_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_full_16to16d_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_full_16to16d_sxy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_full_16to16d_sxy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_full_16to16d_sxy_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_full_16to16d_sxy_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_full_16to16d_sxy_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_full_16to16d_sxy_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_full_16to16d_sxy_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_full_16to16d_sxy_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_16to16d( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_16to16d_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_16to16d_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_16to16d_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_16to16d_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_16to16d_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_16to16d_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_16to16d_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_16to16d_sxy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_16to16d_sxy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_16to16d_sxy_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_16to16d_sxy_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_16to16d_sxy_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_16to16d_sxy_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_16to16d_sxy_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_16to16d_sxy_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_full_16to16d( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_full_16to16d_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_full_16to16d_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_full_16to16d_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_full_16to16d_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_full_16to16d_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_full_16to16d_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_full_16to16d_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_full_16to16d_sxy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_full_16to16d_sxy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_full_16to16d_sxy_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_full_16to16d_sxy_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_full_16to16d_sxy_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_full_16to16d_sxy_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_full_16to16d_sxy_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_full_16to16d_sxy_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_full_16to16d( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_full_16to16d_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_full_16to16d_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_full_16to16d_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_full_16to16d_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_full_16to16d_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_full_16to16d_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_full_16to16d_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_full_16to16d_sxy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_full_16to16d_sxy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_full_16to16d_sxy_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_full_16to16d_sxy_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_full_16to16d_sxy_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_full_16to16d_sxy_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_full_16to16d_sxy_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_full_16to16d_sxy_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_16to16d( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_16to16d_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_16to16d_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_16to16d_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_16to16d_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_16to16d_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_16to16d_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_16to16d_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_16to16d_sxy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_16to16d_sxy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_16to16d_sxy_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_16to16d_sxy_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_16to16d_sxy_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_16to16d_sxy_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_16to16d_sxy_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_16to16d_sxy_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_full_16to16d( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_full_16to16d_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_full_16to16d_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_full_16to16d_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_full_16to16d_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_full_16to16d_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_full_16to16d_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_full_16to16d_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_full_16to16d_sxy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_full_16to16d_sxy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_full_16to16d_sxy_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_full_16to16d_sxy_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_full_16to16d_sxy_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_full_16to16d_sxy_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_full_16to16d_sxy_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_full_16to16d_sxy_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_full_16to16d( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_full_16to16d_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_full_16to16d_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_full_16to16d_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_full_16to16d_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_full_16to16d_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_full_16to16d_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_full_16to16d_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_full_16to16d_sxy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_full_16to16d_sxy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_full_16to16d_sxy_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_full_16to16d_sxy_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_full_16to16d_sxy_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_full_16to16d_sxy_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_full_16to16d_sxy_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_full_16to16d_sxy_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_16to16d( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_16to16d_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_16to16d_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_16to16d_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_16to16d_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_16to16d_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_16to16d_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_16to16d_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_16to16d_sxy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_16to16d_sxy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_16to16d_sxy_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_16to16d_sxy_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_16to16d_sxy_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_16to16d_sxy_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_16to16d_sxy_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_16to16d_sxy_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_full_16to16d( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_full_16to16d_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_full_16to16d_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_full_16to16d_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_full_16to16d_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_full_16to16d_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_full_16to16d_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_full_16to16d_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_full_16to16d_sxy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_full_16to16d_sxy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_full_16to16d_sxy_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_full_16to16d_sxy_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_full_16to16d_sxy_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_full_16to16d_sxy_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_full_16to16d_sxy_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_full_16to16d_sxy_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_full_16to16d( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_full_16to16d_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_full_16to16d_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_full_16to16d_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_full_16to16d_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_full_16to16d_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_full_16to16d_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_full_16to16d_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_full_16to16d_sxy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_full_16to16d_sxy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_full_16to16d_sxy_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_full_16to16d_sxy_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_full_16to16d_sxy_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_full_16to16d_sxy_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_full_16to16d_sxy_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_full_16to16d_sxy_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_16to16d( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_16to16d_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_16to16d_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_16to16d_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_16to16d_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_16to16d_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_16to16d_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_16to16d_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_16to16d_sxy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_16to16d_sxy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_16to16d_sxy_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_16to16d_sxy_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_16to16d_sxy_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_16to16d_sxy_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_16to16d_sxy_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_16to16d_sxy_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_full_16to16d( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_full_16to16d_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_full_16to16d_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_full_16to16d_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_full_16to16d_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_full_16to16d_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_full_16to16d_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_full_16to16d_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_full_16to16d_sxy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_full_16to16d_sxy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_full_16to16d_sxy_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_full_16to16d_sxy_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_full_16to16d_sxy_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_full_16to16d_sxy_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_full_16to16d_sxy_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_full_16to16d_sxy_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_full_16to16d( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_full_16to16d_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_full_16to16d_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_full_16to16d_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_full_16to16d_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_full_16to16d_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_full_16to16d_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_full_16to16d_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_full_16to16d_sxy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_full_16to16d_sxy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_full_16to16d_sxy_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_full_16to16d_sxy_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_full_16to16d_sxy_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_full_16to16d_sxy_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_full_16to16d_sxy_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_full_16to16d_sxy_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_16to16d( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_16to16d_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_16to16d_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_16to16d_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_16to16d_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_16to16d_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_16to16d_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_16to16d_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_16to16d_sxy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_16to16d_sxy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_16to16d_sxy_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_16to16d_sxy_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_16to16d_sxy_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_16to16d_sxy_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_16to16d_sxy_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_16to16d_sxy_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_full_16to16d( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_full_16to16d_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_full_16to16d_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_full_16to16d_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_full_16to16d_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_full_16to16d_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_full_16to16d_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_full_16to16d_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_full_16to16d_sxy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_full_16to16d_sxy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_full_16to16d_sxy_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_full_16to16d_sxy_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_full_16to16d_sxy_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_full_16to16d_sxy_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_full_16to16d_sxy_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_full_16to16d_sxy_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_full_16to16d( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_full_16to16d_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_full_16to16d_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_full_16to16d_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_full_16to16d_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_full_16to16d_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_full_16to16d_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_full_16to16d_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_full_16to16d_sxy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_full_16to16d_sxy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_full_16to16d_sxy_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_full_16to16d_sxy_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_full_16to16d_sxy_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_full_16to16d_sxy_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_full_16to16d_sxy_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_full_16to16d_sxy_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_16to16d( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_16to16d_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_16to16d_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_16to16d_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_16to16d_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_16to16d_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_16to16d_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_16to16d_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_16to16d_sxy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_16to16d_sxy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_16to16d_sxy_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_16to16d_sxy_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_16to16d_sxy_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_16to16d_sxy_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_16to16d_sxy_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_16to16d_sxy_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_full_16to16d( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_full_16to16d_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_full_16to16d_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_full_16to16d_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_full_16to16d_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_full_16to16d_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_full_16to16d_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_full_16to16d_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_full_16to16d_sxy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_full_16to16d_sxy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_full_16to16d_sxy_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_full_16to16d_sxy_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_full_16to16d_sxy_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_full_16to16d_sxy_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_full_16to16d_sxy_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_full_16to16d_sxy_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}


void blit_1x1_full_16to16l( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_full_16to16l_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_full_16to16l_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_full_16to16l_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_full_16to16l_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_full_16to16l_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_full_16to16l_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_full_16to16l_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_full_16to16l_sxy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_full_16to16l_sxy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_full_16to16l_sxy_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_full_16to16l_sxy_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_full_16to16l_sxy_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_full_16to16l_sxy_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_full_16to16l_sxy_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_full_16to16l_sxy_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_full_16to16l( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_full_16to16l_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_full_16to16l_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_full_16to16l_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_full_16to16l_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_full_16to16l_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_full_16to16l_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_full_16to16l_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_full_16to16l_sxy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_full_16to16l_sxy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_full_16to16l_sxy_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_full_16to16l_sxy_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_full_16to16l_sxy_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_full_16to16l_sxy_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_full_16to16l_sxy_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_full_16to16l_sxy_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_16to16l( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_16to16l_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_16to16l_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_16to16l_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_16to16l_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_16to16l_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_16to16l_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_16to16l_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_16to16l_sxy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_16to16l_sxy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_16to16l_sxy_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_16to16l_sxy_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_16to16l_sxy_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_16to16l_sxy_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_16to16l_sxy_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_16to16l_sxy_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_full_16to16l( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_full_16to16l_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_full_16to16l_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_full_16to16l_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_full_16to16l_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_full_16to16l_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_full_16to16l_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_full_16to16l_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_full_16to16l_sxy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_full_16to16l_sxy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_full_16to16l_sxy_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_full_16to16l_sxy_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_full_16to16l_sxy_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_full_16to16l_sxy_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_full_16to16l_sxy_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_full_16to16l_sxy_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_full_16to16l( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_full_16to16l_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_full_16to16l_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_full_16to16l_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_full_16to16l_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_full_16to16l_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_full_16to16l_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_full_16to16l_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_full_16to16l_sxy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_full_16to16l_sxy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_full_16to16l_sxy_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_full_16to16l_sxy_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_full_16to16l_sxy_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_full_16to16l_sxy_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_full_16to16l_sxy_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_full_16to16l_sxy_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_16to16l( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_16to16l_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_16to16l_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_16to16l_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_16to16l_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_16to16l_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_16to16l_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_16to16l_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_16to16l_sxy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_16to16l_sxy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_16to16l_sxy_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_16to16l_sxy_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_16to16l_sxy_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_16to16l_sxy_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_16to16l_sxy_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_16to16l_sxy_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_full_16to16l( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_full_16to16l_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_full_16to16l_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_full_16to16l_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_full_16to16l_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_full_16to16l_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_full_16to16l_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_full_16to16l_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_full_16to16l_sxy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_full_16to16l_sxy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_full_16to16l_sxy_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_full_16to16l_sxy_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_full_16to16l_sxy_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_full_16to16l_sxy_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_full_16to16l_sxy_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_full_16to16l_sxy_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_full_16to16l( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_full_16to16l_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_full_16to16l_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_full_16to16l_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_full_16to16l_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_full_16to16l_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_full_16to16l_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_full_16to16l_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_full_16to16l_sxy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_full_16to16l_sxy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_full_16to16l_sxy_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_full_16to16l_sxy_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_full_16to16l_sxy_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_full_16to16l_sxy_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_full_16to16l_sxy_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_full_16to16l_sxy_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_16to16l( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_16to16l_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_16to16l_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_16to16l_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_16to16l_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_16to16l_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_16to16l_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_16to16l_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_16to16l_sxy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_16to16l_sxy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_16to16l_sxy_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_16to16l_sxy_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_16to16l_sxy_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_16to16l_sxy_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_16to16l_sxy_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_16to16l_sxy_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_full_16to16l( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_full_16to16l_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_full_16to16l_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_full_16to16l_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_full_16to16l_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_full_16to16l_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_full_16to16l_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_full_16to16l_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_full_16to16l_sxy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_full_16to16l_sxy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_full_16to16l_sxy_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_full_16to16l_sxy_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_full_16to16l_sxy_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_full_16to16l_sxy_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_full_16to16l_sxy_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_full_16to16l_sxy_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_full_16to16l( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_full_16to16l_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_full_16to16l_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_full_16to16l_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_full_16to16l_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_full_16to16l_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_full_16to16l_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_full_16to16l_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_full_16to16l_sxy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_full_16to16l_sxy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_full_16to16l_sxy_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_full_16to16l_sxy_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_full_16to16l_sxy_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_full_16to16l_sxy_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_full_16to16l_sxy_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_full_16to16l_sxy_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_16to16l( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_16to16l_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_16to16l_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_16to16l_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_16to16l_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_16to16l_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_16to16l_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_16to16l_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_16to16l_sxy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_16to16l_sxy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_16to16l_sxy_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_16to16l_sxy_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_16to16l_sxy_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_16to16l_sxy_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_16to16l_sxy_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_16to16l_sxy_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_full_16to16l( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_full_16to16l_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_full_16to16l_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_full_16to16l_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_full_16to16l_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_full_16to16l_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_full_16to16l_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_full_16to16l_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_full_16to16l_sxy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_full_16to16l_sxy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_full_16to16l_sxy_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_full_16to16l_sxy_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_full_16to16l_sxy_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_full_16to16l_sxy_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_full_16to16l_sxy_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_full_16to16l_sxy_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_full_16to16l( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_full_16to16l_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_full_16to16l_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_full_16to16l_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_full_16to16l_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_full_16to16l_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_full_16to16l_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_full_16to16l_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_full_16to16l_sxy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_full_16to16l_sxy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_full_16to16l_sxy_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_full_16to16l_sxy_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_full_16to16l_sxy_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_full_16to16l_sxy_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_full_16to16l_sxy_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_full_16to16l_sxy_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_16to16l( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_16to16l_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_16to16l_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_16to16l_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_16to16l_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_16to16l_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_16to16l_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_16to16l_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_16to16l_sxy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_16to16l_sxy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_16to16l_sxy_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_16to16l_sxy_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_16to16l_sxy_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_16to16l_sxy_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_16to16l_sxy_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_16to16l_sxy_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_full_16to16l( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_full_16to16l_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_full_16to16l_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_full_16to16l_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_full_16to16l_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_full_16to16l_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_full_16to16l_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_full_16to16l_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_full_16to16l_sxy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_full_16to16l_sxy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_full_16to16l_sxy_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_full_16to16l_sxy_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_full_16to16l_sxy_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_full_16to16l_sxy_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_full_16to16l_sxy_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_full_16to16l_sxy_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_full_16to16l( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_full_16to16l_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_full_16to16l_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_full_16to16l_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_full_16to16l_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_full_16to16l_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_full_16to16l_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_full_16to16l_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_full_16to16l_sxy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_full_16to16l_sxy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_full_16to16l_sxy_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_full_16to16l_sxy_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_full_16to16l_sxy_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_full_16to16l_sxy_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_full_16to16l_sxy_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_full_16to16l_sxy_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_16to16l( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_16to16l_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_16to16l_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_16to16l_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_16to16l_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_16to16l_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_16to16l_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_16to16l_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_16to16l_sxy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_16to16l_sxy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_16to16l_sxy_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_16to16l_sxy_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_16to16l_sxy_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_16to16l_sxy_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_16to16l_sxy_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_16to16l_sxy_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_full_16to16l( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_full_16to16l_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_full_16to16l_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_full_16to16l_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_full_16to16l_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_full_16to16l_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_full_16to16l_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_full_16to16l_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_full_16to16l_sxy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_full_16to16l_sxy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_full_16to16l_sxy_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_full_16to16l_sxy_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_full_16to16l_sxy_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_full_16to16l_sxy_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_full_16to16l_sxy_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_full_16to16l_sxy_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}


void blit_1x1_full_16to32l( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_full_16to32l_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_full_16to32l_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_full_16to32l_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_full_16to32l_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_full_16to32l_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_full_16to32l_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_full_16to32l_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_full_16to32l_sxy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_full_16to32l_sxy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_full_16to32l_sxy_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_full_16to32l_sxy_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_full_16to32l_sxy_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_full_16to32l_sxy_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_full_16to32l_sxy_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_full_16to32l_sxy_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_full_16to32l( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_full_16to32l_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_full_16to32l_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_full_16to32l_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_full_16to32l_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_full_16to32l_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_full_16to32l_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_full_16to32l_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_full_16to32l_sxy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_full_16to32l_sxy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_full_16to32l_sxy_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_full_16to32l_sxy_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_full_16to32l_sxy_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_full_16to32l_sxy_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_full_16to32l_sxy_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_full_16to32l_sxy_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_16to32l( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_16to32l_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_16to32l_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_16to32l_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_16to32l_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_16to32l_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_16to32l_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_16to32l_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_16to32l_sxy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_16to32l_sxy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_16to32l_sxy_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_16to32l_sxy_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_16to32l_sxy_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_16to32l_sxy_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_16to32l_sxy_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_16to32l_sxy_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_full_16to32l( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_full_16to32l_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_full_16to32l_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_full_16to32l_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_full_16to32l_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_full_16to32l_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_full_16to32l_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_full_16to32l_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_full_16to32l_sxy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_full_16to32l_sxy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_full_16to32l_sxy_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_full_16to32l_sxy_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_full_16to32l_sxy_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_full_16to32l_sxy_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_full_16to32l_sxy_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_full_16to32l_sxy_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_full_16to32l( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_full_16to32l_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_full_16to32l_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_full_16to32l_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_full_16to32l_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_full_16to32l_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_full_16to32l_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_full_16to32l_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_full_16to32l_sxy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_full_16to32l_sxy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_full_16to32l_sxy_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_full_16to32l_sxy_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_full_16to32l_sxy_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_full_16to32l_sxy_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_full_16to32l_sxy_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_full_16to32l_sxy_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_16to32l( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_16to32l_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_16to32l_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_16to32l_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_16to32l_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_16to32l_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_16to32l_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_16to32l_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_16to32l_sxy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_16to32l_sxy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_16to32l_sxy_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_16to32l_sxy_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_16to32l_sxy_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_16to32l_sxy_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_16to32l_sxy_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_16to32l_sxy_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_full_16to32l( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_full_16to32l_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_full_16to32l_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_full_16to32l_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_full_16to32l_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_full_16to32l_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_full_16to32l_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_full_16to32l_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_full_16to32l_sxy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_full_16to32l_sxy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_full_16to32l_sxy_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_full_16to32l_sxy_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_full_16to32l_sxy_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_full_16to32l_sxy_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_full_16to32l_sxy_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_full_16to32l_sxy_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_full_16to32l( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_full_16to32l_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_full_16to32l_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_full_16to32l_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_full_16to32l_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_full_16to32l_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_full_16to32l_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_full_16to32l_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_full_16to32l_sxy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_full_16to32l_sxy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_full_16to32l_sxy_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_full_16to32l_sxy_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_full_16to32l_sxy_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_full_16to32l_sxy_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_full_16to32l_sxy_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_full_16to32l_sxy_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_16to32l( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_16to32l_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_16to32l_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_16to32l_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_16to32l_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_16to32l_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_16to32l_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_16to32l_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_16to32l_sxy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_16to32l_sxy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_16to32l_sxy_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_16to32l_sxy_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_16to32l_sxy_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_16to32l_sxy_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_16to32l_sxy_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_16to32l_sxy_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_full_16to32l( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_full_16to32l_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_full_16to32l_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_full_16to32l_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_full_16to32l_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_full_16to32l_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_full_16to32l_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_full_16to32l_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_full_16to32l_sxy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_full_16to32l_sxy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_full_16to32l_sxy_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_full_16to32l_sxy_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_full_16to32l_sxy_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_full_16to32l_sxy_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_full_16to32l_sxy_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_full_16to32l_sxy_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_full_16to32l( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_full_16to32l_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_full_16to32l_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_full_16to32l_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_full_16to32l_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_full_16to32l_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_full_16to32l_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_full_16to32l_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_full_16to32l_sxy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_full_16to32l_sxy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_full_16to32l_sxy_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_full_16to32l_sxy_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_full_16to32l_sxy_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_full_16to32l_sxy_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_full_16to32l_sxy_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_full_16to32l_sxy_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_16to32l( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_16to32l_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_16to32l_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_16to32l_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_16to32l_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_16to32l_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_16to32l_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_16to32l_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_16to32l_sxy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_16to32l_sxy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_16to32l_sxy_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_16to32l_sxy_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_16to32l_sxy_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_16to32l_sxy_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_16to32l_sxy_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_16to32l_sxy_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_full_16to32l( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_full_16to32l_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_full_16to32l_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_full_16to32l_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_full_16to32l_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_full_16to32l_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_full_16to32l_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_full_16to32l_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_full_16to32l_sxy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_full_16to32l_sxy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_full_16to32l_sxy_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_full_16to32l_sxy_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_full_16to32l_sxy_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_full_16to32l_sxy_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_full_16to32l_sxy_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_full_16to32l_sxy_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_full_16to32l( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_full_16to32l_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_full_16to32l_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_full_16to32l_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_full_16to32l_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_full_16to32l_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_full_16to32l_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_full_16to32l_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_full_16to32l_sxy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_full_16to32l_sxy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_full_16to32l_sxy_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_full_16to32l_sxy_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_full_16to32l_sxy_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_full_16to32l_sxy_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_full_16to32l_sxy_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_full_16to32l_sxy_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_16to32l( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_16to32l_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_16to32l_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_16to32l_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_16to32l_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_16to32l_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_16to32l_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_16to32l_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_16to32l_sxy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_16to32l_sxy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_16to32l_sxy_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_16to32l_sxy_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_16to32l_sxy_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_16to32l_sxy_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_16to32l_sxy_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_16to32l_sxy_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_full_16to32l( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_full_16to32l_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_full_16to32l_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_full_16to32l_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_full_16to32l_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_full_16to32l_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_full_16to32l_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_full_16to32l_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_full_16to32l_sxy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_full_16to32l_sxy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_full_16to32l_sxy_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_full_16to32l_sxy_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_full_16to32l_sxy_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_full_16to32l_sxy_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_full_16to32l_sxy_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_full_16to32l_sxy_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_full_16to32l( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_full_16to32l_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_full_16to32l_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_full_16to32l_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_full_16to32l_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_full_16to32l_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_full_16to32l_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_full_16to32l_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_full_16to32l_sxy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_full_16to32l_sxy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_full_16to32l_sxy_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_full_16to32l_sxy_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_full_16to32l_sxy_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_full_16to32l_sxy_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_full_16to32l_sxy_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_full_16to32l_sxy_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_16to32l( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_16to32l_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_16to32l_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_16to32l_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_16to32l_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_16to32l_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_16to32l_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_16to32l_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_16to32l_sxy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_16to32l_sxy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_16to32l_sxy_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_16to32l_sxy_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_16to32l_sxy_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_16to32l_sxy_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_16to32l_sxy_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_16to32l_sxy_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_full_16to32l( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_full_16to32l_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_full_16to32l_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_full_16to32l_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_full_16to32l_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_full_16to32l_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_full_16to32l_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_full_16to32l_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_full_16to32l_sxy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_full_16to32l_sxy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_full_16to32l_sxy_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_full_16to32l_sxy_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_full_16to32l_sxy_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_full_16to32l_sxy_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_full_16to32l_sxy_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_full_16to32l_sxy_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}


void blit_1x1_full_32to32d( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_full_32to32d_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_full_32to32d_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_full_32to32d_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_full_32to32d_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_full_32to32d_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_full_32to32d_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_full_32to32d_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_full_32to32d_sxy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_full_32to32d_sxy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_full_32to32d_sxy_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_full_32to32d_sxy_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_full_32to32d_sxy_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_full_32to32d_sxy_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_full_32to32d_sxy_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x1_full_32to32d_sxy_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_full_32to32d( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_full_32to32d_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_full_32to32d_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_full_32to32d_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_full_32to32d_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_full_32to32d_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_full_32to32d_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_full_32to32d_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_full_32to32d_sxy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_full_32to32d_sxy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_full_32to32d_sxy_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_full_32to32d_sxy_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_full_32to32d_sxy_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_full_32to32d_sxy_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_full_32to32d_sxy_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_full_32to32d_sxy_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_32to32d( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_32to32d_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_32to32d_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_32to32d_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_32to32d_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_32to32d_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_32to32d_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_32to32d_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_32to32d_sxy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_32to32d_sxy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_32to32d_sxy_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_32to32d_sxy_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_32to32d_sxy_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_32to32d_sxy_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_32to32d_sxy_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_32to32d_sxy_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_full_32to32d( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_full_32to32d_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_full_32to32d_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_full_32to32d_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_full_32to32d_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_full_32to32d_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_full_32to32d_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_full_32to32d_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_full_32to32d_sxy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_full_32to32d_sxy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_full_32to32d_sxy_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_full_32to32d_sxy_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_full_32to32d_sxy_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_full_32to32d_sxy_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_full_32to32d_sxy_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2_delta_full_32to32d_sxy_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_full_32to32d( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_full_32to32d_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_full_32to32d_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_full_32to32d_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_full_32to32d_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_full_32to32d_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_full_32to32d_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_full_32to32d_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_full_32to32d_sxy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_full_32to32d_sxy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_full_32to32d_sxy_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_full_32to32d_sxy_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_full_32to32d_sxy_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_full_32to32d_sxy_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_full_32to32d_sxy_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_full_32to32d_sxy_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_32to32d( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_32to32d_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_32to32d_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_32to32d_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_32to32d_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_32to32d_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_32to32d_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_32to32d_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_32to32d_sxy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_32to32d_sxy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_32to32d_sxy_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_32to32d_sxy_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_32to32d_sxy_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_32to32d_sxy_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_32to32d_sxy_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_32to32d_sxy_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_full_32to32d( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_full_32to32d_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_full_32to32d_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_full_32to32d_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_full_32to32d_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_full_32to32d_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_full_32to32d_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_full_32to32d_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_full_32to32d_sxy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_full_32to32d_sxy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_full_32to32d_sxy_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_full_32to32d_sxy_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_full_32to32d_sxy_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_full_32to32d_sxy_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_full_32to32d_sxy_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_1x2i_delta_full_32to32d_sxy_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_full_32to32d( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_full_32to32d_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_full_32to32d_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_full_32to32d_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_full_32to32d_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_full_32to32d_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_full_32to32d_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_full_32to32d_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_full_32to32d_sxy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_full_32to32d_sxy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_full_32to32d_sxy_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_full_32to32d_sxy_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_full_32to32d_sxy_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_full_32to32d_sxy_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_full_32to32d_sxy_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_full_32to32d_sxy_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_32to32d( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_32to32d_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_32to32d_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_32to32d_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_32to32d_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_32to32d_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_32to32d_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_32to32d_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_32to32d_sxy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_32to32d_sxy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_32to32d_sxy_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_32to32d_sxy_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_32to32d_sxy_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_32to32d_sxy_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_32to32d_sxy_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_32to32d_sxy_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_full_32to32d( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_full_32to32d_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_full_32to32d_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_full_32to32d_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_full_32to32d_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_full_32to32d_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_full_32to32d_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_full_32to32d_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_full_32to32d_sxy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_full_32to32d_sxy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_full_32to32d_sxy_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_full_32to32d_sxy_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_full_32to32d_sxy_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_full_32to32d_sxy_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_full_32to32d_sxy_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2_delta_full_32to32d_sxy_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_full_32to32d( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_full_32to32d_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_full_32to32d_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_full_32to32d_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_full_32to32d_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_full_32to32d_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_full_32to32d_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_full_32to32d_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_full_32to32d_sxy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_full_32to32d_sxy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_full_32to32d_sxy_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_full_32to32d_sxy_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_full_32to32d_sxy_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_full_32to32d_sxy_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_full_32to32d_sxy_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_full_32to32d_sxy_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_32to32d( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_32to32d_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_32to32d_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_32to32d_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_32to32d_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_32to32d_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_32to32d_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_32to32d_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_32to32d_sxy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_32to32d_sxy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_32to32d_sxy_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_32to32d_sxy_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_32to32d_sxy_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_32to32d_sxy_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_32to32d_sxy_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_32to32d_sxy_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_full_32to32d( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_full_32to32d_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_full_32to32d_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_full_32to32d_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_full_32to32d_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_full_32to32d_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_full_32to32d_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_full_32to32d_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_full_32to32d_sxy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_full_32to32d_sxy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_full_32to32d_sxy_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_full_32to32d_sxy_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_full_32to32d_sxy_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_full_32to32d_sxy_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_full_32to32d_sxy_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_2x2i_delta_full_32to32d_sxy_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_full_32to32d( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_full_32to32d_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_full_32to32d_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_full_32to32d_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_full_32to32d_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_full_32to32d_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_full_32to32d_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_full_32to32d_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_full_32to32d_sxy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_full_32to32d_sxy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_full_32to32d_sxy_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_full_32to32d_sxy_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_full_32to32d_sxy_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_full_32to32d_sxy_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_full_32to32d_sxy_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_full_32to32d_sxy_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_32to32d( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_32to32d_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_32to32d_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_32to32d_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_32to32d_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_32to32d_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_32to32d_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_32to32d_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_32to32d_sxy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_32to32d_sxy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_32to32d_sxy_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_32to32d_sxy_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_32to32d_sxy_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_32to32d_sxy_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_32to32d_sxy_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_32to32d_sxy_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_full_32to32d( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_full_32to32d_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_full_32to32d_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_full_32to32d_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_full_32to32d_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_full_32to32d_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_full_32to32d_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_full_32to32d_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_full_32to32d_sxy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_full_32to32d_sxy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_full_32to32d_sxy_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_full_32to32d_sxy_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_full_32to32d_sxy_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_full_32to32d_sxy_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_full_32to32d_sxy_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3_delta_full_32to32d_sxy_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_full_32to32d( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_full_32to32d_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_full_32to32d_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_full_32to32d_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_full_32to32d_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_full_32to32d_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_full_32to32d_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_full_32to32d_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_full_32to32d_sxy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_full_32to32d_sxy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_full_32to32d_sxy_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_full_32to32d_sxy_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_full_32to32d_sxy_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_full_32to32d_sxy_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_full_32to32d_sxy_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_full_32to32d_sxy_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_32to32d( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_32to32d_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_32to32d_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_32to32d_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_32to32d_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_32to32d_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_32to32d_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_32to32d_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_32to32d_sxy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_32to32d_sxy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_32to32d_sxy_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_32to32d_sxy_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_32to32d_sxy_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_32to32d_sxy_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_32to32d_sxy_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_32to32d_sxy_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_full_32to32d( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_full_32to32d_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_full_32to32d_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_full_32to32d_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_full_32to32d_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_full_32to32d_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_full_32to32d_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_full_32to32d_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_full_32to32d_sxy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_full_32to32d_sxy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_full_32to32d_sxy_fx( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_full_32to32d_sxy_fx_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_full_32to32d_sxy_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_full_32to32d_sxy_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_full_32to32d_sxy_fx_fy( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}

void blit_3x3i_delta_full_32to32d_sxy_fx_fy_vec( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}


void blit_1x1_full_32to32l( UInt8 * inSource, UInt8 * inDest, UInt32 inSourceRowBytes, UInt32 inDestRowBytes, UInt32 inWidth, UInt32 inHeight, const DisplayParameters * inParams)
{

}


