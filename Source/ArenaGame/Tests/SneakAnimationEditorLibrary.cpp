#include "SneakAnimationEditorLibrary.h"
#include "Animation/AnimSequence.h"
#include "Animation/BlendSpace.h"
#include "Animation/AnimBlueprint.h"
#include "SneakAnimInstance.h"

#if WITH_EDITOR
#include "AnimationGraph.h"
#include "AnimGraphNode_Root.h"
#include "AnimGraphNode_TwoWayBlend.h"
#include "AnimGraphNode_BlendSpacePlayer.h"
#include "K2Node_VariableGet.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "EdGraph/EdGraphNodeUtils.h"
#endif

bool USneakAnimationEditorLibrary::SetMatchingSkeleton(UAnimSequence* Sequence, USkeleton* Skeleton)
{
#if WITH_EDITOR
    if (!Sequence || !Skeleton) return false;
    Sequence->Modify();
    Sequence->SetPreviewMesh(nullptr);
    Sequence->ClearRetargetSourceAsset();
    return Sequence->ReplaceSkeleton(Skeleton, false);
#else
    return false;
#endif
}

bool USneakAnimationEditorLibrary::ConfigureSneakBlendSpace(UBlendSpace* Space, const TArray<UAnimSequence*>& Clips)
{
#if WITH_EDITOR
    if (!Space || Clips.Num() != 5 || Space->GetNumberOfBlendSamples() != 0) return false;
    for (const UAnimSequence* Clip : Clips)
        if (!Clip || Clip->GetSkeleton() != Space->GetSkeleton()) return false;
    Space->Modify();
    auto& Direction = const_cast<FBlendParameter&>(Space->GetBlendParameter(0));
    Direction.DisplayName = TEXT("Direction"); Direction.Min = -180.f; Direction.Max = 180.f; Direction.GridNum = 4;
    Direction.bWrapInput = true;
    auto& Speed = const_cast<FBlendParameter&>(Space->GetBlendParameter(1));
    Speed.DisplayName = TEXT("Speed"); Speed.Min = 0.f; Speed.Max = 200.f; Speed.GridNum = 4;
    // Clips: idle, forward, backward, left, right. Duplicate endpoints wrap backward movement.
    const int32 WalkIndices[] = {2, 3, 1, 4, 2};
    for (int32 Index = 0; Index < 5; ++Index)
    {
        const float Angle = -180.f + 90.f * Index;
        Space->AddSample(Clips[0], FVector(Angle, 0.f, 0.f));
        Space->AddSample(Clips[WalkIndices[Index]], FVector(Angle, 200.f, 0.f));
    }
    Space->ValidateSampleData();
    Space->ResampleData();
    Space->PostEditChange();
    return true;
#else
    return false;
#endif
}

bool USneakAnimationEditorLibrary::InstallSneakGraph(UAnimBlueprint* Blueprint, UBlendSpace* Space)
{
#if WITH_EDITOR
    if (!Blueprint || !Space || Blueprint->TargetSkeleton != Space->GetSkeleton()) return false;
    if (Blueprint->ParentClass == UAnimInstance::StaticClass())
    {
        Blueprint->Modify();
        Blueprint->ParentClass = USneakAnimInstance::StaticClass();
        FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
        FKismetEditorUtilities::CompileBlueprint(Blueprint);
        if (Blueprint->Status == BS_Error) return false;
    }
    if (Blueprint->ParentClass != USneakAnimInstance::StaticClass()) return false;
    TArray<UEdGraph*> Graphs;
    Blueprint->GetAllGraphs(Graphs);
    UAnimGraphNode_Root* Root = nullptr;
    for (UEdGraph* Graph : Graphs)
        if (Graph->GetFName() == TEXT("AnimGraph"))
            for (UEdGraphNode* Node : Graph->Nodes)
            {
                if (Node->NodeComment == TEXT("ArenaSneakBlend")) return true;
                if (auto* Candidate = Cast<UAnimGraphNode_Root>(Node)) Root = Candidate;
            }
    if (!Root) return false;
    UEdGraphPin* Result = Root->FindPin(TEXT("Result"));
    if (!Result || Result->LinkedTo.Num() != 1) return false;
    // Keep the existing foot IK after the crouch blend.
    UEdGraphNode* LastNode = Result->LinkedTo[0]->GetOwningNode();
    if (LastNode->GetClass()->GetFName() == TEXT("AnimGraphNode_ControlRig"))
        if (UEdGraphPin* RigInput = LastNode->FindPin(TEXT("Source")); RigInput && RigInput->LinkedTo.Num() == 1)
            Result = RigInput;
    UEdGraphPin* OriginalPose = Result->LinkedTo[0];
    UEdGraph* Graph = Root->GetGraph();
    const UEdGraphSchema* Schema = Graph->GetSchema();
    Blueprint->Modify(); Graph->Modify();

    FGraphNodeCreator<UAnimGraphNode_TwoWayBlend> BlendCreator(*Graph);
    auto* Blend = BlendCreator.CreateNode();
    Blend->NodeComment = TEXT("ArenaSneakBlend");
    Blend->NodePosX = Root->NodePosX - 220; Blend->NodePosY = Root->NodePosY;
    BlendCreator.Finalize();
    FGraphNodeCreator<UAnimGraphNode_BlendSpacePlayer> PlayerCreator(*Graph);
    auto* Player = PlayerCreator.CreateNode();
    Player->SetAnimationAsset(Space);
    Player->NodePosX = Blend->NodePosX - 300; Player->NodePosY = Blend->NodePosY + 220;
    PlayerCreator.Finalize();

    auto Connect = [&](UEdGraphPin* From, UEdGraphPin* To)
    {
        if (!From || !To)
        {
            UE_LOG(LogTemp, Error, TEXT("Sneak graph missing pin: %s -> %s"), From ? *From->PinName.ToString() : TEXT("null"), To ? *To->PinName.ToString() : TEXT("null"));
            return false;
        }
        return Schema->TryCreateConnection(From, To);
    };

    auto ConnectInput = [&](const TCHAR* Property, UEdGraphNode* Destination, const TCHAR* PinName)
    {
        FGraphNodeCreator<UK2Node_VariableGet> Creator(*Graph);
        auto* Getter = Creator.CreateNode();
        Getter->VariableReference.SetSelfMember(Property);
        Getter->NodePosX = Destination->NodePosX - 240;
        Getter->NodePosY = Destination->NodePosY + 100;
        Creator.Finalize();
        return Connect(Getter->GetValuePin(), Destination->FindPin(PinName));
    };
    Result->BreakAllPinLinks();
    const bool bConnected = Connect(OriginalPose, Blend->FindPin(TEXT("A")))
        && Connect(Player->FindPin(TEXT("Pose")), Blend->FindPin(TEXT("B")))
        && Connect(Blend->FindPin(TEXT("Pose")), Result)
        && ConnectInput(TEXT("SneakBlendAlpha"), Blend, TEXT("Alpha"))
        && ConnectInput(TEXT("SneakDirection"), Player, TEXT("X"))
        && ConnectInput(TEXT("SneakMoveSpeed"), Player, TEXT("Y"));
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
    FKismetEditorUtilities::CompileBlueprint(Blueprint);
    return bConnected && Blueprint->Status != BS_Error;
#else
    return false;
#endif
}
