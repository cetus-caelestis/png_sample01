//-------------------------------------------------------------
//! @brief	pngイメージ
//! @author	ｹｰﾄｩｽ=ｶｴﾚｽﾃｨｽ
//-------------------------------------------------------------

//-------------------------------------------------------------
// include
//-------------------------------------------------------------
#include <assert.h>

#include "ImagePNGImpl.h"
#include "ZLib.h"

//-------------------------------------------------------------
// using
//-------------------------------------------------------------
using namespace MyUtility;

//-------------------------------------------------------------
// inner
//-------------------------------------------------------------
namespace
{

	//@brief １ピクセルあたりの要素の数を返す
	//-------------------------------------------------------------
	size_t GetNumElementOfPixel(const Image::Png::PngFileData& data)
	{
		using namespace MyUtility::Image::Png;

		int numElement{};
		if (data.colorType[ColorType::COLOR])
			numElement = 3;	 // r, g, b
		else
			numElement = 1;	 // g

		if (data.colorType[ColorType::ALPHA])
			numElement += 1; // +α

		return numElement;
	}

	//@brief ピクセル色の伸長のためのビットシフト値
	//-------------------------------------------------------------
	inline int CalcBitShiftScaling(int bitDepth, int bitStoreScale)
	{
		return std::abs(bitDepth - bitStoreScale);
	}
	//@brief どちら方向のビットシフトか
	//-------------------------------------------------------------
	inline bool IsRightBitShiftScaling(unsigned bitDepth, unsigned bitStoreScale)
	{
		return (bitDepth > bitStoreScale);
	}

	//@brief 差分から色を導出
	//@note  各種逆フィルタ処理から利用される
	//-------------------------------------------------------------
	inline unsigned char CalcSubColor(int base, int sub)
	{
		return static_cast<unsigned char>((base + sub) & 0xff);	// % 256
	}

	//@brief 次回ピクセル色予想関数
	//@note	 次に出現するピクセルに隣接している左、上、左上のピクセルのうち、
	//@note  どれが次に出現する色に近いかを予測し、結果を返す
	//-------------------------------------------------------------
	int	PaethPredictor(int a, int b, int c)
	{
		// ┏━┳━┓
		// ┃ｃ┃ｂ┃
		// ┣━╋━┫
		// ┃ａ┃??┃
		// ┗━┻━┛
		int p = a + b - c;

		int pa = abs(p - a);	// pa = |b - c|　　　横向きの値の変わり具合
		int pb = abs(p - b);	// pb = |a - c|　　　縦向きの値の変わり具合
		int pc = abs(p - c);	// pc = |b-c + a-c|　↑ふたつの合計

		// 横向きのほうがなだらかな値の変化 → 左
		if (pa <= pb && pa <= pc)
			return a;

		// 縦向きのほうがなだらかな値の変化 → 上
		if (pb <= pc)
			return b;

		// 縦横それぞれ正反対に値が変化するため中間色を選択 → 左上        
		return c;
	}
	//@brief 左シフト(拡大)
	//-------------------------------------------------------------
	inline unsigned char LeftShiftScaling(unsigned elem, int bitshift)
	{
		return static_cast<unsigned char>(elem << bitshift);
	};
	//@brief 右シフト(縮小)
	//-------------------------------------------------------------
	inline unsigned char RightShiftScaling(unsigned elem, int bitshift)
	{
		return static_cast<unsigned char>(elem >> bitshift);
	};

} // end namespace

//@brief 一つ左のピクセル要素
//-------------------------------------------------------------
inline unsigned Image::Png::PngDataParser::GetLeft(size_t currentIndex)
{
	if (currentIndex < m_totalReadCountPerPixel)
		return 0;

	size_t prevIndex = currentIndex - m_totalReadCountPerPixel;
	return m_currentLine.at(prevIndex);
}
//@brief 一つ上のピクセル要素
//-------------------------------------------------------------
inline unsigned Image::Png::PngDataParser::GetUp(size_t currentIndex)
{
	if (m_prevLine.size() <= currentIndex)
		return 0;

	return m_prevLine[currentIndex];
}
//@brief 一つ左のピクセル要素
//-------------------------------------------------------------
inline unsigned Image::Png::PngDataParser::GetUpLeft(size_t currentIndex)
{
	if (currentIndex < m_totalReadCountPerPixel)
		return 0;

	size_t prevIndex = currentIndex - m_totalReadCountPerPixel;
	if (m_prevLine.size() <= prevIndex)
		return 0;

	return m_prevLine[prevIndex];
}

//@brief None逆フィルタ
//-------------------------------------------------------------
void Image::Png::PngDataParser::NoneFilter(size_t numReadElement)
{
	// そのまま格納
	for (size_t w = 0; w < numReadElement; ++w)
	{
		unsigned rawElement = 0;
		for (unsigned i = 0; i < m_pathPerElem; ++i)
		{
			unsigned tmp = m_rawStream.GetRange(m_readBitDepth);

			m_currentLine.push_back(tmp);
			rawElement = tmp | (rawElement << 8);
		}
		(this->*m_pushElement)(rawElement);
	}
}
//-------------------------------------------------------------
void Image::Png::PngDataParser::NoneFilter()
{
	NoneFilter(m_widthMulElem);
}

//@brief sub逆フィルタ
//-------------------------------------------------------------
void Image::Png::PngDataParser::SubFilter()
{
	if (m_readBitDepth < 8)
		throw std::runtime_error("未確認のパターン。対応できないっぽい");

	// 最初の１ピクセルはそのままの状態のデータを入れる
	NoneFilter(m_numElem);
	assert(m_currentLine.size() == m_totalReadCountPerPixel);

	// 以降は直前の1ピクセルとの差を取ってデータを入れる
	for (size_t w = m_numElem; w < m_widthMulElem; ++w)
	{
		unsigned rawElement = 0;
		for (unsigned i = 0; i < m_pathPerElem; ++i)
		{
			unsigned prev = GetLeft(m_currentLine.size());
			unsigned tmp  = CalcSubColor(prev, m_rawStream.GetRange(m_readBitDepth));

			m_currentLine.push_back(tmp);
			rawElement = tmp | (rawElement << 8);
		}
		(this->*m_pushElement)(rawElement);
	}
}

//@brief up逆フィルタ
//-------------------------------------------------------------
void Image::Png::PngDataParser::UpFilter()
{
	if (m_readBitDepth < 8)
		throw std::runtime_error("未確認のパターン。対応できないっぽい");

	// 上の行がない
	if (m_prevLine.size() == 0)
	{
		NoneFilter();
		return;
	}

	// 真上の1ピクセルとの差を取ってデータを入れる
	for (size_t w = 0; w < m_widthMulElem; ++w)
	{
		unsigned rawElement = 0;
		for (unsigned i = 0; i < m_pathPerElem; ++i)
		{
			unsigned prev = GetUp(m_currentLine.size());
			unsigned tmp  = CalcSubColor(prev, m_rawStream.GetRange(m_readBitDepth));

			m_currentLine.push_back(tmp);
			rawElement = tmp | (rawElement << 8);
		}
		(this->*m_pushElement)(rawElement);
	}
}
//@brief avrage逆フィルタ
//-------------------------------------------------------------
void Image::Png::PngDataParser::AverageFilter()
{
	if (m_readBitDepth < 8)
		throw std::runtime_error("未確認のパターン。対応できないっぽい");

	// 真上と直前のピクセルの平均との差分
	for (size_t w = 0; w < m_widthMulElem; ++w)
	{
		unsigned rawElement = 0;
		for (unsigned i = 0; i < m_pathPerElem; ++i)
		{
			unsigned left = GetLeft(m_currentLine.size());
			unsigned up   = GetUp(m_currentLine.size());
			unsigned avg = (left + up) / 2;

			unsigned tmp = CalcSubColor(avg, m_rawStream.GetRange(m_readBitDepth));

			m_currentLine.push_back(tmp);
			rawElement = tmp | (rawElement << 8);
		}
		(this->*m_pushElement)(rawElement);
	}
}
//@brief paeth逆フィルタ
//-------------------------------------------------------------
void Image::Png::PngDataParser::PaethFilter()
{
	if (m_readBitDepth < 8)
		throw std::runtime_error("未確認のパターン。対応できないっぽい");

	// 上と左と左上のうち、次に来るピクセルと近いと予想される色との差分
	for (size_t w = 0; w < m_widthMulElem; ++w)
	{
		unsigned rawElement = 0;
		for (unsigned i = 0; i < m_pathPerElem; ++i)
		{
			unsigned left   = GetLeft(m_currentLine.size());
			unsigned up     = GetUp(m_currentLine.size());
			unsigned upleft = GetUpLeft(m_currentLine.size());

			unsigned predictive = PaethPredictor(left, up, upleft);

			unsigned tmp = CalcSubColor(predictive, m_rawStream.GetRange(m_readBitDepth));

			m_currentLine.push_back(tmp);
			rawElement = tmp | (rawElement << 8);
		}
		(this->*m_pushElement)(rawElement);
	}
}

//@brief Gray scale only -> RGBA
//-------------------------------------------------------------
template<Image::Png::PngDataParser::ShiftScalingMethod shiftFunc>
void Image::Png::PngDataParser::PushElementGrayOnlyToRGBA(unsigned elem)
{
	auto pushElem = shiftFunc(elem, m_numBitScalingShift);

	// r + g + b + a
	m_parsedPixelbuffer.push_back(pushElem);
	m_parsedPixelbuffer.push_back(pushElem);
	m_parsedPixelbuffer.push_back(pushElem);
	m_parsedPixelbuffer.push_back(1);
}

//@brief Gray scale with alpha -> RGBA
//-------------------------------------------------------------
template<Image::Png::PngDataParser::ShiftScalingMethod shiftFunc>
void Image::Png::PngDataParser::PushElementGrayAlphaToRGBA(unsigned elem)
{
	auto pushElem = shiftFunc(elem, m_numBitScalingShift);
	m_parsedPixelbuffer.push_back(pushElem);

	// green + blue 情報を追加
	if ( (m_parsedPixelbuffer.size() & 0x3) == 1)
	{
		m_parsedPixelbuffer.push_back(pushElem);
		m_parsedPixelbuffer.push_back(pushElem);
	}
}

//@brief RGB -> RGBA
//-------------------------------------------------------------
template<Image::Png::PngDataParser::ShiftScalingMethod shiftFunc>
void Image::Png::PngDataParser::PushElementRGBToRGBA(unsigned elem)
{
	auto pushElem = shiftFunc(elem, m_numBitScalingShift);
	m_parsedPixelbuffer.push_back(pushElem);

	// alpha情報を追加
	if( (m_parsedPixelbuffer.size()& 0x3) == 3)
		m_parsedPixelbuffer.push_back(1);
}

//@brief RGBA -> RGBA
//-------------------------------------------------------------
template<Image::Png::PngDataParser::ShiftScalingMethod shiftFunc>
void Image::Png::PngDataParser::PushElementRGBAToRGBA(unsigned elem)
{
	auto pushElem = shiftFunc(elem, m_numBitScalingShift);
	m_parsedPixelbuffer.push_back(pushElem);
}

//@brief 適した要素記録用の関数を返す
//-------------------------------------------------------------
Image::Png::PngDataParser::PushElementMethod Image::Png::PngDataParser::ChoosePushElementMethod(const PngFileData& pngData, int bitStoreScale)
{
	// [color][alpha][isRightShift]
	const PushElementMethod methodTable[2][2][2] = 
	{
		{
			{
				&PushElementGrayOnlyToRGBA<LeftShiftScaling>,
				&PushElementGrayOnlyToRGBA<RightShiftScaling>,
			},
			{
				&PushElementGrayAlphaToRGBA<LeftShiftScaling>,
				&PushElementGrayAlphaToRGBA<RightShiftScaling>,
			},		
		},
		{
			{
				&PushElementRGBToRGBA<LeftShiftScaling>,
				&PushElementRGBToRGBA<RightShiftScaling>,
			},
			{
				&PushElementRGBAToRGBA<LeftShiftScaling>,
				&PushElementRGBAToRGBA<RightShiftScaling>,
			},
		},
	};
	
	bool isColor      = pngData.colorType[ColorType::COLOR];
	bool hasAlpha     = pngData.colorType[ColorType::ALPHA];
	bool isRightShift = IsRightBitShiftScaling(pngData.bitDepth, bitStoreScale);

	return methodTable[isColor][hasAlpha][isRightShift];
}


  //@brief ピクセル並びのデータ列を生成する
  //-------------------------------------------------------------
std::vector<unsigned char>& Image::Png::PngDataParser::ToPixelArrayRGBA(const PngFileData& pngData)
{
	if(pngData.colorType[ColorType::PALETTE])
		std::runtime_error("インデックスカラーにまだ対応できてないっぽい");

	// ループ中よく使うパラメータの事前計算
	m_rawData   = MyUtility::ZLib::Decode(pngData.zlibData.data(), pngData.zlibData.size());
	m_rawStream = PNGBitStream(m_rawData.data(), m_rawData.size());

	m_width			= pngData.width;
	m_numElem		= GetNumElementOfPixel(pngData);
	m_widthMulElem	= m_width * m_numElem;

	m_numBitScalingShift = CalcBitShiftScaling(pngData.bitDepth, BIT_STORE_SCALE);
	m_pushElement        = ChoosePushElementMethod(pngData, BIT_STORE_SCALE);

	if (pngData.bitDepth == 16)
	{
		m_readBitDepth  = 8;
		m_pathPerElem   = 2;
	}
	else
	{
		m_readBitDepth  = pngData.bitDepth;
		m_pathPerElem   = 1;
	}
	m_totalReadCountPerPixel = m_numElem * m_pathPerElem;

	// ここから走査開始
	for (size_t h = 0; h < pngData.height; ++h)
	{
		m_rawStream.SkipByte();
		unsigned filterType = m_rawStream.GetRange(8);

		switch (filterType)
		{
			case 0: NoneFilter();		break;
			case 1: SubFilter();		break;
			case 2: UpFilter();			break;
			case 3: AverageFilter();	break;
			case 4: PaethFilter();		break;
			default: throw std::runtime_error("なんかフィルタタイプがおかしいっぽい");
		}
		m_currentLine.swap(m_prevLine);
		m_currentLine.clear();
	}
	return m_parsedPixelbuffer;
}