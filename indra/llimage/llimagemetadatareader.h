// <edit>
#ifndef LL_LLIMAGEMETADATAREADER_H
#define LL_LLIMAGEMETADATAREADER_H
#include "stdtypes.h"
#include <string.h>
#include <vector>
#include <string>

//encryption types
#define ENC_NONE 0
#define ENC_ONYXKDU 1
#define ENC_EMKDU_V1 2
#define ENC_EMKDU_V2 4

class LLJ2cParser
{
public:
	LLJ2cParser(U8* data,int data_size);
	std::vector<U8> GetNextComment();
	std::vector<U8> mData;
private:
	U8 nextChar();
	std::vector<U8> nextCharArray(int len);
	std::vector<U8>::iterator mIter;
};
class LLImageMetaDataReader
{
public:
	static unsigned int ExtractEncodedComment(U8* data,int data_size, std::string& output);
};
#endif
// </edit>
