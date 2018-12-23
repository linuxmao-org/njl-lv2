/*  noise_1922 - Integer Noise
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

/* lrintf sometimes is not defined */
extern long int lrintf(float x);

#define INT_NOISE_URI   "http://jwm-art.net/lv2/njl/int_noise";

static LV2_Descriptor *IntNoiseDescriptor = NULL;

typedef struct {
    float *output;
    float *bits;
} IntNoise;

static void cleanupIntNoise(LV2_Handle instance)
{
    free(instance);
}

static void connectPortIntNoise(
    LV2_Handle instance, uint32_t port, void *data)
{
    IntNoise *plugin = (IntNoise *)instance;
    switch(port){
        case 0: plugin->output = data;  break;
        case 1: plugin->bits = data;    break;
    }
}

static LV2_Handle instantiateIntNoise(
    const LV2_Descriptor *descriptor,
    double s_rate,
    const char *path,
    const LV2_Feature * const* features)
{
    IntNoise *plugin_data = (IntNoise *)malloc(sizeof(IntNoise));
    return (LV2_Handle)plugin_data;
}


static void runIntNoise(LV2_Handle instance, uint32_t sample_count)
{
    IntNoise * PluginData=(IntNoise*)instance;
    float * const output = PluginData->output;
    float bits = *(PluginData->bits);

    if (bits < 1.0f)
        bits = 1.0f;
    else if (bits > 24.0f)
        bits = 24.0f;

    float power = powf(2.0f, bits);
    float next = powf(2.0f, ceilf(bits));
    long precision = lrintf(power);

    uint32_t i;
    for (i = 0; i < sample_count; ++i) {
        float r = random() % precision;
        float result = ceilf(r * (next / power));
        output[i] = (result / next) - 1.0f;
    }
}

static void init()
{
    IntNoiseDescriptor =
        (LV2_Descriptor *) malloc(sizeof(LV2_Descriptor));
    IntNoiseDescriptor->URI =           INT_NOISE_URI;
    IntNoiseDescriptor->activate =      NULL;
    IntNoiseDescriptor->cleanup =       cleanupIntNoise;
    IntNoiseDescriptor->connect_port =  connectPortIntNoise;
    IntNoiseDescriptor->deactivate =    NULL;
    IntNoiseDescriptor->instantiate =   instantiateIntNoise;
    IntNoiseDescriptor->run =           runIntNoise;
    IntNoiseDescriptor->extension_data =NULL;
}

LV2_SYMBOL_EXPORT
const LV2_Descriptor *lv2_descriptor(uint32_t index)
{
    if (!IntNoiseDescriptor) init();
    switch (index) {
      case 0:
        return IntNoiseDescriptor;
      default:
        return NULL;
    }
}

