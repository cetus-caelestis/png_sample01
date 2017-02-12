//-------------------------------------------------------------
//! @brief	pngイメージ
//! @author	ｹｰﾄｩｽ=ｶｴﾚｽﾃｨｽ
//-------------------------------------------------------------
#pragma once
#include <vector>
#include "ImagePNG.h"
#include "PrefixCodeTree.h"

namespace MyUtility
{
namespace Image
{ 
namespace Png
{
//-------------------------------------------------------------
// using
//-------------------------------------------------------------	
using PNGBitStream = MyUtility::DefaultBitStream;

//-------------------------------------------------------------
// class (PngDataConverter)
//-------------------------------------------------------------	
// todo 多バイト出力に対応
class PngDataParser
{
public:

	std::vector<unsigned char>& ToPixelArrayRGBA(const PngFileData& data);

	PngDataParser()
		:m_rawStream(nullptr,0)
	{};

private:

	static constexpr int BIT_STORE_SCALE = 8;
	using  PushElementMethod = void (PngDataParser::*)(unsigned);


	std::vector<unsigned char>	m_parsedPixelbuffer;		//! 最終的なパース結果を保持する とりあえず今は8bit固定
	int							m_numBitScalingShift;

	std::vector<unsigned char>	m_rawData;					//! zlib解凍済み生データ
	PNGBitStream				m_rawStream;				//! 生データを走査するためのstreamクラス

	std::vector<unsigned>		m_currentLine;				//! 読み出した同じ水平ライン上のピクセルデータを保持する
	std::vector<unsigned>		m_prevLine;					//! 一つ上の水平ライン上のピクセルデータを保持する

	size_t						m_width;					//! 画像横幅(ピクセル)
	size_t						m_numElem;					//! １ピクセルごとの要素数(r,g,bなら3, r,g,b,a なら4)
	size_t						m_totalReadCountPerPixel;
	size_t						m_widthMulElem;				//! 画像横幅×要素数
	unsigned					m_readBitDepth;				//! 読み取りビット深度(2～8) 
	unsigned					m_pathPerElem;				//! 一要素を読み出すために必要なパス。readBitDepth * pathPerElem = 実ビット深度 

	PushElementMethod			m_pushElement = nullptr;	//! m_pixelbufferへ読み出した色要素をpushするための関数

private:

	unsigned GetLeft(size_t);
	unsigned GetUp(size_t);
	unsigned GetUpLeft(size_t);

	void NoneFilter(size_t);
	void NoneFilter();

	void SubFilter();
	void UpFilter();
	void AverageFilter();
	void PaethFilter();

	using  ShiftScalingMethod = unsigned char(&)(unsigned, int);
	template<ShiftScalingMethod shiftFunc>
	void PushElementGrayOnlyToRGBA(unsigned);
	template<ShiftScalingMethod shiftFunc>
	void PushElementGrayAlphaToRGBA(unsigned);
	template<ShiftScalingMethod shiftFunc>
	void PushElementRGBToRGBA(unsigned);
	template<ShiftScalingMethod shiftFunc>
	void PushElementRGBAToRGBA(unsigned);

	static PushElementMethod ChoosePushElementMethod(const PngFileData&, int);
};

}// end namespace
}// end namespace
}// end namespace