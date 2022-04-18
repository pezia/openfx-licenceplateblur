#ifndef LICENCEPLATEPROCESSOR_H
#define LICENCEPLATEPROCESSOR_H

#include "LicencePlateProcessorBase.h"
#include "ofxsMacros.h"

using namespace OFX;

template <class PIX, int nComponents, int maxValue>
class LicencePlateProcessor
	: public LicencePlateProcessorBase
{
public:
	LicencePlateProcessor(ImageEffect& instance)
		: LicencePlateProcessorBase(instance)
	{
	}

private:

	void multiThreadProcessImages(const OfxRectI& procWindow, const OfxPointD& rs) OVERRIDE FINAL
	{
		const bool r = _processR && (nComponents != 1);
		const bool g = _processG && (nComponents >= 2);
		const bool b = _processB && (nComponents >= 3);
		const bool a = _processA && (nComponents == 1 || nComponents == 4);

		if (r) {
			if (g) {
				if (b) {
					if (a) {
						return process<true, true, true, true >(procWindow, rs); // RGBA
					}
					else {
						return process<true, true, true, false>(procWindow, rs); // RGBa
					}
				}
				else {
					if (a) {
						return process<true, true, false, true >(procWindow, rs); // RGbA
					}
					else {
						return process<true, true, false, false>(procWindow, rs); // RGba
					}
				}
			}
			else {
				if (b) {
					if (a) {
						return process<true, false, true, true >(procWindow, rs); // RgBA
					}
					else {
						return process<true, false, true, false>(procWindow, rs); // RgBa
					}
				}
				else {
					if (a) {
						return process<true, false, false, true >(procWindow, rs); // RgbA
					}
					else {
						return process<true, false, false, false>(procWindow, rs); // Rgba
					}
				}
			}
		}
		else {
			if (g) {
				if (b) {
					if (a) {
						return process<false, true, true, true >(procWindow, rs); // rGBA
					}
					else {
						return process<false, true, true, false>(procWindow, rs); // rGBa
					}
				}
				else {
					if (a) {
						return process<false, true, false, true >(procWindow, rs); // rGbA
					}
					else {
						return process<false, true, false, false>(procWindow, rs); // rGba
					}
				}
			}
			else {
				if (b) {
					if (a) {
						return process<false, false, true, true >(procWindow, rs); // rgBA
					}
					else {
						return process<false, false, true, false>(procWindow, rs); // rgBa
					}
				}
				else {
					if (a) {
						return process<false, false, false, true >(procWindow, rs); // rgbA
					}
					else {
						return process<false, false, false, false>(procWindow, rs); // rgba
					}
				}
			}
		}
	}

	template<bool processR, bool processG, bool processB, bool processA>
	void process(const OfxRectI& procWindow, const OfxPointD& rs)
	{
		unused(rs);
		assert(nComponents == 1 || nComponents == 3 || nComponents == 4);
		assert(_dstImg);
		float unpPix[4] = { 0.f, 0.f, 0.f, 0.f };
		float tmpPix[4] = { 0.f, 0.f, 0.f, 0.f };
		for (int y = procWindow.y1; y < procWindow.y2; y++) {
			if (_effect.abort()) {
				break;
			}

			PIX* dstPix = (PIX*)_dstImg->getPixelAddress(procWindow.x1, y);

			for (int x = procWindow.x1; x < procWindow.x2; x++) {
				const PIX* srcPix = (const PIX*)(_srcImg ? _srcImg->getPixelAddress(x, y) : 0);
				ofxsUnPremult<PIX, nComponents, maxValue>(srcPix, unpPix, _premult, _premultChannel);
				for (int c = 0; c < 4; ++c) {
					if (processR && (c == 0)) {
						tmpPix[0] = unpPix[0] + (float)_value.r;
					}
					else if (processG && (c == 1)) {
						tmpPix[1] = unpPix[1] + (float)_value.g;
					}
					else if (processB && (c == 2)) {
						tmpPix[2] = unpPix[2] + (float)_value.b;
					}
					else if (processA && (c == 3)) {
						tmpPix[3] = unpPix[3] + (float)_value.a;
					}
					else {
						tmpPix[c] = unpPix[c];
					}
				}
				ofxsPremultMaskMixPix<PIX, nComponents, maxValue, true>(tmpPix, _premult, _premultChannel, x, y, srcPix, _doMasking, _maskImg, (float)_mix, _maskInvert, dstPix);
				// copy back original values from unprocessed channels
				if (nComponents == 1) {
					if (!processA) {
						dstPix[0] = srcPix ? srcPix[0] : PIX();
					}
				}
				else if ((nComponents == 3) || (nComponents == 4)) {
					if (!processR) {
						dstPix[0] = srcPix ? srcPix[0] : PIX();
					}
					if (!processG) {
						dstPix[1] = srcPix ? srcPix[1] : PIX();
					}
					if (!processB) {
						dstPix[2] = srcPix ? srcPix[2] : PIX();
					}
					if (!processA && (nComponents == 4)) {
						dstPix[3] = srcPix ? srcPix[3] : PIX();
					}
				}
				// increment the dst pixel
				dstPix += nComponents;
			}
		}
	}
};

#endif // !LICENCEPLATEPROCESSOR_H
