//-------------------------------------------------------------
//! @brief	png�C���[�W
//! @author	��ĩ�=��ڽè�
//-------------------------------------------------------------

//-------------------------------------------------------------
// include
//-------------------------------------------------------------
#include <fstream>
#include <algorithm>
#include <type_traits>
#include <sstream>
#include <array>

#include "ImagePNG.h"
#include "ImagePNGImpl.h"

//-------------------------------------------------------------
// using
//-------------------------------------------------------------
using namespace MyUtility;
using PNGBitStream = MyUtility::DefaultBitStream;

//-------------------------------------------------------------
// inner
//-------------------------------------------------------------
namespace
{
//struct ChunkType
//{
//	static const uint32_t IHDR = 0x49484452L;
//	static const uint32_t IDAT = 0x49444154L;
//	static const uint32_t IEND = 0x49454E44L;
//};
struct ReflectChunkType
{
	static const uint32_t IHDR = 0x52444849L;	
	static const uint32_t IDAT = 0x54414449L;
	static const uint32_t IEND = 0x444E4549L;
};


//-------------------------------------------------------------
// inner function
//-------------------------------------------------------------

//@brief �r�b�g������[�h(�r�b�O�G���f�B�A����)
//-------------------------------------------------------------
unsigned GetBigEndianRange(PNGBitStream& stream, unsigned numbit)
{
	// note: ���ܓǂ�ł���ʒu���o�C�g���E�łȂ��Ɛ������@�\���Ȃ�
	unsigned bit = 0;
	for (unsigned i = 0; i < numbit; i += 8)
	{
		unsigned subCount = numbit - i;
		if (subCount > 8) subCount = 8;

		bit = (bit << 8) | stream.GetRange(subCount);
	}
	return bit;
}

//@brief 32bit���l���擾
//-------------------------------------------------------------
uint32_t ReadUint32(std::basic_istream<unsigned char>& stream)
{
	uint32_t number = 0;
	number |= static_cast<uint32_t>(stream.get()) << 24;
	number |= static_cast<uint32_t>(stream.get()) << 16;
	number |= static_cast<uint32_t>(stream.get()) << 8;
	number |= static_cast<uint32_t>(stream.get());
	return number;
}

//@brief Reflect���ꂽ32bit���l���擾
//-------------------------------------------------------------
uint32_t ReadReflectedUint32(std::basic_istream<unsigned char>& stream)
{
	// �t���ɓǂݏo��
	uint32_t number = 0;
	number |= static_cast<uint32_t>(stream.get());
	number |= static_cast<uint32_t>(stream.get()) << 8;
	number |= static_cast<uint32_t>(stream.get()) << 16;
	number |= static_cast<uint32_t>(stream.get()) << 24;
	return number;
}


//@brief �͈͂�Ԃ�
//-------------------------------------------------------------
template<size_t num>
std::array<unsigned char, num> ReadRange(std::basic_istream<unsigned char>& stream)
{
	std::array<unsigned char, num> result{};

	for (size_t i = 0; i < num && !stream.eof(); ++i)
	{
		result[i] = static_cast<unsigned char>(stream.get());
	}
	return result;
}
//-------------------------------------------------------------
std::vector<unsigned char> ReadRange(std::basic_istream<unsigned char>& stream, size_t num)
{
	std::vector<unsigned char>  result{};

	for (size_t i = 0; i < num && !stream.eof(); ++i)
	{
		result.push_back(static_cast<unsigned char>(stream.get()));
	}
	return result;
}
//@brief �X�L�b�v����
//-------------------------------------------------------------
void Skip(std::basic_istream<unsigned char>& stream, size_t length)
{
	for (size_t i = 0; i < length && !stream.eof(); ++i)
	{
		stream.get();
	}
}

//@brief png�t�@�C����\���V�O�l�`�����`�F�b�N
//-------------------------------------------------------------
bool IsPngSignature(std::basic_istream<unsigned char>& stream)
{
	const size_t SIGNATURE_SIZE = 8;
	const unsigned char signature[SIGNATURE_SIZE] =
	{
		0x89,
		0x50,	// P
		0x4e,	// N
		0x47,	// G
		0x0d,
		0x0a,
		0x1a,
		0x0a,
	};
	auto checkTmp = ReadRange<SIGNATURE_SIZE>(stream);
	return std::equal(std::begin(checkTmp), std::end(checkTmp),
					  std::begin(signature), std::end(signature));
}
//@brief IHDR�`�����N��ǂݏo��
//-------------------------------------------------------------
bool ReadIHDR(Image::Png::PngFileData* out, uint32_t chunkLength, uint32_t, std::basic_istream<unsigned char>& dataStream)
{
	const size_t HEADER_SIZE = 13;
	if (chunkLength != HEADER_SIZE) return false;

	auto crcPos = dataStream.tellg();

	// �摜�T�C�Y
	out->width  = ReadUint32(dataStream);
	out->height = ReadUint32(dataStream);

	// �r�b�g�[�x(�e�s�N�Z���̐F�\���ɉ�bit�g����)
	out->bitDepth = static_cast<unsigned>(dataStream.get());

	// �J���[�^�C�v
	out->colorType = dataStream.get();

	// ���k���� / �t�B���^�[����
	if (dataStream.get() != 0) return false;
	if (dataStream.get() != 0) return false;

	// �C���^�[���X�L���i�C���^�[���X�ɂ͍���Ή����Ȃ��j
	if (dataStream.get() != 0) return false;

	// todo: crc�`�F�b�N
	Skip(dataStream, 4);
	return true;
}

//@brief IDAT�`�����N��ǂݏo��
//-------------------------------------------------------------
bool ReadIDAT(Image::Png::PngFileData* out, uint32_t chunkLength, uint32_t, std::basic_istream<unsigned char>& dataStream)
{
	// �f�[�^���ǂݏo��
	auto zlibData = ReadRange(dataStream, chunkLength);

	// �A��
	// note: zlib�̉𓀂́A�ǂݏo�����f�[�^���ׂĂ�A����������ɍs��
	std::copy(zlibData.begin(), zlibData.end(), std::back_inserter(out->zlibData));

	// todo: crc�`�F�b�N
	Skip(dataStream, 4);
	return true;
}

//@brief IEND�`�����N��ǂݏo��
//-------------------------------------------------------------
bool ReadIEND(uint32_t, uint32_t, std::basic_istream<unsigned char>& dataStream)
{
	// todo: crc�`�F�b�N
	Skip(dataStream, 4);
	return true;
}
} // end namespace

//@brief Png�̃��[�h
//-------------------------------------------------------------
bool Image::Png::Load(PngFileData* out, const TCHAR* filepath)
{
	std::basic_ifstream<unsigned char> ifs(filepath, std::ios::in | std::ios::binary);
	if (!ifs) return false;

	return Load(out,ifs);
}
//-------------------------------------------------------------
bool Image::Png::Load(PngFileData* out, const unsigned char *dataBuffer, size_t size)
{
	// hack: �������o�C�i���X�g���[���Ƃ��Ďg��
	std::basic_string<unsigned char> streamBuffer;
	streamBuffer.resize(size);
	std::copy(dataBuffer, &dataBuffer[size], streamBuffer.begin());

	std::basic_istringstream<unsigned char> stream(streamBuffer, std::ios_base::in | std::ios_base::binary);
	return Load(out, stream);
}
//-------------------------------------------------------------
bool Image::Png::Load(PngFileData* out, std::basic_istream<unsigned char>& dataStream)
{
	const size_t CRC_SIZE = 4;

	// �V�O�l�`�����m�F
	if (!IsPngSignature(dataStream))
		return false;

	// ��͏���
	std::vector<unsigned char> totalZlibData;
	while (!dataStream.eof())
	{
		// type��CRC���W�X�^�������q�Ƃ��Ďg�����߁A�����ă��t���N�g���ꂽ��ԂŎ󂯎��
		uint32_t chunkLength = ReadUint32(dataStream);
		uint32_t chunkType   = ReadReflectedUint32(dataStream); 

		// �I���`�����N
		if (chunkType == ReflectChunkType::IEND)
		{
			return ReadIEND(chunkLength, chunkType, dataStream);
		}
		// ���̑��̃f�[�^�`�����N
		switch (chunkType)
		{
		case ReflectChunkType::IHDR:
			if (!ReadIHDR(out, chunkLength, chunkType, dataStream)) return false;
			break;
		case ReflectChunkType::IDAT:
			if (!ReadIDAT(out, chunkLength, chunkType, dataStream)) return false;
			break;
		default:
			Skip(dataStream, chunkLength + 4/*(CRC Size)*/); break;
		}
	}

	// �I���`�����N���������Ȃ�����
	return false;
}

//@brief �s�N�Z�����т̃f�[�^��𐶐�����
//-------------------------------------------------------------
std::vector<unsigned char> Image::Png::ToPixelArrayRGBA(const PngFileData& pngData)
{
	PngDataParser parser;
	return parser.ToPixelArrayRGBA(pngData);
}