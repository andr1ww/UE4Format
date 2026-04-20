#include "Readers/UEFModelReader.h"
#include "Misc/Compression.h"
#include "zstd.h"
#include <string>

std::string ReadString(std::ifstream &Ar, int32 Size)
{
    std::string String;
    String.resize(Size);
    Ar.read(&String[0], Size);
    return String;
}

std::string ReadFString(std::ifstream &Ar)
{
    int32 Size = ReadData<int32>(Ar);
    return ReadString(Ar, Size);
}

FQuat ReadBufferQuat(const char *DataArray, int &Offset)
{
    float X = ReadBufferData<float>(DataArray, Offset);
    float Y = ReadBufferData<float>(DataArray, Offset);
    float Z = ReadBufferData<float>(DataArray, Offset);
    float W = ReadBufferData<float>(DataArray, Offset);
    return FQuat(X, Y, Z, W).GetNormalized();
}

std::string ReadBufferFString(const char *DataArray, int &Offset)
{
    int32 Size = ReadBufferData<int32>(DataArray, Offset);
    std::string String;
    String.resize(Size);
    std::memcpy(const_cast<char*>(String.data()), &DataArray[Offset], Size);
    Offset += Size;
    return String;
}

UEFModelReader::UEFModelReader(const FString Filename)
{
    Ar.open(TCHAR_TO_UTF8(*Filename), std::ios::binary);
}

UEFModelReader::~UEFModelReader()
{
    if (Ar.is_open())
        Ar.close();
}

bool UEFModelReader::Read()
{
    std::string Magic = ReadString(Ar, GMAGIC.length());
    if (Magic != GMAGIC) return false;

    Header.Identifier = ReadFString(Ar);
    Header.FileVersionBytes = ReadData<uint8>(Ar);
    Header.ObjectName = ReadFString(Ar);
    Header.IsCompressed = ReadData<bool>(Ar);

    if (Header.IsCompressed) {
        Header.CompressionType = ReadFString(Ar);
        Header.UncompressedSize = ReadData<int32>(Ar);
        Header.CompressedSize = ReadData<int32>(Ar);

        std::vector<char> CompressedBuffer(Header.CompressedSize);
        Ar.read(CompressedBuffer.data(), Header.CompressedSize);
        if (Ar.fail()) {
            UE_LOG(LogTemp, Error, TEXT("Error reading compressed data."));
            return false;
        }

        std::vector<char> UncompressedBuffer(Header.UncompressedSize);

        if (Header.CompressionType == "ZSTD")
            ZSTD_decompress(UncompressedBuffer.data(), Header.UncompressedSize, CompressedBuffer.data(), Header.CompressedSize);

        else if (Header.CompressionType == "GZIP")
            FCompression::UncompressMemory(NAME_Gzip, UncompressedBuffer.data(), Header.UncompressedSize, CompressedBuffer.data(), Header.CompressedSize);

        ReadBuffer(UncompressedBuffer.data(), Header.UncompressedSize);
    }
    else {
        const auto CurrentPos = Ar.tellg();
        Ar.seekg(0, std::ios::end);
        const auto RemainingSize = Ar.tellg() - CurrentPos;
        Ar.seekg(CurrentPos, std::ios::beg);

        std::vector<char> UncompressedBuffer(RemainingSize);
        Ar.read(UncompressedBuffer.data(), RemainingSize);
        if (Ar.fail()) {
            UE_LOG(LogTemp, Error, TEXT("Error reading uncompressed data."));
            return false;
        }

        ReadBuffer(UncompressedBuffer.data(), RemainingSize);
    }

    Ar.close();
    return true;
}

void UEFModelReader::ReadChunks(const char* Buffer, int& Offset, int32 ByteSize, int32 LODIndex)
{
    int32 End = Offset + ByteSize;

    while (Offset < End)
    {
        std::string Chunk = ReadBufferFString(Buffer, Offset);
        int32 Count = ReadBufferData<int32>(Buffer, Offset);
        int32 Size = ReadBufferData<int32>(Buffer, Offset);

        if (Chunk == "VERTICES")
        {
            ReadBufferArray(Buffer, Offset, Count, LODs[LODIndex].Vertices);
        }
        else if (Chunk == "INDICES")
        {
            ReadBufferArray(Buffer, Offset, Count, LODs[LODIndex].Indices);
        }
        else if (Chunk == "NORMALS")
        {
            ReadBufferArray(Buffer, Offset, Count, LODs[LODIndex].Normals);
        }
        else if (Chunk == "TANGENTS")
        {
            ReadBufferArray(Buffer, Offset, Count, LODs[LODIndex].Tangents);
        }
        else if (Chunk == "TEXCOORDS")
        {
            LODs[LODIndex].TextureCoordinates.SetNum(Count);

            for (int32 i = 0; i < Count; i++)
            {
                int32 UVCount = ReadBufferData<int32>(Buffer, Offset);
                LODs[LODIndex].TextureCoordinates[i].SetNum(UVCount);

                for (int32 j = 0; j < UVCount; j++)
                {
                    float U = ReadBufferData<float>(Buffer, Offset);
                    float V = ReadBufferData<float>(Buffer, Offset);

                    LODs[LODIndex].TextureCoordinates[i][j] = FVector2D(U, V);
                }
            }
        }
        else if (Chunk == "MATERIALS")
        {
            LODs[LODIndex].Materials.SetNum(Count);

            for (int32 i = 0; i < Count; i++)
            {
                LODs[LODIndex].Materials[i].Name = ReadBufferFString(Buffer, Offset);
                LODs[LODIndex].Materials[i].Path = ReadBufferFString(Buffer, Offset);
                LODs[LODIndex].Materials[i].FirstIndex = ReadBufferData<int32>(Buffer, Offset);
                LODs[LODIndex].Materials[i].NumFaces = ReadBufferData<int32>(Buffer, Offset);
            }
        }
        else
        {
            Offset += Size;
        }
    }
}

void UEFModelReader::ReadBuffer(const char* Buffer, int32 BufferSize) {
    int32 Offset = 0;

    while (Offset < BufferSize)
    {
        std::string ChunkName = ReadBufferFString(Buffer, Offset);
        int32 ArraySize = ReadBufferData<int32>(Buffer, Offset);
        int32 ByteSize = ReadBufferData<int32>(Buffer, Offset);

        if (ChunkName == "LODS")
        {
            LODs.SetNum(ArraySize);
            for (int32 index = 0; index < ArraySize; ++index) {
                std::string LODName = ReadBufferFString(Buffer, Offset);
                int32 LODByteSize = ReadBufferData<int32>(Buffer, Offset);
                ReadChunks(Buffer, Offset, LODByteSize, index);
            }
        }
        else if (ChunkName == "SKELETON")
            ReadChunks(Buffer, Offset, ByteSize, 0);
        else
            Offset += ByteSize;
    }
}