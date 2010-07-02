// <edit>
#ifndef LL_LLIMAGEMETADATAREADER_H
#define LL_LLIMAGEMETADATAREADER_H
#include "stdtypes.h"
#include <string.h>
#include <vector>
#include <string>

class LLJ2cParser
{
public:
	LLJ2cParser(U8* data,int data_size);
	std::vector<U8> GetNextComment();
private:
	U8 nextChar();
	std::vector<U8> nextCharArray(int len);
	std::vector<U8> mData;
	std::vector<U8>::iterator mIter;
};
class LLImageMetaDataReader
{
public:
	static std::string ExtractEncodedComment(U8* data,int data_size);
};
#endif
// </edit>
