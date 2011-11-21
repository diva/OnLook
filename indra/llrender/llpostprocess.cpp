/** 
 * @file llpostprocess.cpp
 * @brief LLPostProcess class implementation
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#include "linden_common.h"

#include "llpostprocess.h"
#include "llglslshader.h"
#include "llsdserialize.h"
#include "llrender.h"

#include "lldir.h"
extern LLGLSLShader			gPostColorFilterProgram;
extern LLGLSLShader			gPostNightVisionProgram;
extern LLGLSLShader			gPostGaussianBlurProgram;

LLPostProcess * gPostProcess = NULL;


static const unsigned int NOISE_SIZE = 512;

/// CALCULATING LUMINANCE (Using NTSC lum weights)
/// http://en.wikipedia.org/wiki/Luma_%28video%29
static const float LUMINANCE_R = 0.299f;
static const float LUMINANCE_G = 0.587f;
static const float LUMINANCE_B = 0.114f;

static const char * const XML_FILENAME = "postprocesseffects.xml";

LLPostProcess::LLPostProcess(void) : 
					initialized(false),  
					mAllEffects(LLSD::emptyMap()),
					screenW(1), screenH(1)
{
	mSceneRenderTexture = NULL ; 
	mNoiseTexture = NULL ;
	mTempBloomTexture = NULL ;
					
	/*  Do nothing.  Needs to be updated to use our current shader system, and to work with the move into llrender.*/
	std::string pathName(gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, "windlight", XML_FILENAME));
	LL_DEBUGS2("AppInit", "Shaders") << "Loading PostProcess Effects settings from " << pathName << LL_ENDL;

	llifstream effectsXML(pathName);

	if (effectsXML)
	{
		LLPointer<LLSDParser> parser = new LLSDXMLParser();

		parser->parse(effectsXML, mAllEffects, LLSDSerialize::SIZE_UNLIMITED);
	}

	if (!mAllEffects.has("default"))
	{
		LLSD & defaultEffect = (mAllEffects["default"] = LLSD::emptyMap());

		/*defaultEffect["enable_night_vision"] = LLSD::Boolean(false);
		defaultEffect["enable_bloom"] = LLSD::Boolean(false);
		defaultEffect["enable_color_filter"] = LLSD::Boolean(false);*/

		/// NVG Defaults
		defaultEffect["brightness_multiplier"] = 3.0;
		defaultEffect["noise_size"] = 25.0;
		defaultEffect["noise_strength"] = 0.4;

		// TODO BTest potentially add this to tweaks?
		noiseTextureScale = 1.0f;
		
		/// Bloom Defaults
		defaultEffect["extract_low"] = 0.95;
		defaultEffect["extract_high"] = 1.0;
		defaultEffect["bloom_width"] = 2.25;
		defaultEffect["bloom_strength"] = 1.5;

		/// Color Filter Defaults
		defaultEffect["gamma"] = 1.0;
		defaultEffect["brightness"] = 1.0;
		defaultEffect["contrast"] = 1.0;
		defaultEffect["saturation"] = 1.0;

		LLSD& contrastBase = (defaultEffect["contrast_base"] = LLSD::emptyArray());
		contrastBase.append(1.0);
		contrastBase.append(1.0);
		contrastBase.append(1.0);
		contrastBase.append(0.5);

		defaultEffect["gauss_blur_passes"] = 2;
	}

	setSelectedEffect("default");
	//*/
}

LLPostProcess::~LLPostProcess(void)
{
	invalidate() ;
}

// static
void LLPostProcess::initClass(void)
{
	//this will cause system to crash at second time login
	//if first time login fails due to network connection --- bao
	//***llassert_always(gPostProcess == NULL);
	//replaced by the following line:
	if(gPostProcess)
		return ;
	
	
	gPostProcess = new LLPostProcess();
}

// static
void LLPostProcess::cleanupClass()
{
	delete gPostProcess;
	gPostProcess = NULL;
}

void LLPostProcess::setSelectedEffect(std::string const & effectName)
{
	mSelectedEffectName = effectName;
	static_cast<LLSD &>(tweaks) = mAllEffects[effectName];
}

void LLPostProcess::saveEffect(std::string const & effectName)
{
	/*  Do nothing.  Needs to be updated to use our current shader system, and to work with the move into llrender.*/
	mAllEffects[effectName] = tweaks;

	std::string pathName(gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, "windlight", XML_FILENAME));
	//llinfos << "Saving PostProcess Effects settings to " << pathName << llendl;

	llofstream effectsXML(pathName);

	LLPointer<LLSDFormatter> formatter = new LLSDXMLFormatter();

	formatter->format(mAllEffects, effectsXML);
	//*/
}
void LLPostProcess::invalidate()
{
	mSceneRenderTexture = NULL ;
	mNoiseTexture = NULL ;
	mTempBloomTexture = NULL ;
	initialized = FALSE ;
}

void LLPostProcess::apply(unsigned int width, unsigned int height)
{
	if (!initialized || width != screenW || height != screenH){
		initialize(width, height);
	}
	if (shadersEnabled()){
		doEffects();
	}
}

void LLPostProcess::initialize(unsigned int width, unsigned int height)
{
	screenW = width;
	screenH = height;
	createTexture(mSceneRenderTexture, screenW, screenH);
	initialized = true;

	checkError();
	createNightVisionShader();
	//createBloomShader();
	createColorFilterShader();
	createGaussBlurShader();
	checkError();
}

inline bool LLPostProcess::shadersEnabled(void)
{
	return (tweaks.useColorFilter().asBoolean() ||
			tweaks.useNightVisionShader().asBoolean() ||
			/*tweaks.useBloomShader().asBoolean() ||*/
			tweaks.useGaussBlurFilter().asBoolean() );

}

void LLPostProcess::applyShaders(void)
{
	bool copy_buffer = false;
	if (tweaks.useColorFilter())
	{
		applyColorFilterShader();
		checkError();
		copy_buffer = true;
	}
	if (tweaks.useGaussBlurFilter())
	{
		/// If any of the above shaders have been called update the frame buffer;
		if (copy_buffer)
			copyFrameBuffer(mSceneRenderTexture->getTexName(), screenW, screenH);
		applyGaussBlurShader();
		checkError();
		copy_buffer = true;
	}
	if (tweaks.useNightVisionShader())
	{
		/// If any of the above shaders have been called update the frame buffer;
		if (copy_buffer)
			copyFrameBuffer(mSceneRenderTexture->getTexName(), screenW, screenH);
		applyNightVisionShader();
		checkError();
		copy_buffer = true;
	}
	/*if (tweaks.useBloomShader())
	{
		/// If any of the above shaders have been called update the frame buffer;
		if (copy_buffer)
			copyFrameBuffer(mSceneRenderTexture->getTexName(), screenW, screenH);
		applyBloomShader();
		checkError();
		copy_buffer = true;
	}*/
}

void LLPostProcess::applyColorFilterShader(void)
{	
	if(gPostColorFilterProgram.mProgramObject == 0)
		return;
	/*  Do nothing.  Needs to be updated to use our current shader system, and to work with the move into llrender.*/
	gPostColorFilterProgram.bind();

	gGL.getTexUnit(0)->activate();
	gGL.getTexUnit(0)->enable(LLTexUnit::TT_RECT_TEXTURE);

	gGL.getTexUnit(0)->bindManual(LLTexUnit::TT_RECT_TEXTURE, mSceneRenderTexture.get()->getTexName());

	getShaderUniforms(colorFilterUniforms, gPostColorFilterProgram.mProgramObject);
	glUniform1iARB(colorFilterUniforms["RenderTexture"], 0);
	glUniform1fARB(colorFilterUniforms["gamma"], tweaks.getGamma());
	glUniform1fARB(colorFilterUniforms["brightness"], tweaks.getBrightness());
	glUniform1fARB(colorFilterUniforms["contrast"], tweaks.getContrast());
	float baseI = (tweaks.getContrastBaseR() + tweaks.getContrastBaseG() + tweaks.getContrastBaseB()) / 3.0f;
	baseI = tweaks.getContrastBaseIntensity() / ((baseI < 0.001f) ? 0.001f : baseI);
	float baseR = tweaks.getContrastBaseR() * baseI;
	float baseG = tweaks.getContrastBaseG() * baseI;
	float baseB = tweaks.getContrastBaseB() * baseI;
	glUniform3fARB(colorFilterUniforms["contrastBase"], baseR, baseG, baseB);
	glUniform1fARB(colorFilterUniforms["saturation"], tweaks.getSaturation());
	glUniform3fARB(colorFilterUniforms["lumWeights"], LUMINANCE_R, LUMINANCE_G, LUMINANCE_B);
	
	LLGLEnable blend(GL_BLEND);
	gGL.setSceneBlendType(LLRender::BT_REPLACE);
	LLGLDepthTest depth(GL_FALSE);
		
	/// Draw a screen space quad
	drawOrthoQuad(screenW, screenH, QUAD_NORMAL);
	gPostColorFilterProgram.unbind();
	//*/
}

void LLPostProcess::createColorFilterShader(void)
{
	/// Define uniform names
	colorFilterUniforms["RenderTexture"] = 0;
	colorFilterUniforms["gamma"] = 0;
	colorFilterUniforms["brightness"] = 0;
	colorFilterUniforms["contrast"] = 0;
	colorFilterUniforms["contrastBase"] = 0;
	colorFilterUniforms["saturation"] = 0;
	colorFilterUniforms["lumWeights"] = 0;
}

void LLPostProcess::applyNightVisionShader(void)
{	
	if(gPostNightVisionProgram.mProgramObject == 0)
		return;
	/*  Do nothing.  Needs to be updated to use our current shader system, and to work with the move into llrender.*/
	gPostNightVisionProgram.bind();

	gGL.getTexUnit(0)->activate();
	gGL.getTexUnit(0)->enable(LLTexUnit::TT_RECT_TEXTURE);

	getShaderUniforms(nightVisionUniforms, gPostNightVisionProgram.mProgramObject);
	gGL.getTexUnit(0)->bindManual(LLTexUnit::TT_RECT_TEXTURE, mSceneRenderTexture.get()->getTexName());
	glUniform1iARB(nightVisionUniforms["RenderTexture"], 0);

	gGL.getTexUnit(1)->activate();
	gGL.getTexUnit(1)->enable(LLTexUnit::TT_TEXTURE);	

	gGL.getTexUnit(1)->bindManual(LLTexUnit::TT_TEXTURE,  mNoiseTexture.get()->getTexName());
	glUniform1iARB(nightVisionUniforms["NoiseTexture"], 1);

	
	glUniform1fARB(nightVisionUniforms["brightMult"], tweaks.getBrightMult());
	glUniform1fARB(nightVisionUniforms["noiseStrength"], tweaks.getNoiseStrength());
	noiseTextureScale = 0.01f + ((101.f - tweaks.getNoiseSize()) / 100.f);
	noiseTextureScale *= (screenH / NOISE_SIZE);


	glUniform3fARB(nightVisionUniforms["lumWeights"], LUMINANCE_R, LUMINANCE_G, LUMINANCE_B);
	
	LLGLEnable blend(GL_BLEND);
	gGL.setSceneBlendType(LLRender::BT_REPLACE);
	LLGLDepthTest depth(GL_FALSE);
		
	/// Draw a screen space quad
	drawOrthoQuad(screenW, screenH, QUAD_NOISE);
	gPostNightVisionProgram.unbind();
	gGL.getTexUnit(0)->activate();
	//*/
}

void LLPostProcess::createNightVisionShader(void)
{
	/// Define uniform names
	nightVisionUniforms["RenderTexture"] = 0;
	nightVisionUniforms["NoiseTexture"] = 0;
	nightVisionUniforms["brightMult"] = 0;	
	nightVisionUniforms["noiseStrength"] = 0;
	nightVisionUniforms["lumWeights"] = 0;	

	createNoiseTexture(mNoiseTexture);
}

void LLPostProcess::applyBloomShader(void)
{

}

void LLPostProcess::createBloomShader(void)
{
	//createTexture(mTempBloomTexture, unsigned(screenW * 0.5), unsigned(screenH * 0.5));

	/// Create Bloom Extract Shader
	bloomExtractUniforms["RenderTexture"] = 0;
	bloomExtractUniforms["extractLow"] = 0;
	bloomExtractUniforms["extractHigh"] = 0;	
	bloomExtractUniforms["lumWeights"] = 0;	
	
	/// Create Bloom Blur Shader
	bloomBlurUniforms["RenderTexture"] = 0;
	bloomBlurUniforms["bloomStrength"] = 0;	
	bloomBlurUniforms["texelSize"] = 0;
	bloomBlurUniforms["blurDirection"] = 0;
	bloomBlurUniforms["blurWidth"] = 0;
}

void LLPostProcess::applyGaussBlurShader(void)
{
	int pass_count = tweaks.getGaussBlurPasses();
	if(!pass_count || gPostGaussianBlurProgram.mProgramObject == 0)
		return;

	getShaderUniforms(gaussBlurUniforms, gPostGaussianBlurProgram.mProgramObject);
	gPostGaussianBlurProgram.bind();
	gGL.getTexUnit(0)->activate();
	gGL.getTexUnit(0)->enable(LLTexUnit::TT_RECT_TEXTURE);
	gGL.getTexUnit(0)->bindManual(LLTexUnit::TT_RECT_TEXTURE, mSceneRenderTexture.get()->getTexName());
	LLGLEnable blend(GL_BLEND);
	LLGLDepthTest depth(GL_FALSE);
	gGL.setSceneBlendType(LLRender::BT_REPLACE);
	glUniform1iARB(gaussBlurUniforms["RenderTexture"], 0);
	GLint horiz_pass = gaussBlurUniforms["horizontalPass"];
	for(int i = 0;i<pass_count;++i)
	{
		for(int j = 0;j<2;++j)
		{
			if(i || j)
				copyFrameBuffer(mSceneRenderTexture->getTexName(), screenW, screenH);		
			glUniform1iARB(horiz_pass, j);
			drawOrthoQuad(screenW, screenH, QUAD_NORMAL);
			
		}
	}
	gPostGaussianBlurProgram.unbind();
	
}

void LLPostProcess::createGaussBlurShader(void)
{
	gaussBlurUniforms["RenderTexture"] = 0;
	gaussBlurUniforms["horizontalPass"] = 0;
}

void LLPostProcess::getShaderUniforms(glslUniforms & uniforms, GLhandleARB & prog)
{
	/// Find uniform locations and insert into map	
	std::map<const char *, GLuint>::iterator i;
	for (i  = uniforms.begin(); i != uniforms.end(); ++i){
		i->second = glGetUniformLocationARB(prog, i->first);
	}
}

void LLPostProcess::doEffects(void)
{
	/// Save GL State
	glPushAttrib(GL_ALL_ATTRIB_BITS);
	glPushClientAttrib(GL_ALL_ATTRIB_BITS);

	/// Copy the screen buffer to the render texture
	{
		copyFrameBuffer(mSceneRenderTexture->getTexName(), screenW, screenH);
	}

	/// Clear the frame buffer.
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	LLGLDisable(GL_DEPTH_TEST);

	/// Change to an orthogonal view
	viewOrthogonal(screenW, screenH);
	
	checkError();
	applyShaders();
	
	LLGLSLShader::bindNoShader();
	checkError();

	/// Change to a perspective view
	viewPerspective();	

	/// Reset GL State
	glPopClientAttrib();
	glPopAttrib();
	checkError();
}

void LLPostProcess::copyFrameBuffer(LLGLuint texture, unsigned int width, unsigned int height)
{
	gGL.getTexUnit(0)->bindManual(LLTexUnit::TT_RECT_TEXTURE, texture);
	glCopyTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA, 0, 0, width, height, 0);
}

inline void InitQuadArray(F32 *arr, const F32 x, const F32 y, const F32 width, const F32 height)
{
	//Lower left
	*(arr++)=x; 
	*(arr++)=y;
	//Upper left
	*(arr++)=x;
	*(arr++)=y+height;
	//Upper right
	*(arr++)=x+width;
	*(arr++)=y+height;
	//Lower right
	*(arr++)=x+width;
	*(arr++)=y;
}
void LLPostProcess::drawOrthoQuad(unsigned int width, unsigned int height, QuadType type)
{
#if 1
	//Redid the logic here. Less cases. No longer using immediate mode.
	bool second_tex = true;
	//Vertex array used for all post-processing effects
	static F32 verts[8];
	//Texture coord array used for all post-processing effects
	static F32 tex0[8];
	//Texture coord array used for relevant post processing effects
	static F32 tex1[8];

	//Set up vertex array
	InitQuadArray(verts, 0.f, 0.f, width, height);
	
	//Set up first texture coords
	if(type == QUAD_BLOOM_EXTRACT)
	{
		InitQuadArray(tex0, 0.f, 0.f, width*2.f, height*2.f);
		second_tex = false;
	}
	else
	{
		InitQuadArray(tex0, 0.f, 0.f, width, height);
		//Set up second texture coords
		if( type == QUAD_BLOOM_COMBINE)
			InitQuadArray(tex1, 0.f, 0.f, width*.5, height*.5);
		else if( type == QUAD_NOISE )
			InitQuadArray(tex1, ((float) rand() / (float) RAND_MAX), ((float) rand() / (float) RAND_MAX), 
				width * noiseTextureScale / height, noiseTextureScale);
		else
			second_tex = false;
	}
	
	//Prepare to render
	glEnableClientState( GL_VERTEX_ARRAY );
	glVertexPointer(2, GL_FLOAT, sizeof(F32)*2, verts);
	if(second_tex) //tex1 setup
	{
		glClientActiveTextureARB(GL_TEXTURE1);
		glEnableClientState( GL_TEXTURE_COORD_ARRAY );
		glTexCoordPointer(2, GL_FLOAT, sizeof(F32)*2, tex1);
	}
	glClientActiveTextureARB(GL_TEXTURE0);
	glEnableClientState( GL_TEXTURE_COORD_ARRAY );
	glTexCoordPointer(2, GL_FLOAT, sizeof(F32)*2, tex0);

	//Render
	glDrawArrays(GL_QUADS, 0, sizeof(verts)/sizeof(verts[0]));

	//Cleanup
	glDisableClientState( GL_VERTEX_ARRAY );
	glDisableClientState( GL_TEXTURE_COORD_ARRAY ); //for tex0
	if(second_tex)
	{
		glClientActiveTextureARB(GL_TEXTURE1);
		glDisableClientState( GL_TEXTURE_COORD_ARRAY ); //for tex1
	}
#endif
}

void LLPostProcess::viewOrthogonal(unsigned int width, unsigned int height)
{
	gGL.matrixMode(LLRender::MM_PROJECTION);
	gGL.pushMatrix();
	gGL.loadIdentity();
	gGL.ortho( 0.f, (GLdouble) width, 0.f, (GLdouble) height, -1.f, 1.f );
	gGL.matrixMode(LLRender::MM_MODELVIEW);
	gGL.pushMatrix();
	gGL.loadIdentity();
}

void LLPostProcess::viewPerspective(void)
{
	gGL.matrixMode( LLRender::MM_PROJECTION );
	gGL.popMatrix();
	gGL.matrixMode( LLRender::MM_MODELVIEW );
	gGL.popMatrix();
}

void LLPostProcess::changeOrthogonal(unsigned int width, unsigned int height)
{
	viewPerspective();
	viewOrthogonal(width, height);
}

void LLPostProcess::createTexture(LLPointer<LLImageGL>& texture, unsigned int width, unsigned int height)
{
	std::vector<GLubyte> data(width * height * 4, 0) ;

	texture = new LLImageGL(FALSE) ;	
	if(texture->createGLTexture())
	{
		gGL.getTexUnit(0)->bindManual(LLTexUnit::TT_RECT_TEXTURE, texture->getTexName());
		glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, 4, width, height, 0,
			GL_RGBA, GL_UNSIGNED_BYTE, &data[0]);
		gGL.getTexUnit(0)->setTextureFilteringOption(LLTexUnit::TFO_BILINEAR);
		gGL.getTexUnit(0)->setTextureAddressMode(LLTexUnit::TAM_CLAMP);
	}
}

void LLPostProcess::createNoiseTexture(LLPointer<LLImageGL>& texture)
{	
	std::vector<GLubyte> buffer(NOISE_SIZE * NOISE_SIZE);
	for (unsigned int i = 0; i < NOISE_SIZE; i++){
		for (unsigned int k = 0; k < NOISE_SIZE; k++){
			buffer[(i * NOISE_SIZE) + k] = (GLubyte)((double) rand() / ((double) RAND_MAX + 1.f) * 255.f);
		}
	}

	texture = new LLImageGL(FALSE) ;
	if(texture->createGLTexture())
	{
		gGL.getTexUnit(0)->bindManual(LLTexUnit::TT_TEXTURE, texture->getTexName());
		LLImageGL::setManualImage(GL_TEXTURE_2D, 0, GL_LUMINANCE, NOISE_SIZE, NOISE_SIZE, GL_LUMINANCE, GL_UNSIGNED_BYTE, &buffer[0]);
		gGL.getTexUnit(0)->setTextureFilteringOption(LLTexUnit::TFO_BILINEAR);
		gGL.getTexUnit(0)->setTextureAddressMode(LLTexUnit::TAM_WRAP);
	}
}

bool LLPostProcess::checkError(void)
{
	GLenum glErr;
    bool    retCode = false;

    glErr = glGetError();
    while (glErr != GL_NO_ERROR)
    {
		// shaderErrorLog << (const char *) gluErrorString(glErr) << std::endl;
		char const * err_str_raw = (const char *) gluErrorString(glErr);

		if(err_str_raw == NULL)
		{
			std::ostringstream err_builder;
			err_builder << "unknown error number " << glErr;
			mShaderErrorString = err_builder.str();
		}
		else
		{
			mShaderErrorString = err_str_raw;
		}

        retCode = true;
        glErr = glGetError();
    }
    return retCode;
}

void LLPostProcess::checkShaderError(GLhandleARB shader)
{
    GLint infologLength = 0;
    GLint charsWritten  = 0;
    GLchar *infoLog;

    checkError();  // Check for OpenGL errors

    glGetObjectParameterivARB(shader, GL_OBJECT_INFO_LOG_LENGTH_ARB, &infologLength);

    checkError();  // Check for OpenGL errors

    if (infologLength > 0)
    {
        infoLog = (GLchar *)malloc(infologLength);
        if (infoLog == NULL)
        {
            /// Could not allocate infolog buffer
            return;
        }
        glGetInfoLogARB(shader, infologLength, &charsWritten, infoLog);
		// shaderErrorLog << (char *) infoLog << std::endl;
		mShaderErrorString = (char *) infoLog;
        free(infoLog);
    }
    checkError();  // Check for OpenGL errors
}
