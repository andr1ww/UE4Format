// Copyright Epic Games, Inc. All Rights Reserved.
#include "../Public/UE4Format.h"

void FUE4FormatModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
}

void FUE4FormatModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

IMPLEMENT_MODULE(FUE4FormatModule, UE4Format)