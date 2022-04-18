#ifndef LICENCEPLATEPROCESSORBASE_H
#define LICENCEPLATEPROCESSORBASE_H

#include "RGBAValues.h"
#include "ofxsProcessing.H"

using namespace OFX;

class LicencePlateProcessorBase
	: public ImageProcessor
{
protected:
	const Image* _srcImg;
	const Image* _maskImg;
	bool _processR;
	bool _processG;
	bool _processB;
	bool _processA;
	RGBAValues _value;
	bool _premult;
	int _premultChannel;
	bool _doMasking;
	double _mix;
	bool _maskInvert;

public:

	LicencePlateProcessorBase(ImageEffect& instance)
		: ImageProcessor(instance)
		, _srcImg(nullptr)
		, _maskImg(nullptr)
		, _processR(true)
		, _processG(true)
		, _processB(true)
		, _processA(false)
		, _value()
		, _premult(false)
		, _premultChannel(3)
		, _doMasking(false)
		, _mix(1.)
		, _maskInvert(false)
	{
	}

	void setSrcImg(const Image* v)
	{
		_srcImg = v;
	}

	void setMaskImg(const Image* v, bool maskInvert)
	{
		_maskImg = v; _maskInvert = maskInvert;
	}

	void doMasking(bool v) {
		_doMasking = v;
	}

	void setValues(bool processR,
		bool processG,
		bool processB,
		bool processA,
		const RGBAValues& value,
		bool premult,
		int premultChannel,
		double mix)
	{
		_processR = processR;
		_processG = processG;
		_processB = processB;
		_processA = processA;
		_value = value;
		_premult = premult;
		_premultChannel = premultChannel;
		_mix = mix;
	}

private:
};

#endif // !LICENCEPLATEPROCESSORBASE_H