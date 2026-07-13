#pragma once

// GEMS vendoring note: verbatim extraction of Window/HanningWindow/HammingWindow
// from BogaudioModules src/dsp/analyzer.hpp + analyzer.cpp (commit 656eaae4),
// so that table.cpp does not drag in the full FFT analyzer. GPL-3.0 — see NOTICE.md.

#include <math.h>
#include <assert.h>

namespace bogaudio {
namespace dsp {

struct Window {
	int _size;
	float* _window;
	float _sum;

	Window(int size)
	: _size(size)
	, _window(new float[_size] {})
	, _sum(0.0)
	{}
	virtual ~Window() {
		delete[] _window;
	}

	inline int size() { return _size; }
	inline float at(int i) { assert(i >= 0 && i < _size); return _window[i]; }
	inline float sum() { return _sum; }
	void apply(float* in, float* out) {
		for (int i = 0; i < _size; ++i) {
			out[i] = in[i] * _window[i];
		}
	}
};

struct HanningWindow : Window {
	HanningWindow(int size, float alpha = 0.5) : Window(size) {
		const float invAlpha = 1.0 - alpha;
		const float twoPIEtc = 2.0 * M_PI / (float)size;
		for (int i = 0; i < _size; ++i) {
			_sum += _window[i] = invAlpha*cos(twoPIEtc*(float)i + M_PI) + alpha;
		}
	}
};

struct HammingWindow : HanningWindow {
	HammingWindow(int size) : HanningWindow(size, 0.54) {}
};

} // namespace dsp
} // namespace bogaudio
