#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

#include "LootInventoryWidget.h"
#include "LootCollectorComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/TextureRenderTarget2D.h"
#include "ImageUtils.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Serialization/BufferArchive.h"
#include "TextureResource.h"
#include "StaticMeshCompiler.h"
#include "AssetCompilingManager.h"
#include "ShaderCompiler.h"
#include "LootPickup.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLootThumbnailPreviewTest,
    "ArenaGame.Loot.ThumbnailPreview", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLootThumbnailPreviewTest::RunTest(const FString& Parameters)
{
    FPreviewScene OwnerScene(FPreviewScene::ConstructionValues().SetEditor(false).SetCreatePhysicsScene(false));
    ULootInventoryWidget* Widget = NewObject<ULootInventoryWidget>(OwnerScene.GetWorld());
    Widget->AddToRoot();
    const TCHAR* Assets[] = {
        TEXT("/Engine/BasicShapes/Cube.Cube"),
        TEXT("/Game/Office_Pack_Vol_1/Models/SM_Trash.SM_Trash"),
        TEXT("/Game/Office_Pack_Vol_1/Models/SM_glass_1.SM_glass_1")
    };
    const FString OutputDirectory = FPaths::ProjectSavedDir() / TEXT("Automation/LootThumbnails");
    IFileManager::Get().MakeDirectory(*OutputDirectory, true);
    for (const TCHAR* Asset : Assets)
    {
        FLootEntry Entry;
        Entry.PreviewMesh = LoadObject<UStaticMesh>(nullptr, Asset);
        if (!TestNotNull(Asset, Entry.PreviewMesh.Get())) continue;
        // Match the materials applied to the placed pickup, not just the asset defaults.
        for (const FWorldContext& Context : GEngine->GetWorldContexts())
        {
            if (Context.WorldType != EWorldType::Editor || !Context.World()) continue;
            for (TActorIterator<ALootPickup> Pickup(Context.World()); Pickup; ++Pickup)
            {
                if (Pickup->Mesh->GetStaticMesh() == Entry.PreviewMesh)
                {
                    Entry.PreviewMaterials = Pickup->Mesh->GetMaterials();
                    AddInfo(FString::Printf(TEXT("Using placed pickup materials: %s"), *Pickup->ItemName.ToString()));
                    break;
                }
            }
        }
        UStaticMesh* MeshToCompile = Entry.PreviewMesh.Get();
        FStaticMeshCompilingManager::Get().FinishCompilation(MakeArrayView(&MeshToCompile, 1));
        FAssetCompilingManager::Get().FinishAllCompilation();
        if (GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();
        UTextureRenderTarget2D* Image = Cast<UTextureRenderTarget2D>(Widget->MakeThumbnail(Entry));
        if (!TestNotNull(TEXT("Thumbnail render target"), Image)) continue;
        TestTrue(TEXT("Same mesh reuses its cached portrait"), Widget->MakeThumbnail(Entry) == Image);
        TArray<FColor> Pixels;
        const bool bRead = Image->GameThread_GetRenderTargetResource()->ReadPixels(Pixels);
        TestTrue(TEXT("Rendered thumbnail can be read"), bRead);
        int32 MinBrightness = 255;
        int32 MaxBrightness = 0;
        for (const FColor& Pixel : Pixels)
        {
            const int32 Brightness = FMath::Max3(Pixel.R, Pixel.G, Pixel.B);
            MinBrightness = FMath::Min(MinBrightness, Brightness);
            MaxBrightness = FMath::Max(MaxBrightness, Brightness);
        }
        TestTrue(TEXT("Portrait contains visible shape and contrast"), MaxBrightness - MinBrightness > 30);
        FBufferArchive PNG;
        if (FImageUtils::ExportRenderTarget2DAsPNG(Image, PNG))
        {
            const FString FilePath = OutputDirectory / (Entry.PreviewMesh->GetName() + TEXT(".png"));
            TestTrue(TEXT("Saved portrait for visual review"), FFileHelper::SaveArrayToFile(PNG, *FilePath));
            AddInfo(FilePath);
        }
        else AddError(TEXT("Could not export thumbnail PNG"));
    }
    Widget->ReleasePreviews();
    Widget->RemoveFromRoot();
    return true;
}

#endif
