// Copyright ZAKAZANE Studio. All Rights Reserved.

using UnrealBuildTool;

public class ZakazaneUtilities : ModuleRules
{
	public ZakazaneUtilities(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		bWarningsAsErrors = true;

		PublicDependencyModuleNames.AddRange(
			new[]
			{
				"AssetRegistry",
				"Core",
				"CoreUObject",
				"Engine"
			}
		);


		PrivateDependencyModuleNames.AddRange(
			new[]
			{
				"Slate",
				"SlateCore"
			}
		);

		if (Target.bBuildEditor)
			PrivateDependencyModuleNames.AddRange(new[]
			{
				"UnrealEd",
				"SubobjectDataInterface",
				"EditorSubsystem",
				"MessageLog"
			});

		AddUseEngineVersionDef(5, 5);
	}

	private void AddUseEngineVersionDef(int MajorVersion, int MinorVersion)
	{
		var isGivenVersionOrOver = Target.Version.MajorVersion > MajorVersion ||
		                           (Target.Version.MajorVersion == MajorVersion &&
		                            Target.Version.MinorVersion >= MinorVersion);

		// Must remain public due to being used in header file
		PublicDefinitions.Add(string.Format("ZAKAZANE_UTILITIES_USE_{0}_{1}={2}", MajorVersion, MinorVersion,
			isGivenVersionOrOver ? 1 : 0));

		// Uncomment this (e.g. conditionally for shipping builds) to skip optional sanity checks for
		// ordered execution.
		//PublicDefinitions.Add("NO_STAGED_EXECUTION_INSPECTION");
	}
}