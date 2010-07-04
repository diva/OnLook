// <edit>
#include "linden_common.h"
#include "llimagemetadatareader.h"

LLJ2cParser::LLJ2cParser(U8* data,int data_size)
{
    	if(data && data_size)
	{
		mData.resize(data_size);
		//memcpy(&(mData[0]), data, data_size);
		std::copy(data,data+data_size,mData.begin());
	}
	mIter = mData.begin();
}

U8 LLJ2cParser::nextChar()
{
	U8 rtn = 0x00;
	if(mIter != mData.end())
	{
		rtn = (*mIter);
		mIter++;
	}
	return rtn;
}

std::vector<U8> LLJ2cParser::nextCharArray(int len)
{
	std::vector<U8> array;
	if(len > 0)
	{
		array.resize(len);
		for(S32 i = 0; i < len; i++)
		{
			array[i] = nextChar();
		}
	}
	return array;
}

std::vector<U8> LLJ2cParser::GetNextComment()
{
	std::vector<U8> content;
	while (mIter != mData.end())
	{
		U8 marker = nextChar();
		if (marker == 0xff)
		{
		    U8 marker_type = nextChar();
		    if (marker_type == 0x4f)
		    {
			continue;
		    }
		    if (marker_type == 0x90)
		    {
			//llinfos << "FOUND 0x90" << llendl;
			break; //return empty vector
		    }
		    
		    if (marker_type == 0x64)
		    {
			//llinfos << "FOUND 0x64 COMMENT SECTION" << llendl;
			S32 len = ((S32)nextChar())*256 + (S32)nextChar();
		   	if (len > 3) content = nextCharArray(len - 2);
			return content;
		    }
		}
	}
	content.clear(); //return empty vector by clear anything there
	return content;
}

//static
std::string LLImageMetaDataReader::ExtractEncodedComment(U8* data,int data_size)
{
	LLJ2cParser parser = LLJ2cParser(data,data_size);
	while(1)
	{
	    std::vector<U8> comment = parser.GetNextComment();
	    if (comment.empty()) break; //exit loop
	    if (comment[1] == 0x00 && comment.size() == 130)
	    {
		//llinfos << "FOUND PAYLOAD" << llendl;
		std::vector<U8> payload(128);
		S32 i;
		//memcpy(&(payload[0]), &(comment[2]), 128);
		std::copy(comment.begin()+2,comment.end(),payload.begin());
		if (payload[2] == payload[127])
		{
		    // emkdu.dll
		    for (i = 4; i < 128; i += 4)
		    {
			payload[i] ^= payload[3];
			payload[i + 1] ^= payload[1];
			payload[i + 2] ^= payload[0];
			payload[i + 3] ^= payload[2];
		    }
		}
		else if (payload[3] == payload[127])
		{
		    // emkdu.dll or onyxkdu.dll
		    for (i = 4; i < 128; i += 4)
		    {
			payload[i] ^= payload[2];
			payload[i + 1] ^= payload[0];
			payload[i + 2] ^= payload[1];
			payload[i + 3] ^= payload[3];
		    }
		}
		else
		{
		    break;//exit loop
		}
		for (i = 4; i < 128; ++i)
		{
		    if (payload[i] == 0) break;
		}
		if(i < 4) break;
		std::string result(payload.begin()+4,payload.begin()+i);
		//llinfos << "FOUND COMMENT: " << result << llendl;
		return result;
	    }
	}
	//end of loop
	return "";
}
// </edit>
