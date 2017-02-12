//-------------------------------------------------------------
//! @brief	接頭符号ツリー
//! @author	ｹｰﾄｩｽ=ｶｴﾚｽﾃｨｽ
//-------------------------------------------------------------
#pragma once

//-------------------------------------------------------------
// include
//-------------------------------------------------------------
#include <vector>
#include <bitset>
#include <array>

namespace MyUtility
{
namespace PrefixC
{
//-------------------------------------------------------------
// constant
//-------------------------------------------------------------
using index_t		  = int;
using ChildIndexArray = std::array<index_t, 2>;

static constexpr index_t invalidIndex = -1;

//-------------------------------------------------------------
// class (接頭符号ツリー)
//-------------------------------------------------------------
template<typename T>
class BasicPrefixCTree
{
public:

	//! ノードクラス
	class Node;

	//! ツリーをたどるクラス
	class TreeWalker;	

	//! ランダムアクセス
	template<unsigned numBit>
	Node& operator[] (std::bitset<numBit> key);

	//! 符号とそれに対応する値を登録する
	bool Entry(unsigned bitset, T value, unsigned numRead);

	template<unsigned numBit>
	bool Entry(std::bitset<numBit> bitset, T value, unsigned numRead = numBit);

	//! コンストラクタ
	BasicPrefixCTree()
	{
		//! 先頭ノードの作成
		m_nodeList.push_back(Node{});
		m_nodeTop = 0;
	}

private:

	std::vector<Node>		m_nodeList;
	index_t					m_nodeTop = invalidIndex;

	template<unsigned numBit>
	std::pair<Node&, bool> CreateNodeIfNotFound(std::bitset<numBit> key, unsigned readBit);
};

//-------------------------------------------------------------
// class (接頭符号ツリーのノード)
//-------------------------------------------------------------
template<typename T>
class BasicPrefixCTree<T>::Node
{
	friend class BasicPrefixCTree<T>;

public:
	//! メンバアクセス
	T    GetValue() const noexcept { return m_value; }
	void SetValue(T val) noexcept;

	//! このノードが値を持っているか
	bool HasValue() const noexcept { return m_hasvalue; }

	//! 子へのIndexを返す
	index_t ChildIndex(int bit) const;

private:

	T				m_value = T{};
	bool			m_hasvalue = false;
	ChildIndexArray	m_childIndex{ invalidIndex ,invalidIndex };
};

//-------------------------------------------------------------
// helpler class (ツリーをたどるクラス)
//-------------------------------------------------------------
template<typename T>
class BasicPrefixCTree<T>::TreeWalker
{
	friend class BasicPrefixCTree<T>;

public:

	//! 現在指しているノードを返す
	const typename BasicPrefixCTree<T>::Node& Get() const;

	//! 現在指しているノードへアクセス
	const typename BasicPrefixCTree<T>::Node* operator->() const { return &Get(); }

	//! 指定した分岐先に子が存在するか
	bool HasChild(int bit) const;

	//! 移動
	bool Next(int nextBit);

	//! コンストラクタ
	TreeWalker(const BasicPrefixCTree& tree)
		:m_tree(tree)
		, m_current(tree.m_nodeTop)
	{}

private:

	const BasicPrefixCTree&	m_tree;
	index_t					m_current;


	index_t GetNextIndex(int bit) const;
};

//-------------------------------------------------------------
// interface (ビットストリームインターフェース)
//-------------------------------------------------------------
class IbitStream
{
public:
	//! 1ビットロードする
	virtual IbitStream& operator >> (int& out) = 0;

	//! 終端か
	virtual bool Eof() const = 0;

	virtual ~IbitStream() {};
};

//-------------------------------------------------------------
// helpler function
//-------------------------------------------------------------

// @brief ビット列を１単位デコードする
//-------------------------------------------------------------
template<typename T>
inline bool Decode(IbitStream& stream, const BasicPrefixCTree<T>& tree, T* out)
{
	auto walker = BasicPrefixCTree<T>::TreeWalker(tree);
	while(!stream.Eof())
	{
		int bit;
		stream >> bit;

		// 対応する符号が見つからない
		if (walker.HasChild(bit) == false) break;

		walker.Next(bit);

		// 見つかった
		if (walker->HasValue())
		{
			*out = walker->GetValue();
			return true;		
		}
	}
	return false;
}

//-------------------------------------------------------------
// alias
//-------------------------------------------------------------
using PrefixCodeTree = BasicPrefixCTree<unsigned>;

//-------------------------------------------------------------
// implement
//-------------------------------------------------------------

// @brief 値を設定
//-------------------------------------------------------------
template<typename T>
inline void BasicPrefixCTree<T>::Node::SetValue(T val) noexcept
{
	m_value = val;
	m_hasvalue = true;
}

// @brief 子へのIndexを返す
// @note  値はBasicPrefixCTreeの持つvectorへのIndexを指す
//-------------------------------------------------------------
template<typename T>
inline index_t BasicPrefixCTree<T>::Node::ChildIndex(int bit) const
{
	return m_childIndex.at(bit);
}

// @brief ランダムアクセス風
// @note  ランダムアクセスではない
//-------------------------------------------------------------
template<typename T>
template<unsigned numBit>
typename BasicPrefixCTree<T>::Node& BasicPrefixCTree<T>::operator[] (std::bitset<numBit> key)
{
	return CreateNodeIfNotFound(key).first;
}

// @brief	符号とそれに対応する値を登録する
//-------------------------------------------------------------
template<typename T>
bool BasicPrefixCTree<T>::Entry(unsigned bitset, T value, unsigned numRead)
{
	constexpr auto CAPACITY_LENGTH = std::numeric_limits<unsigned>::digits;
	if (numRead > CAPACITY_LENGTH)
	{
		throw std::runtime_error("ビット長さの指定が不正です");
	}

	using Bitset = std::bitset<CAPACITY_LENGTH>;
	return Entry(Bitset(bitset), value, numRead);
}
//-------------------------------------------------------------
template<typename T>
template<unsigned numBit>
bool BasicPrefixCTree<T>::Entry(std::bitset<numBit> bitset, T value, unsigned numRead)
{
	auto result = CreateNodeIfNotFound(bitset, numRead);
	if (result.second == false) 
		return false;

	result.first.SetValue(value);
	return true;
}

// @brief 指定された接頭符号に対応するノードを返す
//-------------------------------------------------------------
template<typename T>
template<unsigned numBit>
std::pair<typename BasicPrefixCTree<T>::Node&, bool> BasicPrefixCTree<T>::CreateNodeIfNotFound(std::bitset<numBit> bits, unsigned numRead)
{
	TreeWalker walker(*this);

	// 先頭ビットから順に読む
	int i = static_cast<int>(numRead) - 1;

	// ツリーをたどって、指定されたkeyにすでにノードが存在するかを確認
	for (; i >= 0; --i)
	{
		if (walker.HasChild(bits[i]) == false)
		{
			break;
		}
		walker.Next(bits[i]);
	}
	if(i < 0)
		return std::pair<Node&, bool>(m_nodeList.at(walker.m_current), false);

	// (ノードが作られていなかった続きから) 新しくノードを作成
	for (; i >= 0; --i)
	{
		index_t newIndex = static_cast<index_t>(m_nodeList.size());
		m_nodeList.push_back(Node{});

		auto& node = m_nodeList.at(walker.m_current);
		node.m_childIndex[(bits[i])] = newIndex;

		walker.Next(bits[i]);
	}
	return std::pair<Node&, bool>(m_nodeList.at(walker.m_current), true);
}

// @brief 現在指しているノードを返す
//-------------------------------------------------------------
template<typename T>
inline const typename typename BasicPrefixCTree<T>::Node& BasicPrefixCTree<T>::TreeWalker::Get() const
{
	return m_tree.m_nodeList.at(m_current);
}

// @brief 指定した分岐先に子が存在するか
//-------------------------------------------------------------
template<typename T>
inline bool BasicPrefixCTree<T>::TreeWalker::HasChild(int bit) const
{
	return GetNextIndex(bit) != invalidIndex;
}

// @brief 移動
//-------------------------------------------------------------
template<typename T>
inline bool BasicPrefixCTree<T>::TreeWalker::Next(int nextBit)
{
	m_current = GetNextIndex(nextBit);
	return m_current != invalidIndex;
}

// @brief 指定した分岐先の子を指すIndexを返す
//-------------------------------------------------------------
template<typename T>
inline index_t BasicPrefixCTree<T>::TreeWalker::GetNextIndex(int bit) const
{
	return Get().ChildIndex(bit);
}

}// end namespace

 //-------------------------------------------------------------
 // interface (デフォルトビットストリームインターフェース)
 //-------------------------------------------------------------
class DefaultBitStream : public PrefixC::IbitStream
{
public:

	explicit DefaultBitStream(const unsigned char* binary, size_t numByte)
		:m_binary(binary)
		,m_numByte(numByte)
	{}

	//! 1ビットロードする
	IbitStream& operator >> (int& out) override
	{
		out = Get();
		return *this;
	}
	//! 終端か
	bool Eof() const noexcept override
	{
		return m_nextByte >= m_numByte;
	}
	//! １ビットロード (戻り値に値を返す)
	int Get()
	{
		int bit = GetBitImpl();
		Next();
		return bit;
	}

	//! ビット列をロード
	//! 符号ではないデータは 要素の最下位ビットから順にパックされている
	unsigned GetRange(unsigned numbit)
	{
		int bit = 0;
		for (unsigned i = 0; i < numbit; ++i)
		{
			bit |= (Get() << i);
		}
		return bit;
	}
	//! ビット列をロード
	//! 符号化されたデータは最上位ビットから順にパックされている
	unsigned GetCodedRange(unsigned numbit)
	{
		int bit = 0;
		for (unsigned i = 0; i < numbit; ++i)
		{
			bit <<= 1;
			bit |= Get();
		}
		return bit;
	}
	//! 次のByte境界までスキップ
	void SkipByte() noexcept
	{
		if (m_nextBit != 0)
		{
			m_nextBit = 0;
			m_nextByte++;
		}
	}

private:

	//! 現在見ているbitを抜き出す
	int GetBitImpl() const
	{
		const int byte = m_binary[m_nextByte];
		const int maskedByte = byte & (0x01 << m_nextBit);

		return (maskedByte == 0) ? 0 : 1;
	}

	//! 次のビットにシフト
	void Next() noexcept
	{
		++m_nextBit;
		if (m_nextBit >= 8)
		{
			m_nextBit = 0;
			++m_nextByte;
		}
	}

	// note:
	// 読むだけで、メモリ確保はしないことにする
	// バイナリデータの生存期間に注意
	const unsigned char*	m_binary;
	size_t					m_numByte;

	unsigned				m_nextBit = 0;	// 次に読むbit
	size_t					m_nextByte = 0;	// 次に読むByte
};

 //-------------------------------------------------------------
 // using
 //-------------------------------------------------------------
using PrefixC::PrefixCodeTree;

}// end namespace