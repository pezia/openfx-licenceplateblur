﻿/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of openfx-misc <https://github.com/NatronGitHub/openfx-misc>,
 * (C) 2018-2021 The Natron Developers
 * (C) 2013-2018 INRIA
 *
 * openfx-misc is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * openfx-misc is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with openfx-misc.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
 * ***** END LICENSE BLOCK ***** */

 /*
  * OFX Licence plate blur plugin.
  */

#include <cmath>
#include <cfloat> // DBL_MAX    
#include <cstring>

#include "LicencePlateProcessorBase.h"
#include "LicencePlateProcessor.h"

#include "ofxsProcessing.H"
#include "ofxsMaskMix.h"
#include "ofxsCoords.h"
#include "ofxsMacros.h"
#ifdef OFX_EXTENSIONS_NATRON
#include "ofxNatron.h"
#endif
#include "ofxsThreadSuite.h"
#include "RGBAValues.h"

using namespace OFX;

OFXS_NAMESPACE_ANONYMOUS_ENTER

#define kPluginName "LicecenceplateBlur"
#define kPluginGrouping "Pezia/Filters"
#define kPluginDescription \
    "Blurs licence plates on the image."

#define STRINGIZE_CPP_NAME_(token) # token
#define STRINGIZE_CPP_(token) STRINGIZE_CPP_NAME_(token)

#ifdef DEBUG
#define kPluginDescriptionDebug " with debug"
#else
#define kPluginDescriptionDebug " without debug"
#endif

#ifdef NDEBUG
#define kPluginDescriptionNDebug ", without assertions"
#else
#define kPluginDescriptionNDebug ", with assertions"
#endif

#if defined(__OPTIMIZE__)
#define kPluginDescriptionOptimize ", with optimizations"
#else
#define kPluginDescriptionOptimize "" // don't know (maybe not gcc or clang)
#endif

#if defined(__NO_INLINE__)
#define kPluginDescriptionInline ", without inlines"
#else
#define kPluginDescriptionInline "" // don't know (maybe not gcc or clang)
#endif

#ifdef cimg_use_openmp
#define kPluginDescriptionOpenMP ", with OpenMP " STRINGIZE_CPP_(_OPENMP)
#else
#define kPluginDescriptionOpenMP ", without OpenMP"
#endif

#ifdef __clang__
#define kPluginDescriptionCompiler ", using Clang version " __clang_version__
#else
#ifdef __GNUC__
#define kPluginDescriptionCompiler ", using GNU C++ version " __VERSION__
#else
#define kPluginDescriptionCompiler ""
#endif
#endif

#define kPluginDescriptionFull kPluginDescription "\n\nThis plugin was compiled " kPluginDescriptionDebug kPluginDescriptionNDebug kPluginDescriptionOptimize kPluginDescriptionInline kPluginDescriptionOpenMP kPluginDescriptionCompiler "."

#define kPluginIdentifier "hu.pezia.openfx.LicenceplateBlur"

// History:
// version 1.0: initial version
#define kPluginVersionMajor 1 // Incrementing this number means that you have broken backwards compatibility of the plug-in.
#define kPluginVersionMinor 0 // Increment this when you have fixed a bug or made it faster.

#define kSupportsTiles 1
#define kSupportsMultiResolution 1
#define kSupportsRenderScale 1
#define kSupportsMultipleClipPARs false
#define kSupportsMultipleClipDepths false
#define kRenderThreadSafety eRenderFullySafe

#ifdef OFX_EXTENSIONS_NATRON
#define kParamProcessR kNatronOfxParamProcessR
#define kParamProcessRLabel kNatronOfxParamProcessRLabel
#define kParamProcessRHint kNatronOfxParamProcessRHint
#define kParamProcessG kNatronOfxParamProcessG
#define kParamProcessGLabel kNatronOfxParamProcessGLabel
#define kParamProcessGHint kNatronOfxParamProcessGHint
#define kParamProcessB kNatronOfxParamProcessB
#define kParamProcessBLabel kNatronOfxParamProcessBLabel
#define kParamProcessBHint kNatronOfxParamProcessBHint
#define kParamProcessA kNatronOfxParamProcessA
#define kParamProcessALabel kNatronOfxParamProcessALabel
#define kParamProcessAHint kNatronOfxParamProcessAHint
#else
#define kParamProcessR      "processR"
#define kParamProcessRLabel "R"
#define kParamProcessRHint  "Process red channel."
#define kParamProcessG      "processG"
#define kParamProcessGLabel "G"
#define kParamProcessGHint  "Process green channel."
#define kParamProcessB      "processB"
#define kParamProcessBLabel "B"
#define kParamProcessBHint  "Process blue channel."
#define kParamProcessA      "processA"
#define kParamProcessALabel "A"
#define kParamProcessAHint  "Process alpha channel."
#endif

#define kParamValueName  "value"
#define kParamValueLabel "Value"
#define kParamValueHint  "Constant to add to the selected channels."

#define kParamPremultChanged "premultChanged"

#ifdef OFX_EXTENSIONS_NATRON
#define OFX_COMPONENTS_OK(c) ((c)== ePixelComponentAlpha || (c) == ePixelComponentXY || (c) == ePixelComponentRGB || (c) == ePixelComponentRGBA)
#else
#define OFX_COMPONENTS_OK(c) ((c)== ePixelComponentAlpha || (c) == ePixelComponentRGB || (c) == ePixelComponentRGBA)
#endif


////////////////////////////////////////////////////////////////////////////////
/** @brief The plugin that does our work */
class LicencePlateBlurPlugin
	: public ImageEffect
{
public:
	/** @brief ctor */
	LicencePlateBlurPlugin(OfxImageEffectHandle handle)
		: ImageEffect(handle)
		, _dstClip(NULL)
		, _srcClip(NULL)
		, _maskClip(NULL)
		, _processR(NULL)
		, _processG(NULL)
		, _processB(NULL)
		, _processA(NULL)
		, _value(NULL)
		, _premult(NULL)
		, _premultChannel(NULL)
		, _mix(NULL)
		, _maskApply(NULL)
		, _maskInvert(NULL)
		, _premultChanged(NULL)
	{

		_dstClip = fetchClip(kOfxImageEffectOutputClipName);
		assert(_dstClip && (!_dstClip->isConnected() || OFX_COMPONENTS_OK(_dstClip->getPixelComponents())));
		_srcClip = getContext() == eContextGenerator ? NULL : fetchClip(kOfxImageEffectSimpleSourceClipName);
		assert((!_srcClip && getContext() == eContextGenerator) ||
			(_srcClip && (!_srcClip->isConnected() || OFX_COMPONENTS_OK(_srcClip->getPixelComponents()))));
		_maskClip = fetchClip(getContext() == eContextPaint ? "Brush" : "Mask");
		assert(!_maskClip || !_maskClip->isConnected() || _maskClip->getPixelComponents() == ePixelComponentAlpha);
		_processR = fetchBooleanParam(kParamProcessR);
		_processG = fetchBooleanParam(kParamProcessG);
		_processB = fetchBooleanParam(kParamProcessB);
		_processA = fetchBooleanParam(kParamProcessA);
		assert(_processR && _processG && _processB && _processA);
		_value = fetchRGBAParam(kParamValueName);
		assert(_value);
		_premult = fetchBooleanParam(kParamPremult);
		_premultChannel = fetchChoiceParam(kParamPremultChannel);
		assert(_premult && _premultChannel);
		_mix = fetchDoubleParam(kParamMix);
		_maskApply = (ofxsMaskIsAlwaysConnected(OFX::getImageEffectHostDescription()) && paramExists(kParamMaskApply)) ? fetchBooleanParam(kParamMaskApply) : 0;
		_maskInvert = fetchBooleanParam(kParamMaskInvert);
		assert(_mix && _maskInvert);
		_premultChanged = fetchBooleanParam(kParamPremultChanged);
		assert(_premultChanged);
	}

private:
	/* Override the render */
	virtual void render(const RenderArguments& args) OVERRIDE FINAL;

	template <int nComponents>
	void renderInternal(const RenderArguments& args, BitDepthEnum dstBitDepth);

	/* set up and run a processor */
	void setupAndProcess(LicencePlateProcessorBase&, const RenderArguments& args);

	virtual bool isIdentity(const IsIdentityArguments& args, Clip*& identityClip, double& identityTime, int& view, std::string& plane) OVERRIDE FINAL;

	/** @brief called when a clip has just been changed in some way (a rewire maybe) */
	virtual void changedClip(const InstanceChangedArgs& args, const std::string& clipName) OVERRIDE FINAL;
	virtual void changedParam(const InstanceChangedArgs& args, const std::string& paramName) OVERRIDE FINAL;

private:
	// do not need to delete these, the ImageEffect is managing them for us
	Clip* _dstClip;
	Clip* _srcClip;
	Clip* _maskClip;
	BooleanParam* _processR;
	BooleanParam* _processG;
	BooleanParam* _processB;
	BooleanParam* _processA;
	RGBAParam* _value;
	BooleanParam* _premult;
	ChoiceParam* _premultChannel;
	DoubleParam* _mix;
	BooleanParam* _maskApply;
	BooleanParam* _maskInvert;
	BooleanParam* _premultChanged; // set to true the first time the user connects src
};


////////////////////////////////////////////////////////////////////////////////
/** @brief render for the filter */

////////////////////////////////////////////////////////////////////////////////
// basic plugin render function, just a skelington to instantiate templates from

/* set up and run a processor */
void LicencePlateBlurPlugin::setupAndProcess(LicencePlateProcessorBase& processor, const RenderArguments& args)
{
	auto_ptr<Image> dst(_dstClip->fetchImage(args.time));

	if (!dst.get()) {
		throwSuiteStatusException(kOfxStatFailed);
	}
# ifndef NDEBUG
	BitDepthEnum dstBitDepth = dst->getPixelDepth();
	PixelComponentEnum dstComponents = dst->getPixelComponents();
	if ((dstBitDepth != _dstClip->getPixelDepth()) ||
		(dstComponents != _dstClip->getPixelComponents())) {
		setPersistentMessage(Message::eMessageError, "", "OFX Host gave image with wrong depth or components");
		throwSuiteStatusException(kOfxStatFailed);
	}
	checkBadRenderScaleOrField(dst, args);
# endif
	auto_ptr<const Image> src((_srcClip && _srcClip->isConnected()) ?
		_srcClip->fetchImage(args.time) : 0);
# ifndef NDEBUG
	if (src.get()) {
		checkBadRenderScaleOrField(src, args);
		BitDepthEnum srcBitDepth = src->getPixelDepth();
		PixelComponentEnum srcComponents = src->getPixelComponents();
		if ((srcBitDepth != dstBitDepth) || (srcComponents != dstComponents)) {
			throwSuiteStatusException(kOfxStatErrImageFormat);
		}
	}
# endif
	bool doMasking = ((!_maskApply || _maskApply->getValueAtTime(args.time)) && _maskClip && _maskClip->isConnected());
	auto_ptr<const Image> mask(doMasking ? _maskClip->fetchImage(args.time) : 0);
	// do we do masking
	if (doMasking) {
		bool maskInvert;
		_maskInvert->getValueAtTime(args.time, maskInvert);
		processor.doMasking(true);
		processor.setMaskImg(mask.get(), maskInvert);
	}

	// set the images
	processor.setDstImg(dst.get());
	processor.setSrcImg(src.get());
	// set the render window
	processor.setRenderWindow(args.renderWindow, args.renderScale);

	bool processR, processG, processB, processA;
	_processR->getValueAtTime(args.time, processR);
	_processG->getValueAtTime(args.time, processG);
	_processB->getValueAtTime(args.time, processB);
	_processA->getValueAtTime(args.time, processA);
	RGBAValues value;
	_value->getValueAtTime(args.time, value.r, value.g, value.b, value.a);
	bool premult;
	int premultChannel;
	_premult->getValueAtTime(args.time, premult);
	_premultChannel->getValueAtTime(args.time, premultChannel);
	double mix;
	_mix->getValueAtTime(args.time, mix);
	processor.setValues(processR, processG, processB, processA,
		value, premult, premultChannel, mix);

	// Call the base class process member, this will call the derived templated process code
	processor.process();
}

// the internal render function
template <int nComponents>
void LicencePlateBlurPlugin::renderInternal(const RenderArguments& args, BitDepthEnum dstBitDepth)
{
	switch (dstBitDepth) {
	case eBitDepthUByte: {
		LicencePlateProcessor<unsigned char, nComponents, 255> fred(*this);
		setupAndProcess(fred, args);
		break;
	}
	case eBitDepthUShort: {
		LicencePlateProcessor<unsigned short, nComponents, 65536> fred(*this);
		setupAndProcess(fred, args);
		break;
	}
	case eBitDepthFloat: {
		LicencePlateProcessor<float, nComponents, 1> fred(*this);
		setupAndProcess(fred, args);
		break;
	}
	default:
		throwSuiteStatusException(kOfxStatErrUnsupported);
	}
}

// the overridden render function
void LicencePlateBlurPlugin::render(const RenderArguments& args) {
	// instantiate the render code based on the pixel depth of the dst clip
	BitDepthEnum dstBitDepth = _dstClip->getPixelDepth();
	PixelComponentEnum dstComponents = _dstClip->getPixelComponents();

	assert(kSupportsMultipleClipPARs || !_srcClip || !_srcClip->isConnected() || _srcClip->getPixelAspectRatio() == _dstClip->getPixelAspectRatio());
	assert(kSupportsMultipleClipDepths || !_srcClip || !_srcClip->isConnected() || _srcClip->getPixelDepth() == _dstClip->getPixelDepth());
#ifdef OFX_EXTENSIONS_NATRON
	assert(dstComponents == ePixelComponentRGBA || dstComponents == ePixelComponentRGB || dstComponents == ePixelComponentXY || dstComponents == ePixelComponentAlpha);
#else
	assert(dstComponents == ePixelComponentRGBA || dstComponents == ePixelComponentRGB || dstComponents == ePixelComponentAlpha);
#endif
	if (dstComponents == ePixelComponentRGBA) {
		renderInternal<4>(args, dstBitDepth);
	}
	else if (dstComponents == ePixelComponentRGB) {
		renderInternal<3>(args, dstBitDepth);
#ifdef OFX_EXTENSIONS_NATRON
	}
	else if (dstComponents == ePixelComponentXY) {
		renderInternal<2>(args, dstBitDepth);
#endif
	}
	else {
		assert(dstComponents == ePixelComponentAlpha);
		renderInternal<1>(args, dstBitDepth);
	}
}

bool LicencePlateBlurPlugin::isIdentity(const IsIdentityArguments& args, Clip*& identityClip,
	double& /*identityTime*/
	, int& /*view*/, std::string& /*plane*/)
{
	double mix;

	_mix->getValueAtTime(args.time, mix);

	if (mix == 0. /*|| (!processR && !processG && !processB && !processA)*/) {
		identityClip = _srcClip;

		return true;
	}

	{
		bool processR, processG, processB, processA;
		_processR->getValueAtTime(args.time, processR);
		_processG->getValueAtTime(args.time, processG);
		_processB->getValueAtTime(args.time, processB);
		_processA->getValueAtTime(args.time, processA);
		RGBAValues value;
		_value->getValueAtTime(args.time, value.r, value.g, value.b, value.a);
		if ((!processR || (value.r == 0.)) &&
			(!processG || (value.g == 0.)) &&
			(!processB || (value.b == 0.)) &&
			(!processA || (value.a == 0.))) {
			identityClip = _srcClip;

			return true;
		}
	}

	bool doMasking = ((!_maskApply || _maskApply->getValueAtTime(args.time)) && _maskClip && _maskClip->isConnected());
	if (doMasking) {
		bool maskInvert;
		_maskInvert->getValueAtTime(args.time, maskInvert);
		if (!maskInvert) {
			OfxRectI maskRoD;
			if (getImageEffectHostDescription()->supportsMultiResolution) {
				// In Sony Catalyst Edit, clipGetRegionOfDefinition returns the RoD in pixels instead of canonical coordinates.
				// In hosts that do not support multiResolution (e.g. Sony Catalyst Edit), all inputs have the same RoD anyway.
				Coords::toPixelEnclosing(_maskClip->getRegionOfDefinition(args.time), args.renderScale, _maskClip->getPixelAspectRatio(), &maskRoD);
				// effect is identity if the renderWindow doesn't intersect the mask RoD
				if (!Coords::rectIntersection<OfxRectI>(args.renderWindow, maskRoD, 0)) {
					identityClip = _srcClip;

					return true;
				}
			}
		}
	}

	return false;
}

void LicencePlateBlurPlugin::changedClip(const InstanceChangedArgs& args, const std::string& clipName)
{
	if ((clipName == kOfxImageEffectSimpleSourceClipName) &&
		_srcClip && _srcClip->isConnected() &&
		!_premultChanged->getValue() &&
		(args.reason == eChangeUserEdit)) {
		if (_srcClip->getPixelComponents() != ePixelComponentRGBA) {
			_premult->setValue(false);
		}
		else {
			switch (_srcClip->getPreMultiplication()) {
			case eImageOpaque:
				_premult->setValue(false);
				break;
			case eImagePreMultiplied:
				_premult->setValue(true);
				break;
			case eImageUnPreMultiplied:
				_premult->setValue(false);
				break;
			}
		}
	}
}

void LicencePlateBlurPlugin::changedParam(const InstanceChangedArgs& args, const std::string& paramName)
{
	if ((paramName == kParamPremult) && (args.reason == eChangeUserEdit)) {
		_premultChanged->setValue(true);
	}
}

mDeclarePluginFactory(LicencePlateBlurPluginFactory, { ofxsThreadSuiteCheck(); }, {});
void LicencePlateBlurPluginFactory::describe(ImageEffectDescriptor& desc)
{
	// basic labels
	desc.setLabel(kPluginName);
	desc.setPluginGrouping(kPluginGrouping);
	desc.setPluginDescription(kPluginDescription);

	desc.addSupportedContext(eContextFilter);
	desc.addSupportedContext(eContextGeneral);
	desc.addSupportedContext(eContextPaint);
	desc.addSupportedBitDepth(eBitDepthUByte);
	desc.addSupportedBitDepth(eBitDepthUShort);
	desc.addSupportedBitDepth(eBitDepthFloat);

	// set a few flags
	desc.setSingleInstance(false);
	desc.setHostFrameThreading(false);
	desc.setSupportsMultiResolution(kSupportsMultiResolution);
	desc.setSupportsTiles(kSupportsTiles);
	desc.setTemporalClipAccess(false);
	desc.setRenderTwiceAlways(false);
	desc.setSupportsMultipleClipPARs(kSupportsMultipleClipPARs);
	desc.setSupportsMultipleClipDepths(kSupportsMultipleClipDepths);
	desc.setRenderThreadSafety(kRenderThreadSafety);

#ifdef OFX_EXTENSIONS_NATRON
	desc.setChannelSelector(ePixelComponentNone); // we have our own channel selector
#endif
}

void LicencePlateBlurPluginFactory::describeInContext(ImageEffectDescriptor& desc, ContextEnum context)
{
	// Source clip only in the filter context
	// create the mandated source clip
	ClipDescriptor* srcClip = desc.defineClip(kOfxImageEffectSimpleSourceClipName);

	srcClip->addSupportedComponent(ePixelComponentRGBA);
	srcClip->addSupportedComponent(ePixelComponentRGB);
	srcClip->addSupportedComponent(ePixelComponentAlpha);
	srcClip->setTemporalClipAccess(false);
	srcClip->setSupportsTiles(kSupportsTiles);
	srcClip->setIsMask(false);

	// create the mandated output clip
	ClipDescriptor* dstClip = desc.defineClip(kOfxImageEffectOutputClipName);
	dstClip->addSupportedComponent(ePixelComponentRGBA);
	dstClip->addSupportedComponent(ePixelComponentRGB);
	dstClip->addSupportedComponent(ePixelComponentAlpha);
	dstClip->setSupportsTiles(kSupportsTiles);

	ClipDescriptor* maskClip = (context == eContextPaint) ? desc.defineClip("Brush") : desc.defineClip("Mask");
	maskClip->addSupportedComponent(ePixelComponentAlpha);
	maskClip->setTemporalClipAccess(false);
	if (context != eContextPaint) {
		maskClip->setOptional(true);
	}
	maskClip->setSupportsTiles(kSupportsTiles);
	maskClip->setIsMask(true);

	// make some pages and to things in
	PageParamDescriptor* page = desc.definePageParam("Controls");

	{
		BooleanParamDescriptor* param = desc.defineBooleanParam(kParamProcessR);
		param->setLabel(kParamProcessRLabel);
		param->setHint(kParamProcessRHint);
		param->setDefault(true);
		param->setLayoutHint(eLayoutHintNoNewLine, 1);
		if (page) {
			page->addChild(*param);
		}
	}
	{
		BooleanParamDescriptor* param = desc.defineBooleanParam(kParamProcessG);
		param->setLabel(kParamProcessGLabel);
		param->setHint(kParamProcessGHint);
		param->setDefault(true);
		param->setLayoutHint(eLayoutHintNoNewLine, 1);
		if (page) {
			page->addChild(*param);
		}
	}
	{
		BooleanParamDescriptor* param = desc.defineBooleanParam(kParamProcessB);
		param->setLabel(kParamProcessBLabel);
		param->setHint(kParamProcessBHint);
		param->setDefault(true);
		param->setLayoutHint(eLayoutHintNoNewLine, 1);
		if (page) {
			page->addChild(*param);
		}
	}
	{
		BooleanParamDescriptor* param = desc.defineBooleanParam(kParamProcessA);
		param->setLabel(kParamProcessALabel);
		param->setHint(kParamProcessAHint);
		param->setDefault(false);
		if (page) {
			page->addChild(*param);
		}
	}

	{
		RGBAParamDescriptor* param = desc.defineRGBAParam(kParamValueName);
		param->setLabel(kParamValueLabel);
		param->setHint(kParamValueHint);
		param->setDefault(0.0, 0.0, 0.0, 0.0);
		param->setRange(-DBL_MAX, -DBL_MAX, -DBL_MAX, -DBL_MAX, DBL_MAX, DBL_MAX, DBL_MAX, DBL_MAX); // Resolve requires range and display range or values are clamped to (-1,1)
		param->setDisplayRange(0, 0, 0, 0, 4, 4, 4, 4);
		param->setAnimates(true); // can animate
		if (page) {
			page->addChild(*param);
		}
	}

	ofxsPremultDescribeParams(desc, page);
	ofxsMaskMixDescribeParams(desc, page);

	{
		BooleanParamDescriptor* param = desc.defineBooleanParam(kParamPremultChanged);
		param->setDefault(false);
		param->setIsSecretAndDisabled(true);
		param->setAnimates(false);
		param->setEvaluateOnChange(false);
		if (page) {
			page->addChild(*param);
		}
	}
}

ImageEffect* LicencePlateBlurPluginFactory::createInstance(OfxImageEffectHandle handle, ContextEnum /*context*/)
{
	return new LicencePlateBlurPlugin(handle);
}

static LicencePlateBlurPluginFactory p(kPluginIdentifier, kPluginVersionMajor, kPluginVersionMinor);
mRegisterPluginFactoryInstance(p)

OFXS_NAMESPACE_ANONYMOUS_EXIT
