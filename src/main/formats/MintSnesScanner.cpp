#include "stdafx.h"
#include "MintSnesScanner.h"
#include "MintSnesSeq.h"
#include "SNESDSP.h"

//; Gokinjo Boukentai SPC
//0c3c: 1c        asl   a                 ; song index in A
//0c3d: fd        mov   y,a
//0c3e: f6 fe 11  mov   a,$11fe+y
//0c41: c4 04     mov   $04,a
//0c43: f6 ff 11  mov   a,$11ff+y
//0c46: c4 05     mov   $05,a             ; set song header ptr
//0c48: 8d 00     mov   y,#$00
//0c4a: f7 04     mov   a,($04)+y         ; read first byte
//0c4c: 10 05     bpl   $0c53
//0c4e: 68 ff     cmp   a,#$ff
//0c50: d0 14     bne   $0c66
//0c52: 6f        ret
BytePattern MintSnesScanner::ptnLoadSeq(
	"\x1c\xfd\xf6\xfe\x11\xc4\x04\xf6"
	"\xff\x11\xc4\x05\x8d\x00\xf7\x04"
	"\x10\x05\x68\xff\xd0\x14\x6f"
	,
	"xxx??x?x"
	"??x?xxx?"
	"xxxxx?x"
	,
	23);

//; Gokinjo Bouken Tai
//02f1: 8f 11 f3  mov   $f3,#$11
//02f4: 8f 6c f2  mov   $f2,#$6c
BytePattern MintSnesScanner::ptnSetDIR(
	"\x8f\x11\xf3\x8f\x6c\xf2"
	,
	"x?xxxx"
	,
	6);

void MintSnesScanner::Scan(RawFile* file, void* info)
{
	uint32_t nFileLength = file->size();
	if (nFileLength == 0x10000)
	{
		SearchForMintSnesFromARAM(file);
	}
	else
	{
		SearchForMintSnesFromROM(file);
	}
	return;
}

void MintSnesScanner::SearchForMintSnesFromARAM(RawFile* file)
{
	MintSnesVersion version = MINTSNES_NONE;
	std::wstring name = file->tag.HasTitle() ? file->tag.title : RawFile::removeExtFromPath(file->GetFileName());

	// scan for song list table
	UINT ofsLoadSeq;
	uint16_t addrSongList;
	if (file->SearchBytePattern(ptnLoadSeq, ofsLoadSeq)) {
		addrSongList = file->GetShort(ofsLoadSeq + 3);
	}
	else {
		return;
	}

	// TODO: guess song index
	int8_t guessedSongIndex = -1;
	if (addrSongList + 2 <= 0x10000) {
		guessedSongIndex = 1;
	}

	// scan DIR address
	UINT ofsSetDIR;
	uint16_t spcDirAddr = 0;
	if (file->SearchBytePattern(ptnSetDIR, ofsSetDIR)) {
		spcDirAddr = file->GetByte(ofsSetDIR + 1) << 8;
	}

	UINT addrSongHeaderPtr = addrSongList + guessedSongIndex * 2;
	if (addrSongHeaderPtr + 2 <= 0x10000) {
		uint16_t addrSongHeader = file->GetShort(addrSongHeaderPtr);

		MintSnesSeq* newSeq = new MintSnesSeq(file, version, addrSongHeader, name);
		if (!newSeq->LoadVGMFile()) {
			delete newSeq;
			return;
		}

		if (spcDirAddr != 0) {
			MintSnesInstrSet * newInstrSet = new MintSnesInstrSet(file, version, spcDirAddr, newSeq->InstrumentAddresses, newSeq->InstrumentHints);
			if (!newInstrSet->LoadVGMFile())
			{
				delete newInstrSet;
				return;
			}
		}
	}
}

void MintSnesScanner::SearchForMintSnesFromROM(RawFile* file)
{
}
