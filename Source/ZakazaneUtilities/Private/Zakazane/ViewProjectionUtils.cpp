#include "Zakazane/ViewProjectionUtils.h"

#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Zakazane/ReturnIfMacros.h"

namespace Zkz
{

namespace ViewProjectionUtilsPrivate
{

float CalculateVerticalFOV_Rad_Impl(const float HorizontalFOV_Rad, const float AspectRatio)
{
	return 2.0f * FMath::Atan(HorizontalFOV_Rad / AspectRatio);
}

}  // namespace ViewProjectionUtilsPrivate

float CalculateVerticalFOV_Deg(const float HorizontalFOV_Deg, const float AspectRatio)
{
	using namespace ViewProjectionUtilsPrivate;

	const float TanHalfWantedFOV = FMath::Tan(FMath::DegreesToRadians(HorizontalFOV_Deg) * 0.5f);
	return FMath::RadiansToDegrees(CalculateVerticalFOV_Rad_Impl(TanHalfWantedFOV, AspectRatio));
}

TPair<float, float> CalculateEffectiveFOV_Deg(const float WantedHorizontalFOV_Deg, const float DefaultAspectRatio)
{
	const auto [HorizTanHalfFOV, VertTanHalfFOV] =
		CalculateTanHalfEffectiveFOV(WantedHorizontalFOV_Deg, DefaultAspectRatio);

	return {2.0f * FMath::Atan(HorizTanHalfFOV), 2.0f * FMath::Atan(VertTanHalfFOV)};
}

TPair<float, float> CalculateTanHalfEffectiveFOV(const float WantedHorizontalFOV_Deg, const float DefaultAspectRatio)
{
	using namespace ViewProjectionUtilsPrivate;

	const float TanHalfWantedFOV = FMath::Tan(FMath::DegreesToRadians(WantedHorizontalFOV_Deg) * 0.5f);
	const float DefaultAspectRatioWantedVertFOV_Rad =
		CalculateVerticalFOV_Rad_Impl(TanHalfWantedFOV, DefaultAspectRatio);

	ZKZ_RETURN_IF_INVALID(GEngine, {TanHalfWantedFOV, FMath::Tan(DefaultAspectRatioWantedVertFOV_Rad * 0.5f)});

	FVector2D ViewportSize;
	GEngine->GameViewport->GetViewportSize(ViewportSize);
	const float AspectRatio = (FMath::IsNearlyZero(ViewportSize.Y) || FMath::IsNearlyZero(ViewportSize.X))
								  ? DefaultAspectRatio
								  : (ViewportSize.X / ViewportSize.Y);

	// #TODO #Camera: this should be conditionally done when maintain vertical fov is on (probably), check and if that's
	// true implement a solution for when it's off. At least add an ensure, but it's a really simple implementation,
	// just use TanHalfWantedFOV and calculate vertical fov from actual aspect ratio using CalculateVerticalFOV_Rad
	{
		// If maintaining vertical FOV, use the wanted vertical FOV from DefaultAspectRatio and calculate the resulting
		// horizontal FOV based on that and the current AspectRatio.
		const float TanHalfEffectiveVertFOV = FMath::Tan(DefaultAspectRatioWantedVertFOV_Rad * 0.5f);
		const float TanHalfEffectiveHorizFOV = FMath::Tan(DefaultAspectRatioWantedVertFOV_Rad * 0.5f) * AspectRatio;

		return {TanHalfEffectiveHorizFOV, TanHalfEffectiveVertFOV};
	}
}

float CalculateEncompassingSphericalSectorAngle_Deg(const float WantedHorizontalFOV_Deg, const float DefaultAspectRatio)
{
	ZKZ_RETURN_IF_INVALID_ENSURE(GEngine, WantedHorizontalFOV_Deg);

	FVector2D ViewportSize;
	GEngine->GameViewport->GetViewportSize(ViewportSize);
	ZKZ_RETURN_IF(FMath::IsNearlyZero(ViewportSize.X), WantedHorizontalFOV_Deg);

	const auto [TanHalfEffectiveHorizFOV, TanHalfEffectiveVertFOV] =
		CalculateTanHalfEffectiveFOV(WantedHorizontalFOV_Deg, DefaultAspectRatio);

	const float HalfDiagonalLength = ViewportSize.Length() * 0.5f;

	const float Near_Inv = TanHalfEffectiveHorizFOV / (ViewportSize.X * 0.5f);

	const auto EncompassingSphericalSectorHalfAngle_Rad = FMath::Atan(HalfDiagonalLength * Near_Inv);

	return FMath::RadiansToDegrees(EncompassingSphericalSectorHalfAngle_Rad) * 2.0f;
}

float CalculateDistanceToFit_Cm(
	const FVector& BoxExtent_CameraSpace,
	const FVector2D& Padding,
	const float WantedHorizontalFOV_Deg,
	const float DefaultAspectRatio)
{
	using namespace ViewProjectionUtilsPrivate;

	const auto [TanHalfEffectiveHorizFOV, TanHalfEffectiveVertFOV] =
		CalculateTanHalfEffectiveFOV(WantedHorizontalFOV_Deg, DefaultAspectRatio);

	const float DistanceToFitHoriz_Cm =
		(BoxExtent_CameraSpace.ProjectOnTo(FVector::RightVector).Length() + Padding.X) / TanHalfEffectiveHorizFOV;
	const float DistanceToFitVert_Cm =
		(BoxExtent_CameraSpace.ProjectOnTo(FVector::UpVector).Length() + Padding.Y) / TanHalfEffectiveVertFOV;

	return FMath::Max(DistanceToFitHoriz_Cm, DistanceToFitVert_Cm);
}

}  // namespace Zkz
