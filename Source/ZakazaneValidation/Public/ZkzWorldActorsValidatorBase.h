// Copyright ZAKAZANE Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ZkzValidatorBase.h"
#include "ZkzWorldActorsValidatorBase.generated.h"

UCLASS(Abstract)
class ZAKAZANEVALIDATION_API UZkzWorldActorsValidatorBase : public UZkzValidatorBase
{
	GENERATED_BODY()
	
public:
	UZkzWorldActorsValidatorBase();

	// ~ UEditorValidatorBase
	virtual bool CanValidateAsset_Implementation(
		const FAssetData& InAssetData, UObject* InObject, FDataValidationContext& InContext) const override;
	// ~ UEditorValidatorBase
	
protected:
	static void ForEachWorldActor(const UWorld* InWorld, const TFunction<void(const AActor&, const FTransform&)>& InFunction);
};
