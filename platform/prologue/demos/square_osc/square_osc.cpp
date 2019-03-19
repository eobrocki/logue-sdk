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
 * File: waves.cpp
 *
 * Morphing wavetable oscillator
 *
 */

#include "userosc.h"

const float PWM_MIN = 0.01;
const float PWM_MAX = 0.99;

struct Square {
	float pulsewidth;
	uint32_t frames;
	float lfo;

	Square(void) {
		init();
	}

	void init(void) {
		set_pulsewidth(0.5f);
		frames = 0;
		lfo = 0;
	}

	void note_on(void) {
		frames = 0;
	}

	void set_pulsewidth(float value) {
		pulsewidth = clipminmaxf(PWM_MIN, value, PWM_MAX);
	}

	void increment_frame() {
		frames++;
		if (frames >= k_samplerate)
			frames = 0;
	}

	float sig(uint8_t note, uint8_t mod, float lfo) {
		float ret = -1.0f;

		const float f0 = osc_notehzf(note);
		const float f1 = osc_notehzf(note + 1);
		float hz = clipmaxf(linintf(mod * k_note_mod_fscale, f0, f1), k_note_max_hz);
		if (hz < 1)
			hz = 1.0f;

		float frames_per_cycle = k_samplerate / hz;
		// Now we know when to reset the waveform
		if (frames >= frames_per_cycle)
			frames = 0;

		const float lfo_knob_scale = 0.5;
		float pwm = pulsewidth + lfo_knob_scale * lfo;
		pwm = clipminmaxf(PWM_MIN, pwm, PWM_MAX);
		const float frames_on = frames_per_cycle * pwm;
		if (frames < frames_on)
			ret = 1.0f;

		return ret;
	}
};

static Square s_square;

void OSC_INIT(uint32_t platform, uint32_t api)
{
	s_square.init();
}

void OSC_CYCLE(const user_osc_param_t * const params,
               int32_t *yn,
               const uint32_t frames)
{      

	q31_t * __restrict y = (q31_t *)yn;
	const q31_t * y_e = y + frames;
	uint8_t note = (params->pitch) >> 8;
	uint8_t mod = params->pitch & 0xFF;
	const float lfo = q31_to_f32(params->shape_lfo);
	const float cutoff = param_val_to_f32(params->cutoff);
	const float resonance = param_val_to_f32(params->resonance);

	for (; y != y_e; y++) {
		s_square.increment_frame();
		float sig = s_square.sig(note, mod, lfo);
		*y = f32_to_q31(sig);
	}
}

void OSC_NOTEON(const user_osc_param_t * const params)
{
	s_square.note_on();
}

void OSC_NOTEOFF(const user_osc_param_t * const params)
{
}

void OSC_PARAM(uint16_t index, uint16_t value)
{   
	
  switch (index) {
  case k_osc_param_shape:
  {
	  // pulsewidth
	  float valf = param_val_to_f32(value);
	  s_square.set_pulsewidth(valf);
	  break;
  }
  case k_osc_param_id1:
  case k_osc_param_id2:
  case k_osc_param_id3:
  case k_osc_param_id4:
  case k_osc_param_id5:
  case k_osc_param_id6:
  case k_osc_param_shiftshape:
  default:
    break;
  }
}

