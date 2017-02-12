//-------------------------------------------------------------
//! @brief	png�C���[�W
//! @author	��ĩ�=��ڽè�
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

	//@brief �P�s�N�Z��������̗v�f�̐���Ԃ�
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
			numElement += 1; // +��

		return numElement;
	}

	//@brief �s�N�Z���F�̐L���̂��߂̃r�b�g�V�t�g�l
	//-------------------------------------------------------------
	inline int CalcBitShiftScaling(int bitDepth, int bitStoreScale)
	{
		return std::abs(bitDepth - bitStoreScale);
	}
	//@brief �ǂ�������̃r�b�g�V�t�g��
	//-------------------------------------------------------------
	inline bool IsRightBitShiftScaling(unsigned bitDepth, unsigned bitStoreScale)
	{
		return (bitDepth > bitStoreScale);
	}

	//@brief ��������F�𓱏o
	//@note  �e��t�t�B���^�������痘�p�����
	//-------------------------------------------------------------
	inline unsigned char CalcSubColor(int base, int sub)
	{
		return static_cast<unsigned char>((base + sub) & 0xff);	// % 256
	}

	//@brief ����s�N�Z���F�\�z�֐�
	//@note	 ���ɏo������s�N�Z���ɗאڂ��Ă��鍶�A��A����̃s�N�Z���̂����A
	//@note  �ǂꂪ���ɏo������F�ɋ߂�����\�����A���ʂ�Ԃ�
	//-------------------------------------------------------------
	int	PaethPredictor(int a, int b, int c)
	{
		// ����������
		// ����������
		// ����������
		// ������??��
		// ����������
		int p = a + b - c;

		int pa = abs(p - a);	// pa = |b - c|�@�@�@�������̒l�̕ς��
		int pb = abs(p - b);	// pb = |a - c|�@�@�@�c�����̒l�̕ς��
		int pc = abs(p - c);	// pc = |b-c + a-c|�@���ӂ��̍��v

		// �������̂ق����Ȃ��炩�Ȓl�̕ω� �� ��
		if (pa <= pb && pa <= pc)
			return a;

		// �c�����̂ق����Ȃ��炩�Ȓl�̕ω� �� ��
		if (pb <= pc)
			return b;

		// �c�����ꂼ�ꐳ���΂ɒl���ω����邽�ߒ��ԐF��I�� �� ����        
		return c;
	}
	//@brief ���V�t�g(�g��)
	//-------------------------------------------------------------
	inline unsigned char LeftShiftScaling(unsigned elem, int bitshift)
	{
		return static_cast<unsigned char>(elem << bitshift);
	};
	//@brief �E�V�t�g(�k��)
	//-------------------------------------------------------------
	inline unsigned char RightShiftScaling(unsigned elem, int bitshift)
	{
		return static_cast<unsigned char>(elem >> bitshift);
	};

} // end namespace

//@brief ����̃s�N�Z���v�f
//-------------------------------------------------------------
inline unsigned Image::Png::PngDataParser::GetLeft(size_t currentIndex)
{
	if (currentIndex < m_totalReadCountPerPixel)
		return 0;

	size_t prevIndex = currentIndex - m_totalReadCountPerPixel;
	return m_currentLine.at(prevIndex);
}
//@brief ���̃s�N�Z���v�f
//-------------------------------------------------------------
inline unsigned Image::Png::PngDataParser::GetUp(size_t currentIndex)
{
	if (m_prevLine.size() <= currentIndex)
		return 0;

	return m_prevLine[currentIndex];
}
//@brief ����̃s�N�Z���v�f
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

//@brief None�t�t�B���^
//-------------------------------------------------------------
void Image::Png::PngDataParser::NoneFilter(size_t numReadElement)
{
	// ���̂܂܊i�[
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

//@brief sub�t�t�B���^
//-------------------------------------------------------------
void Image::Png::PngDataParser::SubFilter()
{
	if (m_readBitDepth < 8)
		throw std::runtime_error("���m�F�̃p�^�[���B�Ή��ł��Ȃ����ۂ�");

	// �ŏ��̂P�s�N�Z���͂��̂܂܂̏�Ԃ̃f�[�^������
	NoneFilter(m_numElem);
	assert(m_currentLine.size() == m_totalReadCountPerPixel);

	// �ȍ~�͒��O��1�s�N�Z���Ƃ̍�������ăf�[�^������
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

//@brief up�t�t�B���^
//-------------------------------------------------------------
void Image::Png::PngDataParser::UpFilter()
{
	if (m_readBitDepth < 8)
		throw std::runtime_error("���m�F�̃p�^�[���B�Ή��ł��Ȃ����ۂ�");

	// ��̍s���Ȃ�
	if (m_prevLine.size() == 0)
	{
		NoneFilter();
		return;
	}

	// �^���1�s�N�Z���Ƃ̍�������ăf�[�^������
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
//@brief avrage�t�t�B���^
//-------------------------------------------------------------
void Image::Png::PngDataParser::AverageFilter()
{
	if (m_readBitDepth < 8)
		throw std::runtime_error("���m�F�̃p�^�[���B�Ή��ł��Ȃ����ۂ�");

	// �^��ƒ��O�̃s�N�Z���̕��ςƂ̍���
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
//@brief paeth�t�t�B���^
//-------------------------------------------------------------
void Image::Png::PngDataParser::PaethFilter()
{
	if (m_readBitDepth < 8)
		throw std::runtime_error("���m�F�̃p�^�[���B�Ή��ł��Ȃ����ۂ�");

	// ��ƍ��ƍ���̂����A���ɗ���s�N�Z���Ƌ߂��Ɨ\�z�����F�Ƃ̍���
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

	// green + blue ����ǉ�
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

	// alpha����ǉ�
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

//@brief �K�����v�f�L�^�p�̊֐���Ԃ�
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


  //@brief �s�N�Z�����т̃f�[�^��𐶐�����
  //-------------------------------------------------------------
std::vector<unsigned char>& Image::Png::PngDataParser::ToPixelArrayRGBA(const PngFileData& pngData)
{
	if(pngData.colorType[ColorType::PALETTE])
		std::runtime_error("�C���f�b�N�X�J���[�ɂ܂��Ή��ł��ĂȂ����ۂ�");

	// ���[�v���悭�g���p�����[�^�̎��O�v�Z
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

	// �������瑖���J�n
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
			default: throw std::runtime_error("�Ȃ񂩃t�B���^�^�C�v�������������ۂ�");
		}
		m_currentLine.swap(m_prevLine);
		m_currentLine.clear();
	}
	return m_parsedPixelbuffer;
}