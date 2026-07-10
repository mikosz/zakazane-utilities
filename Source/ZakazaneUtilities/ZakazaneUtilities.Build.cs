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
				"SlateCore",
				"ImGui",
				"Json",
				"JsonUtilities"
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

		// By default, optional inspections are performed on non-shipping and non-test builds.
		// If you want to override this behaviour use FORCE_XYZ_INSPECTIONS set to 0 (to force disable)
		// or 1 (to force enable).
		// PublicDefinitions.Add("FORCE_EXECUTION_GRAPH_INSPECTIONS=0");
		// PublicDefinitions.Add("FORCE_BUDGETING_INSPECTIONS=0");
	}

	private void AddUseEngineVersionDef(int MajorVersion, int MinorVersion)
	{
		var isGivenVersionOrOver = Target.Version.MajorVersion > MajorVersion ||
		                           (Target.Version.MajorVersion == MajorVersion &&
		                            Target.Version.MinorVersion >= MinorVersion);

		// Must remain public due to being used in header file
		PublicDefinitions.Add(string.Format("ZAKAZANE_UTILITIES_USE_{0}_{1}={2}", MajorVersion, MinorVersion,
			isGivenVersionOrOver ? 1 : 0));
	}
}