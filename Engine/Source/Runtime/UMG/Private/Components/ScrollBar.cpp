// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

#define LOCTEXT_NAMESPACE "UMG"

/////////////////////////////////////////////////////
// UScrollBar

UScrollBar::UScrollBar(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bIsVariable = false;

	bAlwaysShowScrollbar = true;
	Orientation = Orient_Vertical;
	Thickness = FVector2D(12.0f, 12.0f);
}

TSharedRef<SWidget> UScrollBar::RebuildWidget()
{
	const FScrollBarStyle* StylePtr = ( Style != NULL ) ? Style->GetStyle<FScrollBarStyle>() : NULL;
	if ( StylePtr == NULL )
	{
		SScrollBar::FArguments Defaults;
		StylePtr = Defaults._Style;
	}

	MyScrollBar = SNew(SScrollBar)
		.Style(StylePtr)
		.AlwaysShowScrollbar(bAlwaysShowScrollbar)
		.Orientation(Orientation)
		.Thickness(Thickness);

	//SLATE_EVENT(FOnUserScrolled, OnUserScrolled)

	return MyScrollBar.ToSharedRef();
}

void UScrollBar::SyncronizeProperties()
{
	//MyScrollBar->SetScrollOffset(DesiredScrollOffset);
}

void UScrollBar::SetState(float InOffsetFraction, float InThumbSizeFraction)
{
	if ( MyScrollBar.IsValid() )
	{
		MyScrollBar->SetState(InOffsetFraction, InThumbSizeFraction);
	}
}

#if WITH_EDITOR

const FSlateBrush* UScrollBar::GetEditorIcon()
{
	return FUMGStyle::Get().GetBrush("Widget.ScrollBar");
}

#endif

/////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
