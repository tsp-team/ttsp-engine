/**
 * PANDA3D BSP LIBRARY
 *
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file lightmap_palettes.h
 * @author Brian Lach
 * @date August 01, 2018
 */

#ifndef LIGHTMAP_PALETTES_H
#define LIGHTMAP_PALETTES_H

#include "pnmImage.h"
#include "pvector.h"
#include "notifyCategoryProxy.h"
#include "aa_luse.h"
#include "texture.h"

#include "imagePacker.h"
#ifndef CPPPARSER
#include "mathlib.h"
#define NUM_LIGHTMAPS 1 + (NUM_BUMP_VECTS + 1)
#else
struct colorrgbexp32_t;
#define NUM_LIGHTMAPS 1
#endif

#include "config_bsp.h"

class BSPLevel;

// Max size per palette before making a new one.
static const int max_palette = 4096;

/**
 * A single lightmap palette/page. Contains an array texture.
 * Slice 0 - Bounced light
 * Slice 1 - Non-bumped direct light
 * Slice 2 - Bump 0 direct light
 * Slice 3 - Bump 1 direct light
 * Slice 4 - Bump 2 direct light
 */
class LightmapPalette : public ReferenceCount {
public:
  class Entry : public ReferenceCount {
  public:
    LightmapPalette *palette;
    int facenum;
    int offset[2];

    Entry() {
      facenum = 0;
      offset[0] = offset[1] = 0;
      palette = nullptr;
    }
  };

  pvector<PT(Entry)> entries;
  ImagePacker packer;
  PT(Texture) texture;
  int size[2];

  LightmapPalette() {
    packer.reset(0, max_palette, max_palette);
    texture = nullptr;
    size[0] = size[1] = 0;
  }
};

class LightmapPaletteDirectory : public ReferenceCount {
public:
  pvector<PT(LightmapPalette)> palettes;
  pvector<LightmapPalette::Entry *> face_palette_entries;
};

NotifyCategoryDeclNoExport(lightmapPalettizer);

INLINE PN_stdfloat
gamma_encode(PN_stdfloat linear, PN_stdfloat gamma) {
  return pow(linear, 1.0 / gamma);
}

INLINE LRGBColor color_shift_pixel(colorrgbexp32_t *colsample, PN_stdfloat gamma)
{
  LVector3 sample(0);
  ColorRGBExp32ToVector(*colsample, sample);

  return LRGBColor(gamma_encode(sample[0] / 255.0, gamma),
                   gamma_encode(sample[1] / 255.0, gamma),
                   gamma_encode(sample[2] / 255.0, gamma));
}


class EXPCL_PANDABSP LightmapPalettizer {
public:
  LightmapPalettizer(const BSPLevel *level);
  PT(LightmapPaletteDirectory) palettize_lightmaps();

private:
  const BSPLevel *_level;
};

#endif // LIGHTMAP_PALETTES_H
