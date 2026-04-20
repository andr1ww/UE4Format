// Copyright © 2025 Marcel K. All rights reserved.

#pragma once
#include <fstream>
#include <vector>
#include "CoreMinimal.h"
#include "Math/Quat.h"
#include "Math/Color.h"
#include "Containers/Array.h"

template <typename T>
T ReadData(std::ifstream &Ar)
{
    T Data;
    Ar.read(reinterpret_cast<char *>(&Data), sizeof(T));
    return Data;
}

std::string ReadString(std::ifstream &Ar, int32 Size);
std::string ReadFString(std::ifstream &Ar);

template <typename T>
T ReadBufferData(const char *DataArray, int &Offset)
{
    T Data;
    std::memcpy(&Data, &DataArray[Offset], sizeof(T));
    Offset += sizeof(T);
    return Data;
}

template <typename T>
void ReadBufferArray(const char *DataArray, int &Offset, int32 ArraySize, TArray<T> &Data)
{
    Data.SetNum(ArraySize);
    for (int32 i = 0; i < ArraySize; i++)
    {
        std::memcpy(&Data[i], &DataArray[Offset], sizeof(T));
        Offset += sizeof(T);
    }
}

FQuat ReadBufferQuat(const char *DataArray, int &Offset);
std::string ReadBufferString(const char *DataArray, int &Offset, int32 Size);
std::string ReadBufferFString(const char *DataArray, int &Offset);

struct FVertexColorChunk
{
    std::string Name;
    int32 Count;
    TArray<FColor> Data;
};

struct FMaterialChunk
{
    std::string Name;
    std::string Path;
    int32 FirstIndex;
    int32 NumFaces;
};

struct FLODData
{
    TArray<FVector> Vertices;
    TArray<int32> Indices;
    TArray<FVector4> Normals;
    TArray<FVector> Tangents;
    TArray<FVertexColorChunk> VertexColors;
    TArray<TArray<FVector2D>> TextureCoordinates;
    TArray<FMaterialChunk> Materials;
};

struct FUE4FormatHeader
{
    std::string Identifier;
    uint8 FileVersionBytes;
    std::string ObjectName;
    bool IsCompressed;
    std::string CompressionType;
    int32 CompressedSize;
    int32 UncompressedSize;
};

class UE4FORMAT_API UEFModelReader
{
public:
    UEFModelReader(const FString Filename);
    ~UEFModelReader();

    bool Read();

    FUE4FormatHeader Header;
    TArray<FLODData> LODs;

private:
    const std::string GMAGIC = "UEFORMAT";

    std::ifstream Ar;

    void ReadBuffer(const char *Buffer, int32 BufferSize);
    void ReadChunks(const char *Buffer, int &Offset, int32 ByteSize, int32 LODIndex);
};
