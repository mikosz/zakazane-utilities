// Copyright ZAKAZANE Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

namespace Zkz
{

/// Calculates the vertical FOV angle based on the horizontal FOV and the viewport's aspect ratio.
ZAKAZANEUTILITIES_API float CalculateVerticalFOV_Deg(const float HorizontalFOV_Deg, const float AspectRatio);

/// Calculates the _effective_ horizontal and vertical FOV.
///
/// The actual horizontal FOV of the viewport may differ from the one passed to the camera, if the aspect ratio
/// of the window differs from default. This is because when resizing the window, unreal will by default try to
/// keep the vertical, not the horizontal FOV (otherwise resizing the window horizontally would rescale the
/// scene vertically). Effective FOV in this case means the actual FOV taking into account this change.
///
/// @param WantedHorizontalFOV_Deg is the FOV used by the camera
/// @param DefaultAspectRatio is given as a fallback, the function attempts to calculate the aspect ratio from the
/// viewport size.
ZAKAZANEUTILITIES_API TPair<float, float> CalculateEffectiveFOV_Deg(
	const float WantedHorizontalFOV_Deg, const float DefaultAspectRatio);

/// Calculates the tan half of the _effective_ horizontal and vertical FOV.
/// @see CalculateEffectiveFOV
ZAKAZANEUTILITIES_API TPair<float, float> CalculateTanHalfEffectiveFOV(
	const float WantedHorizontalFOV_Deg, const float DefaultAspectRatio);

/// Calculates the angle in degrees of a spherical sector encompassing the camera frustum.
ZAKAZANEUTILITIES_API float CalculateEncompassingSphericalSectorAngle_Deg(
	const float WantedHorizontalFOV_Deg, const float DefaultAspectRatio);

/// Calculates the camera distance required to fit the given box extent completely on the screen.
///
/// @param DefaultAspectRatio is given as a fallback, the function attempts to calculate the aspect ratio from the
/// viewport size.
ZAKAZANEUTILITIES_API float CalculateDistanceToFit_Cm(
	const FVector& BoxExtent_CameraSpace,
	const FVector2D& Padding,
	const float WantedHorizontalFOV_Deg,
	const float DefaultAspectRatio);

}  // namespace Zkz
