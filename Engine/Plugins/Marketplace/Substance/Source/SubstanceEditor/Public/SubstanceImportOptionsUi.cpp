// Copyright 2018 Allegorithmic Inc. All rights reserved.
// File: SubstanceImportOptionsUI.cpp

#include "SubstanceEditorPrivatePCH.h"
#include "SubstanceImportOptionsUi.h"

USubstanceImportOptionsUi::USubstanceImportOptionsUi(const class FObjectInitializer& PCIP)
	: Super(PCIP)
{
	bOverrideFullName = false;
	bCreateInstance = true;
	bCreateMaterial = true;
}