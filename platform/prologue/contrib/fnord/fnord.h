
#include <stdint.h>

#include "userosc.h"

const int NUM_OPS = 3; // 6;


inline float myremainder(float a, float b) {
	float d = a / b;
	d = floorf(d);
	return a - (d * b);
}

inline bool equalf(float a, float b, float e) {
	return (fabsf(a - b) <= e);
}

// Return [0..99]
inline int dx7_clipi(int r) {
	if (r < 0)
		r = 0;
	else if (r > 99)
		r = 99;

	return r;
}

/* Return [0..1] from DX7 [0..99] level
 */
inline float dx7_scale(float dx7_level) {
	float ret = dx7_level / 99.0;
	ret = clip01f(ret);
	return ret;
}

struct dx7_envelope {
	uint8_t rate[4];
	uint8_t level[4];
};

enum ratio_mode {
	FREQ_FIXED = 0,
	FREQ_RATIO = 1
};

struct dx7_op {
	struct dx7_envelope e;
	uint8_t level;
	uint8_t ratio_mode; // ratio_mode: 0 = fixed, 1 = ratio
	uint8_t freq_coarse; // [0..31]
	uint8_t freq_fine; // [0-99]
	uint8_t detune; // [0-14], 0 means -7 detune
};

struct dx7_mod_matrix {
	uint8_t mm[/*carrier*/NUM_OPS][/*modulator*/NUM_OPS]; // Modulation matrix
	void reset() {
		for (int i = 0;i < NUM_OPS;i++) {
			for (int j = 0;j < NUM_OPS; j++) {
				mm[i][j] = 0;
			}
		}
	}

};



struct dx7_patch {
	struct dx7_op ops[NUM_OPS];
	struct dx7_mod_matrix mm;
	uint8_t output_level[NUM_OPS];
};


const int NUM_PATCHES = 100;
extern const struct dx7_patch DX7_PATCHES[NUM_PATCHES];

const float DX7_RATES[100] = {
0.9999980835,
0.9999978521,
0.9999975927,
0.999997302,
0.9999969763,
0.9999966111,
0.9999962019,
0.9999957433,
0.9999952293,
0.9999946532,
0.9999940076,
0.999993284,
0.999992473,
0.9999915641,
0.9999905455,
0.9999894038,
0.9999881243,
0.9999866903,
0.9999850831,
0.9999832819,
0.9999812631,
0.9999790006,
0.9999764649,
0.999973623,
0.9999704379,
0.9999668682,
0.9999628675,
0.9999583836,
0.9999533584,
0.9999477263,
0.9999414141,
0.9999343398,
0.9999264112,
0.9999175252,
0.9999075661,
0.9998964046,
0.9998838952,
0.9998698753,
0.9998541624,
0.9998365522,
0.9998168156,
0.9997946957,
0.9997699047,
0.9997421203,
0.9997109807,
0.999676081,
0.9996369671,
0.9995931301,
0.9995439997,
0.9994889367,
0.9994272248,
0.9993580609,
0.9992805454,
0.9991936698,
0.9990963037,
0.9989871804,
0.9988648803,
0.9987278122,
0.9985741928,
0.9984020236,
0.9982090645,
0.9979928053,
0.9977504322,
0.9974787922,
0.9971743509,
0.9968331478,
0.9964507437,
0.9960221634,
0.9955418312,
0.9950034979,
0.9944001596,
0.993723967,
0.9929661227,
0.9921167671,
0.9911648499,
0.9900979868,
0.9889022976,
0.9875622265,
0.9860603389,
0.9843770952,
0.9824905963,
0.9803762986,
0.9780066949,
0.9753509566,
0.9723745323,
0.9690386983,
0.9653000554,
0.9611099633,
0.9564139087,
0.9511507956,
0.9452521504,
0.9386412312,
0.9312320295,
0.9229281509,
0.9136215614,
0.9031911814,
0.8915013108,
0.8783998636,
0.8637163887,
0.8472598531,
};