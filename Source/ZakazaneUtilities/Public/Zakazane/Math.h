// Copyright ZAKAZANE Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "Math.generated.h"

UENUM(BlueprintType)
enum class EZkzComparisonType : uint8
{
	Equals,
	NotEquals,
	Less,
	LessEquals,
	Greater,
	GreaterEquals
};

namespace Zkz::Math
{

/// Creates a translation vector between two points.
inline FVector MakeFromToVector(const FVector& From, const FVector& To)
{
	return To - From;
}

/// Similar to FVector::GetAbsMax but while GetAbsMax returns the maximum absolute component value, this returns
/// the maximum absolute component's index. If two absolute components are equal, returns the smaller index.
/// E.g.
/// @code
/// GetVectorAbsMaxComponentIndex(<1, 5, 2>) == 1
/// GetVectorAbsMaxComponentIndex(<1, -5, 2>) == 1
/// GetVectorAbsMaxComponentIndex(<-3, 3, 2>) == 0
/// @endcode
inline int32 GetVectorAbsMaxComponentIndex(const FVector& Vector)
{
	const FVector::FReal AbsX = FMath::Abs(Vector.X);
	const FVector::FReal AbsY = FMath::Abs(Vector.Y);
	const FVector::FReal AbsZ = FMath::Abs(Vector.Z);

	if (AbsX >= AbsY && AbsX >= AbsZ)
	{
		return 0;
	}

	if (AbsY >= AbsZ)
	{
		return 1;
	}

	return 2;
}

/// Similar to FVector::SetComponentForAxis but takes a component index instead of EAxis. Checks that ComponentIndex is
/// a correct index, i.e. 0, 1 or 2.
/// E.g.
/// VectorSetComponent(<0, 1, 2>, 1, 3) changes vector to <0, 3, 1>.
inline void VectorSetComponent(FVector& Vector, const int32 ComponentIndex, const FVector::FReal ComponentValue)
{
	check(ComponentIndex >= 0 && ComponentIndex < 3);

	if (ComponentIndex == 0)
	{
		Vector.X = ComponentValue;
	}
	else if (ComponentIndex == 1)
	{
		Vector.Y = ComponentValue;
	}
	else if (ComponentIndex == 2)
	{
		Vector.Z = ComponentValue;
	}
}

/// Makes a new vector out of another vector and sets its component at ComponentIndex to ComponentValue.
/// E.g.
/// MakeVectorSetComponent(<0, 1, 2>, 1, 3) returns vector <0, 3, 1>
inline [[nodiscard]] FVector MakeVectorSetComponent(
	const FVector& Vector, const int32 ComponentIndex, const FVector::FReal ComponentValue)
{
	FVector Result = Vector;
	VectorSetComponent(Result, ComponentIndex, ComponentValue);
	return Result;
}

/// Similar to MakeVectorSetComponent(Vector, ComponentIndex, ComponentValue) but uses a zero vector.
/// E.g.
/// MakeVectorSetComponent(1, 3) returns vector <0, 3, 0>
inline [[nodiscard]] FVector MakeVectorSetComponent(const int32 ComponentIndex, const FVector::FReal ComponentValue)
{
	return MakeVectorSetComponent(FVector::ZeroVector, ComponentIndex, ComponentValue);
}

/// Projects the given Vector on the XY plane, effectively zeroing the Z component. Result is not normalized.
inline FVector ProjectOntoXY(const FVector& Vector)
{
	return MakeVectorSetComponent(Vector, 2, 0.0);
}

inline float AngleBetweenVectorsRad(const FVector& Vec1, const FVector& Vec2)
{
	const float AngleRad = FMath::Acos(FVector::DotProduct(Vec1.GetSafeNormal(), Vec2.GetSafeNormal()));
	return AngleRad;
}

inline float AngleBetweenVectorsDeg(const FVector& Vec1, const FVector& Vec2)
{
	const float AngleRad = AngleBetweenVectorsRad(Vec1, Vec2);
	const float AngleDeg = FMath::RadiansToDegrees(AngleRad);
	return AngleDeg;
}

inline float SignedAngleBetweenVectorsRad(const FVector& Vec1, const FVector& Vec2)
{
	const float AngleRad = AngleBetweenVectorsRad(Vec1, Vec2);
	const float Sign = FVector::CrossProduct(Vec1, Vec2).Z >= 0 ? 1.0f : -1.0f;

	const float SignedAngleRad = AngleRad * Sign;
	return SignedAngleRad;
}

inline float SignedAngleBetweenVectorsDeg(const FVector& Vec1, const FVector& Vec2)
{
	const float SignedAngleDeg = FMath::RadiansToDegrees(SignedAngleBetweenVectorsRad(Vec1, Vec2));
	return SignedAngleDeg;
}

/// Template version of UE_SMALL_NUMBER / UE_DOUBLE_SMALL_NUMBER.
template <class T UE_REQUIRES(TIsFloatingPoint<T>::Value)>
constexpr T SmallNumber;

template <>
constexpr float SmallNumber<float> = UE_SMALL_NUMBER;

template <>
constexpr float SmallNumber<double> = UE_DOUBLE_SMALL_NUMBER;

/// Constexpr version of FMath::Abs
template <class T UE_REQUIRES(TIsArithmetic<T>::Value)>
constexpr T Abs(const T Value)
{
	if constexpr (TIsFloatingPoint<T>::Value || TIsSigned<T>::Value)
	{
		return (Value < static_cast<T>(0)) ? -Value : Value;
	}
	else
	{
		return Value;
	}
}

static_assert(Abs(-3) == 3);
static_assert(Abs(3) == 3);
static_assert(Abs(2u) == 2u);
static_assert(Abs(-2.0f) > 1.9f && Abs(-2.0f) < 2.1f);

/// Constexpr and template version of FMath::IsNearlyEqual
template <class T UE_REQUIRES(TIsFloatingPoint<T>::Value)>
constexpr bool IsNearlyEqual(const T A, const T B, const T ErrorTolerance = SmallNumber<T>)
{
	return Abs(A - B) <= ErrorTolerance;
}

/// Extension of FMath::IsNearlyEqual to integral numbers.
template <class T UE_REQUIRES(TIsIntegral<T>::Value)>
constexpr bool IsNearlyEqual(const T A, const T B, const T ErrorTolerance)
{
	return (A < B) ? (B - A <= ErrorTolerance) : (A - B <= ErrorTolerance);
}

static_assert(IsNearlyEqual(2.1f, 2.2f, 0.15f));
static_assert(!IsNearlyEqual(2.1f, 2.2f, 0.05f));
static_assert(IsNearlyEqual(2, 3, 2));
static_assert(!IsNearlyEqual(2, 3, 0));
static_assert(!IsNearlyEqual(2, 4, 1));

/// Returns SnapValue if the InputValue is within Threshold of it. Otherwise, returns InputValue.
/// @code
/// SnapIfNear(-2, 0, 1) == -2
/// SnapIfNear(-1, 0, 1) == 0
/// SnapIfNear(0, 0, 1) == 0
/// SnapIfNear(1, 0, 1) == 0
/// SnapIfNear(2, 0, 1) == 2
/// @endcode
template <class T UE_REQUIRES(TIsArithmetic<T>::Value)>
constexpr T SnapIfNear(T InputValue, T SnapValue, T Threshold)
{
	if (IsNearlyEqual(SnapValue, InputValue, Threshold))
	{
		return SnapValue;
	}

	return InputValue;
}

static_assert(SnapIfNear(-2, 0, 1) == -2);
static_assert(SnapIfNear(-1, 0, 1) == 0);
static_assert(SnapIfNear(0, 0, 1) == 0);
static_assert(SnapIfNear(1, 0, 1) == 0);
static_assert(SnapIfNear(2, 0, 1) == 2);

/// Returns A or B depending on whether InputValue is closer to A or B.
/// @code
/// Snap(3, 0, 10) == 0
/// Snap(6, 0, 10) == 10
/// Snap(-2, 0, 10) == 0
/// Snap(12, 0, 10) == 10
/// @endcode
template <class T UE_REQUIRES(TIsArithmetic<T>::Value)>
constexpr T Snap(T InputValue, T A, T B)
{
	const float DistanceA = FMath::Abs(A - InputValue);
	const float DistanceB = FMath::Abs(B - InputValue);

	return DistanceA < DistanceB ? A : B;
}

static_assert(Snap(3, 0, 10) == 0);
static_assert(Snap(6, 0, 10) == 10);
static_assert(Snap(-2, 0, 10) == 0);
static_assert(Snap(12, 0, 10) == 10);

/// Same as SnapIfNear for arithmetic types, but for vectors.
inline FVector SnapIfNear(const FVector& InputPoint, const FVector& SnapPoint, const double Threshold)
{
	if (FVector::DistSquared(SnapPoint, InputPoint) <= FMath::Square(Threshold))
	{
		return SnapPoint;
	}

	return InputPoint;
}

/// Takes a path represented as an array of points and shortens it by a given distance.
/// @param ShortenBy total length removed from the path
/// @param bReversed if true, removes elements / shortens segments at the start of the path instead of the end
[[nodiscard]] ZAKAZANEUTILITIES_API TArray<FVector> GetShortenedPath(
	TArrayView<const FVector> Path, float ShortenBy, const bool bReversed = false);

}  // namespace Zkz::Math

// Calculate the yaw rotation for a vector and return it as a Rotator
inline FRotator GetYawRotationForVector(const FVector& Vector)
{
	FVector NormalizedVector = Vector.GetSafeNormal();
	float Yaw = FMath::RadiansToDegrees(FMath::Atan2(NormalizedVector.Y, NormalizedVector.X));
	return FRotator(0, Yaw, 0);
}