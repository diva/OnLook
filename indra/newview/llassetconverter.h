// <edit>
#ifndef LL_LLASSETCONVERTER_H
#define LL_LLASSETCONVERTER_H

#include "llcommon.h"
#include "llassettype.h"

class LLAssetConverter
{
public:
	static LLAssetType::EType convert(std::string src_filename, std::string filename);
	static BOOL copyFile(std::string src_filename, std::string dest_filename);
};
#endif
// </edit>
