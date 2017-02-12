//-------------------------------------------------------------
//! @brief	png�C���[�W
//! @author	��ĩ�=��ڽè�
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
// struct�i�J���[�^�C�v�񋓁j
//-------------------------------------------------------------
struct ColorType
{
	enum
	{
		PALETTE = 0,	// 1 - �p���b�g�g�p
		COLOR   = 1,	// 2 - �J���[
		ALPHA   = 2,	// 4 - ���`�����l��
	};
};

//-------------------------------------------------------------
// struct�i�J���[�^�C�v�j
//-------------------------------------------------------------
using ColorTypeBitset = std::bitset<8>;


//-------------------------------------------------------------
// struct (png�t�@�C���f�[�^)
//-------------------------------------------------------------
struct PngFileData
{
	unsigned					width = 0;       //!< ����
	unsigned					height = 0;      //!< �c��

	unsigned					bitDepth = 0;    //!< bit�[�x
	ColorTypeBitset				colorType;		 //!< �J���[�^�C�v

	std::vector<unsigned char>	zlibData;        //!< ���k�ς݃f�[�^
};

//-------------------------------------------------------------
// helper function 
//-------------------------------------------------------------

//!< png�摜�f�[�^�̃��[�h
bool Load(PngFileData* out, const TCHAR* filepath);
bool Load(PngFileData* out, const unsigned char *dataBuffer, size_t size);
bool Load(PngFileData* out, std::basic_istream<unsigned char>& dataStream);

//!< �s�N�Z���f�[�^���тɐL��
std::vector<unsigned char> ToPixelArrayRGBA(const PngFileData& pngData);

}// end namespace
}// end namespace
}// end namespace