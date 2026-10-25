#pragma once

#include "CoreMinimal.h"

/** Helpers to move rotations between world space and "gravity space" (where gravity points straight down). */
namespace HexGravity
{
	inline FRotator ToGravityRelative(const FRotator& Rotation, const FVector& GravityDirection)
	{
		if (GravityDirection.Equals(FVector::DownVector)) return Rotation;
		return (FQuat::FindBetweenNormals(GravityDirection, FVector::DownVector) * Rotation.Quaternion()).Rotator();
	}

	inline FRotator ToGravityWorld(const FRotator& Rotation, const FVector& GravityDirection)
	{
		if (GravityDirection.Equals(FVector::DownVector)) return Rotation;
		return (FQuat::FindBetweenNormals(FVector::DownVector, GravityDirection) * Rotation.Quaternion()).Rotator();
	}
}
