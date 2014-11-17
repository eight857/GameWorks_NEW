// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "BTTask_PlaySound.generated.h"

class UBehaviorTreeComponent;
class USoundCue;

UCLASS()
class UBTTask_PlaySound : public UBTTaskNode
{
	GENERATED_UCLASS_BODY()

	/** CUE to play */
	UPROPERTY(Category=Node, EditAnywhere)
	USoundCue* SoundToPlay;

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) override;
	virtual FString GetStaticDescription() const override;

#if WITH_EDITOR
	virtual FName GetNodeIconName() const override;
#endif // WITH_EDITOR
};