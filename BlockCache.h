// $Id$

#ifndef BLOCKCACHE_H_INC
#define BLOCKCACHE_H_INC

#include "LargeFileReader.h"

template<int nBufSize>
class BlockCache {
public:
	BlockCache(int nMaxBlocks, LargeFileReader* pLFReader);
	~BlockCache();

	bool setMaxBlocks(int nMaxBlocks);
	int  getMaxBlocks() const { return m_nMaxBlocks; }

	


private:
	int m_nMaxBlocks;
	LargeFileReader* m_pLFReader;
	BYTE* m_pCachedBlocks;

	struct CacheInfo {
		filesize_t m_qAddress;
		struct CacheInfo* m_pNext;

	};

};

#endif // BLOCK_CACHE_H_INC
