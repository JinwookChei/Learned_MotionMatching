// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class LearnedMM : ModuleRules
{
	public LearnedMM(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput" });
	}
}
