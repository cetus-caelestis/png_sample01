//-------------------------------------------------------------
//! @brief	pngイメージ
//! @author	ｹｰﾄｩｽ=ｶｴﾚｽﾃｨｽ
//-------------------------------------------------------------
#pragma once
#include <vector>
#include <iostream> // istream
#include <bitset>   // bitset
#include <tchar.h>

namespace MyUtility
{
namespace Image
{ 
namespace Png
{
//-------------------------------------------------------------
// struct（カラータイプ列挙）
//-------------------------------------------------------------
struct ColorType
{
	enum
	{
		PALETTE = 0,	// 1 - パレット使用
		COLOR   = 1,	// 2 - カラー
		ALPHA   = 2,	// 4 - αチャンネル
	};
};

//-------------------------------------------------------------
// struct（カラータイプ）
//-------------------------------------------------------------
using ColorTypeBitset = std::bitset<8>;


//-------------------------------------------------------------
// struct (pngファイルデータ)
//-------------------------------------------------------------
struct PngFileData
{
	unsigned					width = 0;       //!< 横幅
	unsigned					height = 0;      //!< 縦幅

	unsigned					bitDepth = 0;    //!< bit深度
	ColorTypeBitset				colorType;		 //!< カラータイプ

	std::vector<unsigned char>	zlibData;        //!< 圧縮済みデータ
};

//-------------------------------------------------------------
// helper function 
//-------------------------------------------------------------

//!< png画像データのロード
bool Load(PngFileData* out, const TCHAR* filepath);
bool Load(PngFileData* out, const unsigned char *dataBuffer, size_t size);
bool Load(PngFileData* out, std::basic_istream<unsigned char>& dataStream);

//!< ピクセルデータ並びに伸長
std::vector<unsigned char> ToPixelArrayRGBA(const PngFileData& pngData);

}// end namespace
}// end namespace
}// end namespace