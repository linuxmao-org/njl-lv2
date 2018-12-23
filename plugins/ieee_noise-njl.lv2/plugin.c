/*  noise_1921 - IEEE Single Precision Noise
    Copyright (C) 2002-2005  Nick Lamb <njl195@zepler.org.uk>

    LV2 port by James W. Morris <james@jwm-art.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/

#include <stdlib.h>
#include <math.h>
#include <lv2.h>

#define IEEE_NOISE_URI   "http://jwm-art.net/lv2/njl/ieee_noise";

static LV2_Descriptor *IeeeNoiseDescriptor = NULL;

typedef struct {
    float *output;
} IeeeNoise;

static void cleanupIeeeNoise(LV2_Handle instance)
{
    free(instance);
}

static void connectPortIeeeNoise(
    LV2_Handle instance, uint32_t port, void *data)
{
    IeeeNoise *plugin = (IeeeNoise *)instance;
    if(port==0)
        plugin->output = data;
}

static LV2_Handle instantiateIeeeNoise(
    const LV2_Descriptor *descriptor,
    double s_rate,
    const char *path,
    const LV2_Feature * const* features)
{
    IeeeNoise *plugin_data = (IeeeNoise *)malloc(sizeof(IeeeNoise));
    return (LV2_Handle)plugin_data;
}


static void runIeeeNoise(LV2_Handle instance, uint32_t sample_count)
{
    float * const output = ((IeeeNoise *)instance)->output;
    uint32_t i;
    for (i = 0; i < sample_count; ++i) {
        union { float f; unsigned int u; } sample;
        unsigned int exponent, mantissa, sign;
        sign = random() % 2;
        exponent = random() % 127;
        mantissa = random() % 0x800000;
        if (exponent > 0)
            sample.u = (sign << 31) + (exponent << 23) + mantissa;
        else
            sample.u = (sign << 31);
        output[i] = sample.f;
    }
}

static void init()
{
    IeeeNoiseDescriptor =
        (LV2_Descriptor *) malloc(sizeof(LV2_Descriptor));
    IeeeNoiseDescriptor->URI =              IEEE_NOISE_URI;
    IeeeNoiseDescriptor->activate =         NULL;
    IeeeNoiseDescriptor->cleanup =          cleanupIeeeNoise;
    IeeeNoiseDescriptor->connect_port =     connectPortIeeeNoise;
    IeeeNoiseDescriptor->deactivate =       NULL;
    IeeeNoiseDescriptor->instantiate =      instantiateIeeeNoise;
    IeeeNoiseDescriptor->run =              runIeeeNoise;
    IeeeNoiseDescriptor->extension_data =   NULL;
}

LV2_SYMBOL_EXPORT
const LV2_Descriptor *lv2_descriptor(uint32_t index)
{
    if (!IeeeNoiseDescriptor) init();
    switch (index) {
      case 0:
        return IeeeNoiseDescriptor;
      default:
        return NULL;
    }
}

