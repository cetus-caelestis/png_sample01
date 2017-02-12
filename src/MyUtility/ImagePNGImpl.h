//-------------------------------------------------------------
//! @brief	png�C���[�W
//! @author	��ĩ�=��ڽè�
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
// todo ���o�C�g�o�͂ɑΉ�
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


	std::vector<unsigned char>	m_parsedPixelbuffer;		//! �ŏI�I�ȃp�[�X���ʂ�ێ����� �Ƃ肠��������8bit�Œ�
	int							m_numBitScalingShift;

	std::vector<unsigned char>	m_rawData;					//! zlib�𓀍ςݐ��f�[�^
	PNGBitStream				m_rawStream;				//! ���f�[�^�𑖍����邽�߂�stream�N���X

	std::vector<unsigned>		m_currentLine;				//! �ǂݏo���������������C����̃s�N�Z���f�[�^��ێ�����
	std::vector<unsigned>		m_prevLine;					//! ���̐������C����̃s�N�Z���f�[�^��ێ�����

	size_t						m_width;					//! �摜����(�s�N�Z��)
	size_t						m_numElem;					//! �P�s�N�Z�����Ƃ̗v�f��(r,g,b�Ȃ�3, r,g,b,a �Ȃ�4)
	size_t						m_totalReadCountPerPixel;
	size_t						m_widthMulElem;				//! �摜�����~�v�f��
	unsigned					m_readBitDepth;				//! �ǂݎ��r�b�g�[�x(2�`8) 
	unsigned					m_pathPerElem;				//! ��v�f��ǂݏo�����߂ɕK�v�ȃp�X�BreadBitDepth * pathPerElem = ���r�b�g�[�x 

	PushElementMethod			m_pushElement = nullptr;	//! m_pixelbuffer�֓ǂݏo�����F�v�f��push���邽�߂̊֐�

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