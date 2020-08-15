#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <algorithm>

#ifdef _WIN32
#include <Windows.h>
//#include "wingdi.h"
// BITMAPFILEHEADER
// BITMAPINFOHEADER
#endif


typedef size_t sizeint;

#define MAKE_ALPHA_UNICASE(c) ((c) & ~('A' ^ 'a'))
#define MACRO_MAX(a, b) ((a) > (b) ? (a) : (b))


#if defined(__GNUC__)
#define __IN_RANGE_TYPENAME__ typename
#else
#define __IN_RANGE_TYPENAME__
#endif
#define IN_RANGE(Value, Min, Max) ((__IN_RANGE_TYPENAME__ _sized_unsigned<MACRO_MAX(sizeof(Value), sizeof(Min))>::type)((__IN_RANGE_TYPENAME__ _sized_unsigned<MACRO_MAX(sizeof(Value), sizeof(Min))>::type)(Value) - (__IN_RANGE_TYPENAME__ _sized_unsigned<MACRO_MAX(sizeof(Value), sizeof(Min))>::type)(Min)) < (__IN_RANGE_TYPENAME__ _sized_unsigned<MACRO_MAX(sizeof(Max), sizeof(Min))>::type)((__IN_RANGE_TYPENAME__ _sized_unsigned<MACRO_MAX(sizeof(Max), sizeof(Min))>::type)(Max) - (__IN_RANGE_TYPENAME__ _sized_unsigned<MACRO_MAX(sizeof(Max), sizeof(Min))>::type)(Min)))

template <typename TElementType, size_t tsiElementsCount>
TElementType(*__ARRAY_SIZE_HELPER(TElementType(&a)[tsiElementsCount]))[tsiElementsCount];
#define ARRAY_SIZE(a) (sizeof(*__ARRAY_SIZE_HELPER(a)) / sizeof((*__ARRAY_SIZE_HELPER(a))[0]))


template<size_t tsiTypeSize>
struct _sized_unsigned;

template<>
struct _sized_unsigned<sizeof(int8_t)>
{
	typedef uint8_t type;
};

template<>
struct _sized_unsigned<sizeof(int16_t)>
{
	typedef uint16_t type;
};

template<>
struct _sized_unsigned<sizeof(int32_t)>
{
	typedef uint32_t type;
};

template<>
struct _sized_unsigned<sizeof(int64_t)>
{
	typedef uint64_t type;
};



enum
{
	SOURCE_LINE_CHAR_COUNT		= 300,
	
	DESTINATIONLINE_WIDTH		= 1000,
	DESTINATION_BYTES_PER_PIXEL	= 3,
};

#define BITMAP_LINE_ALIGNMENT		sizeof(uint32_t)


#define NEWLINE_CHARACTER		'\n'
#define CARRIAGE_RETURN_CHARACTER '\r'
#define FIRST_READ_MAP_CHAR		'a'
#define FIRST_WRITE_MAP_CHAR	's'

static const char g_ascOperationMapStartMarker[] = "Operation sequence map:";

static const uint8_t g_auiInvalidWritePixel[DESTINATION_BYTES_PER_PIXEL] = { 0xFF, 0xFF, 0xFF, };
static const uint8_t g_auiInvalidReadPixel[DESTINATION_BYTES_PER_PIXEL] = { 0x7F, 0x7F, 0x7F, };

#define COLOR_COMPONENT_STEP		25
#define COLOR_COMPONENT_BASE		24
#define MAKE_COLOR_COMPONENT(uiThreadIndex) ((uiThreadIndex + 1) * COLOR_COMPONENT_STEP % (256 - COLOR_COMPONENT_BASE) + COLOR_COMPONENT_BASE)

static const uint8_t g_auiWritePixelStep[DESTINATION_BYTES_PER_PIXEL] = { COLOR_COMPONENT_STEP, 0x0, COLOR_COMPONENT_STEP, };
static const uint8_t g_auiSingleReadPixelStep[DESTINATION_BYTES_PER_PIXEL] = { COLOR_COMPONENT_STEP, COLOR_COMPONENT_STEP, 0x0, };
static const uint8_t g_auiSharedReadPixelStep[DESTINATION_BYTES_PER_PIXEL] = { 0x0, COLOR_COMPONENT_STEP, COLOR_COMPONENT_STEP, };

static const char g_ascdestinationExtension[] = ".bmp";

#ifdef _WIN32
#define PATH_SEPARATOR_CHAR '\\'
#else
#define PATH_SEPARATOR_CHAR '/'
#endif


class CBMPGenProgram
{
public:
	static bool Execute(int iArgumentCount, char *apszArgumentValues[]);

private:
	struct CProgramArguments
	{
		char		*m_szInputFilePath;
	};

	struct CConversionContext
	{
		CConversionContext() : 
			m_psfSourceFile(NULL), 
			m_psfDestinationFile(NULL), 
			m_uiBufferSizeParsed(0),
			m_uiBufferSizeUsed(0), 
			m_siCurrentSourceLineIndex(0)
		{
		}

		~CConversionContext() { Finalize(); }
		
		bool Initialize(const CProgramArguments &paProgramArguments);
		void Finalize();

		bool ReadNextSourceLine(const char *&szOutNextLine, bool &bOutEOFReached);
		bool FindNextLineInTheCachedData(const char *&szOutNextLine);

		sizeint GetCurrentSourceLineIndex() const { return m_siCurrentSourceLineIndex; }

		FILE		*m_psfSourceFile;
		FILE		*m_psfDestinationFile;
		sizeint		m_siCurrentSourceLineIndex;
		unsigned	m_uiBufferSizeParsed;
		unsigned	m_uiBufferSizeUsed;
		char		m_ascReadBuffer[SOURCE_LINE_CHAR_COUNT + 2];
	};

	struct CRunningMapState
	{
		CRunningMapState() : m_uiOpenReadCount(0), m_bWriteOpened(false) {}
		~CRunningMapState() { assert(m_uiOpenReadCount == 0); assert(!m_bWriteOpened); }

		unsigned	m_uiOpenReadCount;
		bool		m_bWriteOpened;
	};

private:
	static bool ParseProgramArguments(CProgramArguments &paProgramArguments, int iArgumentCount, char *apszArgumentValues[]);
	static bool PerformConversionPrepared(CConversionContext &ccRefWorkContext);
	static bool SkipSourceHeaderLines(CConversionContext &ccRefWorkContext);
	static bool SkipDestinationHeaders(CConversionContext &ccRefWorkContext);
	static bool ConvertMapData(CConversionContext &ccRefWorkContext, sizeint &siOutBitmapLines);
	static bool ConvertSingleSourceLine(const char *szSourceLine, CRunningMapState &msRefMapState,
		unsigned &uiVarGeneratedPixelCount, uint8_t auiDestinationLine[][DESTINATION_BYTES_PER_PIXEL], unsigned uiMaximalPixelCount, sizeint siLineIndex);
	static bool WriteDestinationHeaders(CConversionContext &ccRefWorkContext, sizeint siBitmapLines);

private:
	static void PrintError(const char *szErrorText, ...);
};


/*static */
bool CBMPGenProgram::Execute(int iArgumentCount, char *apszArgumentValues[])
{
	bool bResult = false;
	
	do
	{
		CProgramArguments paProgramArguments;
		
		if (!ParseProgramArguments(paProgramArguments, iArgumentCount, apszArgumentValues))
		{
			break;
		}

		CConversionContext ccWorkContext;
		if (!ccWorkContext.Initialize(paProgramArguments))
		{
			break;
		}
		
		if (!PerformConversionPrepared(ccWorkContext))
		{
			break;
		}
		
		bResult = true;
	}
	while (false);
	
	return bResult;
}


bool CBMPGenProgram::CConversionContext::Initialize(const CProgramArguments &paProgramArguments)
{
	bool bResult = false;
	
	assert(m_psfSourceFile == NULL);

	do
	{
		if (fopen_s(&m_psfSourceFile, paProgramArguments.m_szInputFilePath, "rb") != 0)
		{
			PrintError("Failed to open source file with the path %s\n", paProgramArguments.m_szInputFilePath);
			break;
		}

		sizeint nPathLength = strlen(paProgramArguments.m_szInputFilePath);
		char *pscDestinationPath = new char[nPathLength + ARRAY_SIZE(g_ascdestinationExtension)];
		
		char *pscDotPosition = strrchr(paProgramArguments.m_szInputFilePath, '.');
		
		if (pscDotPosition != NULL)
		{
			char *pscSlashPosition = strrchr(paProgramArguments.m_szInputFilePath, PATH_SEPARATOR_CHAR);
			
			if (pscSlashPosition != NULL && pscSlashPosition > pscDotPosition)
			{
				pscDotPosition = paProgramArguments.m_szInputFilePath + nPathLength;
			}
		}
		else
		{
			pscDotPosition = paProgramArguments.m_szInputFilePath + nPathLength;
		}

		sizeint siPathCopySize = pscDotPosition - paProgramArguments.m_szInputFilePath;
		memcpy(pscDestinationPath, paProgramArguments.m_szInputFilePath, siPathCopySize);
		memcpy(pscDestinationPath + siPathCopySize, g_ascdestinationExtension, sizeof(g_ascdestinationExtension));

		if (fopen_s(&m_psfDestinationFile, pscDestinationPath, "w+b") != 0)
		{
			PrintError("Failed to open destination file with the path %s\n", pscDestinationPath);
			delete[] pscDestinationPath;
			break;
		}

		delete[] pscDestinationPath;

		m_siCurrentSourceLineIndex = 0;
		m_uiBufferSizeParsed = 0;
		m_uiBufferSizeUsed = 0;

		bResult = true;
	}
	while (false);
	
	return bResult;
}

void CBMPGenProgram::CConversionContext::Finalize()
{
	if (m_psfDestinationFile != NULL)
	{
		fclose(m_psfDestinationFile);
		m_psfDestinationFile = NULL;
	}

	if (m_psfSourceFile != NULL)
	{
		fclose(m_psfSourceFile);
		m_psfSourceFile = NULL;
	}
}


bool CBMPGenProgram::CConversionContext::ReadNextSourceLine(const char *&szOutNextLine, bool &bOutEOFReached)
{
	bool bResult = false;
	
	do
	{
		if (!FindNextLineInTheCachedData(szOutNextLine))
		{
			sizeint siBytesRead = fread_s(m_ascReadBuffer + m_uiBufferSizeUsed, ARRAY_SIZE(m_ascReadBuffer) - m_uiBufferSizeUsed, 1, ARRAY_SIZE(m_ascReadBuffer) - m_uiBufferSizeUsed, m_psfSourceFile);

			if (siBytesRead == 0)
			{
				bOutEOFReached = feof(m_psfSourceFile) != 0;
				break;
			}

			m_uiBufferSizeUsed += (unsigned)siBytesRead;

			if (!FindNextLineInTheCachedData(szOutNextLine))
			{
				bOutEOFReached = false;
				break;
			}
		}
		
		bResult = true;
	}
	while (false);
	
	return bResult;
}

bool CBMPGenProgram::CConversionContext::FindNextLineInTheCachedData(const char *&szOutNextLine)
{
	bool bResult = false;
	
	do
	{
		unsigned uiCacheSizeRemainder = m_uiBufferSizeUsed - m_uiBufferSizeParsed;

		if (uiCacheSizeRemainder == 0)
		{
			m_uiBufferSizeUsed = m_uiBufferSizeParsed = 0;
			break;
		}

		char *pscCacheDataStart = m_ascReadBuffer + m_uiBufferSizeParsed;

		char *pscNewlinePosition = (char *)memchr(pscCacheDataStart, NEWLINE_CHARACTER, uiCacheSizeRemainder);
		if (pscNewlinePosition == NULL)
		{
			if (m_uiBufferSizeParsed != 0)
			{
				memmove(m_ascReadBuffer, pscCacheDataStart, uiCacheSizeRemainder);
				m_uiBufferSizeParsed = 0;
				m_uiBufferSizeUsed = uiCacheSizeRemainder;
			}

			break;
		}

		pscNewlinePosition[0] = '\0';
		if (pscNewlinePosition != pscCacheDataStart && pscNewlinePosition[-1] == CARRIAGE_RETURN_CHARACTER)
		{
			pscNewlinePosition[-1] = '\0';
		}

		m_uiBufferSizeParsed += (unsigned)(pscNewlinePosition - pscCacheDataStart) + 1;
		++m_siCurrentSourceLineIndex;
		
		szOutNextLine = pscCacheDataStart;
		
		bResult = true;
	}
	while (false);
	
	return bResult;
}


/*static */
bool CBMPGenProgram::ParseProgramArguments(CProgramArguments &paProgramArguments, int iArgumentCount, char *apszArgumentValues[])
{
	bool bResult = false;

	do
	{
		if (iArgumentCount < 2)
		{
			PrintError("Usage: %s <MapFilePath>\n", apszArgumentValues[0]);
			break;
		}

		paProgramArguments.m_szInputFilePath = apszArgumentValues[1];

		bResult = true;
	}
	while (false);

	return bResult;
}

/*static */
bool CBMPGenProgram::PerformConversionPrepared(CConversionContext &ccRefWorkContext)
{
	bool bResult = false;
	
	do
	{
		if (!SkipSourceHeaderLines(ccRefWorkContext))
		{
			break;
		}

		if (!SkipDestinationHeaders(ccRefWorkContext))
		{
			break;
		}

		sizeint siBitmapLines;
		if (!ConvertMapData(ccRefWorkContext, siBitmapLines))
		{
			break;
		}

		if (!WriteDestinationHeaders(ccRefWorkContext, siBitmapLines))
		{
			break;
		}
		
		bResult = true;
	}
	while (false);
	
	return bResult;
}

/*static */
bool CBMPGenProgram::SkipSourceHeaderLines(CConversionContext &ccRefWorkContext)
{
	bool bAnyFault = false;

	for (;;)
	{
		bool bEOFReached;
		const char *szNextLine;
		if (!ccRefWorkContext.ReadNextSourceLine(szNextLine, bEOFReached))
		{
			PrintError(!bEOFReached ? "Failed parsing source line %zu while skipping header\n" : "End of file reached skipping source header at line %zu\n", ccRefWorkContext.GetCurrentSourceLineIndex());
			bAnyFault = true;
			break;
		}

		if (strcmp(szNextLine, g_ascOperationMapStartMarker) == 0)
		{
			break;
		}
	}

	bool bResult = !bAnyFault;
	return bResult;
}

/*static */
bool CBMPGenProgram::SkipDestinationHeaders(CConversionContext &ccRefWorkContext)
{
	bool bResult = false;
	
	do
	{
		unsigned uiSizeToBeSkipped = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

		if (fseek(ccRefWorkContext.m_psfDestinationFile, uiSizeToBeSkipped, SEEK_SET) != 0)
		{
			PrintError("Failed seeking destination file\n");
			break;
		}
		
		bResult = true;
	}
	while (false);
	
	return bResult;
}

/*static */
bool CBMPGenProgram::ConvertMapData(CConversionContext &ccRefWorkContext, sizeint &siOutBitmapLines)
{
	bool bAnyFault = false;

	sizeint siGeneratedLineCount = 0;
	unsigned uiGeneratedPixelCount = 0;
	CRunningMapState msMapState;

	uint8_t auiDestinationLine[DESTINATIONLINE_WIDTH][DESTINATION_BYTES_PER_PIXEL];
	
	for (;;)
	{
		bool bEOFReached;
		const char *szSourceLine;
		if (!ccRefWorkContext.ReadNextSourceLine(szSourceLine, bEOFReached))
		{
			if (bEOFReached)
			{
				break;
			}

			PrintError("Failed parsing source line %zu while converting data\n", ccRefWorkContext.GetCurrentSourceLineIndex());
			bAnyFault = true;
			break;
		}

		if (!ConvertSingleSourceLine(szSourceLine, msMapState, uiGeneratedPixelCount, auiDestinationLine, ARRAY_SIZE(auiDestinationLine), siGeneratedLineCount))
		{
			break;
		}

		if (uiGeneratedPixelCount == ARRAY_SIZE(auiDestinationLine))
		{
			sizeint siPixelsWritten = fwrite(auiDestinationLine, DESTINATION_BYTES_PER_PIXEL, uiGeneratedPixelCount, ccRefWorkContext.m_psfDestinationFile);
			if (siPixelsWritten != uiGeneratedPixelCount)
			{
				PrintError("Failed writing destination %zu\n", siGeneratedLineCount);
				bAnyFault = true;
				break;
			}

			assert(DESTINATION_BYTES_PER_PIXEL * uiGeneratedPixelCount % BITMAP_LINE_ALIGNMENT == 0);

			uiGeneratedPixelCount = 0;
			++siGeneratedLineCount;
		}
	}

	if (!bAnyFault)
	{
		siOutBitmapLines = siGeneratedLineCount;
	}
	
	bool bResult = !bAnyFault;
	return bResult;
}

/*static */
bool CBMPGenProgram::ConvertSingleSourceLine(const char *szSourceLine, CRunningMapState &msRefMapState, 
	unsigned &uiVarGeneratedPixelCount, uint8_t auiDestinationLine[][DESTINATION_BYTES_PER_PIXEL], unsigned uiMaximalPixelCount, sizeint siLineIndex)
{
	bool bAnyFault = false;
	
	unsigned uiCurrentPixelCount = uiVarGeneratedPixelCount;
	assert(uiCurrentPixelCount < uiMaximalPixelCount);

	for (; ; )
	{
		char cFirstChar = szSourceLine[0];
		if (cFirstChar == '\0')
		{
			break;
		}

		if (uiCurrentPixelCount >= uiMaximalPixelCount)
		{
			PrintError("Bitmap line %zu is too long\n", siLineIndex);
			bAnyFault = true;
			break;
		}

		if (!IN_RANGE(MAKE_ALPHA_UNICASE(cFirstChar), MAKE_ALPHA_UNICASE('A'), MAKE_ALPHA_UNICASE('Z') + 1))
		{
			PrintError("Unexpected first character for destination pixel %u in bitmap line %zu\n", uiCurrentPixelCount, siLineIndex);
			bAnyFault = true;
			break;
		}

		char cSecondChar = szSourceLine[1];
		if (!IN_RANGE(cSecondChar, '0', '9' + 1) && !IN_RANGE(MAKE_ALPHA_UNICASE(cSecondChar), MAKE_ALPHA_UNICASE('A'), MAKE_ALPHA_UNICASE('F') + 1))
		{
			PrintError("Unexpected second character for destination pixel %u in bitmap line %zu\n", uiCurrentPixelCount, siLineIndex);
			bAnyFault = true;
			break;
		}

		char cThirdChar = szSourceLine[2];
		if (cThirdChar != ',' && cThirdChar != '.')
		{
			PrintError("Unexpected separator for destination pixel %u in bitmap line %zu\n", uiCurrentPixelCount, siLineIndex);
			bAnyFault = true;
			break;
		}

		if (MAKE_ALPHA_UNICASE(cFirstChar) >= MAKE_ALPHA_UNICASE(FIRST_WRITE_MAP_CHAR))
		{
			if (cFirstChar >= FIRST_WRITE_MAP_CHAR)
			{
				if (!msRefMapState.m_bWriteOpened)
				{
					PrintError("Unexpected write unlock in pixel %u of bitmap line %zu\n", uiCurrentPixelCount, siLineIndex);

					std::copy(g_auiInvalidWritePixel, g_auiInvalidWritePixel + ARRAY_SIZE(g_auiInvalidWritePixel), auiDestinationLine[uiCurrentPixelCount]);
				}
				else
				{
					unsigned uiThreadIndex = (cFirstChar - FIRST_WRITE_MAP_CHAR) * 16 + (IN_RANGE(cSecondChar, '0', '9' + 1) ? cSecondChar - '0' : MAKE_ALPHA_UNICASE(cSecondChar) - MAKE_ALPHA_UNICASE('A') + 10);
					unsigned uiColorValue = MAKE_COLOR_COMPONENT(uiThreadIndex);
					for (unsigned uiColorIndex = 0; uiColorIndex != ARRAY_SIZE(g_auiWritePixelStep); ++uiColorIndex)
					{
						auiDestinationLine[uiCurrentPixelCount][uiColorIndex] = g_auiWritePixelStep[uiColorIndex] != 0 ? uiColorValue : 0;
					}
				}

				msRefMapState.m_bWriteOpened = false;
			}
			else
			{
				if (msRefMapState.m_bWriteOpened || msRefMapState.m_uiOpenReadCount != 0)
				{
					PrintError("Unexpected write lock in pixel %u of bitmap line %zu\n", uiCurrentPixelCount, siLineIndex);

					std::copy(g_auiInvalidWritePixel, g_auiInvalidWritePixel + ARRAY_SIZE(g_auiInvalidWritePixel), auiDestinationLine[uiCurrentPixelCount]);
				}
				else
				{
					unsigned uiThreadIndex = (MAKE_ALPHA_UNICASE(cFirstChar) - MAKE_ALPHA_UNICASE(FIRST_WRITE_MAP_CHAR)) * 16 + (IN_RANGE(cSecondChar, '0', '9' + 1) ? cSecondChar - '0' : MAKE_ALPHA_UNICASE(cSecondChar) - MAKE_ALPHA_UNICASE('A') + 10);
					unsigned uiColorValue = MAKE_COLOR_COMPONENT(uiThreadIndex);
					for (unsigned uiColorIndex = 0; uiColorIndex != ARRAY_SIZE(g_auiWritePixelStep); ++uiColorIndex)
					{
						auiDestinationLine[uiCurrentPixelCount][uiColorIndex] = g_auiWritePixelStep[uiColorIndex] != 0 ? uiColorValue : 0;
					}
				}

				msRefMapState.m_bWriteOpened = true;
			}
		}
		else
		{
			if (cFirstChar >= FIRST_READ_MAP_CHAR)
			{
				if (msRefMapState.m_uiOpenReadCount < 1)
				{
					PrintError("Unexpected read unlock in pixel %u of bitmap line %zu\n", uiCurrentPixelCount, siLineIndex);

					std::copy(g_auiInvalidReadPixel, g_auiInvalidReadPixel + ARRAY_SIZE(g_auiInvalidReadPixel), auiDestinationLine[uiCurrentPixelCount]);
				}
				else if (msRefMapState.m_uiOpenReadCount > 1)
				{
					unsigned uiThreadIndex = (cFirstChar - FIRST_READ_MAP_CHAR) * 16 + (IN_RANGE(cSecondChar, '0', '9' + 1) ? cSecondChar - '0' : MAKE_ALPHA_UNICASE(cSecondChar) - MAKE_ALPHA_UNICASE('A') + 10);
					unsigned uiColorValue = MAKE_COLOR_COMPONENT(uiThreadIndex);
					for (unsigned uiColorIndex = 0; uiColorIndex != ARRAY_SIZE(g_auiSharedReadPixelStep); ++uiColorIndex)
					{
						auiDestinationLine[uiCurrentPixelCount][uiColorIndex] = g_auiSharedReadPixelStep[uiColorIndex] != 0 ? uiColorValue : 0;
					}
				}
				else
				{
					unsigned uiThreadIndex = (cFirstChar - FIRST_READ_MAP_CHAR) * 16 + (IN_RANGE(cSecondChar, '0', '9' + 1) ? cSecondChar - '0' : MAKE_ALPHA_UNICASE(cSecondChar) - MAKE_ALPHA_UNICASE('A') + 10);
					unsigned uiColorValue = MAKE_COLOR_COMPONENT(uiThreadIndex);
					for (unsigned uiColorIndex = 0; uiColorIndex != ARRAY_SIZE(g_auiSingleReadPixelStep); ++uiColorIndex)
					{
						auiDestinationLine[uiCurrentPixelCount][uiColorIndex] = g_auiSingleReadPixelStep[uiColorIndex] != 0 ? uiColorValue : 0;
					}
				}

				msRefMapState.m_uiOpenReadCount = msRefMapState.m_uiOpenReadCount - 1;
			}
			else
			{
				if (msRefMapState.m_bWriteOpened)
				{
					PrintError("Unexpected read lock in pixel %u of bitmap line %zu\n", uiCurrentPixelCount, siLineIndex);

					std::copy(g_auiInvalidReadPixel, g_auiInvalidReadPixel + ARRAY_SIZE(g_auiInvalidReadPixel), auiDestinationLine[uiCurrentPixelCount]);
				}
				else if (msRefMapState.m_uiOpenReadCount > 0)
				{
					unsigned uiThreadIndex = (MAKE_ALPHA_UNICASE(cFirstChar) - MAKE_ALPHA_UNICASE(FIRST_READ_MAP_CHAR)) * 16 + (IN_RANGE(cSecondChar, '0', '9' + 1) ? cSecondChar - '0' : MAKE_ALPHA_UNICASE(cSecondChar) - MAKE_ALPHA_UNICASE('A') + 10);
					unsigned uiColorValue = MAKE_COLOR_COMPONENT(uiThreadIndex);
					for (unsigned uiColorIndex = 0; uiColorIndex != ARRAY_SIZE(g_auiSharedReadPixelStep); ++uiColorIndex)
					{
						auiDestinationLine[uiCurrentPixelCount][uiColorIndex] = g_auiSharedReadPixelStep[uiColorIndex] != 0 ? uiColorValue : 0;
					}
				}
				else
				{
					unsigned uiThreadIndex = (MAKE_ALPHA_UNICASE(cFirstChar) - MAKE_ALPHA_UNICASE(FIRST_READ_MAP_CHAR)) * 16 + (IN_RANGE(cSecondChar, '0', '9' + 1) ? cSecondChar - '0' : MAKE_ALPHA_UNICASE(cSecondChar) - MAKE_ALPHA_UNICASE('A') + 10);
					unsigned uiColorValue = MAKE_COLOR_COMPONENT(uiThreadIndex);
					for (unsigned uiColorIndex = 0; uiColorIndex != ARRAY_SIZE(g_auiSingleReadPixelStep); ++uiColorIndex)
					{
						auiDestinationLine[uiCurrentPixelCount][uiColorIndex] = g_auiSingleReadPixelStep[uiColorIndex] != 0 ? uiColorValue : 0;
					}
				}

				++msRefMapState.m_uiOpenReadCount;
			}
		}

		szSourceLine += 3;
		++uiCurrentPixelCount;
	}

	if (!bAnyFault)
	{
		uiVarGeneratedPixelCount = uiCurrentPixelCount;
	}

	bool bResult = !bAnyFault;
	return bResult;
}

/*static */
bool CBMPGenProgram::WriteDestinationHeaders(CConversionContext &ccRefWorkContext, sizeint siBitmapLines)
{
	bool bResult = false;
	
	do
	{
		if (fseek(ccRefWorkContext.m_psfDestinationFile, 0, SEEK_SET) != 0)
		{
			PrintError("Failed to rewind destination file\n");
			break;
		}

		BITMAPFILEHEADER fhFileHeader;
		BITMAPINFOHEADER ihInfoHeader;

		uint32_t siDataOffset = sizeof(fhFileHeader) + sizeof(ihInfoHeader);
		uint32_t siDataSize = (uint32_t)(siBitmapLines * DESTINATIONLINE_WIDTH * DESTINATION_BYTES_PER_PIXEL);
		assert(DESTINATIONLINE_WIDTH * DESTINATION_BYTES_PER_PIXEL % BITMAP_LINE_ALIGNMENT == 0);

		memset(&fhFileHeader, 0, sizeof(fhFileHeader));
		fhFileHeader.bfType = 0x4D42;
		fhFileHeader.bfSize = siDataOffset + siDataSize;
		fhFileHeader.bfOffBits = siDataOffset;

		memset(&ihInfoHeader, 0, sizeof(ihInfoHeader));
		ihInfoHeader.biSize = sizeof(ihInfoHeader);
		ihInfoHeader.biWidth = DESTINATIONLINE_WIDTH;
		ihInfoHeader.biHeight = (uint32_t)siBitmapLines; ihInfoHeader.biHeight = -ihInfoHeader.biHeight;
		ihInfoHeader.biPlanes = 1;
		ihInfoHeader.biBitCount = 24;
		ihInfoHeader.biCompression = BI_RGB;

		sizeint siFileHeaderBytesWritten = fwrite(&fhFileHeader, 1, sizeof(fhFileHeader), ccRefWorkContext.m_psfDestinationFile);
		if (siFileHeaderBytesWritten != sizeof(fhFileHeader))
		{
			PrintError("Failed to write bitmap file header\n");
			break;
		}

		sizeint siInfoHeaderBytesWritten = fwrite(&ihInfoHeader, 1, sizeof(ihInfoHeader), ccRefWorkContext.m_psfDestinationFile);
		if (siInfoHeaderBytesWritten != sizeof(ihInfoHeader))
		{
			PrintError("Failed to write bitmap info header\n");
			break;
		}

		bResult = true;
	}
	while (false);
	
	return bResult;
}


/*static */
void CBMPGenProgram::PrintError(const char *szErrorText, ...)
{
	va_list vaArguments;
	va_start(vaArguments, szErrorText);
	vfprintf(stderr, szErrorText, vaArguments);
	va_end(vaArguments);
}


int main(int iArgumentCount, char *apszArgumentValues[])
{
	return CBMPGenProgram::Execute(iArgumentCount, apszArgumentValues) ? 0 : 1;
}
