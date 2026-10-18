#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SneakAnimationEditorLibrary.generated.h"
class UAnimSequence;
class USkeleton;
class UBlendSpace;
class UAnimBlueprint;

/** Editor-only asset setup used by Scripts/InstallSneakAnimations.py. */
UCLASS()
class USneakAnimationEditorLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, meta=(DevelopmentOnly), Category="Development|Animation")
    static bool SetMatchingSkeleton(UAnimSequence* Sequence, USkeleton* Skeleton);
    UFUNCTION(BlueprintCallable, meta=(DevelopmentOnly), Category="Development|Animation")
    static bool ConfigureSneakBlendSpace(UBlendSpace* BlendSpace, const TArray<UAnimSequence*>& Clips);
    UFUNCTION(BlueprintCallable, meta=(DevelopmentOnly), Category="Development|Animation")
    static bool InstallSneakGraph(UAnimBlueprint* Blueprint, UBlendSpace* BlendSpace);
};
