#include "Factories/UEFModelFactory.h"
#include "StaticMeshAttributes.h"
#include "Engine/StaticMesh.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "IContentBrowserSingleton.h"
#include "ContentBrowserModule.h"

UEFModelFactory::UEFModelFactory(const FObjectInitializer &ObjectInitializer)
	: Super(ObjectInitializer)
{
	Formats.Add(TEXT("uemodel; UEMODEL Mesh File"));
	SupportedClass = UStaticMesh::StaticClass();
	bCreateNew = false;
	bEditorImport = true;
}

UObject* UEFModelFactory::FactoryCreateFile(
	UClass* Class,
	UObject* Parent,
	FName Name,
	EObjectFlags Flags,
	const FString& Filename,
	const TCHAR* Params,
	FFeedbackContext* Warn,
	bool& bOutOperationCanceled)
{
	UEFModelReader Data(Filename);
	if (!Data.Read() || Data.LODs.Num() == 0)
		return nullptr;

	int32 ContentIdx = Filename.Replace(TEXT("\\"), TEXT("/")).Find(TEXT("/Content/"), ESearchCase::IgnoreCase);

	UPackage* Package = CreatePackage(*FString::Printf(TEXT("/Game/%s/%s"),
		ContentIdx != INDEX_NONE
		? *FPaths::GetPath(Filename.Replace(TEXT("\\"), TEXT("/")).Mid(ContentIdx + 9))
		: *FPackageName::GetLongPackagePath(Parent->GetOutermost()->GetName()),
		*Name.ToString()));

	Package->FullyLoad();

	UStaticMesh* Mesh = CreateStaticMesh(Data.LODs, Package, Name, Flags | RF_Public | RF_Standalone);
	Mesh->PostEditChange();
	FAssetRegistryModule::AssetCreated(Mesh);
	FContentBrowserModule& ContentBrowser = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	ContentBrowser.Get().SyncBrowserToAssets({ FAssetData(Mesh) });
	Package->MarkPackageDirty();

	return Mesh;
}

void UEFModelFactory::PopulateMeshDescription(FMeshDescription &MeshDesc, FLODData &Data)
{
	for (int32 i = 0; i < Data.Vertices.Num(); i++)
		MeshDesc.CreateVertex();

	for (int32 i = 0; i < Data.Indices.Num(); i++)
		MeshDesc.CreateVertexInstance(FVertexID(Data.Indices[i]));
}

void UEFModelFactory::SetMeshAttributes(FMeshDescription &MeshDesc, FLODData &Data)
{
	FStaticMeshAttributes Attributes(MeshDesc);
	Attributes.Register();

	auto Positions = Attributes.GetVertexPositions();

	for (int32 i = 0; i < Data.Vertices.Num(); i++)
		Positions[FVertexID(i)] = Data.Vertices[i];
}

void UEFModelFactory::CreatePolygonGroups(FMeshDescription &MeshDesc, FLODData &Data)
{
	FPolygonGroupID Group = MeshDesc.CreatePolygonGroup();

	for (int32 i = 0; i < Data.Indices.Num(); i += 3)
	{
		MeshDesc.CreatePolygon(Group, {FVertexInstanceID(i),
									   FVertexInstanceID(i + 1),
									   FVertexInstanceID(i + 2)});
	}
}

UStaticMesh* UEFModelFactory::CreateStaticMesh(
	TArray<FLODData>& LODData,
	UObject* Parent,
	FName Name,
	EObjectFlags Flags)
{
	UStaticMesh* Mesh = NewObject<UStaticMesh>(Parent, Name, Flags);

	TArray<FMeshDescription> MeshDescriptions;
	TArray<const FMeshDescription*> MeshDescPtrs;

	MeshDescriptions.SetNum(LODData.Num());
	MeshDescPtrs.SetNum(LODData.Num());

	for (int32 i = 0; i < LODData.Num(); i++)
	{
		PopulateMeshDescription(MeshDescriptions[i], LODData[i]);
		SetMeshAttributes(MeshDescriptions[i], LODData[i]);
		CreatePolygonGroups(MeshDescriptions[i], LODData[i]);

		MeshDescPtrs[i] = &MeshDescriptions[i];
	}

	Mesh->BuildFromMeshDescriptions(MeshDescPtrs);
	Mesh->PostEditChange();

	return Mesh;
}