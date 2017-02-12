#include <iostream>
#include <vector>
#include <string>

#include "MyUtility\ImagePNG.h"

int main()
{
	// 白色 1x1 pixelのpng画像
	const unsigned char rawData[] = "\x89\x50\x4E\x47\x0D\x0A\x1A\x0A\x00\x00\x00\x0D\x49\x48\x44\x52\x00\x00\x00\x01\x00\x00\x00\x01\x08\x06\x00\x00\x00\x1F\x15\xC4\x89\x00\x00\x00\x04\x67\x41\x4D\x41\x00\x00\xB1\x8F\x0B\xFC\x61\x05\x00\x00\x00\x09\x70\x48\x59\x73\x00\x00\x16\x25\x00\x00\x16\x25\x01\x49\x52\x24\xF0\x00\x00\x00\x0D\x49\x44\x41\x54\x18\x57\x63\xF8\xFF\xFF\xFF\x7F\x00\x09\xFB\x03\xFD\x05\x43\x45\xCA\x00\x00\x00\x00\x49\x45\x4E\x44\xAE\x42\x60\x82";

	// ここからpngパース
	MyUtility::Image::Png::PngFileData data;
	if (Load(&data, rawData, std::size(rawData)))
	{
		std::cout << "--- 読み出し成功 ---" << std::endl;

		// 結果を表示
		std::cout << "width:" << data.width << std::endl;
		std::cout << "height:" << data.height << std::endl;

		auto pixelArray = ToPixelArrayRGBA(data);
		for (auto byte : pixelArray)
		{
			std::cout << static_cast<int>(byte) << " ";
		}
	}
	else
	{
		std::cout << "--- 読み出し失敗 ---" << std::endl;
	}
	getchar();
	return 0;
}

