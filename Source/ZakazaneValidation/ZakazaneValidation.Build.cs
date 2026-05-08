using UnrealBuildTool;

public class ZakazaneValidation : ModuleRules
{
	public ZakazaneValidation(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		bWarningsAsErrors = true;

		PublicDependencyModuleNames.AddRange(
			new[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"DeveloperSettings",
				"UnrealEd",
				"DataValidation",
				"AssetRegistry",
				"DeveloperToolSettings",
				"GameplayTags"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new[]
			{
				"Slate",
				"SlateCore",
				"ZakazaneUtilities",
				"ZakazaneUtilitiesEditor",
				"Blutility",
				"UnrealEd",
				"SourceControl"
			}
		);
	}
}