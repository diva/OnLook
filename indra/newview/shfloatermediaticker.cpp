
#include "llviewerprecompiledheaders.h"

#include "shfloatermediaticker.h"

// Library includes
#include "llaudioengine.h"
#include "lliconctrl.h"
#include "llstreamingaudio.h"
#include "lluictrlfactory.h"

// Viewer includes
#include "llviewercontrol.h"


SHFloaterMediaTicker::SHFloaterMediaTicker() : LLFloater()/*, LLSingleton<SHFloaterMediaTicker>()*/,
	mPlayState(STATE_PAUSED), 
	mArtistScrollChars(0), 
	mTitleScrollChars(0), 
	mCurScrollChar(0),
	mTickerBackground(NULL),
	mArtistText(NULL),
	mTitleText(NULL),
	mVisualizer(NULL)
{
	setIsChrome(TRUE);									
	LLUICtrlFactory::getInstance()->buildFloater(this, "sh_floater_media_ticker.xml");
}

/*virtual*/ BOOL SHFloaterMediaTicker::postBuild()
{
	mTickerBackground = getChild<LLIconCtrl>("ticker_background");
	mArtistText =	getChild<LLTextBox>("artist_text",true,false);
	mTitleText	=	getChild<LLTextBox>("title_text",true,false);
	mVisualizer =	getChild<LLUICtrl>("visualizer_box",true,false);
	mszLoading	=	getString("loading");
	mszPaused	=	getString("paused");
	
	if(mArtistText)	mArtistText->setText(mszPaused);
	if(mTitleText)	mTitleText->setText(mszPaused);

	if(!gAudiop->getStreamingAudioImpl()->supportsWaveData())	//Can't visualize. Extend textboxes.
	{
		if(mArtistText)
		{
			LLRect text_rect = mArtistText->getRect();
			text_rect.mRight = llmax(mTickerBackground->getRect().mRight-2,text_rect.mRight);
			mArtistText->setRect(text_rect);
		}
		if(mTitleText)
		{
			LLRect text_rect = mTitleText->getRect();
			text_rect.mRight = llmax(mTickerBackground->getRect().mRight-2,text_rect.mRight);
			mArtistText->setRect(text_rect);
		}
	}

	return LLFloater::postBuild();
}

/*virtual*/ void SHFloaterMediaTicker::draw()
{
	updateTickerText();
	LLFloater::draw();
	drawOscilloscope();
}

/*virtual*/ void SHFloaterMediaTicker::onOpen()
{
	LLFloater::onOpen();

	gSavedSettings.setBOOL("SHShowMediaTicker", TRUE);
}
/*virtual*/ void SHFloaterMediaTicker::onClose(bool app_quitting)
{
	LLFloater::onClose(app_quitting);

	if (!app_quitting)
	{
		gSavedSettings.setBOOL("SHShowMediaTicker", FALSE);
		delete this;
	}
}

void SHFloaterMediaTicker::updateTickerText() //called via draw.
{
	bool stream_paused = gAudiop->getStreamingAudioImpl()->isPlaying() != 1;	//will return 1 if playing.

	bool dirty = setPaused(stream_paused);
	if(!stream_paused)
	{
		const LLSD* metadata = gAudiop->getStreamingAudioImpl()->getMetaData();
		LLSD artist = metadata ? metadata->get("ARTIST") : LLSD();
		LLSD title = metadata ? metadata->get("TITLE") : LLSD();
	
		dirty |= setArtist(artist.isDefined() ? artist.asString() : mszLoading);
		dirty |= setTitle(title.isDefined() ? title.asString() : mszLoading);

		if(dirty)
			resetTicker();
		else iterateTickerOffset();
	}
}

void SHFloaterMediaTicker::drawOscilloscope() //called via draw.
{
	if(!mVisualizer || !gAudiop->getStreamingAudioImpl()->supportsWaveData())
		return;

	static const S32 NUM_LINE_STRIPS = 64;			//How many lines to draw. 64 is more than enough.
	static const S32 WAVE_DATA_STEP_SIZE = 4;		//Increase to provide more history at expense of cpu/memory.

	static const S32 NUM_WAVE_DATA_VALUES = NUM_LINE_STRIPS*WAVE_DATA_STEP_SIZE;	//Actual buffer size. Don't toy with this. Change above vars to tweak.
	static F32 buf[NUM_WAVE_DATA_VALUES];

	if(!gAudiop->getStreamingAudioImpl()->getWaveData(&buf[0],NUM_WAVE_DATA_VALUES,WAVE_DATA_STEP_SIZE))
		return;

	LLRect root_rect = mVisualizer->getRect();

	F32 height = root_rect.getHeight();
	F32 height_scale = height / 2.f;	//WaveData ranges from 1 to -1, so height_scale = height / 2
	F32 width_scale = root_rect.getWidth() / (F32)NUM_WAVE_DATA_VALUES;

	gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
	gGL.color4f(0.f,0.f,0.f,.75f);
	gGL.pushMatrix();
		gGL.translatef((F32)root_rect.mLeft, (F32)root_rect.mBottom + height*.5f, 0.f);
		gGL.begin( LLRender::LINE_STRIP );
		if((mPlayState != STATE_PAUSED))
		{
			for(S32 i = NUM_WAVE_DATA_VALUES; i>=0;i-=WAVE_DATA_STEP_SIZE)
				gGL.vertex2f((F32)i * width_scale, buf[i]*height_scale);
		}
		else
		{
			gGL.vertex2f(0.f, 0.f);
			gGL.vertex2f(root_rect.getWidth(), 0.f);
		}
		gGL.end();
	gGL.popMatrix();
	gGL.flush();
}

bool SHFloaterMediaTicker::setPaused(bool pause)
{
	if(pause == (mPlayState == STATE_PAUSED))
		return false;
	mPlayState = pause ? STATE_PAUSED : STATE_PLAYING;
	if(pause)
	{
		if(mArtistText)	mArtistText->setText(mszPaused);
		if(mTitleText)	mTitleText->setText(mszPaused);
	}
	return true;
}

void SHFloaterMediaTicker::resetTicker()
{
	mScrollTimer.reset();
	mCurScrollChar=0;
	if(mArtistText)	mArtistText->setText(LLStringExplicit(mszArtist.substr(0,mszArtist.length()-mArtistScrollChars)));
	if(mTitleText)	mTitleText->setText(LLStringExplicit(mszTitle.substr(0,mszTitle.length()-mTitleScrollChars)));
}

bool SHFloaterMediaTicker::setArtist(const std::string &artist)
{
	if(!mArtistText || mszArtist == artist)
		return false;
	mszArtist = artist;
	mArtistText->setText(mszArtist);
	mArtistScrollChars = countExtraChars(mArtistText,mszArtist);
	return true;
}

bool SHFloaterMediaTicker::setTitle(const std::string &title)
{
	if(!mTitleText || mszTitle == title)
		return false;
	mszTitle=title;
	mTitleText->setText(mszTitle);
	mTitleScrollChars = countExtraChars(mTitleText,mszTitle);
	return true;
}

S32 SHFloaterMediaTicker::countExtraChars(LLTextBox *texbox, const std::string &text)
{
	S32 text_width = texbox->getTextPixelWidth();
	S32 box_width = texbox->getRect().getWidth();
	if(text_width > box_width)
	{
		const LLFontGL* font = texbox->getFont();
		for(S32 count = 1;count<(S32)text.length();count++)
		{
			//This isn't very efficient...
			const std::string substr = text.substr(0,text.length()-count);
			if(font->getWidth(substr) <= box_width)
				return count;
		}
	}
	return 0;
}

void SHFloaterMediaTicker::iterateTickerOffset()
{
	if(	(mPlayState != STATE_PAUSED) &&
		(mArtistScrollChars || mTitleScrollChars) &&
		((!mCurScrollChar && mScrollTimer.getElapsedTimeF32() >= 5.f) ||
		 ( mCurScrollChar && mScrollTimer.getElapsedTimeF32() >= .5f)))
	{
		if(++mCurScrollChar > llmax(mArtistScrollChars, mTitleScrollChars))
		{
			if(mScrollTimer.getElapsedTimeF32() >= 2.f)	//pause for a bit when it reaches beyond last character.
				resetTicker();	
		}
		else
		{
			mScrollTimer.reset();
			if(mArtistText && mCurScrollChar <= mArtistScrollChars)
			{
				mArtistText->setText(LLStringExplicit(mszArtist.substr(mCurScrollChar,mszArtist.length()-mArtistScrollChars+mCurScrollChar)));
			}
			if(mTitleText && mCurScrollChar <= mTitleScrollChars)
			{
				mTitleText->setText(LLStringExplicit(mszTitle.substr(mCurScrollChar,mszTitle.length()-mTitleScrollChars+mCurScrollChar)));
			}
		}
	}
}

/*static*/
void SHFloaterMediaTicker::showInstance()
{
	if(!handle_ticker_enabled(NULL))
		return;
	if(!SHFloaterMediaTicker::instanceExists())
	{
		SHFloaterMediaTicker::getInstance();
	}
}

BOOL handle_ticker_enabled(void *)
{
	return gAudiop && gAudiop->getStreamingAudioImpl() && gAudiop->getStreamingAudioImpl()->supportsMetaData();
}
BOOL handle_ticker_check(void *)
{
	return SHFloaterMediaTicker::instanceExists();
}
void handle_ticker_toggle(void *)
{
	if(!handle_ticker_enabled(NULL))
		return;
	if(!SHFloaterMediaTicker::instanceExists())
	{
		SHFloaterMediaTicker::getInstance();
	}
	else
	{
		SHFloaterMediaTicker::getInstance()->close();
	}
}