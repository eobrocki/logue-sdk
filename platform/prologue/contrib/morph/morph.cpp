/*
	BSD 3-Clause License

	Copyright (c) 2018, KORG INC.
	All rights reserved.

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions are met:

	* Redistributions of source code must retain the above copyright notice, this
	  list of conditions and the following disclaimer.

	* Redistributions in binary form must reproduce the above copyright notice,
	  this list of conditions and the following disclaimer in the documentation
	  and/or other materials provided with the distribution.

	* Neither the name of the copyright holder nor the names of its
	  contributors may be used to endorse or promote products derived from
	  this software without specific prior written permission.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
	AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
	IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
	DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
	FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
	DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
	SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
	CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
	OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
	OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

//*/

/*
 * Morphs between several wave shapes: triangle, sine, square, sawtooth
 */

#include "userosc.h"

const float PWM_MIN = 0.00f;
const float PWM_MAX = 1.0f; // 0.95f;

const float TROUGH = -1.0f;
const float PEAK = 1.0f;
const float AMPLITUDE = PEAK - TROUGH;

#if 0
class BaseOsc {
public:
	// Current frame counter
	float frame;

	BaseOsc(void) {
		frame = 0;
	}

	virtual void note_on(void) {
	}

	virtual void note_off(void) {
	}

	virtual void set_param_shape(float value) {
	}

	virtual void set_param_shiftshape(float value) {
	}

	virtual void set_param_id1(float value) {
	}

	virtual void set_param_id2(float value) {
	}

	virtual void set_param_id3(float value) {
	}

	virtual void set_param_id4(float value) {
	}

	virtual void set_param_id5(float value) {
	}

	virtual void set_param_id6(float value) {
	}

	virtual float y(float x, float lfo) = 0;

	float cycle(float hz, float lfo) {
		float frames_per_cycle = float(k_samplerate) / hz;
		if (frames_per_cycle < 1)
			frames_per_cycle = 1;

		float cycles = frame / frames_per_cycle;
		cycles = floorf(cycles);
		if (cycles > 0)
			frame -= (cycles * frames_per_cycle);
		if (frame < 0)
			frame = 0;

		float x = frame / frames_per_cycle;
		float ret = y(x, lfo);

		frame++;

		return ret;	
	}

	void NOTEON(const user_osc_param_t * const params) {
		note_on();
	}

	void NOTEOFF(const user_osc_param_t * const params)
	{
		note_off();
	}

	void CYCLE(const user_osc_param_t * const params,
		int32_t *yn,
		const uint32_t frames) 
	{
		q31_t * __restrict y = (q31_t *)yn;
		uint8_t note = (params->pitch) >> 8;
		uint8_t mod = params->pitch & 0xFF;
		const float lfo = q31_to_f32(params->shape_lfo);
		const float cutoff = param_val_to_f32(params->cutoff);
		const float resonance = param_val_to_f32(params->resonance);

		const float f0 = osc_notehzf(note);
		const float f1 = osc_notehzf(note + 1);
		float hz = linintf(mod * k_note_mod_fscale, f0, f1);
		hz = clipmaxf(hz, k_note_max_hz);
		if (hz < 1)
			hz = 1.0f;

		for (int i = 0; i < frames; i++) {
			float sig = cycle(hz, lfo);
			y[i] = f32_to_q31(sig);
		}
	}

	void PARAM(uint16_t index, uint16_t value)
	{
		float valf = param_val_to_f32(value);

		switch (index) {
		case k_osc_param_shape:
			set_param_shape(valf);
			break;
		case k_osc_param_shiftshape:
			set_param_shiftshape(valf);
			break;
		case k_osc_param_id1:
			set_param_id1(valf);
			break;
		case k_osc_param_id2:
			set_param_id2(valf);
			break;
		case k_osc_param_id3:
			set_param_id3(valf);
			break;
		case k_osc_param_id4:
			set_param_id4(valf);
			break;
		case k_osc_param_id5:
			set_param_id5(valf);
			break;
		case k_osc_param_id6:
			set_param_id6(valf);
			break;
		default:
			break;
		}
	}

	void INIT(uint32_t platform, uint32_t api) {

	}
};

class MorphOsc : public BaseOsc {
public:
	MorphOsc() {

	}

	virtual float y(float x, float lfo) {
		int xx = x;
		float ret = xx % 1000;
		ret /= 1000.0;
		return ret;
		return (xx % 1000);
		//return osc_sinf(x);
	}
};

static MorphOsc s_osc;

void OSC_INIT(uint32_t platform, uint32_t api)
{
	s_osc.INIT(platform, api);
}

void OSC_CYCLE(const user_osc_param_t * const params,
	int32_t *yn,
	const uint32_t frames)
{
	s_osc.CYCLE(params, yn, frames);
}

void OSC_NOTEON(const user_osc_param_t * const params)
{
	s_osc.NOTEON(params);
}

void OSC_NOTEOFF(const user_osc_param_t * const params)
{
	s_osc.NOTEOFF(params);
}

void OSC_PARAM(uint16_t index, uint16_t value)
{
	s_osc.PARAM(index, value);
}

#endif

float myremainder(float a, float b) {
	float d = a / b;
	d = floorf(d);
	return a - (d * b);
}

class MorphOsc {
public:
	// Current frame counter
	float frame;

	float shape;
	float shiftshape;

	MorphOsc(void) {
		frame = 0;
		shape = 0;
		shiftshape = 0;
	}

	void note_on(void) {
	}

	void note_off(void) {
	}

	void set_param_shape(float value) {
		shape = value;
	}

	void set_param_shiftshape(float value) {
		shiftshape = value;
	}

	void set_param_id1(float value) {
	}

	void set_param_id2(float value) {
	}

	void set_param_id3(float value) {
	}

	void set_param_id4(float value) {
	}

	void set_param_id5(float value) {
	}

	void set_param_id6(float value) {
	}



	float y(float x, float lfo) {

		float lfo_weight = 1 - shiftshape;
		x += (lfo_weight * lfo);
		x = myremainder(x, 1.0);

		const int NUM_OSC = 3;
		float vals[NUM_OSC];

		vals[0] = osc_sawf(x);
		vals[1] = osc_sqrf(x);
		vals[2] = osc_sinf(x);

		float tmp_shape = shape + shiftshape * lfo;
		tmp_shape = clip01f(tmp_shape);

		float weight = 0;
		int index_0 = 0;
		int index_1 = 1;
		const float NUM_SLICES = NUM_OSC - 1;
		const float SLICE = (1.0 / NUM_SLICES);
		if (tmp_shape <= SLICE) {
			index_0 = 0;
			index_1 = 1;
			weight = tmp_shape * NUM_SLICES;
		}
		else {
			index_0 = 1;
			index_1 = 2;
			weight = (tmp_shape - SLICE) * NUM_SLICES;
		}
		return ((1.0 - weight) * vals[index_0]) + (weight * vals[index_1]);
	}

	float cycle(float hz, float lfo) {
		float frames_per_cycle = float(k_samplerate) / hz;
		if (frames_per_cycle < 1)
			frames_per_cycle = 1;

		frame = myremainder(frame, frames_per_cycle);
		if (frame < 0)
			frame = 0;

		float x = frame / frames_per_cycle;
		float ret = y(x, lfo);

		frame++;

		return ret;
	}

	void NOTEON(const user_osc_param_t * const params) {
		note_on();
	}

	void NOTEOFF(const user_osc_param_t * const params)
	{
		note_off();
	}

	void CYCLE(const user_osc_param_t * const params,
		int32_t *yn,
		const uint32_t frames)
	{
		q31_t * __restrict y = (q31_t *)yn;
		uint8_t note = (params->pitch) >> 8;
		uint8_t mod = params->pitch & 0xFF;
		const float lfo = q31_to_f32(params->shape_lfo);
		const float cutoff = param_val_to_f32(params->cutoff);
		const float resonance = param_val_to_f32(params->resonance);

		const float f0 = osc_notehzf(note);
		const float f1 = osc_notehzf(note + 1);
		float hz = linintf(mod * k_note_mod_fscale, f0, f1);
		hz = clipmaxf(hz, k_note_max_hz);
		if (hz < 1)
			hz = 1.0f;

		for (int i = 0; i < frames; i++) {
			float sig = cycle(hz, lfo);
			y[i] = f32_to_q31(sig);
		}
	}

	void PARAM(uint16_t index, uint16_t value)
	{
		float valf = param_val_to_f32(value);

		switch (index) {
		case k_osc_param_shape:
			set_param_shape(valf);
			break;
		case k_osc_param_shiftshape:
			set_param_shiftshape(valf);
			break;
		case k_osc_param_id1:
			set_param_id1(valf);
			break;
		case k_osc_param_id2:
			set_param_id2(valf);
			break;
		case k_osc_param_id3:
			set_param_id3(valf);
			break;
		case k_osc_param_id4:
			set_param_id4(valf);
			break;
		case k_osc_param_id5:
			set_param_id5(valf);
			break;
		case k_osc_param_id6:
			set_param_id6(valf);
			break;
		default:
			break;
		}
	}

	void INIT(uint32_t platform, uint32_t api) {

	}
};

static MorphOsc s_osc;

void OSC_INIT(uint32_t platform, uint32_t api)
{
	s_osc.INIT(platform, api);
}

void OSC_CYCLE(const user_osc_param_t * const params,
	int32_t *yn,
	const uint32_t frames)
{
	s_osc.CYCLE(params, yn, frames);
}

void OSC_NOTEON(const user_osc_param_t * const params)
{
	s_osc.NOTEON(params);
}

void OSC_NOTEOFF(const user_osc_param_t * const params)
{
	s_osc.NOTEOFF(params);
}

void OSC_PARAM(uint16_t index, uint16_t value)
{
	s_osc.PARAM(index, value);
}
