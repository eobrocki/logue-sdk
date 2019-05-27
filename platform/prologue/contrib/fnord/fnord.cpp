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

*/

/*
 * FM baby!
 */


#include "fnord.h"

const float PWM_MIN = 0.00f;
const float PWM_MAX = 1.0f; // 0.95f;

const float TROUGH = -1.0f;
const float PEAK = 1.0f;
const float AMPLITUDE = PEAK - TROUGH;



struct EnvelopeStep {
	float num_frames;
	float dl_df; // Change in level per frame
	float start_level; // Level at beginning of envelope step [0..1]
	float end_level; // Level at end of envelope step [0..1]
	struct EnvelopeStep * next_step;

	void reset() {
		num_frames = 0;
		dl_df = 0;
		start_level = 0;
		end_level = 0;
		next_step = this;
	}

	bool is_ascending() {
		return (start_level < end_level);
	}
};

enum STEP_ENUM {
	E1 = 0,
	E2 = 1,
	E3 = 2,
	ESUSTAIN = 3,
	E4 = 4,
	NUM_ENVELOPE_STEPS = 5
};

struct Envelope {
	struct EnvelopeStep steps[NUM_ENVELOPE_STEPS];
	struct EnvelopeStep * currentStep;
	float frame;
	float level;

	void reset() {
		for (int i = 0; i < NUM_ENVELOPE_STEPS; i++) {
			steps[i].reset();
		}

		steps[E1].next_step = steps + E2;
		steps[E2].next_step = steps + E3;
		steps[E3].next_step = steps + ESUSTAIN;
		steps[ESUSTAIN].next_step = steps + ESUSTAIN;
		steps[E4].next_step = steps + E4;

		go_to_step(E1);
	}

	void go_to_step(enum STEP_ENUM s) {
		frame = 0;
		currentStep = steps + s;
		level = steps[s].start_level;
	}

	void note_on() {
		go_to_step(E1);
	}

	void note_off() {
		go_to_step(E4);
	}

	void increment_frame() {
		frame += 1;

		const float tiny = 0.0001f;
		if (level < tiny)
			level = tiny;

		level *= currentStep->dl_df;

		bool advance = false;
		if (frame >= currentStep->num_frames)
			advance = true;
		if (equalf(level, currentStep->end_level, 0.001f))
			advance = true;

		// Detect overshooting
		if (currentStep->is_ascending()) {
			if (level > currentStep->end_level)
				advance = true;
		}
		else {
			if (level < currentStep->end_level)
				advance = true;
		}


		if (advance && currentStep != steps + E4) {
			frame = 0;
			currentStep = currentStep->next_step;
// XXX			level = currentStep->start_level;
		}
	}


	float do_output() {
		float ret = level;
		increment_frame();
		return ret;
	}



	/* Return [0..1] from DX7 [0..99] level
	 */
	float dx7_scale_level(float dx7_level) {
		return dx7_scale(dx7_level);
	}

	float dx7_scale_rate(float r, float sl, float el) {
		int ri = dx7_clipi(r);
		float ret = DX7_RATES[ri];

		if (sl < el) {
			ret = 1 / ret;
		}

		return ret;
	}

	float dx7_rate_to_frames(float r, float sl, float el) {
		sl = dx7_scale(sl);
		el = dx7_scale(el);

		r = dx7_clipi(r);

		float ret = k_samplerate * 5;
		ret = 192.543832467932 * powf(0.893461267114903, r) * k_samplerate;

		return ret;
	}

	void set_dx7(const struct dx7_envelope &e) {
		set_dx7(e.rate[0], e.rate[1], e.rate[2], e.rate[3], e.level[0], e.level[1], e.level[2], e.level[3]);
	}

	/* Set DX7 style envelop.
	 */
	void set_dx7(float r1, float r2, float r3, float r4, float l1, float l2, float l3, float l4) {
		reset();

		// Set levels
		steps[E1].start_level = 0;
		steps[E2].start_level = steps[E1].end_level = dx7_scale_level(l1);
		steps[E3].start_level = steps[E2].end_level = dx7_scale_level(l2);
		steps[E4].start_level = steps[ESUSTAIN].end_level = steps[ESUSTAIN].start_level = steps[E3].end_level = dx7_scale_level(l3);
		steps[E4].end_level = 0; // Ignore l4, always set it to 0

		// Set rates
		steps[E1].dl_df = dx7_scale_rate(r1, steps[E1].start_level, steps[E1].end_level);
		steps[E2].dl_df = dx7_scale_rate(r2, steps[E2].start_level, steps[E2].end_level);
		steps[E3].dl_df = dx7_scale_rate(r3, steps[E3].start_level, steps[E3].end_level);
		steps[ESUSTAIN].dl_df = 1.0f;
		steps[E4].dl_df = dx7_scale_rate(r4, steps[E4].start_level, steps[E4].end_level);

		// Set frames
		steps[E1].num_frames = dx7_rate_to_frames(r1, steps[E1].start_level, steps[E1].end_level);
		steps[E2].num_frames = dx7_rate_to_frames(r2, steps[E2].start_level, steps[E2].end_level);
		steps[E3].num_frames = dx7_rate_to_frames(r3, steps[E3].start_level, steps[E3].end_level);
		steps[ESUSTAIN].num_frames = 1;
		steps[E4].num_frames = dx7_rate_to_frames(r4, steps[E4].start_level, steps[E4].end_level);
	}
};

class Operator {
public:
	float w; // Phase accumulator
	float r; // Frequency ratio
	struct Envelope e;

	void reset() {
		w = 0;
		r = 1;
		e.reset();

	}

	void note_on() {
		w = 0;
		e.note_on();
	}

	void note_off() {
		e.note_off();
	}

	float do_output(float hz, float lfo, float mod[NUM_OPS], float values[NUM_OPS]) {
		float el = e.do_output();

		float dw = r * hz * k_samplerate_recipf;
		w += dw;

		float tw = w;
		for (int i = 0; i < NUM_OPS; i++) {
			tw += mod[i] * values[i]; // *k_samplerate_recipf;
		}




		while (tw < 0)
			tw += 1;
		while (tw > 1)
			tw -= 1;
		tw = clip01f(tw);

		float ret = el * osc_sinf(tw);

		while (w < 0)
			w += 1;
		while (w > 1)
			w -= 1;
		w = clip01f(w);

		return ret;
	}


	/* Set DX7 style envelop.
	 */
	void set_dx7_e(float r1, float r2, float r3, float r4, float l1, float l2, float l3, float l4) {
		e.set_dx7(r1, r2, r3, r4, l1, l2, l3, l4);
	}

	void set_dx7(const struct dx7_op &o) {
		reset();
		r = o.freq_coarse;
		e.set_dx7(o.e);

	}
};

struct mod_matrix {
	float mm[/*carrier*/NUM_OPS][/*modulator*/NUM_OPS]; // Modulation matrix
	void reset() {
		for (int i = 0;i < NUM_OPS;i++) {
			for (int j = 0;j < NUM_OPS; j++) {
				mm[i][j] = 0;
			}
		}
	}
	void load(const struct dx7_mod_matrix *dmm) {
		for (int i = 0;i < NUM_OPS;i++) {
			for (int j = 0;j < NUM_OPS; j++) {
				mm[i][j] = dx7_scale(dmm->mm[i][j]);
			}
		}
	}
};

class FMOsc {
public:
	Operator ops[NUM_OPS];
	struct mod_matrix mm;
	int current_y_buf;
	float y_buf[2][NUM_OPS];
	float output_level[NUM_OPS];

	float shape;
	float shiftshape;

	int bank_num;
	int patch_num;

	void reset_y_buf() {
		current_y_buf = 0;
		memset(y_buf, 0, sizeof(y_buf));
	}

	void reset() {
		for (int i = 0; i < NUM_OPS; i++) {
			ops[i].reset();
		}

		mm.reset();

		reset_y_buf();

		shape = 0;
		shiftshape = 0;


		for (int i = 0; i < NUM_OPS; i++) {
			output_level[i] = 0;
		}

		set_param_id2(1);
	}

	void set_patch(const struct dx7_patch &p) {
		for (int i = 0; i < NUM_OPS; i++) {
			ops[i].set_dx7(p.ops[i]);

			output_level[i] = dx7_scale(p.output_level[i]);
		}

		mm.load(&p.mm);

	}

	void note_on(void) {
		reset_y_buf();
		for (int i = 0; i < NUM_OPS; i++) {
			ops[i].note_on();
		}
	}

	void note_off(void) {
		for (int i = 0; i < NUM_OPS; i++) {
			ops[i].note_off();
		}
	}

	void set_param_shape(float value) {
		shape = value;
	}

	void set_param_shiftshape(float value) {
		shiftshape = value;
	}

	void set_param_id1(int value) {
		bank_num = dx7_clipi(value);
	}

	void set_param_id2(int value) {
		patch_num = dx7_clipi(value);
		//const struct dx7_patch &p = DX7_PATCHES[patch_num]; // XXX - assume bank 0
		const struct dx7_patch &p = DX7_PATCHES[patch_num]; // XXX - assume bank 0
		set_patch(p);
	}

	void set_param_id3(float value) {
	}

	void set_param_id4(float value) {
	}

	void set_param_id5(float value) {
	}

	void set_param_id6(float value) {
	}

	float do_output(float hz, float lfo) {
		float ret = 0;
		for (int i = 0; i < NUM_OPS; i++) {
			float thz = hz;
#if 0
			for (int j = 0; j < NUM_OPS; j++) {
				thz += mm.mm[i][j] * y_buf[!current_y_buf][j];
			}
#endif

			float out = ops[i].do_output(thz, lfo, mm.mm[i], y_buf[!current_y_buf]);
			y_buf[current_y_buf][i] = out;

			ret += output_level[i] * out;

			//ops[i].increment_phase(hz, )
		}

		current_y_buf = !current_y_buf;

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
			float sig = do_output(hz, lfo);
			y[i] = f32_to_q31(sig);
		}
	}

	void PARAM(uint16_t index, uint16_t value)
	{
		float vf = param_val_to_f32(value);
		int v7 = dx7_clipi(value);


		switch (index) {
		case k_osc_param_shape:
			set_param_shape(vf);
			break;
		case k_osc_param_shiftshape:
			set_param_shiftshape(vf);
			break;
		case k_osc_param_id1: // Bank
			set_param_id1(v7);
			break;
		case k_osc_param_id2: // Patch
			set_param_id2(v7);
			break;
		case k_osc_param_id3:
			set_param_id3(vf);
			break;
		case k_osc_param_id4:
			set_param_id4(vf);
			break;
		case k_osc_param_id5:
			set_param_id5(vf);
			break;
		case k_osc_param_id6:
			set_param_id6(vf);
			break;
		default:
			break;
		}
	}

	void INIT(uint32_t platform, uint32_t api) {
		reset();
	}
};

static FMOsc s_osc;

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
