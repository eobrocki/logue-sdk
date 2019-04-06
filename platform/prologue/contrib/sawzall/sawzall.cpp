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
 *
 * Super Saw Oscillator.
 * https://www.nada.kth.se/utbildning/grukth/exjobb/rapportlistor/2010/rapporter10/szabo_adam_10131.pdf
 * "How to Emulate the Super Saw" by Adam Szabo
 */

#include "userosc.h"

const float PWM_MIN = 0.00f;
const float PWM_MAX = 1.0f; // 0.95f;

const float TROUGH = -1.0f;
const float PEAK = 1.0f;
const float AMPLITUDE = PEAK - TROUGH;




const int NUM_SAWS = 7;
const int MIDDLE_SAW = 3;

const float DETUNE_OFFSETS[NUM_SAWS] = {
	-0.11002313,
	-0.06288439,
	-0.01952356,
	0.0,
	0.01991221,
	0.06216538,
	0.10745242
};


struct Saw {
	// Current frame counter
	float frame;

	Saw(void) {
		init();
	}

	void init(void) {
		frame = 0;
		frame = osc_white() * k_samplerate; // Simulate free running oscillator
		frame = fabs(frame);
	}

	void note_on(void) {
	}

	float sig(float hz) {
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
		float y = TROUGH + AMPLITUDE * x;

		frame++;

		return y;
		//return float(frame % 1000) / 1000.0; // For testing...
	}
};


struct SuperSaw {
	// Amount of detune, [0..1].
	float detune;
	// Amount of mix, [0..1]. 0 is the most volume to the detuned oscillators, 1 is the least.
	float mix;

	Saw saws[NUM_SAWS];

	SuperSaw(void) {
		init();
	}

	void init(void) {
		detune = 0.0f;
		mix = 0.0f;
	}

	void note_on(void) {

	}

	void set_detune(float value) {
		detune = clip01f(value);
	}

	void set_mix(float value) {
		mix = clip01f(value);
	}

	float sig(float hz, float lfo) {
		float tmp_detune = detune;
		tmp_detune = clip01f(tmp_detune);

		float tmp_mix = 1.0 - mix - lfo;
		tmp_mix = clip01f(tmp_mix);

		float ret = 0.0f;
		float normalize = 0.0f;
		for (int i = 0; i < NUM_SAWS; i++) {
			float saw_hz = hz * (1.0f + tmp_detune * DETUNE_OFFSETS[i]);

			float m = 1.0;
			if (i != MIDDLE_SAW)
				m = tmp_mix;
			normalize += m;

			float s = saws[i].sig(saw_hz);
			ret += m * s;
		}
		ret /= normalize;

		return ret;
	}
};

static SuperSaw s_state;

void OSC_INIT(uint32_t platform, uint32_t api)
{
	s_state.init();
}

void OSC_CYCLE(const user_osc_param_t * const params,
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
		float sig = s_state.sig(hz, lfo);
		y[i] = f32_to_q31(sig);
	}
}

void OSC_NOTEON(const user_osc_param_t * const params)
{
	s_state.note_on();
}

void OSC_NOTEOFF(const user_osc_param_t * const params)
{
}

void OSC_PARAM(uint16_t index, uint16_t value)
{
	float valf = param_val_to_f32(value);

	switch (index) {
	case k_osc_param_shape:
	{
		s_state.set_detune(valf);
		break;
	}
	case k_osc_param_shiftshape:
	{
		s_state.set_mix(valf);
		break;
	}
	case k_osc_param_id1:
	case k_osc_param_id2:
	case k_osc_param_id3:
	case k_osc_param_id4:
	case k_osc_param_id5:
	case k_osc_param_id6:
	default:
		break;
	}
}

