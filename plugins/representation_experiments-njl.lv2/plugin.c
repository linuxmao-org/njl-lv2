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

#define REPRESENTATION_EXPERIMENTS_URI "http://www.jwm-art.net/lv2/njl/representation_experiments";

static LV2_Descriptor *RepresentationExperimentsDescriptor = NULL;

typedef struct {
    float rate;
    float *output;
    float *input;
    float *mantissa;
    float *exponent;
} RepresentationExperiments;

static void cleanupRepresentationExperiments(LV2_Handle instance)
{
    free(instance);
}

static LV2_Handle instantiateRepresentationExperiments(
    const LV2_Descriptor *descriptor,
    double s_rate,
    const char *path,
    const LV2_Feature * const* features)
{
    return (LV2_Handle)
        calloc(sizeof(RepresentationExperiments),1);
}

static void connectPortRepresentationExperiments(
    LV2_Handle instance, uint32_t port, void *data)
{
    RepresentationExperiments *plugin = (RepresentationExperiments *)instance;
    switch(port){
        case 0: plugin->output = data;      break;
        case 1: plugin->input = data;       break;
        case 2: plugin->mantissa = data;    break;
        case 3: plugin->exponent = data;    break;
    }
}

static void runRepresentationExperiments(LV2_Handle instance, uint32_t sample_count)
{
    RepresentationExperiments * data=(RepresentationExperiments*)instance;
    int mbits = lrintf(*(data->mantissa));
    int ebits = lrintf(*(data->exponent));

    if (mbits > 23)mbits = 23;
    if (mbits < 0) mbits = 0;
    if (ebits > 8) ebits = 8;
    if (ebits < 1) ebits = 1;

    uint32_t i;
    for (i = 0; i < sample_count; ++i) {
        union { float f; unsigned int u; } sample;
        unsigned int exponent, mantissa, sign;
        sample.f = data->input[i];
        /* take sample apart */
        sign = (sample.u & 0x80000000) >> 31;
        exponent = (sample.u & 0x7f800000) >> 23;
        mantissa = (sample.u & 0x007fffff);
        /* shorten mantissa */
        unsigned int power = 1 << (23 - mbits);
        unsigned int fraction = mantissa % power;
        unsigned int whole = mantissa / power;
        if (fraction * 2 > power)
            mantissa = (whole + 1) * power;
        else
            mantissa = whole * power;
        if (mantissa >= 0x800000) {
            mantissa = 0;
            exponent+= 1;
        }
        /* range limit exponent */
        if (exponent > 126){
            exponent = 127;
            mantissa= 0;
        }
        if ((1 << ebits) <= (127 - exponent)) {
            exponent = 0;
            mantissa= 0;
        }
        /* put sample back together */
        sample.u = (sign << 31) + (exponent << 23) + mantissa;
        data->output[i] = sample.f;
    }
}

static void init()
{
    RepresentationExperimentsDescriptor =
        (LV2_Descriptor *) malloc(sizeof(LV2_Descriptor));
    RepresentationExperimentsDescriptor->URI =           REPRESENTATION_EXPERIMENTS_URI;
    RepresentationExperimentsDescriptor->activate =      NULL;
    RepresentationExperimentsDescriptor->cleanup =       cleanupRepresentationExperiments;
    RepresentationExperimentsDescriptor->connect_port =  connectPortRepresentationExperiments;
    RepresentationExperimentsDescriptor->deactivate =    NULL;
    RepresentationExperimentsDescriptor->instantiate =   instantiateRepresentationExperiments;
    RepresentationExperimentsDescriptor->run =           runRepresentationExperiments;
    RepresentationExperimentsDescriptor->extension_data =NULL;
}

LV2_SYMBOL_EXPORT
const LV2_Descriptor *lv2_descriptor(uint32_t index)
{
    if (!RepresentationExperimentsDescriptor) init();
    switch (index) {
      case 0:
        return RepresentationExperimentsDescriptor;
      default:
        return NULL;
    }
}

