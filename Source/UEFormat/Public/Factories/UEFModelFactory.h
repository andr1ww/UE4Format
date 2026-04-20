#pragma once
#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "Readers/UEFModelReader.h"
#include "Engine/StaticMesh.h"
#include "UEFModelFactory.generated.h"

UCLASS(hidecategories = Object)
class UE4FORMAT_API UEFModelFactory : public UFactory
{
	GENERATED_UCLASS_BODY()

	virtual UObject *FactoryCreateFile(
		UClass *Class,
		UObject *Parent,
		FName Name,
		EObjectFlags Flags,
		const FString &Filename,
		const TCHAR *Params,
		FFeedbackContext *Warn,
		bool &bOutOperationCanceled) override;

	void PopulateMeshDescription(FMeshDescription &MeshDesc, FLODData &Data);
	void SetMeshAttributes(FMeshDescription &MeshDesc, FLODData &Data);
	void CreatePolygonGroups(FMeshDescription &MeshDesc, FLODData &Data);

	UStaticMesh *CreateStaticMesh(TArray<FLODData> &LODData, UObject *Parent, FName Name, EObjectFlags Flags);
};