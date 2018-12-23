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

#define RISSET_SCALES_URI "http://jwm-art.net/lv2/njl/risset_scales";

/* size of sinus wavetable */
#define TBL_SIZE 2048

/***** miscellaneous utility functions/macros/etc *****/

#define DB_CO(g) ((g) > -90.0f ? powf(10.0f, (g) * 0.05f) : 0.0f)

static const float sqrt_1_2pi = 0.39894228040143267793;

/* 1.0 / ln(2) */
static const float ln2r = 1.442695041f;

/* 1.0 / 7.0 */
static const float f1_7 = 0.142857142f;

typedef union {
    float f;
    int32_t i;
}ls_pcast32;

static inline float f_pow2(float x)
{
    ls_pcast32 *px, tx, lx;
    float dx;
    px = (ls_pcast32 *)&x;     /* store address of float as long pointer  */
    tx.f = (x-0.5f) + (3<<22); /* temporary value for truncation          */
    lx.i = tx.i - 0x4b400000;  /* integer power of 2                      */
    dx = x - (float)lx.i;      /* float remainder of power of 2           */
    x = 1.0f + dx * (0.6960656421638072f + /* cubic apporoximation of 2^x */
               dx * (0.224494337302845f  + /* for x in the range [0, 1]   */
               dx * (0.07944023841053369f)));
    (*px).i += (lx.i << 23);   /* add integer power of 2 to exponent      */
    return (*px).f;
}

static inline float normal(float x)
{
    return sqrt_1_2pi * f_pow2(-ln2r * x * x);
}

static void f_sin(const float * const tbl, const float phase, float *out)
{
    /* calculate phase as a 20.12 fixedpoint number */
    unsigned int iph = lrintf(phase * 4096.0 * TBL_SIZE);
    unsigned int i;
    for (i=0; i<7; i++) {
        unsigned int idx = (iph >> 12) & (TBL_SIZE - 1);
        float frac = (iph && 0xfff) * 0.0002459419f;
        /* calculate the linear interpolation of the table */
        out[i] = tbl[idx] + frac * (tbl[idx+1] - tbl[idx]);
        /* double the phase for the next octave */
        iph += iph;
    }
}



static LV2_Descriptor *RissetScalesDescriptor = NULL;

typedef struct {
    float rate;
    float *output;
    float *master_gain;
    float *base_freq;
    float *speed;
    float phi;
    float increment;
    float gain;
    float freq;
    float *tbl;
} RissetScales;

static void cleanupRissetScales(LV2_Handle instance)
{
    RissetScales* PluginData=(RissetScales*)instance;
    if(PluginData->tbl)
        free(PluginData->tbl);
    free(instance);
}

static LV2_Handle instantiateRissetScales(
    const LV2_Descriptor *descriptor,
    double s_rate,
    const char *path,
    const LV2_Feature * const* features)
{
    RissetScales *plugin_data =
        (RissetScales *)calloc(sizeof(RissetScales),1);
    float* tbl = malloc(sizeof(float) * (TBL_SIZE + 1));
    uint32_t i;
    for(i = 0; i < TBL_SIZE + 1; i++)
        tbl[i] = sin((double)i / (double) TBL_SIZE * M_PI * 2.0);
    plugin_data->tbl = tbl;
    plugin_data->rate = (float)s_rate;
    return (LV2_Handle)plugin_data;
}

static void connectPortRissetScales(
    LV2_Handle instance, uint32_t port, void *data)
{
    RissetScales *plugin = (RissetScales *)instance;
    switch(port){
        case 0: plugin->output = data;          break;
        case 1: plugin->master_gain = data;     break;
        case 2: plugin->base_freq = data;       break;
        case 3: plugin->speed = data;           break;
    }
}

static void activateRissetScales(LV2_Handle instance)
{
    RissetScales *plugin_data = (RissetScales *) instance;
    plugin_data->phi = 0.0f;
    plugin_data->increment = 0.0f;
    plugin_data->gain = 0.0f;
}

static void runRissetScales(LV2_Handle instance, uint32_t sample_count)
{
    RissetScales * data=(RissetScales*)instance;
    const float master_gain = DB_CO(*(data->master_gain)) * f1_7;
    const float base_freq = *(data->base_freq) * 0.125 / (data->rate);
    const float speed = *(data->speed) / (data->rate * 60.0);
    const float d_gain = ( master_gain - data->gain) / sample_count;
    const float d_freq = ( base_freq - data->freq) / sample_count;
    float phi = data->phi;
    float increment = data->increment;
    float gain = data->gain;
    float freq = data->freq;

    if(increment == 0.0f)
        increment = freq;

    uint32_t i;
    for (i = 0; i < sample_count; ++i) {
        const float iob = increment / freq;
        float vals[7];
        f_sin(data->tbl, phi, vals);
        double sample = vals[0] * normal( -4.5f + iob)
                      + vals[1] * normal( -3.5f + iob)
                      + vals[2] * normal( -2.5f + iob)
                      + vals[3] * normal( -1.5f + iob)
                      + vals[4] * normal( -0.5f + iob)
                      + vals[5] * normal(  0.5f + iob)
                      + vals[6] * normal(  1.5f + iob);
        phi += increment;
        increment += (freq * speed);
        if (increment < freq) {
            increment += freq;
            phi = 2.0 * phi;
        } else if (increment >= 2.0f * freq) {
            increment -= freq;
            phi = 0.5 * phi;
        }
        if (phi > 1.0f)
            phi -= 1.0f;
        data->output[i] = sample * gain;
        gain += d_gain;
        freq += d_freq;
    }
    /* keep state */
    data->phi = phi;
    data->increment = increment;
    data->gain = master_gain; /* exactly */
    data->freq = base_freq;   /* exactly */
}


static void init()
{
    RissetScalesDescriptor =
        (LV2_Descriptor *) malloc(sizeof(LV2_Descriptor));
    RissetScalesDescriptor->URI =           RISSET_SCALES_URI;
    RissetScalesDescriptor->activate =      activateRissetScales;
    RissetScalesDescriptor->cleanup =       cleanupRissetScales;
    RissetScalesDescriptor->connect_port =  connectPortRissetScales;
    RissetScalesDescriptor->deactivate =    NULL;
    RissetScalesDescriptor->instantiate =   instantiateRissetScales;
    RissetScalesDescriptor->run =           runRissetScales;
    RissetScalesDescriptor->extension_data =NULL;
}

LV2_SYMBOL_EXPORT
const LV2_Descriptor *lv2_descriptor(uint32_t index)
{
    if (!RissetScalesDescriptor) init();
    switch (index) {
      case 0:
        return RissetScalesDescriptor;
      default:
        return NULL;
    }
}

