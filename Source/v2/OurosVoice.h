#pragma once

// GEMS v2 — CVfunk Ouros port (chunk 4.6). Transcribed 1:1 from
// codygeary/CVfunk-Modules src/Ouros.cpp (GPL-3.0, see NOTICE.md), mono channel,
// patch-wired inputs only: RATE (1V/oct from Quack), NODE CV, SPREAD CV.
// Patch instance #9698: rate 0, node 2.11446, rotate 0, spread 0, feedback 0.496386,
// multiply 1, position 157.879; node att 0.805334, spread att 0.658667. L output used.
// GEMS: rack::simd::sin (Cephes polynomial, ~1e-7 abs err) replaced by std::sin.

#include <cmath>

namespace gemsv2 {

struct OurosVoice {
	static constexpr float SEMITONE_TO_HZ = 261.625565f;

	// params (patch values as defaults)
	float rateKnob = 0.0f;
	float nodeKnob = 2.11446f;
	float rotateKnob = 0.0f;
	float spreadKnob = 0.0f;
	float feedbackKnob = 0.496386f;
	float multiplyKnob = 1.0f;
	float positionKnob = 157.879f;
	float nodeAtt = 0.805334f;
	float spreadAtt = 0.658667f;

	// state (single channel)
	float oscPhase[4] {};
	float place[4] {};
	float oscOutput[4] {};
	float lastoscPhase[4] {};
	float sampleTime = 1.0f / 48000.0f;

	float outL = 0.0f, outR = 0.0f;   // ±5 V

	static inline float fmodWrap (float x, float y) { return x - y * std::trunc (x / y); }
	static inline float wrap01 (float x) { return x - std::floor (x); }
	static inline float wrapPhaseDiff (float x) { return x - std::round (x); }
	static inline float lerp (float a, float b, float f) { return a + f * (b - a); }
	static inline float clampf_ (float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

	void setSampleRate (float sr) { sampleTime = 1.0f / sr; }

	void reset() {
		for (int i = 0; i < 4; ++i) { oscPhase[i] = place[i] = oscOutput[i] = lastoscPhase[i] = 0.0f; }
		outL = outR = 0.0f;
	}

	// rateCv: 1 V/oct (Quack out); nodeCv/spreadCv: volts (env CV taps)
	void process (float rateCv, float nodeCv, float spreadCv) {
		const float deltaTime = sampleTime;

		float multiply = multiplyKnob;
		multiply = clampf_ (multiply, 0.000001f, 10.0f);

		// nonlinear detent adjustment
		float baseMultiple = (float) (int) multiply;
		float remainder = multiply - baseMultiple;
		float remPow5 = remainder * remainder * remainder * remainder * remainder;
		float om = 1.0f - remainder;
		multiply = (remainder < 0.5f)
			? baseMultiple + remPow5
			: (baseMultiple + 1.0f) - (om * om * om * om * om);

		float rate = rateKnob;
		rate += rateCv;                       // RATE_INPUT connected in patch
		rate = SEMITONE_TO_HZ * std::exp2 (rate);

		float multi_rate = rate * multiply;

		float rotate = rotateKnob;            // ROTATE_INPUT unpatched

		float spread = spreadKnob;
		spread += spreadCv * 36.0f * spreadAtt;   // SPREAD_INPUT connected

		float eat = positionKnob;             // POSITION_INPUT unpatched

		float feedback = feedbackKnob;        // FEEDBACK_INPUT unpatched

		float NodePosition = nodeKnob;
		NodePosition += nodeCv * nodeAtt;     // NODE_INPUT connected

		NodePosition += feedback * oscOutput[3];
		NodePosition = fmodWrap (NodePosition, 5.0f);

		// reset logic: HARD_SYNC unpatched, no manual reset -> latch path inert

		for (int i = 0; i < 4; ++i) {
			float nodeOne = (rotate + spread / 2.0f) / 360.0f;
			float nodeTwo = (rotate - spread / 2.0f) / 360.0f;
			float nodeThree = eat / 360.0f;
			float currentNode = (i == 0) ? nodeOne : (i == 1) ? nodeTwo : (i == 3) ? nodeThree : 0.0f;

			float basePhase = currentNode;
			float targetPhase = basePhase;

			if (NodePosition < 1.0f) {
				targetPhase = lerp (basePhase, 0.5f, NodePosition);
			} else if (NodePosition < 2.0f) {
				float bimodalPhase = fmodWrap (currentNode, 2.0f) * 0.5f;
				float dynamicFactor = -1.0f * (NodePosition - 1.0f) * ((currentNode + 1.0f) * 0.5f);
				targetPhase = lerp (0.5f, bimodalPhase * dynamicFactor, NodePosition - 1.0f);
			} else if (NodePosition < 3.0f) {
				float bimodalPhase = fmodWrap (currentNode, 2.0f) * 0.5f;
				float dynamicFactor = -1.0f * (NodePosition - 1.0f) * ((currentNode + 1.0f) * 0.5f);
				float trimodalPhase = fmodWrap (currentNode, 3.0f) / 3.0f;
				float blendFactor = NodePosition - 2.0f;
				float adjustedTrimodalPhase = lerp (bimodalPhase * dynamicFactor, trimodalPhase, blendFactor);
				targetPhase = adjustedTrimodalPhase;
			} else if (NodePosition < 4.0f) {
				float trimodalPhase = fmodWrap (currentNode, 3.0f) / 3.0f;
				float blendFactor = NodePosition - 3.0f;
				targetPhase = lerp (trimodalPhase, 0.5f, blendFactor);
			} else {
				float blendFactor = NodePosition - 4.0f;
				targetPhase = lerp (0.5f, basePhase, blendFactor);
			}

			targetPhase += place[i];
			if (i == 2) targetPhase = place[i];
			targetPhase = wrap01 (targetPhase);

			float phaseDiff = wrapPhaseDiff (targetPhase - oscPhase[i]);
			oscPhase[i] += phaseDiff * 0.05f;

			if (i == 3) {
				oscPhase[i] += multi_rate * deltaTime;
				place[i] += multi_rate * deltaTime;
				if (oscPhase[2] == 0.0f) {
					oscPhase[3] = 0.0f;
					place[3] = 0.0f;
				}
			} else {
				oscPhase[i] += rate * deltaTime;
				place[i] += rate * deltaTime;
			}

			oscPhase[i] = wrap01 (oscPhase[i]);
			place[i] -= (place[i] >= 1.0f) ? 1.0f : 0.0f;
		}

		for (int i = 0; i < 4; ++i)
			oscOutput[i] = clampf_ (std::sin (oscPhase[i] * 2.0f * (float) M_PI) * 5.0f, -5.0f, 5.0f);

		outL = oscOutput[0];
		outR = oscOutput[1];

		lastoscPhase[2] = oscPhase[2];
		for (int i = 0; i < 4; ++i) {
			if (oscPhase[i] < lastoscPhase[i])
				lastoscPhase[i] = oscPhase[i];
		}
	}
};

} // namespace gemsv2
