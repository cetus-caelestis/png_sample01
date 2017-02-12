//-------------------------------------------------------------
//! @brief	pngイメージ
//! @author	ｹｰﾄｩｽ=ｶｴﾚｽﾃｨｽ
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

//@brief ビット列をロード(ビッグエンディアン版)
//-------------------------------------------------------------
unsigned GetBigEndianRange(PNGBitStream& stream, unsigned numbit)
{
	// note: いま読んでいる位置がバイト境界でないと正しく機能しない
	unsigned bit = 0;
	for (unsigned i = 0; i < numbit; i += 8)
	{
		unsigned subCount = numbit - i;
		if (subCount > 8) subCount = 8;

		bit = (bit << 8) | stream.GetRange(subCount);
	}
	return bit;
}

//@brief 32bit数値を取得
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

//@brief Reflectされた32bit数値を取得
//-------------------------------------------------------------
uint32_t ReadReflectedUint32(std::basic_istream<unsigned char>& stream)
{
	// 逆順に読み出す
	uint32_t number = 0;
	number |= static_cast<uint32_t>(stream.get());
	number |= static_cast<uint32_t>(stream.get()) << 8;
	number |= static_cast<uint32_t>(stream.get()) << 16;
	number |= static_cast<uint32_t>(stream.get()) << 24;
	return number;
}


//@brief 範囲を返す
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
//@brief スキップする
//-------------------------------------------------------------
void Skip(std::basic_istream<unsigned char>& stream, size_t length)
{
	for (size_t i = 0; i < length && !stream.eof(); ++i)
	{
		stream.get();
	}
}

//@brief pngファイルを表すシグネチャをチェック
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
//@brief IHDRチャンクを読み出す
//-------------------------------------------------------------
bool ReadIHDR(Image::Png::PngFileData* out, uint32_t chunkLength, uint32_t, std::basic_istream<unsigned char>& dataStream)
{
	const size_t HEADER_SIZE = 13;
	if (chunkLength != HEADER_SIZE) return false;

	auto crcPos = dataStream.tellg();

	// 画像サイズ
	out->width  = ReadUint32(dataStream);
	out->height = ReadUint32(dataStream);

	// ビット深度(各ピクセルの色表現に何bit使うか)
	out->bitDepth = static_cast<unsigned>(dataStream.get());

	// カラータイプ
	out->colorType = dataStream.get();

	// 圧縮方式 / フィルター方式
	if (dataStream.get() != 0) return false;
	if (dataStream.get() != 0) return false;

	// インターレス有無（インターレスには今回対応しない）
	if (dataStream.get() != 0) return false;

	// todo: crcチェック
	Skip(dataStream, 4);
	return true;
}

//@brief IDATチャンクを読み出す
//-------------------------------------------------------------
bool ReadIDAT(Image::Png::PngFileData* out, uint32_t chunkLength, uint32_t, std::basic_istream<unsigned char>& dataStream)
{
	// データ部読み出し
	auto zlibData = ReadRange(dataStream, chunkLength);

	// 連結
	// note: zlibの解凍は、読み出したデータすべてを連結させた後に行う
	std::copy(zlibData.begin(), zlibData.end(), std::back_inserter(out->zlibData));

	// todo: crcチェック
	Skip(dataStream, 4);
	return true;
}

//@brief IENDチャンクを読み出す
//-------------------------------------------------------------
bool ReadIEND(uint32_t, uint32_t, std::basic_istream<unsigned char>& dataStream)
{
	// todo: crcチェック
	Skip(dataStream, 4);
	return true;
}
} // end namespace

//@brief Pngのロード
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
	// hack: 無理やりバイナリストリームとして使う
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

	// シグネチャを確認
	if (!IsPngSignature(dataStream))
		return false;

	// 解析処理
	std::vector<unsigned char> totalZlibData;
	while (!dataStream.eof())
	{
		// typeはCRCレジスタ初期化子として使うため、あえてリフレクトされた状態で受け取る
		uint32_t chunkLength = ReadUint32(dataStream);
		uint32_t chunkType   = ReadReflectedUint32(dataStream); 

		// 終了チャンク
		if (chunkType == ReflectChunkType::IEND)
		{
			return ReadIEND(chunkLength, chunkType, dataStream);
		}
		// その他のデータチャンク
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

	// 終了チャンクが見つけられなかった
	return false;
}

//@brief ピクセル並びのデータ列を生成する
//-------------------------------------------------------------
std::vector<unsigned char> Image::Png::ToPixelArrayRGBA(const PngFileData& pngData)
{
	PngDataParser parser;
	return parser.ToPixelArrayRGBA(pngData);
}